#include "walkspeed.h"

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>
#include <verify/typing_check.h>

namespace walkspeed
{
    void run()
    {
        for (;;)
        {
            Sleep(10);
            static bool original_speed_set = false;
            static float original_speed = 16.0f;
            static bool was_disabled_by_typing = false;
            static bool previous_walkspeed_setting = false;

            if (check::textchatopen)
            {
                was_disabled_by_typing = true;
                original_speed_set = false;

                {
                    std::shared_lock lock(game::game_state_mtx);
                    if (game::local_player.address != 0)
                    {
                        rbx::player_t local_player_obj = { game::local_player.address };
                        rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
                        if (model_instance.address != 0)
                        {
                            rbx::humanoid_t humanoid = { model_instance.find_first_child("Humanoid").address };
                            if (humanoid.address != 0 && original_speed_set)
                            {
                                if (!memory->write<float>(humanoid.address + Offsets::Humanoid::Walkspeed, original_speed))
                                    printf("\x1b[38;5;196m   [!] Walkspeed: write failed (restore, textchat)\x1b[0m\n");
                                memory->write<float>(humanoid.address + Offsets::Humanoid::WalkspeedCheck, original_speed);
                            }
                        }
                    }
                }
                continue;
            }

            if (was_disabled_by_typing && !check::textchatopen)
            {
                if (previous_walkspeed_setting != settings::expl::walkspeed)
                {
                    was_disabled_by_typing = false;
                }
                else
                {
                    settings::expl::walkspeed = false;
                    continue;
                }
            }

            previous_walkspeed_setting = settings::expl::walkspeed;

            if (!settings::expl::walkspeed)
            {
                original_speed_set = false;

                {
                    std::shared_lock lock(game::game_state_mtx);
                    if (game::local_player.address != 0)
                    {
                        rbx::player_t local_player_obj = { game::local_player.address };
                        rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
                        if (model_instance.address != 0)
                        {
                            rbx::humanoid_t humanoid = { model_instance.find_first_child("Humanoid").address };
                            if (humanoid.address != 0 && original_speed_set)
                            {
                                if (!memory->write<float>(humanoid.address + Offsets::Humanoid::Walkspeed, original_speed))
                                    printf("\x1b[38;5;196m   [!] Walkspeed: write failed (restore, disabled)\x1b[0m\n");
                                memory->write<float>(humanoid.address + Offsets::Humanoid::WalkspeedCheck, original_speed);
                            }
                        }
                    }
                }
                continue;
            }

            std::uint64_t lp_addr;
            {
                std::shared_lock lock(game::game_state_mtx);
                lp_addr = game::local_player.address;
            }
            rbx::player_t local_player_obj = { lp_addr };
            if (local_player_obj.address == 0)
                continue;

            rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
            if (model_instance.address == 0)
                continue;

            rbx::humanoid_t humanoid = { model_instance.find_first_child("Humanoid").address };
            if (humanoid.address == 0)
                continue;

            bool should_activate = false;

            switch (settings::expl::walkspeed_mode)
            {
            case 0:
                should_activate = true;
                break;
            case 1:
            {
                rbx::instance_t body_effects = model_instance.find_first_child("BodyEffects");
                if (body_effects.address != 0)
                {
                    rbx::instance_t reload = body_effects.find_first_child("Reload");
                    if (reload.address != 0)
                    {
                        bool reload_value = memory->read<bool>(reload.address + Offsets::Misc::Value);
                        should_activate = reload_value;
                    }
                }
                break;
            }
            case 2:
            {
                float health = humanoid.get_health();
                should_activate = (health < settings::expl::walkspeed_health_threshold);
                break;
            }
            }

            if (should_activate)
            {
                if (!original_speed_set)
                {
                    original_speed = memory->read<float>(humanoid.address + Offsets::Humanoid::Walkspeed);
                    original_speed_set = true;
                }

                float current_speed = memory->read<float>(humanoid.address + Offsets::Humanoid::Walkspeed);
                if (current_speed != settings::expl::walkspeed_speed)
                {
                    if (!memory->write<float>(humanoid.address + Offsets::Humanoid::Walkspeed, settings::expl::walkspeed_speed))
                        printf("\x1b[38;5;196m   [!] Walkspeed: write failed (activate, mode %d)\x1b[0m\n", settings::expl::walkspeed_mode);
                    memory->write<float>(humanoid.address + Offsets::Humanoid::WalkspeedCheck, settings::expl::walkspeed_speed);
                }
            }
            else
            {
                if (original_speed_set)
                {
                    if (!memory->write<float>(humanoid.address + Offsets::Humanoid::Walkspeed, original_speed))
                        printf("\x1b[38;5;196m   [!] Walkspeed: write failed (restore, deactivate)\x1b[0m\n");
                    memory->write<float>(humanoid.address + Offsets::Humanoid::WalkspeedCheck, original_speed);
                }
            }

            if (settings::expl::jump_height_enabled && humanoid.address != 0)
            {
                // Force the engine to use JumpPower instead of JumpHeight
                if (!memory->write<bool>(humanoid.address + Offsets::Humanoid::UseJumpPower, true))
                    printf("\x1b[38;5;196m   [!] Walkspeed: UseJumpPower write failed\x1b[0m\n");
                
                memory->write<float>(humanoid.address + Offsets::Humanoid::JumpPower, settings::expl::jump_height);
                memory->write<float>(humanoid.address + Offsets::Humanoid::JumpHeight, settings::expl::jump_height);
            }
        }
    }
}
