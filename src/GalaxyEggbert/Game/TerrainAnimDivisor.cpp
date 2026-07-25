#include "TerrainAnimDivisor.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>

namespace GalaxyEggbert::Game
{
    int AnimDivisor(std::uint16_t base) noexcept
    {
        const auto& animation = GalaxyEggbert::GetBlockDefinition(base).animation;
        return animation.IsAnimated() ? animation.divisor : 3;
    }
}
