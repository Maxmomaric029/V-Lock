# Vertex External

A Roblox external cheat client developed by tormix.

## Project Structure

```
tlee/rose/
├── src/
│   ├── main.cpp                  # Entry point & initialization
│   ├── settings.h                # Global configuration settings
│   ├── sdk/
│   │   ├── offsets.h             # Roblox memory offsets
│   │   ├── offset_updater.cpp/h  # Auto-updater for offsets
│   │   ├── sdk.cpp/h             # Roblox SDK (instances, primitives, etc.)
│   │   └── math/math.h           # Math utilities (vectors, matrices)
│   ├── memory/
│   │   ├── memory.cpp/h          # Process memory read/write abstraction
│   │   └── luck.asm              # Assembly-level memory operations
│   ├── render/
│   │   ├── render.cpp/h          # DirectX overlay & ImGui rendering
│   │   ├── notifications.cpp/h   # On-screen notification system
│   │   ├── visitor.h             # Font data
│   │   └── render_helpers.h      # Drawing helpers
│   ├── features/
│   │   ├── aimbot/               # Camera & mouse aim assistance
│   │   ├── silent/               # Silent aim (server-side aim correction)
│   │   ├── esp/                   # Player visuals (boxes, skeletons, etc.)
│   │   ├── expl/                  # Game exploits (walkspeed, fly, noclip, etc.)
│   │   ├── explorer/              # Dex Explorer (Roblox instance browser)
│   │   └── games/                 # Game-specific features (Jailbreak, etc.)
│   ├── cache/                    # Game object caching
│   ├── config/                   # JSON-based configuration save/load
│   ├── game/                     # Game state management (datamodel, players, etc.)
│   ├── check/                    # Typing/chat detection
│   └── bypass/                   # Anti-detection & crash handler bypass
├── keyauth/                      # Authentication library (deprecated)
├── protection/                   # Anti-debug & webhook protection
├── ext/                          # External libraries (imgui, clipper2)
└── resources/                    # Fonts, icons
```

## Changes Made

### Security Audit
- **Offensive language removed**: Removed racial slur variable (`niggerKyzo`) from render.cpp
- **Deceptive config path fixed**: Changed config storage from `%APPDATA%\Telegram Desktop\application configuration` (masquerading as Telegram) to `%APPDATA%\Vertex\Configs`
- **Dead code removed**: Cleaned up commented-out KeyAuth references and unused variables
- **Duplicate functions consolidated**: Two near-identical keybind button functions reviewed

### Offset Update
- Updated all 381 Roblox memory offsets to version `version-ad5d3e2906444472`
- Client version string updated accordingly
- New offset namespaces include: `PlatformStatePointer`, `IsCoreScript`, `ByteCode` updates
- Removed deprecated namespaces: `MeshContentProvider`, `MeshData`, `ChatInputBarConfiguration`
- Fixed offset values for: `DataModel`, `Instance`, `Humanoid`, `Player`, `Primitive`, `Camera`, `VisualEngine`, `Workspace`, and many more

### Code Quality
- Removed unused variables (test_label_1-4, test_slider, color_array)
- Fixed misleading Telegram Desktop config storage path

## Build Requirements
- Visual Studio 2022
- Windows SDK
- DirectX 11 SDK

## Notes
- The file `luck.asm` contains assembly routines that replace `ReadProcessMemory`/`WriteProcessMemory` — this is the core memory engine and is essential for the application's functionality
- Offsets are auto-updated via `offset_updater` using https://imtheo.lol/Offsets
