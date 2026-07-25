#pragma once

#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::Game
{
    // Per-icon face configuration for the "DirectionalCube" render mode
    // identified by mobile-eggbert-reference/
    // questionnaire-unidentified-tiles.md and questionnaire-all-remaining-
    // tiles.md's direct user Q&A: a cube where only some faces show the
    // tile's real texture; the rest are either a flat fallback color or
    // genuinely open (no face at all). Every block type NOT listed here
    // keeps the default UniformCube treatment (all 6 faces textured) that
    // TerrainRenderer already does.
    //
    // All ~99 confirmed `DirectionalCube` icons are wired up as of
    // 2026-07-09 (NEXT.md §8 task 1, complete): icon 200 (Platform), 48 "all
    // 4 sides textured, top/bottom independently flat-color-or-open" icons,
    // the 4 fan tiles (126/129/132/135), icons 30/31 (needed real texture
    // alpha -- see BlockDefinition::alphaBlend), icon 107 (top=false
    // here on purpose -- its actual grass top texture is rendered separately
    // by BlockDefinition::grassTop/m_grassRenderer, not this
    // table), 37 "1 or 2 of 4 sides textured" icons whose facing was
    // determined directly from their crop image (mobile-eggbert confirmed to
    // have NO per-placement rotation field at all -- for any tile that
    // varies by facing it just uses a different icon number, e.g.
    // FanLeft/FanRight/FanUp/FanDown; see NEXT.md §3), icons 15-18 (axis +
    // open/color side pair, defaulted 2026-07-09 -- crops didn't give a
    // confident read even after direct user review on mobile, see NEXT.md
    // §3), and icons 108-109 (own texture + icon 107's texture + one open
    // side + grass top, 2026-07-09 -- same default-facing reasoning as
    // 15-18). Do not guess a new icon's face config; only add entries whose
    // exact wording is confirmed in the questionnaire files, and only once
    // the table can actually express that answer (add a real capability,
    // don't fake one) -- or, for a facing call, only once the crop image
    // gives an actually confident directional read.
    //
    // Fills outFaces[6] (indexed by Easy3D::CubeFace) for the
    // DirectionalCube mode already selected by BlockDefinitionRegistry. A
    // "flat fallback color" face's
    // Uv is NOT tileUv -- it's a small swatch of the SAME tile's texture
    // sampled near its top or bottom edge (see DirectionalCubeTiles.cpp's
    // SwatchUv), approximating that color without a second vertex-color
    // shader path; this reproduces the actual per-icon hue the questionnaire
    // answers describe (each says a color "belonging" to that specific icon,
    // not a fixed palette). Returns false (and leaves outFaces untouched)
    // for every other face treatment. Calling this for a non-DirectionalCube
    // icon is a programming error; this helper no longer classifies icons.
    //
    // @p icon107Uv is icon 107's own tile UV (TileAtlas::GetTileUv(107)),
    // needed only by icons 108/109, whose confirmed answer reuses icon 107's
    // texture on one of their own side faces. Every other icon ignores it --
    // callers already compute a UV per block anyway, so this is just
    // threading one extra, cheap, already-available value through rather
    // than growing this function's own icon-lookup responsibilities.
    void ConfigureDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                       const Easy3D::UvRect& icon107Uv,
                                       Easy3D::DirectionalCubeFace (&outFaces)[6]);
}
