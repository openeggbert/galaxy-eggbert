#pragma once

#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::CNA
{
    // Per-icon face configuration for the "DirectionalCube" render mode
    // identified by mobile-eggbert-reference/
    // questionnaire-unidentified-tiles.md and questionnaire-all-remaining-
    // tiles.md's direct user Q&A: a cube where only some faces show the
    // tile's real texture; the rest are either a flat fallback color or
    // genuinely open (no face at all). Every block type NOT listed here
    // keeps the default UniformCube treatment (all 6 faces textured) that
    // GETerrainRenderer already does.
    //
    // 90 of ~99 confirmed `DirectionalCube` icons are wired up as of
    // 2026-07-08 (NEXT.md §8 task 1): icon 200 (Platform), 48 "all 4 sides
    // textured, top/bottom independently flat-color-or-open" icons, the 4
    // fan tiles (126/129/132/135), and 37 "1 or 2 of 4 sides textured"
    // icons whose facing was determined directly from their crop image
    // (mobile-eggbert confirmed to have NO per-placement rotation field at
    // all -- for any tile that varies by facing it just uses a different
    // icon number, e.g. FanLeft/FanRight/FanUp/FanDown; see NEXT.md §3).
    // Still missing: icons 30/31 (need real texture alpha), icons 107-109
    // (need a not-yet-sourced grass top texture), and icons 15-18 (need a
    // two-part axis+side-asymmetry read their crops didn't give a confident
    // answer for). Do not guess a new icon's face config; only add entries
    // whose exact wording is confirmed in the questionnaire files, and only
    // once the table can actually express that answer (add a real
    // capability, don't fake one) -- or, for a facing call, only once the
    // crop image gives an actually confident directional read.
    //
    // Returns true and fills outFaces[6] (indexed by Easy3D::CubeFace) if
    // icon is a known DirectionalCube tile. A "flat fallback color" face's
    // Uv is NOT tileUv -- it's a small swatch of the SAME tile's texture
    // sampled near its top or bottom edge (see GEDirectionalCubeTiles.cpp's
    // SwatchUv), approximating that color without a second vertex-color
    // shader path; this reproduces the actual per-icon hue the questionnaire
    // answers describe (each says a color "belonging" to that specific icon,
    // not a fixed palette). Returns false (and leaves outFaces untouched)
    // for every other icon.
    bool TryGetDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                    Easy3D::DirectionalCubeFace (&outFaces)[6]);
}
