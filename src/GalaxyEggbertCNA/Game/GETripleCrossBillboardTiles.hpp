#pragma once

namespace GalaxyEggbert::CNA
{
    // Icons confirmed "TripleCrossBillboard" in mobile-eggbert-reference/
    // questionnaire-all-remaining-tiles.md: the same texture on 3 vertical
    // double-sided planes through the block's center, 60° apart, forming a
    // triangle in plan view (Easy3D::TripleCrossItem). Rotationally
    // symmetric by construction, so -- unlike DirectionalCube/InnerFlatPlate
    // -- there is no per-icon facing decision to make; all 10 of 10
    // confirmed icons are wired up (2026-07-08).
    bool IsTripleCrossBillboardIcon(int icon);
}
