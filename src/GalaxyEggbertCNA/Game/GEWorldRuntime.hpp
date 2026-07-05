#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>
#include <GalaxyEggbert/def/ObjectType.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // One moving/interactive object loaded from a mobile-eggbert MoveObject
    // line — mirrors GESimple3D::MobileObjSpec (galaxy-eggbert's own,
    // already-approved parsing logic), kept engine-agnostic here (plain
    // floats, not a CNA/XNA Vector3) to match this class's existing style.
    // No renderer consumes this list yet (see 15-3d-render-mapping-design.md
    // for the planned Billboard/UniformCube treatment per type).
    struct MobileObjSpec
    {
        ObjectType type;
        float posStartX = 0.0f, posStartY = 0.0f, posStartZ = 0.0f;
        float posEndX = 0.0f, posEndY = 0.0f, posEndZ = 0.0f;
        float speed = 1.5f;
    };

    // Minimal mobile-eggbert .txt world-file loader for the CNA/Easy3D target.
    // Parses the tile grid, Blupi spawn point, and MoveObject records into
    // the engine-agnostic Worlds::World / mobileObjects_ list. Nothing here
    // renders yet — see plan.md Phase 5/6 and
    // 15-3d-render-mapping-design.md for the planned renderer.
    class GEWorldRuntime
    {
    public:
        static constexpr int kWorldCenterX = 50; // matches GESimple3D::GEWorldRuntime::kWCX
        static constexpr int kWorldCenterZ = 50; // matches GESimple3D::GEWorldRuntime::kWCZ

        GEWorldRuntime();

        // Parses a mobile-eggbert .txt world file into the voxel World.
        // Returns true on success; on failure the world is left empty (all air).
        // Kept as a reference/secondary path — see plan.md E3D-MIG-058;
        // LoadFromVwrFile() is the default for hand-authored 3D worlds.
        bool LoadFromMobileEggbertFile(const std::string& path);

        // Loads a genuinely 3D, hand-authored world from the engine-agnostic
        // `.vwr` binary format (plan.md E3D-MIG-058). Unlike
        // LoadFromMobileEggbertFile(), the format carries no spawn/sky-region
        // header, so those reset to 0. Returns true on success; on failure
        // the world is left empty (all air).
        bool LoadFromVwrFile(const std::string& path);

        // Advances the animated-tile clock. 6 fps tick, matching Simple3D's
        // GEWorldRuntime::Update() and mobile-eggbert's animated tile rate.
        void Update(float dt);

        [[nodiscard]] const Worlds::World& GetWorld() const { return *world_; }
        [[nodiscard]] int GetSpawnTileX() const { return spawnTileX_; }
        [[nodiscard]] int GetSpawnTileZ() const { return spawnTileZ_; }
        [[nodiscard]] int GetSkyRegion() const { return skyRegion_; }
        [[nodiscard]] int GetAnimPhase() const { return animPhase_; }

        // BigDecor: is a second 100x100 background tile layer in
        // mobile-eggbert level files (see mobile-eggbert-2d-reference.md
        // §2.3) — parsed and stored here (same icon-id-to-block-type
        // conversion as the main grid), but not yet rendered anywhere.
        // Recommended 3D treatment: Billboard, confirmed non-colliding — see
        // 15-3d-render-mapping-design.md §9.2. Row-major, [row*100 + col].
        // Empty when loaded from a `.vwr` file (that format has no BigDecor
        // concept).
        [[nodiscard]] const std::vector<std::uint16_t>& GetBigDecor() const { return bigDecor_; }

        // MoveObject: records (pickups, enemies, effects) parsed from a
        // mobile-eggbert .txt file — not yet rendered anywhere. Recommended
        // treatment per ObjectType: Billboard by default, UniformCube for
        // platform lifts + crates — see 15-3d-render-mapping-design.md §5.
        // Empty when loaded from a `.vwr` file (hand-authored 3D worlds have
        // no MoveObject concept yet).
        [[nodiscard]] const std::vector<MobileObjSpec>& GetMobileObjects() const { return mobileObjects_; }

    private:
        std::unique_ptr<Worlds::World> world_;
        std::vector<std::uint16_t> bigDecor_;
        std::vector<MobileObjSpec> mobileObjects_;
        int spawnTileX_ = 0;
        int spawnTileZ_ = 0;
        int skyRegion_ = 0;
        float animTimer_ = 0.0f;
        int animPhase_ = 0;
    };
}
