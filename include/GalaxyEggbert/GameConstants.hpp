#pragma once

namespace GalaxyEggbert {

// Game-world grid dimensions. Levels are 100×100 tiles max (same as original).
inline constexpr int MAXCELX = 100;
inline constexpr int MAXCELY = 100;

// Logical 2D viewport size (original game resolution).
inline constexpr int LXIMAGE = 640;
inline constexpr int LYIMAGE = 480;

// Sprite cell dimensions (pixels in the original sprite sheets).
inline constexpr int DIMOBJX    = 64;  // moving-object sprite cell width
inline constexpr int DIMOBJY    = 64;
inline constexpr int DIMBLUPIX  = 60;  // Blupi character sprite cell width
inline constexpr int DIMBLUPIY  = 60;
inline constexpr int DIMEXPLOX  = 128; // explosion sprite cell width
inline constexpr int DIMEXPLOY  = 128;
inline constexpr int DIMBUTTONX = 40;  // UI button sprite cell width
inline constexpr int DIMBUTTONY = 40;
inline constexpr int DIMJAUGEX  = 124; // HUD gauge sprite cell width
inline constexpr int DIMJAUGEY  = 22;
inline constexpr int DIMSTATX   = 60;  // status display area width
inline constexpr int DIMSTATY   = 30;
inline constexpr int DIMTEXTX   = 32;  // font glyph cell width
inline constexpr int DIMTEXTY   = 32;

} // namespace GalaxyEggbert
