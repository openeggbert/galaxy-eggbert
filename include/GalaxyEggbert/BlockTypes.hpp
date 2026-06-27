#pragma once
#include <cstdint>

// Block type IDs for galaxy-eggbert's voxel World format.
//
// Design: block type = icon index into object-m.png.
// Air = 0 (no block). Every other value is the icon ID displayed as the
// tile face texture. Named constants equal their icon IDs so gameplay code
// can reference them by name without a separate mapping table.

namespace GalaxyEggbert {
namespace BlockTypes {

// object-m.png dimensions: 1301×1431 px, 64×64 px per tile, 20 columns.
constexpr int kSheetW    = 1301;
constexpr int kSheetH    = 1431;
constexpr int kTileSize  = 64;
constexpr int kSheetCols = kSheetW / kTileSize; // 20

constexpr uint16_t Air      =   0;   // empty / no block

// Named tile types — value equals the icon index in object-m.png.
constexpr uint16_t Ground   =  10;   // grass/ground
constexpr uint16_t StoneA   =  18;   // light stone
constexpr uint16_t StoneB   =  25;   // dark stone
constexpr uint16_t Wall     = 183;   // brick wall
constexpr uint16_t Platform = 200;   // floating platform
constexpr uint16_t Sp0      = 158;
constexpr uint16_t Sp1      = 159;
constexpr uint16_t Sp2      = 160;
constexpr uint16_t Sp3      = 161;
constexpr uint16_t Sp4      = 162;
constexpr uint16_t Sp5      = 163;
constexpr uint16_t Sp6      = 164;
constexpr uint16_t Sp7      = 165;
constexpr uint16_t Marker   = 309;
constexpr uint16_t Tile411  = 411;
constexpr uint16_t Tile412  = 412;
constexpr uint16_t Tile413  = 413;

// Hazard tiles — same rule: value = icon index. Gameplay checks use these.
constexpr uint16_t Lava     =  68;   // kills Blupi on contact (anim group 68–72)
constexpr uint16_t Spike    = 373;   // kills Blupi on contact (anim group 347,373,374)
constexpr uint16_t Crusher  = 317;   // kills Blupi on contact (anim group 317–323)
constexpr uint16_t Saw      = 378;   // kills Blupi on contact (anim group 378–383)

// Water tiles — decorative (anim groups 92–95 and 96–98).
constexpr uint16_t Water1   =  92;
constexpr uint16_t Water2   =  96;

// Spring tile — launches Blupi upward automatically on contact (SoundChannel41).
constexpr uint16_t Spring   = 211;

inline bool isSpring(uint16_t bt) { return bt == Spring; }

// Temp tile — oscillating 20-frame cycle (icons 324-329); invisible for 2 frames.
// When SetActive(false) during invisible frames, Blupi falls through (faithful to mobile-eggbert IsTemp).
constexpr uint16_t Temp = 324;

// Ventilator fan tiles — base frames only (127-128, 130-131, 133-134, 136-137 are passable Air).
// Fan blows Blupi: kills on contact unless shielded (mobile-eggbert IsVentillo → BlupiDead).
constexpr uint16_t FanLeft  = 126;  // blows in -X direction
constexpr uint16_t FanRight = 129;  // blows in +X direction
constexpr uint16_t FanUp    = 132;  // blows in -Z direction (2D: up = -Y)
constexpr uint16_t FanDown  = 135;  // blows in +Z direction (2D: down = +Y)

inline bool isFan(uint16_t bt) {
    return bt == FanLeft || bt == FanRight || bt == FanUp || bt == FanDown;
}

// Blitz tile — electric floor, kills Blupi 25 % of ticks (animPhase % 4 == 0).
// Icon 305 is a solid floor with an active-kill phase (same pattern as Crusher).
constexpr uint16_t Blitz    = 305;

inline bool isBlitz(uint16_t bt) { return bt == Blitz; }

// Teleporter tiles — solid pillars; icons 330-333 encode the pair ID.
// When Blupi is adjacent, scan the world for the matching icon to find the exit.
constexpr uint16_t Teleport1 = 330;
constexpr uint16_t Teleport2 = 331;
constexpr uint16_t Teleport3 = 332;
constexpr uint16_t Teleport4 = 333;

inline bool isTeleporter(uint16_t bt) {
    return bt == Teleport1 || bt == Teleport2 || bt == Teleport3 || bt == Teleport4;
}

// Switch tiles — Blupi presses Action to toggle (384=open/active, 385=closed/inactive).
// ActiveSwitch scans ±20 tiles in X at same Z for linked saws and toggles them.
constexpr uint16_t Switch    = 384;  // open — linked saws spinning (deadly)
constexpr uint16_t SwitchOff = 385;  // closed — linked saws stopped (safe)

// Stopped saw state set by switch. 378 = spinning (hazard), 379 = stopped (safe).
constexpr uint16_t SawStopped = 379;

inline bool isSwitch(uint16_t bt) { return bt == Switch || bt == SwitchOff; }

// Bridge tile — passable trigger in mobile-eggbert; in galaxy-eggbert rendered as solid.
// When Blupi steps on it, bridge-building animation (ObjectType52, PICKUP-064) triggers.
constexpr uint16_t Bridge = 364;

inline bool isBridge(uint16_t bt) { return bt == Bridge; }

// Marine tile — animated water surface (table_marine, 11 frames: 203→208→203).
constexpr uint16_t Marine = 203;

// Door tiles — blocked by key. Icon 334=red(key49), 335=green(key50), 336=blue(key51).
// Removed when Blupi is adjacent and holds the matching key.
constexpr uint16_t Door1    = 334;
constexpr uint16_t Door2    = 335;
constexpr uint16_t Door3    = 336;

inline bool isDoor(uint16_t bt)      { return bt == Door1 || bt == Door2 || bt == Door3; }
inline int  doorKeyType(uint16_t bt) {
    if (bt == Door1) return 49;
    if (bt == Door2) return 50;
    if (bt == Door3) return 51;
    return -1;
}

// Map any icon in an animated tile group to the group's base/master icon.
// All tiles in a group share one cached material whose UV is updated each frame.
inline uint16_t tileAnimBase(uint16_t icon) {
    if (icon >= 68  && icon <= 72)  return Lava;     // lava 8-frame loop
    if (icon == 347 || icon == 373 || icon == 374) return Spike; // spike 16-frame
    if (icon >= 317 && icon <= 323) return Crusher;  // crusher 10-frame
    if (icon >= 378 && icon <= 383) return Saw;      // saw 6-frame loop
    if (icon >= 92  && icon <= 95)  return Water1;   // water1 6-frame loop
    if (icon == 91  || (icon >= 96 && icon <= 98)) return Water2; // water2 6-frame
    if (icon >= 126 && icon <= 128) return FanLeft;  // fan-left 3-frame
    if (icon >= 129 && icon <= 131) return FanRight; // fan-right 3-frame
    if (icon >= 132 && icon <= 134) return FanUp;    // fan-up 3-frame
    if (icon >= 135 && icon <= 137) return FanDown;  // fan-down 3-frame
    if (icon >= 324 && icon <= 329) return Temp;     // temp tile 20-frame
    if (icon >= 203 && icon <= 208) return Marine;   // marine 11-frame
    return icon;
}

// True for water tiles (91=deep water, 92=water surface) — Blupi swims when grounded on these.
inline bool isWater(uint16_t bt) {
    uint16_t b = tileAnimBase(bt);
    return b == Water1 || b == Water2;
}

// True if a tile type is a hazard (kills Blupi on contact when not shielded).
inline bool isHazard(uint16_t bt) {
    uint16_t base = tileAnimBase(bt);
    return base == Lava || base == Spike || base == Crusher || base == Saw || base == Blitz;
}

// Block type → icon index. Since block type IS the icon index, this is trivial.
// Returns -1 for Air.
inline int toIconIndex(uint16_t t) {
    return (t == Air) ? -1 : static_cast<int>(t);
}

// True for icon IDs that have all-zero table_decor_quart sub-cells in
// mobile-eggbert (fully transparent/decorative — no collision).
// Icons 68 (Lava) and 317 (Crusher) are excluded and kept solid for the
// galaxy-eggbert hazard system even though they are quart-passable.
inline bool isMobileTransparent(int icon) {
    static const bool kPassable[441] = {
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, true, true, false, true, true, true, true, true, false, false, true, true, false, false,
        false, false, false, false, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true,
        true, true, true, true, true, true, false, true, true, false, true, true, false, true, true, false, true, true, true, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, false, false, true, true, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
        true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true,
        true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, true, true, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, false, true, true, false, false, true, true, true, true, true, false, false, false, false, false, false, false, true, true,
        true, true, true, true, false, true, true, true, true, true, false, false, false, false, false, false, false, true, false, true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, true, true, true, true, true, true, false, false, true, true, true, false, false,
        true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false
    };
    return icon >= 0 && icon < 441 && kPassable[icon];
}

// Convert a mobile-eggbert decor icon ID to a block type.
// Passable (decorative) tiles become Air; everything else keeps its icon ID.
// 0 or negative → Air.
inline uint16_t fromMobileIconId(int icon) {
    if (icon <= 0) return Air;
    if (isMobileTransparent(icon)) return Air;
    return static_cast<uint16_t>(icon);
}

// UV of tile for icon index (row-major, 20 cols).
inline void tileUV(int icon, float& uOff, float& vOff, float& uScale, float& vScale) {
    const int col = icon % kSheetCols;
    const int row = icon / kSheetCols;
    uScale = static_cast<float>(kTileSize) / static_cast<float>(kSheetW);
    vScale = static_cast<float>(kTileSize) / static_cast<float>(kSheetH);
    uOff   = col * uScale;
    vOff   = row * vScale;
}

} // namespace BlockTypes
} // namespace GalaxyEggbert
