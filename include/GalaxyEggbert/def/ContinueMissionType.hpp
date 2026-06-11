#pragma once
#include <cstdint>

namespace GalaxyEggbert {

// State of a "continue from checkpoint" request in the game session.
enum class ContinueMissionType : uint8_t
{
    None    = 0,
    Pending = 1,
    Active  = 2,
};

constexpr uint8_t           ToRaw(ContinueMissionType v)        { return static_cast<uint8_t>(v); }
constexpr ContinueMissionType ToContinueMissionType(int v)      { return static_cast<ContinueMissionType>(static_cast<uint8_t>(v)); }

} // namespace GalaxyEggbert
