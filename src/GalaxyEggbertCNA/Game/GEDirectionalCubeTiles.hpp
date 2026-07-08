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
    // Icon 200 (Platform, all 4 sides textured, open top/bottom) plus 48
    // "all 4 sides textured, top/bottom independently flat-color-or-open"
    // icons are wired up as of 2026-07-08 (NEXT.md §8 task 1) -- every icon
    // confirmed `DirectionalCube` in the questionnaires EXCEPT the ones
    // needing capabilities this table can't express yet: per-placement face
    // rotation ("směr v metadatech bloku" -- only 1 or 2 of 4 sides
    // textured; galaxy-eggbert's World/Block format has no per-block
    // orientation field to read that from), real texture alpha (icons
    // 30/31), or a not-yet-sourced grass top texture (icons 107-109). Do not
    // guess a new icon's face config; only add entries whose exact wording
    // is confirmed in the questionnaire files, and only once the table can
    // actually express that answer (add a real capability, don't fake one).
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
