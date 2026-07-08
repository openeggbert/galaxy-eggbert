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
    // Only icon 200 (Platform) is wired up so far -- the first confirmed,
    // fully-specified DirectionalCube tile, used to prove the render mode
    // end-to-end (NEXT.md §8 task 1, 2026-07-08). ~100 other confirmed
    // DirectionalCube icons from the questionnaires still need their
    // per-face answer transcribed here -- see NEXT.md for that follow-on
    // task. Do not guess a new icon's face config; only add entries whose
    // exact wording is confirmed in the questionnaire files.
    //
    // Returns true and fills outFaces[6] (indexed by Easy3D::CubeFace) if
    // icon is a known DirectionalCube tile; outFaces[i].Uv is always set to
    // tileUv for visible faces (DirectionalCube tiles found so far all show
    // the same icon texture on every textured face -- no tile has been
    // confirmed needing a different texture per face). Returns false (and
    // leaves outFaces untouched) for every other icon.
    bool TryGetDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                    Easy3D::DirectionalCubeFace (&outFaces)[6]);
}
