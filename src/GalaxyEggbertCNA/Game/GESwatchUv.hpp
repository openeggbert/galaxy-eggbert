#pragma once

#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::CNA
{
    // A small sample of a tile's OWN texture -- used to approximate a
    // "flat fallback color" face (DirectionalCube/InnerPillarBox) without a
    // second, vertex-color shader path. See GEDirectionalCubeTiles.hpp's
    // header comment for the full justification: every named fallback color
    // in the questionnaire answers is per-icon, not a fixed palette, so
    // sampling the icon's own pixels reproduces the right hue automatically.
    // kTopSwatchV/kBottomSwatchV anchor to a face described as "shora"/
    // "zdola" (top/bottom); kMidSwatchV is for a generic side face with no
    // such wording to anchor to.
    constexpr float kTopSwatchV = 0.08f;
    constexpr float kBottomSwatchV = 0.92f;
    constexpr float kMidSwatchV = 0.5f;

    Easy3D::UvRect SwatchUv(const Easy3D::UvRect& tile, float vCenterFrac);
}
