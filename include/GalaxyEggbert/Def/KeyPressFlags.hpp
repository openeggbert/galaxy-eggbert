#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// Bitmask of logical game buttons active this frame. Powers of two — combinable with OR.
enum class KeyPressFlags : uint8_t
{
    None  = 0,
    Jump  = 1,
    Fire  = 2,
    Down  = 4,
};

constexpr uint8_t      ToRaw(KeyPressFlags f)       { return static_cast<uint8_t>(f); }
constexpr KeyPressFlags ToKeyPressFlags(int v)      { return static_cast<KeyPressFlags>(static_cast<uint8_t>(v)); }

constexpr KeyPressFlags operator|(KeyPressFlags a, KeyPressFlags b) { return static_cast<KeyPressFlags>(ToRaw(a) | ToRaw(b)); }
constexpr KeyPressFlags operator&(KeyPressFlags a, KeyPressFlags b) { return static_cast<KeyPressFlags>(ToRaw(a) & ToRaw(b)); }

} // namespace GalaxyEggbert::Def
