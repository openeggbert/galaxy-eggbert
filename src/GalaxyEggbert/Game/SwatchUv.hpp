#pragma once

#include <CnaEngine/Graphics/MeshBuilder.hpp>

namespace GalaxyEggbert::Game
{
    // A small sample of a tile's OWN texture -- used to approximate a
    // "flat fallback color" face (DirectionalCube/InnerPillarBox) without a
    // second, vertex-color shader path. See DirectionalCubeTiles.hpp's
    // header comment for the full justification: every named fallback color
    // in the questionnaire answers is per-icon, not a fixed palette, so
    // sampling the icon's own pixels reproduces the right hue automatically.
    // kTopSwatchV/kBottomSwatchV anchor to a face described as "shora"/
    // "zdola" (top/bottom); kMidSwatchV is for a generic side face with no
    // such wording to anchor to.
    constexpr float kTopSwatchV = 0.08f;
    constexpr float kBottomSwatchV = 0.92f;
    constexpr float kMidSwatchV = 0.5f;

    CnaEngine::Graphics::UvRect SwatchUv(const CnaEngine::Graphics::UvRect& tile, float vCenterFrac);
}
