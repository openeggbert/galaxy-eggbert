#pragma once
#include <cstdint>

namespace GalaxyEggbert::Def {

// Active special power-up. Only one can be active at a time. Values match SEC_* constants.
enum class SecretPower : uint8_t
{
    None   = 0,
    Shield = 1,
    Power  = 2,
    Cloud  = 3,
    Hide   = 4,
};

constexpr uint8_t    ToRaw(SecretPower p)     { return static_cast<uint8_t>(p); }
constexpr SecretPower ToSecretPower(int v)    { return static_cast<SecretPower>(static_cast<uint8_t>(v)); }

} // namespace GalaxyEggbert::Def
