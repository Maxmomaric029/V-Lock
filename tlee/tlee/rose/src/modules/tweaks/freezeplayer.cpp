#include "freezeplayer.h"

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>
#include <verify/typing_check.h>
#include <Windows.h>
#include <thread>
#include <chrono>
#include <iostream>

namespace freezeplayer
{
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

	void run()
	{
		static bool key_was_pressed = false;
		static bool toggle_state = false;
		static bool was_disabled_by_typing = false;
		
		static math::vector3 freeze_pos;
		static math::matrix3 freeze_rot;
		static bool is_frozen = false;

		for (;;)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			if (check::textchatopen)
			{
				was_disabled_by_typing = true;
				toggle_state = false;
				is_frozen = false;
				continue;
			}
			else if (was_disabled_by_typing)
			{
				toggle_state = false;
				was_disabled_by_typing = false;
			}

			if (!settings::expl::freeze_player)
			{
				toggle_state = false;
				if (is_frozen) {
					// Reset anchored state if we were in anchor mode
					if (game::local_player.address != 0) {
						rbx::player_t lp(game::local_player.address);
						rbx::model_instance_t model = lp.get_model_instance();
						if (model.address != 0) {
							rbx::instance_t hrp = model.find_first_child("HumanoidRootPart");
							if (hrp.address != 0) {
								rbx::part_t(hrp.address).get_primitive().set_anchored(false);
							}
						}
					}
					is_frozen = false;
				}
				continue;
			}

			bool active = is_key_active(settings::expl::freeze_player_keybind, settings::expl::freeze_player_keybind_mode, key_was_pressed, toggle_state);

			if (game::local_player.address == 0) continue;

			rbx::player_t local_player_obj = { game::local_player.address };
			rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
			if (model_instance.address == 0) continue;

			rbx::instance_t hrp = model_instance.find_first_child("HumanoidRootPart");
			if (hrp.address == 0) continue;

			rbx::part_t part(hrp.address);
			rbx::primitive_t prim = part.get_primitive();
			if (prim.address == 0) continue;

			if (active)
			{
				if (!is_frozen)
				{
					freeze_pos = prim.get_position();
					freeze_rot = prim.get_rotation();
					is_frozen = true;
				}

				int mode = settings::expl::freeze_player_mode;

				if (mode == 0) // Anchor
				{
					prim.set_anchored(true);
					// Also zero out velocity for safety
					prim.set_velocity(math::vector3(0, 0, 0));
					prim.set_angular_velocity(math::vector3(0, 0, 0));
				}
				else if (mode == 1) // Velocity Lock
				{
					prim.set_anchored(false);
					// Zero velocity
					prim.set_velocity(math::vector3(0, 0, 0));
					prim.set_angular_velocity(math::vector3(0, 0, 0));
				}
				else if (mode == 2) // Position Lock
				{
					prim.set_anchored(false);
					// Lock position but allow rotation (client will still try to rotate)
					prim.set_position(freeze_pos);
					prim.set_velocity(math::vector3(0, 0, 0));
					prim.set_angular_velocity(math::vector3(0, 0, 0));
				}
			}
			else
			{
				if (is_frozen)
				{
					prim.set_anchored(false);
					is_frozen = false;
				}
			}
		}
	}
}

