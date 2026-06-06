#include "misc.h"
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>
#include <runtime/memory.h>
#include <windows.h>
#include <cmath>

static void free_cam_run()
{
    static math::vector3 freecam_pos;
    static float yaw = 0.0f, pitch = 0.0f;
    static bool initialized = false;
    static POINT last_cursor_pos;

    rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
    if (camera_instance.address == 0) return;

    rbx::camera_t camera{ camera_instance.address };

    if (!settings::expl::free_cam) {
        if (initialized) {
            // Restore CameraType to Default (0)
            memory->write<int>(camera.address + Offsets::Camera::CameraType, 0);
            initialized = false;
        }
        return;
    }

    if (!initialized) {
        freecam_pos = camera.get_position();
        GetCursorPos(&last_cursor_pos);
        // We start with zeroed yaw/pitch, the game will snap to it once we start moving
        initialized = true;
    }

    // Handle Rotation (Right-Click to Look)
    POINT current_cursor_pos;
    GetCursorPos(&current_cursor_pos);
    
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
        float delta_x = (float)(current_cursor_pos.x - last_cursor_pos.x);
        float delta_y = (float)(current_cursor_pos.y - last_cursor_pos.y);
        
        yaw -= delta_x * 0.003f;
        pitch -= delta_y * 0.003f;
        
        // Clamp pitch to avoid gimbal lock
        if (pitch > 1.5f) pitch = 1.5f;
        if (pitch < -1.5f) pitch = -1.5f;
    }
    last_cursor_pos = current_cursor_pos;

    // Construct rotation matrix (Euler YXZ)
    float cy = std::cos(yaw), sy = std::sin(yaw);
    float cp = std::cos(pitch), sp = std::sin(pitch);
    
    math::matrix3 rotation;
    rotation.m[0] = cy;     rotation.m[1] = sy * sp;  rotation.m[2] = sy * cp;
    rotation.m[3] = 0;      rotation.m[4] = cp;       rotation.m[5] = -sp;
    rotation.m[6] = -sy;    rotation.m[7] = cy * sp;  rotation.m[8] = cy * cp;

    // Movement Input
    math::vector3 move_dir(0, 0, 0);
    if (GetAsyncKeyState('W') & 0x8000) move_dir.z -= 1.0f;
    if (GetAsyncKeyState('S') & 0x8000) move_dir.z += 1.0f;
    if (GetAsyncKeyState('A') & 0x8000) move_dir.x -= 1.0f;
    if (GetAsyncKeyState('D') & 0x8000) move_dir.x += 1.0f;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) move_dir.y += 1.0f;
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000) move_dir.y -= 1.0f;

    if (move_dir.x != 0 || move_dir.y != 0 || move_dir.z != 0) {
        // Normalize move direction
        float len = std::sqrt(move_dir.x * move_dir.x + move_dir.y * move_dir.y + move_dir.z * move_dir.z);
        move_dir.x /= len; move_dir.y /= len; move_dir.z /= len;

        float speed = settings::expl::free_cam_speed;
        if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) speed *= 3.0f; // Boost speed

        // Transform local movement to world space using our rotation matrix
        // In Roblox matrices: Col0=Right, Col1=Up, Col2=Back (negative Look)
        math::vector3 right = { rotation.m[0], rotation.m[3], rotation.m[6] };
        math::vector3 up = { rotation.m[1], rotation.m[4], rotation.m[7] };
        math::vector3 forward = { -rotation.m[2], -rotation.m[5], -rotation.m[8] };

        math::vector3 world_move = (right * move_dir.x) + (up * move_dir.y) + (forward * -move_dir.z);
        freecam_pos = freecam_pos + (world_move * speed);
    }

    // Force CameraType to Scriptable (7)
    memory->write<int>(camera.address + Offsets::Camera::CameraType, 7);
    
    // Apply new Position and Rotation
    memory->write<math::vector3>(camera.address + Offsets::Camera::Position, freecam_pos);
    memory->write<math::matrix3>(camera.address + Offsets::Camera::Rotation, rotation);
}

static void click_tp_run()
{
    if (!settings::expl::click_tp) return;

    static bool was_pressed = false;
    bool key_pressed = (settings::expl::click_tp_keybind == 0) ? true : (GetAsyncKeyState(settings::expl::click_tp_keybind) & 0x8000);
    bool mouse_clicked = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

    if (key_pressed && mouse_clicked && !was_pressed)
    {
        if (game::local_player.address == 0) return;

        uintptr_t mouse = memory->read<uintptr_t>(game::local_player.address + Offsets::Player::Mouse);
        if (mouse == 0) return;

        // Mouse.Hit is a CFrame. Position is at PlayerMouse::HitPosition.
        math::vector3 hit_pos = memory->read<math::vector3>(mouse + Offsets::PlayerMouse::HitPosition);

        rbx::player_t local_player_obj = { game::local_player.address };
        rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
        if (model_instance.address != 0)
        {
            rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
            if (hrp.address != 0)
            {
                rbx::part_t part(hrp.address);
                rbx::primitive_t prim = part.get_primitive();
                if (prim.address != 0)
                {
                    hit_pos.y += 3.0f; // Teleport slightly above to avoid getting stuck
                    memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, hit_pos);
                }
            }
        }
    }
    was_pressed = key_pressed && mouse_clicked;
}

static void fling_run(rbx::primitive_t prim)
{
    if (!settings::expl::fling_enabled) return;
    if (prim.address == 0) return;

    // Apply massive angular velocity to cause physics flinging on contact
    // We use a very high value to ensure a powerful launch upon collision
    float power = settings::expl::fling_power;
    
    // Spinning on all axes for maximum "hitbox" coverage
    math::vector3 angular(power * 20.0f, power * 20.0f, power * 20.0f);

    // We only write to AssemblyAngularVelocity to keep the user's linear position stable
    memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyAngularVelocity, angular);
    
    // Optional: Small linear "vibration" to help bypass physics sleeping
    static bool flip = false;
    float jitter = flip ? 50.0f : -50.0f;
    memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, math::vector3(jitter, jitter, jitter));
    flip = !flip;
}

void misc::run()
{
    float spin_angle = 0.0f;
    for (;;)
    {
        Sleep(10);

        free_cam_run();
        click_tp_run();

        if (game::local_player.address == 0) continue;

        rbx::player_t local_player_obj = { game::local_player.address };
        rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
        if (model_instance.address == 0) continue;

        rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
        if (hrp.address == 0) continue;

        rbx::part_t part(hrp.address);
        rbx::primitive_t prim = part.get_primitive();
        if (prim.address == 0) continue;

        fling_run(prim);

        // Gravity Controller
        if (settings::expl::gravity_enabled)
        {
            auto world = memory->read<uintptr_t>(game::workspace.address + Offsets::Workspace::World);
            if (world)
            {
                memory->write<float>(world + Offsets::World::Gravity, settings::expl::gravity_amount);
            }
        }

        // Vertex User Signature (HipHeight Signaling) & Jump Height Application
        if (model_instance.address != 0)
        {
            rbx::instance_t hum_inst = model_instance.find_first_child("Humanoid");
            if (hum_inst.address != 0)
            {
                rbx::humanoid_t hum(hum_inst.address);
                hum.set_hip_height(2.01337f);

                if (settings::expl::jump_height_enabled)
                {
                    hum.set_jump_height(settings::expl::jump_height);
                    hum.set_jump_power(settings::expl::jump_height);
                }
            }
        }

        // Fixed Infinite Jump (Humanoid Method)
        if (settings::expl::infinite_jump)
        {
            if (GetAsyncKeyState(VK_SPACE) & 0x8000)
            {
                rbx::instance_t humanoid_inst = model_instance.find_first_child("Humanoid");
                if (humanoid_inst.address != 0)
                {
                    memory->write<bool>(humanoid_inst.address + Offsets::Humanoid::Jump, true);
                    
                    math::vector3 velocity = prim.get_velocity();
                    if (velocity.y < 5.0f) {
                        // Use the custom jump height if enabled, otherwise default to 50
                        float power = settings::expl::jump_height_enabled ? settings::expl::jump_height : 50.0f;
                        velocity.y = power;
                        memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, velocity);
                    }
                }
            }
        }

        // God Mode & No Fall Damage removed (Incompatible with External)

        // Spinbot
        if (settings::expl::spinbot)
        {
            spin_angle += settings::expl::spinbot_speed * 0.1f;
            if (spin_angle > 360.0f) spin_angle = 0.0f;

            float rad = spin_angle * (3.14159265f / 180.0f);
            math::matrix3 rot;
            memset(&rot, 0, sizeof(rot));
            
            rot.m[0] = std::cos(rad);
            rot.m[2] = std::sin(rad);
            rot.m[6] = -std::sin(rad);
            rot.m[8] = std::cos(rad);
            rot.m[4] = 1.0f; // Y axis stays fixed

            memory->write<math::matrix3>(prim.address + Offsets::Primitive::Rotation, rot);
        }

        // Teleport to Nearest Player
        if (settings::expl::teleport)
        {
            static bool was_pressed = false;
            bool is_pressed = (settings::expl::teleport_keybind == 0) ? true : (GetAsyncKeyState(settings::expl::teleport_keybind) & 0x8000);
            
            bool should_tp = false;
            if (settings::expl::teleport_keybind == 0) {
                should_tp = true;
            } else if (settings::expl::teleport_keybind_mode == 0) { // Hold
                should_tp = is_pressed;
            } else if (settings::expl::teleport_keybind_mode == 1) { // Toggle
                static bool toggled = false;
                if (is_pressed && !was_pressed) toggled = !toggled;
                should_tp = toggled;
            } else if (settings::expl::teleport_keybind_mode == 2) { // Always (for completeness)
                should_tp = true;
            }
            was_pressed = is_pressed;

            if (should_tp)
            {
                float closest_dist = 1000000.0f;
                math::vector3 target_pos(0, 0, 0);
                bool found = false;

                auto players_list = game::players.get_children();
                math::vector3 local_pos = prim.get_position();

                for (auto& player_inst : players_list)
                {
                    if (player_inst.address == game::local_player.address) continue;
                    
                    rbx::player_t player(player_inst.address);
                    rbx::model_instance_t char_model = player.get_model_instance();
                    if (char_model.address == 0) continue;

                    rbx::instance_t target_hrp_inst = char_model.find_first_child("HumanoidRootPart");
                    if (target_hrp_inst.address == 0) continue;

                    rbx::part_t target_hrp(target_hrp_inst.address);
                    rbx::primitive_t target_prim = target_hrp.get_primitive();
                    if (target_prim.address == 0) continue;

                    math::vector3 p_pos = target_prim.get_position();
                    math::vector3 diff = p_pos - local_pos;
                    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);

                    if (dist < closest_dist && dist > 5.0f) // Don't TP to ourselves or if too close
                    {
                        closest_dist = dist;
                        target_pos = p_pos;
                        found = true;
                    }
                }

                if (found)
                {
                    target_pos.y += 3.0f; // Slightly above
                    memory->write<math::vector3>(prim.address + Offsets::Primitive::Position, target_pos);
                    
                    // If not always/hold, reset toggle to avoid infinite TP if it's meant to be a one-shot
                    // But usually "Teleport" is a toggle for "Magnet" or similar.
                    // If the user wants a one-shot TP, they should use a keybind with "Hold" or I should add a "Press" mode.
                }
            }
        }
    }
}
