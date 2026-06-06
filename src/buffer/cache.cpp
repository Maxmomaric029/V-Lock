#define NOMINMAX
#include "cache.h"
#include <thread>
#include <session/game.h>
#include <settings.h>
#include <cmath>
#include <algorithm>

void cache::run()
{
	while (true)
	{
		// Retry resolving Players and LocalPlayer if they weren't ready during startup
		std::uint64_t dm_addr = 0;
		std::uint64_t players_addr = 0;
		{
			std::shared_lock slock(game::game_state_mtx);
			dm_addr = game::datamodel.address;
			players_addr = game::players.address;
		}
		if (players_addr == 0 && dm_addr != 0)
		{
			rbx::instance_t dm_inst{ dm_addr };
			auto players_inst = dm_inst.find_first_child_by_class("Players");
			if (players_inst.address)
			{
				{
					std::unique_lock lock(game::game_state_mtx);
					game::players = players_inst;
				}

				std::uint64_t local_player_addr = memory->read<std::uint64_t>(players_inst.address + Offsets::Player::LocalPlayer);
				if (memory->is_valid_instance_address(local_player_addr))
				{
					{
						std::unique_lock lock(game::game_state_mtx);
						game::local_player = { local_player_addr };
					}
				}
			}
		}

		{
			std::uint64_t lp_addr = 0;
			{
				std::shared_lock slock(game::game_state_mtx);
				lp_addr = game::local_player.address;
			}
			if (lp_addr != 0)
			{
				rbx::player_t local_player_obj{ lp_addr };
				auto char_addr = local_player_obj.get_model_instance().address;
				if (char_addr != 0)
				{
					std::unique_lock ulock(game::game_state_mtx);
					game::local_character = { char_addr };
				}
			}
		}

		// Detect Game
		if (dm_addr != 0)
		{
			game::place_id = memory->read<std::int64_t>(dm_addr + Offsets::DataModel::PlaceId);
			
			// Map popular games
			static std::int64_t last_id = 0;
			if (game::place_id != last_id)
			{
				last_id = game::place_id;
				switch (game::place_id)
				{
				case 606849621:   game::game_name = "Jailbreak"; break;
				case 2753915549:  game::game_name = "Blox Fruits"; break;
				case 920587237:   game::game_name = "Adopt Me"; break;
				case 4924144171:  game::game_name = "Brookhaven"; break;
				case 142823291:   game::game_name = "Murder Mystery 2"; break;
				case 2788229376:  game::game_name = "Da Hood"; break;
				case 160331737:   game::game_name = "Frontlines"; break;
				case 6872265039:  game::game_name = "BedWars"; break;
				case 155615604:   game::game_name = "Prison Life"; break;
				case 2317712696:  game::game_name = "Wild West"; break;
				case 3237341397:  game::game_name = "Pet Simulator X"; break;
				case 1054526971:  game::game_name = "Blackhawk Rescue Mission 5"; break;
				case 3351323050:  game::game_name = "Combat Warriors"; break;
				case 1215132535:  game::game_name = "Mad City"; break;
				case 2534724415:  game::game_name = "Emergency Response: Liberty County"; break;
				default:          game::game_name = "Roblox (" + std::to_string(game::place_id) + ")"; break;
				}
			}
		}

		std::vector<rbx::player_t> players = game::players.get_children<rbx::player_t>();

		// === DIAGNÓSTICO: cuántos players encontró (solo 1 vez) ===
		static bool cache_printed = false;
		if (players.size() > 0 && !cache_printed) {
			cache_printed = true;
			printf("\x1b[38;5;118m   [CACHE] Found %zu players!\x1b[0m\n", players.size());
			for (auto& p : players) {
				printf("\x1b[38;5;118m   [CACHE]   player 0x%llx\x1b[0m\n", (unsigned long long)p.address);
			}
		}

		std::vector<cache::entity_t> temp_cache;
		
		for (rbx::player_t& player : players)
		{
			cache::entity_t entity{};

			entity.instance = { player.address };
			entity.name = player.get_name();
			entity.user_id = player.get_user_id();
			entity.team_address = memory->read<std::uint64_t>(player.address + Offsets::Player::Team);
			entity.team_name = "";
			if (entity.team_address != 0)
			{
				entity.team_name = rbx::instance_t{ entity.team_address }.get_name();
			}
			
			rbx::model_instance_t model_instance = player.get_model_instance();

			for (rbx::part_t& part : model_instance.get_children<rbx::part_t>())
			{
				std::string part_class = part.get_class_name();
				if (part_class.find("Part") != std::string::npos)
				{
					entity.parts[part.get_name()] = part;
				}
			}

			entity.humanoid = { model_instance.find_first_child("Humanoid").address };
			entity.rig_type = entity.humanoid.get_rig_type();
			
			// User detection check
			entity.is_user = false;
			if (entity.humanoid.address != 0)
			{
				float hip = entity.humanoid.get_hip_height();
				// Secret signature check (2.01337)
				if (std::abs(hip - 2.01337f) < 0.0001f)
				{
					entity.is_user = true;
				}
			}

			rbx::instance_t backpack = player.find_first_child("Backpack");
			rbx::instance_t character_model = { model_instance.address };
			
			entity.tool_name = "";
			for (rbx::instance_t& child : character_model.get_children<rbx::instance_t>())
			{
				std::string child_class = child.get_class_name();
				if (child_class == "Tool" || child_class == "HopperBin")
				{
					entity.tool_name = child.get_name();
					break;
				}
			}

			temp_cache.push_back(entity);
		}
		std::vector<cache::primitive_data_t> temp_primitives;
		if (settings::aimbot::wall_check || settings::visuals::visible_check)
		{
			std::uint64_t world_addr = memory->read<std::uint64_t>(game::workspace.address + Offsets::Workspace::World);
			if (world_addr)
			{
				std::uint64_t prim_start = memory->read<std::uint64_t>(world_addr + Offsets::World::Primitives);
				std::uint64_t prim_end   = memory->read<std::uint64_t>(world_addr + Offsets::World::Primitives + 0x8);
			
			if (prim_start && prim_end && prim_end > prim_start)
				{
					std::uint64_t count = (prim_end - prim_start) / sizeof(std::uint64_t);
					if (count > 4096) count = 4096;

					std::unordered_map<std::uint64_t, bool> player_prim_addrs;
					for (auto& ent : temp_cache)
					{
						for (auto& kv : ent.parts)
						{
							if (!kv.second.address) continue;
							std::uint64_t prim_addr = memory->read<std::uint64_t>(kv.second.address + Offsets::BasePart::Primitive);
							if (prim_addr) player_prim_addrs[prim_addr] = true;
						}
					}

					for (std::uint64_t i = 0; i < count; ++i)
					{
						std::uint64_t prim_ptr = memory->read<std::uint64_t>(prim_start + i * sizeof(std::uint64_t));
						if (!memory->is_valid_instance_address(prim_ptr) || player_prim_addrs.count(prim_ptr))
							continue;

						std::uint8_t flags = memory->read<std::uint8_t>(prim_ptr + Offsets::Primitive::Flags);
						if (!(flags & Offsets::PrimitiveFlags::CanCollide))
							continue;

						math::vector3 prim_pos  = memory->read<math::vector3>(prim_ptr + Offsets::Primitive::Position);
						math::vector3 prim_size = memory->read<math::vector3>(prim_ptr + Offsets::Primitive::Size);

						if (!std::isfinite(prim_pos.x) || !std::isfinite(prim_size.x))
							continue;
						if (prim_size.x <= 0.f || prim_size.y <= 0.f || prim_size.z <= 0.f)
							continue;

						math::vector3 half{ prim_size.x * 0.5f, prim_size.y * 0.5f, prim_size.z * 0.5f };
						temp_primitives.push_back({ prim_pos, half });
					}
				}
			}
		}

		{
			std::lock_guard<std::recursive_mutex> lock(mtx);
			cached_players = std::move(temp_cache);
			cached_primitives = std::move(temp_primitives);
			
			for (cache::entity_t& entity : cached_players)
			{
				if (entity.instance.address == game::local_player.address)
				{
					cached_local_player = entity;
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

bool cache::has_wall_between(const math::vector3& camera_pos, const math::vector3& target_pos)
{
	math::vector3 dir = { target_pos.x - camera_pos.x, target_pos.y - camera_pos.y, target_pos.z - camera_pos.z };
	float distance = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
	if (distance < 1.0f) return false;

	// Precision-friendly inverse direction
	math::vector3 inv_dir = { 
		(std::abs(dir.x) > 1e-6f) ? 1.0f / dir.x : 1e6f,
		(std::abs(dir.y) > 1e-6f) ? 1.0f / dir.y : 1e6f,
		(std::abs(dir.z) > 1e-6f) ? 1.0f / dir.z : 1e6f
	};

	std::lock_guard<std::recursive_mutex> lock(cache::mtx);
	if (cached_primitives.empty()) return false;

	for (const auto& prim : cached_primitives)
	{
		float t1 = (prim.pos.x - prim.half_size.x - camera_pos.x) * inv_dir.x;
		float t2 = (prim.pos.x + prim.half_size.x - camera_pos.x) * inv_dir.x;
		float t3 = (prim.pos.y - prim.half_size.y - camera_pos.y) * inv_dir.y;
		float t4 = (prim.pos.y + prim.half_size.y - camera_pos.y) * inv_dir.y;
		float t5 = (prim.pos.z - prim.half_size.z - camera_pos.z) * inv_dir.z;
		float t6 = (prim.pos.z + prim.half_size.z - camera_pos.z) * inv_dir.z;

		float tmin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
		float tmax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

		// Check if the intersection interval is valid and in front of the camera
		if (tmax < 0 || tmin > tmax) continue;
        
        // t is normalized distance [0, 1]. 
		// We ignore objects very close to camera (0.01) and the target itself (0.99)
        if (tmin > 0.01f && tmin < 0.99f)
            return true;
	}

	return false;
}