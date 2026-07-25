#pragma once

#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::Game
{
    // The "thin-bar" render mode (plan.md TILE-055) for icon 202 -- a real
    // grabbable bar/rope tile (plan.md TILE-045, BlupiController's
    // Suspended/hanging mode) confirmed by the user's own 2026-07-07
    // questionnaire answer: "tenky hranolovy (ne krychlovy) model tyce,
    // textura na 4 dlouhych stranach, 2 celni (male, ctvercove) strany
    // modre; jina geometrie nez ThinMechanical i DirectionalCube" -- a thin
    // prismatic (not cubic) rod, textured on its 4 long sides, with 2 small
    // square blue end caps; explicitly a different shape than the existing
    // InnerFlatPlate/DirectionalCube modes, not a variant of either.
    //
    // Reuses Easy3D::DirectionalCubeItem/AppendDirectionalCubeMesh directly,
    // same technique as InnerPillarBox -- a thin bar is really just a
    // DirectionalCubeItem with a non-cubic Size (full block width along its
    // long axis, so adjacent bar blocks connect seamlessly when Blupi walks
    // across several in a row, thin in the other two dimensions) and
    // per-face Visible/Uv. Oriented along X by default -- mobile-eggbert's
    // own 2D world has no Z depth, so every real bar conceptually runs
    // along its only horizontal axis; no per-instance rotation data exists
    // in the world format to pick a different orientation, same "no
    // confirmed facing cue" precedent already used elsewhere (e.g.
    // InnerPillarBoxTiles' icon 384/385 defaulting to PosZ). The 2 blue
    // end caps use the same SwatchUv "sample the icon's own texture"
    // fallback-color technique as InnerPillarBox's icon 384/385 faces --
    // the questionnaire names a colour ("modre"), not a fixed palette
    // entry, and doesn't anchor it to "shora"/"zdola" wording, so the
    // generic kMidSwatchV sample is used (same default InnerPillarBox
    // already established for an unanchored single face).
    constexpr float kThinBarThickness = 0.3f; // Y/Z extent (thin rod cross-section)

    bool TryGetThinBarFaces(int icon, const Easy3D::UvRect& tileUv,
                             Easy3D::DirectionalCubeFace (&outFaces)[6]);
}
