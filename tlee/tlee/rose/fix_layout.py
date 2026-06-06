with open('src/render/render.cpp', 'r', encoding='utf-8') as f:
    content = f.read()
content = content.replace('ImGui::SetCursorPos(ImVec2(22.f, 78.f));', 'ImGui::SetCursorPos(ImVec2(15.f, 75.f));')
content = content.replace('ImGui::SetCursorPos(ImVec2(303.f, 78.f));', 'ImGui::SetCursorPos(ImVec2(window_width / 2 + 5.f, 75.f));')
content = content.replace('ImGui::GetContentRegionAvail().x / 2 - 9.f', 'window_width / 2 - 20.f')
content = content.replace('ImGui::GetContentRegionAvail().x - 13.f', 'window_width / 2 - 20.f')
with open('src/render/render.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
