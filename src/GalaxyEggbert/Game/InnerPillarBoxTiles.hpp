#pragma once

#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::Game
{
    // The "InnerPillarBox" render mode identified by mobile-eggbert-reference/
    // questionnaire-all-remaining-tiles.md: the block's outer 6 faces are
    // never drawn at all (fully transparent); instead a smaller box (a
    // "post"/"pillar") sits centered inside it. Reuses
    // Easy3D::DirectionalCubeItem/AppendDirectionalCubeMesh directly -- an
    // InnerPillarBox is really just a DirectionalCubeItem with a Size
    // smaller than the full block (see kInnerPillarWidth/kInnerPillarHeight
    // below) and per-face Visible/Uv exactly like DirectionalCube.
    //
    // 3 of 3 confirmed icons wired up (2026-07-08): icon 76 (4 side faces
    // textured, top/bottom open -- unstated in the questionnaire answer, so
    // treated as open to match the "outer cube fully transparent" theme)
    // and icons 384/385 (`BlockTypes::Switch`/`SwitchOff` -- 1 side
    // textured, other 5 flat fallback color, same `SwatchUv` approach as
    // `DirectionalCubeTiles`). Icon 384/385's single textured face has no
    // confirmed facing cue in either the questionnaire text or their crop
    // (a symmetric wall-panel icon) -- defaults to `CubeFace::PosZ`, same
    // convention as `DirectionalCubeTiles`'s symmetric-icon default.
    constexpr float kInnerPillarWidth = 0.35f;  // X/Z extent (thin post)
    constexpr float kInnerPillarHeight = 0.9f;  // Y extent (nearly full block height)

    // Called only after BlockDefinitionRegistry selected InnerPillarBox;
    // this helper supplies face parameters and does not classify icons.
    void ConfigureInnerPillarBoxFaces(int icon, const Easy3D::UvRect& tileUv,
                                      Easy3D::DirectionalCubeFace (&outFaces)[6]);
}
