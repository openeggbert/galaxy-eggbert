#pragma once
#include <cstdint>

// Block type IDs and tile UV helpers for galaxy-eggbert's voxel World format.
// Icon indices and tile categories are derived from the mobile-eggbert world format
// (see mobile-eggbert/worlds/world001.txt and Decor.cpp for the original 2D mapping).

namespace GalaxyEggbert {
namespace BlockTypes {

// Galaxy-eggbert block type IDs (12-bit, 0=air).
// Tile icons reference object-m.png (20-column, 64×64 tile grid).
constexpr uint16_t Air      = 0;
constexpr uint16_t Ground   = 1;   // icon  10  – grass/ground
constexpr uint16_t StoneA   = 2;   // icon  18  – light stone
constexpr uint16_t StoneB   = 3;   // icon  25  – dark stone
constexpr uint16_t Wall     = 4;   // icon 183  – brick wall
constexpr uint16_t Platform = 5;   // icon 200  – floating platform
constexpr uint16_t Sp0      = 6;   // icon 158
constexpr uint16_t Sp1      = 7;   // icon 159
constexpr uint16_t Sp2      = 8;   // icon 160
constexpr uint16_t Sp3      = 9;   // icon 161
constexpr uint16_t Sp4      = 10;  // icon 162
constexpr uint16_t Sp5      = 11;  // icon 163
constexpr uint16_t Sp6      = 12;  // icon 164
constexpr uint16_t Sp7      = 13;  // icon 165
constexpr uint16_t Marker   = 14;  // icon 309
constexpr uint16_t Tile411  = 15;  // icon 411
constexpr uint16_t Tile412  = 16;  // icon 412
constexpr uint16_t Tile413  = 17;  // icon 413

// Returns the icon index into object-m.png for a given block type, or -1 for air.
inline int toIconIndex(uint16_t t) {
    switch (t) {
        case Ground:   return  10;
        case StoneA:   return  18;
        case StoneB:   return  25;
        case Wall:     return 183;
        case Platform: return 200;
        case Sp0:      return 158;
        case Sp1:      return 159;
        case Sp2:      return 160;
        case Sp3:      return 161;
        case Sp4:      return 162;
        case Sp5:      return 163;
        case Sp6:      return 164;
        case Sp7:      return 165;
        case Marker:   return 309;
        case Tile411:  return 411;
        case Tile412:  return 412;
        case Tile413:  return 413;
        default:       return  -1;
    }
}

// object-m.png dimensions: 1301×1431 px, 64×64 px per tile, 20 columns.
constexpr int kSheetW   = 1301;
constexpr int kSheetH   = 1431;
constexpr int kTileSize = 64;
constexpr int kSheetCols = kSheetW / kTileSize; // 20
constexpr int kSheetRows = kSheetH / kTileSize; // 22

// UV of tile for icon index (col-major layout: icon = row*cols + col).
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
