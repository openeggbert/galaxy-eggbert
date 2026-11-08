#pragma once

#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <vector>

namespace GalaxyEggbert::Editor
{
struct LiftEndpointMarker
{
    float x = 0, y = 0, z = 0;
    int icon = 30;
    bool operator==(const LiftEndpointMarker &) const = default;
};
// blupi_recon/src/decblock.cpp: BUILD draws object-m 30 at posStart and 31 at posEnd.
// Coordinates remain world data; the renderer converts X/Z to render space.
[[nodiscard]] inline std::vector<LiftEndpointMarker> BuildLiftEndpointMarkers(
    const std::vector<MoveObjectRecord> &records)
{
    std::vector<LiftEndpointMarker> markers;
    for (const auto &r : records)
    {
        if (Game::GetObjectDefinition(r.type).semanticKind != Game::ObjectSemanticKind::Lift)
            continue;
        markers.push_back({r.posStartX, r.posStartY, r.posStartZ, 30});
        if (r.posStartX != r.posEndX || r.posStartY != r.posEndY || r.posStartZ != r.posEndZ)
            markers.push_back({r.posEndX, r.posEndY, r.posEndZ, 31});
    }
    return markers;
}
} // namespace GalaxyEggbert::Editor
