#include "GEVoxelRaycast.hpp"

#include <cmath>
#include <limits>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr float kInf = std::numeric_limits<float>::infinity();

        // t distance along the ray at which it first crosses this axis's
        // NEXT integer cell boundary, given the (already block-centering-
        // shifted, see Raycast() below) origin/cell/step/direction.
        float InitialTMax(float shiftedOrigin, int cell, int step, float dirComponent)
        {
            if (step == 0)
            {
                return kInf;
            }
            const float boundary = step > 0 ? static_cast<float>(cell + 1) : static_cast<float>(cell);
            return (boundary - shiftedOrigin) / dirComponent;
        }
    }

    RaycastHit Raycast(const Worlds::World& world,
                        float originX, float originY, float originZ,
                        float dirX, float dirY, float dirZ,
                        float maxDistance)
    {
        RaycastHit result;

        const float length = std::sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
        if (length < 1e-6f)
        {
            return result; // degenerate direction -- no hit
        }
        dirX /= length;
        dirY /= length;
        dirZ /= length;

        // Block-centering shift (see this function's own header comment):
        // block index N spans [N-0.5, N+0.5), so shifting the origin by
        // +0.5 lets the rest of this function use the standard "cell K
        // spans [K, K+1)" DDA math unmodified, with the resulting cell
        // index already being the correct raw grid block index.
        const float shiftedOriginX = originX + 0.5f;
        const float shiftedOriginY = originY + 0.5f;
        const float shiftedOriginZ = originZ + 0.5f;

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());

        int cellX = static_cast<int>(std::floor(shiftedOriginX));
        int cellY = static_cast<int>(std::floor(shiftedOriginY));
        int cellZ = static_cast<int>(std::floor(shiftedOriginZ));

        const int stepX = dirX > 0.0f ? 1 : (dirX < 0.0f ? -1 : 0);
        const int stepY = dirY > 0.0f ? 1 : (dirY < 0.0f ? -1 : 0);
        const int stepZ = dirZ > 0.0f ? 1 : (dirZ < 0.0f ? -1 : 0);

        const float tDeltaX = (stepX != 0) ? std::fabs(1.0f / dirX) : kInf;
        const float tDeltaY = (stepY != 0) ? std::fabs(1.0f / dirY) : kInf;
        const float tDeltaZ = (stepZ != 0) ? std::fabs(1.0f / dirZ) : kInf;

        float tMaxX = InitialTMax(shiftedOriginX, cellX, stepX, dirX);
        float tMaxY = InitialTMax(shiftedOriginY, cellY, stepY, dirY);
        float tMaxZ = InitialTMax(shiftedOriginZ, cellZ, stepZ, dirZ);

        float t = 0.0f;
        int lastAxis = -1; // -1 = origin's own cell (no face crossed yet), 0=X, 1=Y, 2=Z

        while (t <= maxDistance)
        {
            if (cellX >= 0 && cellX < blocksPerAxis &&
                cellY >= 0 && cellY < blocksPerAxis &&
                cellZ >= 0 && cellZ < blocksPerAxis)
            {
                const auto block = world.getBlock(static_cast<std::uint16_t>(cellX),
                                                   static_cast<std::uint16_t>(cellY),
                                                   static_cast<std::uint16_t>(cellZ));
                if (!block.isAir())
                {
                    result.hit = true;
                    result.x = static_cast<std::uint16_t>(cellX);
                    result.y = static_cast<std::uint16_t>(cellY);
                    result.z = static_cast<std::uint16_t>(cellZ);
                    result.distance = t;
                    if (lastAxis == 0) result.normalX = static_cast<std::int8_t>(-stepX);
                    else if (lastAxis == 1) result.normalY = static_cast<std::int8_t>(-stepY);
                    else if (lastAxis == 2) result.normalZ = static_cast<std::int8_t>(-stepZ);
                    return result;
                }
            }

            if (tMaxX <= tMaxY && tMaxX <= tMaxZ)
            {
                if (stepX == 0) break; // no further progress possible on any axis
                cellX += stepX;
                t = tMaxX;
                tMaxX += tDeltaX;
                lastAxis = 0;
            }
            else if (tMaxY <= tMaxZ)
            {
                if (stepY == 0) break;
                cellY += stepY;
                t = tMaxY;
                tMaxY += tDeltaY;
                lastAxis = 1;
            }
            else
            {
                if (stepZ == 0) break;
                cellZ += stepZ;
                t = tMaxZ;
                tMaxZ += tDeltaZ;
                lastAxis = 2;
            }
        }

        return result; // hit stays false
    }
}
