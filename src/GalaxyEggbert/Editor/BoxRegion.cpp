#include "BoxRegion.hpp"

#include <algorithm>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        std::uint16_t ClampAxis(int value, int blocksPerAxis)
        {
            const int clamped = std::clamp(value, 0, blocksPerAxis - 1);
            return static_cast<std::uint16_t>(clamped);
        }
    }

    BoxRegion NormalizeAndClamp(int x0, int y0, int z0,
                                int x1, int y1, int z1,
                                int blocksPerAxis)
    {
        BoxRegion region;
        region.minX = ClampAxis(std::min(x0, x1), blocksPerAxis);
        region.maxX = ClampAxis(std::max(x0, x1), blocksPerAxis);
        region.minY = ClampAxis(std::min(y0, y1), blocksPerAxis);
        region.maxY = ClampAxis(std::max(y0, y1), blocksPerAxis);
        region.minZ = ClampAxis(std::min(z0, z1), blocksPerAxis);
        region.maxZ = ClampAxis(std::max(z0, z1), blocksPerAxis);
        return region;
    }
}
