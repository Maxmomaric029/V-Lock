#pragma once
#include "../ext/imgui/imgui.h"
#include <Windows.h>
#include <string>

namespace menu
{
	inline ImVec4 accent_color = ImVec4(0.f / 255.0f, 150.f / 255.0f, 255.f / 255.0f, 1.0f);  // Blue accent
	inline bool watermark{ false };
	inline ImVec2 watermark_pos = ImVec2(-1, 10);
	inline bool streamproof{ false };
	inline bool hide_console{ false };
	inline bool disable_animations{ false };
	inline float rounding{ 12.0f };
	inline float opacity{ 0.95f };
	inline ImVec4 bg_color = ImVec4(0.05f, 0.05f, 0.06f, 1.0f);
	inline ImVec4 header_color = ImVec4(20.f / 255.0f, 20.f / 255.0f, 22.f / 255.0f, 1.0f);
	inline const char* version = "1.1.1";
	inline int menu_keybind{ VK_INSERT };
}

namespace settings
{
namespace aimbot
	{
  		inline bool enabled{ false };
  		inline int aimbot_type{ 0 }; // 0 = camera, 1 = mouse
  		inline bool sticky_aim{ false };
  		inline bool draw_fov{ false };
  		inline bool filled_fov{ false };
  		inline bool rotate_fov{ false };
  		inline bool rainbow_fov{ false };
  		inline float fov{ 200.0f };
  		inline float fov_color[4]{ 1.0f, 1.0f, 1.0f, 0.5f };
  		inline bool fov_check{ true };
	inline bool knocked_check{ false };
	inline bool team_check{ false };
	inline bool wall_check{ false };
	inline int keybind{ 0 };
  	inline int keybind_mode{ 0 };
  	inline int aim_part{ 0 }; // 0 = head, 1 = torso, 2 = closest point
  		
  	inline bool mouse_smooth{ false };
  	inline float mouse_smooth_x{ 1.0f };
  	inline float mouse_smooth_y{ 1.0f };
  	inline float mouse_sensitivity{ 1.0f };
  	inline bool mouse_prediction{ false };
  	inline float mouse_prediction_x{ 1.0f };
  	inline float mouse_prediction_y{ 1.0f };
  		
  	inline bool camera_smooth{ false };
  	inline float camera_smooth_x{ 1.0f };
  	inline float camera_smooth_y{ 1.0f };
  	inline bool camera_prediction{ false };
  	inline float camera_prediction_x{ 1.0f };
  	inline float camera_prediction_y{ 1.0f };
  		
  	inline bool shake{ false };
  	inline float shake_x{ 0.0f };
  	inline float shake_y{ 0.0f };

		inline bool silent_aim{ false }; // Enable silent aim mode
  	}
namespace silent
	{
		inline bool enabled{ false };
		inline int keybind{ 0 };
		inline int keybind_mode{ 0 };
		inline int aim_part{ 0 };
		inline float fov{ 100.0f };
		inline bool fov_check{ false };
		inline bool knocked_check{ false };
		inline bool team_check{ false };
		inline bool wall_check{ false };
		inline bool gun_based_fov{ false };
		inline float fov_double_barrel{ 100.0f };
		inline float fov_tactical_shotgun{ 100.0f };
		inline float fov_revolver{ 100.0f };
		inline bool sticky_aim{ false };
		inline bool prediction{ false };
		inline bool spoof_mouse{ false };
		inline bool draw_fov{ false };
		inline bool filled_fov{ false };
		inline bool rotate_fov{ false };
		inline bool rainbow_fov{ false };
		inline float fov_color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	}
namespace visuals
	{
  		inline bool box{ false };
  		inline int box_type{ 0 }; // 0 = normal, 1 = corner, 2 = rounded
  		inline float box_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  		inline bool box_outline{ true };
  		inline bool box_fill{ false };
  	inline float box_fill_color[4]{ 1.0f, 1.0f, 1.0f, 0.2f };

  	inline bool skeleton{ false };
  		inline float skeleton_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  	inline bool skeleton_joints{ false };

  	inline bool snaplines{ false };
  	inline int snaplines_origin{ 0 }; // 0 = bottom, 1 = center, 2 = mouse
  	inline float snaplines_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  
  	inline bool name{ false };
  	inline float name_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  
  	inline bool distance{ false };
  	inline float distance_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  
  	inline bool tool{ false };
  	inline float tool_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  
  	inline bool weapon_icon{ false };
  	inline float weapon_icon_color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
  
  	inline bool chams{ false }; // Chams/Glow
  	inline float chams_color[4]{ 1.0f, 0.0f, 0.0f, 0.5f };
  
  	inline bool localplayer{ false };
  
  	inline bool highlights{ false };
  	inline float highlights_color[4]{ 1.0f, 1.0f, 1.0f, 0.4f };
  
  	inline bool healthbar{ false };
  	inline float healthbar_color[3]{ 0.0f, 1.0f, 0.0f };
  	inline bool health_text{ false };
  	inline float health_text_color[3]{ 1.0f, 1.0f, 1.0f };
  	inline bool feature_indicator{ false };
  	inline float feature_indicator_x{ 50.0f };
  	inline float feature_indicator_y{ 0.0f };
  	inline bool target{ false };
  	inline bool visible_check{ false };
  	inline float visible_color[4]{ 0.0f, 1.0f, 0.0f, 1.0f };
  	inline float invisible_color[4]{ 1.0f, 0.0f, 0.0f, 1.0f };
  	}
 
	namespace expl
	{
 		inline bool walkspeed{ false };
 		inline float walkspeed_speed{ 16.0f };
 		inline int walkspeed_mode{ 0 };
 		inline float walkspeed_health_threshold{ 50.0f };
		inline int walkspeed_keybind{ 0 };
		inline int walkspeed_keybind_mode{ 0 };
		
		inline bool tickrate{ false };
 		inline float tickrate_amount{ 0 };
 		
inline bool fly_enabled{ false };
		inline float fly_speed{ 50.0f };
		inline int fly_mode{ 0 }; // 0 = velocity, 1 = cframe
		inline int fly_keybind{ 0 };
		inline int fly_keybind_mode{ 0 };

		inline bool fly_noclip_enabled{ false }; // Fly + Noclip combined
		inline int fly_noclip_keybind{ 0 };
		inline int fly_noclip_keybind_mode{ 0 };

		inline bool vehicle_fly_enabled{ false };
		inline float vehicle_fly_speed{ 100.0f };
		inline int vehicle_fly_keybind{ 0 };
		inline int vehicle_fly_keybind_mode{ 0 };

		inline bool noclip_enabled{ false };
		inline int noclip_keybind{ 0 };
		inline int noclip_keybind_mode{ 0 };

		inline bool jump_height_enabled{ false };
		inline float jump_height{ 50.0f };

	inline bool teleport{ false };
	inline int teleport_keybind{ 0 };
	inline int teleport_keybind_mode{ 0 };
	inline int teleport_type{ 0 }; // 0 = Nearest Player
	
	inline bool click_tp{ false };
	inline int click_tp_keybind{ 0 };
	inline int click_tp_keybind_mode{ 0 };
	inline std::string selected_player_name{ "" };

	inline bool triggerbot{ false };
	inline int triggerbot_keybind{ 0 };
	inline int triggerbot_keybind_mode{ 0 };
	inline int triggerbot_delay{ 10 }; // ms

	inline bool bunnyhop{ false };
	inline int bunnyhop_keybind{ VK_SPACE };

	inline bool fov_changer{ false };
	inline float fov_value{ 70.0f };

	inline bool anti_afk{ false };
		
	inline bool gravity_enabled{ false };
	inline float gravity_amount{ 196.2f };

	inline bool infinite_jump{ false };

	inline bool spinbot{ false };
	inline float spinbot_speed{ 50.0f };

	inline bool fling_enabled{ false };
	inline float fling_power{ 5000.0f };

	inline bool free_cam{ false };
	inline float free_cam_speed{ 1.0f };
	inline int free_cam_keybind{ 0 };

	inline bool freeze_player{ false };
	inline int freeze_player_mode{ 0 }; // 0 = Anchor, 1 = Velocity, 2 = Position
	inline int freeze_player_keybind{ 0 };
	inline int freeze_player_keybind_mode{ 0 };
	}
 
	namespace hitbox_expander
	{
 		inline bool enabled{ false };
 		inline float size_x{ 2.2f };
 		inline float size_y{ 2.2f };
 		inline float size_z{ 1.2f };
 	}
 
	namespace dex_explorer
	{
 		inline bool enabled{ false };
 	}

	namespace games
	{
		namespace jailbreak
		{
			inline bool no_fall{ false };
			inline bool instant_interact{ false };
		}
		namespace blox_fruits
		{
			inline bool chest_esp{ false };
			inline bool fruit_esp{ false };
		}
		namespace mm2
		{
			inline bool show_murderer{ false };
			inline bool show_sheriff{ false };
			inline bool show_gun{ false };
		}
	}
}
