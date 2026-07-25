#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// Bitmask of keys currently held by Blupi. Collected via ObjectType49/50/51.
// Serialised to/from save files — do not renumber.
enum class DoorKeyFlags : uint8_t
{
    None = 0,
    Key1 = 1 << 0, // ObjectType49
    Key2 = 1 << 1, // ObjectType50
    Key3 = 1 << 2, // ObjectType51
    All  = Key1 | Key2 | Key3,
};

constexpr uint8_t    ToRaw(DoorKeyFlags f)       { return static_cast<uint8_t>(f); }
constexpr DoorKeyFlags ToDoorKeyFlags(int v)     { return static_cast<DoorKeyFlags>(static_cast<uint8_t>(v)); }

constexpr DoorKeyFlags operator|(DoorKeyFlags a, DoorKeyFlags b) { return static_cast<DoorKeyFlags>(ToRaw(a) | ToRaw(b)); }
constexpr DoorKeyFlags operator&(DoorKeyFlags a, DoorKeyFlags b) { return static_cast<DoorKeyFlags>(ToRaw(a) & ToRaw(b)); }

constexpr bool operator<(DoorKeyFlags a, DoorKeyFlags b)  { return ToRaw(a) <  ToRaw(b); }
constexpr bool operator<=(DoorKeyFlags a, DoorKeyFlags b) { return ToRaw(a) <= ToRaw(b); }
constexpr bool operator>(DoorKeyFlags a, DoorKeyFlags b)  { return ToRaw(a) >  ToRaw(b); }
constexpr bool operator>=(DoorKeyFlags a, DoorKeyFlags b) { return ToRaw(a) >= ToRaw(b); }

} // namespace GalaxyEggbert::Def
