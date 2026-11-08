#pragma once

#include "LavaSurfaceData.hpp"
#include <algorithm>

namespace GalaxyEggbert::Game
{
// Shared by the pool mesh and collision. Heights are fractions above the cell's bottom;
// controller feet and rendered feet differ by the same half-cell as ordinary terrain.
[[nodiscard]] inline float LavaSurfaceHeight(int frame, float u, float w) noexcept
{
    const float gx = std::clamp(u, 0.0f, 1.0f) * (kLavaGridX - 1);
    const float gz = std::clamp(w, 0.0f, 1.0f) * (kLavaGridZ - 1);
    const int x0 = std::min(static_cast<int>(gx), kLavaGridX - 2);
    const int z0 = std::min(static_cast<int>(gz), kLavaGridZ - 2);
    const float fx = gx - x0, fz = gz - z0;
    const auto &h = kLavaHeights[std::clamp(frame, 0, kLavaFrames - 1)];
    return (h[z0][x0] * (1 - fx) + h[z0][x0 + 1] * fx) * (1 - fz) +
           (h[z0 + 1][x0] * (1 - fx) + h[z0 + 1][x0 + 1] * fx) * fz;
}
} // namespace GalaxyEggbert::Game
