    #define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX
#include <display/render.h>
#include "render_helpers.h"
#include "notifications.h"
#include <modules/assist/silent.h>
#include <dwmapi.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <algorithm>
#include <cstring>
#include <windows.h>
#include <shellapi.h>

#ifndef WDA_NONE
#define WDA_NONE 0x00000000
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

typedef BOOL(WINAPI* SetWindowDisplayAffinityProc)(HWND, DWORD);

#include <settings.h>
#include <verify/typing_check.h>
#include <modules/markers/esp.h>
#include <modules/assist/silent.h>
#include <modules/inspector/dex_explorer.h>
#include "visitor.h"
#include "../resources/WeaponIcon.hpp"
#include "../preferences/config.h"
#include <runtime/memory.h>
#include <native/offsets.h>
#include <session/rescan.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


// helpers
static int selected_tab_index = 0;
static float animationProgress = 0.0f;
static bool waiting_for_keybind = false;
static std::unordered_map<std::string, bool> waiting_for_keybind_map;

static const char* get_key_name(int vk_code)
{
    switch (vk_code)
    {
    case 0: return "None";
    case VK_LBUTTON: return "LM";
    case VK_RBUTTON: return "RM";
    case VK_MBUTTON: return "MM";
    case VK_XBUTTON1: return "MB1";
    case VK_XBUTTON2: return "MB2";
    case VK_BACK: return "Backspace";
    case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt";
    case VK_CAPITAL: return "Caps";
    case VK_ESCAPE: return "Esc";
    case VK_SPACE: return "Space";
    case VK_PRIOR: return "PgUp";
    case VK_NEXT: return "PgDown";
    case VK_END: return "End";
    case VK_HOME: return "Home";
    case VK_LEFT: return "Left";
    case VK_UP: return "Up";
    case VK_RIGHT: return "Right";
    case VK_DOWN: return "Down";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_F1: return "F1";
    case VK_F2: return "F2";
    case VK_F3: return "F3";
    case VK_F4: return "F4";
    case VK_F5: return "F5";
    case VK_F6: return "F6";
    case VK_F7: return "F7";
    case VK_F8: return "F8";
    case VK_F9: return "F9";
    case VK_F10: return "F10";
    case VK_F11: return "F11";
    case VK_F12: return "F12";
    default:
        if (vk_code >= 'A' && vk_code <= 'Z') { static char buf[2]; buf[0] = (char)vk_code; buf[1] = 0; return buf; }
        if (vk_code >= '0' && vk_code <= '9') { static char buf[2]; buf[0] = (char)vk_code; buf[1] = 0; return buf; }
        return "???";
    }
}

static std::unordered_map<std::string, float> keybind_tween_progress;
static std::unordered_map<std::string, bool> keybind_popup_open;
static std::unordered_map<std::string, ImVec2> keybind_popup_pos;
static std::unordered_map<std::string, bool> multiselect_popup_open;
static std::unordered_map<std::string, ImVec2> multiselect_popup_pos;

static bool inline_keybind_button(const char* label, int* key, int* mode = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    std::string key_id = label;
    if (waiting_for_keybind_map.find(key_id) == waiting_for_keybind_map.end())
        waiting_for_keybind_map[key_id] = false;

    bool is_waiting = waiting_for_keybind_map[key_id];
    const char* display_text;
    if (is_waiting)
        display_text = "..";
    else if (*key == 0)
        display_text = "-";
    else
        display_text = get_key_name(*key);

    ImVec2 label_size = ImGui::CalcTextSize(display_text, nullptr, true);
    float checkbox_height = ImGui::GetFrameHeight();
    ImVec2 button_size = ImVec2(label_size.x + style.FramePadding.x * 2.0f + 16.0f, checkbox_height);
    if (button_size.x < 40.0f) button_size.x = 40.0f;

    ImVec2 pos = window->DC.CursorPos;

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(label)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

    if (keybind_popup_open.find(key_id) == keybind_popup_open.end())
        keybind_popup_open[key_id] = false;

    if (mode != nullptr && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        keybind_popup_open[key_id] = !keybind_popup_open[key_id];
        keybind_popup_pos[key_id] = ImVec2(pos.x + button_size.x + 5, pos.y);
    }

    if (pressed)
        waiting_for_keybind_map[key_id] = !waiting_for_keybind_map[key_id];

    is_waiting = waiting_for_keybind_map[key_id];
    if (is_waiting)
    {
        for (int i = 1; i < 256; i++)
        {
            if (i == VK_ESCAPE)
            {
                if (GetAsyncKeyState(i) & 0x8000)
                {
                    *key = 0;
                    waiting_for_keybind_map[key_id] = false;
                    break;
                }
                continue;
            }
            
            if (GetAsyncKeyState(i) & 0x8000)
            {
                *key = i;
                waiting_for_keybind_map[key_id] = false;
                break;
            }
        }
    }

    if (keybind_tween_progress.find(key_id) == keybind_tween_progress.end())
        keybind_tween_progress[key_id] = 0.0f;

    float& tween_progress = keybind_tween_progress[key_id];

    ImVec4 base_color = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    ImVec4 target_color = menu::accent_color;

    float target_progress = 0.0f;
    if (is_waiting || held)
        target_progress = 1.0f;
    else if (hovered)
        target_progress = 0.5f;

    float tween_speed = 4.0f;
    tween_progress += (target_progress - tween_progress) * g.IO.DeltaTime * tween_speed;

    ImVec4 tweened_color = ImVec4(
        base_color.x + (target_color.x - base_color.x) * tween_progress,
        base_color.y + (target_color.y - base_color.y) * tween_progress,
        base_color.z + (target_color.z - base_color.z) * tween_progress,
        base_color.w + (target_color.w - base_color.w) * tween_progress
    );

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char bracket_text[64];
    sprintf_s(bracket_text, "[ %s ]", display_text);
    ImVec2 bracket_text_size = ImGui::CalcTextSize(bracket_text);
    float text_y = pos.y + style.FramePadding.y - 4.0f;
    ImVec2 text_pos = ImVec2(bb.Min.x + (button_size.x - bracket_text_size.x) * 0.5f, text_y);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            dl->AddText(ImVec2(text_pos.x + x * 1.f, text_pos.y + y * 1.f), IM_COL32_BLACK, bracket_text);
        }
    }
    dl->AddText(text_pos, IM_COL32_WHITE, bracket_text);

    if (mode != nullptr && keybind_popup_open[key_id])
    {
        ImGui::SetNextWindowPos(keybind_popup_pos[key_id], ImGuiCond_Always);
        
        bool is_walkspeed = (strcmp(label, "walkspeed_keybind") == 0);
        bool is_silent = (strcmp(label, "silent_keybind") == 0);
        int mode_count = (is_walkspeed || is_silent) ? 3 : 2;
        ImVec2 popup_size = ImVec2(80, mode_count == 3 ? 100 : 80);
        ImGui::SetNextWindowSize(popup_size);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(menu::accent_color));

        char popup_name[128];
        sprintf_s(popup_name, "##keybind_mode_%s", label);

        ImGui::Begin(popup_name, nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);

        ImVec2 popup_pos = ImGui::GetWindowPos();
        popup_size = ImGui::GetWindowSize();
        ImDrawList* popup_dl = ImGui::GetWindowDrawList();

        const char* modes[3];
        if (is_walkspeed || is_silent)
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
            modes[2] = "Always";
        }
        else
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
        }

        for (int i = 0; i < mode_count; i++)
        {
            ImVec2 item_pos = ImGui::GetCursorScreenPos();
            ImVec2 item_size = ImVec2(popup_size.x - 12, 20);
            ImRect item_bb(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y));

            bool item_hovered = ImGui::IsMouseHoveringRect(item_bb.Min, item_bb.Max);
            bool item_clicked = item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            ImU32 item_bg = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(ImVec4(menu::accent_color.x * 0.3f, menu::accent_color.y * 0.3f, menu::accent_color.z * 0.3f, 1.0f)) :
                           (item_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255));
            popup_dl->AddRectFilled(item_bb.Min, item_bb.Max, item_bg);

            ImVec2 mode_text_size = ImGui::CalcTextSize(modes[i]);
            ImVec2 mode_text_pos = ImVec2(item_bb.Min.x + (item_size.x - mode_text_size.x) * 0.5f, item_bb.Min.y + (item_size.y - mode_text_size.y) * 0.5f);

            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue;
                    popup_dl->AddText(ImVec2(mode_text_pos.x + x, mode_text_pos.y + y), IM_COL32(0, 0, 0, 255), modes[i]);
                }
            }
            
            ImU32 text_col = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(menu::accent_color) : IM_COL32(255, 255, 255, 255);
            popup_dl->AddText(mode_text_pos, text_col, modes[i]);

            if (item_clicked)
            {
                *mode = i;
                keybind_popup_open[key_id] = false;
            }

            ImGui::Dummy(item_size);
        }

        ImVec2 separator_start = ImGui::GetCursorScreenPos();
        separator_start.y += 2;
        ImVec2 separator_end = ImVec2(separator_start.x + popup_size.x - 12, separator_start.y);
        popup_dl->AddLine(separator_start, separator_end, IM_COL32(60, 60, 60, 255));
        ImGui::Dummy(ImVec2(0, 6));

        ImVec2 clear_item_pos = ImGui::GetCursorScreenPos();
        ImVec2 clear_item_size = ImVec2(popup_size.x - 12, 20);
        ImRect clear_item_bb(clear_item_pos, ImVec2(clear_item_pos.x + clear_item_size.x, clear_item_pos.y + clear_item_size.y));

        bool clear_hovered = ImGui::IsMouseHoveringRect(clear_item_bb.Min, clear_item_bb.Max);
        bool clear_clicked = clear_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        ImU32 clear_bg = clear_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255);
        popup_dl->AddRectFilled(clear_item_bb.Min, clear_item_bb.Max, clear_bg);

        const char* clear_text = "Clear";
        ImVec2 clear_text_size = ImGui::CalcTextSize(clear_text);
        ImVec2 clear_text_pos = ImVec2(clear_item_bb.Min.x + (clear_item_size.x - clear_text_size.x) * 0.5f, clear_item_bb.Min.y + (clear_item_size.y - clear_text_size.y) * 0.5f);

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) continue;
                popup_dl->AddText(ImVec2(clear_text_pos.x + x, clear_text_pos.y + y), IM_COL32(0, 0, 0, 255), clear_text);
            }
        }
        
        popup_dl->AddText(clear_text_pos, IM_COL32(255, 255, 255, 255), clear_text);

        if (clear_clicked)
        {
            *key = 0;
            keybind_popup_open[key_id] = false;
        }

        ImGui::Dummy(clear_item_size);

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    return pressed;
}

static bool keybind_button(const char* label, int* key, int* mode = nullptr)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const char* display_text = waiting_for_keybind ? "..." : get_key_name(*key);
    
    ImVec2 label_size = ImGui::CalcTextSize(display_text, nullptr, true);
    ImVec2 button_size = ImVec2(label_size.x + style.FramePadding.x * 4.0f, label_size.y + style.FramePadding.y * 2.0f);
    if (button_size.x < 50.0f) button_size.x = 50.0f;

    ImVec2 pos = window->DC.CursorPos;
    pos.x += 2;

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(label)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

    std::string key_id = label;
    
    if (keybind_popup_open.find(key_id) == keybind_popup_open.end())
        keybind_popup_open[key_id] = false;

    if (mode != nullptr && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
    {
        keybind_popup_open[key_id] = !keybind_popup_open[key_id];
        keybind_popup_pos[key_id] = ImVec2(pos.x + button_size.x + 5, pos.y);
    }

    if (pressed)
        waiting_for_keybind = !waiting_for_keybind;

    if (waiting_for_keybind)
    {
        for (int i = 1; i < 256; i++)
        {
            if (i == VK_ESCAPE)
            {
                if (GetAsyncKeyState(i) & 0x8000)
                {
                    *key = 0;
                    waiting_for_keybind = false;
                    break;
                }
                continue;
            }
            
            if (GetAsyncKeyState(i) & 0x8000)
            {
                *key = i;
                waiting_for_keybind = false;
                break;
            }
        }
    }

    ImU32 col = IM_COL32(35, 35, 35, 255);

    if (keybind_tween_progress.find(key_id) == keybind_tween_progress.end())
        keybind_tween_progress[key_id] = 0.0f;

    float& tween_progress = keybind_tween_progress[key_id];

    ImVec4 base_color = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    ImVec4 target_color = menu::accent_color;

    float target_progress = 0.0f;
    if (waiting_for_keybind || held)
        target_progress = 1.0f;
    else if (hovered)
        target_progress = 0.5f;

    float tween_speed = 4.0f;
    tween_progress += (target_progress - tween_progress) * g.IO.DeltaTime * tween_speed;

    ImVec4 tweened_color = ImVec4(
        base_color.x + (target_color.x - base_color.x) * tween_progress,
        base_color.y + (target_color.y - base_color.y) * tween_progress,
        base_color.z + (target_color.z - base_color.z) * tween_progress,
        base_color.w + (target_color.w - base_color.w) * tween_progress
    );

    ImDrawList* dl = ImGui::GetWindowDrawList();

    char bracket_text[64];
    const char* display_text_bracket;
    if (waiting_for_keybind)
        display_text_bracket = "..";
    else if (*key == 0)
        display_text_bracket = "-";
    else
        display_text_bracket = display_text;
    
    sprintf_s(bracket_text, "[ %s ]", display_text_bracket);
    ImVec2 bracket_text_size = ImGui::CalcTextSize(bracket_text);
    ImVec2 text_pos = ImVec2(bb.Min.x + (button_size.x - bracket_text_size.x) * 0.5f, bb.Min.y + (button_size.y - bracket_text_size.y) * 0.5f);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            dl->AddText(ImVec2(text_pos.x + x * 1.f, text_pos.y + y * 1.f), IM_COL32_BLACK, bracket_text);
        }
    }
    dl->AddText(text_pos, IM_COL32_WHITE, bracket_text);

    if (mode != nullptr && keybind_popup_open[key_id])
    {
        ImGui::SetNextWindowPos(keybind_popup_pos[key_id], ImGuiCond_Always);
        
        bool is_walkspeed = (strcmp(label, "walkspeed_keybind") == 0);
        bool is_silent = (strcmp(label, "silent_keybind") == 0);
        int mode_count = (is_walkspeed || is_silent) ? 3 : 2;
        ImVec2 popup_size = ImVec2(80, mode_count == 3 ? 100 : 80);
        ImGui::SetNextWindowSize(popup_size);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(menu::accent_color));

        char popup_name[128];
        sprintf_s(popup_name, "##keybind_mode_%s", label);

        ImGui::Begin(popup_name, nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);

        ImVec2 popup_pos = ImGui::GetWindowPos();
        popup_size = ImGui::GetWindowSize();
        ImDrawList* popup_dl = ImGui::GetWindowDrawList();

        const char* modes[3];
        if (is_walkspeed || is_silent)
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
            modes[2] = "Always";
        }
        else
        {
            modes[0] = "Hold";
            modes[1] = "Toggle";
        }

        for (int i = 0; i < mode_count; i++)
        {
            ImVec2 item_pos = ImGui::GetCursorScreenPos();
            ImVec2 item_size = ImVec2(popup_size.x - 12, 20);
            ImRect item_bb(item_pos, ImVec2(item_pos.x + item_size.x, item_pos.y + item_size.y));

            bool item_hovered = ImGui::IsMouseHoveringRect(item_bb.Min, item_bb.Max);
            bool item_clicked = item_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

            ImU32 item_bg = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(ImVec4(menu::accent_color.x * 0.3f, menu::accent_color.y * 0.3f, menu::accent_color.z * 0.3f, 1.0f)) :
                           (item_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255));
            popup_dl->AddRectFilled(item_bb.Min, item_bb.Max, item_bg);

            ImVec2 mode_text_size = ImGui::CalcTextSize(modes[i]);
            ImVec2 mode_text_pos = ImVec2(item_bb.Min.x + (item_size.x - mode_text_size.x) * 0.5f, item_bb.Min.y + (item_size.y - mode_text_size.y) * 0.5f);

            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue;
                    popup_dl->AddText(ImVec2(mode_text_pos.x + x, mode_text_pos.y + y), IM_COL32(0, 0, 0, 255), modes[i]);
                }
            }
            
            ImU32 text_col = (*mode == i) ? ImGui::ColorConvertFloat4ToU32(menu::accent_color) : IM_COL32(255, 255, 255, 255);
            popup_dl->AddText(mode_text_pos, text_col, modes[i]);

            if (item_clicked)
            {
                *mode = i;
                keybind_popup_open[key_id] = false;
            }

            ImGui::Dummy(item_size);
        }

        ImVec2 separator_start = ImGui::GetCursorScreenPos();
        separator_start.y += 2;
        ImVec2 separator_end = ImVec2(separator_start.x + popup_size.x - 12, separator_start.y);
        popup_dl->AddLine(separator_start, separator_end, IM_COL32(60, 60, 60, 255));
        ImGui::Dummy(ImVec2(0, 6));

        ImVec2 clear_item_pos = ImGui::GetCursorScreenPos();
        ImVec2 clear_item_size = ImVec2(popup_size.x - 12, 20);
        ImRect clear_item_bb(clear_item_pos, ImVec2(clear_item_pos.x + clear_item_size.x, clear_item_pos.y + clear_item_size.y));

        bool clear_hovered = ImGui::IsMouseHoveringRect(clear_item_bb.Min, clear_item_bb.Max);
        bool clear_clicked = clear_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        ImU32 clear_bg = clear_hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255);
        popup_dl->AddRectFilled(clear_item_bb.Min, clear_item_bb.Max, clear_bg);

        const char* clear_text = "Clear";
        ImVec2 clear_text_size = ImGui::CalcTextSize(clear_text);
        ImVec2 clear_text_pos = ImVec2(clear_item_bb.Min.x + (clear_item_size.x - clear_text_size.x) * 0.5f, clear_item_bb.Min.y + (clear_item_size.y - clear_text_size.y) * 0.5f);

        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                if (x == 0 && y == 0) continue;
                popup_dl->AddText(ImVec2(clear_text_pos.x + x, clear_text_pos.y + y), IM_COL32(0, 0, 0, 255), clear_text);
            }
        }
        
        popup_dl->AddText(clear_text_pos, IM_COL32(255, 255, 255, 255), clear_text);

        if (clear_clicked)
        {
            *key = 0;
            keybind_popup_open[key_id] = false;
        }

        ImGui::Dummy(clear_item_size);

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    return pressed;
}

static bool styled_button(const char* label, const ImVec2& size = ImVec2(0, 0))
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImVec2 button_size = size;
    if (button_size.x == 0) button_size.x = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
    if (button_size.y == 0) button_size.y = ImGui::GetFrameHeight();

    ImVec2 pos = window->DC.CursorPos;
    pos.x += 2;

    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, window->GetID(label)))
        return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);

    static std::unordered_map<std::string, float> button_tween_progress;
    std::string button_id = label;
    
    if (button_tween_progress.find(button_id) == button_tween_progress.end())
        button_tween_progress[button_id] = 0.0f;

    float& tween_progress = button_tween_progress[button_id];

    ImVec4 base_color = ImVec4(60.0f / 255.0f, 60.0f / 255.0f, 60.0f / 255.0f, 1.0f);
    ImVec4 target_color = menu::accent_color;

    float target_progress = 0.0f;
    if (held)
        target_progress = 1.0f;
    else if (hovered)
        target_progress = 0.5f;

    float tween_speed = 4.0f;
    tween_progress += (target_progress - tween_progress) * g.IO.DeltaTime * tween_speed;

    ImVec4 tweened_color = ImVec4(
        base_color.x + (target_color.x - base_color.x) * tween_progress,
        base_color.y + (target_color.y - base_color.y) * tween_progress,
        base_color.z + (target_color.z - base_color.z) * tween_progress,
        base_color.w + (target_color.w - base_color.w) * tween_progress
    );

    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 bg_col = IM_COL32(35, 35, 35, 255);

    dl->AddRect(ImVec2(pos.x - 1, pos.y - 1), ImVec2(pos.x + button_size.x + 1, pos.y + button_size.y + 1), ImGui::ColorConvertFloat4ToU32(tweened_color), style.FrameRounding);
    dl->AddRect(ImVec2(pos.x - 2, pos.y - 2), ImVec2(pos.x + button_size.x + 2, pos.y + button_size.y + 2), IM_COL32(0, 0, 0, 255), style.FrameRounding);

    dl->AddRectFilled(bb.Min, bb.Max, bg_col, style.FrameRounding);

    ImVec2 text_size = ImGui::CalcTextSize(label);
    ImVec2 text_pos = ImVec2(bb.Min.x + (button_size.x - text_size.x) * 0.5f, bb.Min.y + (button_size.y - text_size.y) * 0.5f);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            if (x == 0 && y == 0) continue;
            dl->AddText(ImVec2(text_pos.x + x * 1.f, text_pos.y + y * 1.f), IM_COL32_BLACK, label);
        }
    }
    dl->AddText(text_pos, IM_COL32_WHITE, label);

    return pressed;
}

static void multiselect_combo(const char* label, bool* fov_check, bool* knocked_check)
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    ImGuiStyle& style = g.Style;
    
    std::string popup_id = std::string("##multiselect_") + label;
    std::string key_id = std::string(label);
    
    if (multiselect_popup_open.find(key_id) == multiselect_popup_open.end())
        multiselect_popup_open[key_id] = false;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, 20.f);
    ImRect bb(pos, ImVec2(pos.x + button_size.x, pos.y + button_size.y));
    
    bool hovered = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    bool clicked = hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    
    if (clicked)
    {
        multiselect_popup_open[key_id] = !multiselect_popup_open[key_id];
        if (multiselect_popup_open[key_id])
        {
            multiselect_popup_pos[key_id] = ImVec2(bb.Min.x, bb.Max.y + 2);
        }
    }
    
    ImU32 col = hovered ? IM_COL32(45, 45, 45, 255) : IM_COL32(35, 35, 35, 255);
    
    std::string display_text = "check";
    std::vector<std::string> selected_items;
    if (*fov_check) selected_items.push_back("fov check");
    if (*knocked_check) selected_items.push_back("knocked check");
    
    if (!selected_items.empty())
    {
        display_text = selected_items[0];
        for (size_t i = 1; i < selected_items.size(); i++)
        {
            display_text += ", " + selected_items[i];
        }
    }
    
    ImVec2 label_size = ImGui::CalcTextSize(display_text.c_str());
    if (label_size.x > button_size.x - 20)
    {
        display_text = std::to_string(selected_items.size()) + " selected";
        label_size = ImGui::CalcTextSize(display_text.c_str());
    }
    
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(bb.Min, bb.Max, col, style.FrameRounding);
    
    ImVec2 text_pos = ImVec2(bb.Min.x + 8, bb.Min.y + (button_size.y - label_size.y) * 0.5f);
    dl->AddText(text_pos, IM_COL32(255, 255, 255, 255), display_text.c_str());
    
    ImVec2 arrow_pos = ImVec2(bb.Max.x - 15, bb.Min.y + (button_size.y - 8) * 0.5f);
    dl->AddTriangleFilled(
        arrow_pos,
        ImVec2(arrow_pos.x + 8, arrow_pos.y),
        ImVec2(arrow_pos.x + 4, arrow_pos.y + 8),
        IM_COL32(255, 255, 255, 255)
    );
    
    ImGui::Dummy(button_size);
    
    if (multiselect_popup_open[key_id])
    {
        ImGui::SetNextWindowPos(multiselect_popup_pos[key_id], ImGuiCond_Always);
        ImVec2 popup_size = ImVec2(button_size.x, 50);
        ImGui::SetNextWindowSize(popup_size);
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6, 6));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(25, 25, 25, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertFloat4ToU32(menu::accent_color));
        
        ImGui::Begin(popup_id.c_str(), nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize);
        
        ImGui::Checkbox("FOV Check", fov_check);
        ImGui::Checkbox("Knocked Check", knocked_check);
        
        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }
    
    if (multiselect_popup_open[key_id] && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        ImVec2 mouse_pos = ImGui::GetMousePos();
        ImVec2 popup_min = multiselect_popup_pos[key_id];
        ImVec2 popup_max = ImVec2(popup_min.x + button_size.x, popup_min.y + 50);
        
        if (!(mouse_pos.x >= popup_min.x && mouse_pos.x <= popup_max.x &&
              mouse_pos.y >= popup_min.y && mouse_pos.y <= popup_max.y) &&
            !(mouse_pos.x >= bb.Min.x && mouse_pos.x <= bb.Max.x &&
              mouse_pos.y >= bb.Min.y && mouse_pos.y <= bb.Max.y))
        {
            multiselect_popup_open[key_id] = false;
        }
    }
}

LRESULT CALLBACK wnd_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
    {
        return true;
    }

    switch (msg)
    {
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
        {
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (wParam == VK_F4) {
            DestroyWindow(hwnd);
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

render_t::render_t()
{
    detail = std::make_unique<detail_t>();
}

render_t::~render_t()
{
	if (initialized) {
		destroy_imgui();
		destroy_window();
		destroy_device();
	}
}

bool render_t::create_window()
{
    detail->window_class.cbSize = sizeof(detail->window_class);
    detail->window_class.style = CS_CLASSDC;
    detail->window_class.lpszClassName = "T4";
    detail->window_class.hInstance = GetModuleHandleA(0);
    detail->window_class.lpfnWndProc = wnd_proc;

    RegisterClassExA(&detail->window_class);

    detail->window = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        detail->window_class.lpszClassName,
        "T4",
        WS_POPUP,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        0,
        0,
        detail->window_class.hInstance,
        0
    );

    if (!detail->window)
    {
        return false;
    }

    SetLayeredWindowAttributes(detail->window, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);

    RECT client_area{};
    RECT window_area{};

    GetClientRect(detail->window, &client_area);
    GetWindowRect(detail->window, &window_area);

    POINT diff{};
    ClientToScreen(detail->window, &diff);

    MARGINS margins
    {
        window_area.left + (diff.x - window_area.left),
        window_area.top + (diff.y - window_area.top),
        window_area.right,
        window_area.bottom,
    };

    DwmExtendFrameIntoClientArea(detail->window, &margins);

    ShowWindow(detail->window, SW_SHOW);
    UpdateWindow(detail->window);

    return true;
}

bool render_t::create_device()
{
    DXGI_SWAP_CHAIN_DESC swap_chain_desc{};

    swap_chain_desc.BufferCount = 1;

    swap_chain_desc.BufferDesc.Width = 0;
    swap_chain_desc.BufferDesc.Height = 0;
    swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    swap_chain_desc.OutputWindow = detail->window;

    swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swap_chain_desc.Windowed = 1;

    swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    swap_chain_desc.SampleDesc.Count = 2;
    swap_chain_desc.SampleDesc.Quality = 0;

    swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

    D3D_FEATURE_LEVEL feature_level;
    D3D_FEATURE_LEVEL feature_level_list[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };

    HRESULT result = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        feature_level_list,
        2,
        D3D11_SDK_VERSION,
        &swap_chain_desc,
        &detail->swap_chain,
        &detail->device,
        &feature_level,
        &detail->device_context
    );

    if (result == DXGI_ERROR_UNSUPPORTED)
    {
        result = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            feature_level_list,
            2,
            D3D11_SDK_VERSION,
            &swap_chain_desc,
            &detail->swap_chain,
            &detail->device,
            &feature_level,
            &detail->device_context
        );
    }

    if (result != S_OK)
    {
        MessageBoxA(nullptr, "This software can not run on your computer.", "Critical Problem", MB_ICONERROR | MB_OK);
    }

    ID3D11Texture2D* back_buffer{ nullptr };
    detail->swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));

    if (back_buffer)
    {
        detail->device->CreateRenderTargetView(back_buffer, nullptr, &detail->render_target_view);
        back_buffer->Release();

        return true;
    }

    return false;
}

bool render_t::create_imgui()
{
    using namespace ImGui;
    CreateContext();
    StyleColorsDark();

    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    style.ScaleAllSizes(main_scale);
    style.FontScaleDpi = main_scale;

    // Modern glassmorphism/dark theme properties
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.09f, 0.96f);
    colors[ImGuiCol_Border] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_Tab] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.19f, 0.19f, 0.21f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.15f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.11f, 0.12f, 1.00f);


	io.Fonts->AddFontDefault();
	
	Visualize.visitor = io.Fonts->AddFontFromMemoryTTF((void*)rawData, sizeof(rawData), 9.0f);

	ImFontConfig weaponIconConfig;
	weaponIconConfig.OversampleH = 3;
	weaponIconConfig.OversampleV = 3;
	weaponIconConfig.FontDataOwnedByAtlas = false;
	Visualize.weapon_icon_font = io.Fonts->AddFontFromMemoryTTF((void*)cs_icon, sizeof(cs_icon), 12.0f, &weaponIconConfig);

	if (!ImGui_ImplWin32_Init(detail->window))
    {
        return false;
    }

    if (!detail->device || !detail->device_context)
    {
        return false;
    }

    if (!ImGui_ImplDX11_Init(detail->device, detail->device_context))
    {
        return false;
    }

    return true;
}

void render_t::destroy_device()
{
    if (detail->render_target_view) detail->render_target_view->Release();
    if (detail->swap_chain) detail->swap_chain->Release();
    if (detail->device_context) detail->device_context->Release();
    if (detail->device) detail->device->Release();
}

void render_t::destroy_window()
{
    DestroyWindow(detail->window);
    UnregisterClassA(detail->window_class.lpszClassName, detail->window_class.hInstance);
}

void render_t::destroy_imgui()
{
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void render_t::start_render()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    static HMODULE user32 = GetModuleHandleA("user32.dll");
    static SetWindowDisplayAffinityProc SetWindowDisplayAffinity = nullptr;
    if (!SetWindowDisplayAffinity && user32)
    {
        SetWindowDisplayAffinity = (SetWindowDisplayAffinityProc)GetProcAddress(user32, "SetWindowDisplayAffinity");
    }

    if (SetWindowDisplayAffinity && detail->window)
    {
        if (menu::streamproof)
        {
            SetWindowDisplayAffinity(detail->window, WDA_EXCLUDEFROMCAPTURE);
        }
        else
        {
            SetWindowDisplayAffinity(detail->window, WDA_NONE);
        }
    }

    static HWND console_window = GetConsoleWindow();
    if (console_window)
    {
        if (menu::hide_console)
        {
            ShowWindow(console_window, SW_HIDE);
        }
        else
        {
            ShowWindow(console_window, SW_SHOW);
        }
    }

    // Check if Roblox is the foreground window
    static HWND roblox_window = nullptr;
    static bool last_visibility_state = true;
    HWND foreground_window = GetForegroundWindow();
    
    if (!roblox_window || !IsWindow(roblox_window))
    {
        roblox_window = FindWindowA(nullptr, "Roblox");
    }
    
    bool roblox_is_focused = false;
    bool overlay_is_focused = false;
    
    if (foreground_window && detail->window)
    {
        // Check if the overlay window itself is focused (user clicked on menu)
        overlay_is_focused = (foreground_window == detail->window || IsChild(detail->window, foreground_window));
        
        // Check if the foreground window is Roblox or a child of Roblox
        if (roblox_window)
        {
            roblox_is_focused = (foreground_window == roblox_window || IsChild(roblox_window, foreground_window));
        }
    }
    
    // Show overlay if Roblox is focused OR if the overlay itself is focused
    bool should_be_visible = roblox_is_focused || overlay_is_focused;
    
    // Only update visibility if state changed to prevent flickering
    if (should_be_visible != last_visibility_state && detail->window)
    {
        if (should_be_visible)
        {
            ShowWindow(detail->window, SW_SHOW);
        }
        else
        {
            ShowWindow(detail->window, SW_HIDE);
        }
        last_visibility_state = should_be_visible;
    }

    if (GetAsyncKeyState(menu::menu_keybind) & 1)
    {
        if (!check::textchatopen)
        {
            running = !running;

            if (running)
            {
                SetWindowLong(detail->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_LAYERED);
            }
            else
            {
                SetWindowLong(detail->window, GWL_EXSTYLE, WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED);
            }
        }
    }
}

void render_t::end_render()
{
    ImGui::Render();

    float clear_color[4]{ 0, 0, 0, 0 };
    detail->device_context->OMSetRenderTargets(1, &detail->render_target_view, nullptr);
    detail->device_context->ClearRenderTargetView(detail->render_target_view, clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    detail->swap_chain->Present(0, 0);
}

static void draw_custom_cursor()
{
	if (!g_silent_aim_instance.address)
	{
		return;
	}

	bool is_visible = false;
	is_visible = memory->read<bool>(g_silent_aim_instance.address + Offsets::GuiObject::Visible);

	if (!is_visible)
	{
		return;
	}

	POINT pt;
	if (!GetCursorPos(&pt))
	{
		return;
	}

	bool right_click_held = GetAsyncKeyState(VK_RBUTTON) & 0x8000;
	float gap = right_click_held ? 4.0f : 10.0f;
	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	ImU32 col = IM_COL32(255, 255, 255, 255);
	float dot_size = 4.0f;
	float line_width = 2.0f;
	float line_length = 10.0f;
	ImVec2 center = { (float)pt.x, (float)pt.y };
	ImVec2 dot_min(center.x - dot_size * 0.5f, center.y - dot_size * 0.5f);
	ImVec2 dot_max(center.x + dot_size * 0.5f, center.y + dot_size * 0.5f);
	draw->AddRectFilled(dot_min, dot_max, col, 0.0f);
	ImVec2 top_min(center.x - line_width * 0.5f, center.y - gap - line_length);
	ImVec2 top_max(center.x + line_width * 0.5f, center.y - gap);
	draw->AddRectFilled(top_min, top_max, col, 0.0f);
	ImVec2 bottom_min(center.x - line_width * 0.5f, center.y + gap);
	ImVec2 bottom_max(center.x + line_width * 0.5f, center.y + gap + line_length);
	draw->AddRectFilled(bottom_min, bottom_max, col, 0.0f);
	ImVec2 left_min(center.x - gap - line_length, center.y - line_width * 0.5f);
	ImVec2 left_max(center.x - gap, center.y + line_width * 0.5f);
	draw->AddRectFilled(left_min, left_max, col, 0.0f);
	ImVec2 right_min(center.x + gap, center.y - line_width * 0.5f);
	ImVec2 right_max(center.x + gap + line_length, center.y + line_width * 0.5f);
	draw->AddRectFilled(right_min, right_max, col, 0.0f);
}

void render_t::render_menu()
{
        // Turbo-Close Animation Logic
    static float open_progress = 0.0f;
    float target_progress = running ? 1.0f : 0.0f;
    
    if (menu::disable_animations) {
        open_progress = target_progress;
    } else {
        // High-speed synchronized animations for snappy response
        float anim_speed = 12.0f;
        open_progress = ImGui::GetIO().DeltaTime * anim_speed * (target_progress - open_progress) + open_progress;
    }
    
    if (open_progress < 0.001f)
        return;

    static int current_page = 0;
    static int target_page = 0;
    static float transition_alpha = 1.0f;
    static bool transitioning = false;
    
    // Page Transition Animation State Machine
    if (current_page != target_page) {
        transitioning = true;
        transition_alpha -= ImGui::GetIO().DeltaTime * 10.0f; // Fade out
        if (transition_alpha <= 0.0f) {
            current_page = target_page;
            transition_alpha = 0.0f;
        }
    } else if (transition_alpha < 1.0f) {
        transition_alpha += ImGui::GetIO().DeltaTime * 10.0f; // Fade in
        if (transition_alpha >= 1.0f) {
            transition_alpha = 1.0f;
            transitioning = false;
        }
    }

    // Window Setup with Scaling and Sliding
    ImVec2 base_size(700, 450);
    float scale = 0.95f + (open_progress * 0.05f); 
    float slide_up = (1.0f - open_progress) * 20.0f; 
    
    ImGui::SetNextWindowSize(ImVec2(base_size.x * scale, base_size.y * scale));
    ImGui::SetNextWindowBgAlpha(open_progress * 0.95f);
    
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowPos(ImVec2((display_size.x - base_size.x) * 0.5f, (display_size.y - base_size.y) * 0.5f + slide_up), ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, menu::rounding); 
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, open_progress); 
    ImGui::PushStyleColor(ImGuiCol_WindowBg, menu::bg_color);

    ImGui::Begin("Vertex", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);

    ImVec2 p = ImGui::GetWindowPos();
    ImVec2 s = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Drop Shadow
    for (int i = 1; i <= 10; i++)
        dl->AddRect(ImVec2(p.x - i, p.y - i), ImVec2(p.x + s.x + i, p.y + s.y + i), IM_COL32(0, 0, 0, (int)(open_progress * (20 - i * 2))), menu::rounding + i);

    // FiveM Style Header
    dl->AddRectFilled(p, ImVec2(p.x + s.x, p.y + 60), ImGui::ColorConvertFloat4ToU32(menu::header_color));
    dl->AddLine(ImVec2(p.x, p.y + 60), ImVec2(p.x + s.x, p.y + 60), IM_COL32(45, 45, 48, 255));
    
    ImGui::SetCursorPos(ImVec2(25, 18));
    ImGui::PushFont(Visualize.visitor);
    ImGui::TextColored(menu::accent_color, "V E R T E X");
    ImGui::PopFont();
    
    ImGui::SameLine();
    ImGui::SetCursorPosX(105); // Position after title
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::Text("v%s", menu::version);
    ImGui::PopStyleColor();
    
    ImGui::SameLine();
    ImGui::SetCursorPosX(140);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    const char* pages[] = { "COMBAT", "VISUALS", "EXPLOITS", "MISC", "SETTINGS", "CONFIG", "INFO", "EXECUTOR [WIP]" };
    ImGui::Text("> %s", pages[target_page]);
    ImGui::PopStyleColor();

    // Sidebar
    dl->AddRectFilled(ImVec2(p.x, p.y + 60), ImVec2(p.x + 180, p.y + s.y), IM_COL32(15, 15, 17, 255));
    dl->AddLine(ImVec2(p.x + 180, p.y + 60), ImVec2(p.x + 180, p.y + s.y), IM_COL32(45, 45, 48, 255));

    auto add_list_item = [&](const char* label, int index) {
        ImGui::SetCursorPosX(10);
        ImGui::SetCursorPosY(70 + (index * 42)); // Reduced gap for 8 items
        
        bool active = (target_page == index);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        
        bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + 160, pos.y + 36));
        float hover_alpha = hovered ? 0.25f : (active ? 0.15f : 0.0f);
        
        if (hover_alpha > 0.0f)
            dl->AddRectFilled(pos, ImVec2(pos.x + 160, pos.y + 36), IM_COL32(255, 255, 255, (int)(hover_alpha * 255 * open_progress)), 6.0f);
            
        if (active)
            dl->AddRectFilled(ImVec2(pos.x, pos.y + 4), ImVec2(pos.x + 4, pos.y + 32), ImGui::ColorConvertFloat4ToU32(menu::accent_color), 2.0f);

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 0));
        
        if (ImGui::Button(std::string("##item_" + std::to_string(index)).c_str(), ImVec2(160, 36))) {
            target_page = index;
        }
        ImGui::PopStyleColor(3);

        dl->AddText(ImVec2(pos.x + 20, pos.y + 10), active ? IM_COL32_WHITE : IM_COL32(180, 180, 180, 255), label);
    };

    for (int i = 0; i < 8; i++) {
        add_list_item(pages[i], i);
    }

    // Sidebar Footer (Game Detection)
    ImGui::SetCursorPos(ImVec2(15, s.y - 45));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::Text("CURRENT SESSION");
    ImGui::PopStyleColor();
    
    ImGui::SetCursorPos(ImVec2(15, s.y - 30));
    ImGui::PushFont(Visualize.visitor);
    ImGui::TextColored(menu::accent_color, "%s", game::game_name.c_str());
    ImGui::PopFont();

    // Main Content Area
    float content_slide_y = (1.0f - transition_alpha) * 10.0f; 
    ImGui::SetCursorPos(ImVec2(200, 80 + content_slide_y));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, transition_alpha * open_progress);
    ImGui::BeginGroup();
    {
        float content_w = s.x - 220;
        float content_h = s.y - 100;

        switch (current_page)
        {
        case 0: // COMBAT
        {
            ImGui::BeginChild("CombatContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "AIM ASSISTANCE");
                ImGui::Separator();
                ImGui::Checkbox("Master Aimbot", &settings::aimbot::enabled);
                if (settings::aimbot::enabled) {
                    ImGui::SameLine();
                    inline_keybind_button("aim_key", &settings::aimbot::keybind, &settings::aimbot::keybind_mode);
                    
                    static bool advanced_smoothing = false;
                    if (!advanced_smoothing) {
                        if (ImGui::SliderFloat("Smoothing", &settings::aimbot::camera_smooth_x, 1.0f, 20.0f)) {
                            settings::aimbot::camera_smooth_y = settings::aimbot::camera_smooth_x;
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Advance")) {
                            advanced_smoothing = true;
                        }
                    } else {
                        ImGui::SliderFloat("Smoothing X", &settings::aimbot::camera_smooth_x, 1.0f, 20.0f);
                        ImGui::SameLine();
                        if (ImGui::Button("Basic")) {
                            advanced_smoothing = false;
                            settings::aimbot::camera_smooth_y = settings::aimbot::camera_smooth_x;
                        }
                        ImGui::SliderFloat("Smoothing Y", &settings::aimbot::camera_smooth_y, 1.0f, 20.0f);
                    }
                    
                    ImGui::SliderFloat("FOV", &settings::aimbot::fov, 10.0f, 500.0f);
                }
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "SILENT AIM");
                ImGui::Separator();
                ImGui::Checkbox("Enable Silent", &settings::silent::enabled);
                if (settings::silent::enabled) {
                    ImGui::Checkbox("Prediction", &settings::silent::prediction);
                    ImGui::SliderFloat("Silent FOV", &settings::silent::fov, 10.0f, 800.0f);
                }

                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "TRIGGERBOT");
                ImGui::Separator();
                ImGui::Checkbox("Enable Triggerbot", &settings::expl::triggerbot);
                if (settings::expl::triggerbot) {
                    ImGui::SameLine();
                    inline_keybind_button("trigger_key", &settings::expl::triggerbot_keybind, &settings::expl::triggerbot_keybind_mode);
                    ImGui::SliderInt("Click Delay (ms)", &settings::expl::triggerbot_delay, 0, 500);
                }

                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "FILTERS");
                ImGui::Separator();
                ImGui::Checkbox("Wall Check", &settings::aimbot::wall_check);
                ImGui::Checkbox("Team Check", &settings::aimbot::team_check);
            }
            ImGui::EndChild();
            break;
        }
        case 1: // VISUALS
        {
            ImGui::BeginChild("VisualsContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "ENVIRONMENT & PLAYERS");
                ImGui::Separator();
                ImGui::Checkbox("Box ESP", &settings::visuals::box);
                ImGui::Checkbox("Skeleton ESP", &settings::visuals::skeleton);
                ImGui::Checkbox("Health Info", &settings::visuals::healthbar);
                ImGui::Checkbox("Name Tags", &settings::visuals::name);
                ImGui::Checkbox("Weapon/Tool Info", &settings::visuals::tool);
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "VISIBILITY");
                ImGui::Separator();
                ImGui::Checkbox("Visible Check", &settings::visuals::visible_check);
                if (settings::visuals::visible_check) {
                    ImGui::Text("Visible Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("##vis_col", (float*)&settings::visuals::visible_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                    
                    ImGui::Text("Invisible Color");
                    ImGui::SameLine();
                    ImGui::ColorEdit4("##invis_col", (float*)&settings::visuals::invisible_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                }
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "OVERLAY");
                ImGui::Separator();
                ImGui::Checkbox("Watermark", &menu::watermark);
                ImGui::Checkbox("Status Indicator", &settings::visuals::feature_indicator);
            }
            ImGui::EndChild();
            break;
        }
        case 2: // EXPLOITS
        {
            ImGui::BeginChild("ExploitsContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "BLATANT MODIFICATIONS");
                ImGui::Separator();
                ImGui::Checkbox("Spinbot", &settings::expl::spinbot);
                if (settings::expl::spinbot) ImGui::SliderFloat("Speed", &settings::expl::spinbot_speed, 1.0f, 100.0f);
                
                ImGui::Checkbox("Hitbox Expander", &settings::hitbox_expander::enabled);
                if (settings::hitbox_expander::enabled) ImGui::SliderFloat("Radius", &settings::hitbox_expander::size_x, 1.0f, 20.0f);
                
                ImGui::Checkbox("Fling", &settings::expl::fling_enabled);
                if (settings::expl::fling_enabled) ImGui::SliderFloat("Fling Power", &settings::expl::fling_power, 100.0f, 20000.0f);

                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "PHYSICS");
                ImGui::Separator();
                ImGui::Checkbox("Infinite Jump", &settings::expl::infinite_jump);
                ImGui::Checkbox("Gravity Control", &settings::expl::gravity_enabled);
                if (settings::expl::gravity_enabled) ImGui::SliderFloat("Amount", &settings::expl::gravity_amount, 0.0f, 196.0f);
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "FREEZE PLAYER");
                ImGui::Separator();
                ImGui::Checkbox("Freeze Enabled", &settings::expl::freeze_player);
                if (settings::expl::freeze_player) {
                    ImGui::SameLine();
                    inline_keybind_button("freeze_key", &settings::expl::freeze_player_keybind, &settings::expl::freeze_player_keybind_mode);
                    
                    const char* freeze_modes[] = { "Anchor", "Velocity Lock", "Position Lock" };
                    ImGui::Combo("Mode", &settings::expl::freeze_player_mode, freeze_modes, IM_ARRAYSIZE(freeze_modes));
                }
            }
            ImGui::EndChild();
            break;
        }
        case 3: // MISC
        {
            ImGui::BeginChild("MiscContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "MOVEMENT & CAMERA");
                ImGui::Separator();
                ImGui::Checkbox("Walkspeed", &settings::expl::walkspeed);
                if (settings::expl::walkspeed) ImGui::SliderFloat("Speed", &settings::expl::walkspeed_speed, 16.0f, 500.0f);

                ImGui::Checkbox("Jump Power", &settings::expl::jump_height_enabled);
                if (settings::expl::jump_height_enabled) ImGui::SliderFloat("Jump Height", &settings::expl::jump_height, 0.0f, 500.0f);
                
                ImGui::Checkbox("Fly", &settings::expl::fly_enabled);
                if (settings::expl::fly_enabled) ImGui::SliderFloat("Fly Speed", &settings::expl::fly_speed, 10.0f, 500.0f);
                
                ImGui::Checkbox("Noclip", &settings::expl::noclip_enabled);
                
                ImGui::Checkbox("Field of View", &settings::expl::fov_changer);
                if (settings::expl::fov_changer) ImGui::SliderFloat("FOV Amount", &settings::expl::fov_value, 10.0f, 120.0f);
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "UTILITIES");
                ImGui::Separator();
                ImGui::Checkbox("Free Cam", &settings::expl::free_cam);
                if (settings::expl::free_cam) ImGui::SliderFloat("Cam Sensitivity", &settings::expl::free_cam_speed, 0.1f, 5.0f);
                ImGui::Checkbox("Anti-AFK", &settings::expl::anti_afk);

                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "TELEPORTATION");
                ImGui::Separator();
                ImGui::Checkbox("Teleport To Nearest", &settings::expl::teleport);
                if (settings::expl::teleport) {
                    ImGui::SameLine();
                    inline_keybind_button("tp_key", &settings::expl::teleport_keybind, &settings::expl::teleport_keybind_mode);
                }

                ImGui::Checkbox("Click TP", &settings::expl::click_tp);
                if (settings::expl::click_tp) {
                    ImGui::SameLine();
                    inline_keybind_button("click_tp_key", &settings::expl::click_tp_keybind, &settings::expl::click_tp_keybind_mode);
                }

                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "PLAYER TELEPORT");
                ImGui::Separator();
                
                static char search_filter[64] = "";
                ImGui::SetNextItemWidth(-1);
                ImGui::InputTextWithHint("##Search", "Search Player...", search_filter, 64);
                
                if (ImGui::BeginChild("PlayerList", ImVec2(0, 150), true)) {
                    auto players = game::players.get_children();
                    for (auto& player_inst : players) {
                        rbx::player_t player(player_inst.address);
                        std::string name = player.get_name();
                        
                        if (search_filter[0] != '\0' && name.find(search_filter) == std::string::npos)
                            continue;

                        bool selected = (settings::expl::selected_player_name == name);
                        if (ImGui::Selectable(name.c_str(), selected)) {
                            settings::expl::selected_player_name = name;
                        }
                    }
                }
                ImGui::EndChild();
                
                if (!settings::expl::selected_player_name.empty()) {
                    if (ImGui::Button("Teleport to Selected", ImVec2(-1, 30))) {
                        auto players = game::players.get_children();
                        for (auto& player_inst : players) {
                            rbx::player_t player(player_inst.address);
                            if (player.get_name() == settings::expl::selected_player_name) {
                                rbx::model_instance_t model = player.get_model_instance();
                                if (model.address != 0) {
                                    rbx::instance_t hrp = model.find_first_child("HumanoidRootPart");
                                    if (hrp.address != 0) {
                                        math::vector3 pos = rbx::part_t(hrp.address).get_primitive().get_position();
                                        pos.y += 3.0f;
                                        
                                        rbx::player_t lp(game::local_player.address);
                                        rbx::model_instance_t lp_model = lp.get_model_instance();
                                        if (lp_model.address != 0) {
                                            rbx::instance_t lp_hrp = lp_model.find_first_child("HumanoidRootPart");
                                            if (lp_hrp.address != 0) {
                                                memory->write<math::vector3>(rbx::part_t(lp_hrp.address).get_primitive().address + Offsets::Primitive::Position, pos);
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }

                    if (ImGui::Button("Target Fling", ImVec2(-1, 30))) {
                        auto players = game::players.get_children();
                        for (auto& player_inst : players) {
                            rbx::player_t player(player_inst.address);
                            if (player.get_name() == settings::expl::selected_player_name) {
                                rbx::model_instance_t model = player.get_model_instance();
                                if (model.address != 0) {
                                    rbx::instance_t hrp = model.find_first_child("HumanoidRootPart");
                                    if (hrp.address != 0) {
                                        rbx::player_t lp(game::local_player.address);
                                        rbx::model_instance_t lp_model = lp.get_model_instance();
                                        if (lp_model.address != 0) {
                                            rbx::instance_t lp_hrp = lp_model.find_first_child("HumanoidRootPart");
                                            if (lp_hrp.address != 0) {
                                                rbx::primitive_t lp_prim = rbx::part_t(lp_hrp.address).get_primitive();
                                                math::vector3 original_pos = lp_prim.get_position();
                                                math::vector3 target_pos = rbx::part_t(hrp.address).get_primitive().get_position();
                                                
                                                float p = settings::expl::fling_power;
                                                math::vector3 v(p * 2.0f, p * 2.0f, p * 2.0f);
                                                
                                                // Ghost Fling sequence
                                                memory->write<math::vector3>(lp_prim.address + Offsets::Primitive::Position, target_pos);
                                                memory->write<math::vector3>(lp_prim.address + Offsets::Primitive::AssemblyLinearVelocity, v);
                                                memory->write<math::vector3>(lp_prim.address + Offsets::Primitive::Position, original_pos);
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                    }
                }
            }
            ImGui::EndChild();
            break;
        }
        case 4: // SETTINGS
        {
            ImGui::BeginChild("SettingsContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "INTERFACE CUSTOMIZATION");
                ImGui::Separator();
                ImGui::Checkbox("Overlay Watermark", &menu::watermark);
                ImGui::Checkbox("Streamproof (Anti-OBS)", &menu::streamproof);
                ImGui::Checkbox("Stealth Console (Hide)", &menu::hide_console);
                ImGui::Checkbox("Disable UI Animations", &menu::disable_animations);
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "GLOBAL THEME");
                ImGui::Separator();
                ImGui::Text("Menu Accent Color");
                ImGui::SameLine();
                ImGui::ColorEdit4("##accent", (float*)&menu::accent_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                
                ImGui::Text("Background Color");
                ImGui::SameLine();
                ImGui::ColorEdit4("##bg", (float*)&menu::bg_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                
                ImGui::Text("Header Color");
                ImGui::SameLine();
                ImGui::ColorEdit4("##header", (float*)&menu::header_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
                
                ImGui::SliderFloat("Menu Rounding", &menu::rounding, 0.0f, 30.0f);
            }
            ImGui::EndChild();
            break;
        }
        case 5: // CONFIG
        {
            ImGui::BeginChild("ConfigContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "PROFILE MANAGEMENT");
                ImGui::Separator();
                static char cfg_name[64] = "";
                ImGui::InputText("Configuration Name", cfg_name, 64);
                if (styled_button("SAVE PROFILE", ImVec2(content_w - 20, 35))) config::save_config(cfg_name);
                
                ImGui::Spacing();
                ImGui::Text("AVAILABLE PROFILES");
                auto configs = config::get_config_files();
                for (const auto& cfg : configs) {
                    if (ImGui::Selectable(cfg.name.c_str())) config::load_config(cfg.name);
                }
            }
            ImGui::EndChild();
            break;
        }
        case 6: // INFO
        {
            ImGui::BeginChild("InfoContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "VERTEX PREMIUM EXTERNAL");
                ImGui::Separator();
                ImGui::Text("Version: %s", menu::version);
                ImGui::Text("Status: Undetected / Safe");
                ImGui::Text("User: Premium License");
                
                ImGui::Spacing();
                ImGui::TextColored(menu::accent_color, "CONTRIBUTORS");
                ImGui::Separator();
                ImGui::Text("- tormix (Engine & Bypass)");
                
                ImGui::SetCursorPosY(content_h - 45);
                if (styled_button("EMERGENCY SHUTDOWN", ImVec2(content_w - 20, 35))) ExitProcess(0);
            }
            ImGui::EndChild();
            break;
        }
        case 7: // EXECUTOR (Dynamic Content)
        {
            ImGui::BeginChild("ExecutorContent", ImVec2(content_w, content_h), false);
            {
                ImGui::TextColored(menu::accent_color, "LUA EXECUTOR");
                ImGui::Separator();
                
                static char script_buffer[8192] = "print('Hello from Vertex Executor!')\n-- Paste your script here\n";
                ImGui::InputTextMultiline("##source", script_buffer, IM_ARRAYSIZE(script_buffer), ImVec2(-FLT_MIN, content_h - 100), ImGuiInputTextFlags_AllowTabInput);
                
                ImGui::Spacing();
                
                if (ImGui::Button("Execute", ImVec2(120, 30))) {
                    // Send to pipe
                    HANDLE hPipe;
                    hPipe = CreateFileA("\\\\.\\pipe\\VertexExecutor", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
                    if (hPipe != INVALID_HANDLE_VALUE) {
                        DWORD bytesWritten;
                        WriteFile(hPipe, script_buffer, strlen(script_buffer) + 1, &bytesWritten, NULL);
                        CloseHandle(hPipe);
                    }
                }
                
                ImGui::SameLine();
                if (ImGui::Button("Clear", ImVec2(120, 30))) {
                    memset(script_buffer, 0, sizeof(script_buffer));
                }

                ImGui::SameLine();
                if (ImGui::Button("Inject Engine", ImVec2(120, 30))) {
                    // Placeholder for injector
                    MessageBoxA(NULL, "Injector placeholder. Will attach to vertex_internal.dll", "Vertex", MB_OK);
                }
            }
            ImGui::EndChild();
            break;
        }
        }
    }
    ImGui::EndGroup();
    ImGui::PopStyleVar();

    ImGui::End();

    dex_explorer::DexExplorer::render();
}

void render_t::render_visuals()
{
    if (menu::watermark)
    {
        ImDrawList* draw = ImGui::GetForegroundDrawList();
        draw->Flags &= ~ImDrawListFlags_AntiAliasedLines;

        ImVec2 size = ImVec2(120, 28);
        ImVec2 display_size = ImGui::GetIO().DisplaySize;

        if (menu::watermark_pos.x < 0)
            menu::watermark_pos.x = (display_size.x - size.x) * 0.5f;

        static bool dragging = false;
        static ImVec2 drag_offset;

        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        bool mouse_down = ImGui::GetIO().MouseDown[0];

        ImVec2 pos = menu::watermark_pos;

        bool hovered = mouse_pos.x >= pos.x && mouse_pos.x <= pos.x + size.x &&
                       mouse_pos.y >= pos.y && mouse_pos.y <= pos.y + size.y;

        if (hovered && mouse_down && !dragging)
        {
            dragging = true;
            drag_offset = ImVec2(mouse_pos.x - pos.x, mouse_pos.y - pos.y);
        }

        if (dragging)
        {
            if (mouse_down)
            {
                menu::watermark_pos.x = mouse_pos.x - drag_offset.x;
                menu::watermark_pos.y = mouse_pos.y - drag_offset.y;
                pos = menu::watermark_pos;
            }
            else
            {
                dragging = false;
            }
        }

        draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + size.x, pos.y + size.y), IM_COL32(20, 20, 20, 255));
        draw->AddRect(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + size.x - 1, pos.y + size.y - 1), IM_COL32(60, 60, 60, 255));
        draw->AddRect(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + size.x - 2, pos.y + size.y - 2), IM_COL32(0, 0, 0, 255));

        ImU32 accent = ImGui::ColorConvertFloat4ToU32(menu::accent_color);
        draw->AddRectFilled(ImVec2(pos.x + 2, pos.y + 2), ImVec2(pos.x + size.x - 2, pos.y + 4), accent);

        draw->AddRectFilled(ImVec2(pos.x + 5, pos.y + 7), ImVec2(pos.x + size.x - 5, pos.y + size.y - 5), IM_COL32(35, 35, 35, 255));
        draw->AddRect(ImVec2(pos.x + 5, pos.y + 7), ImVec2(pos.x + size.x - 5, pos.y + size.y - 5), IM_COL32(0, 0, 0, 255));
        draw->AddRect(ImVec2(pos.x + 6, pos.y + 8), ImVec2(pos.x + size.x - 6, pos.y + size.y - 6), IM_COL32(60, 60, 60, 255));

        ImVec2 text_size = ImGui::CalcTextSize("Vertex");
        ImVec2 text_pos = ImVec2(pos.x + (size.x - text_size.x) * 0.5f, pos.y + 7 + ((size.y - 12) - text_size.y) * 0.5f);
        
        const char* text = "Vertex";
        ImU32 outline_color = IM_COL32(0, 0, 0, 255);
        ImU32 text_color = ImGui::ColorConvertFloat4ToU32(menu::accent_color);
        
        draw->AddText(ImVec2(text_pos.x - 1, text_pos.y - 1), outline_color, text);
        draw->AddText(ImVec2(text_pos.x + 1, text_pos.y - 1), outline_color, text);
        draw->AddText(ImVec2(text_pos.x - 1, text_pos.y + 1), outline_color, text);
        draw->AddText(ImVec2(text_pos.x + 1, text_pos.y + 1), outline_color, text);
        draw->AddText(text_pos, text_color, text);
    }

    esp::run();
    
    render_feature_indicator();
    
    draw_custom_cursor();
}

void render_t::render_feature_indicator()
{
    if (!settings::visuals::feature_indicator) {
        return;
    }
    
    if (!Visualize.visitor) {
        return;
    }
    
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImVec2 display_size = ImGui::GetIO().DisplaySize;
    
    float start_x = settings::visuals::feature_indicator_x;
    float start_y = settings::visuals::feature_indicator_y;
    
    if (start_y == 0.0f) {
        start_y = display_size.y * 0.5f;
    }
    
    float font_size = 9.0f;
    
    std::vector<std::string> lines;
    
    bool silent_enabled = settings::silent::enabled;
    bool silent_active = silent_enabled && g_silent_aim_locked;
    
    lines.push_back("SILENT AIM: " + std::string(silent_active ? "ON" : "OFF"));
    
    std::string target_name = "NONE";
    if (silent_active && g_silent_cached_target.instance.address != 0 && !g_silent_cached_target.name.empty()) {
        target_name = g_silent_cached_target.name;
    }
    lines.push_back("SILENT AIM TARGET: " + target_name);
    
    float text_y = start_y;
    ImU32 text_color = IM_COL32(255, 255, 255, 255);
    
    for (const auto& line : lines) {
        ImVec2 text_size = Visualize.visitor->CalcTextSizeA(font_size, FLT_MAX, 0.0f, line.c_str());
        float text_x = start_x;
        
        Visualize.DrawTextWithSpacingAndOutline(draw, Visualize.visitor, font_size, ImVec2(text_x, text_y), text_color, IM_COL32(0, 0, 0, 255), line);
        
        text_y += text_size.y + 2.0f;
    }
}

void render_t::render_notifications()
{
    notifications::update();
    notifications::render();
}
