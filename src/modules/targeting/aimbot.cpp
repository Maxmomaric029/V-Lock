#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <buffer/cache.h>
#include <session/game.h>
#include <settings.h>
#include <verify/typing_check.h>
#include "aimbot.h"

static bool is_same_team(cache::entity_t& player)
{
	if (cache::cached_local_player.instance.address == 0)
		return false;

	std::uint64_t local_team  = cache::cached_local_player.team_address;
	std::uint64_t target_team = player.team_address;

	// If BOTH have no team assigned (0), treat as same team (unassigned players)
	if (local_team == 0 && target_team == 0)
		return true;

	// If only ONE has no team, they are on different teams
	if ((local_team == 0) != (target_team == 0))
		return false;

	// Both have non-zero team addresses — validate before reading
	if (!memory->is_valid_instance_address(local_team) || !memory->is_valid_instance_address(target_team))
		return false;

	if (local_team == target_team)
		return true;

	// Check color (Roblox standard)
	std::uint32_t local_color  = memory->read<std::uint32_t>(local_team  + Offsets::Team::BrickColor);
	std::uint32_t target_color = memory->read<std::uint32_t>(target_team + Offsets::Team::BrickColor);

	if (local_color != 0 && local_color == target_color)
		return true;

	// Advanced Side-based detection for games like Jailbreak, Mad City, ER:LC
	// This ensures Prisoners/Criminals don't kill each other even if they are on different Team objects.
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
			   name.find("sheriff") != std::string::npos || 
			   name.find("officer") != std::string::npos || 
			   name.find("fbi") != std::string::npos || 
			   name.find("swat") != std::string::npos ||
			   name.find("guard") != std::string::npos;
	};

	if (is_lawless(l) && is_lawless(t)) return true;
	if (is_law(l) && is_law(t)) return true;

	return false;
}

// Local has_wall_between removed, using cache::has_wall_between

static bool is_player_knocked(cache::entity_t& player)
{
	if (player.instance.address == 0)
		return false;

	rbx::player_t player_instance(player.instance.address);
	rbx::model_instance_t model_instance = player_instance.get_model_instance();

	if (model_instance.address == 0)
		return false;

	rbx::instance_t body_effects = model_instance.find_first_child("BodyEffects");
	if (body_effects.address == 0)
		return false;

	rbx::instance_t ko = body_effects.find_first_child("K.O");
	if (ko.address == 0)
		return false;

	// Defense: validate address before read
	if (!memory->is_valid_instance_address(ko.address))
		return false;

	bool ko_value = memory->read<bool>(ko.address + Offsets::Misc::Value);
	return ko_value;
}

static math::vector3 normalize(const math::vector3& vec)
{
	float length = std::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	// Minimum threshold prevents division by zero + invalid downstream matrix
	if (length < 0.001f)
		return { 0.0f, 1.0f, 0.0f };  // Safe up vector
	return math::vector3{ vec.x / length, vec.y / length, vec.z / length };
}

static math::vector3 cross_product(const math::vector3& vec1, const math::vector3& vec2)
{
	return {
		vec1.y * vec2.z - vec1.z * vec2.y,
		vec1.z * vec2.x - vec1.x * vec2.z,
		vec1.x * vec2.y - vec1.y * vec2.x
	};
}

static math::matrix3 look_at_to_matrix(const math::vector3& cameraPosition, const math::vector3& targetPosition)
{
	math::vector3 forward = normalize(math::vector3{ 
		targetPosition.x - cameraPosition.x, 
		targetPosition.y - cameraPosition.y, 
		targetPosition.z - cameraPosition.z 
	});
	math::vector3 right = normalize(cross_product({ 0, 1, 0 }, forward));
	math::vector3 up = cross_product(forward, right);

	math::matrix3 lookAtMatrix{};
	lookAtMatrix.m[0] = -right.x;  lookAtMatrix.m[1] = up.x;  lookAtMatrix.m[2] = -forward.x;
	lookAtMatrix.m[3] = right.y;  lookAtMatrix.m[4] = up.y;  lookAtMatrix.m[5] = -forward.y;
	lookAtMatrix.m[6] = -right.z;  lookAtMatrix.m[7] = up.z;  lookAtMatrix.m[8] = -forward.z;

	return lookAtMatrix;
}

static void perform_camera_aimbot(const math::vector3& targetPos, const math::vector3& cameraPos)
{
	rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
	if (!camera_instance.address)
		return;
	
	rbx::camera_t camera{ camera_instance.address };

	math::matrix3 currentRotation = camera.get_rotation();
	math::matrix3 targetMatrix = look_at_to_matrix(cameraPos, targetPos);

	if (settings::aimbot::camera_smooth)
	{
		float smoothX = std::clamp(settings::aimbot::camera_smooth_x, 1.0f, 100.0f);
		float smoothY = std::clamp(settings::aimbot::camera_smooth_y, 1.0f, 100.0f);

		math::matrix3 result{};
		for (int i = 0; i < 9; ++i)
		{
			float factor = (i % 3 == 0) ? (1.0f / smoothX) : (1.0f / smoothY);
			result.m[i] = currentRotation.m[i] + (targetMatrix.m[i] - currentRotation.m[i]) * factor;
		}

		camera.write_rotation(result);
	}
	else
	{
		camera.write_rotation(targetMatrix);
	}

	if (settings::aimbot::shake)
	{
		math::matrix3 shaken = targetMatrix;
		for (int i = 0; i < 9; ++i)
		{
			if (i % 3 == 0)
			{
				shaken.m[i] += (float(rand() % int(settings::aimbot::shake_x * 2 + 1) - settings::aimbot::shake_x) * 0.001f);
			}
			else
			{
				shaken.m[i] += (float(rand() % int(settings::aimbot::shake_y * 2 + 1) - settings::aimbot::shake_y) * 0.001f);
			}
		}
		camera.write_rotation(shaken);
	}
}

static rbx::part_t get_closest_part(cache::entity_t& player, const POINT& cursor_point)
{
	rbx::part_t closest_part{};
	float min_distance = FLT_MAX;

	// Validate visengine address before accessing
	if (!memory->is_valid_instance_address(game::visengine.address))
		return closest_part;

	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 view_matrix = game::visengine.get_viewmatrix();
	math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };

	const char* part_names[] = {
		"Head", "HumanoidRootPart", "UpperTorso", "LowerTorso",
		"LeftHand", "RightHand", "LeftUpperArm", "RightUpperArm",
		"LeftUpperLeg", "RightUpperLeg", "LeftFoot", "RightFoot"
	};

	for (int i = 0; i < 12; ++i)
	{
		auto part_it = player.parts.find(part_names[i]);
		if (part_it == player.parts.end())
			continue;

		rbx::part_t part = part_it->second;
		if (!part.address)
			continue;

		rbx::primitive_t primitive = part.get_primitive();
		math::vector3 part_position = primitive.get_position();
		math::vector2 part_screen_pos{};
		
		if (!game::visengine.world_to_screen(part_position, part_screen_pos, dimensions, view_matrix))
			continue;

		float distance = std::sqrt(
			(part_screen_pos.x - cursor.x) * (part_screen_pos.x - cursor.x) +
			(part_screen_pos.y - cursor.y) * (part_screen_pos.y - cursor.y)
		);

		if (distance < min_distance)
		{
			min_distance = distance;
			closest_part = part;
		}
	}

	return closest_part;
}

static rbx::part_t get_target_part(cache::entity_t& player, int aim_part, const POINT& cursor_point)
{
	rbx::part_t target_part{};
	
	if (aim_part == 0)
	{
		auto part_it = player.parts.find("Head");
		if (part_it != player.parts.end())
			target_part = part_it->second;
	}
	else if (aim_part == 1)
	{
		auto torso_it = player.parts.find("UpperTorso");
		if (torso_it != player.parts.end())
			target_part = torso_it->second;
		else
		{
			auto torso_r6 = player.parts.find("Torso");
			if (torso_r6 != player.parts.end())
				target_part = torso_r6->second;
		}
	}
	else if (aim_part == 2)
	{
		target_part = get_closest_part(player, cursor_point);
	}

	return target_part;
}

static bool is_target_within_fov(cache::entity_t& player)
{
	if (player.instance.address == 0)
		return false;

	POINT cursor_point;
	HWND rblxWnd = FindWindowA(nullptr, "Roblox");
	if (!rblxWnd)
		return false;

	if (!GetCursorPos(&cursor_point))
		return false;

	if (!ScreenToClient(rblxWnd, &cursor_point))
		return false;

	math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };

	POINT cursor_point_temp;
	GetCursorPos(&cursor_point_temp);
	HWND rblxWnd_temp = FindWindowA(nullptr, "Roblox");
	if (rblxWnd_temp)
		ScreenToClient(rblxWnd_temp, &cursor_point_temp);
	rbx::part_t target_part = get_target_part(player, settings::aimbot::aim_part, cursor_point_temp);
	if (!target_part.address)
		return false;

	rbx::primitive_t primitive = target_part.get_primitive();
	math::vector3 part_world_pos = primitive.get_position();
	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 viewMatrix = game::visengine.get_viewmatrix();
	math::vector2 screen_pos{};
	
	if (!game::visengine.world_to_screen(part_world_pos, screen_pos, dimensions, viewMatrix))
		return false;

	float cursor_dist = std::sqrt(
		(screen_pos.x - cursor.x) * (screen_pos.x - cursor.x) +
		(screen_pos.y - cursor.y) * (screen_pos.y - cursor.y)
	);

	return cursor_dist <= settings::aimbot::fov;
}

static cache::entity_t get_closest_player()
{
	cache::entity_t closest_player{};
	float shortest_distance = FLT_MAX;

	POINT cursor_point;
	GetCursorPos(&cursor_point);
	HWND rblxWnd = FindWindowA(nullptr, "Roblox");
	if (!rblxWnd || !ScreenToClient(rblxWnd, &cursor_point))
		return closest_player;

	math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 viewMatrix = game::visengine.get_viewmatrix();

	std::lock_guard<std::recursive_mutex> lock(cache::mtx);
	
	for (auto& player : cache::cached_players)
	{
		if (player.instance.address == 0 || player.instance.address == cache::cached_local_player.instance.address)
			continue;

		if (settings::aimbot::knocked_check && is_player_knocked(player))
			continue;

		if (settings::aimbot::team_check && is_same_team(player))
			continue;

		POINT cursor_point_temp;
		GetCursorPos(&cursor_point_temp);
		HWND rblxWnd_temp = FindWindowA(nullptr, "Roblox");
		if (rblxWnd_temp)
			ScreenToClient(rblxWnd_temp, &cursor_point_temp);
		rbx::part_t root_part = get_target_part(player, 1, cursor_point_temp);
		if (!root_part.address)
			continue;

		rbx::primitive_t primitive = root_part.get_primitive();
		math::vector3 part_world_pos = primitive.get_position();

		// Skip invalid coordinates (0, 0, 0) which usually mean the character hasn't spawned or is spectating
		if (std::abs(part_world_pos.x) < 0.1f && std::abs(part_world_pos.y) < 0.1f && std::abs(part_world_pos.z) < 0.1f)
			continue;

		math::vector2 screen_pos{};
		
		if (!game::visengine.world_to_screen(part_world_pos, screen_pos, dimensions, viewMatrix))
			continue;

		float cursor_dist = std::sqrt(
			(screen_pos.x - cursor.x) * (screen_pos.x - cursor.x) +
			(screen_pos.y - cursor.y) * (screen_pos.y - cursor.y)
		);

		if (settings::aimbot::fov_check && cursor_dist > settings::aimbot::fov)
			continue;

		if (settings::aimbot::wall_check)
		{
			rbx::instance_t cam_inst = game::workspace.find_first_child_by_class("Camera");
			if (cam_inst.address)
			{
				rbx::camera_t cam{ cam_inst.address };
				math::vector3 cam_pos = cam.get_position();
				if (cache::has_wall_between(cam_pos, part_world_pos))
					continue;
			}
		}

		if (cursor_dist < shortest_distance)
		{
			shortest_distance = cursor_dist;
			closest_player = player;
		}
	}

	return closest_player;
}

void rbx::aimbot::run()
{
	printf("\x1b[38;5;118m   [AIMBOT] Thread started\x1b[0m\n");
	for (;;)
	{
		Sleep(10);
		if (!settings::aimbot::enabled)
			continue;

		if (settings::aimbot::aimbot_type != 0 && settings::aimbot::aimbot_type != 1)
			continue;

		// === DIAGNÓSTICO: estado de punteros cada 100 iteraciones ===
		{
			static int diag_counter = 0;
			if (++diag_counter % 100 == 0) {
				printf("\x1b[38;5;214m   [AIMBOT] enabled=%d | players=0x%llx | local=0x%llx | char=0x%llx | cached_players=%zu\x1b[0m\n",
					settings::aimbot::enabled,
					(unsigned long long)game::players.address,
					(unsigned long long)game::local_player.address,
					(unsigned long long)game::local_character.address,
					cache::cached_players.size());
			}
		}

		static bool key_was_pressed = false;
		static bool toggle_state = false;
		static cache::entity_t saved_target{};
		static bool is_aimbotting = false;
		static bool was_disabled_by_typing = false;
		bool key_pressed = false;

		if (check::textchatopen)
		{
			was_disabled_by_typing = true;
			toggle_state = false;
			is_aimbotting = false;
			saved_target = cache::entity_t{};
			continue;
		}

		if (was_disabled_by_typing && !check::textchatopen)
		{
			toggle_state = false;
			was_disabled_by_typing = false;
		}

		if (settings::aimbot::keybind_mode == 0)
		{
			key_pressed = GetAsyncKeyState(settings::aimbot::keybind) & 0x8000;
		}
		else if (settings::aimbot::keybind_mode == 1)
		{
			bool current_key_state = GetAsyncKeyState(settings::aimbot::keybind) & 0x8000;
			if (current_key_state && !key_was_pressed)
			{
				toggle_state = !toggle_state;
				key_was_pressed = true;
			}
			else if (!current_key_state)
			{
				key_was_pressed = false;
			}
			key_pressed = toggle_state;
		}
		else if (settings::aimbot::keybind_mode == 2)
		{
			key_pressed = true;
		}

		if (!key_pressed)
		{
			is_aimbotting = false;
			saved_target = cache::entity_t{};
			continue;
		}

		if (!game::workspace.address)
			continue;

		cache::entity_t target{};
		if (settings::aimbot::sticky_aim && is_aimbotting && saved_target.instance.address)
		{
			std::lock_guard<std::recursive_mutex> lock(cache::mtx);
			for (auto& player : cache::cached_players)
			{
				if (player.instance.address == saved_target.instance.address)
				{
					if (settings::aimbot::knocked_check && is_player_knocked(player))
					{
						is_aimbotting = false;
						saved_target = cache::entity_t{};
						break;
					}

					if (settings::aimbot::team_check && is_same_team(player))
					{
						is_aimbotting = false;
						saved_target = cache::entity_t{};
						break;
					}

					if (settings::aimbot::fov_check && !is_target_within_fov(player))
					{
						is_aimbotting = false;
						saved_target = cache::entity_t{};
						break;
					}

					if (settings::aimbot::wall_check)
					{
						rbx::instance_t cam_inst = game::workspace.find_first_child_by_class("Camera");
						if (cam_inst.address)
						{
							rbx::camera_t cam{ cam_inst.address };
							math::vector3 cam_pos = cam.get_position();
							auto root_it = player.parts.find("HumanoidRootPart");
							if (root_it != player.parts.end())
							{
								math::vector3 tgt_pos = root_it->second.get_primitive().get_position();
								if (cache::has_wall_between(cam_pos, tgt_pos))
								{
									is_aimbotting = false;
									saved_target = cache::entity_t{};
									break;
								}
							}
						}
					}

					target = player;
					break;
				}
			}
		}

		if (!target.instance.address)
		{
			target = get_closest_player();
			if (!target.instance.address)
			{
				is_aimbotting = false;
				saved_target = cache::entity_t{};
				continue;
			}
		}

		POINT cursor_point;
		GetCursorPos(&cursor_point);
		HWND rblxWnd = FindWindowA(nullptr, "Roblox");
		if (rblxWnd)
			ScreenToClient(rblxWnd, &cursor_point);

		rbx::part_t target_part = get_target_part(target, settings::aimbot::aim_part, cursor_point);
		if (!target_part.address)
		{
			continue;
		}

		rbx::primitive_t primitive = target_part.get_primitive();
		math::vector3 target_pos = primitive.get_position();

		if (settings::aimbot::aimbot_type == 0)
		{
			if (settings::aimbot::camera_prediction)
			{
				math::vector3 velocity = primitive.get_velocity();

				if (std::isfinite(velocity.x) && std::isfinite(velocity.y) && std::isfinite(velocity.z) &&
					std::abs(velocity.x) < 1000.0f && std::abs(velocity.y) < 1000.0f && std::abs(velocity.z) < 1000.0f)
				{
					float prediction_scale = 0.016f;
					target_pos.x += velocity.x * prediction_scale * settings::aimbot::camera_prediction_x;
					target_pos.y += velocity.y * prediction_scale * settings::aimbot::camera_prediction_y;
					target_pos.z += velocity.z * prediction_scale * settings::aimbot::camera_prediction_x;
				}
			}
		}
		else if (settings::aimbot::aimbot_type == 1)
		{
			if (settings::aimbot::mouse_prediction)
			{
				math::vector3 velocity = primitive.get_velocity();

				if (std::isfinite(velocity.x) && std::isfinite(velocity.y) && std::isfinite(velocity.z) &&
					std::abs(velocity.x) < 1000.0f && std::abs(velocity.y) < 1000.0f && std::abs(velocity.z) < 1000.0f)
				{
					float prediction_scale = 0.016f;
					target_pos.x += velocity.x * prediction_scale * settings::aimbot::mouse_prediction_x;
					target_pos.y += velocity.y * prediction_scale * settings::aimbot::mouse_prediction_y;
					target_pos.z += velocity.z * prediction_scale * settings::aimbot::mouse_prediction_x;
				}
			}
		}

		if (settings::aimbot::aimbot_type == 0)
		{
			rbx::instance_t camera_instance = game::workspace.find_first_child_by_class("Camera");
			if (!camera_instance.address)
				continue;

			rbx::camera_t camera{ camera_instance.address };

			math::vector3 camera_pos = camera.get_position();
			perform_camera_aimbot(target_pos, camera_pos);
		}
		else if (settings::aimbot::aimbot_type == 1)
		{
			math::vector2 dimensions = game::visengine.get_dimensions();
			math::matrix4 viewMatrix = game::visengine.get_viewmatrix();
			math::vector2 screen_pos{};

			if (!game::visengine.world_to_screen(target_pos, screen_pos, dimensions, viewMatrix))
				continue;

			if (!rblxWnd)
				continue;

			float deltaX = screen_pos.x - cursor_point.x;
			float deltaY = screen_pos.y - cursor_point.y;

			float aimSensitivity = std::clamp(settings::aimbot::mouse_sensitivity, 0.1f, 10.0f);

			if (settings::aimbot::mouse_smooth)
			{
				float smoothX = std::clamp(settings::aimbot::mouse_smooth_x, 1.0f, 100.0f);
				float smoothY = std::clamp(settings::aimbot::mouse_smooth_y, 1.0f, 100.0f);

				deltaX /= smoothX;
				deltaY /= smoothY;
			}

			if (settings::aimbot::shake)
			{
				deltaX += float(rand() % (int)(settings::aimbot::shake_x * 2 + 1) - settings::aimbot::shake_x);
				deltaY += float(rand() % (int)(settings::aimbot::shake_y * 2 + 1) - settings::aimbot::shake_y);
			}

			deltaX *= aimSensitivity;
			deltaY *= aimSensitivity;

			if (std::isfinite(deltaX) && std::isfinite(deltaY) &&
				(std::abs(deltaX) >= 0.3f || std::abs(deltaY) >= 0.3f))
			{
				INPUT input = {};
				input.type = INPUT_MOUSE;
				input.mi.dx = static_cast<LONG>(deltaX);
				input.mi.dy = static_cast<LONG>(deltaY);
				input.mi.dwFlags = MOUSEEVENTF_MOVE;
				SendInput(1, &input, sizeof(INPUT));
			}
		}

		saved_target = target;
		is_aimbotting = true;
	}
}

void rbx::aimbot::initialize()
{
}

