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
constexpr uint16_t Lava     =  68;   // kills Blupi on contact
constexpr uint16_t Spike    = 373;   // kills Blupi on contact
constexpr uint16_t Crusher  = 317;   // kills Blupi on contact

// Block type → icon index. Since block type IS the icon index, this is trivial.
// Returns -1 for Air.
inline int toIconIndex(uint16_t t) {
    return (t == Air) ? -1 : static_cast<int>(t);
}

// Convert a mobile-eggbert decor icon ID to a block type.
// Every positive ID is stored directly; 0 or negative → Air.
inline uint16_t fromMobileIconId(int icon) {
    return icon > 0 ? static_cast<uint16_t>(icon) : Air;
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
