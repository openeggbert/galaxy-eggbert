#pragma once

#include <Simple3D/Simple3D.h>
#include "GEWorldRuntime.hpp"

namespace GESimple3D {

// Builds Simple3D entity blocks for the voxel world.
//
// S3D-1 limitation: all blocks are grey Box.mdl cubes — no tile textures.
// Tile-atlas UV material is missing from Simple3D (see docs/SIMPLE3D_GAPS.md).
// TODO(S3D-2): Replace with atlas-UV material once Simple3D exposes UV-offset API.
class GETerrainRenderer {
public:
    // Destroys all terrain entities via game.DestroyEntity then spawns new ones.
    // Call after loading a world.
    void Build(Simple3D::Game& game, const GEWorldRuntime& world);

    // Destroys all previously spawned terrain entities.
    void Clear(Simple3D::Game& game);

private:
    std::vector<Simple3D::Entity*> terrainEntities_;
};

} // namespace GESimple3D
