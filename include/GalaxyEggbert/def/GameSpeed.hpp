#pragma once
#include <cstdint>

namespace GalaxyEggbert {

// Simulation ticks per rendered frame. Normal=1 is default.
enum class GameSpeed : uint8_t
{
    Slow    = 0,
    Normal  = 1,
    Fast    = 2,
    Faster  = 4,
    Fastest = 8,
};

constexpr uint8_t  ToRaw(GameSpeed s)      { return static_cast<uint8_t>(s); }
constexpr GameSpeed ToGameSpeed(int v)     { return static_cast<GameSpeed>(static_cast<uint8_t>(v)); }

constexpr bool operator<(GameSpeed a, GameSpeed b)  { return ToRaw(a) <  ToRaw(b); }
constexpr bool operator<=(GameSpeed a, GameSpeed b) { return ToRaw(a) <= ToRaw(b); }
constexpr bool operator>(GameSpeed a, GameSpeed b)  { return ToRaw(a) >  ToRaw(b); }
constexpr bool operator>=(GameSpeed a, GameSpeed b) { return ToRaw(a) >= ToRaw(b); }

} // namespace GalaxyEggbert
