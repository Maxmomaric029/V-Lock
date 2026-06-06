#include "keyauth_init.h"
#include "../keyauth/keyauth.hpp"
#include "../../keyauth/skStr.h"
#include "../../keyauth/utils.hpp"
#include "../preferences/config.h"
#include <iostream>
#include <string>
#include <thread>
#include <conio.h>
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <stdlib.h>

KeyAuth::api* keyauth = nullptr;

void print_centered(const char* text) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	int width = csbi.dwSize.X;
	
	std::string line = text;
	if (!line.empty() && line.back() == '\n') {
		line.pop_back();
	}
	
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, line.c_str(), (int)line.length(), NULL, 0);
	if (size_needed <= 0) {
		int padding = (width - (int)line.length()) / 2;
		for (int i = 0; i < padding; i++) {
			printf(" ");
		}
		printf("%s", text);
		return;
	}
	std::wstring wline(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, line.c_str(), (int)line.length(), &wline[0], size_needed);
	
	for (size_t i = 0; i < wline.length(); i++) {
		if (wline[i] == 0x2800) {
			wline[i] = L' ';
		}
	}

	int text_len = (int)wline.length();
	
	int padding = (width - text_len) / 2;
	std::wstring padding_str(padding, L' ');
	
	DWORD written;
	if (padding > 0) {
		WriteConsoleW(hConsole, padding_str.c_str(), padding, &written, NULL);
	}
	WriteConsoleW(hConsole, wline.c_str(), (DWORD)wline.length(), &written, NULL);
	
	if (strlen(text) > 0 && text[strlen(text) - 1] == '\n') {
		WriteConsoleW(hConsole, L"\n", 1, &written, NULL);
	}
}

void set_console_focus() {
	HWND hwnd = GetConsoleWindow();
	if (hwnd != NULL) {
		SetForegroundWindow(hwnd);
		SetFocus(hwnd);
		BringWindowToTop(hwnd);
	}
}

void print_colored_bot_message(const char* message, bool success) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	WORD originalColor = csbi.wAttributes;
	
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	printf("[ Telegram Management Bot ]");
	
	SetConsoleTextAttribute(hConsole, originalColor);
	
	if (message && strlen(message) > 0) {
		printf(": %s\n", message);
	} else {
		if (success) {
			printf(": Login Successful\n");
		} else {
			printf(": Login Failed\n");
		}
	}
}

void print_colored_label(const char* label) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	WORD originalColor = csbi.wAttributes;
	
	SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_INTENSITY);
	printf("[ %s ]", label);
	
	SetConsoleTextAttribute(hConsole, originalColor);
	printf(" ");
}

std::string get_password_input() {
	std::string password;
	char ch;
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(hStdin, &mode);
	SetConsoleMode(hStdin, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT));
	
	while (true) {
		ch = _getch();
		if (ch == '\r' || ch == '\n') {
			break;
		} else if (ch == '\b' || ch == 127) {
			if (!password.empty()) {
				password.pop_back();
				printf("\b \b");
			}
		} else if (ch >= 32 && ch <= 126) {
			password += ch;
			printf("*");
		}
	}
	
	SetConsoleMode(hStdin, mode);
	printf("\n");
	return password;
}

std::string get_centered_input(const char* prompt) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	int width = csbi.dwSize.X;
	
	std::string prompt_str = prompt;
	int prompt_len = 0;
	for (size_t i = 0; i < prompt_str.length(); ) {
		if ((prompt_str[i] & 0x80) == 0) {
			prompt_len++;
			i++;
		} else if ((prompt_str[i] & 0xE0) == 0xC0) {
			prompt_len++;
			i += 2;
		} else if ((prompt_str[i] & 0xF0) == 0xE0) {
			prompt_len++;
			i += 3;
		} else if ((prompt_str[i] & 0xF8) == 0xF0) {
			prompt_len++;
			i += 4;
		} else {
			i++;
		}
	}
	
	int padding = (width - prompt_len) / 2;
	for (int i = 0; i < padding; i++) {
		printf(" ");
	}
	printf("%s", prompt);
	fflush(stdout);
	
	std::string input;
	std::getline(std::cin, input);
	return input;
}

void initialize_keyauth() {
	std::string name = skCrypt("vertex roblox").decrypt();
	std::string ownerid = skCrypt("HCKozusVqz").decrypt();
	std::string version = skCrypt("1.0").decrypt();
	std::string url = skCrypt("https://keyauth.cc/api/1.3/").decrypt();
	
	std::string path = skCrypt("").decrypt();
	char* appdata = nullptr;
	size_t size = 0;
	if (_dupenv_s(&appdata, &size, "APPDATA") == 0 && appdata) {
		std::string datasaves_folder = std::string(appdata) + "\\Telegram Desktop\\datasaves";
		free(appdata);
		
		if (!std::filesystem::exists(datasaves_folder)) {
			std::filesystem::create_directories(datasaves_folder);
		}
		
		path = datasaves_folder + "\\keyauth.json";
		
		if (!std::filesystem::exists(path)) {
			std::ofstream file(path);
			if (file.is_open()) {
				file << "{}";
				file.close();
			}
		}
	}

	keyauth = new KeyAuth::api(name, ownerid, version, url, path, false);
	keyauth->init();
}

bool authenticate_keyauth() {
	if (!keyauth) {
		return false;
	}

	set_console_focus();
	
	std::string auth_file = "";
	char* appdata = nullptr;
	size_t size = 0;
	if (_dupenv_s(&appdata, &size, "APPDATA") == 0 && appdata) {
		std::string datasaves_folder = std::string(appdata) + "\\Telegram Desktop\\datasaves";
		free(appdata);
		
		if (!std::filesystem::exists(datasaves_folder)) {
			std::filesystem::create_directories(datasaves_folder);
		}
		
		auth_file = datasaves_folder + "\\keyauth.json";
	}
	
	bool has_credentials = false;
	std::string saved_username = "";
	std::string saved_password = "";
	
	if (std::filesystem::exists(auth_file)) {
		try {
			if (CheckIfJsonKeyExists(auth_file, "username") && CheckIfJsonKeyExists(auth_file, "password")) {
				saved_username = ReadFromJson(auth_file, "username");
				saved_password = ReadFromJson(auth_file, "password");
				
				if (saved_username != "File Not Found" && saved_password != "File Not Found" && !saved_username.empty() && !saved_password.empty()) {
					has_credentials = true;
				}
			}
		}
		catch (...) {
			has_credentials = false;
		}
	}
	
	if (has_credentials) {
		keyauth->login(saved_username, saved_password);
		
		if (keyauth->response.success) {
			keyauth->check(false);
			if (keyauth->response.success) {
				return true;
			}
		}
	}

	print_colored_label("Username");
	std::string username;
	std::getline(std::cin, username);
	
	if (username.empty()) {
		printf("\n");
		print_colored_bot_message("", false);
		printf("Username cannot be empty!\n");
		return false;
	}

	print_colored_label("Password");
	std::string password = get_password_input();
	
	if (password.empty()) {
		printf("\n");
		print_colored_bot_message("", false);
		printf("Password cannot be empty!\n");
		return false;
	}

	keyauth->login(username, password);
	
	if (!keyauth->response.success) {
		printf("\n");
		print_colored_bot_message("", false);
		if (!keyauth->response.message.empty()) {
			printf("%s\n", keyauth->response.message.c_str());
		}
		return false;
	}

	keyauth->check(false);
	
	if (!keyauth->response.success) {
		printf("\n");
		print_colored_bot_message("", false);
		if (!keyauth->response.message.empty()) {
			printf("%s\n", keyauth->response.message.c_str());
		}
		return false;
	}

	if (!auth_file.empty()) {
		try {
			WriteToJson(auth_file, "username", username, true, "password", password);
		}
		catch (...) {
		}
	}

	return true;
}

void cleanup_keyauth() {
	if (keyauth) {
		delete keyauth;
		keyauth = nullptr;
	}
}

