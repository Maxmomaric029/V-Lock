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

	// Read both pointers from the inline vector struct
	// Roblox vector layout may have [end, begin] instead of [begin, end]
	// So we read both values and use min() as begin, max() as end
	std::uint64_t val1 = memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart);
	std::uint64_t val2 = memory->read<std::uint64_t>(base->address + Offsets::Instance::ChildrenStart + Offsets::Instance::ChildrenEnd);

	// Validate: both values must be in heap range (0x001... to 0x7FE...)
	// Module addresses (0x7FFx...) would produce module-address entries, not instances.
	if (!memory->is_valid_instance_address(val1) || !memory->is_valid_instance_address(val2))
		return {};

	// Determine begin (smaller) and end (larger) regardless of field order
	std::uint64_t begin = (val1 < val2) ? val1 : val2;
	std::uint64_t end   = (val1 < val2) ? val2 : val1;

	// Sanity check: cap at a reasonable max (4096 for general instances)
	std::uint64_t count = (end - begin) / sizeof(std::uint64_t);
	if (count == 0 || count > 4096)
		return {};

	std::vector<T> children;
	children.reserve(static_cast<size_t>(count));

	for (std::uint64_t ptr = begin; ptr < end; ptr += sizeof(std::uint64_t))
	{
		std::uint64_t child_addr = memory->read<std::uint64_t>(ptr);
		// Filter: only heap addresses are valid instance addresses
		if (memory->is_valid_instance_address(child_addr))
			children.emplace_back(child_addr);
	}

	return children;
}