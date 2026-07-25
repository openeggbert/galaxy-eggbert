#include "SwatchUv.hpp"

namespace GalaxyEggbert::Game
{
    Easy3D::UvRect SwatchUv(const Easy3D::UvRect& tile, float vCenterFrac)
    {
        constexpr float kHalfSize = 0.06f;
        const float uMid = (tile.U0 + tile.U1) * 0.5f;
        const float uHalf = (tile.U1 - tile.U0) * kHalfSize;
        const float vSpan = tile.V1 - tile.V0;
        const float vCenter = tile.V0 + vSpan * vCenterFrac;
        const float vHalf = vSpan * kHalfSize;
        return Easy3D::UvRect{uMid - uHalf, vCenter - vHalf, uMid + uHalf, vCenter + vHalf};
    }
}
