#include <cstdint>
#include <chrono>
#include <thread>
#include <string>
#include <cstdio>
#include <Windows.h>

#include <runtime/memory.h>
#include <native/offsets.h>
#include <native/sdk.h>
#include <session/game.h>
#include <buffer/cache.h>
#include <display/render.h>
#include <display/notifications.h>
#include <modules/assist/silent.h>
#include <modules/tweaks/walkspeed.h>
#include <modules/tweaks/freezeplayer.h>
#include <modules/tweaks/hitbox_expander.h>
#include <modules/tweaks/fly.h>
#include <modules/tweaks/triggerbot.h>
#include <modules/tweaks/bunnyhop.h>
#include <modules/tweaks/fov_changer.h>
#include <modules/tweaks/anti_afk.h>
#include <modules/tweaks/misc.h>
#include <modules/targeting/aimbot.h>
#include <modules/adapters/jailbreak.h>
#include <verify/typing_check.h>
#include <session/rescan.h>
#include <workarounds/kill_crash_handler.h>
#include "../protection/protection/antidebug.h"

// Prevent console from minimizing
WNDPROC oldWndProc = nullptr;
LRESULT CALLBACK ConsoleWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_SYSCOMMAND && wParam == SC_MINIMIZE)
        return 0; // Block minimize
    return CallWindowProc(oldWndProc, hwnd, uMsg, wParam, lParam);
}

std::int32_t main()
{
	std::thread(debugger_detection).detach();
	std::thread(rbx::bypass::run).detach();

	// Using hardcoded offsets from offsets.h directly
	printf("\x1b[38;5;45m   [\xbb] \x1b[0m\x1b[38;5;118mUsing built-in offsets (version %s)\x1b[0m\n", Offsets::ClientVersion.c_str());


    // Set UTF-8 encoding for cool characters
	SetConsoleCP(65001);
	SetConsoleOutputCP(65001);

	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(hConsole, &mode);
    SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); // Enable ANSI codes

	system("cls");

    // Futuristic Logo
    printf("\x1b[38;5;45m");
    printf("\n");
    printf("     ██╗   ██╗███████╗██████╗ ████████╗███████╗██╗  ██╗\n");
    printf("     ██║   ██║██╔════╝██╔══██╗╚══██╔══╝██╔════╝╚██╗██╔╝\n");
    printf("     ██║   ██║█████╗  ██████╔╝   ██║   █████╗   ╚███╔╝ \n");
    printf("     ╚██╗ ██╔╝██╔══╝  ██╔══██╗   ██║   ██╔══╝   ██╔██╗ \n");
    printf("      ╚████╔╝ ███████╗██║  ██║   ██║   ███████╗██╔╝ ██╗\n");
    printf("       ╚═══╝  ╚══════╝╚═╝  ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝\n");
    printf("\n");

    printf("\x1b[38;5;171m   >─────────────────────────────────────────────────────────<\n\x1b[0m");
	printf("\x1b[38;5;118m            [ SYSTEM READY ] \x1b[0m \x1b[38;5;45m vertex External v1.0\n\x1b[0m");
	printf("\x1b[38;5;45m            Developed by tormix | Premium External\n\n\x1b[0m");

    printf("\x1b[38;5;171m   ┌─[ BUILD INFO ]──────────────────────────────────────────┐\n");
	printf("   │ \x1b[38;5;118mTIMESTAMP:\x1b[0m %s %s                      │\n", __DATE__, __TIME__);
    printf("   └─────────────────────────────────────────────────────────┘\n\n");


	static const char* BINARY_NAME = "RobloxPlayerBeta.exe";

	// Wait for Roblox to start
	printf("\x1b[38;5;45m   [\xbb] \x1b[0m\x1b[38;5;171mSearching for process: \x1b[38;5;45m%s\x1b[0m", BINARY_NAME);
	while (!memory->find_process_id(BINARY_NAME))
	{
		printf("\x1b[38;5;171m.\x1b[0m");
		std::this_thread::sleep_for(std::chrono::seconds(2));
	}
	printf("\n");

	if (!memory->attach_to_process(BINARY_NAME))
	{
		printf("\n\x1b[38;5;196m   [!] FATAL ERROR: Unable to attach to process memory!\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 1;
	}

	if (!memory->find_module_address(BINARY_NAME))
	{
		printf("\n\x1b[38;5;196m   [!] FATAL ERROR: Base module address not found!\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(3));
		return 1;
	}



	// Wait for Roblox window and DataModel to be ready
	printf("\x1b[38;5;45m   [\xbb] \x1b[0m\x1b[38;5;171mSyncing with engine state\x1b[0m");
	fflush(stdout);
	HWND robloxWnd = nullptr;

	while (true)
	{
		if (!robloxWnd)
		{
			robloxWnd = FindWindowA(nullptr, "Roblox");
			if (!robloxWnd)
			{
				printf("\x1b[38;5;240m.\x1b[0m"); // gray dot = waiting for window
				fflush(stdout);
				std::this_thread::sleep_for(std::chrono::seconds(2));
				continue;
			}
		}

		// Safe pointer chain: validate EACH step before using it as the next address
		std::uint64_t base = memory->get_module_address();
		if (!base)
		{
			printf("\x1b[38;5;240m!\x1b[0m"); // gray ! = module not ready
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		std::uint64_t fake_datamodel = memory->read<std::uint64_t>(base + Offsets::FakeDataModel::Pointer);
		if (!fake_datamodel || fake_datamodel > 0x7FFFFFFFFFFFFFFF)
		{
			printf("\x1b[38;5;45mf\x1b[0m"); // cyan f = waiting for fake_datamodel
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		std::uint64_t real_datamodel = memory->read<std::uint64_t>(fake_datamodel + Offsets::FakeDataModel::RealDataModel);
		if (!real_datamodel || real_datamodel == 0xCCCCCCCCCCCCCCCC || real_datamodel > 0x7FFFFFFFFFFFFFFF)
		{
			printf("\x1b[38;5;45md\x1b[0m"); // cyan d = waiting for datamodel
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		// DataModel is valid! Now resolve the rest of the pointer chain
		{
			std::unique_lock lock(game::game_state_mtx);
			game::datamodel = rbx::instance_t(real_datamodel);
		}

		std::uint64_t workspace_addr = memory->read<std::uint64_t>(real_datamodel + Offsets::DataModel::Workspace);
		if (!workspace_addr || workspace_addr > 0x7FFFFFFFFFFFFFFF)
		{
			printf("\x1b[38;5;220mw\x1b[0m"); // yellow w = waiting for workspace
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		{
			std::unique_lock lock(game::game_state_mtx);
			game::workspace = { workspace_addr };
		}

		std::uint64_t visengine_addr = memory->read<std::uint64_t>(base + Offsets::VisualEngine::Pointer);
		if (!visengine_addr || visengine_addr > 0x7FFFFFFFFFFFFFFF)
		{
			printf("\x1b[38;5;220mv\x1b[0m"); // yellow v = waiting for visengine
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}
		{
			std::unique_lock lock(game::game_state_mtx);
			game::visengine = { visengine_addr };
		}

		// Wait for GameLoaded before resolving children (otherwise children vector may have garbage entries)
		bool game_loaded = memory->read<bool>(real_datamodel + Offsets::DataModel::GameLoaded);
		if (!game_loaded)
		{
			printf("\x1b[38;5;240mL\x1b[0m"); // gray L = waiting for GameLoaded
			fflush(stdout);
			std::this_thread::sleep_for(std::chrono::seconds(1));
			continue;
		}

		// Now resolve Players and LocalPlayer (non-blocking - cache thread will retry)
		auto players_inst = game::datamodel.find_first_child_by_class("Players");
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

				// Resolve local character (may be 0 until player spawns - cache thread updates it)
				rbx::player_t local_player_obj{ local_player_addr };
				auto char_addr = local_player_obj.get_model_instance().address;
				if (memory->is_valid_instance_address(char_addr))
				{
					std::unique_lock lock(game::game_state_mtx);
					game::local_character = { char_addr };
				}
			}
			printf("\x1b[38;5;118m.\x1b[0m"); // bright dot = Players found
		}
		else
		{
			printf("\x1b[38;5;240mp\x1b[0m"); // gray p = Players not yet available (cache thread will retry)
		}
		fflush(stdout);

		// All pointer chain resolved successfully!
		printf("\x1b[38;5;118m [ SUCCESS ]\x1b[0m\n");
		fflush(stdout);
		break;
	}
	printf("\n");

	// === DIAGNÓSTICO: dump de punteros globales ===
	printf("\x1b[38;5;214m   [DEBUG] === POINTER DUMP ===\x1b[0m\n");
	printf("\x1b[38;5;214m   [DEBUG] datamodel:     0x%llx\x1b[0m\n", (unsigned long long)game::datamodel.address);
	printf("\x1b[38;5;214m   [DEBUG] workspace:     0x%llx\x1b[0m\n", (unsigned long long)game::workspace.address);
	printf("\x1b[38;5;214m   [DEBUG] visengine:     0x%llx\x1b[0m\n", (unsigned long long)game::visengine.address);
	printf("\x1b[38;5;214m   [DEBUG] players:       0x%llx\x1b[0m\n", (unsigned long long)game::players.address);
	printf("\x1b[38;5;214m   [DEBUG] local_player:  0x%llx\x1b[0m\n", (unsigned long long)game::local_player.address);
	printf("\x1b[38;5;214m   [DEBUG] local_char:    0x%llx\x1b[0m\n", (unsigned long long)game::local_character.address);
	printf("\x1b[38;5;214m   [DEBUG] =======================\x1b[0m\n");

	// Now all game pointers are valid - start subsystems that depend on them
	std::thread(cache::run).detach();
	std::thread(jailbreak::run).detach();

	if (!InitializeStorage())
	{
		printf("\n\x1b[38;5;214m   [!] WARNING: Initial memory scan failed. Retrying in background...\x1b[0m\n");
	}

	std::thread(AutoRescanHandler).detach();

	rbx::silent::initialize();

	if (!render->create_window())
	{
		printf("\n\x1b[38;5;196m   [!] Overlay creation failed!\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}
	printf("\x1b[38;5;45m   [+] Overlay linked\x1b[0m\n");

	if (!render->create_device())
	{
		printf("\x1b[38;5;196m   [!] DirectX device creation failed!\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}
	printf("\x1b[38;5;45m   [+] GPU Pipeline ready\x1b[0m\n");

	if (!render->create_imgui())
	{
		printf("\x1b[38;5;196m   [!] ImGui core injection failed!\x1b[0m\n");
		std::this_thread::sleep_for(std::chrono::seconds(5));
		return 1;
	}
	printf("\x1b[38;5;45m   [+] UI Core initialized\x1b[0m\n");

	render->initialized = true;
	game::running = true;

	printf("\n\x1b[38;5;45m   BOOTING CORE SYSTEMS\n   \x1b[0m");
	printf("\x1b[38;5;118m[\x1b[0m");
	for (int i = 0; i < 40; i++) {
		printf("\x1b[38;5;45m\u2588\x1b[0m");
		fflush(stdout);
		std::this_thread::sleep_for(std::chrono::milliseconds(25));
	}
	printf("\x1b[38;5;118m] 100%%\x1b[0m\n\n");
	printf("   \x1b[38;5;118m[+] VERTEX INITIALIZED - READY FOR OPERATION\x1b[0m\n\n");
    printf("\x1b[38;5;171m   >─────────────────────────────────────────────────────────<\n\n\x1b[0m");

	notifications::add("vertex Loaded Successfully!", notifications::NotificationType::Success, 8.0f);


	std::thread([]() {
		HANDLE proc_handle = memory->get_process_handle();
		for (;;)
		{
			if (proc_handle == NULL || proc_handle == INVALID_HANDLE_VALUE)
				break;
			DWORD exit_code;
			if (!GetExitCodeProcess(proc_handle, &exit_code) || exit_code != STILL_ACTIVE)
			{
                printf("\n\x1b[38;5;196m   [!] ROBLOX CLOSED - SYSTEM STANDBY\x1b[0m\n");
                printf("\x1b[38;5;45m   [i] You can now safely close this window.\x1b[0m\n");
				game::running = false;
                break;
			}
			Sleep(1000);
		}
		}).detach();

	std::thread(check::run).detach();
	std::thread(walkspeed::run).detach();
	std::thread(freezeplayer::run).detach();
	std::thread(fly::run).detach();
	std::thread(rbx::aimbot::run).detach();
	std::thread(rbx::hitbox_expander::run).detach();
	std::thread(triggerbot::run).detach();
	std::thread(bunnyhop::run).detach();
	std::thread(fov_changer::run).detach();
	if (settings::expl::anti_afk) std::thread(anti_afk::run).detach();
	std::thread(misc::run).detach();

	// Gravity / Tickrate controller (uses World::Gravity from new offsets)
	std::thread([]()
		{
			for (;;)
			{
				Sleep(200);

				if (!settings::expl::tickrate)
					continue;

				std::uint64_t ws_addr = 0;
				{
					std::shared_lock slock(game::game_state_mtx);
					ws_addr = game::workspace.address;
				}

				if (!memory->is_valid_instance_address(ws_addr))
					continue;

				auto world = memory->read<uintptr_t>(ws_addr + Offsets::Workspace::World);
				if (memory->is_valid_instance_address(world))
					memory->write<float>(world + Offsets::World::Gravity, settings::expl::tickrate_amount * 4);
			}
		}
	).detach();

    game::wnd = FindWindowA(nullptr, "Roblox");

    // Frame rate limiter: cap at ~166 FPS (6ms per frame) to reduce CPU usage
    auto _frame_start = std::chrono::steady_clock::now();
    constexpr auto _frame_duration = std::chrono::milliseconds(6);

	while (game::running)
	{
		render->start_render();

		render->render_visuals();
		render->render_menu();

		render->render_notifications();

		render->end_render();

        // Frame limiter - prevents 100%% CPU usage
        auto _frame_end = std::chrono::steady_clock::now();
        auto _elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(_frame_end - _frame_start);
        if (_elapsed < _frame_duration) {
            std::this_thread::sleep_for(_frame_duration - _elapsed);
        }
        _frame_start = std::chrono::steady_clock::now();
	}

	printf("\x1b[38;5;45m   [+] Render loop stopped cleanly.\x1b[0m\n");
	return 0;
}
