#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// Horizontal facing direction of Blupi or an enemy.
enum class Direction : uint8_t
{
    None  = 0,
    Left  = 1,
    Right = 2,
};

constexpr uint8_t  ToRaw(Direction d)   { return static_cast<uint8_t>(d); }
constexpr Direction ToDirection(int v)  { return static_cast<Direction>(static_cast<uint8_t>(v)); }

} // namespace GalaxyEggbert::Def
