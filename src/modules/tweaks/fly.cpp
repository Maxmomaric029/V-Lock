#include "fly.h"

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>
#include <verify/typing_check.h>
#include <windows.h>
#include <cmath>

static math::vector3 normalize(const math::vector3& vec)
{
    float length = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
    return (length != 0) ? math::vector3{ vec.x / length, vec.y / length, vec.z / length } : vec;
}

static float length(const math::vector3& vec)
{
    return std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

void fly_function(const math::cframe& cframe, const math::vector3& velocity)
{
    if (!game::local_player.address)
        return;

    rbx::player_t local_player_obj = { game::local_player.address };
    rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
    if (model_instance.address == 0)
        return;

    rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
    if (hrp.address == 0)
        return;

    rbx::part_t part(hrp.address);
    rbx::primitive_t prim = part.get_primitive();
    if (prim.address == 0)
        return;

    memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, cframe.position);
    memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, cframe.rotation);
    memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, velocity);
}

static bool is_key_active(int keybind, int mode, bool& key_was_pressed, bool& toggle_state)
{
    if (keybind == 0) return true;
    if (mode == 2) return true; // Always on

    bool current_state = GetAsyncKeyState(keybind) & 0x8000;
    if (mode == 0) // Hold
    {
        return current_state;
    }
    else if (mode == 1) // Toggle
    {
        if (current_state && !key_was_pressed)
        {
            toggle_state = !toggle_state;
        }
        key_was_pressed = current_state;
        return toggle_state;
    }
    return false;
}

static void set_noclip_recursive(rbx::instance_t instance, bool enabled)
{
    std::string class_name = instance.get_class_name();
    
    // Cover all BasePart types
    if (class_name.find("Part") != std::string::npos || class_name == "TrussPart" || class_name == "CornerWedgePart")
    {
        rbx::part_t part(instance.address);
        rbx::primitive_t prim = part.get_primitive();
        if (prim.address != 0)
        {
            // Aggressive collision disabling
            prim.set_can_collide(enabled);
            
            // Disable touch and query to bypass raycast-based anti-cheats
            // Note: Offsets for CanTouch/CanQuery might vary, but standard BasePart bits handle this.
            // For now, we focus on the primary collision bit.
        }
    }

    if (class_name == "Humanoid")
    {
        rbx::humanoid_t humanoid(instance.address);
        if (!enabled) // If noclip is active (enabled is false for 'CanCollide')
        {
            // Set state to 11 (NoClip) or 8 (Physics)
            // memory->write<uint8_t>(humanoid.address + Offsets::Humanoid::State, 11); 
        }
    }

    std::vector<rbx::instance_t> children = instance.get_children();
    for (auto& child : children)
    {
        set_noclip_recursive(child, enabled);
    }
}

static void noclip_run()
{
    static bool key_was_pressed = false;
    static bool toggle_state = false;

    if (!settings::expl::noclip_enabled)
    {
        toggle_state = false;
        return;
    }

    bool active = is_key_active(settings::expl::noclip_keybind, settings::expl::noclip_keybind_mode, key_was_pressed, toggle_state);

    if (game::local_player.address == 0)
        return;

    rbx::player_t local_player_obj = { game::local_player.address };
    rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
    if (model_instance.address == 0)
        return;

    // Apply noclip
    set_noclip_recursive(model_instance, !active);

    // If active, zero out velocity to prevent flinging when passing through walls
    if (active)
    {
        rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
        if (hrp.address != 0)
        {
            rbx::part_t part(hrp.address);
            rbx::primitive_t prim = part.get_primitive();
            if (prim.address != 0)
            {
                // Only zero out if not flying (fly handles its own velocity)
                if (!settings::expl::fly_enabled && !settings::expl::fly_noclip_enabled)
                {
                    memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(0, 0, 0));
                }
            }
        }
    }
}


static bool vehicle_fly_run()
{
    static bool key_was_pressed = false;
    static bool toggle_state = false;
    static bool was_disabled_by_typing = false;

    if (check::textchatopen)
    {
        was_disabled_by_typing = true;
        toggle_state = false;
        return false;
    }

    if (was_disabled_by_typing && !check::textchatopen)
    {
        toggle_state = false;
        was_disabled_by_typing = false;
    }

    if (!settings::expl::vehicle_fly_enabled)
    {
        toggle_state = false;
        return false;
    }

    bool active = is_key_active(settings::expl::vehicle_fly_keybind, settings::expl::vehicle_fly_keybind_mode, key_was_pressed, toggle_state);

    if (!active)
        return false;

    if (game::local_player.address == 0)
        return false;

    rbx::player_t local_player_obj = { game::local_player.address };
    rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
    if (model_instance.address == 0)
        return false;

    rbx::instance_t humanoid_instance = model_instance.find_first_child_by_class("Humanoid");
    if (humanoid_instance.address == 0)
        return false;

    rbx::humanoid_t humanoid(humanoid_instance.address);
    rbx::instance_t seat_instance = humanoid.get_seat_part();

    if (seat_instance.address == 0)
    {
        const char* seat_names[] = { "Seat", "VehicleSeat", "DriveSeat", "PilotSeat", "DriverSeat" };
        for (const char* name : seat_names)
        {
            seat_instance = model_instance.find_first_child(name);
            if (seat_instance.address != 0)
                break;
        }
    }

    if (seat_instance.address == 0)
        return false;

    rbx::part_t seat_part(seat_instance.address);
    rbx::primitive_t seat_prim = seat_part.get_primitive();
    if (seat_prim.address == 0)
        return false;

    rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
    if (camera_instance.address == 0)
        return false;

    rbx::camera_t camera{ camera_instance.address };
    math::matrix3 camera_rotation = camera.get_rotation();

    math::vector3 move_direction(0.0f, 0.0f, 0.0f);
    if (GetAsyncKeyState('W') & 0x8000) move_direction.z -= 1.0f;
    if (GetAsyncKeyState('S') & 0x8000) move_direction.z += 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) move_direction.x -= 1.0f;
    if (GetAsyncKeyState('D') & 0x8000) move_direction.x += 1.0f;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) move_direction.y += 1.0f;
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) move_direction.y -= 1.0f;

    if (length(move_direction) > 0.0f)
    {
        move_direction = normalize(move_direction);
        math::vector3 new_velocity = camera_rotation * move_direction * settings::expl::vehicle_fly_speed;
        memory->write<math::vector3>(seat_prim.address + Offsets::Primitive::AssemblyLinearVelocity, new_velocity);
    }
    else
    {
        memory->write<math::vector3>(seat_prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(0.0f, 0.0f, 0.0f));
    }

    // Vehicle Noclip
    rbx::instance_t current = seat_instance;
    for (int i = 0; i < 3; i++) // Go up to find the vehicle model
    {
        rbx::instance_t parent = current.get_parent();
        if (parent.address == 0 || parent.get_class_name() == "Workspace") break;
        current = parent;
        if (current.get_class_name() == "Model") break;
    }
    set_noclip_recursive(current, false);

    return true;
}


static void perform_fly(rbx::primitive_t prim, const math::matrix3& camera_rotation, float speed, int mode)
{
    math::vector3 move_direction(0.0f, 0.0f, 0.0f);
    if (GetAsyncKeyState('W') & 0x8000) move_direction.z -= 1.0f;
    if (GetAsyncKeyState('S') & 0x8000) move_direction.z += 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) move_direction.x -= 1.0f;
    if (GetAsyncKeyState('D') & 0x8000) move_direction.x += 1.0f;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) move_direction.y += 1.0f;
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) move_direction.y -= 1.0f;

    if (length(move_direction) > 0.0f)
    {
        move_direction = normalize(move_direction);
        math::vector3 rotated_direction = camera_rotation * move_direction;

        if (mode == 0) // Velocity
        {
            math::vector3 new_velocity = rotated_direction * speed;
            memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, new_velocity);
        }
        else if (mode == 1) // CFrame
        {
            math::vector3 current_position = prim.get_position();
            math::matrix3 current_rotation = prim.get_rotation();
            math::vector3 new_position = current_position + (rotated_direction * (speed / 165.0f));
            memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, new_position);
            memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, current_rotation);
        }
    }
    else
    {
        if (mode == 0) // Velocity - zero out when not moving
        {
            memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(0.0f, 0.0f, 0.0f));
        }
        else if (mode == 1) // CFrame - hold position
        {
            math::vector3 current_position = prim.get_position();
            math::matrix3 current_rotation = prim.get_rotation();
            memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(0.0f, 0.0f, 0.0f));
        }
    }
}


static void fly_noclip_run()
{
    static bool key_was_pressed = false;
    static bool toggle_state = false;
    static bool was_disabled_by_typing = false;

    if (check::textchatopen)
    {
        was_disabled_by_typing = true;
        toggle_state = false;
        return;
    }

    if (was_disabled_by_typing && !check::textchatopen)
    {
        toggle_state = false;
        was_disabled_by_typing = false;
    }

    if (!settings::expl::fly_noclip_enabled)
    {
        toggle_state = false;
        return;
    }

    bool active = is_key_active(settings::expl::fly_noclip_keybind, settings::expl::fly_noclip_keybind_mode, key_was_pressed, toggle_state);

    if (!active)
        return;

    if (game::local_player.address == 0)
        return;

    rbx::player_t local_player_obj = { game::local_player.address };
    rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
    if (model_instance.address == 0)
        return;

    // Noclip
    set_noclip_recursive(model_instance, false);

    // Fly
    rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
    if (hrp.address == 0)
        return;

    rbx::part_t part(hrp.address);
    rbx::primitive_t prim = part.get_primitive();
    if (prim.address == 0)
        return;

    rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
    if (camera_instance.address == 0)
        return;

    rbx::camera_t camera{ camera_instance.address };
    perform_fly(prim, camera.get_rotation(), settings::expl::fly_speed, settings::expl::fly_mode);
}
namespace fly
{
    void run()
    {
        static bool key_was_pressed = false;
        static bool toggle_state = false;
        static bool was_disabled_by_typing = false;

        for (;;)
        {
            Sleep(10);

            if (check::textchatopen)
            {
                was_disabled_by_typing = true;
                toggle_state = false;
            }
            else if (was_disabled_by_typing)
            {
                toggle_state = false;
                was_disabled_by_typing = false;
            }

            vehicle_fly_run();
            fly_noclip_run();
            noclip_run();

            if (!settings::expl::fly_enabled)
            {
                toggle_state = false;
                continue;
            }

            bool active = is_key_active(settings::expl::fly_keybind, settings::expl::fly_keybind_mode, key_was_pressed, toggle_state);

            if (!active)
                continue;

            if (game::local_player.address == 0)
                continue;

            rbx::player_t local_player_obj = { game::local_player.address };
            rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
            if (model_instance.address == 0)
                continue;

            rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
            if (hrp.address == 0)
                continue;

            rbx::part_t part(hrp.address);
            rbx::primitive_t prim = part.get_primitive();
            if (prim.address == 0)
                continue;

            rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
            if (camera_instance.address == 0)
                continue;

            rbx::camera_t camera{ camera_instance.address };
            perform_fly(prim, camera.get_rotation(), settings::expl::fly_speed, settings::expl::fly_mode);
        }
    }
}

