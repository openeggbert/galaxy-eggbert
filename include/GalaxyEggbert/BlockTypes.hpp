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

// Map any icon in an animated tile group to the group's base/master icon.
// All tiles in a group share one cached material whose UV is updated each frame.
inline uint16_t tileAnimBase(uint16_t icon) {
    if (icon >= 68  && icon <= 72)  return Lava;     // lava 8-frame loop
    if (icon == 347 || icon == 373 || icon == 374) return Spike; // spike 16-frame
    if (icon >= 317 && icon <= 323) return Crusher;  // crusher 10-frame
    if (icon >= 378 && icon <= 383) return Saw;      // saw 6-frame loop
    if (icon >= 92  && icon <= 95)  return Water1;   // water1 6-frame loop
    if (icon == 91  || (icon >= 96 && icon <= 98)) return Water2; // water2 6-frame
    return icon;
}

// True if a tile type is a hazard (kills Blupi on contact when not shielded).
inline bool isHazard(uint16_t bt) {
    uint16_t base = tileAnimBase(bt);
    return base == Lava || base == Spike || base == Crusher || base == Saw;
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
        false, false, false, false, false, true, true, false, false, false, false, true, true, true, true, true, false, false, false, false,
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
        true, false, true, true, false, true, true, true, true, true, true, false, false, false, false, false, false, false, true, true,
        true, true, true, true, false, true, true, true, true, true, false, false, false, false, false, false, false, true, false, true,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, true, true, true, true, true, true, false, false, true, true, true, true, true,
        true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, true, true,
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
