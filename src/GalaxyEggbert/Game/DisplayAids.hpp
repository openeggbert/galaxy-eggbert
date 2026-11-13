#pragma once

#include "ObjectDefinitionRegistry.hpp"
#include "WorldRuntime.hpp"

namespace GalaxyEggbert::Game
{
    // Shared by the SceneFrame builder and the pure-logic verifier. An in-flight collected
    // pickup is inactive before its voyage reward arrives, so it disappears immediately.
    [[nodiscard]] inline bool IsXrayTarget(const MobileObjSpec& object) noexcept
    {
        if (!object.active) return false;
        const auto& definition = GetObjectDefinition(object.type);
        return definition.levelExit || definition.semanticKind == ObjectSemanticKind::Collectible ||
               definition.semanticKind == ObjectSemanticKind::Pickup;
    }
}
