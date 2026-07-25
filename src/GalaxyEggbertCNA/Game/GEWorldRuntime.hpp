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
    // Y is the center of the occupied voxel cell, matching
    // MoveObjectRecord: a ground object above a Y=0 floor has Y=1.
    struct MobileObjSpec
    {
        ObjectType type;
        // Optional object-m.png tile override carried by .vwr
        // MoveObjectRecord variants such as the secret wooden case.
        // Zero keeps the normal ObjectType/phase-derived icon.
        std::uint16_t visualIcon = 0;
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

        // Real shared patrol-turn timing (2026-07-11, plan.md E3D-MIG-131,
        // `Decor::MoveObjectStepLine` -- see GEInteractionSystem.cpp's
        // AdvancePatrolStep() for the state machine that uses these).
        // Ticks are at the real 20Hz reference rate, same convention as
        // `phase` above. Does NOT apply to platform lifts/crates (types 1/
        // 12/47/48), which keep their own existing speed-based ping-pong
        // patrol in GEInteractionSystem -- these fields are only consumed
        // for every other MoveObject type.
        float stepAdvanceTicks = 60.0f;
        float stepRecedeTicks = 60.0f;
        float timeStopStartTicks = 40.0f;
        float timeStopEndTicks = 40.0f;
        int patrolStep = 1; // 1=dwell@start, 2=advance, 3=dwell@end, 4=recede
        float patrolTime = 0.0f; // ticks elapsed within the current patrolStep
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

        // Re-derives skyRegion_/missionNumber_/mobileObjects_ from
        // whatever is CURRENTLY in the loaded World -- no disk I/O, unlike
        // LoadFromVwrFile() above (plan.md EDITOR-109). Lets the in-game
        // editor's live GetWorldMutable() edits (block AND MoveObject
        // placement/removal alike) show up immediately without a real
        // save-then-reload round trip. Deliberately does NOT touch
        // bigDecor_ or animPhase_/animTimer_. Spawn coordinates ARE
        // re-derived because EDITOR-125 makes them live world metadata.
        // LoadFromVwrFile() itself calls this after loading, so both paths
        // share one implementation.
        void ResyncFromWorld();

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

        // Mutable access for test tooling (2026-07-10, plan.md E3D-MIG-134)
        // -- lets a verification tool carve a small guaranteed-shape test
        // column (e.g. a ledge over a pit) for blupih/blupit's real
        // downward/horizontal raycast, without depending on incidental
        // terrain shape elsewhere in the loaded world. Same mutability
        // precedent as GetMobileObjectsMutable() below.
        [[nodiscard]] Worlds::World& GetWorldMutable() { return *world_; }

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

        // Real fan hazard (plan.md E3D-MIG-149, `Decor::IsVentillo` per
        // Decor.cpp:7667-7752, verified directly against source -- NOT just
        // mobile-eggbert-reference/12-hazards-and-interactables.md's own
        // summary, which mischaracterizes icons 127/128/130/131/133/134/
        // 136/137 as the fan's "air-column/trail tiles": those are actually
        // just the 4 head icons' own idle animation frames
        // (`BlockTypes::tileAnimBase()` already maps them back to their
        // base FanLeft/Right/Up/Down type, so they never appear as a
        // placed block's own stored type). The REAL trail-continuation
        // icons IsVentillo walks/clears are 110/114/118/122 -- part of a
        // wholly separate, not-yet-render-decided "wind-vent particle
        // stream" tile family (icons 110-125, per
        // mobile-eggbert-reference/08-animations.md §6's own "deferred"
        // list) with no `BlockTypes` constant and never placed anywhere in
        // this engine yet. Consequently this only ports the part that IS
        // reachable today: checked one cell ABOVE Blupi's own position
        // (matching `GetBlockTypeAbove()`'s convention, real check is
        // "Blupi's own current tile" but this engine's fan placements sit
        // one cell above the walkable floor, same convention the
        // teleporter already established) -- a no-op (`std::nullopt`)
        // unless that cell is one of the 4 real fan head icons
        // (`BlockTypes::isFan()`). On a match: immediately clears that cell
        // to `Air` (real `ModifDecor(pos, -1)` on the head tile itself,
        // unconditional per the real source) and returns the icon that was
        // there. The real trail-walk beyond the head tile is NOT ported
        // (no confirmed trail tiles exist to walk). Real sub-tile-band
        // gating (only the mouth-facing half of the tile counts) and
        // focus/shield/hide/SuperBlupi immunity are NOT modeled -- no
        // sub-tile position or those buff concepts exist in this engine
        // yet, same simplification already applied to spikes/teleporter/
        // every other hazard this session -- so contact anywhere in the
        // cell is unconditionally lethal (caller's job). No grounded gate
        // (unlike `TryActivateSwitch`) -- the real `IsVentillo` check has
        // none either.
        [[nodiscard]] std::optional<std::uint16_t> TryConsumeFan(float blupiX, float blupiY, float blupiZ);

        // Real teleporter pairing (plan.md E3D-MIG-147, `Decor::
        // SearchTeleporte` per Decor.cpp:7406-7429): a linear scan of the
        // entire grid for the first OTHER cell whose type equals `icon`
        // (330-333), excluding any candidate within `kEntryExclusionRadius`
        // grid units of `blupiX`/`Y`/`Z` (his position when he entered --
        // he's fully frozen for the whole real transit, so this is still
        // his position at completion time too) to skip the entry pillar he
        // just triggered. The real source uses an exact ~0.625-grid-unit/
        // 40-real-px distance threshold from the entry TILE's own position;
        // this uses a slightly larger radius from Blupi's own position
        // instead (he stands one cell below, not exactly at, the entry
        // pillar's own cell -- see `GEBlupiController::GetBlockTypeAbove()`'s
        // own comment), since a plain tile-equality skip would need the
        // entry pillar's own cell coordinates, which this method isn't
        // passed directly; a radius comfortably larger than "one cell away
        // from Blupi" but far smaller than realistic inter-pillar level-
        // design spacing achieves the same real intent. On a match, fills
        // `destX`/`Y`/`Z` with the position Blupi should land at -- one
        // grid cell BELOW the matched pillar in Y (the same real
        // relationship as the entry side: he stands in the open space
        // beneath a teleporter pillar, not inside/on top of it) and offset
        // one cell in +Z from directly beneath it (NOT the matched
        // pillar's exact X/Z) -- landing exactly beneath it would
        // immediately satisfy the same trigger condition again, producing
        // an infinite teleport-back-and-forth ping-pong (confirmed live
        // during this task). Level data must place a walkable cell at that
        // offset, same authoring responsibility the real "keep teleporter
        // icons in matched pairs" warning already implies -- and returns
        // true. Returns false if no other cell shares the icon anywhere in
        // the grid (real: Blupi simply regains control in place, a silent
        // no-op -- the caller doesn't need to do anything different, since
        // his position was never touched during the transit).
        [[nodiscard]] bool FindTeleportDestination(std::uint16_t icon, float blupiX, float blupiY, float blupiZ,
                                                     float& destX, float& destY, float& destZ) const;
        [[nodiscard]] int GetSpawnTileX() const { return spawnTileX_; }
        [[nodiscard]] int GetSpawnTileY() const { return spawnTileY_; }
        [[nodiscard]] int GetSpawnTileZ() const { return spawnTileZ_; }
        [[nodiscard]] bool HasExplicitSpawnPoint() const { return hasExplicitSpawnPoint_; }
        [[nodiscard]] float GetSpawnRenderX() const
        {
            return static_cast<float>(spawnTileX_ - kWorldCenterX);
        }
        [[nodiscard]] float GetSpawnRenderY() const
        {
            return static_cast<float>(spawnTileY_);
        }
        [[nodiscard]] float GetSpawnRenderZ() const
        {
            return static_cast<float>(spawnTileZ_ - kWorldCenterZ);
        }
        [[nodiscard]] int GetSkyRegion() const { return skyRegion_; }
        // Real m_mission (2026-07-13, plan.md HUD-024), a direct pass-
        // through of Worlds::World::missionNumber() -- gates level-specific
        // logic such as the real training-hint overlay. 0 for worlds that
        // never set it (matches the real default for non-tutorial levels).
        [[nodiscard]] int GetMissionNumber() const { return missionNumber_; }
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

        // Real vanishing/Temp tile timing (plan.md E3D-MIG-146, icon 324,
        // `Decor::IsPassIcon`/`IsBlocIcon` per Decor.cpp:7503-7538, verified
        // directly against source): solid 90% of the time, passable
        // (Blupi falls through) only during the last 2 of a raw, un-scaled
        // `m_time`-driven 20-value cycle (`m_time / 4 % 20 >= 18`) -- NOT
        // `Config::ScaleDiv`-normalized, an explicit exception the
        // reference doc calls out (same category as Crusher's own raw
        // `m_time`, see IsCrusherActiveAtPhase's comment). At this
        // project's Fps20 reference rate `Config::ScaleDiv(N) == N`
        // exactly, so raw `m_time` and this class's own 20-ticks/sec
        // `animPhase_` are numerically identical -- unlike Crusher, this
        // is an exact reuse, not an approximation. No per-cell phase
        // offset in the real source, so every Temp tile in a level blinks
        // in perfect lockstep -- this is why a single phase-only, static/
        // pure function (not a per-cell one) is correct here. Consumed by
        // `GEBlupiController::Step()`'s `tempPassable` parameter, not by
        // `GetGroundBlockType()` -- unlike lava/spikes/Blitz/saw/Crusher,
        // this changes whether the tile IS the ground at all (a
        // collision-shape question, not a "what am I standing on" query).
        [[nodiscard]] static bool IsTempPassableAtPhase(int animPhase) noexcept;

        // Real mission-number arithmetic (plan.md hub/mission-progression
        // system, found 2026-07-17 via direct `Decor.cpp`/`Game1.cpp` read):
        // ComputeWorldSelectTarget() is what touching a real hub-screen
        // world-select marker (`BlockTypes::isWorldSelect()`, icons 158-165)
        // computes, contextually reinterpreted by the CURRENT mission
        // (`Decor.cpp`'s `Bye`-action handler): from the global hub
        // (mission==1), marker N selects world N*10; from within a world hub
        // (mission==X0), marker N selects sublevel X0+N. ComputeMissionBack()
        // is the real "go to this mission's own hub" formula, shared by
        // finishing a sublevel (`Win`-action handler, `mission/10*10`) and
        // the real `MissionBack`/`PauseBack` button (`Game1.cpp`): from a
        // hub already (mission%10==0), back goes to the global hub (1);
        // otherwise it's the same `mission/10*10`. Static/pure so a tool can
        // test the arithmetic directly, same reasoning as IsBlitzActiveAtPhase().
        [[nodiscard]] static int ComputeWorldSelectTarget(int currentMission, int selectIndex) noexcept;
        [[nodiscard]] static int ComputeMissionBack(int currentMission) noexcept;

        // Real win-exit formula (found 2026-07-17, `Decor.cpp:6411-6434`,
        // the `BlupiAction::Win` mission handler) -- distinct from
        // ComputeMissionBack() above (which is only the real `else` branch
        // of this same handler, shared with `MissionBack`/`PauseBack`).
        // Real special cases: reaching the GLOBAL HUB's own exit
        // (mission==1) goes to the real final bonus world (199); reaching
        // mission 199's own exit is the true real ending (`m_term=-2`) --
        // this engine has no distinct "game complete" screen, so it's
        // simplified to loop back to the global hub (1) instead of
        // modeling a new terminal state. The real `m_bFoundCle`-gated
        // "found a level's hidden secret -> straight to the global hub"
        // case is a per-level hidden-pickup mechanic, out of scope here
        // (same "not attempted" precedent as every other hidden/secondary
        // mechanic this session) -- every other mission falls through to
        // ComputeMissionBack() exactly as the real `else` branch does.
        [[nodiscard]] static int ComputeWinExitTarget(int currentMission) noexcept;

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
        int spawnTileY_ = 1;
        int spawnTileZ_ = 0;
        bool hasExplicitSpawnPoint_ = false;
        int skyRegion_ = 0;
        int missionNumber_ = 0;
        float animTimer_ = 0.0f;
        int animPhase_ = 0;
    };
}
