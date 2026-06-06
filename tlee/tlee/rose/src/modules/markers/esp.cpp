#define NOMINMAX
#include <vector>
#include <string>
#include <unordered_map>
#include <mutex>
#include <Windows.h>
#include <cmath>
#include <cstring>
#include <algorithm>

#include "../resources/GetWeaponIcon.h"
#include "esp.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include <settings.h>
#include <session/game.h>
#include <buffer/cache.h>
#include <modules/assist/silent.h>
#include <modules/targeting/aimbot.h>

static float get_effective_fov()
{
	if (!settings::silent::gun_based_fov)
		return settings::silent::fov;

	std::string tool_name;
	{
		std::lock_guard<std::recursive_mutex> lock(cache::mtx);
		tool_name = cache::cached_local_player.tool_name;
	}

	std::string tool_name_lower = tool_name;
	std::transform(tool_name_lower.begin(), tool_name_lower.end(), tool_name_lower.begin(), ::tolower);

	if (tool_name_lower.find("double-barrel") != std::string::npos || 
		tool_name_lower.find("double barrel") != std::string::npos ||
		tool_name_lower.find("doublebarrel") != std::string::npos)
		return settings::silent::fov_double_barrel;
	else if (tool_name_lower.find("tacticalshotgun") != std::string::npos ||
		tool_name_lower.find("tactical shotgun") != std::string::npos)
		return settings::silent::fov_tactical_shotgun;
	else if (tool_name_lower.find("revolver") != std::string::npos)
		return settings::silent::fov_revolver;

	return settings::silent::fov;
}

#include <clipper2/clipper.h>

#define M_PI 3.14159265358979323846

namespace helper
{
	__forceinline void corner_box(ImDrawList* draw, ImVec2 min, ImVec2 max, ImU32 col, float thickness = 1.f)
	{
		float x = min.x;
		float y = min.y;
		float w = max.x - min.x;
		float h = max.y - min.y;

		float line_w = w * 0.25f;
		float line_h = h * 0.25f;
		
		// Adjust for thickness to keep it inside the bounds
		float t = thickness;
		ImU32 outline = IM_COL32(0, 0, 0, 200);

		auto draw_corner = [&](float x1, float y1, float x2, float y2, float dx, float dy) {
			// Outline
			draw->AddLine(ImVec2(x1, y1), ImVec2(x1 + dx, y1), outline, t + 2.0f);
			draw->AddLine(ImVec2(x1, y1), ImVec2(x1, y1 + dy), outline, t + 2.0f);
			
			// Main Line
			draw->AddLine(ImVec2(x1, y1), ImVec2(x1 + dx, y1), col, t);
			draw->AddLine(ImVec2(x1, y1), ImVec2(x1, y1 + dy), col, t);
		};

		// Top Left
		draw_corner(x, y, x + line_w, y + line_h, line_w, line_h);
		// Top Right
		draw_corner(x + w, y, x + w - line_w, y + line_h, -line_w, line_h);
		// Bottom Left
		draw_corner(x, y + h, x + line_w, y + h - line_h, line_w, -line_h);
		// Bottom Right
		draw_corner(x + w, y + h, x + w - line_w, y + h - line_h, -line_w, -line_h);
	}

	__forceinline void box(ImVec2& c1, ImVec2& c2, ImU32 color)
	{
		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		
		c1.x = std::round(c1.x);
		c1.y = std::round(c1.y);
		c2.x = std::round(c2.x);
		c2.y = std::round(c2.y);

		ImVec2 min = c1;
		ImVec2 max = ImVec2(c1.x + c2.x, c1.y + c2.y);
		ImU32 outline_col = IM_COL32(0, 0, 0, 255);

		if (settings::visuals::box_fill)
		{
			ImU32 fill_col = ImGui::ColorConvertFloat4ToU32({
				settings::visuals::box_fill_color[0],
				settings::visuals::box_fill_color[1],
				settings::visuals::box_fill_color[2],
				settings::visuals::box_fill_color[3]
			});
			draw->AddRectFilled(min, max, fill_col, settings::visuals::box_type == 2 ? 4.f : 0.f);
		}

		if (settings::visuals::box_type == 1) // corner box
		{
			corner_box(draw, min, max, color, 1.f);
		}
		else if (settings::visuals::box_type == 2) // rounded box
		{
			if (settings::visuals::box_outline)
				draw->AddRect(ImVec2(min.x - 1.f, min.y - 1.f), ImVec2(max.x + 1.f, max.y + 1.f), outline_col, 4.f, 0, 2.f);
			draw->AddRect(min, max, color, 4.f, 0, 1.f);
		}
		else // normal box
		{
			if (settings::visuals::box_outline)
				draw->AddRect(ImVec2(min.x - 1.f, min.y - 1.f), ImVec2(max.x + 1.f, max.y + 1.f), outline_col, 0.f, 0, 2.f);
			draw->AddRect(min, max, color, 0.f, 0, 1.f);
		}
	}

	__forceinline void skeleton(cache::entity_t& entity, math::vector2& dims, math::matrix4& view, ImU32 col)
	{
		ImDrawList* draw = ImGui::GetBackgroundDrawList();
		
		auto draw_bone = [&](const std::string& b1, const std::string& b2) {
			auto it1 = entity.parts.find(b1);
			auto it2 = entity.parts.find(b2);
			if (it1 == entity.parts.end() || it2 == entity.parts.end()) return;

			math::vector3 p1 = it1->second.get_primitive().get_position();
			math::vector3 p2 = it2->second.get_primitive().get_position();
			math::vector2 s1, s2;

			if (game::visengine.world_to_screen(p1, s1, dims, view) && game::visengine.world_to_screen(p2, s2, dims, view))
			{
				draw->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), IM_COL32(0, 0, 0, 255), 2.5f);
				draw->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), col, 1.0f);
				
				if (settings::visuals::skeleton_joints)
					draw->AddCircleFilled(ImVec2(s1.x, s1.y), 2.0f, col);
			}
		};

		if (entity.rig_type == 1) // R15
		{
			draw_bone("Head", "UpperTorso");
			draw_bone("UpperTorso", "LowerTorso");
			
			draw_bone("UpperTorso", "LeftUpperArm");
			draw_bone("LeftUpperArm", "LeftLowerArm");
			draw_bone("LeftLowerArm", "LeftHand");
			
			draw_bone("UpperTorso", "RightUpperArm");
			draw_bone("RightUpperArm", "RightLowerArm");
			draw_bone("RightLowerArm", "RightHand");
			
			draw_bone("LowerTorso", "LeftUpperLeg");
			draw_bone("LeftUpperLeg", "LeftLowerLeg");
			draw_bone("LeftLowerLeg", "LeftFoot");
			
			draw_bone("LowerTorso", "RightUpperLeg");
			draw_bone("RightUpperLeg", "RightLowerLeg");
			draw_bone("RightLowerLeg", "RightFoot");
		}
		else // R6
		{
			draw_bone("Head", "Torso");
			draw_bone("Torso", "Left Arm");
			draw_bone("Torso", "Right Arm");
			draw_bone("Torso", "Left Leg");
			draw_bone("Torso", "Right Leg");
		}
	}
}

void DrawPolygonalFOV(ImDrawList* draw, ImVec2 center, float radius, ImU32 color, bool filled = false, float rotation = 0.0f) 
{
	constexpr int segments = 12;
	const float angle_step = 2.0f * (float)M_PI / (float)segments;

	ImVec2 vertices[segments + 1];
	for (int i = 0; i < segments; i++) 
	{
		float angle = i * angle_step + rotation;
		vertices[i] = ImVec2(
			center.x + radius * cosf(angle),
			center.y + radius * sinf(angle)
		);
	}
	vertices[segments] = vertices[0];

	if (filled)
	{
		draw->AddConvexPolyFilled(vertices, segments, color);
	}

	ImU32 outline_color = (color & 0x00FFFFFF) | 0xFF000000;
	for (int i = 0; i < segments; i++) 
	{
		draw->AddLine(vertices[i], vertices[i + 1], IM_COL32(0, 0, 0, 255), 4.0f);
		draw->AddLine(vertices[i], vertices[i + 1], outline_color, 2.0f);
	}
}

void esp::run()
{
	static math::vector3 corners[8] =
	{
		{-1, -1, -1}, {1, -1, -1}, {-1, 1, -1},{1, 1, -1},
		{-1, -1, 1}, {1, -1, 1}, {-1, 1, 1}, {1, 1, 1}
	};

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	draw->Flags |= ImDrawListFlags_AntiAliasedLines;

	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	ScreenToClient(FindWindowA(nullptr, "Roblox"), &cursor_pos);

	math::vector2 dims = game::visengine.get_dimensions();
	math::matrix4 view = game::visengine.get_viewmatrix();

	if (settings::silent::draw_fov)
	{
		ImVec2 center((float)cursor_pos.x, (float)cursor_pos.y);
		
		float rotation = 0.0f;
		if (settings::silent::rotate_fov)
		{
			static float rotation_time = 0.0f;
			rotation_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rotation_time > 2.0f * (float)M_PI)
				rotation_time -= 2.0f * (float)M_PI;
			rotation = rotation_time;
		}
		
		ImVec4 color_vec;
		if (settings::silent::rainbow_fov)
		{
			static float rainbow_time = 0.0f;
			rainbow_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rainbow_time > 2.0f * (float)M_PI)
				rainbow_time -= 2.0f * (float)M_PI;
			
			float h = rainbow_time / (2.0f * (float)M_PI);
			float s = 1.0f;
			float v = 1.0f;
			
			int i = (int)(h * 6.0f);
			float f = (h * 6.0f) - i;
			float p = v * (1.0f - s);
			float q = v * (1.0f - s * f);
			float t = v * (1.0f - s * (1.0f - f));
			
			i %= 6;
			switch (i)
			{
			case 0: color_vec = ImVec4(v, t, p, settings::silent::fov_color[3]); break;
			case 1: color_vec = ImVec4(q, v, p, settings::silent::fov_color[3]); break;
			case 2: color_vec = ImVec4(p, v, t, settings::silent::fov_color[3]); break;
			case 3: color_vec = ImVec4(p, q, v, settings::silent::fov_color[3]); break;
			case 4: color_vec = ImVec4(t, p, v, settings::silent::fov_color[3]); break;
			case 5: color_vec = ImVec4(v, p, q, settings::silent::fov_color[3]); break;
			}
		}
		else
		{
			color_vec = ImVec4(
				settings::silent::fov_color[0],
				settings::silent::fov_color[1],
				settings::silent::fov_color[2],
				settings::silent::fov_color[3]
			);
		}
		
		ImU32 fov_color = ImGui::ColorConvertFloat4ToU32(color_vec);
		DrawPolygonalFOV(draw, center, get_effective_fov() - 1.0f, fov_color, settings::silent::filled_fov, rotation);
	}

	if (settings::aimbot::draw_fov)
	{
		ImVec2 center((float)cursor_pos.x, (float)cursor_pos.y);
		
		float rotation = 0.0f;
		if (settings::aimbot::rotate_fov)
		{
			static float rotation_time = 0.0f;
			rotation_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rotation_time > 2.0f * (float)M_PI)
				rotation_time -= 2.0f * (float)M_PI;
			rotation = rotation_time;
		}
		
		ImVec4 color_vec;
		if (settings::aimbot::rainbow_fov)
		{
			static float rainbow_time = 0.0f;
			rainbow_time += ImGui::GetIO().DeltaTime * 2.0f;
			if (rainbow_time > 2.0f * (float)M_PI)
				rainbow_time -= 2.0f * (float)M_PI;
			
			float h = rainbow_time / (2.0f * (float)M_PI);
			float s = 1.0f;
			float v = 1.0f;
			
			int i = (int)(h * 6.0f);
			float f = (h * 6.0f) - i;
			float p = v * (1.0f - s);
			float q = v * (1.0f - s * f);
			float t = v * (1.0f - s * (1.0f - f));
			
			i %= 6;
			switch (i)
			{
			case 0: color_vec = ImVec4(v, t, p, settings::aimbot::fov_color[3]); break;
			case 1: color_vec = ImVec4(q, v, p, settings::aimbot::fov_color[3]); break;
			case 2: color_vec = ImVec4(p, v, t, settings::aimbot::fov_color[3]); break;
			case 3: color_vec = ImVec4(p, q, v, settings::aimbot::fov_color[3]); break;
			case 4: color_vec = ImVec4(t, p, v, settings::aimbot::fov_color[3]); break;
			case 5: color_vec = ImVec4(v, p, q, settings::aimbot::fov_color[3]); break;
			}
		}
		else
		{
			color_vec = ImVec4(
				settings::aimbot::fov_color[0],
				settings::aimbot::fov_color[1],
				settings::aimbot::fov_color[2],
				settings::aimbot::fov_color[3]
			);
		}
		
		ImU32 fov_color = ImGui::ColorConvertFloat4ToU32(color_vec);
		DrawPolygonalFOV(draw, center, settings::aimbot::fov - 1.0f, fov_color, settings::aimbot::filled_fov, rotation);
	}

	std::vector<cache::entity_t> snapshot;
	{
		std::lock_guard<std::recursive_mutex> lock(cache::mtx);
		snapshot = cache::cached_players;
	}

	for (cache::entity_t& entity : snapshot)
	{
		bool valid = false;
		float left = FLT_MAX, top = FLT_MAX;
		float right = -FLT_MAX, bottom = -FLT_MAX;

		if (entity.instance.address == 0)
		{
			continue;
		}

		if (!settings::visuals::localplayer && entity.instance.address == game::local_player.address)
		{
			continue;
		}

		if (settings::visuals::target && g_silent_aim_locked)
		{
			if (entity.instance.address != g_silent_cached_target.instance.address)
			{
				continue;
			}
		}

		for (auto& parts : entity.parts)
		{
			rbx::primitive_t prim = parts.second.get_primitive();
			auto size = prim.get_size();
			
			if (settings::hitbox_expander::enabled && parts.first == "HumanoidRootPart")
			{
				size = math::vector3{ 2.0f, 2.0f, 1.0f };
			}
			
			auto pos = prim.get_position();
			auto rot = prim.get_rotation();

			if (size.x == 0 || size.y == 0 || size.z == 0)
			{
				continue;
			}

			for (auto& corner : corners)
			{
				math::vector3 world = pos + rot * math::vector3
				{
					corner.x * size.x * 0.5f,
					corner.y * size.y * 0.5f,
					corner.z * size.z * 0.5f
				};

				math::vector2 out{};
				if (game::visengine.world_to_screen(world, out, dims, view))
				{
					valid = true;
					left = std::min(left, out.x);
					top = std::min(top, out.y);
					right = std::max(right, out.x);
					bottom = std::max(bottom, out.y);
				}
			}
		}

		if (!valid || left >= right || top >= bottom)
		{
			continue;
		}

		ImVec2 c1(left, top);
		ImVec2 c2(right - left, bottom - top);

		ImU32 box_col = ImGui::ColorConvertFloat4ToU32
		(
			{
				settings::visuals::box_color[0],
				settings::visuals::box_color[1],
				settings::visuals::box_color[2],
				settings::visuals::box_color[3]
			}
		);

		bool is_visible = true;
		if (settings::visuals::visible_check)
		{
			rbx::instance_t cam_inst = game::workspace.find_first_child_by_class("Camera");
			if (cam_inst.address != 0)
			{
				rbx::camera_t cam{ cam_inst.address };
				math::vector3 cam_pos = cam.get_position();
				
				auto hrp_it = entity.parts.find("Head");
				if (hrp_it == entity.parts.end()) hrp_it = entity.parts.find("HumanoidRootPart");

				if (hrp_it != entity.parts.end())
				{
					math::vector3 tgt_pos = hrp_it->second.get_primitive().get_position();
					is_visible = !cache::has_wall_between(cam_pos, tgt_pos);
				}
			}

			ImVec4 v_col = is_visible ? 
				ImVec4(settings::visuals::visible_color[0], settings::visuals::visible_color[1], settings::visuals::visible_color[2], settings::visuals::visible_color[3]) :
				ImVec4(settings::visuals::invisible_color[0], settings::visuals::invisible_color[1], settings::visuals::invisible_color[2], settings::visuals::invisible_color[3]);
			
			box_col = ImGui::ColorConvertFloat4ToU32(v_col);
		}

		if (settings::visuals::box)
		{
			bool is_target = (g_silent_aim_locked && entity.instance.address == g_silent_cached_target.instance.address);
			if (is_target)
			{
				static float target_pulse = 0.0f;
				target_pulse += ImGui::GetIO().DeltaTime * 5.0f;
				float pulse_val = (std::sin(target_pulse) * 0.5f) + 0.5f;

				// Animated target highlight
				ImU32 pulse_col = IM_COL32(255, 50, 50, (int)(100 + pulse_val * 155));
				draw->AddRect(ImVec2(c1.x - 3, c1.y - 3), ImVec2(c1.x + c2.x + 3, c1.y + c2.y + 3), pulse_col, 0, 0, 1.5f);
				
				// "TARGET" header
				if (Visualize.visitor) {
					const char* t_label = "LOCKED";
					float t_size = 9.0f;
					ImVec2 l_size = Visualize.visitor->CalcTextSizeA(t_size, FLT_MAX, 0.0f, t_label);
					float l_x = c1.x + (c2.x * 0.5f) - (l_size.x * 0.5f);
					float l_y = c1.y - l_size.y - 15.0f;
					
					draw->AddRectFilled(ImVec2(l_x - 4, l_y - 2), ImVec2(l_x + l_size.x + 4, l_y + l_size.y + 2), IM_COL32(255, 0, 0, 180), 3.0f);
					draw->AddText(Visualize.visitor, t_size, ImVec2(l_x, l_y), IM_COL32_WHITE, t_label);
				}
				
				if (!settings::visuals::visible_check)
					box_col = IM_COL32(255, 255, 255, 255); // White box for target if no vis check
			}

			helper::box(c1, c2, box_col);
		}

		if (settings::visuals::highlights)
		{
			ImDrawList* draw = ImGui::GetBackgroundDrawList();

			auto project_part = [&](rbx::part_t& part, const std::string& part_name) -> std::vector<ImVec2> {
				std::vector<ImVec2> projected;
				if (!part.address) return projected;

				rbx::primitive_t prim = part.get_primitive();
				math::vector3 size = prim.get_size();
				
				if (settings::hitbox_expander::enabled && part_name == "HumanoidRootPart")
				{
					size = math::vector3{ 2.0f, 2.0f, 1.0f };
				}
				
				math::vector3 pos = prim.get_position();
				math::matrix3 rot = prim.get_rotation();

				for (const auto& lc : corners) {
					math::vector3 world = pos + rot * math::vector3{ lc.x * size.x * 0.5f, lc.y * size.y * 0.5f, lc.z * size.z * 0.5f };
					math::vector2 screen{};
					if (game::visengine.world_to_screen(world, screen, dims, view)) {
						if (screen.x >= 0.f && screen.y >= 0.f)
							projected.push_back(ImVec2(screen.x, screen.y));
					}
				}

				if (projected.size() < 3) return {};

				std::sort(projected.begin(), projected.end(), [](const ImVec2& a, const ImVec2& b) {
					return a.x < b.x || (a.x == b.x && a.y < b.y);
					});

				std::vector<ImVec2> hull;
				auto cross = [](const ImVec2& O, const ImVec2& A, const ImVec2& B) {
					return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
					};

				for (auto& p : projected) {
					while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
						hull.pop_back();
					hull.push_back(p);
				}

				size_t t = hull.size() + 1;
				for (int i = (int)projected.size() - 1; i >= 0; --i) {
					auto& p = projected[i];
					while (hull.size() >= t && cross(hull[hull.size() - 2], hull[hull.size() - 1], p) <= 0)
						hull.pop_back();
					hull.push_back(p);
				}

				hull.pop_back();
				return hull;
				};

			Clipper2Lib::Paths64 all_parts;
			for (auto& part_pair : entity.parts) {
				if (part_pair.first == "HumanoidRootPart") continue;

				rbx::part_t part = part_pair.second;
				if (!part.address) continue;
				auto hull = project_part(part, part_pair.first);
				if (hull.size() < 3) continue;

				Clipper2Lib::Path64 path;
				for (auto& pt : hull)
					path.push_back({ static_cast<int64_t>(pt.x * 1000.0), static_cast<int64_t>(pt.y * 1000.0) });
				all_parts.push_back(path);
			}

			if (all_parts.empty()) continue;

			auto unified_solution = Clipper2Lib::Union(all_parts, Clipper2Lib::FillRule::NonZero);

			std::vector<std::vector<ImVec2>> all_polys;
			for (auto& sp : unified_solution) {
				std::vector<ImVec2> poly;
				for (auto& pt : sp) poly.push_back(ImVec2(pt.x / 1000.0f, pt.y / 1000.0f));
				if (poly.size() >= 3) all_polys.push_back(poly);
			}

			ImU32 fill_color = ImGui::ColorConvertFloat4ToU32({
				settings::visuals::highlights_color[0],
				settings::visuals::highlights_color[1],
				settings::visuals::highlights_color[2],
				settings::visuals::highlights_color[3]
			});

			for (auto& poly : all_polys) {
				draw->AddConvexPolyFilled(poly.data(), static_cast<int>(poly.size()), fill_color);
			}

			for (auto& poly : all_polys) {
				draw->AddPolyline(poly.data(), static_cast<int>(poly.size()), IM_COL32_WHITE, true, 1.0f);
			}
		}

		ImVec2 boxPos = c1;
		ImVec2 boxBR = ImVec2(c1.x + c2.x, c1.y + c2.y);

		if (settings::visuals::name && Visualize.visitor)
		{
			std::string player_name = entity.name;
			
			ImFont* font = Visualize.visitor;
			float font_size = 9.0f;
			float char_spacing = 1.0f;
			
			float total_width = 0.0f;
			float max_height = 0.0f;
			for (size_t i = 0; i < player_name.length(); i++) {
				char c = player_name[i];
				char char_str[2] = { c, '\0' };
				ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
				total_width += char_size.x + (i < player_name.length() - 1 ? char_spacing : 0.0f);
				max_height = std::max(max_height, char_size.y);
			}
			
			float centerX = (boxPos.x + boxBR.x) * 0.5f;
			float textX = centerX - (total_width * 0.5f);
			float textY = boxPos.y - max_height - 2.0f;
			
			ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
			
			ImU32 nameColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
				settings::visuals::name_color[0],
				settings::visuals::name_color[1],
				settings::visuals::name_color[2],
				settings::visuals::name_color[3]
			));
			
			// Draw a nice semi-transparent background for the name
			ImVec2 bg_min = ImVec2(textPos.x - 2, textPos.y - 1);
			ImVec2 bg_max = ImVec2(textPos.x + total_width + 2, textPos.y + max_height + 1);
			draw->AddRectFilled(bg_min, bg_max, IM_COL32(0, 0, 0, 120), 2.0f);
			
			if (entity.is_user)
			{
				std::string tag = "[VERTEX] ";
				ImVec2 tag_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, tag.c_str());
				ImU32 accent_col = ImGui::ColorConvertFloat4ToU32(menu::accent_color);
				
				// Draw tag
				Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, accent_col, IM_COL32(0, 0, 0, 255), tag, char_spacing);
				
				// Offset name
				textPos.x += tag_size.x;
			}
			
			Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, nameColor, IM_COL32(0, 0, 0, 255), player_name, char_spacing);
		}

		if (settings::visuals::distance && Visualize.visitor)
		{
			auto hrp_it = entity.parts.find("HumanoidRootPart");
			if (hrp_it != entity.parts.end() && game::local_character.address != 0)
			{
				rbx::part_t local_hrp = { game::local_character.find_first_child("HumanoidRootPart").address };
				math::vector3 local_pos = local_hrp.get_primitive().get_position();
				math::vector3 enemy_pos = hrp_it->second.get_primitive().get_position();
				
				float dx = enemy_pos.x - local_pos.x;
				float dy = enemy_pos.y - local_pos.y;
				float dz = enemy_pos.z - local_pos.z;
				float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
				
				char distanceStr[32];
				snprintf(distanceStr, sizeof(distanceStr), "[%.1fM]", dist);
				
				ImFont* font = Visualize.visitor;
				float font_size = 9.0f;
				float char_spacing = 1.0f;
				
				float total_width = 0.0f;
				float max_height = 0.0f;
				for (size_t i = 0; i < strlen(distanceStr); i++) {
					char c = distanceStr[i];
					char char_str[2] = { c, '\0' };
					ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
					total_width += char_size.x + (i < strlen(distanceStr) - 1 ? char_spacing : 0.0f);
					max_height = std::max(max_height, char_size.y);
				}
				
				float centerX = (boxPos.x + boxBR.x) * 0.5f;
				float textX = centerX - (total_width * 0.5f);
				float textY = boxBR.y + 2.0f;
				
				ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
				
				ImU32 distColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
					settings::visuals::distance_color[0],
					settings::visuals::distance_color[1],
					settings::visuals::distance_color[2],
					settings::visuals::distance_color[3]
				));
				
				Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, distColor, IM_COL32(0, 0, 0, 255), std::string(distanceStr), char_spacing);
			}
		}

		if (settings::visuals::tool && Visualize.visitor)
		{
			std::string toolName = entity.tool_name;
			
			if (!toolName.empty())
			{
				
				ImFont* font = Visualize.visitor;
				float font_size = 9.0f;
				float char_spacing = 1.0f;
				
				float total_width = 0.0f;
				float max_height = 0.0f;
				for (size_t i = 0; i < toolName.length(); i++) {
					char c = toolName[i];
					char char_str[2] = { c, '\0' };
					ImVec2 char_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, char_str);
					total_width += char_size.x + (i < toolName.length() - 1 ? char_spacing : 0.0f);
					max_height = std::max(max_height, char_size.y);
				}
				
				float centerX = (boxPos.x + boxBR.x) * 0.5f;
				float textX = centerX - (total_width * 0.5f);
				float textY = boxBR.y + 2.0f;
				
				if (settings::visuals::distance)
					textY += 8.0f;
				
				ImVec2 textPos = ImVec2(std::floor(textX + 0.5f), std::floor(textY + 0.5f));
				
				ImU32 toolColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
					settings::visuals::tool_color[0],
					settings::visuals::tool_color[1],
					settings::visuals::tool_color[2],
					settings::visuals::tool_color[3]
				));
				
				Visualize.DrawTextWithSpacingAndOutline(draw, font, font_size, textPos, toolColor, IM_COL32(0, 0, 0, 255), toolName, char_spacing);
			}
		}

		if (settings::visuals::weapon_icon && Visualize.weapon_icon_font)
		{
			std::string toolName = entity.tool_name;
			
			if (!toolName.empty())
			{
				const char* weaponIcon = GetWeaponIcon(toolName);
				
				if (weaponIcon && strlen(weaponIcon) > 0)
				{
					float iconFontSize = 12.0f;
					ImVec2 iconTextSize = Visualize.weapon_icon_font->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, weaponIcon);
					
					float centerX = (boxPos.x + boxBR.x) * 0.5f;
					float iconX = centerX - (iconTextSize.x * 0.5f);
					float iconY = boxBR.y + 2.0f;
					
					if (settings::visuals::distance)
						iconY += 8.0f;
					if (settings::visuals::tool)
						iconY += 8.0f;
					
					ImVec2 iconPos = ImVec2(std::floor(iconX + 0.5f), std::floor(iconY + 0.5f));
					
					ImU32 iconColor = ImGui::ColorConvertFloat4ToU32(ImVec4(
						settings::visuals::weapon_icon_color[0],
						settings::visuals::weapon_icon_color[1],
						settings::visuals::weapon_icon_color[2],
						settings::visuals::weapon_icon_color[3]
					));
					
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x + 1, iconPos.y + 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x - 1, iconPos.y + 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x + 1, iconPos.y - 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, ImVec2(iconPos.x - 1, iconPos.y - 1), IM_COL32(0, 0, 0, 255), weaponIcon);
					
					draw->AddText(Visualize.weapon_icon_font, iconFontSize, iconPos, iconColor, weaponIcon);
				}
			}
		}

		if (settings::visuals::healthbar)
		{
			float health = 0.0f;
			float max_health = 100.0f;
			
			if (entity.humanoid.address != 0)
			{
				health = entity.humanoid.get_health();
				max_health = entity.humanoid.get_max_health();
			}

			if (max_health > 0.0f)
			{
				constexpr float healthBarWidth = 2.0f;
				float boxHeight = c2.y;

				ImVec2 barPos = ImVec2(boxPos.x - 6.0f, boxPos.y);
				ImVec2 barSize = ImVec2(healthBarWidth, boxHeight);

				Visualize.draw_health_bar(draw, max_health, health, barPos, barSize, 1.0f, settings::visuals::health_text);
			}
		}

		if (settings::visuals::skeleton)
		{
			ImU32 skel_col = ImGui::ColorConvertFloat4ToU32({
				settings::visuals::skeleton_color[0],
				settings::visuals::skeleton_color[1],
				settings::visuals::skeleton_color[2],
				settings::visuals::skeleton_color[3]
			});
			helper::skeleton(entity, dims, view, skel_col);
		}

		if (settings::visuals::snaplines)
		{
			ImVec2 screen_center = ImVec2(dims.x * 0.5f, dims.y * 0.5f);
			ImVec2 origin;
			if (settings::visuals::snaplines_origin == 0) // Bottom
				origin = ImVec2(dims.x * 0.5f, dims.y);
			else if (settings::visuals::snaplines_origin == 1) // Center
				origin = screen_center;
			else // Mouse
				origin = ImVec2(cursor_pos.x, cursor_pos.y);

			ImU32 line_col = ImGui::ColorConvertFloat4ToU32({
				settings::visuals::snaplines_color[0],
				settings::visuals::snaplines_color[1],
				settings::visuals::snaplines_color[2],
				settings::visuals::snaplines_color[3]
			});

			draw->AddLine(origin, ImVec2(boxPos.x + c2.x * 0.5f, boxPos.y + c2.y), line_col, 1.0f);
		}

	}

}

void esp_render_t::draw_health_bar(ImDrawList* draw, float max, float current, ImVec2 pos, ImVec2 size, float alpha_factor, bool show_text)
{
	if (max <= 0.0f) max = 100.0f;
	
	float clamped_current = std::clamp(current, 0.0f, max);
	float health_percent = (max > 0.0f) ? (clamped_current / max) : 0.0f;
	health_percent = std::clamp(health_percent, 0.0f, 1.0f);
	
	float bar_width = 1.0f;
	float bar_height = size.y;
	float bar_x = std::round(pos.x);
	float bar_y = std::round(pos.y);

	ImDrawListFlags old_flags = draw->Flags;
	draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

	float outline_x1 = std::round(bar_x - 1);
	float outline_y1 = std::round(bar_y - 1);
	float outline_x2 = std::round(bar_x + bar_width + 1);
	float outline_y2 = std::round(bar_y + bar_height + 1);
	
	draw->AddRectFilled(
		ImVec2(outline_x1, outline_y1),
		ImVec2(outline_x2, outline_y2),
		IM_COL32(0, 0, 0, 255)
	);

	float fill_height = bar_height * health_percent;
	
	ImU32 health_color = IM_COL32(
		static_cast<int>(settings::visuals::healthbar_color[0] * 255.f),  
		static_cast<int>(settings::visuals::healthbar_color[1] * 255.f),          
		static_cast<int>(settings::visuals::healthbar_color[2] * 255.f),
		static_cast<int>(255 * alpha_factor)
	);

	float bg_x1 = std::round(bar_x);
	float bg_y1 = std::round(bar_y);
	float bg_x2 = std::round(bar_x + bar_width);
	float bg_y2 = std::round(bar_y + bar_height);
	
	draw->AddRectFilled(
		ImVec2(bg_x1, bg_y1),
		ImVec2(bg_x2, bg_y2),
		IM_COL32(50, 50, 50, 255)
	);
	
	if (fill_height > 0.1f) {
		float fill_start_y = bar_y + bar_height - fill_height;
		float fill_y1 = std::round(fill_start_y);
		float fill_y2 = std::round(bar_y + bar_height);
		
		// Calculate color based on health percentage
		ImVec4 col_v = ImVec4(1.0f - health_percent, health_percent, 0.0f, alpha_factor);
		if (health_percent > 0.5f)
			col_v = ImVec4((1.0f - health_percent) * 2.0f, 1.0f, 0.0f, alpha_factor);
		else
			col_v = ImVec4(1.0f, (health_percent * 2.0f), 0.0f, alpha_factor);

		// Multi-color gradient for a "healthier" look
		ImU32 col_top = ImGui::ColorConvertFloat4ToU32(ImVec4(col_v.x * 1.2f, col_v.y * 1.2f, col_v.z * 1.2f, alpha_factor));
		ImU32 col_bottom = ImGui::ColorConvertFloat4ToU32(ImVec4(col_v.x * 0.7f, col_v.y * 0.7f, col_v.z * 0.7f, alpha_factor));

		draw->AddRectFilledMultiColor(
			ImVec2(bg_x1, fill_y1),
			ImVec2(bg_x2, fill_y2),
			col_top, col_top, col_bottom, col_bottom
		);
		
		// Subtle accent glow
		draw->AddRect(ImVec2(bg_x1, fill_y1), ImVec2(bg_x2, fill_y2), IM_COL32(255, 255, 255, 60), 0.f, 0, 1.0f);
	}
	
	draw->Flags = old_flags;

	if (show_text && settings::visuals::health_text && health_percent < 1.0f && visitor)
	{
		char buffer[32];
		sprintf_s(buffer, "%.0f", current);

		ImFont* font = visitor;
		float font_size = 9.0f;
		ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0f, buffer);

		float text_center_y = bar_y + (bar_height * 0.5f);
		
		ImVec2 text_pos = ImVec2(
			std::round(bar_x - (text_size.x / 2.0f) + (bar_width / 2.0f)),
			std::round(text_center_y - (text_size.y / 2.0f))
		);

		ImU32 text_color = IM_COL32(
			static_cast<int>(settings::visuals::health_text_color[0] * 255.f),
			static_cast<int>(settings::visuals::health_text_color[1] * 255.f),
			static_cast<int>(settings::visuals::health_text_color[2] * 255.f),
			255
		);

		draw->AddText(font, font_size, ImVec2(text_pos.x + 1, text_pos.y + 1), IM_COL32(0, 0, 0, 255), buffer);
		draw->AddText(font, font_size, ImVec2(text_pos.x - 1, text_pos.y + 1), IM_COL32(0, 0, 0, 255), buffer);
		draw->AddText(font, font_size, ImVec2(text_pos.x + 1, text_pos.y - 1), IM_COL32(0, 0, 0, 255), buffer);
		draw->AddText(font, font_size, ImVec2(text_pos.x - 1, text_pos.y - 1), IM_COL32(0, 0, 0, 255), buffer);
		
		draw->AddText(font, font_size, text_pos, text_color, buffer);
	}
}
