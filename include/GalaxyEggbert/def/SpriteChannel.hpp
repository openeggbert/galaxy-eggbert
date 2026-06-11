#pragma once
#include <cstdint>

namespace GalaxyEggbert {

// Identifies which sprite sheet (texture atlas) to use for a draw call.
// Values mirror the CH* constants from the original game. Gaps at 7 and 8 are unused.
// In galaxy-eggbert these textures are used as cube face textures and billboard quads.
enum class SpriteChannel : uint8_t
{
    Object                = 1,  // moving objects (object-m.png)
    Blupi                 = 2,  // main Blupi character (blupi.png)
    Background            = 3,  // world/decor background tiles (decor*.png)
    Button                = 4,  // UI buttons (button.png)
    Jauge                 = 5,  // HUD gauge (jauge.png)
    Text                  = 6,  // font glyphs (text.png)
    // 7, 8: unused in original
    Explosion             = 9,  // explosion particles (explo.png)
    Element               = 10, // collectibles and effects (element.png)
    Blupi1_11             = 11, // alternate Blupi skin 1 (blupi1.png)
    Blupi1_12             = 12, // alternate Blupi skin 2
    Blupi1_13             = 13, // alternate Blupi skin 3
    Pad                   = 14, // touch-input pad overlay (pad.png)
    SpeedyBlupiBackground = 15, // title screen background
    BlupiYoupieBackground = 16, // Blupi Youpie background
    GearBackground        = 17, // settings/gear background
};

constexpr uint8_t     ToRaw(SpriteChannel c)       { return static_cast<uint8_t>(c); }
constexpr SpriteChannel ToSpriteChannel(int v)     { return static_cast<SpriteChannel>(static_cast<uint8_t>(v)); }

} // namespace GalaxyEggbert
