#include "triggerbot.h"

#include <Windows.h>
#include <thread>
#include <chrono>

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>
#include <buffer/cache.h>
#include <modules/assist/silent.h>
#include <verify/typing_check.h>

#include <string>
#include <algorithm>
#include <cctype>

float get_max_float()
{
    return (std::numeric_limits<float>::max)();
}

static bool is_same_team(cache::entity_t& player)
{
	if (cache::cached_local_player.instance.address == 0)
		return false;

	std::uint64_t local_team  = cache::cached_local_player.team_address;
	std::uint64_t target_team = player.team_address;

	if (local_team == 0 || target_team == 0)
		return true;

	if (local_team == target_team)
		return true;

	std::uint32_t local_color  = memory->read<std::uint32_t>(local_team  + Offsets::Team::BrickColor);
	std::uint32_t target_color = memory->read<std::uint32_t>(target_team + Offsets::Team::BrickColor);

	if (local_color != 0 && local_color == target_color)
		return true;

	auto to_lower = [](std::string s) {
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	};

	std::string l = to_lower(cache::cached_local_player.team_name);
	std::string t = to_lower(player.team_name);

	if (l.empty() || t.empty()) return false;

	auto is_lawless = [](const std::string& name) {
		return name.find("prisoner") != std::string::npos || 
			   name.find("criminal") != std::string::npos || 
			   name.find("mafia") != std::string::npos ||
			   name.find("bandit") != std::string::npos ||
			   name.find("raider") != std::string::npos;
	};

	auto is_law = [](const std::string& name) {
		return name.find("police") != std::string::npos || 
			   name.find("cop") != std::string::npos || 
			   name.find("guard") != std::string::npos ||
			   name.find("sheriff") != std::string::npos ||
			   name.find("hero") != std::string::npos;
	};

	if (is_lawless(l) && is_lawless(t)) return true;
	if (is_law(l) && is_law(t)) return true;

	return false;
}

static cache::entity_t get_target_at_crosshair()
{
    if (!game::visengine.address)
        return {};

    math::vector2 dims = game::visengine.get_dimensions();
    math::matrix4 view = game::visengine.get_viewmatrix();

    POINT cursor_point;
    HWND rbloxWnd = FindWindowA(nullptr, "Roblox");
    if (!rbloxWnd || !GetCursorPos(&cursor_point) || !ScreenToClient(rbloxWnd, &cursor_point))
        return {};

    math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };

    std::vector<cache::entity_t> players_snapshot;
    {
        std::lock_guard<std::recursive_mutex> lock(cache::mtx);
        players_snapshot = cache::cached_players;
    }

    cache::entity_t closest_player{};
    float shortest_distance = get_max_float();

    for (cache::entity_t& player : players_snapshot)
    {
        if (player.instance.address == 0 || player.instance.address == game::local_player.address)
            continue;

        if (settings::silent::knocked_check)
        {
            rbx::player_t player_instance(player.instance.address);
            rbx::model_instance_t model_instance = player_instance.get_model_instance();
            if (model_instance.address)
            {
                rbx::instance_t body_effects = model_instance.find_first_child("BodyEffects");
                if (body_effects.address)
                {
                    rbx::instance_t ko = body_effects.find_first_child("K.O");
                    if (ko.address)
                    {
                        bool ko_value = memory->read<bool>(ko.address + Offsets::Misc::Value);
                        if (ko_value) continue;
                    }
                }
            }
        }

        if (settings::aimbot::team_check && is_same_team(player))
        {
            continue;
        }

        rbx::part_t target_part = rbx::part_t{};
        if (settings::silent::aim_part == 0)
        {
            auto head_it = player.parts.find("Head");
            if (head_it != player.parts.end())
                target_part = head_it->second;
        }
        else if (settings::silent::aim_part == 1)
        {
            auto torso_it = player.parts.find("UpperTorso");
            if (torso_it != player.parts.end())
                target_part = torso_it->second;
        }
        else
        {
            for (auto& part_pair : player.parts)
            {
                rbx::primitive_t prim = part_pair.second.get_primitive();
                if (!prim.address) continue;

                math::vector3 part_position = prim.get_position();
                math::vector2 part_screen{};
                game::visengine.world_to_screen(part_position, part_screen, dims, view);

                if (part_screen.x < 0 || part_screen.y < 0) continue;

                float distance = std::sqrt((part_screen.x - cursor.x) * (part_screen.x - cursor.x) +
                                            (part_screen.y - cursor.y) * (part_screen.y - cursor.y));
                if (distance < shortest_distance)
                {
                    // For closest part mode, we don't do wall check on every part yet to save perf,
                    // we do it at the end for the chosen part.
                    shortest_distance = distance;
                    target_part = part_pair.second;
                }
            }
        }

        if (!target_part.address) continue;

        rbx::primitive_t prim = target_part.get_primitive();
        if (!prim.address) continue;

        math::vector3 part_position = prim.get_position();
        
        if (settings::aimbot::wall_check)
        {
            std::uint64_t camera_ptr = memory->read<std::uint64_t>(game::workspace.address + Offsets::Workspace::CurrentCamera);
            if (camera_ptr)
            {
                math::vector3 cam_pos = rbx::camera_t{ camera_ptr }.get_position();
                if (cache::has_wall_between(cam_pos, part_position))
                    continue; // Skip if blocked by wall
            }
        }

        math::vector2 part_screen{};
        game::visengine.world_to_screen(part_position, part_screen, dims, view);

        if (part_screen.x < 0 || part_screen.y < 0) continue;

        float distance = std::sqrt((part_screen.x - cursor.x) * (part_screen.x - cursor.x) +
                                    (part_screen.y - cursor.y) * (part_screen.y - cursor.y));

        if (settings::silent::fov_check && distance > settings::silent::fov)
            continue;

        // Always accept the selected player (already the closest part for aim_part==2)
        // or the closest in FOV for aim_part 0/1
        if (settings::silent::aim_part == 2)
        {
            closest_player = player;
        }
        else if (distance < shortest_distance)
        {
            shortest_distance = distance;
            closest_player = player;
        }
    }

    return closest_player;
}

void triggerbot::run()
{
    for (;;)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(settings::expl::triggerbot_delay));

        if (!settings::expl::triggerbot)
            continue;

        if (check::textchatopen)
            continue;

        bool key_pressed = false;
        if (settings::expl::triggerbot_keybind == 0)
        {
            key_pressed = true;
        }
        else if (settings::expl::triggerbot_keybind_mode == 0)
        {
            key_pressed = GetAsyncKeyState(settings::expl::triggerbot_keybind) & 0x8000;
        }
        else if (settings::expl::triggerbot_keybind_mode == 1)
        {
            static bool key_was_pressed = false;
            bool current_state = GetAsyncKeyState(settings::expl::triggerbot_keybind) & 0x8000;
            if (current_state && !key_was_pressed)
            {
                key_pressed = true;
            }
            key_was_pressed = current_state;
        }
        else if (settings::expl::triggerbot_keybind_mode == 2)
        {
            key_pressed = true;
        }

        if (!key_pressed)
            continue;

        cache::entity_t target = get_target_at_crosshair();
        if (target.instance.address == 0)
            continue;

        // Simulate mouse click
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}
