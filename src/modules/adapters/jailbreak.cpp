#include <string>
#include <cmath>
#include "jailbreak.h"
#include <thread>
#include <chrono>
#include <vector>
#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>

void jailbreak::run()
{
	for (;;)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		if (game::place_id != 606849621) // Only run if in Jailbreak
			continue;

		if (settings::games::jailbreak::instant_interact)
		{
			// Instant Interact: Scan Workspace for ProximityPrompts
			// This is a simplified version; in a real scenario, we might want to be more selective
			std::vector<rbx::instance_t> descendants;
			
			// We scan Workspace and common containers
			std::vector<rbx::instance_t> root_containers = { game::workspace };
			
			for (auto& root : root_containers)
			{
				auto children = root.get_children();
				for (auto& child : children)
				{
					std::string class_name = child.get_class_name();
					if (class_name == "ProximityPrompt")
					{
						memory->write<float>(child.address + Offsets::ProximityPrompt::HoldDuration, 0.0f);
					}
					else if (class_name == "Model" || class_name == "Folder" || class_name == "Part")
					{
						// Shallow recursive scan (1 level deep) for performance
						auto sub_children = child.get_children();
						for (auto& sub : sub_children)
						{
							if (sub.get_class_name() == "ProximityPrompt")
							{
								memory->write<float>(sub.address + Offsets::ProximityPrompt::HoldDuration, 0.0f);
							}
						}
					}
				}
			}
		}

		if (settings::games::jailbreak::no_fall)
		{
			// Simple No Fall logic
			rbx::player_t lp(game::local_player.address);
			rbx::model_instance_t model = lp.get_model_instance();
			if (model.address != 0)
			{
				rbx::humanoid_t humanoid = { model.find_first_child_by_class("Humanoid").address };
				if (humanoid.address != 0)
				{
					rbx::instance_t hrp = model.find_first_child("HumanoidRootPart");
					if (hrp.address != 0)
					{
						rbx::primitive_t prim = rbx::part_t(hrp.address).get_primitive();
						math::vector3 vel = prim.get_velocity();
						
						// If falling too fast, zero out Y velocity right before hitting ground
						// (This is a basic version, real ones check floor material)
						if (vel.y < -50.0f) 
						{
							vel.y = 0.0f;
							memory->write<math::vector3>(prim.address + Offsets::Primitive::AssemblyLinearVelocity, vel);
						}
					}
				}
			}
		}
	}
}
