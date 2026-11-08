#pragma once

#include "InteractionSystem.hpp"
#include <utility>

namespace GalaxyEggbert::Game
{
// A reusable pickup is hidden during its action, not consumed permanently. Return
// it on completion OR interruption; death must not grant the unfinished power.
struct ReusablePickup
{
    float x = 0, y = 0, z = 0;
    GalaxyEggbert::Def::ObjectType type = GalaxyEggbert::Def::ObjectType::ObjectType0;

    void RestoreIfReady(WorldRuntime& world, InteractionSystem& interaction, bool actionBusy)
    {
        if (actionBusy || type == GalaxyEggbert::Def::ObjectType::ObjectType0) return;
        const auto returnedType = std::exchange(type, GalaxyEggbert::Def::ObjectType::ObjectType0);
        interaction.RespawnPickupItem(world, x, y, z, returnedType);
    }
};
} // namespace GalaxyEggbert::Game
