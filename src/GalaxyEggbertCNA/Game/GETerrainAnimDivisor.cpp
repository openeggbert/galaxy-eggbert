#include "GETerrainAnimDivisor.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::CNA
{
    int AnimDivisor(std::uint16_t base) noexcept
    {
        using namespace GalaxyEggbert::BlockTypes;
        switch (base)
        {
            case Saw:                                              return 1; // 50ms/frame
            case Lava:                                              return 2; // 100ms/frame
            case FanLeft: case FanRight: case FanUp: case FanDown:
            case Water1: case Crusher: case Water2: case Marine:    return 3; // 150ms/frame
            case Spike: case Temp:                                  return 4; // 200ms/frame
            default:                                                return 3;
        }
    }
}
