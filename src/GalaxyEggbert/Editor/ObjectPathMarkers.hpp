#pragma once

#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <vector>

namespace GalaxyEggbert::Editor
{
struct ObjectPathMarker
{
    float x = 0, y = 0, z = 0;
    int icon = 30;
    bool operator==(const ObjectPathMarker &) const = default;
};
// blupi_recon/games/sb2/src/decblock.cpp: BUILD draws object-m 30 at posStart and 31 at posEnd.
// The 3D editor uses the same endpoint cubes for every authored object route, including
// fish and moving mines, whose special motion handlers do not set patrolMotion.
// Coordinates remain world data; the renderer converts X/Z to render space.
[[nodiscard]] inline std::vector<ObjectPathMarker> BuildObjectPathMarkers(
    const std::vector<MoveObjectRecord> &records)
{
    std::vector<ObjectPathMarker> markers;
    for (const auto &r : records)
    {
        const auto& d=Game::GetObjectDefinition(r.type);
        const bool moves=r.posStartX!=r.posEndX || r.posStartY!=r.posEndY || r.posStartZ!=r.posEndZ;
        if (d.semanticKind!=Game::ObjectSemanticKind::Lift && d.patrolTicksPerCell==0 && !(d.patrolMotion && moves))
            continue;
        markers.push_back({r.posStartX, r.posStartY, r.posStartZ, 30});
        if (r.posStartX != r.posEndX || r.posStartY != r.posEndY || r.posStartZ != r.posEndZ)
            markers.push_back({r.posEndX, r.posEndY, r.posEndZ, 31});
    }
    return markers;
}
} // namespace GalaxyEggbert::Editor
