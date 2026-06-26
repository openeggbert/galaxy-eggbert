#pragma once

#include <Simple3D/Simple3D.h>
#include "GEWorldRuntime.hpp"

namespace GESimple3D {

// Builds Simple3D entity blocks for the voxel world.
//
// Top face of each cube is textured with the tile's UV region from object-m.png.
// Fill/edge blocks below cliff edges use a flat dark colour.
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
