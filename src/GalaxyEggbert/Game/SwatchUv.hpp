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

    // Centred on the tile's (uCenterFrac, vCenterFrac) and three texels across (2026-10-26:
    // it was 12 % of the tile, about 8 x 8 texels, whose detail stretched over a whole face
    // showed as stripes, reported on the window panels 186-192), so it reads as one colour.
    CnaEngine::Graphics::UvRect SwatchUv(const CnaEngine::Graphics::UvRect& tile, float vCenterFrac,
                                         float uCenterFrac = 0.5f);
}
