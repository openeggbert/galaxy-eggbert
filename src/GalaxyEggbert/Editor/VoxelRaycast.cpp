#include "VoxelRaycast.hpp"

namespace GalaxyEggbert::Editor
{
    RaycastHit Raycast(const Worlds::World& world, float originX, float originY, float originZ,
                       float dirX, float dirY, float dirZ, float maxDistance)
    {
        return Worlds::RaycastGrid(world.blocksPerAxis(), originX, originY, originZ,
            dirX, dirY, dirZ, maxDistance, [&world](int x, int y, int z) {
                return !world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                       static_cast<std::uint16_t>(z)).isAir();
            });
    }
}
