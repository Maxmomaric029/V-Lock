#include <native/sdk.h>
#include <native/offsets.h>
#include <runtime/memory.h>
#include <session/game.h>
#include <cstdio>

// Scan instance memory from 0x60 to 0x100 to find the first valid child pointer
std::uint64_t rbx::find_children_start(std::uint64_t instance_addr)
{
	for (std::uint64_t offset = 0x60; offset < 0x100; offset += 0x8)
	{
		std::uint64_t val = memory->read<std::uint64_t>(instance_addr + offset);
		if (memory->is_valid_instance_address(val))
		{
			// Confirm the pointed-to instance has a valid Name string pointer
			std::uint64_t name_ptr = memory->read<std::uint64_t>(val + Offsets::Instance::Name);
			if (name_ptr && memory->is_valid_instance_address(name_ptr))
			{
				printf("\x1b[38;5;118m   [DEBUG] Found ChildrenStart at offset 0x%llx (child=0x%llx)\x1b[0m\n",
					(unsigned long long)offset, (unsigned long long)val);
				return offset;
			}
		}
	}
	printf("\x1b[38;5;214m   [WARN] Could not find ChildrenStart dynamically, using fallback 0x%llx\x1b[0m\n",
		(unsigned long long)Offsets::Instance::ChildrenStart);
	return 0;
}

std::string rbx::nameable_t::get_name()
{
	std::uint64_t name = memory->read<std::uint64_t>(this->address + Offsets::Instance::Name);

	if (name)
	{
		return memory->read_string(name);
	}

	return "unknown";
}

std::string rbx::nameable_t::get_class_name()
{
	std::uint64_t class_descriptor = memory->read<std::uint64_t>(this->address + Offsets::Instance::ClassDescriptor);
	std::uint64_t class_name = memory->read<std::uint64_t>(class_descriptor + Offsets::Instance::ClassName);

	if (class_name)
	{
		return memory->read_string(class_name);
	}

	return "unknown";
}

std::vector<rbx::instance_t> rbx::interface_t::get_children()
{
	return get_children<rbx::instance_t>();
}

rbx::instance_t rbx::interface_t::find_first_child(std::string_view str)
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::instance_t rbx::interface_t::find_first_child_by_class(std::string_view str)
{
	std::vector<rbx::instance_t> children = this->get_children();

	for (rbx::instance_t& child : children)
	{
		if (child.get_class_name() == str)
		{
			return child;
		}
	}

	return {};
}

rbx::instance_t rbx::interface_t::get_parent()
{
	rbx::instance_t* base = static_cast<rbx::instance_t*>(this);
	return { memory->read<std::uint64_t>(base->address + Offsets::Instance::Parent) };
}

rbx::model_instance_t rbx::player_t::get_model_instance()
{
	return { memory->read<std::uint64_t>(this->address + Offsets::Player::ModelInstance) };
}

std::int64_t rbx::player_t::get_user_id()
{
	return memory->read<std::int64_t>(this->address + Offsets::Player::UserId);
}

std::uint8_t rbx::humanoid_t::get_rig_type()
{
	return { memory->read<std::uint8_t>(this->address + Offsets::Humanoid::RigType) };
}

float rbx::humanoid_t::get_health()
{
	return memory->read<float>(this->address + Offsets::Humanoid::Health);
}

float rbx::humanoid_t::get_max_health()
{
	return memory->read<float>(this->address + Offsets::Humanoid::MaxHealth);
}

float rbx::humanoid_t::get_hip_height()
{
	return memory->read<float>(this->address + Offsets::Humanoid::HipHeight);
}

bool rbx::humanoid_t::set_hip_height(float height)
{
	return memory->write<float>(this->address + Offsets::Humanoid::HipHeight, height);
}

bool rbx::humanoid_t::set_jump_height(float height)
{
	return memory->write<float>(this->address + Offsets::Humanoid::JumpHeight, height);
}

bool rbx::humanoid_t::set_jump_power(float power)
{
	return memory->write<float>(this->address + Offsets::Humanoid::JumpPower, power);
}

rbx::instance_t rbx::humanoid_t::get_seat_part()
{
	return { memory->read<std::uint64_t>(this->address + Offsets::Humanoid::SeatPart) };
}

rbx::primitive_t rbx::part_t::get_primitive()
{
	return { memory->read<std::uint64_t>(this->address + Offsets::BasePart::Primitive) };
}

math::vector3 rbx::primitive_t::get_size()
{
	return memory->read<math::vector3>(this->address + Offsets::Primitive::Size);
}

bool rbx::primitive_t::set_size(const math::vector3& size)
{
	return memory->write<math::vector3>(this->address + Offsets::Primitive::Size, size);
}

bool rbx::primitive_t::get_can_collide()
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;
	
	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::Primitive::Flags);
	return (flags & Offsets::PrimitiveFlags::CanCollide) != 0;
}

bool rbx::primitive_t::set_can_collide(bool enable)
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;
	
	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::Primitive::Flags);
	
	if (enable)
		flags |= Offsets::PrimitiveFlags::CanCollide;
	else
		flags &= ~Offsets::PrimitiveFlags::CanCollide;
	
	return memory->write<std::uint8_t>(primitive + Offsets::Primitive::Flags, flags);
}

bool rbx::primitive_t::get_anchored()
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;

	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::Primitive::Flags);
	return (flags & Offsets::PrimitiveFlags::Anchored) != 0;
}

bool rbx::primitive_t::set_anchored(bool enable)
{
	std::uint64_t primitive = this->address;
	if (!primitive) return false;

	std::uint8_t flags = memory->read<std::uint8_t>(primitive + Offsets::Primitive::Flags);

	if (enable)
		flags |= Offsets::PrimitiveFlags::Anchored;
	else
		flags &= ~Offsets::PrimitiveFlags::Anchored;

	return memory->write<std::uint8_t>(primitive + Offsets::Primitive::Flags, flags);
}

math::vector3 rbx::primitive_t::get_position()
{
	return memory->read<math::vector3>(this->address + Offsets::Primitive::Position);
}

bool rbx::primitive_t::set_position(const math::vector3& pos)
{
	return memory->write<math::vector3>(this->address + Offsets::Primitive::Position, pos);
}

math::matrix3 rbx::primitive_t::get_rotation()
{
	return memory->read<math::matrix3>(this->address + Offsets::Primitive::Rotation);
}

bool rbx::primitive_t::set_rotation(const math::matrix3& rot)
{
	return memory->write<math::matrix3>(this->address + Offsets::Primitive::Rotation, rot);
}

math::vector3 rbx::primitive_t::get_velocity()
{
	return memory->read<math::vector3>(this->address + Offsets::Primitive::AssemblyLinearVelocity);
}

bool rbx::primitive_t::set_velocity(const math::vector3& vel)
{
	return memory->write<math::vector3>(this->address + Offsets::Primitive::AssemblyLinearVelocity, vel);
}

math::vector3 rbx::primitive_t::get_angular_velocity()
{
	return memory->read<math::vector3>(this->address + Offsets::Primitive::AssemblyAngularVelocity);
}

bool rbx::primitive_t::set_angular_velocity(const math::vector3& vel)
{
	return memory->write<math::vector3>(this->address + Offsets::Primitive::AssemblyAngularVelocity, vel);
}

math::vector2 rbx::visualengine_t::get_dimensions()
{
	return memory->read<math::vector2>(this->address + Offsets::VisualEngine::Dimensions);
}

math::matrix4 rbx::visualengine_t::get_viewmatrix()
{
	return memory->read<math::matrix4>(this->address + Offsets::VisualEngine::ViewMatrix);
}

bool rbx::visualengine_t::world_to_screen(const math::vector3& world, math::vector2& out, const math::vector2& dims, const math::matrix4& view)
{
	math::vector4 clip = view.multiply({ world.x, world.y, world.z, 1.0f });

	if (clip.w < 0.1f)
	{
		return false;
	}

	clip.x /= clip.w;
	clip.y /= clip.w;

	out.x = (dims.x * 0.5f * clip.x) + (dims.x * 0.5f);
	out.y = -(dims.y * 0.5f * clip.y) + (dims.y * 0.5f);

	HWND roblox_window = game::wnd;
	if (roblox_window)
	{
		RECT client_rect{};
		POINT client_pos{};
		if (GetClientRect(roblox_window, &client_rect))
		{
			client_pos.x = client_rect.left;
			client_pos.y = client_rect.top;
			ClientToScreen(roblox_window, &client_pos);
			out.x += (float)client_pos.x;
			out.y += (float)client_pos.y;
		}
	}

	return true;
}

math::vector3 rbx::camera_t::get_position()
{
	return memory->read<math::vector3>(this->address + Offsets::Camera::Position);
}

math::matrix3 rbx::camera_t::get_rotation()
{
	return memory->read<math::matrix3>(this->address + Offsets::Camera::Rotation);
}

bool rbx::camera_t::write_rotation(const math::matrix3& rotation)
{
	return memory->write<math::matrix3>(this->address + Offsets::Camera::Rotation, rotation);
}