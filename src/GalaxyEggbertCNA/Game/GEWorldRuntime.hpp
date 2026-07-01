#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <memory>
#include <string>

namespace GalaxyEggbert::CNA
{
    // Minimal mobile-eggbert .txt world-file loader for the CNA/Easy3D target.
    // Parses the tile grid and Blupi spawn point into the engine-agnostic
    // Worlds::World. No MoveObject/object parsing yet (see plan.md Phase 7),
    // and nothing here renders — see plan.md Phase 5/6 for that.
    class GEWorldRuntime
    {
    public:
        static constexpr int kWorldCenterX = 50; // matches GESimple3D::GEWorldRuntime::kWCX
        static constexpr int kWorldCenterZ = 50; // matches GESimple3D::GEWorldRuntime::kWCZ

        GEWorldRuntime();

        // Parses a mobile-eggbert .txt world file into the voxel World.
        // Returns true on success; on failure the world is left empty (all air).
        bool LoadFromMobileEggbertFile(const std::string& path);

        [[nodiscard]] const Worlds::World& GetWorld() const { return *world_; }
        [[nodiscard]] int GetSpawnTileX() const { return spawnTileX_; }
        [[nodiscard]] int GetSpawnTileZ() const { return spawnTileZ_; }
        [[nodiscard]] int GetSkyRegion() const { return skyRegion_; }

    private:
        std::unique_ptr<Worlds::World> world_;
        int spawnTileX_ = 0;
        int spawnTileZ_ = 0;
        int skyRegion_ = 0;
    };
}
