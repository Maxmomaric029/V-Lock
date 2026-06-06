#define NOMINMAX
#include <Windows.h>
#include <thread>
#include <chrono>
#include <mutex>

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <buffer/cache.h>
#include <session/game.h>
#include <settings.h>
#include <verify/typing_check.h>
#include "hitbox_expander.h"

void rbx::hitbox_expander::run()
{
	for (;;) {
		Sleep(1);
		static int frame_counter = 0;
		frame_counter++;
		static bool was_disabled_by_typing = false;
		static bool previous_enabled_setting = false;

		if (check::textchatopen)
		{
			was_disabled_by_typing = true;
			std::lock_guard<std::recursive_mutex> lock(cache::mtx);

			for (auto& player : cache::cached_players)
			{
				if (!player.instance.address) continue;

				if (player.instance.address == cache::cached_local_player.instance.address)
					continue;

				auto root_part_it = player.parts.find("HumanoidRootPart");
				if (root_part_it == player.parts.end())
					continue;

				rbx::part_t root_part = root_part_it->second;
				if (!root_part.address) continue;

				rbx::primitive_t prim = root_part.get_primitive();
				if (!prim.address) continue;

				math::vector3 normal_size = { 2.0f, 2.0f, 1.0f };
				prim.set_size(normal_size);
				prim.set_can_collide(false);
			}
			continue;
		}

		if (was_disabled_by_typing && !check::textchatopen)
		{
			if (previous_enabled_setting != settings::hitbox_expander::enabled)
			{
				was_disabled_by_typing = false;
			}
			else
			{
				settings::hitbox_expander::enabled = false;
				std::lock_guard<std::recursive_mutex> lock(cache::mtx);

				for (auto& player : cache::cached_players)
				{
					if (!player.instance.address) continue;

					if (player.instance.address == cache::cached_local_player.instance.address)
						continue;

					auto root_part_it = player.parts.find("HumanoidRootPart");
					if (root_part_it == player.parts.end())
						continue;

					rbx::part_t root_part = root_part_it->second;
					if (!root_part.address) continue;

					rbx::primitive_t prim = root_part.get_primitive();
					if (!prim.address) continue;

					math::vector3 normal_size = { 2.0f, 2.0f, 1.0f };
					prim.set_size(normal_size);
					prim.set_can_collide(false);
				}
				continue;
			}
		}

		previous_enabled_setting = settings::hitbox_expander::enabled;

		if (!settings::hitbox_expander::enabled)
		{
			std::lock_guard<std::recursive_mutex> lock(cache::mtx);

			for (auto& player : cache::cached_players)
			{
				if (!player.instance.address) continue;

				if (player.instance.address == cache::cached_local_player.instance.address)
					continue;

				auto root_part_it = player.parts.find("HumanoidRootPart");
				if (root_part_it == player.parts.end())
					continue;

				rbx::part_t root_part = root_part_it->second;
				if (!root_part.address) continue;

				rbx::primitive_t prim = root_part.get_primitive();
				if (!prim.address) continue;

				math::vector3 normal_size = { 2.0f, 2.0f, 1.0f };
				prim.set_size(normal_size);
				prim.set_can_collide(false);
			}
			continue;
		}

		if (frame_counter % 5 != 0)
			continue;

		if (!game::datamodel.address || !game::local_player.address)
		{
			continue;
		}

		if (settings::hitbox_expander::size_x <= 0.0f ||
			settings::hitbox_expander::size_y <= 0.0f ||
			settings::hitbox_expander::size_z <= 0.0f)
		{
			continue;
		}

		std::lock_guard<std::recursive_mutex> lock(cache::mtx);

		for (auto& player : cache::cached_players)
		{
			if (!player.instance.address) continue;

			if (player.instance.address == cache::cached_local_player.instance.address)
				continue;

			auto root_part_it = player.parts.find("HumanoidRootPart");
			if (root_part_it == player.parts.end()) continue;

			rbx::part_t root_part = root_part_it->second;
			if (!root_part.address) continue;

			rbx::primitive_t prim = root_part.get_primitive();
			if (!prim.address) continue;

			math::vector3 new_size = {
				settings::hitbox_expander::size_x,
				settings::hitbox_expander::size_y,
				settings::hitbox_expander::size_z
			};

			try {
				prim.set_size(new_size);
				prim.set_can_collide(false);
			}
			catch (...) {
				continue;
			}
		}
	}
}

void rbx::hitbox_expander::initialize()
{
}

