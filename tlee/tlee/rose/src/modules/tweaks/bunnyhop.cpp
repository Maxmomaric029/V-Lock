#include "bunnyhop.h"

#include <Windows.h>
#include <thread>
#include <chrono>

#include <settings.h>
#include <session/game.h>
#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>

static bool is_grounded()
{
    if (!game::local_player.address)
        return false;

    rbx::player_t local_player_obj(game::local_player.address);
    rbx::model_instance_t model_instance = local_player_obj.get_model_instance();
    if (model_instance.address == 0)
        return false;

    rbx::instance_t humanoid_instance = model_instance.find_first_child("Humanoid");
    if (humanoid_instance.address == 0)
        return false;

    uint64_t floor_material = memory->read<uint64_t>(humanoid_instance.address + Offsets::Humanoid::FloorMaterial);
    return (floor_material != 0); // 0 = Air, else grounded
}

namespace bunnyhop
{
    void run()
    {
        for (;;)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if (!settings::expl::bunnyhop)
                continue;

            if (!is_grounded())
            {
                // In air, do nothing
                continue;
            }

            // Just landed, jump again
            if (GetAsyncKeyState(settings::expl::bunnyhop_keybind) & 0x8000)
            {
                // Simulate space press
                INPUT input = { 0 };
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = VK_SPACE;
                SendInput(1, &input, sizeof(INPUT));

                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                input.ki.dwFlags = KEYEVENTF_KEYUP;
                SendInput(1, &input, sizeof(INPUT));
            }
        }
    }
}
