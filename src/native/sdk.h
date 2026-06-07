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

	// Scan per-instance for the children vector offset
	std::uint64_t cs = 0;
	for (std::uint64_t off = 0x60; off < 0x300; off += 0x8)
	{
		std::uint64_t b = memory->read<std::uint64_t>(base->address + off);
		std::uint64_t e = memory->read<std::uint64_t>(base->address + off + 0x8);

		if (!memory->is_valid_instance_address(b) || !memory->is_valid_instance_address(e))
			continue;

		if (b > e) std::swap(b, e);

		std::uint64_t count = (e - b) / 16;
		if (count == 0 || count > 2048)
			continue;

		// Validar que el primer hijo tenga ClassDescriptor en rango del módulo
		std::uint64_t first = memory->read<std::uint64_t>(b);
		if (!memory->is_valid_instance_address(first))
			continue;

		std::uint64_t desc = memory->read<std::uint64_t>(first + Offsets::Instance::ClassDescriptor);
		// ClassDescriptor real apunta al módulo de Roblox (0x7ff0... - 0x7fff...)
		if (desc < 0x7ff000000000ULL || desc > 0x7fffffffffffULL)
			continue;

		// DEBUG: log every passing candidate
		std::printf("\x1b[38;5;240m[SCAN OK] off=0x%llx b=0x%llx e=0x%llx count=%llu first=0x%llx desc=0x%llx\x1b[0m\n",
			(unsigned long long)off,
			(unsigned long long)b,
			(unsigned long long)e,
			(unsigned long long)count,
			(unsigned long long)first,
			(unsigned long long)desc);

		cs = off;
		break;
	}

	if (cs == 0) return {};

	std::uint64_t v1 = memory->read<std::uint64_t>(base->address + cs);
	std::uint64_t v2 = memory->read<std::uint64_t>(base->address + cs + 0x8);
	std::uint64_t begin = (v1 < v2) ? v1 : v2;
	std::uint64_t end   = (v1 < v2) ? v2 : v1;

	std::uint64_t count = (end - begin) / 16;
	if (count == 0 || count > 2048) return {};

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