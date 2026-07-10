#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>
#include <GalaxyEggbert/def/ObjectType.hpp>

#include <cstdint>
#include <memory>
#include <optional>
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

        // Per-instance animation tick counter (2026-07-09, NEXT.md §3),
        // advanced by GEWorldRuntime::Update() at mobile-eggbert's original
        // 20fps reference tick rate (Config::ScaleTime(1)==1 at that rate,
        // Decor.cpp/Config.hpp) so GetObjIcon()'s existing phase-indexed
        // formulas -- most of them already written anticipating this, per
        // their own "no per-instance animation timers exist yet" comments
        // -- finally animate instead of being frozen at phase=0.
        float phase = 0.0f;

        // Live state (2026-07-10, interactive object system -- see
        // GEInteractionSystem). current{X,Y,Z} start equal to posStart and
        // move independently for patrolling platforms (ObjectType1/47/48)
        // and pushed crates (ObjectType12); rendering uses these, not
        // posStart, so movement is actually visible. direction is the
        // patrol direction (+1 = heading toward posEnd, -1 = heading back
        // toward posStart), matching GalaxyEggbertSimple3D's own
        // GEDecorSystem::ObjState::direction. active is false once a
        // one-shot pickup (egg/key) has been collected -- posStart/posEnd
        // stay untouched either way, since they're the object's real path
        // bounds, not its current position.
        float currentX = 0.0f, currentY = 0.0f, currentZ = 0.0f;
        float direction = 1.0f;
        bool active = true;
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
        // `.vwr` binary format (plan.md E3D-MIG-058). Since header v2
        // (2026-07-09, NEXT.md §3), the format carries a real skyRegion
        // field (Worlds::World::skyRegion(), see World Format.md) -- unlike
        // LoadFromMobileEggbertFile()'s region= parsing, this is a genuine
        // binary header field, not text. Spawn point still has no `.vwr`
        // equivalent and resets to 0. Returns true on success; on failure
        // the world is left empty (all air).
        bool LoadFromVwrFile(const std::string& path);

        // Advances the animated-tile raw tick (20 fps, matching
        // mobile-eggbert's real Config::ScaleTime(1) reference rate -- fixed
        // 2026-07-09 from an incorrect flat 6 fps shared by every tile type;
        // GETerrainRenderer::AnimIcon() now divides this raw tick by each
        // tile type's own real per-type divisor, see Update()'s .cpp
        // comment) and every MobileObjSpec's per-instance phase (same 20 fps
        // base rate, mobile-eggbert's original reference rate for
        // MoveObject animation).
        void Update(float dt);

        [[nodiscard]] const Worlds::World& GetWorld() const { return *world_; }

        // Real switch/saw linking (plan.md E3D-MIG-142, `Decor::ActiveSwitch`
        // per mobile-eggbert-reference/12-hazards-and-interactables.md,
        // verified directly against Decor.cpp:7131-7148). Call on an
        // edge-detected action-button press while Blupi is grounded; a
        // no-op (returns `std::nullopt`) unless he's standing directly on a
        // switch tile (`BlockTypes::Switch`/`SwitchOff`). On an actual
        // toggle: flips the switch tile's own block between the two (real
        // behavior: always toggles to the opposite of its current state,
        // this call itself has no separate "which way" parameter), then
        // scans a fixed 41-cell window (this switch's X ±20, same Y and Z
        // -- BlockTypes.hpp's own `isSwitch()` comment already documents
        // this exact real mapping) toggling every matching Saw/SawStopped
        // block it finds to match the new switch state. Returns the new
        // switch state (`true` = now on/`Switch`, `false` = now off/
        // `SwitchOff`) so the caller can play the matching real sound
        // (channel 77 "on"/76 "off") -- this method has no `GESound&`
        // parameter, same separation as `GEInteractionSystem`'s `LoseLife()`.
        std::optional<bool> TryActivateSwitch(float blupiX, float blupiY, float blupiZ,
                                               bool blupiOnGround);
        [[nodiscard]] int GetSpawnTileX() const { return spawnTileX_; }
        [[nodiscard]] int GetSpawnTileZ() const { return spawnTileZ_; }
        [[nodiscard]] int GetSkyRegion() const { return skyRegion_; }
        // Raw 20fps animation tick (2026-07-09) -- NOT a ready-to-index
        // frame number; GETerrainRenderer::AnimIcon() divides it by each
        // tile type's own real tick divisor before indexing that type's
        // frame table. See Update()'s comment for why.
        [[nodiscard]] int GetAnimPhase() const { return animPhase_; }

        // Real Blitz hazard timing (plan.md E3D-MIG-144, `Decor::BlitzActif`
        // per mobile-eggbert-reference/12-hazards-and-interactables.md): a
        // 100-tick cycle at this class's own 20-ticks/sec reference rate
        // (see Update()'s animPhase_ comment) -- lethal only on even ticks
        // within the first half of the cycle (a ~2.5s flicker at 25% duty
        // cycle), then fully inactive for the second half. Static/pure
        // (only needs an animPhase_ value, not a live instance) so a tool
        // can test the cycle math directly -- see
        // tools/VerifyInteractionSystem.cpp.
        [[nodiscard]] static bool IsBlitzActiveAtPhase(int animPhase) noexcept;

        // Real Crusher hazard timing (plan.md E3D-MIG-143, `Decor::
        // IsEcraseur` per mobile-eggbert-reference/12-hazards-and-
        // interactables.md, verified directly against Decor.cpp:7277-7288:
        // `m_time/3 % 10 <= 2`). The real check runs on `m_time` (a raw,
        // NOT `Config::ScaleDiv`-normalized frame counter -- an explicit
        // exception the reference doc calls out, unlike every other timer)
        // rather than this class's 20-ticks/sec `animPhase_` -- approximated
        // here by reusing `animPhase_` for the same divisor shape (still a
        // ~30% duty cycle), since the real unnormalized rate has no clean
        // equivalent at a fixed reference tick rate. Static/pure, same
        // reasoning as IsBlitzActiveAtPhase() above.
        [[nodiscard]] static bool IsCrusherActiveAtPhase(int animPhase) noexcept;

        // BigDecor: is a second 100x100 background tile layer in
        // mobile-eggbert level files (see mobile-eggbert-2d-reference.md
        // §2.3) — parsed and stored here (same icon-id-to-block-type
        // conversion as the main grid), but not yet rendered anywhere.
        // Recommended 3D treatment: Billboard, confirmed non-colliding — see
        // 15-3d-render-mapping-design.md §9.2. Row-major, [row*100 + col].
        // Empty when loaded from a `.vwr` file (that format has no BigDecor
        // concept).
        [[nodiscard]] const std::vector<std::uint16_t>& GetBigDecor() const { return bigDecor_; }

        // MoveObject records (pickups, enemies, platform lifts, crates) --
        // from a mobile-eggbert .txt file's MoveObject: lines, OR from a
        // hand-authored `.vwr` world's embedded GalaxyEggbert::MoveObjectRecord
        // entries (2026-07-09, see MoveObjectRecord.hpp -- unlike BigDecor:,
        // MoveObjects ARE representable directly in the 3D format itself, via
        // Worlds::World's block-extra-metadata mechanism). Rendered as
        // Billboard by default, UniformCube for platform lifts + crates --
        // see 15-3d-render-mapping-design.md §5 and
        // GEObjectIcons::IsUniformCubeObject.
        [[nodiscard]] const std::vector<MobileObjSpec>& GetMobileObjects() const { return mobileObjects_; }

        // Mutable access for GEInteractionSystem (2026-07-10) -- patrol
        // movement, crate push, and pickup collection all need to modify
        // live object state in place every frame.
        [[nodiscard]] std::vector<MobileObjSpec>& GetMobileObjectsMutable() { return mobileObjects_; }

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
