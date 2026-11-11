#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// Simulation ticks per rendered frame. Normal=1 is default. The slow ones step the simulation by a
// fraction instead (SpeedFactor): Slow (Tab, the original's 0.5), SuperSlow 0.25 and UltraSlow 0.125
// (Galaxy's, the owner 2026-11-09: Tab steps 0.5, 0.25, 0.125, back to 1). Their raw values are
// stored in demo files; the new ones take values of their own, so compare speeds with the helpers
// below, not by their order.
enum class GameSpeed : uint8_t
{
    Slow      = 0,
    Normal    = 1,
    Fast      = 2,
    Faster    = 4,
    Fastest   = 8,
    OneAndHalf = 0x20, // F9: 1.5x (Galaxy's, the owner 2026-11-11), between F5's 1x and F6's 2x
    SuperSlow = 0x40,
    UltraSlow = 0x80,
};

constexpr uint8_t  ToRaw(GameSpeed s)      { return static_cast<uint8_t>(s); }
constexpr GameSpeed ToGameSpeed(int v)     { return static_cast<GameSpeed>(static_cast<uint8_t>(v)); }

// How many times the normal pace.
constexpr float SpeedFactor(GameSpeed s)
{
    return s == GameSpeed::Slow        ? 0.5f
           : s == GameSpeed::SuperSlow  ? 0.25f
           : s == GameSpeed::UltraSlow  ? 0.125f
           : s == GameSpeed::OneAndHalf ? 1.5f
                                        : static_cast<float>(ToRaw(s));
}
constexpr bool IsSlow(GameSpeed s) { return s == GameSpeed::Slow || s == GameSpeed::SuperSlow || s == GameSpeed::UltraSlow; }
// F7's and F8's (the "quick" cheat's), beyond the plain Fast.
constexpr bool IsBeyondFast(GameSpeed s) { return s == GameSpeed::Faster || s == GameSpeed::Fastest; }
// Tab's next: 0.5, 0.25, 0.125, then back to normal; from any other speed, 0.5.
constexpr GameSpeed NextSlowSpeed(GameSpeed s)
{
    return s == GameSpeed::Slow        ? GameSpeed::SuperSlow
           : s == GameSpeed::SuperSlow ? GameSpeed::UltraSlow
           : s == GameSpeed::UltraSlow ? GameSpeed::Normal
                                       : GameSpeed::Slow;
}

// 1.5x is a plain speed: no cheat needed (under F6's 2x), not slow, not Tab's.
static_assert(SpeedFactor(GameSpeed::OneAndHalf) == 1.5f && !IsBeyondFast(GameSpeed::OneAndHalf) &&
              !IsSlow(GameSpeed::OneAndHalf) && NextSlowSpeed(GameSpeed::OneAndHalf) == GameSpeed::Slow);
static_assert(SpeedFactor(GameSpeed::Normal) == 1.0f && SpeedFactor(GameSpeed::Fast) == 2.0f &&
              SpeedFactor(GameSpeed::Slow) == 0.5f && SpeedFactor(GameSpeed::Fastest) == 8.0f);

constexpr bool operator<(GameSpeed a, GameSpeed b)  { return ToRaw(a) <  ToRaw(b); }
constexpr bool operator<=(GameSpeed a, GameSpeed b) { return ToRaw(a) <= ToRaw(b); }
constexpr bool operator>(GameSpeed a, GameSpeed b)  { return ToRaw(a) >  ToRaw(b); }
constexpr bool operator>=(GameSpeed a, GameSpeed b) { return ToRaw(a) >= ToRaw(b); }

} // namespace GalaxyEggbert::Def
