#include "SwatchUv.hpp"

namespace GalaxyEggbert::Game
{
    CnaEngine::Graphics::UvRect SwatchUv(const CnaEngine::Graphics::UvRect& tile, float vCenterFrac,
                                         float uCenterFrac)
    {
        constexpr float kHalfSize = 1.5f / 64.0f;
        const float uMid = tile.u0 + (tile.u1 - tile.u0) * uCenterFrac;
        const float uHalf = (tile.u1 - tile.u0) * kHalfSize;
        const float vSpan = tile.v1 - tile.v0;
        const float vCenter = tile.v0 + vSpan * vCenterFrac;
        const float vHalf = vSpan * kHalfSize;
        return CnaEngine::Graphics::UvRect{uMid - uHalf, vCenter - vHalf, uMid + uHalf, vCenter + vHalf};
    }
}
