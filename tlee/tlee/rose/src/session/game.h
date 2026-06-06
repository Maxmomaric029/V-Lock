#pragma once
#include <native/sdk.h>
#include <string>
#include <shared_mutex>
#include <atomic>

namespace game
{
	// Shared mutex to protect game state that is written from the sync loop
	// and read from multiple background threads (cache, aimbot, walkspeed, etc.)
	inline std::shared_mutex game_state_mtx;

	inline rbx::instance_t datamodel{};
	inline rbx::visualengine_t visengine{};
	inline rbx::instance_t workspace{};
	inline rbx::instance_t players{};
	inline rbx::instance_t local_player{};
	inline rbx::instance_t local_character{};

	inline HWND wnd;
	inline std::atomic<std::int64_t> place_id{ 0 };
	inline std::string game_name{ "Unknown" };
	inline std::atomic<bool> running{ false };
}