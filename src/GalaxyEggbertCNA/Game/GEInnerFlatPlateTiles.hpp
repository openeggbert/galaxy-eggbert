#pragma once

namespace GalaxyEggbert::CNA
{
    // Icons confirmed "InnerFlatPlate" in mobile-eggbert-reference/
    // questionnaire-all-remaining-tiles.md: the block's outer 6 faces are
    // never drawn (fully transparent); instead a single flat double-sided
    // plate (Easy3D::PlateItem) sits centered inside it -- signposts, thin
    // posts, screens. 63 of 63 confirmed icons wired up (2026-07-08). Every
    // one uses the same fixed plate size/axis (see GETerrainRenderer.cpp's
    // kInnerFlatPlateWidth/Height and PlateAxis::Z default) -- a sample of
    // their crops (77, 110, 114, 264, 367) showed the same pattern already
    // found for most single-face DirectionalCube icons: small, front-facing
    // or left-right-symmetric decorations with no reliable axis cue, so a
    // fixed default is a harmless tie-break rather than a guess (see
    // GEDirectionalCubeTiles.hpp's identical reasoning for symmetric
    // icons). Not individually re-verified per icon beyond that sample.
    bool IsInnerFlatPlateIcon(int icon);
}
