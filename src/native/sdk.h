#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <runtime/memory.h>
#include <native/offsets.h>
#include "math/math.h"

namespace rbx
{
	struct instance_t;
	struct primitive_t;
	struct model_instance_t;

	struct addressable_t
	{
		std::uint64_t address;

		addressable_t() : address(0) {}
		addressable_t(std::uint64_t address) : address(address) {}
	};

	struct nameable_t : public addressable_t
	{
		using addressable_t::addressable_t;

		std::string get_name();
		std::string get_class_name();
	};

	struct interface_t
	{
		template <typename T>
		std::vector<T> get_children();

		std::vector<rbx::instance_t> get_children();
		rbx::instance_t find_first_child(std::string_view str);
		rbx::instance_t find_first_child_by_class(std::string_view str);
		rbx::instance_t get_parent();
	};

	struct instance_t : public nameable_t, public interface_t
	{
		using nameable_t::nameable_t;
	};

	struct player_t final : public instance_t
	{
		using instance_t::instance_t;

		rbx::model_instance_t get_model_instance();
		std::int64_t get_user_id();
	};

	struct model_instance_t final : public instance_t
	{
		using instance_t::instance_t;
	};
	
	struct humanoid_t final : public addressable_t
	{
		using addressable_t::addressable_t;

		std::uint8_t get_rig_type();
		float get_health();
		float get_max_health();
		float get_hip_height();
		bool set_hip_height(float height);
		bool set_jump_height(float height);
		bool set_jump_power(float power);
		rbx::instance_t get_seat_part();
	};

	struct part_t : public instance_t
	{
		using instance_t::instance_t;

		rbx::primitive_t get_primitive();
	};

	struct primitive_t final : public addressable_t
	{
		using addressable_t::addressable_t;

		math::vector3 get_size();
		bool set_size(const math::vector3& size);
		math::vector3 get_position();
		bool set_position(const math::vector3& pos);
		math::matrix3 get_rotation();
		bool set_rotation(const math::matrix3& rot);
		math::vector3 get_velocity();
		bool set_velocity(const math::vector3& vel);
		math::vector3 get_angular_velocity();
		bool set_angular_velocity(const math::vector3& vel);
		bool get_can_collide();
		bool set_can_collide(bool enable);
		bool get_anchored();
		bool set_anchored(bool enable);
	};

	struct visualengine_t final : public addressable_t
	{
		math::vector2 get_dimensions();
		math::matrix4 get_viewmatrix();
		bool world_to_screen(const math::vector3& world, math::vector2& out, const math::vector2& dims, const math::matrix4& view);
	};

	struct camera_t final : public instance_t
	{
		using instance_t::instance_t;
		
		math::vector3 get_position();
		math::matrix3 get_rotation();
		bool write_rotation(const math::matrix3& rotation);
	};
}

template <typename T>
std::vector<T> rbx::interface_t::get_children()
{
	rbx::instance_t* base = static_cast<rbx::instance_t*>(this);
	std::uint64_t addr = base->address;
	if (!addr)
		return {};

	// Scan this specific instance for its children vector offset (0x60 to 0x200)
	// Different instance classes can have the vector at different offsets.
	std::uint64_t cs = 0;
	for (std::uint64_t off = 0x60; off < 0x200; off += 0x8)
	{
		std::uint64_t b = memory->read<std::uint64_t>(addr + off);
		std::uint64_t e = memory->read<std::uint64_t>(addr + off + 8);

		// Both must be valid heap addresses, and begin must be <= end
		if (memory->is_valid_instance_address(b) && memory->is_valid_instance_address(e) && b <= e)
		{
			std::uint64_t count = (e - b) / 16;
			if (count > 0 && count <= 2048)
			{
				// Confirm first child looks like a real instance (has a valid Name)
				std::uint64_t first = memory->read<std::uint64_t>(b);
				if (memory->is_valid_instance_address(first))
				{
					std::uint64_t name_ptr = memory->read<std::uint64_t>(first + Offsets::Instance::Name);
					if (name_ptr && memory->is_valid_instance_address(name_ptr))
					{
						cs = off;
						break;
					}
				}
			}
		}
	}

	if (cs == 0)
		return {};

	// Read the 3-field inline vector: [begin, end, capacity]
	// Order can vary between instance classes, so sort to find true begin/end
	std::uint64_t vals[3] = {
		memory->read<std::uint64_t>(addr + cs),
		memory->read<std::uint64_t>(addr + cs + 8),
		memory->read<std::uint64_t>(addr + cs + 0x10)
	};

	// Sort ascending so vals[0]=begin, vals[1]=end, vals[2]=capacity
	if (vals[0] > vals[1]) std::swap(vals[0], vals[1]);
	if (vals[0] > vals[2]) std::swap(vals[0], vals[2]);
	if (vals[1] > vals[2]) std::swap(vals[1], vals[2]);

	std::uint64_t begin = vals[0];
	std::uint64_t end   = vals[1];

	std::uint64_t count = (end - begin) / 16;
	if (count == 0 || count > 2048)
		return {};

	std::printf("\x1b[38;5;240m[DEBUG CHILDREN] begin=0x%llx end=0x%llx cap=0x%llx count=%llu\x1b[0m\n",
		(unsigned long long)begin,
		(unsigned long long)vals[1],
		(unsigned long long)vals[2],
		(unsigned long long)count);

	std::vector<T> children;
	children.reserve(static_cast<size_t>(count));

	for (std::uint64_t i = 0; i < count; ++i)
	{
		std::uint64_t child_addr = memory->read<std::uint64_t>(begin + i * 16);
		if (memory->is_valid_instance_address(child_addr))
			children.emplace_back(child_addr);
	}

	return children;
}