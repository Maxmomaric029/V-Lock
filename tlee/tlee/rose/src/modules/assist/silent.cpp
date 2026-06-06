#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <immintrin.h>
#include <cmath>
#include <limits>

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <buffer/cache.h>
#include <session/game.h>
#include <settings.h>
#include <verify/typing_check.h>
#include "silent.h"

std::uint64_t c_silent_helper::cached_input_object = 0;

static math::vector2 world_to_screen(math::vector3 world, math::vector2 dimensions, math::matrix4 viewmatrix)
{
	float clipX = world.x * viewmatrix.m[0][0] + world.y * viewmatrix.m[0][1] + world.z * viewmatrix.m[0][2] + viewmatrix.m[0][3];
	float clipY = world.x * viewmatrix.m[1][0] + world.y * viewmatrix.m[1][1] + world.z * viewmatrix.m[1][2] + viewmatrix.m[1][3];
	float clipZ = world.x * viewmatrix.m[2][0] + world.y * viewmatrix.m[2][1] + world.z * viewmatrix.m[2][2] + viewmatrix.m[2][3];
	float clipW = world.x * viewmatrix.m[3][0] + world.y * viewmatrix.m[3][1] + world.z * viewmatrix.m[3][2] + viewmatrix.m[3][3];

	if (clipW <= 1e-6f)
		return { -1.0f, -1.0f };

	float inv_w = 1.0f / clipW;
	float ndcX = clipX * inv_w;
	float ndcY = clipY * inv_w;

	return {
		(dimensions.x / 2.0f) * (ndcX + 1.0f),
		(dimensions.y / 2.0f) * (1.0f - ndcY)
	};
}

static float get_magnitude(const math::vector2& a, const math::vector2& b)
{
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return std::sqrt(dx * dx + dy * dy);
}

static float get_effective_fov()
{
	if (!settings::silent::gun_based_fov)
		return settings::silent::fov;

	std::string tool_name;
	{
		std::lock_guard<std::recursive_mutex> lock(cache::mtx);
		tool_name = cache::cached_local_player.tool_name;
	}

	std::string tool_name_lower = tool_name;
	std::transform(tool_name_lower.begin(), tool_name_lower.end(), tool_name_lower.begin(), ::tolower);

	if (tool_name_lower.find("double-barrel") != std::string::npos || 
		tool_name_lower.find("double barrel") != std::string::npos ||
		tool_name_lower.find("doublebarrel") != std::string::npos)
		return settings::silent::fov_double_barrel;
	else if (tool_name_lower.find("tacticalshotgun") != std::string::npos ||
		tool_name_lower.find("tactical shotgun") != std::string::npos)
		return settings::silent::fov_tactical_shotgun;
	else if (tool_name_lower.find("revolver") != std::string::npos)
		return settings::silent::fov_revolver;

	return settings::silent::fov;
}

static rbx::part_t get_target_part(cache::entity_t& player, int aim_part)
{
	rbx::part_t target_part{};
	
	if (aim_part == 0)
	{
		auto head_it = player.parts.find("Head");
		if (head_it != player.parts.end())
			target_part = head_it->second;
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
		POINT cursor_point;
		HWND rblxWnd = FindWindowA(nullptr, "Roblox");
		if (rblxWnd && GetCursorPos(&cursor_point) && ScreenToClient(rblxWnd, &cursor_point))
		{
			math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };
			math::vector2 dimensions = game::visengine.get_dimensions();
			math::matrix4 viewmatrix = game::visengine.get_viewmatrix();
			
			float shortest_distance = std::numeric_limits<float>::max();
			
			for (auto& part_pair : player.parts)
			{
				rbx::primitive_t prim = part_pair.second.get_primitive();
				if (!prim.address)
					continue;
					
				math::vector3 part_position = prim.get_position();
				math::vector2 part_screen = world_to_screen(part_position, dimensions, viewmatrix);
				
				if (part_screen.x < 0 || part_screen.y < 0)
					continue;
					
				float distance = get_magnitude(part_screen, cursor);
				if (distance < shortest_distance)
				{
					shortest_distance = distance;
					target_part = part_pair.second;
				}
			}
		}
	}
	
		return target_part;
}

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

	bool ko_value = memory->read<bool>(ko.address + Offsets::Misc::Value);
	return ko_value;
}

static bool is_same_team(cache::entity_t& player)
{
	if (player.instance.address == 0 || cache::cached_local_player.instance.address == 0)
		return false;

	uint64_t player_team = memory->read<uint64_t>(player.instance.address + Offsets::Player::Team);
	uint64_t local_team = memory->read<uint64_t>(cache::cached_local_player.instance.address + Offsets::Player::Team);

	if (player_team != 0 && local_team != 0 && player_team == local_team)
		return true;

	return false;
}

static bool is_target_visible(cache::entity_t& player)
{
	if (player.instance.address == 0 || !game::workspace.address)
		return false;

	rbx::camera_t camera{ game::workspace.find_first_child_by_class("Camera").address };
	if (!camera.address)
		return true;

	math::vector3 camera_pos = camera.get_position();
	const char* check_parts[] = { "Head", "UpperTorso", "LowerTorso" };

	for (int i = 0; i < 3; i++) {
		auto part_it = player.parts.find(check_parts[i]);
		if (part_it == player.parts.end() || !part_it->second.address)
			continue;

		rbx::primitive_t primitive = part_it->second.get_primitive();
		if (!primitive.address)
			continue;

		math::vector3 target_pos = primitive.get_position();
		math::matrix3 camera_rot = camera.get_rotation();
		math::vector3 camera_look = { camera_rot.m[6], camera_rot.m[7], camera_rot.m[8] };

		math::vector3 to_target = { target_pos.x - camera_pos.x, target_pos.y - camera_pos.y, target_pos.z - camera_pos.z };
		float dot = to_target.x * camera_look.x + to_target.y * camera_look.y + to_target.z * camera_look.z;
		if (dot <= 0)
			continue;

		float dist = sqrt(to_target.x * to_target.x + to_target.y * to_target.y + to_target.z * to_target.z);
		if (dist <= 1000.0f)
			return true;
	}

	return false;
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

	rbx::part_t target_part = get_target_part(player, settings::silent::aim_part);
	if (!target_part.address)
		return false;

	rbx::primitive_t prim = target_part.get_primitive();
	if (!prim.address)
		return false;

	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 viewmatrix = game::visengine.get_viewmatrix();

	math::vector3 part_position = prim.get_position();
	math::vector2 part_screen = world_to_screen(part_position, dimensions, viewmatrix);

	if (part_screen.x < 0 || part_screen.y < 0)
		return false;

	float distance_from_cursor = get_magnitude(part_screen, cursor);
	return distance_from_cursor <= get_effective_fov();
}

static cache::entity_t get_closest_player_from_cursor()
{
	POINT cursor_point;
	HWND rblxWnd = FindWindowA(nullptr, "Roblox");
	if (!rblxWnd)
		return {};

	if (!GetCursorPos(&cursor_point))
		return {};

	if (!ScreenToClient(rblxWnd, &cursor_point))
		return {};

	math::vector2 cursor = { static_cast<float>(cursor_point.x), static_cast<float>(cursor_point.y) };

	std::vector<cache::entity_t> players_snapshot;
	{
		std::lock_guard<std::recursive_mutex> lock(cache::mtx);
		players_snapshot = cache::cached_players;
	}

	if (players_snapshot.empty())
		return {};

	cache::entity_t closest_player{};
	float shortest_distance = std::numeric_limits<float>::max();

	math::vector2 dimensions = game::visengine.get_dimensions();
	math::matrix4 viewmatrix = game::visengine.get_viewmatrix();

	for (cache::entity_t& player : players_snapshot)
	{
		if (player.instance.address == 0)
			continue;

		if (player.instance.address == game::local_player.address)
			continue;

		if (settings::silent::knocked_check && is_player_knocked(player))
			continue;

		if (settings::silent::team_check && is_same_team(player))
			continue;

		if (settings::silent::wall_check && !is_target_visible(player))
			continue;

		rbx::part_t target_part = get_target_part(player, settings::silent::aim_part);

		if (!target_part.address)
			continue;

		rbx::primitive_t prim = target_part.get_primitive();
		if (!prim.address)
			continue;

		math::vector3 part_position = prim.get_position();
		
		// Apply prediction if enabled
		if (settings::silent::prediction && prim.address)
		{
			math::vector3 velocity = prim.get_velocity();
			part_position.x += velocity.x * 0.1f; // Predict 100ms ahead
			part_position.y += velocity.y * 0.1f;
			part_position.z += velocity.z * 0.1f;
		}

		// Skip invalid coordinates (0, 0, 0) which usually mean the character hasn't spawned or is spectating
		if (std::abs(part_position.x) < 0.1f && std::abs(part_position.y) < 0.1f && std::abs(part_position.z) < 0.1f)
			continue;

		math::vector2 part_screen = world_to_screen(part_position, dimensions, viewmatrix);

		if (part_screen.x < 0 || part_screen.y < 0)
			continue;

		float distance_from_cursor = get_magnitude(part_screen, cursor);

		if (settings::silent::fov_check && distance_from_cursor > get_effective_fov())
			continue;

		if (distance_from_cursor < shortest_distance)
		{
			shortest_distance = distance_from_cursor;
			closest_player = player;
		}
	}

	return closest_player;
}


static std::uint64_t get_current_input_object(std::uint64_t base_address)
{
	return memory->read<std::uint64_t>(base_address + Offsets::MouseService::InputObject + sizeof(std::shared_ptr<void*>));
}

void c_silent_helper::set_frame_pos_x(std::uint64_t position)
{
	//memory->write<std::uint64_t>(address + Offsets::Silent::FramePositionOffsetX, position);
}

void c_silent_helper::set_frame_pos_y(std::uint64_t position)
{
	//memory->write<std::uint64_t>(address + Offsets::Silent::FramePositionOffsetY, position);
}

void c_silent_helper::initialize_mouse_service(std::uint64_t address)
{
	cached_input_object = get_current_input_object(address);

	if (cached_input_object && cached_input_object != 0xFFFFFFFFFFFFFFFF)
	{
		const char* base_pointer = reinterpret_cast<const char*>(cached_input_object);
		_mm_prefetch(base_pointer + Offsets::MouseService::MousePosition, _MM_HINT_T0);
		_mm_prefetch(base_pointer + Offsets::MouseService::MousePosition + sizeof(math::vector2), _MM_HINT_T0);
	}
}

void c_silent_helper::write_mouse_position(std::uint64_t address, float x, float y) 
{
	cached_input_object = get_current_input_object(address);
	if (cached_input_object != 0 && cached_input_object != 0xFFFFFFFFFFFFFFFF) 
	{
		math::vector2 new_position = { x, y };
		memory->write<math::vector2>(cached_input_object + Offsets::MouseService::MousePosition, new_position);
	}
}

static bool should_silent_aim_be_active()
{
	if (!settings::silent::enabled)
		return false;

	return g_silent_aim_locked;
}

static void update_silent_aim_key_state()
{
	// Always mode: bypass keybind check entirely
	if (settings::silent::keybind_mode == 2)
	{
		g_silent_aim_locked = true;
		return;
	}

	if (settings::silent::keybind == 0)
		return;

	static bool was_disabled_by_typing = false;

	if (check::textchatopen)
	{
		was_disabled_by_typing = true;
		g_silent_aim_locked = false;
		g_silent_cached_target = {};
		g_silent_found_target = false;
		g_silent_data_ready = false;
		return;
	}

	if (was_disabled_by_typing && !check::textchatopen)
	{
		g_silent_aim_locked = false;
		was_disabled_by_typing = false;
	}

	bool key_currently_pressed = (GetAsyncKeyState(settings::silent::keybind) & 0x8000) != 0;

	if (settings::silent::keybind_mode == 0)
	{
		if (key_currently_pressed && !g_silent_aim_locked)
		{
			g_silent_aim_locked = true;
		}
		else if (!key_currently_pressed && g_silent_aim_locked)
		{
			g_silent_aim_locked = false;
			g_silent_cached_target = {};
			g_silent_found_target = false;
			g_silent_data_ready = false;
		}
	}
	else
	{
		if (key_currently_pressed && !g_silent_aim_key_was_pressed)
		{
			if (!g_silent_aim_locked)
			{
				g_silent_aim_locked = true;
			}
			else
			{
				g_silent_aim_locked = false;
				g_silent_cached_target = {};
				g_silent_found_target = false;
				g_silent_data_ready = false;
			}
		}
	}

	g_silent_aim_key_was_pressed = key_currently_pressed;
}

void rbx::silent::silent_aim_1()
{
	cache::entity_t target{};

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	HWND roblox_window = FindWindowA(0, "Roblox");

	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));

		{
			auto ms = std::make_shared<rbx::instance_t>(game::datamodel.find_first_child_by_class("MouseService"));
			g_mouseservice = ms;
		}

		if (!g_mouseservice || !game::datamodel.address || !game::visengine.address)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		update_silent_aim_key_state();

		// Update aim instance size based on checkbox setting
		if (g_silent_aim_instance.address && g_silent_has_original_sizes)
		{
			if (settings::silent::enabled)
			{
				memory->write<math::vector2>(g_silent_aim_instance.address + Offsets::GuiObject::Size, { 0, 0 });
				std::vector<rbx::instance_t> aim_children = g_silent_aim_instance.get_children();
				for (rbx::instance_t& child : aim_children)
				{
					if (child.address)
					{
						memory->write<math::vector2>(child.address + Offsets::GuiObject::Size, { 0, 0 });
					}
				}
			}
			else
			{
				memory->write<math::vector2>(g_silent_aim_instance.address + Offsets::GuiObject::Size, g_silent_original_size);
				for (const auto& [child_addr, original_size] : g_silent_original_children_sizes)
				{
					memory->write<math::vector2>(child_addr + Offsets::GuiObject::Size, original_size);
				}
			}
		}

		if (!should_silent_aim_be_active())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			g_silent_data_ready = false;
			if (g_silent_cached_target.instance.address != 0)
			{
				g_silent_cached_target = {};
			}
			target = {};
			g_silent_found_target = false;
			g_silent_target_needs_reset = false;
			continue;
		}

		if (!game::datamodel.address)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		rbx::instance_t players = game::datamodel.find_first_child_by_class("Players");
		if (players.address == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		rbx::instance_t local_player = rbx::instance_t(memory->read<std::uint64_t>(players.address + Offsets::Player::LocalPlayer));

		if (local_player.address == 0)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		static int aim_instance_check_counter = 0;
		if (aim_instance_check_counter++ % 10 == 0)
		{
			try
			{
				rbx::instance_t player_gui = local_player.find_first_child("PlayerGui");
				if (player_gui.address != 0)
				{
					rbx::instance_t aim_frame{};
					std::vector<rbx::instance_t> children = player_gui.get_children();

					for (rbx::instance_t& child : children)
					{
						if (!child.address)
							continue;

						std::string child_name = child.get_name();
						if (child_name == "Aim")
						{
							aim_frame = child;
							break;
						}

						std::string child_class = child.get_class_name();
						if (child_class == "Frame" || child_class == "ScreenGui" || child_class == "GuiObject")
						{
							std::string child_lower = child_name;
							std::transform(child_lower.begin(), child_lower.end(), child_lower.begin(), ::tolower);

							if (child_lower.find("main") != std::string::npos)
							{
								std::vector<rbx::instance_t> grandchildren = child.get_children();
								for (rbx::instance_t& grandchild : grandchildren)
								{
									if (grandchild.address)
									{
										std::string grandchild_name = grandchild.get_name();
										if (grandchild_name == "Aim")
										{
											aim_frame = grandchild;
											break;
										}
									}
								}

								if (aim_frame.address)
									break;
							}
						}
					}

					if (aim_frame.address != g_silent_aim_instance.address)
					{
						if (g_silent_aim_instance.address && g_silent_has_original_sizes)
						{
							memory->write<math::vector2>(g_silent_aim_instance.address + Offsets::GuiObject::Size, g_silent_original_size);
							for (const auto& [child_addr, original_size] : g_silent_original_children_sizes)
							{
								memory->write<math::vector2>(child_addr + Offsets::GuiObject::Size, original_size);
							}
						}

						g_silent_aim_instance = aim_frame;
						g_silent_has_original_sizes = false;
						g_silent_original_children_sizes.clear();

						if (g_silent_aim_instance.address)
						{
							g_silent_original_size = memory->read<math::vector2>(g_silent_aim_instance.address + Offsets::GuiObject::Size);
							std::vector<rbx::instance_t> aim_children = g_silent_aim_instance.get_children();
							for (rbx::instance_t& child : aim_children)
							{
								if (child.address)
								{
									math::vector2 child_size = memory->read<math::vector2>(child.address + Offsets::GuiObject::Size);
									g_silent_original_children_sizes.push_back({ child.address, child_size });
								}
							}
							g_silent_has_original_sizes = true;
						}
					}
				}
			}
			catch (...)
			{
				g_silent_aim_instance = rbx::instance_t{};
			}
		}

		bool always_mode = (settings::silent::keybind_mode == 2);
		
		if (!g_silent_found_target || g_silent_cached_target.instance.address == 0)
		{
			target = get_closest_player_from_cursor();
			
			g_silent_cached_last_target = target;
			
			rbx::part_t target_part = get_target_part(target, settings::silent::aim_part);
			g_silent_found_target = (target_part.address != 0);
			g_silent_cached_target = target;
		}
		else
		{
			if (always_mode || !settings::silent::sticky_aim)
			{
				target = get_closest_player_from_cursor();
				
				g_silent_cached_last_target = target;
				
				rbx::part_t target_part = get_target_part(target, settings::silent::aim_part);
				g_silent_found_target = (target_part.address != 0);
				g_silent_cached_target = target;
			}
			else
			{
				target = g_silent_cached_target;
				
				if (settings::silent::fov_check && game::visengine.address)
				{
					if (!is_target_within_fov(g_silent_cached_target))
					{
						g_silent_found_target = false;
						g_silent_cached_target = {};
						target = {};
						continue;
					}
				}
			}
		}

		if (g_silent_found_target && g_silent_cached_target.instance.address != 0 && game::visengine.address)
		{
			if (settings::silent::knocked_check && is_player_knocked(g_silent_cached_target))
			{
				g_silent_found_target = false;
				g_silent_cached_target = {};
				continue;
			}

			rbx::part_t target_part = get_target_part(g_silent_cached_target, settings::silent::aim_part);
			
			if (target_part.address != 0)
			{
				rbx::primitive_t prim = target_part.get_primitive();
				if (prim.address)
				{
					math::vector3 part_3d = prim.get_position();
					math::matrix4 view = game::visengine.get_viewmatrix();
					math::vector2 dims = game::visengine.get_dimensions();

					g_silent_partpos = world_to_screen(part_3d, dims, view);
					POINT cursor_point;
					GetCursorPos(&cursor_point);
					if (roblox_window)
						ScreenToClient(roblox_window, &cursor_point);

					g_silent_cached_position_x = static_cast<std::uint64_t>(cursor_point.x);
					g_silent_cached_position_y = static_cast<std::uint64_t>(dims.y - std::abs(dims.y - static_cast<float>(cursor_point.y)) - 58);
					g_silent_data_ready = true;
				}
				else
				{
					g_silent_data_ready = false;
				}
			}
			else
			{
				g_silent_data_ready = false;
			}
		}
		else
		{
			g_silent_data_ready = false;
		}
	}
}

void rbx::silent::silent_aim_2()
{
	c_silent_helper mouse_service_instance{};
	bool mouse_service_initialized = false;

	for (;;)
	{
		if (!g_mouseservice)
		{
			mouse_service_initialized = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}

		if (!should_silent_aim_be_active())
		{
			mouse_service_initialized = false;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			{
				std::lock_guard<std::mutex> lock(silent_mtx);
				if (g_silent_cached_target.instance.address != 0)
					g_silent_target_needs_reset = true;
			}
			continue;
		}

		if (g_silent_data_ready && g_mouseservice && g_mouseservice->address != 0)
		{
			rbx::instance_t target_copy{};
			math::vector2 partpos_copy{};
			{
				std::lock_guard<std::mutex> lock(silent_mtx);
				if (g_silent_cached_target.instance.address == 0) continue;
				target_copy = g_silent_cached_target;
				partpos_copy = g_silent_partpos;
			}
			
			if (partpos_copy.x < 0.0f || partpos_copy.y < 0.0f ||
				partpos_copy.x > 10000.0f || partpos_copy.y > 10000.0f)
			{
				continue;
			}

			try
			{
				if (!mouse_service_initialized)
				{
					mouse_service_instance.initialize_mouse_service(g_mouseservice->address);
					mouse_service_initialized = true;
				}

				mouse_service_instance.write_mouse_position(g_mouseservice->address, partpos_copy.x, partpos_copy.y);
			}
			catch (...)
			{
				mouse_service_initialized = false;
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
		}
	}
}

void rbx::silent::initialize()
{
	std::thread(silent_aim_1).detach();
	std::thread(silent_aim_2).detach();
}