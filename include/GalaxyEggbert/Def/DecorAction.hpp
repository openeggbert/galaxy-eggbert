#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// Camera-shake animation type. Values 3 and 4 are unassigned in the original game.
enum class DecorAction : uint8_t
{
    None          = 0,
    SmallShake    = 1, // minor impacts: crate landing, small explosion, bonus collected
    BigShake      = 2, // major impacts: fan blade hit, large explosion
    ElectricShake = 5, // Blupi contacts electric field (ObjectType90)
};

constexpr uint8_t   ToRaw(DecorAction a)       { return static_cast<uint8_t>(a); }
constexpr DecorAction ToDecorAction(int v)     { return static_cast<DecorAction>(static_cast<uint8_t>(v)); }

constexpr bool operator<(DecorAction a, DecorAction b)  { return ToRaw(a) <  ToRaw(b); }
constexpr bool operator<=(DecorAction a, DecorAction b) { return ToRaw(a) <= ToRaw(b); }
constexpr bool operator>(DecorAction a, DecorAction b)  { return ToRaw(a) >  ToRaw(b); }
constexpr bool operator>=(DecorAction a, DecorAction b) { return ToRaw(a) >= ToRaw(b); }

} // namespace GalaxyEggbert::Def
