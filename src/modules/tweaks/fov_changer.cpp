#include "fov_changer.h"

#include <Windows.h>
#include <thread>
#include <chrono>

#include <runtime/memory.h>
#include <native/sdk.h>
#include <native/offsets.h>
#include <session/game.h>
#include <settings.h>

void fov_changer::run()
{
    static bool last_enabled = false;
    for (;;)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 100Hz refresh to beat game scripts

        if (!game::workspace.address)
            continue;

        // Get Camera directly from Workspace pointer
        std::uint64_t camera_ptr = memory->read<std::uint64_t>(game::workspace.address + Offsets::Workspace::CurrentCamera);
        if (!camera_ptr)
            continue;

        if (!settings::expl::fov_changer) {
            if (last_enabled) {
                memory->write<float>(camera_ptr + Offsets::Camera::FieldOfView, 70.0f);
                last_enabled = false;
            }
            continue;
        }

        last_enabled = true;

        // Set FOV
        memory->write<float>(camera_ptr + Offsets::Camera::FieldOfView, settings::expl::fov_value);
    }
}
