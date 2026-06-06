#include "anti_afk.h"

#include <Windows.h>
#include <thread>
#include <chrono>

#include <session/game.h>
#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <settings.h>

void anti_afk::run()
{
    for (;;)
    {
        std::this_thread::sleep_for(std::chrono::seconds(30));

        if (!settings::expl::anti_afk)
            continue;

        if (!game::workspace.address)
            continue;

        // Send a dummy mouse move to prevent AFK kick
        INPUT input = { 0 };
        input.type = INPUT_MOUSE;
        input.mi.dwFlags = MOUSEEVENTF_MOVE;
        input.mi.dx = 1;
        input.mi.dy = 1;
        SendInput(1, &input, sizeof(INPUT));

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        input.mi.dx = -1;
        input.mi.dy = -1;
        SendInput(1, &input, sizeof(INPUT));
    }
}
