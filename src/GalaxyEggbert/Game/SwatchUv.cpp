#include "SwatchUv.hpp"

namespace GalaxyEggbert::Game
{
    CnaEngine::Graphics::UvRect SwatchUv(const CnaEngine::Graphics::UvRect& tile, float vCenterFrac)
    {
        constexpr float kHalfSize = 0.06f;
        const float uMid = (tile.u0 + tile.u1) * 0.5f;
        const float uHalf = (tile.u1 - tile.u0) * kHalfSize;
        const float vSpan = tile.v1 - tile.v0;
        const float vCenter = tile.v0 + vSpan * vCenterFrac;
        const float vHalf = vSpan * kHalfSize;
        return CnaEngine::Graphics::UvRect{uMid - uHalf, vCenter - vHalf, uMid + uHalf, vCenter + vHalf};
    }
}
