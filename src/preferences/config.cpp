#include "config.h"
#include "../settings.h"
#include <iostream>
#include <sstream>
#include <iomanip>

namespace config
{
	std::string get_config_folder()
	{
		char* appdata = nullptr;
		size_t size = 0;
		if (_dupenv_s(&appdata, &size, "APPDATA") == 0 && appdata)
		{
			std::string folder = std::string(appdata) + "\\Vertex\\Configs";
			free(appdata);
			return folder;
		}
		return "";
	}

	bool create_config_folder()
	{
		std::string folder = get_config_folder();
		if (folder.empty())
			return false;

		if (!std::filesystem::exists(folder))
		{
			std::filesystem::create_directories(folder);
		}
		return true;
	}

	std::vector<config_file_t> get_config_files()
	{
		std::vector<config_file_t> configs;
		std::string folder = get_config_folder();
		
		if (folder.empty() || !std::filesystem::exists(folder))
			return configs;

		for (const auto& entry : std::filesystem::directory_iterator(folder))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
			{
				config_file_t config;
				config.name = entry.path().stem().string();
				config.path = entry.path().string();
				configs.push_back(config);
			}
		}

		return configs;
	}

	std::string escape_json_string(const std::string& str)
	{
		std::ostringstream o;
		for (size_t i = 0; i < str.length(); ++i)
		{
			switch (str[i])
			{
			case '"': o << "\\\""; break;
			case '\\': o << "\\\\"; break;
			case '\b': o << "\\b"; break;
			case '\f': o << "\\f"; break;
			case '\n': o << "\\n"; break;
			case '\r': o << "\\r"; break;
			case '\t': o << "\\t"; break;
			default:
				if ('\x00' <= str[i] && str[i] <= '\x1f')
				{
					o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)str[i];
				}
				else
				{
					o << str[i];
				}
			}
		}
		return o.str();
	}

	bool save_config(const std::string& name)
	{
		std::string folder = get_config_folder();
		if (folder.empty())
			return false;

		if (!std::filesystem::exists(folder))
		{
			std::filesystem::create_directories(folder);
		}

		std::string file_path = folder + "\\" + name + ".json";
		std::ofstream file(file_path);
		
		if (!file.is_open())
			return false;

		file << "{\n";

		// Menu settings
		file << "  \"menu\": {\n";
		file << "    \"accent_color\": [" << menu::accent_color.x << "," << menu::accent_color.y << "," << menu::accent_color.z << "," << menu::accent_color.w << "],\n";
		file << "    \"watermark\": " << (menu::watermark ? "true" : "false") << ",\n";
		file << "    \"watermark_pos\": [" << menu::watermark_pos.x << "," << menu::watermark_pos.y << "],\n";
		file << "    \"streamproof\": " << (menu::streamproof ? "true" : "false") << ",\n";
		file << "    \"hide_console\": " << (menu::hide_console ? "true" : "false") << ",\n";
		file << "    \"menu_keybind\": " << menu::menu_keybind << "\n";
		file << "  },\n";

		// Aimbot settings
		file << "  \"aimbot\": {\n";
		file << "    \"enabled\": " << (settings::aimbot::enabled ? "true" : "false") << ",\n";
		file << "    \"aimbot_type\": " << settings::aimbot::aimbot_type << ",\n";
		file << "    \"sticky_aim\": " << (settings::aimbot::sticky_aim ? "true" : "false") << ",\n";
		file << "    \"draw_fov\": " << (settings::aimbot::draw_fov ? "true" : "false") << ",\n";
		file << "    \"filled_fov\": " << (settings::aimbot::filled_fov ? "true" : "false") << ",\n";
		file << "    \"rotate_fov\": " << (settings::aimbot::rotate_fov ? "true" : "false") << ",\n";
		file << "    \"rainbow_fov\": " << (settings::aimbot::rainbow_fov ? "true" : "false") << ",\n";
		file << "    \"fov\": " << settings::aimbot::fov << ",\n";
		file << "    \"fov_color\": [" << settings::aimbot::fov_color[0] << "," << settings::aimbot::fov_color[1] << "," << settings::aimbot::fov_color[2] << "," << settings::aimbot::fov_color[3] << "],\n";
		file << "    \"fov_check\": " << (settings::aimbot::fov_check ? "true" : "false") << ",\n";
		file << "    \"knocked_check\": " << (settings::aimbot::knocked_check ? "true" : "false") << ",\n";
		file << "    \"keybind\": " << settings::aimbot::keybind << ",\n";
		file << "    \"keybind_mode\": " << settings::aimbot::keybind_mode << ",\n";
		file << "    \"aim_part\": " << settings::aimbot::aim_part << ",\n";
		file << "    \"mouse_smooth\": " << (settings::aimbot::mouse_smooth ? "true" : "false") << ",\n";
		file << "    \"mouse_smooth_x\": " << settings::aimbot::mouse_smooth_x << ",\n";
		file << "    \"mouse_smooth_y\": " << settings::aimbot::mouse_smooth_y << ",\n";
		file << "    \"mouse_sensitivity\": " << settings::aimbot::mouse_sensitivity << ",\n";
		file << "    \"mouse_prediction\": " << (settings::aimbot::mouse_prediction ? "true" : "false") << ",\n";
		file << "    \"mouse_prediction_x\": " << settings::aimbot::mouse_prediction_x << ",\n";
		file << "    \"mouse_prediction_y\": " << settings::aimbot::mouse_prediction_y << ",\n";
		file << "    \"camera_smooth\": " << (settings::aimbot::camera_smooth ? "true" : "false") << ",\n";
		file << "    \"camera_smooth_x\": " << settings::aimbot::camera_smooth_x << ",\n";
		file << "    \"camera_smooth_y\": " << settings::aimbot::camera_smooth_y << ",\n";
		file << "    \"camera_prediction\": " << (settings::aimbot::camera_prediction ? "true" : "false") << ",\n";
		file << "    \"camera_prediction_x\": " << settings::aimbot::camera_prediction_x << ",\n";
		file << "    \"camera_prediction_y\": " << settings::aimbot::camera_prediction_y << ",\n";
		file << "    \"shake\": " << (settings::aimbot::shake ? "true" : "false") << ",\n";
		file << "    \"shake_x\": " << settings::aimbot::shake_x << ",\n";
		file << "    \"shake_y\": " << settings::aimbot::shake_y << "\n";
		file << "  },\n";

		// Silent settings
		file << "  \"silent\": {\n";
		file << "    \"enabled\": " << (settings::silent::enabled ? "true" : "false") << ",\n";
		file << "    \"sticky_aim\": " << (settings::silent::sticky_aim ? "true" : "false") << ",\n";
		file << "    \"spoof_mouse\": " << (settings::silent::spoof_mouse ? "true" : "false") << ",\n";
		file << "    \"draw_fov\": " << (settings::silent::draw_fov ? "true" : "false") << ",\n";
		file << "    \"filled_fov\": " << (settings::silent::filled_fov ? "true" : "false") << ",\n";
		file << "    \"rotate_fov\": " << (settings::silent::rotate_fov ? "true" : "false") << ",\n";
		file << "    \"rainbow_fov\": " << (settings::silent::rainbow_fov ? "true" : "false") << ",\n";
		file << "    \"fov\": " << settings::silent::fov << ",\n";
		file << "    \"keybind\": " << settings::silent::keybind << ",\n";
		file << "    \"keybind_mode\": " << settings::silent::keybind_mode << ",\n";
		file << "    \"aim_part\": " << settings::silent::aim_part << ",\n";
		file << "    \"fov_check\": " << (settings::silent::fov_check ? "true" : "false") << ",\n";
		file << "    \"knocked_check\": " << (settings::silent::knocked_check ? "true" : "false") << ",\n";
		file << "    \"gun_based_fov\": " << (settings::silent::gun_based_fov ? "true" : "false") << ",\n";
		file << "    \"fov_double_barrel\": " << settings::silent::fov_double_barrel << ",\n";
		file << "    \"fov_tactical_shotgun\": " << settings::silent::fov_tactical_shotgun << ",\n";
		file << "    \"fov_revolver\": " << settings::silent::fov_revolver << ",\n";
		file << "    \"fov_color\": [" << settings::silent::fov_color[0] << "," << settings::silent::fov_color[1] << "," << settings::silent::fov_color[2] << "," << settings::silent::fov_color[3] << "]\n";
		file << "  },\n";

		// Visuals settings
		file << "  \"visuals\": {\n";
		file << "    \"box\": " << (settings::visuals::box ? "true" : "false") << ",\n";
		file << "    \"box_color\": [" << settings::visuals::box_color[0] << "," << settings::visuals::box_color[1] << "," << settings::visuals::box_color[2] << "," << settings::visuals::box_color[3] << "],\n";
		file << "    \"name\": " << (settings::visuals::name ? "true" : "false") << ",\n";
		file << "    \"name_color\": [" << settings::visuals::name_color[0] << "," << settings::visuals::name_color[1] << "," << settings::visuals::name_color[2] << "," << settings::visuals::name_color[3] << "],\n";
		file << "    \"distance\": " << (settings::visuals::distance ? "true" : "false") << ",\n";
		file << "    \"distance_color\": [" << settings::visuals::distance_color[0] << "," << settings::visuals::distance_color[1] << "," << settings::visuals::distance_color[2] << "," << settings::visuals::distance_color[3] << "],\n";
		file << "    \"tool\": " << (settings::visuals::tool ? "true" : "false") << ",\n";
		file << "    \"tool_color\": [" << settings::visuals::tool_color[0] << "," << settings::visuals::tool_color[1] << "," << settings::visuals::tool_color[2] << "," << settings::visuals::tool_color[3] << "],\n";
		file << "    \"weapon_icon\": " << (settings::visuals::weapon_icon ? "true" : "false") << ",\n";
		file << "    \"weapon_icon_color\": [" << settings::visuals::weapon_icon_color[0] << "," << settings::visuals::weapon_icon_color[1] << "," << settings::visuals::weapon_icon_color[2] << "," << settings::visuals::weapon_icon_color[3] << "],\n";
		file << "    \"localplayer\": " << (settings::visuals::localplayer ? "true" : "false") << ",\n";
		file << "    \"highlights\": " << (settings::visuals::highlights ? "true" : "false") << ",\n";
		file << "    \"highlights_color\": [" << settings::visuals::highlights_color[0] << "," << settings::visuals::highlights_color[1] << "," << settings::visuals::highlights_color[2] << "," << settings::visuals::highlights_color[3] << "],\n";
		file << "    \"healthbar\": " << (settings::visuals::healthbar ? "true" : "false") << ",\n";
		file << "    \"healthbar_color\": [" << settings::visuals::healthbar_color[0] << "," << settings::visuals::healthbar_color[1] << "," << settings::visuals::healthbar_color[2] << "],\n";
		file << "    \"health_text\": " << (settings::visuals::health_text ? "true" : "false") << ",\n";
		file << "    \"health_text_color\": [" << settings::visuals::health_text_color[0] << "," << settings::visuals::health_text_color[1] << "," << settings::visuals::health_text_color[2] << "],\n";
		file << "    \"target\": " << (settings::visuals::target ? "true" : "false") << "\n";
		file << "  },\n";

		// Exploits settings
		file << "  \"expl\": {\n";
		file << "    \"walkspeed\": " << (settings::expl::walkspeed ? "true" : "false") << ",\n";
		file << "    \"walkspeed_speed\": " << settings::expl::walkspeed_speed << ",\n";
		file << "    \"walkspeed_mode\": " << settings::expl::walkspeed_mode << ",\n";
		file << "    \"walkspeed_health_threshold\": " << settings::expl::walkspeed_health_threshold << ",\n";
		file << "    \"walkspeed_keybind\": " << settings::expl::walkspeed_keybind << ",\n";
		file << "    \"walkspeed_keybind_mode\": " << settings::expl::walkspeed_keybind_mode << "\n";
		file << "  }\n";

		file << "}\n";

		file.close();
		return true;
	}

	std::string extract_json_section(const std::string& json, const std::string& section_name)
	{
		std::string search_key = "\"" + section_name + "\"";
		size_t pos = json.find(search_key);
		if (pos == std::string::npos)
			return "";

		pos = json.find("{", pos);
		if (pos == std::string::npos)
			return "";

		int brace_count = 1;
		size_t start = pos;
		pos++;
		while (pos < json.length() && brace_count > 0)
		{
			if (json[pos] == '{') brace_count++;
			else if (json[pos] == '}') brace_count--;
			pos++;
		}

		return json.substr(start, pos - start);
	}

	bool parse_json_value(const std::string& json, const std::string& key, std::string& value)
	{
		std::string search_key = "\"" + key + "\"";
		size_t pos = json.find(search_key);
		if (pos == std::string::npos)
			return false;

		pos = json.find(":", pos);
		if (pos == std::string::npos)
			return false;

		pos++;
		while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
			pos++;

		size_t end_pos = pos;
		if (json[pos] == '"')
		{
			pos++;
			end_pos = json.find('"', pos);
			if (end_pos == std::string::npos) return false;
		}
		else if (json[pos] == '[')
		{
			int bracket_count = 1;
			end_pos = pos + 1;
			while (end_pos < json.length() && bracket_count > 0)
			{
				if (json[end_pos] == '[') bracket_count++;
				else if (json[end_pos] == ']') bracket_count--;
				end_pos++;
			}
		}
		else
		{
			while (end_pos < json.length() && json[end_pos] != ',' && json[end_pos] != '}' && json[end_pos] != '\n' && json[end_pos] != '\r')
				end_pos++;
		}

		value = json.substr(pos, end_pos - pos);
		if (value.length() > 0 && value.front() == '"' && value.back() == '"')
			value = value.substr(1, value.length() - 2);

		return true;
	}

	bool parse_json_array(const std::string& array_str, float* values, size_t count)
	{
		std::string str = array_str;
		if (str.front() == '[') str = str.substr(1);
		if (str.back() == ']') str = str.substr(0, str.length() - 1);

		std::istringstream iss(str);
		std::string token;
		size_t idx = 0;

		while (std::getline(iss, token, ',') && idx < count)
		{
			token.erase(0, token.find_first_not_of(" \t"));
			token.erase(token.find_last_not_of(" \t") + 1);
			try {
				values[idx++] = std::stof(token);
			}
			catch (...) {
				return false;
			}
		}

		return idx == count;
	}

	bool load_config(const std::string& name)
	{
		std::string folder = get_config_folder();
		if (folder.empty())
			return false;

		std::string file_path = folder + "\\" + name + ".json";
		
		if (!std::filesystem::exists(file_path))
			return false;

		std::ifstream file(file_path);
		
		if (!file.is_open())
			return false;

		std::string json_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		std::string value;
		std::string section;

		// Parse menu settings
		section = extract_json_section(json_content, "menu");
		if (!section.empty())
		{
			if (parse_json_value(section, "accent_color", value))
			{
				float colors[4];
				if (parse_json_array(value, colors, 4))
				{
					menu::accent_color.x = colors[0];
					menu::accent_color.y = colors[1];
					menu::accent_color.z = colors[2];
					menu::accent_color.w = colors[3];
				}
			}
			if (parse_json_value(section, "watermark", value))
				menu::watermark = (value == "true");
			if (parse_json_value(section, "watermark_pos", value))
			{
				float pos[2];
				if (parse_json_array(value, pos, 2))
				{
					menu::watermark_pos.x = pos[0];
					menu::watermark_pos.y = pos[1];
				}
			}
			if (parse_json_value(section, "streamproof", value))
				menu::streamproof = (value == "true");
			if (parse_json_value(section, "hide_console", value))
				menu::hide_console = (value == "true");
			if (parse_json_value(section, "menu_keybind", value))
				menu::menu_keybind = std::stoi(value);
		}

		// Parse aimbot settings
		section = extract_json_section(json_content, "aimbot");
		if (!section.empty())
		{
			if (parse_json_value(section, "enabled", value))
				settings::aimbot::enabled = (value == "true");
			if (parse_json_value(section, "aimbot_type", value))
				settings::aimbot::aimbot_type = std::stoi(value);
			if (parse_json_value(section, "sticky_aim", value))
				settings::aimbot::sticky_aim = (value == "true");
			if (parse_json_value(section, "draw_fov", value))
				settings::aimbot::draw_fov = (value == "true");
			if (parse_json_value(section, "filled_fov", value))
				settings::aimbot::filled_fov = (value == "true");
			if (parse_json_value(section, "rotate_fov", value))
				settings::aimbot::rotate_fov = (value == "true");
			if (parse_json_value(section, "rainbow_fov", value))
				settings::aimbot::rainbow_fov = (value == "true");
			if (parse_json_value(section, "fov", value))
				settings::aimbot::fov = std::stof(value);
			if (parse_json_value(section, "fov_color", value))
				parse_json_array(value, settings::aimbot::fov_color, 4);
			if (parse_json_value(section, "fov_check", value))
				settings::aimbot::fov_check = (value == "true");
			if (parse_json_value(section, "knocked_check", value))
				settings::aimbot::knocked_check = (value == "true");
			if (parse_json_value(section, "keybind", value))
				settings::aimbot::keybind = std::stoi(value);
			if (parse_json_value(section, "keybind_mode", value))
				settings::aimbot::keybind_mode = std::stoi(value);
			if (parse_json_value(section, "aim_part", value))
				settings::aimbot::aim_part = std::stoi(value);
			if (parse_json_value(section, "mouse_smooth", value))
				settings::aimbot::mouse_smooth = (value == "true");
			if (parse_json_value(section, "mouse_smooth_x", value))
				settings::aimbot::mouse_smooth_x = std::stof(value);
			if (parse_json_value(section, "mouse_smooth_y", value))
				settings::aimbot::mouse_smooth_y = std::stof(value);
			if (parse_json_value(section, "mouse_sensitivity", value))
				settings::aimbot::mouse_sensitivity = std::stof(value);
			if (parse_json_value(section, "mouse_prediction", value))
				settings::aimbot::mouse_prediction = (value == "true");
			if (parse_json_value(section, "mouse_prediction_x", value))
				settings::aimbot::mouse_prediction_x = std::stof(value);
			if (parse_json_value(section, "mouse_prediction_y", value))
				settings::aimbot::mouse_prediction_y = std::stof(value);
			if (parse_json_value(section, "camera_smooth", value))
				settings::aimbot::camera_smooth = (value == "true");
			if (parse_json_value(section, "camera_smooth_x", value))
				settings::aimbot::camera_smooth_x = std::stof(value);
			if (parse_json_value(section, "camera_smooth_y", value))
				settings::aimbot::camera_smooth_y = std::stof(value);
			if (parse_json_value(section, "camera_prediction", value))
				settings::aimbot::camera_prediction = (value == "true");
			if (parse_json_value(section, "camera_prediction_x", value))
				settings::aimbot::camera_prediction_x = std::stof(value);
			if (parse_json_value(section, "camera_prediction_y", value))
				settings::aimbot::camera_prediction_y = std::stof(value);
			if (parse_json_value(section, "shake", value))
				settings::aimbot::shake = (value == "true");
			if (parse_json_value(section, "shake_x", value))
				settings::aimbot::shake_x = std::stof(value);
			if (parse_json_value(section, "shake_y", value))
				settings::aimbot::shake_y = std::stof(value);
		}

		// Parse silent settings
		section = extract_json_section(json_content, "silent");
		if (!section.empty())
		{
			if (parse_json_value(section, "enabled", value))
				settings::silent::enabled = (value == "true");
			if (parse_json_value(section, "sticky_aim", value))
				settings::silent::sticky_aim = (value == "true");
			if (parse_json_value(section, "spoof_mouse", value))
				settings::silent::spoof_mouse = (value == "true");
			if (parse_json_value(section, "draw_fov", value))
				settings::silent::draw_fov = (value == "true");
			if (parse_json_value(section, "filled_fov", value))
				settings::silent::filled_fov = (value == "true");
			if (parse_json_value(section, "rotate_fov", value))
				settings::silent::rotate_fov = (value == "true");
			if (parse_json_value(section, "rainbow_fov", value))
				settings::silent::rainbow_fov = (value == "true");
			if (parse_json_value(section, "fov", value))
				settings::silent::fov = std::stof(value);
			if (parse_json_value(section, "keybind", value))
				settings::silent::keybind = std::stoi(value);
			if (parse_json_value(section, "keybind_mode", value))
				settings::silent::keybind_mode = std::stoi(value);
			if (parse_json_value(section, "aim_part", value))
				settings::silent::aim_part = std::stoi(value);
			if (parse_json_value(section, "fov_check", value))
				settings::silent::fov_check = (value == "true");
			if (parse_json_value(section, "knocked_check", value))
				settings::silent::knocked_check = (value == "true");
			if (parse_json_value(section, "gun_based_fov", value))
				settings::silent::gun_based_fov = (value == "true");
			if (parse_json_value(section, "fov_double_barrel", value))
				settings::silent::fov_double_barrel = std::stof(value);
			if (parse_json_value(section, "fov_tactical_shotgun", value))
				settings::silent::fov_tactical_shotgun = std::stof(value);
			if (parse_json_value(section, "fov_revolver", value))
				settings::silent::fov_revolver = std::stof(value);
			if (parse_json_value(section, "fov_color", value))
				parse_json_array(value, settings::silent::fov_color, 4);
		}

		// Parse visuals settings
		section = extract_json_section(json_content, "visuals");
		if (!section.empty())
		{
			if (parse_json_value(section, "box", value))
				settings::visuals::box = (value == "true");
			if (parse_json_value(section, "box_color", value))
				parse_json_array(value, settings::visuals::box_color, 4);
			if (parse_json_value(section, "name", value))
				settings::visuals::name = (value == "true");
			if (parse_json_value(section, "name_color", value))
				parse_json_array(value, settings::visuals::name_color, 4);
			if (parse_json_value(section, "distance", value))
				settings::visuals::distance = (value == "true");
			if (parse_json_value(section, "distance_color", value))
				parse_json_array(value, settings::visuals::distance_color, 4);
			if (parse_json_value(section, "tool", value))
				settings::visuals::tool = (value == "true");
			if (parse_json_value(section, "tool_color", value))
				parse_json_array(value, settings::visuals::tool_color, 4);
			if (parse_json_value(section, "weapon_icon", value))
				settings::visuals::weapon_icon = (value == "true");
			if (parse_json_value(section, "weapon_icon_color", value))
				parse_json_array(value, settings::visuals::weapon_icon_color, 4);
			if (parse_json_value(section, "localplayer", value))
				settings::visuals::localplayer = (value == "true");
		if (parse_json_value(section, "highlights", value))
			settings::visuals::highlights = (value == "true");
		if (parse_json_value(section, "highlights_color", value))
			parse_json_array(value, settings::visuals::highlights_color, 4);
		if (parse_json_value(section, "healthbar", value))
				settings::visuals::healthbar = (value == "true");
			if (parse_json_value(section, "healthbar_color", value))
				parse_json_array(value, settings::visuals::healthbar_color, 3);
			if (parse_json_value(section, "health_text", value))
				settings::visuals::health_text = (value == "true");
			if (parse_json_value(section, "health_text_color", value))
				parse_json_array(value, settings::visuals::health_text_color, 3);
			if (parse_json_value(section, "target", value))
				settings::visuals::target = (value == "true");
		}

		// Parse exploits settings
		section = extract_json_section(json_content, "expl");
		if (!section.empty())
		{
			if (parse_json_value(section, "walkspeed", value))
				settings::expl::walkspeed = (value == "true");
			if (parse_json_value(section, "walkspeed_speed", value))
				settings::expl::walkspeed_speed = std::stof(value);
			if (parse_json_value(section, "walkspeed_mode", value))
				settings::expl::walkspeed_mode = std::stoi(value);
			if (parse_json_value(section, "walkspeed_health_threshold", value))
				settings::expl::walkspeed_health_threshold = std::stof(value);
			if (parse_json_value(section, "walkspeed_keybind", value))
				settings::expl::walkspeed_keybind = std::stoi(value);
			if (parse_json_value(section, "walkspeed_keybind_mode", value))
				settings::expl::walkspeed_keybind_mode = std::stoi(value);
		}

		return true;
	}

	bool delete_config(const std::string& name)
	{
		std::string folder = get_config_folder();
		if (folder.empty())
			return false;

		std::string file_path = folder + "\\" + name + ".json";
		
		if (!std::filesystem::exists(file_path))
			return false;

		return std::filesystem::remove(file_path);
	}
}
