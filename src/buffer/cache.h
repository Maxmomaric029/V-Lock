#pragma once
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

#include <native/sdk.h>

namespace cache
{
	inline std::recursive_mutex mtx;

	struct entity_t final
	{
		rbx::instance_t instance;
		std::string name;
		std::string tool_name;
		std::int64_t user_id;
		std::uint64_t team_address;
		std::string team_name;

		std::uint8_t rig_type;
		
		rbx::humanoid_t humanoid;
		bool is_user;
		std::unordered_map<std::string, rbx::part_t> parts;
	};

	struct primitive_data_t {
		math::vector3 pos;
		math::vector3 half_size;
	};

	inline cache::entity_t cached_local_player;
	inline std::vector<cache::entity_t> cached_players;
	inline std::vector<primitive_data_t> cached_primitives;

	void run();
	bool has_wall_between(const math::vector3& camera_pos, const math::vector3& target_pos);
}