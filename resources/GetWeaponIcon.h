#pragma once
#include <string>
#include <unordered_map>

static const std::unordered_map<std::string, const char*> robloxWeaponIcons = 
{
    // Rifles
    {"AK47", "W"},           // AK-47
    {"AR", "T"},             // M4A1 style
    {"SilencerAR", "T"},     // M4A1 silenced
    {"AUG", "U"},            // AUG
    
    // SMGs
    {"SMG", "K"},            // MAC-10 style
    {"P90", "O"},            // P90
    {"DrumGun", "M"},        // Bizon style (drum mag)
    
    // Shotguns
    {"Shotgun", "e"},        // Nova style
    {"TacticalShotgun", "b"}, // XM1014 style
    {"Double-Barrel SG", "c"}, // Sawed-off style
    {"Double-Barrel", "c"},   // Alternate name
    {"DoubleBarrel", "c"},    // No space/hyphen variant
    {"DB", "c"},              // Short name
    
    // Pistols
    {"Glock", "D"},          // Glock
    {"Silencer", "G"},       // USP style (silenced pistol)
    {"Revolver", "J"},       // Revolver
    
    // Heavy
    {"LMG", "f"},            // Negev style
    {"RPG", "g"},            // M249 style (closest we have)
    {"Flamethrower", "h"},   // Taser style (special weapon)
    
    // Knives/Melee
    {"Knife", "["},          // T knife
    {"Sword", "]"},          // CT knife
    
    // Grenades/Throwables
    {"Grenade", "j"},        // HE grenade
    {"Flashbang", "i"},      // Flashbang
    {"Smoke", "k"},          // Smoke grenade
    {"Molotov", "l"},        // Molotov
};

// Size and offset configuration for each weapon icon
struct WeaponIconSize
{
    float width;
    float height;
    float offsetX;
    float offsetY;
};

static std::unordered_map<std::string, WeaponIconSize> weaponIconSizes = 
{
    // Rifles - larger icons
    {"AK47", {16.0f, 10.0f, -3.0f, 0.0f}},
    {"AR", {16.0f, 10.0f, -3.0f, 0.0f}},
    {"SilencerAR", {18.0f, 10.0f, -4.0f, 0.0f}},
    {"AUG", {16.0f, 10.0f, -3.0f, 0.0f}},
    
    // SMGs - medium icons
    {"SMG", {14.0f, 10.0f, -2.0f, 0.0f}},
    {"P90", {14.0f, 10.0f, -2.0f, 0.0f}},
    {"DrumGun", {14.0f, 10.0f, -2.0f, 0.0f}},
    
    // Shotguns - medium icons
    {"Shotgun", {14.0f, 10.0f, 0.0f, 0.0f}},
    {"TacticalShotgun", {16.0f, 10.0f, 0.0f, 0.0f}},
    {"Double-Barrel SG", {14.0f, 10.0f, 0.0f, 0.0f}},
    {"Double-Barrel", {14.0f, 10.0f, 0.0f, 0.0f}},
    {"DoubleBarrel", {14.0f, 10.0f, 0.0f, 0.0f}},
    {"DB", {14.0f, 10.0f, 0.0f, 0.0f}},
    
    // Pistols - smaller icons
    {"Glock", {10.0f, 10.0f, -1.0f, 0.0f}},
    {"Silencer", {12.0f, 10.0f, -1.0f, 0.0f}},
    {"Revolver", {12.0f, 10.0f, -1.0f, 0.0f}},
    
    // Heavy - larger icons
    {"LMG", {18.0f, 10.0f, -4.0f, 0.0f}},
    {"RPG", {18.0f, 10.0f, -4.0f, 0.0f}},
    {"Flamethrower", {18.0f, 10.0f, -4.0f, 0.0f}},
    
    // Melee
    {"Knife", {13.0f, 13.0f, -5.0f, 0.0f}},
    {"Sword", {13.0f, 13.0f, -5.0f, 0.0f}},
    
    // Throwables
    {"Grenade", {10.0f, 10.0f, 0.0f, 0.0f}},
    {"Flashbang", {10.0f, 10.0f, 0.0f, 0.0f}},
    {"Smoke", {10.0f, 10.0f, 0.0f, 0.0f}},
    {"Molotov", {10.0f, 10.0f, 0.0f, 0.0f}},
};

inline const char* GetWeaponIcon(const std::string& weaponName)
{
    std::string cleanName = weaponName;

    if (!cleanName.empty() && cleanName.front() == '[' && cleanName.back() == ']') {
        cleanName = cleanName.substr(1, cleanName.length() - 2);
    }
    
    auto it = robloxWeaponIcons.find(cleanName);
    if (it != robloxWeaponIcons.end()) {
        return it->second;
    }
    return "";
}


inline WeaponIconSize GetWeaponIconSize(const std::string& weaponName)
{
    std::string cleanName = weaponName;
    
    // Remove brackets if present
    if (!cleanName.empty() && cleanName.front() == '[' && cleanName.back() == ']') {
        cleanName = cleanName.substr(1, cleanName.length() - 2);
    }
    
    auto it = weaponIconSizes.find(cleanName);
    if (it != weaponIconSizes.end()) {
        return it->second;
    }
    
    // Default size
    return {12.0f, 10.0f, 0.0f, 0.0f};
}
