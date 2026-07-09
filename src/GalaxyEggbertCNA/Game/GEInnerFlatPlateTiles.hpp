#pragma once

#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::CNA
{
    // Icons confirmed "InnerFlatPlate" in mobile-eggbert-reference/
    // questionnaire-all-remaining-tiles.md: the block's outer 6 faces are
    // never drawn (fully transparent); instead a single flat double-sided
    // plate (Easy3D::PlateItem) sits centered inside it -- signposts, thin
    // posts, screens. 63 of 63 confirmed icons wired up (2026-07-08). Every
    // one uses the same fixed plate size (see GETerrainRenderer.cpp's
    // kInnerFlatPlateWidth/Height) -- a sample of their crops (77, 110, 114,
    // 264, 367) showed the same pattern already found for most single-face
    // DirectionalCube icons: small, front-facing or left-right-symmetric
    // decorations with no reliable axis cue, so a fixed Z-axis default is a
    // harmless tie-break rather than a guess (see GEDirectionalCubeTiles.hpp's
    // identical reasoning for symmetric icons). Not individually
    // re-verified per icon beyond that sample -- except icons 368-372
    // (2026-07-09 spot-check, NEXT.md §8 task 4): their crops show a clearly
    // different pattern from the rest (an elongated, horizontally-lying
    // segmented shape sitting mid-tile, not a vertical frame/bracket), so
    // GetInnerFlatPlateAxis() returns PlateAxis::Y (horizontal, matching the
    // grass-top overlay's usage of the same axis) for those 5 only.
    bool IsInnerFlatPlateIcon(int icon);

    // PlateAxis::Z for every confirmed icon except 368-372 (PlateAxis::Y --
    // see the reasoning above). Only meaningful when IsInnerFlatPlateIcon()
    // is true.
    Easy3D::PlateAxis GetInnerFlatPlateAxis(int icon);
}
