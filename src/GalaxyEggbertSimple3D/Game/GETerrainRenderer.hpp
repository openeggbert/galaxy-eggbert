#pragma once

#include <Simple3D/Simple3D.h>
#include "GEWorldRuntime.hpp"
#include <GalaxyEggbert/Worlds/World.hpp>
#include <cstdint>
#include <map>
#include <utility>

namespace GESimple3D {

// Builds Simple3D entity blocks for the voxel world.
// Each tile is one Box.mdl with the tile texture on all 6 faces (no separate Plane entity).
// Fill/edge blocks below cliff edges use a flat dark colour.
class GETerrainRenderer {
public:
    // Destroys all terrain entities via game.DestroyEntity then spawns new ones.
    // Call after loading a world.
    void Build(Simple3D::Game& game, const GEWorldRuntime& world);

    // Destroys all previously spawned terrain entities.
    void Clear(Simple3D::Game& game);

    // Update UV rects for animated tiles (lava, crusher, saw, spike, water, fan, temp).
    // Only does work when animPhase changes from the previous call.
    void Update(int animPhase);

    // Remove a door block at world tile (wx, wz) — destroys its entity and sets World to Air.
    // Returns true if a door entity was found and removed.
    bool OpenDoor(Simple3D::Game& game, int wx, int wz,
                  GalaxyEggbert::Worlds::World* world);

    // Toggle the switch at (wx, wz): flips icon 384↔385, scans ±20 tiles in X at same Z
    // for linked saw tiles (378↔379), updates all entity UVs and World data.
    // Returns true if a switch entity was found.
    bool ToggleSwitch(int wx, int wz, GalaxyEggbert::Worlds::World* world);

    // Set the Box entity UV for tile at (wx, wz).
    // icon >= 0: show and update UV; icon < 0: hide (SetActive false, kills visual+physics).
    // Returns false if position not in tileEntityMap_.
    bool SetTileIcon(int wx, int wz, int icon);

private:
    struct AnimTile {
        Simple3D::Entity* entity;  // Box entity — UV texture updates and SetActive go here
        uint16_t          base;
        bool              active = true;  // false when switch-disabled (stopped saw)
    };

    std::vector<Simple3D::Entity*>               terrainEntities_;
    std::vector<AnimTile>                        animTiles_;
    std::map<std::pair<int,int>, Simple3D::Entity*> doorEntities_;
    std::map<std::pair<int,int>, Simple3D::Entity*> tileEntityMap_;  // Box entity at y=0
    int                                          lastAnimPhase_ = -1;
};

} // namespace GESimple3D
