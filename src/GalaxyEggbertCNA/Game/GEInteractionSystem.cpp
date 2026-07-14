#include "GEInteractionSystem.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        bool IsPlatformLift(ObjectType t)
        {
            return t == ObjectType::ObjectType1 || t == ObjectType::ObjectType47 ||
                   t == ObjectType::ObjectType48;
        }

        bool IsCrate(ObjectType t)
        {
            return t == ObjectType::ObjectType12;
        }

        // Opens a door tile (plan.md E3D-MIG-160/162, real `Decor::OpenDoor`,
        // ~11667): removes the tile from the terrain grid (becomes passable)
        // and spawns a transient ObjectType22 at that cell (real
        // `table_bridge`-style slide, handled by this file's own
        // ObjectType22 branch in the main Update() loop, not the generic
        // patrol system) so it visually slides up and out of the way
        // instead of just vanishing. Real channel 33. Shared by both the
        // key-gated (160) and treasure-gated (162) door families --
        // opening itself is identical, only the trigger condition differs.
        // The door's own icon is NOT carried onto the slide object --
        // `GEObjectIcons::GetObjIcon()` already has no confirmed icon data
        // for type 22 (returns 0 regardless), a pre-existing gap unrelated
        // to this task.
        void OpenDoorAt(GEWorldRuntime& worldRuntime, int gx, int gy, int gz, GESound& sound)
        {
            worldRuntime.GetWorldMutable().setBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                      static_cast<std::uint16_t>(gz),
                                                      Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));

            MobileObjSpec slide;
            slide.type = ObjectType::ObjectType22;
            slide.posStartX = slide.posEndX = slide.currentX =
                static_cast<float>(gx) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
            slide.posStartY = slide.posEndY = slide.currentY = static_cast<float>(gy);
            slide.posStartZ = slide.posEndZ = slide.currentZ =
                static_cast<float>(gz) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
            slide.phase = 0.0f;
            slide.active = true;

            auto& objects = worldRuntime.GetMobileObjectsMutable();
            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = slide;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(slide);
            }
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel33);
        }

        // Real Decor::OpenDoorsTresor (~11642): scans the whole grid,
        // opening every treasure-gated door (icon 421+N) whose
        // requirement N is now met. Shared by the real per-pickup
        // trigger (Update()'s own treasureDoorScanNeeded) and
        // GEInteractionSystem::CheatAllTreasure() below, which needs the
        // exact same scan after crediting every treasure at once.
        void ScanAndOpenTreasureDoors(GEWorldRuntime& worldRuntime, int treasuresCollected, GESound& sound)
        {
            auto& terrain = worldRuntime.GetWorldMutable();
            const int axis = static_cast<int>(terrain.blocksPerAxis());
            for (int gx = 0; gx < axis; ++gx)
            {
                for (int gy = 0; gy < axis; ++gy)
                {
                    for (int gz = 0; gz < axis; ++gz)
                    {
                        const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                            static_cast<std::uint16_t>(gz))
                                               .type();
                        if (icon >= 421 && icon <= 420 + treasuresCollected)
                        {
                            OpenDoorAt(worldRuntime, gx, gy, gz, sound);
                        }
                    }
                }
            }
        }

        // Real shared patrol-turn mechanic (plan.md E3D-MIG-131,
        // `Decor::MoveObjectStepLine`, verified directly against
        // Decor.cpp:8005-8141): a 4-phase cycle -- 1=dwell at posStart for
        // timeStopStartTicks, 2=advance to posEnd over stepAdvanceTicks,
        // 3=dwell at posEnd for timeStopEndTicks, 4=recede back over
        // stepRecedeTicks, then loop to 1. The real position update is
        // linear interpolation on the elapsed-time fraction (not per-tick
        // accumulation), so it can't drift -- reproduced here with a
        // normalized `t` in [0,1] rather than the real integer pixel math,
        // same effect. Ticks are at the 20Hz reference rate (same
        // convention as MobileObjSpec::phase). Applies to every MoveObject
        // EXCEPT platform lifts/crates, which keep their own existing
        // speed-based ping-pong patrol (see the two branches above) --
        // callers gate that exclusion, not this function. A no-op if
        // posStart==posEnd is the caller's responsibility too (matches the
        // real guard exactly -- see Update()'s call site).
        constexpr float kTicksPerSecond = 20.0f;

        void AdvancePatrolStep(MobileObjSpec& obj, float dt)
        {
            const float dtTicks = dt * kTicksPerSecond;
            switch (obj.patrolStep)
            {
                case 1: // dwell at posStart
                    obj.patrolTime += dtTicks;
                    if (obj.patrolTime >= obj.timeStopStartTicks)
                    {
                        obj.patrolStep = 2;
                        obj.patrolTime = 0.0f;
                    }
                    break;
                case 2: // advance posStart -> posEnd
                {
                    obj.patrolTime += dtTicks;
                    const float t = (obj.stepAdvanceTicks > 0.0f)
                        ? std::min(obj.patrolTime / obj.stepAdvanceTicks, 1.0f) : 1.0f;
                    obj.currentX = obj.posStartX + (obj.posEndX - obj.posStartX) * t;
                    obj.currentY = obj.posStartY + (obj.posEndY - obj.posStartY) * t;
                    obj.currentZ = obj.posStartZ + (obj.posEndZ - obj.posStartZ) * t;
                    if (t >= 1.0f)
                    {
                        // Real per-type special case at this exact junction
                        // (Decor.cpp:8095-8098): a fired projectile
                        // (ObjectType23) does NOT dwell at posEnd like every
                        // other MoveObject -- it self-destructs the instant
                        // it reaches the end of its pre-computed clear path
                        // (real: type reset to ObjectType0; here: active=false,
                        // this class's existing "destroyed" convention).
                        if (obj.type == ObjectType::ObjectType23)
                        {
                            obj.active = false;
                        }
                        else
                        {
                            obj.patrolStep = 3;
                            obj.patrolTime = 0.0f;
                        }
                    }
                    break;
                }
                case 3: // dwell at posEnd
                    obj.patrolTime += dtTicks;
                    if (obj.patrolTime >= obj.timeStopEndTicks)
                    {
                        obj.patrolStep = 4;
                        obj.patrolTime = 0.0f;
                    }
                    break;
                case 4: // recede posEnd -> posStart
                {
                    obj.patrolTime += dtTicks;
                    const float t = (obj.stepRecedeTicks > 0.0f)
                        ? std::min(obj.patrolTime / obj.stepRecedeTicks, 1.0f) : 1.0f;
                    obj.currentX = obj.posEndX + (obj.posStartX - obj.posEndX) * t;
                    obj.currentY = obj.posEndY + (obj.posStartY - obj.posEndY) * t;
                    obj.currentZ = obj.posEndZ + (obj.posStartZ - obj.posEndZ) * t;
                    if (t >= 1.0f)
                    {
                        obj.patrolStep = 1;
                        obj.patrolTime = 0.0f;
                    }
                    break;
                }
                default:
                    obj.patrolStep = 1;
                    obj.patrolTime = 0.0f;
                    break;
            }
        }

        // The real shared kill list (Decor.cpp:5782-5816, verified directly
        // against source for this task): ObjectType2/3 (generic patrol
        // hazards), 4 (bulldozer), 16 (spider), 17 (fish), 20 (bird), 96/97
        // (follower, both dormant and awake) all use the exact same contact-
        // death check -- the only difference in that source block is purely
        // cosmetic (17/20 get a bigger screen-shake + a different explosion
        // ObjectType than the rest), not a behavioral difference in whether
        // or how Blupi dies, so this deliberately does NOT split them into
        // separate per-type checks. Real per-type quirks that ARE modeled:
        // type3's duck-immunity (see the blupiCrouching check below). Real
        // per-type quirks NOT modeled: type17/20's bigger explosion effect
        // (cosmetic), type2's wider "thrown object" anticipation box and
        // taunt-suppression (cosmetic/reaction polish), type16's always-
        // self-destroys/no-turn-table framing (already true here since
        // every hazard here is destroyed on contact), follower 96/97's real
        // homing-toward-Blupi movement AI (a separate, NOT-yet-implemented
        // feature from this contact-death check -- an un-homing follower
        // still correctly kills Blupi on contact if he touches it).
        bool IsGenericHazard(ObjectType t)
        {
            switch (t)
            {
                case ObjectType::ObjectType2:
                case ObjectType::ObjectType3:
                case ObjectType::ObjectType4:
                case ObjectType::ObjectType16:
                case ObjectType::ObjectType17:
                case ObjectType::ObjectType20:
                case ObjectType::ObjectType96:
                case ObjectType::ObjectType97:
                    return true;
                default:
                    return false;
            }
        }

        // Real balloon-pop subset (Decor.cpp:5766-5781): exactly types
        // 3/16/96/97, NOT the full 8-type IsGenericHazard() list -- 2/4/17/20
        // still kill Blupi even while ballooned, per the real source's
        // if/else-if chain (the pop check comes first and is mutually
        // exclusive with the kill check; only these 4 types are ever
        // eligible for the pop branch at all).
        bool IsBalloonPoppableHazard(ObjectType t)
        {
            switch (t)
            {
                case ObjectType::ObjectType3:
                case ObjectType::ObjectType16:
                case ObjectType::ObjectType96:
                case ObjectType::ObjectType97:
                    return true;
                default:
                    return false;
            }
        }

        // True once during the frame `prevTicks` crosses `threshold` --
        // real mobile-eggbert fires blupih/blupit's shots on an exact tick
        // equality (`time == Config::ScaleTime(N)`), which a continuously-
        // accumulated float can step past without ever equaling; this is
        // the same "fires exactly once" edge-detection shape used
        // elsewhere in this codebase for tick-gated events.
        bool CrossedTick(float prevTicks, float dtTicks, float threshold)
        {
            return prevTicks < threshold && (prevTicks + dtTicks) >= threshold;
        }

        // Real ObjectStart's SearchDistRight-driven travel distance
        // (Decor.cpp:7794-7869, "@note" comment): casts from
        // (gx,gy,gz) one cell at a time in direction (dx,dy,dz), counting
        // consecutive AIR cells until the first solid cell or the world
        // edge. Returns 0 if the very first stepped-to cell is already
        // solid (the real "num3==0" case) -- the caller cancels the shot
        // (no visible projectile) but, per the real ObjectStart code, still
        // plays the attack sound (its `!= -1` sound gate only checks for a
        // free object-pool slot, never whether the raycast found room to
        // travel -- `ObjectStart` returns a valid, non -1 index even on
        // this early-cancel path). One grid cell here == one real 64px
        // mobile-eggbert tile, matching this file's existing world<->grid
        // conversion convention (see GEWorldRuntime's kMobileTileSize use).
        int SearchAirDistance(const Worlds::World& world, int gx, int gy, int gz, int dx, int dy, int dz)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            int x = gx;
            int y = gy;
            int z = gz;
            int dist = 0;
            while (true)
            {
                x += dx;
                y += dy;
                z += dz;
                if (x < 0 || x >= blocksPerAxis || y < 0 || y >= blocksPerAxis || z < 0 || z >= blocksPerAxis)
                {
                    break;
                }
                if (!world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                     static_cast<std::uint16_t>(z)).isAir())
                {
                    break;
                }
                ++dist;
            }
            return dist;
        }

        // Converts a live object's world-space position to a clamped grid
        // cell, matching GEBlupiController's own kWorldCenterX/Z + lround
        // convention exactly (duplicated here rather than exposed from
        // GEBlupiController, which keeps its own version private).
        void ToGridCell(const Worlds::World& world, float wx, float wy, float wz, int& gx, int& gy, int& gz)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            gx = std::clamp(static_cast<int>(std::lround(wx)) + GEWorldRuntime::kWorldCenterX, 0, blocksPerAxis - 1);
            gy = std::clamp(static_cast<int>(std::lround(wy)), 0, blocksPerAxis - 1);
            gz = std::clamp(static_cast<int>(std::lround(wz)) + GEWorldRuntime::kWorldCenterZ, 0, blocksPerAxis - 1);
        }

        // Matches GEBlupiController::IsSolidAt's own logic exactly
        // (duplicated here rather than exposed from GEBlupiController,
        // which keeps it private -- same reason ToGridCell above is its
        // own copy rather than a shared call).
        bool IsSolidAt(const Worlds::World& world, int gx, int gy, int gz)
        {
            return !world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                    static_cast<std::uint16_t>(gz)).isAir();
        }

        // Spawns a fired-projectile MobileObjSpec (ObjectType23) travelling
        // `dist` grid cells from `obj`'s own position in direction
        // (dirX,dirY,dirZ) (a unit axis vector) -- shared by blupih (straight
        // down) and blupit (horizontal) below. stepAdvanceTicks reuses the
        // real ObjectStart formula directly (Decor.cpp:7866:
        // `ScaleTime(abs(speed*dist/64))`, speed magnitude 5 for both real
        // callers here): since one grid cell already equals one real 64px
        // tile, `dist` (in cells) IS the real formula's `dist/64` term, so
        // `5.0f * dist` ticks is a direct, non-approximated transcription,
        // not an invented pacing constant. patrolStep starts at 2 (advance),
        // skipping the dwell entirely, matching real ObjectStart's own
        // `step=2; time=0` initialization (a fresh shot never dwells at its
        // spawn point).
        MobileObjSpec MakeBullet(const MobileObjSpec& obj, float dirX, float dirY, float dirZ, int dist)
        {
            MobileObjSpec bullet;
            bullet.type = ObjectType::ObjectType23;
            bullet.posStartX = bullet.currentX = obj.currentX;
            bullet.posStartY = bullet.currentY = obj.currentY;
            bullet.posStartZ = bullet.currentZ = obj.currentZ;
            bullet.posEndX = obj.currentX + dirX * static_cast<float>(dist);
            bullet.posEndY = obj.currentY + dirY * static_cast<float>(dist);
            bullet.posEndZ = obj.currentZ + dirZ * static_cast<float>(dist);
            bullet.patrolStep = 2;
            bullet.patrolTime = 0.0f;
            bullet.stepAdvanceTicks = 5.0f * static_cast<float>(dist);
            return bullet;
        }

        // Treasure/collectible sparkle burst (plan.md VISUAL-012,
        // ObjectType39) -- confirmed via a direct source read
        // (`Decor.cpp:5948-5960`, the real ObjectType5/treasure-collect
        // site): 4 `ObjectStart(pos, ObjectType39, speed)` calls
        // (`speed ∈ {-60,60,10,-10}`, encoding up/down/+X/-X) at the
        // collected object's own position, no pre-offset -- same real
        // `SearchDistRight()` short-circuit as Invert's burst
        // (`Decor.cpp:7628-7653`, flat 500 real-px for this ObjectType
        // too), same 64px-per-tile conversion, same real screen-Y-to-
        // world-Y sign flip. Appends directly to pendingSpawns since,
        // unlike Invert's grant/expiry (fired from the game class after
        // this class's own Update() has already returned), the real
        // treasure-collect site is INSIDE this class's own per-object
        // loop, so the existing deferred-spawn flush at the end of
        // Update() already covers it.
        void AppendSparkleBurst(float x, float y, float z, std::vector<MobileObjSpec>& pendingSpawns)
        {
            // Real behavior (`Decor::ObjectStart()`, same as the Invert
            // burst's own updated comment) -- spawns exactly AT the
            // collected object's position (no pre-offset, matching
            // Invert's GRANT shape) and slides toward a 500px-out posEnd
            // over 78 ticks, self-deleting at phase>=11 (see below) long
            // before actually arriving.
            constexpr float kReach = 500.0f / 64.0f;
            const float dirs[4][3] = {
                {0.0f, 1.0f, 0.0f},  // up
                {0.0f, -1.0f, 0.0f}, // down
                {1.0f, 0.0f, 0.0f},  // +X (real "right")
                {-1.0f, 0.0f, 0.0f}, // -X (real "left")
            };
            for (const auto& dir : dirs)
            {
                MobileObjSpec spec;
                spec.type = ObjectType::ObjectType39;
                spec.active = true;
                spec.phase = 0.0f;
                spec.currentX = spec.posStartX = x;
                spec.currentY = spec.posStartY = y;
                spec.currentZ = spec.posStartZ = z;
                spec.posEndX = x + dir[0] * kReach;
                spec.posEndY = y + dir[1] * kReach;
                spec.posEndZ = z + dir[2] * kReach;
                pendingSpawns.push_back(spec);
            }
        }

        // Dynamite-blast explosion flash (plan.md VISUAL-008,
        // ObjectType8) -- a single instance spawned exactly at the given
        // blast-center position, no offset (real `ObjectStart(posStart,
        // ObjectType8, 0)`, `Decor.cpp:9065`, the real
        // `Decor::DynamiteStart()` called once per blast in the 9-blast
        // sequence -- speed=0 means no direction/offset encoding, same
        // reasoning as `SpawnFanHitFlash()`). Appends directly to
        // pendingSpawns since, like the treasure sparkle above, the real
        // spawn site is INSIDE this class's own per-object loop (the
        // dynamite-fuse ObjectType56 block).
        void AppendDynamiteBlastFlash(float x, float y, float z, std::vector<MobileObjSpec>& pendingSpawns)
        {
            MobileObjSpec spec;
            spec.type = ObjectType::ObjectType8;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = spec.posEndX = x;
            spec.currentY = spec.posStartY = spec.posEndY = y;
            spec.currentZ = spec.posStartZ = spec.posEndZ = z;
            pendingSpawns.push_back(spec);
        }

        // Blupih (ObjectType32, plan.md E3D-MIG-134) attack: verified
        // directly against Decor.cpp:8878-8886 -- during a turn-dwell
        // (step 1 or 3), at dwell-frame 21 exactly, drops one ObjectType23
        // straight down via `ObjectStart(pos, ObjectType23, 55)` (the real
        // speed encoding's >50 branch always means "straight down",
        // magnitude 55-50=5 -- see SearchAirDistance's own comment for why
        // aiming at Blupi is never modeled: it isn't real behavior).
        void FireBlupihShot(const MobileObjSpec& obj, const Worlds::World& world, GESound& sound,
                             std::vector<MobileObjSpec>& pendingSpawns)
        {
            int gx, gy, gz;
            ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, gx, gy, gz);
            const int dist = SearchAirDistance(world, gx, gy, gz, 0, -1, 0);
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel52);
            if (dist > 0)
            {
                pendingSpawns.push_back(MakeBullet(obj, 0.0f, -1.0f, 0.0f, dist));
            }
        }

        // Blupit (ObjectType33, plan.md E3D-MIG-134) attack: verified
        // directly against Decor.cpp:8928-8969 -- fires two horizontal
        // shots per turn-dwell bracketing the turn, one at dwell-frame 3,
        // one at dwell-frame 21 (see the two CrossedTick() call sites in
        // Update() below, which pick `dirXSign` for each per the real
        // if/else condition and pass it here). Just the raycast+spawn+sound
        // half of the real ObjectStart call, mirroring FireBlupihShot's
        // shape for the horizontal case.
        void FireBlupitShot(const MobileObjSpec& obj, float dirXSign, const Worlds::World& world,
                             GESound& sound, std::vector<MobileObjSpec>& pendingSpawns)
        {
            int gx, gy, gz;
            ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, gx, gy, gz);
            const int dirSign = (dirXSign > 0.0f) ? 1 : -1;
            const int dist = SearchAirDistance(world, gx, gy, gz, dirSign, 0, 0);
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel52);
            if (dist > 0)
            {
                pendingSpawns.push_back(MakeBullet(obj, dirXSign, 0.0f, 0.0f, dist));
            }
        }

        // Matches GalaxyEggbertSimple3D's GEDecorSystem (AddTriggerSphere(0.7f)).
        constexpr float kPickupRadius = 0.7f;
        constexpr float kMaxEggCount = 10; // real mobile-eggbert MAX_EGG_COUNT (Decor.cpp:96)
        // The real contact hitbox is a tile-based rectangle overlap
        // (MoveObjectDetect), not a radius -- kPickupRadius is reused here
        // as the closest existing documented approximation, same
        // simplification already applied to every pickup type above.
        constexpr float kHazardContactRadius = kPickupRadius;

        // Real `BlupiElectro` aura (plan.md `068`, Decor.cpp:9610-9638,
        // mobile-eggbert-reference/10-blupi-mechanics.md §9): while
        // `m_blupiCloud` (this engine's SecretPower::Cloud) is active,
        // instantly destroys small enemies within 40px of Blupi's own
        // box -- an offensive aura Blupi carries, unrelated to the
        // `Blitz` lightning HAZARD despite the similarly-named real
        // function. Real check expands Blupi's own box by 40px; modeled
        // as a circular radius (same simplification as every other
        // proximity test in this file) combining that 40px real
        // expansion with a 32px half-tile (Blupi's own box is measured
        // from its center here, not its edge) -- same "half-tile + real
        // px offset, /64" combination already used for
        // kFollowerWakeRadius below.
        constexpr float kCloudAuraRadius = (40.0f + 32.0f) / 64.0f;

        // Follower wake box (ObjectType96, plan.md E3D-MIG-137, real
        // Decor.cpp:9646-9678 MoveObjectFollow): the real check is an
        // axis-aligned rect test (the follower's own tile padded +-100px
        // on all sides vs. a narrow ~28px-wide strip through Blupi's
        // default hitbox) -- approximated here as a circular distance
        // check, same simplification as every other proximity test in
        // this file. 100px real padding + a 32px half-tile (the real rect
        // is measured from the follower's TILE edges, not its center)
        // converts to grid units via the same 64px/cell convention used
        // throughout this file (SearchAirDistance etc.).
        constexpr float kFollowerWakeRadius = (100.0f + 32.0f) / 64.0f;

        // Real follower homing speed (ObjectType97, Decor.cpp:8025-8044):
        // exactly 1 real px/tick, independently per axis (a Chebyshev-
        // style step-toward, NOT a normalized diagonal), at the same 20Hz
        // reference tick rate MoveObjectStepLine itself runs at (matching
        // AdvancePatrolStep's own convention) -- a direct, non-
        // approximated transcription via the same 64px/cell grid
        // conversion used throughout this file.
        constexpr float kFollowerHomingSpeed = 20.0f / 64.0f;
    }

    void GEInteractionSystem::Update(float dt, GEWorldRuntime& worldRuntime,
                                      float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                                      GESound& sound, bool blupiCrouching, bool blupiBallooned,
                                      int blupiFacingDX, int blupiFacingDZ, bool blupiInvincible,
                                      bool blupiCanGrantShield, bool blupiCanGrantPower,
                                      bool blupiCanGrantCloud, bool blupiCanGrantHide,
                                      bool blupiFirePressed, bool blupiCanFire, bool blupiCloudActive,
                                      bool blupiCanGrantInvert)
    {
        diedThisFrame_ = false;
        balloonTouchedThisFrame_ = false;
        balloonPoppedThisFrame_ = false;
        shieldGrantedThisFrame_ = false;
        powerGrantedThisFrame_ = false;
        cloudGrantedThisFrame_ = false;
        hideGrantedThisFrame_ = false;
        invertGrantedThisFrame_ = false;
        smallShakeTriggeredThisFrame_ = false;
        bigShakeTriggeredThisFrame_ = false;
        ridingLift_ = false;
        bool treasureDoorScanNeeded = false;
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        const Worlds::World& world = worldRuntime.GetWorld();

        // Platform lift riding (plan.md E3D-MIG-152) -- detect which lift
        // (if any) Blupi is standing on BEFORE it takes this frame's patrol
        // step below, using its position as of the end of last frame.
        // Remembered by pointer (this loop never resizes `objects`, only
        // mutates elements in place, so the pointer stays valid for the
        // rest of this call) so the platform-lift branch below can compute
        // the exact same lift's own displacement once it moves.
        MobileObjSpec* riddenLift = nullptr;
        for (auto& candidate : objects)
        {
            if (!candidate.active || !IsPlatformLift(candidate.type))
            {
                continue;
            }
            const float liftStandY = candidate.currentY + 2.0f;
            if (std::fabs(blupiX - candidate.currentX) < 0.5f && std::fabs(blupiZ - candidate.currentZ) < 0.5f &&
                std::fabs(blupiY - liftStandY) < 0.2f)
            {
                riddenLift = &candidate;
                break;
            }
        }
        const float riddenLiftOldX = riddenLift ? riddenLift->currentX : 0.0f;
        const float riddenLiftOldZ = riddenLift ? riddenLift->currentZ : 0.0f;

        if (totalTreasures_ < 0)
        {
            totalTreasures_ = 0;
            for (const auto& obj : objects)
            {
                if (obj.type == ObjectType::ObjectType5)
                {
                    ++totalTreasures_;
                }
            }
        }

        bool touchingExitThisFrame = false;
        // Fired projectiles (ObjectType23) spawned by blupih/blupit this
        // frame -- collected here rather than appended to `objects`
        // directly, since this loop holds references into that same
        // vector and a mid-loop push_back could reallocate and invalidate
        // them. Flushed into `objects` (reusing an inactive slot first,
        // matching the real fixed-pool MoveObjectFree()'s slot-reuse
        // semantics rather than growing unboundedly) after the loop below.
        std::vector<MobileObjSpec> pendingSpawns;

        for (auto& obj : objects)
        {
            if (!obj.active)
            {
                continue;
            }

            // Real `BlupiElectro` aura (plan.md `068`) -- checked before
            // every other per-object branch so a Type4 enemy (also in
            // IsGenericHazard's own list) is destroyed by the aura rather
            // than also killing Blupi via hazard contact the same frame.
            if (blupiCloudActive &&
                (obj.type == ObjectType::ObjectType4 || obj.type == ObjectType::ObjectType32 ||
                 obj.type == ObjectType::ObjectType33))
            {
                const float adx = obj.currentX - blupiX;
                const float ady = obj.currentY - blupiY;
                const float adz = obj.currentZ - blupiZ;
                if (adx * adx + ady * ady + adz * adz < kCloudAuraRadius * kCloudAuraRadius)
                {
                    obj.active = false;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel59);
                    continue;
                }
            }

            // Invert start/stop particle burst (plan.md VISUAL-014/015,
            // ObjectType41/42) -- purely cosmetic, no interaction with
            // Blupi. Real self-delete at phase>=16 (`Decor.cpp:8575-8596`,
            // `Config::ScaleTime(16)==16` at this build's 20Hz reference
            // rate) -- `phase` itself is already advanced generically by
            // `GEWorldRuntime::Update()` (called every frame before this
            // one), so this only needs to check it, not increment it.
            // Real `stepAdvance` (`Decor::ObjectStart()`, `Decor.cpp:7866`)
            // is 78 ticks for every direction of this burst (the same
            // 500px-reach/magnitude-10 combination in all 4 cases,
            // fixed 2026-07-14 -- this object always self-deletes at 16/78
            // ticks, ~20% of the way, long before reaching posEnd, so no
            // arrival/dwell handling is needed).
            if (obj.type == ObjectType::ObjectType41 || obj.type == ObjectType::ObjectType42)
            {
                constexpr float kStepAdvance = 78.0f;
                const float t = obj.phase / kStepAdvance;
                obj.currentX = obj.posStartX + (obj.posEndX - obj.posStartX) * t;
                obj.currentY = obj.posStartY + (obj.posEndY - obj.posStartY) * t;
                obj.currentZ = obj.posStartZ + (obj.posEndZ - obj.posStartZ) * t;
                if (obj.phase >= 16.0f)
                {
                    obj.active = false;
                }
                continue;
            }

            // Treasure/collectible sparkle burst (plan.md VISUAL-012,
            // ObjectType39) -- purely cosmetic. Real self-delete at
            // phase>=11 (`Decor.cpp:8382-8389`, `Config::ScaleTime(11)==11`
            // at this build's 20Hz reference rate) -- an 11-frame lifetime,
            // shorter than Invert's 16. Same real `stepAdvance`=78 slide
            // toward posEnd as the Invert burst above (fixed 2026-07-14) --
            // self-deletes at 11/78 ticks, ~14% of the way.
            if (obj.type == ObjectType::ObjectType39)
            {
                constexpr float kStepAdvance = 78.0f;
                const float t = obj.phase / kStepAdvance;
                obj.currentX = obj.posStartX + (obj.posEndX - obj.posStartX) * t;
                obj.currentY = obj.posStartY + (obj.posEndY - obj.posStartY) * t;
                obj.currentZ = obj.posStartZ + (obj.posEndZ - obj.posStartZ) * t;
                if (obj.phase >= 11.0f)
                {
                    obj.active = false;
                }
                continue;
            }

            // Fan-hit shockwave flash (plan.md CAM-009-adjacent,
            // ObjectType11) -- purely cosmetic, completes the real Fan-hit
            // effect alongside the already-wired BigShake camera shake.
            // Real self-delete at phase>=9 (`Decor.cpp:8431-8440`,
            // `Config::ScaleTime(9)==9` at this build's 20Hz reference
            // rate) -- a 9-frame lifetime.
            if (obj.type == ObjectType::ObjectType11)
            {
                if (obj.phase >= 9.0f)
                {
                    obj.active = false;
                }
                continue;
            }

            // Dynamite-blast explosion flash (plan.md VISUAL-008,
            // ObjectType8) -- purely cosmetic. Real self-delete at
            // phase>=39 (`Decor.cpp:8397-8399`,
            // `Tables::table_explo1Length==39`, `Config::ScaleDiv(1)==1`
            // at this build's 20Hz reference rate) -- the longest of the 4
            // particle-effect lifetimes modeled so far.
            if (obj.type == ObjectType::ObjectType8)
            {
                if (obj.phase >= 39.0f)
                {
                    obj.active = false;
                }
                continue;
            }

            // Platform lift patrol (ObjectType1/47/48): ping-pong between
            // posStart and posEnd at `speed` units/sec -- matches
            // GalaxyEggbertSimple3D's GEDecorSystem::Update() exactly
            // (same target-select/distance-flip/move-toward-target shape).
            if (IsPlatformLift(obj.type))
            {
                const float targetX = (obj.direction > 0.0f) ? obj.posEndX : obj.posStartX;
                const float targetY = (obj.direction > 0.0f) ? obj.posEndY : obj.posStartY;
                const float targetZ = (obj.direction > 0.0f) ? obj.posEndZ : obj.posStartZ;
                const float dx = targetX - obj.currentX;
                const float dy = targetY - obj.currentY;
                const float dz = targetZ - obj.currentZ;
                const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dist < 0.05f)
                {
                    obj.direction = -obj.direction;
                }
                else
                {
                    const float step = obj.speed * dt / std::max(dist, 0.001f);
                    obj.currentX += dx * step;
                    obj.currentY += dy * step;
                    obj.currentZ += dz * step;
                }

                // Platform lift riding (plan.md E3D-MIG-152): this is the
                // exact lift Blupi was standing on before the patrol step
                // just above -- report its own displacement this tick (a
                // delta, not an absolute position) plus the real constant
                // conveyor nudge for types 47/48 (kConveyorNudgeSpeed is an
                // approximation, plan.md E3D-MIG-154: the real 2px/tick has
                // no exact unit-conversion established for this engine's
                // grid scale), and the new absolute stand height (a snap,
                // matching the real source's own "correct Y drift every
                // frame" approach). The caller applies these via
                // GEBlupiController::RideLift().
                if (&obj == riddenLift)
                {
                    constexpr float kConveyorNudgeSpeed = 0.3f;
                    float nudgeX = 0.0f;
                    if (obj.type == ObjectType::ObjectType47) nudgeX = kConveyorNudgeSpeed * dt;
                    else if (obj.type == ObjectType::ObjectType48) nudgeX = -kConveyorNudgeSpeed * dt;

                    ridingLift_ = true;
                    rideDeltaX_ = (obj.currentX - riddenLiftOldX) + nudgeX;
                    rideDeltaZ_ = obj.currentZ - riddenLiftOldZ;
                    rideStandY_ = obj.currentY + 2.0f;
                }
                continue;
            }

            // Crate push (ObjectType12) -- faithful to mobile-eggbert
            // TestPushCaisse (see GalaxyEggbertSimple3D's GEDecorSystem.cpp,
            // whose own comment cites the real function name). X-axis only,
            // matching that reference exactly (mobile-eggbert is a 2D side-
            // scroller; X is its only horizontal movement axis, so a
            // Z-axis push was never a real mechanic to begin with).
            if (IsCrate(obj.type))
            {
                const float relX = obj.currentX - blupiX;
                const float relZ = obj.currentZ - blupiZ;
                const float distX = std::fabs(relX);
                const float distZ = std::fabs(relZ);
                if (distZ < 0.6f && distX > 0.3f && distX < 1.1f)
                {
                    const float pushDir = (relX > 0.0f) ? 1.0f : -1.0f;
                    if (blupiMoveDX * pushDir > 0.01f)
                    {
                        // Linked crates (plan.md E3D-MIG-150, real
                        // Decor::SearchLinkCaisse ~9397): flood-fill outward
                        // from the seed (pushed) crate, adding any other
                        // crate whose box touches an already-linked one (1
                        // grid unit in X or Y, same Z column -- this
                        // engine's crates only ever occupy a single Z per
                        // real mobile-eggbert's own X/Y-only geometry),
                        // restricted to crates AT OR ABOVE the seed's own
                        // row (Y) -- "you push the stack, not the floor it
                        // rests on", matching the real restriction exactly.
                        std::vector<MobileObjSpec*> linked;
                        linked.push_back(&obj);
                        bool addedAny = true;
                        while (addedAny)
                        {
                            addedAny = false;
                            for (auto& other : objects)
                            {
                                if (!other.active || !IsCrate(other.type)) continue;
                                if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                if (other.currentY < obj.currentY - 0.5f) continue;
                                if (std::fabs(other.currentZ - obj.currentZ) > 0.5f) continue;
                                bool touches = false;
                                for (auto* member : linked)
                                {
                                    if (std::fabs(other.currentX - member->currentX) < 1.5f &&
                                        std::fabs(other.currentY - member->currentY) < 1.5f)
                                    {
                                        touches = true;
                                        break;
                                    }
                                }
                                if (touches)
                                {
                                    linked.push_back(&other);
                                    addedAny = true;
                                }
                            }
                        }

                        // TestPushCaisse (~9321): every linked member must
                        // clear at its own proposed destination -- floor
                        // support (real: two lower-corner edge strips) is
                        // only re-checked for members at the seed's own row
                        // (the row being actively pushed); members stacked
                        // ABOVE that row skip the floor test entirely and
                        // only need the general non-collision check, same
                        // as the real source. A stack moves atomically:
                        // any one member blocked cancels the whole push.
                        bool allClear = true;
                        for (auto* member : linked)
                        {
                            const float destX = member->currentX + pushDir;
                            const int wx = static_cast<int>(std::round(destX)) + GEWorldRuntime::kWorldCenterX;
                            const int wz = static_cast<int>(std::round(member->currentZ)) + GEWorldRuntime::kWorldCenterZ;
                            const int axis = static_cast<int>(world.blocksPerAxis());
                            if (wx < 0 || wx >= axis || wz < 0 || wz >= axis)
                            {
                                allClear = false;
                                break;
                            }

                            const bool isSeedRow = std::fabs(member->currentY - obj.currentY) < 0.5f;
                            if (isSeedRow)
                            {
                                const bool hasFloor = !world.getBlock(static_cast<std::uint16_t>(wx), 0,
                                                                       static_cast<std::uint16_t>(wz))
                                                            .isAir();
                                if (!hasFloor)
                                {
                                    allClear = false;
                                    break;
                                }
                            }

                            bool occupied = false;
                            for (const auto& other : objects)
                            {
                                if (!other.active || !IsCrate(other.type)) continue;
                                if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                if (std::fabs(other.currentX - destX) < 0.5f &&
                                    std::fabs(other.currentY - member->currentY) < 0.5f &&
                                    std::fabs(other.currentZ - member->currentZ) < 0.5f)
                                {
                                    occupied = true;
                                    break;
                                }
                            }
                            if (occupied)
                            {
                                allClear = false;
                                break;
                            }
                        }

                        if (allClear)
                        {
                            for (auto* member : linked)
                            {
                                member->currentX += pushDir;
                            }
                        }
                    }
                }
                continue;
            }

            // Door-opening slide effect (ObjectType22, plan.md E3D-MIG-160,
            // real `Decor::OpenDoor` ~11667): slides up by exactly one grid
            // unit over the real `Config::ScaleTime(50)` = 50 ticks (2.5s
            // at the 20Hz reference rate obj.phase already advances at),
            // then self-destructs -- a one-shot animation, not the generic
            // dwell/advance/dwell/recede patrol (which loops and doesn't
            // fit a "play once and vanish" effect), so it's handled
            // directly here instead.
            if (obj.type == ObjectType::ObjectType22)
            {
                constexpr float kSlideTicks = 50.0f;
                const float t = std::min(obj.phase / kSlideTicks, 1.0f);
                obj.currentY = obj.posStartY + t;
                if (obj.phase >= kSlideTicks)
                {
                    obj.active = false;
                }
                continue;
            }

            // Dynamite fuse (ObjectType56, plan.md E3D-MIG-155) -- real
            // 9-blast sequence at fixed ticks (Decor.cpp ~8252-8296),
            // verified directly against the source, not just the reference
            // doc's rounded "~200px/~100px" summary: tick 50 is the center
            // blast (dx,dy)=(0,0); 53=(-100,8); 55=(80,10); 56=(-15,-100);
            // 59=(20,70); 62=(30,-50); 64=(-40,30); 67=(-180,10); 69=
            // (200,-10) -- real pixel offsets, /64 to this engine's grid
            // units (real dx -> this engine's X, real dy -> this engine's Y,
            // same X/Y-only 2D-source convention already used for blupih/
            // blupit/follower; Z is always 0, no real source axis maps to
            // it). obj.phase already advances at the real 20Hz reference
            // rate (GEWorldRuntime::Update()); reconstructing the phase as
            // of the START of this frame (`prevPhase`) lets each blast fire
            // on the exact frame its tick is crossed, not every frame after.
            if (obj.type == ObjectType::ObjectType56)
            {
                struct Blast { float tick, dx, dy; };
                static constexpr Blast kBlasts[] = {
                    {50.0f, 0.0f / 64.0f, 0.0f / 64.0f},
                    {53.0f, -100.0f / 64.0f, 8.0f / 64.0f},
                    {55.0f, 80.0f / 64.0f, 10.0f / 64.0f},
                    {56.0f, -15.0f / 64.0f, -100.0f / 64.0f},
                    {59.0f, 20.0f / 64.0f, 70.0f / 64.0f},
                    {62.0f, 30.0f / 64.0f, -50.0f / 64.0f},
                    {64.0f, -40.0f / 64.0f, 30.0f / 64.0f},
                    {67.0f, -180.0f / 64.0f, 10.0f / 64.0f},
                    {69.0f, 200.0f / 64.0f, -10.0f / 64.0f},
                };
                const float prevPhase = obj.phase - dt * 20.0f;
                for (const auto& blast : kBlasts)
                {
                    if (prevPhase < blast.tick && obj.phase >= blast.tick)
                    {
                        const float centerX = obj.currentX + blast.dx;
                        const float centerY = obj.currentY + blast.dy;
                        const float centerZ = obj.currentZ;

                        // Real dynamite-blast explosion flash (plan.md
                        // VISUAL-008, ObjectType8, Decor.cpp:9065) -- one
                        // instance per blast, at its own center, not just
                        // the central (dx=0,dy=0) one.
                        AppendDynamiteBlastFlash(centerX, centerY, centerZ, pendingSpawns);

                        if (blast.dx == 0.0f && blast.dy == 0.0f)
                        {
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
                            // Real camera shake (plan.md CAM-008,
                            // Decor::DynamiteStart(), Decor.cpp:9068-9070,
                            // confirmed via direct source read): SmallShake
                            // fires ONLY for the center blast (dx=0,dy=0),
                            // not for every peripheral tick in the 9-blast
                            // sequence.
                            smallShakeTriggeredThisFrame_ = true;
                        }

                        // Real 128x128px (2x2 tile) area -- +-1 grid unit
                        // in X/Y around the blast center, same Z.
                        constexpr float kBlastHalfExtent = 1.0f;

                        // Clear destructible hazard tiles by icon (real:
                        // saws 378/379, drip hazards 404/410 -- the latter
                        // has no placeable BlockTypes constant in this
                        // engine yet, so only Saw/SawStopped are handled).
                        const int wx0 = static_cast<int>(std::round(centerX - kBlastHalfExtent)) +
                                        GEWorldRuntime::kWorldCenterX;
                        const int wx1 = static_cast<int>(std::round(centerX + kBlastHalfExtent)) +
                                        GEWorldRuntime::kWorldCenterX;
                        const int wz = static_cast<int>(std::round(centerZ)) + GEWorldRuntime::kWorldCenterZ;
                        const int wy = static_cast<int>(std::round(centerY)) < 0
                                           ? 0
                                           : static_cast<int>(std::round(centerY));
                        const int axis = static_cast<int>(world.blocksPerAxis());
                        if (wy >= 0 && wy < axis && wz >= 0 && wz < axis)
                        {
                            for (int wx = std::max(0, wx0); wx <= std::min(axis - 1, wx1); ++wx)
                            {
                                const auto blockType = world.getBlock(static_cast<std::uint16_t>(wx),
                                                                       static_cast<std::uint16_t>(wy),
                                                                       static_cast<std::uint16_t>(wz))
                                                            .type();
                                if (blockType == GalaxyEggbert::BlockTypes::Saw ||
                                    blockType == GalaxyEggbert::BlockTypes::SawStopped)
                                {
                                    worldRuntime.GetWorldMutable().setBlock(
                                        static_cast<std::uint16_t>(wx), static_cast<std::uint16_t>(wy),
                                        static_cast<std::uint16_t>(wz),
                                        Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));
                                }
                            }
                        }

                        // Destroy every enemy/crate/object overlapping the
                        // blast rect, from the real explicit type list
                        // (Decor.cpp ~9102-9132, ported exactly, not
                        // approximated) -- crates are destroyed as a full
                        // linked group (real SearchLinkCaisse), everything
                        // else is a plain deactivate. The blast flash itself
                        // is spawned above; no further per-victim debris
                        // visual is modeled (real source has none either --
                        // ObjectType8 above is the only cosmetic effect at
                        // this site).
                        for (auto& victim : objects)
                        {
                            if (!victim.active || &victim == &obj)
                            {
                                continue;
                            }
                            const bool isDestructibleType =
                                victim.type == ObjectType::ObjectType2 || victim.type == ObjectType::ObjectType3 ||
                                victim.type == ObjectType::ObjectType4 || victim.type == ObjectType::ObjectType6 ||
                                victim.type == ObjectType::ObjectType12 || victim.type == ObjectType::ObjectType13 ||
                                victim.type == ObjectType::ObjectType16 || victim.type == ObjectType::ObjectType17 ||
                                victim.type == ObjectType::ObjectType18 || victim.type == ObjectType::ObjectType19 ||
                                victim.type == ObjectType::ObjectType20 || victim.type == ObjectType::ObjectType24 ||
                                victim.type == ObjectType::ObjectType25 || victim.type == ObjectType::ObjectType26 ||
                                victim.type == ObjectType::ObjectType28 || victim.type == ObjectType::ObjectType30 ||
                                victim.type == ObjectType::ObjectType32 || victim.type == ObjectType::ObjectType33 ||
                                victim.type == ObjectType::ObjectType34 || victim.type == ObjectType::ObjectType40 ||
                                victim.type == ObjectType::ObjectType44 || victim.type == ObjectType::ObjectType46 ||
                                victim.type == ObjectType::ObjectType52 || victim.type == ObjectType::ObjectType54 ||
                                victim.type == ObjectType::ObjectType96 || victim.type == ObjectType::ObjectType97 ||
                                victim.type == ObjectType::ObjectType200 || victim.type == ObjectType::ObjectType201 ||
                                victim.type == ObjectType::ObjectType202 || victim.type == ObjectType::ObjectType203;
                            if (!isDestructibleType)
                            {
                                continue;
                            }
                            if (std::fabs(victim.currentX - centerX) > kBlastHalfExtent + 0.5f ||
                                std::fabs(victim.currentY - centerY) > kBlastHalfExtent + 0.5f ||
                                std::fabs(victim.currentZ - centerZ) > 0.5f)
                            {
                                continue;
                            }
                            if (IsCrate(victim.type))
                            {
                                std::vector<MobileObjSpec*> linked;
                                linked.push_back(&victim);
                                bool addedAny = true;
                                while (addedAny)
                                {
                                    addedAny = false;
                                    for (auto& other : objects)
                                    {
                                        if (!other.active || !IsCrate(other.type)) continue;
                                        if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                        bool touches = false;
                                        for (auto* member : linked)
                                        {
                                            if (std::fabs(other.currentX - member->currentX) < 1.5f &&
                                                std::fabs(other.currentY - member->currentY) < 1.5f &&
                                                std::fabs(other.currentZ - member->currentZ) < 0.5f)
                                            {
                                                touches = true;
                                                break;
                                            }
                                        }
                                        if (touches)
                                        {
                                            linked.push_back(&other);
                                            addedAny = true;
                                        }
                                    }
                                }
                                for (auto* member : linked)
                                {
                                    member->active = false;
                                }
                            }
                            else
                            {
                                victim.active = false;
                            }
                        }

                        // Blupi death check (real: `m_blupiFocus && !shield
                        // && !hide && !superblupi`) -- Shield/Hide (plan.md
                        // E3D-MIG-170, blupiInvincible) modeled now; focus/
                        // superBlupi don't exist in this engine yet.
                        if (!blupiInvincible && std::fabs(blupiX - centerX) <= kBlastHalfExtent + 0.5f &&
                            std::fabs(blupiY - centerY) <= kBlastHalfExtent + 0.5f &&
                            std::fabs(blupiZ - centerZ) <= 0.5f)
                        {
                            LoseLife();
                            diedThisFrame_ = true;
                        }
                    }
                }
                if (obj.phase >= 70.0f)
                {
                    obj.active = false;
                }
                continue;
            }

            // Bridge construction (ObjectType52, plan.md PICKUP-064, real
            // Decor.cpp ~8540-8562): each tick writes the real
            // `table_bridge` icon at that phase BOTH to this object's own
            // sprite AND directly into the live terrain grid cell at its
            // spawn position -- ground collision genuinely toggles across
            // the sequence (icons 367-372 and the -1 "no tile" sentinel are
            // non-solid per real `table_decor_quart`/this engine's own
            // `BlockTypes::isMobileTransparent()`, already confirmed to
            // agree). Real per-mobile-eggbert's exact frame-by-frame
            // `table_bridge` array is not transcribed here (no pre-approved
            // in-repo source for it, unlike blupi's own tables) -- this
            // instead reproduces the DOCUMENTED aggregate shape from
            // mobile-eggbert-reference/14-crates-lifts-bridges-effects.md:
            // 28 ticks ascending through icons 365-372, 112 ticks holding
            // the -1 sentinel, 17 ticks descending back through 372..365
            // ending at the original 364 -- with this engine's own
            // reasonable uniform pacing within each of those 3 documented
            // windows, not a copied per-tick table.
            if (obj.type == ObjectType::ObjectType52)
            {
                constexpr float kAscendTicks = 28.0f;
                constexpr float kHoldTicks = 112.0f;
                constexpr float kDescendTicks = 17.0f;
                constexpr float kTotalTicks = kAscendTicks + kHoldTicks + kDescendTicks; // 157
                constexpr float kProgressSoundTick = 137.0f;

                const float prevPhase = obj.phase - dt * 20.0f;
                if (prevPhase < kProgressSoundTick && obj.phase >= kProgressSoundTick)
                {
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel73);
                }

                int icon;
                if (obj.phase < kAscendTicks)
                {
                    const int step = static_cast<int>(obj.phase * 8.0f / kAscendTicks);
                    icon = 365 + std::clamp(step, 0, 7);
                }
                else if (obj.phase < kAscendTicks + kHoldTicks)
                {
                    icon = -1; // real "no tile" sentinel -- fromMobileIconId() maps this to Air
                }
                else
                {
                    const float descendPhase = obj.phase - (kAscendTicks + kHoldTicks);
                    const int step = static_cast<int>(descendPhase * 9.0f / kDescendTicks);
                    icon = 372 - std::clamp(step, 0, 8); // 372 down to 364
                }

                const int gx = static_cast<int>(std::lround(obj.currentX)) + GEWorldRuntime::kWorldCenterX;
                const int gz = static_cast<int>(std::lround(obj.currentZ)) + GEWorldRuntime::kWorldCenterZ;
                const int gy = static_cast<int>(std::lround(obj.currentY)) - 1;
                const int axis = static_cast<int>(world.blocksPerAxis());
                if (gx >= 0 && gx < axis && gy >= 0 && gy < axis && gz >= 0 && gz < axis)
                {
                    worldRuntime.GetWorldMutable().setBlock(
                        static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                        static_cast<std::uint16_t>(gz),
                        Worlds::Block::make(GalaxyEggbert::BlockTypes::fromMobileIconId(icon)));
                }

                if (obj.phase >= kTotalTicks)
                {
                    obj.active = false;
                }
                continue;
            }

            // Follower wake-up (ObjectType96 -> 97, plan.md E3D-MIG-137,
            // real Decor.cpp:9646-9678 `MoveObjectFollow`) -- a dormant
            // follower promotes to the homing type the instant Blupi comes
            // within its padded detection box, playing its wake sound
            // (real channel 92) exactly once at the transition. Does NOT
            // `continue` -- a follower that wakes this exact frame also
            // takes its first homing step this same frame, below.
            if (obj.type == ObjectType::ObjectType96)
            {
                const float fwdx = obj.currentX - blupiX;
                const float fwdy = obj.currentY - blupiY;
                const float fwdz = obj.currentZ - blupiZ;
                if (fwdx * fwdx + fwdy * fwdy + fwdz * fwdz < kFollowerWakeRadius * kFollowerWakeRadius)
                {
                    obj.type = ObjectType::ObjectType97;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel92);
                }
            }

            // Follower homing (ObjectType97, plan.md E3D-MIG-137, real
            // Decor.cpp:8025-8064) -- once awake, steps X and Y
            // independently (Chebyshev-style, NOT a normalized diagonal)
            // toward Blupi's live position every frame at
            // kFollowerHomingSpeed. currentZ is deliberately left
            // untouched -- real mobile-eggbert has no Z axis at all, only
            // X (horizontal) and Y (vertical) exist in the source this is
            // ported from, matching blupih/blupit's own shots, which never
            // touch Z either. Collapses posStart/posEnd to the new
            // position every successful step (matching the real source's
            // own `posStart = posEnd = end`), which naturally makes the
            // generic patrol-turn block below a no-op for this object (its
            // own `posStart != posEnd` guard), exactly like the real
            // source's generic dwell/advance/dwell/recede block becomes a
            // no-op immediately after this same real function's homing
            // branch runs -- no separate type exclusion needed.
            //
            // Blocked-path self-destruct (real: `TestPath` fails ->
            // `ObjectDelete` + a real `ObjectType9` explosion + channel 10)
            // is modeled as a single-point solid check at the destination
            // cell via `GEBlupiController::IsSolidAt` (this file's own
            // existing tile-grid convention, e.g. `SearchAirDistance`) --
            // real `TestPath` sweeps a rectangle, not a point, same
            // simplification as every other collision check in this file.
            // The cosmetic `ObjectType9` debris object is deliberately NOT
            // spawned here -- a particle-effects system now exists (Invert
            // burst, treasure sparkle, Fan-hit/dynamite-blast flash, all
            // 2026-07-14), but `ObjectType9`'s own `GetObjIcon()` formula
            // is still unaudited/unfixed (real `table_explo2` has `-1`
            // blank-frame sentinels the renderer doesn't handle yet, see
            // `GEObjectIcons.cpp`) -- a real, scoped follow-up, not done
            // here; only the death itself and its sound are ported.
            // `continue`s on self-destruct so an already-destroyed
            // follower can't also register a contact-kill against Blupi
            // this same frame via the generic hazard check below; falls
            // through (no `continue`) on a successful step, since that
            // check is exactly how a follower that catches up to Blupi is
            // supposed to kill him.
            if (obj.type == ObjectType::ObjectType97)
            {
                float endX = obj.currentX;
                float endY = obj.currentY;
                const float stepAmount = kFollowerHomingSpeed * dt;
                if (endX < blupiX) endX = std::min(endX + stepAmount, blupiX);
                else if (endX > blupiX) endX = std::max(endX - stepAmount, blupiX);
                if (endY < blupiY) endY = std::min(endY + stepAmount, blupiY);
                else if (endY > blupiY) endY = std::max(endY - stepAmount, blupiY);

                int fgx, fgy, fgz;
                ToGridCell(world, endX, endY, obj.currentZ, fgx, fgy, fgz);
                if (IsSolidAt(world, fgx, fgy, fgz))
                {
                    obj.active = false;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
                    continue;
                }
                obj.currentX = obj.posStartX = obj.posEndX = endX;
                obj.currentY = obj.posStartY = obj.posEndY = endY;
                obj.posStartZ = obj.posEndZ = obj.currentZ;
            }

            // Real shared patrol-turn mechanic (plan.md E3D-MIG-131) --
            // every remaining MoveObject type (enemies, pickups, etc., NOT
            // lifts/crates handled above). Does NOT `continue`: the wasp/
            // hazard/pickup contact checks below must see this frame's
            // freshly-updated currentX/Y/Z, not last frame's position. A
            // no-op if posStart==posEnd, matching the real guard exactly
            // (stationary objects never advance patrolStep/patrolTime).
            if (obj.posStartX != obj.posEndX || obj.posStartY != obj.posEndY || obj.posStartZ != obj.posEndZ)
            {
                const int prevPatrolStep = obj.patrolStep;
                const float prevPatrolTime = obj.patrolTime;
                AdvancePatrolStep(obj, dt);

                // Stationary shooters (ObjectType32 blupih / 33 blupit,
                // plan.md E3D-MIG-134) -- attack timing is gated on the
                // dwell step (1 or 3) and exact dwell-frame BEFORE this
                // frame's AdvancePatrolStep() call, using CrossedTick() on
                // the pre-call patrolTime -- not the post-call value, which
                // may already belong to a different patrolStep if the
                // dwell happened to end this same frame.
                if (prevPatrolStep == 1 || prevPatrolStep == 3)
                {
                    const float dtTicks = dt * kTicksPerSecond;
                    if (obj.type == ObjectType::ObjectType32 && CrossedTick(prevPatrolTime, dtTicks, 21.0f))
                    {
                        FireBlupihShot(obj, world, sound, pendingSpawns);
                    }
                    else if (obj.type == ObjectType::ObjectType33)
                    {
                        const bool aboutToWalkRight =
                            (obj.posStartX < obj.posEndX && prevPatrolStep == 1) ||
                            (obj.posStartX > obj.posEndX && prevPatrolStep == 3);
                        if (CrossedTick(prevPatrolTime, dtTicks, 3.0f))
                        {
                            FireBlupitShot(obj, aboutToWalkRight ? -1.0f : 1.0f, world, sound, pendingSpawns);
                        }
                        if (CrossedTick(prevPatrolTime, dtTicks, 21.0f))
                        {
                            FireBlupitShot(obj, aboutToWalkRight ? 1.0f : -1.0f, world, sound, pendingSpawns);
                        }
                    }
                }
            }

            // Blupih/blupit's own body (ObjectType32/33) is deliberately
            // NOT in IsGenericHazard() -- real mobile-eggbert never treats
            // walking into their body as a damage path, only their fired
            // projectile is harmful (see FireBlupihShot/FireBlupitShot
            // above). Falls through to the final pickup-type filter below,
            // which already excludes them (no separate `continue` needed).

            // Fired projectile contact (ObjectType23, plan.md E3D-MIG-134):
            // verified directly against Decor.cpp:5914-5947 -- always fatal
            // (real Glu/glue-style death) and destroys the bullet itself.
            // Real Shield/Hide immunity IS modeled now (plan.md E3D-MIG-170,
            // blupiInvincible); win-and-death-action/superBlupi gates are
            // NOT modeled -- neither concept exists in this engine yet.
            // Real death has no distinct sound call of
            // its own (StartSploutchGlu only spawns silent splash-effect
            // debris) -- channel 74 reused here for consistency with this
            // class's existing hazard-death sound approximation.
            if (obj.type == ObjectType::ObjectType23)
            {
                const float bdx = obj.currentX - blupiX;
                const float bdy = obj.currentY - blupiY;
                const float bdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && bdx * bdx + bdy * bdy + bdz * bdz < kHazardContactRadius * kHazardContactRadius)
                {
                    obj.active = false;
                    LoseLife();
                    diedThisFrame_ = true;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                }
                continue;
            }

            // Wasp (ObjectType44, plan.md E3D-MIG-135) -- does NOT kill or
            // destroy itself; contact signals BalloonTouchedThisFrame() so
            // the caller can attempt GEBlupiController::TriggerBalloon()
            // (idempotent there, not here -- see the class comment). Real
            // gate also includes `!m_blupiShield && !m_blupiHide`
            // (Decor.cpp:5826, plan.md E3D-MIG-170) -- a shielded/hidden
            // Blupi doesn't get ballooned at all.
            if (obj.type == ObjectType::ObjectType44)
            {
                const float wdx = obj.currentX - blupiX;
                const float wdy = obj.currentY - blupiY;
                const float wdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && wdx * wdx + wdy * wdy + wdz * wdz < kHazardContactRadius * kHazardContactRadius)
                {
                    balloonTouchedThisFrame_ = true;
                }
                continue;
            }

            // Large creature (ObjectType54, plan.md E3D-MIG-136) -- verified
            // directly against Decor.cpp:5867-5913: contact is lethal ONLY
            // while the creature is paused mid-turn (patrolStep 1 or 3, the
            // real `step != 2 && step != 4` gate) -- unlike every hazard
            // above, walking into it while it is actually mid-walk is
            // completely safe. Real immunity is
            // `!m_blupiBalloon && !m_blupiShield && !m_blupiHide &&
            // !m_bSuperBlupi` (plus a `m_blupiFocus` gate); balloon AND
            // Shield/Hide (plan.md E3D-MIG-170, blupiInvincible) are both
            // modeled now -- only superBlupi/focus don't exist in this
            // engine yet -- unlike the 4 balloon-
            // POPPABLE types, contact while ballooned does nothing at all
            // here (matches the real source: `!m_blupiBalloon` gates the
            // whole branch, there's no separate pop path for type 54). The
            // creature itself is never destroyed by this contact (no
            // `ObjectDelete` in the real branch, unlike the shared kill-list
            // types below) -- it always survives to keep guarding. Real
            // contact also destroys Blupi's current vehicle instead of
            // killing him outright if he's riding one (channel 10 +
            // SmallShake) -- NOT modeled, no vehicle concept exists yet, so
            // contact always takes the real no-vehicle branch instead
            // (channel 51, Decor.cpp:5905's PlaySound call for that path).
            // The real unconditional taunt icon (mockery `83` regardless of
            // facing, Decor.cpp:9575-9578) is also NOT modeled -- no idle-
            // taunt animation system exists in this engine at all yet
            // (cosmetic, same as type2's taunt-suppression above).
            if (obj.type == ObjectType::ObjectType54)
            {
                if ((obj.patrolStep == 1 || obj.patrolStep == 3) && !blupiBallooned && !blupiInvincible)
                {
                    const float gdx = obj.currentX - blupiX;
                    const float gdy = obj.currentY - blupiY;
                    const float gdz = obj.currentZ - blupiZ;
                    if (gdx * gdx + gdy * gdy + gdz * gdz < kHazardContactRadius * kHazardContactRadius)
                    {
                        LoseLife();
                        diedThisFrame_ = true;
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel51);
                    }
                }
                continue;
            }

            // Generic hazard contact (ObjectType2/3/4/16/17/20/96/97, plan.md
            // E3D-MIG-132) -- real shared kill list: BlupiDead(Clear1,
            // Clear2) + the hazard itself is destroyed (converted to an
            // explosion). Type3's real duck-immunity (MoveObjectDetect
            // skips it entirely while Blupi's action is Down) is modeled
            // via blupiCrouching; the others have no such immunity. Real
            // death sound is a 50/50 coinflip (BlupiDead's own Clear2
            // branch plays channel 74, Clear1 plays nothing, per
            // Decor.cpp:6547-6614) -- simplified to always channel 74
            // rather than modeling the coinflip. While ballooned, exactly
            // 4 of these 8 types (3/16/96/97, IsBalloonPoppableHazard())
            // pop the balloon instead of killing (real channel 41 is
            // played by GEBlupiController's own IsBallooned() before/after
            // comparison in the caller, not here -- see PopBalloon()'s
            // comment) -- 2/4/17/20 still kill even while ballooned. Real
            // gate also includes `!m_blupiShield && !m_blupiHide`
            // (Decor.cpp:5784, plan.md E3D-MIG-170) -- modeled via
            // blupiInvincible, skipping the whole contact (no kill, no
            // balloon-pop either).
            if (IsGenericHazard(obj.type))
            {
                if (obj.type == ObjectType::ObjectType3 && blupiCrouching)
                {
                    continue;
                }
                const float hdx = obj.currentX - blupiX;
                const float hdy = obj.currentY - blupiY;
                const float hdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && hdx * hdx + hdy * hdy + hdz * hdz < kHazardContactRadius * kHazardContactRadius)
                {
                    if (blupiBallooned && IsBalloonPoppableHazard(obj.type))
                    {
                        balloonPoppedThisFrame_ = true;
                    }
                    else
                    {
                        obj.active = false;
                        LoseLife();
                        diedThisFrame_ = true;
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                        // Real camera shake (plan.md CAM-008/009, Decor.cpp
                        // ~5782-5814, confirmed via direct source read) --
                        // this exact real site plays SmallShake for every
                        // one of these 8 hazard types EXCEPT the fish (17)
                        // and bird (20) variants, which play BigShake
                        // instead (real behavior, not an approximation).
                        if (obj.type == ObjectType::ObjectType17 || obj.type == ObjectType::ObjectType20)
                        {
                            bigShakeTriggeredThisFrame_ = true;
                        }
                        else
                        {
                            smallShakeTriggeredThisFrame_ = true;
                        }
                    }
                }
                continue;
            }

            // Pickup collection -- treasure/egg/level-exit/keys, the only
            // IsPickup() types placed in today's sample world. Sound
            // channels and removal behavior come from `mobile-eggbert-
            // reference/13-object-pickups.md`/`07-sounds.md`, not from
            // GalaxyEggbertSimple3D's GESound shortcuts (PlayCollect/
            // PlayKey/PlayLife) -- that spec is the more carefully-verified
            // source and corrects 2 real mistakes the shortcuts would have
            // propagated: treasure/key pickup is channel 11 (or 19 for the
            // set-completing treasure), not channel 10, and egg pickup is
            // channel 3, not channel 42 (42 is Shield activation, unrelated).
            if (obj.type != ObjectType::ObjectType5 && obj.type != ObjectType::ObjectType6 &&
                obj.type != ObjectType::ObjectType7 && obj.type != ObjectType::ObjectType49 &&
                obj.type != ObjectType::ObjectType50 && obj.type != ObjectType::ObjectType51 &&
                obj.type != ObjectType::ObjectType55 && obj.type != ObjectType::ObjectType25 &&
                obj.type != ObjectType::ObjectType26 && obj.type != ObjectType::ObjectType30 &&
                obj.type != ObjectType::ObjectType31 && obj.type != ObjectType::ObjectType29 &&
                obj.type != ObjectType::ObjectType40)
            {
                continue;
            }

            const float dx = obj.currentX - blupiX;
            const float dy = obj.currentY - blupiY;
            const float dz = obj.currentZ - blupiZ;
            const bool touching = (dx * dx + dy * dy + dz * dz) < kPickupRadius * kPickupRadius;

            if (obj.type == ObjectType::ObjectType7)
            {
                // Level-exit goal: debounced to fire once per contact
                // "session" (real mobile-eggbert re-checks every 50 ticks
                // of continued contact via m_goalPhase; approximated here
                // as "once per entry" rather than exactly replicating that
                // tick-based re-fire, which needs no equivalent without a
                // HUD/rejection-sound spam concern this simpler debounce
                // already avoids). Gated on having all treasure, per
                // `13-object-pickups.md`'s real m_nbTresor >= m_totalTresor
                // check -- win fanfare (channel 14) if so, rejection sound
                // (channel 13) if not. Never removed (active stays true) --
                // it's a goal marker you touch, not a consumable pickup.
                if (touching)
                {
                    touchingExitThisFrame = true;
                    if (!exitContactActive_)
                    {
                        exitContactActive_ = true;
                        if (treasuresCollected_ >= totalTreasures_)
                        {
                            exitReached_ = true;
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel14);
                        }
                        else
                        {
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel13);
                        }
                    }
                }
                continue;
            }

            if (!touching)
            {
                continue;
            }

            // Every other pickup type here is deleted immediately on
            // contact per `13-object-pickups.md`'s "Voyage" section ("the
            // world object is deleted immediately, and a voyage animation
            // is [flown to the HUD]") -- no HUD-fly animation exists here,
            // just the immediate removal + reward + sound.
            switch (obj.type)
            {
                case ObjectType::ObjectType5: // treasure
                {
                    const bool completesSet = (treasuresCollected_ + 1 >= totalTreasures_);
                    ++treasuresCollected_;
                    sound.Play(completesSet ? GalaxyEggbert::SoundChannel::SoundChannel19
                                             : GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    // Treasure-gated doors (plan.md E3D-MIG-162, real
                    // Decor::OpenDoorsTresor ~11642) -- deferred to a single
                    // whole-grid scan after this loop, in case more than one
                    // treasure is somehow collected in the same frame.
                    treasureDoorScanNeeded = true;
                    // Real sparkle burst (plan.md VISUAL-012) -- see
                    // AppendSparkleBurst()'s own comment for the full real
                    // citation. Real source also fires this same burst for
                    // ObjectType49/50/51 (key-gated doors, not key pickups
                    // themselves) -- NOT wired here, since this engine's own
                    // door model uses static terrain tiles, not MobileObjSpec
                    // instances, for those; a separate follow-up if picked up.
                    AppendSparkleBurst(obj.currentX, obj.currentY, obj.currentZ, pendingSpawns);
                    break;
                }
                case ObjectType::ObjectType6: // extra-life egg
                    // Real MAX_EGG_COUNT=10 gate: at the cap, touching an
                    // egg does nothing at all -- not even removed.
                    if (lifeEggCount_ < kMaxEggCount)
                    {
                        ++lifeEggCount_;
                        ++lives_;
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                        obj.active = false;
                    }
                    break;
                case ObjectType::ObjectType49: // key 1
                    ++keys1_;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                case ObjectType::ObjectType50: // key 2
                    ++keys2_;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                case ObjectType::ObjectType51: // key 3
                    ++keys3_;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                case ObjectType::ObjectType55: // dynamite stick
                    // Real gate: only picked up while carrying none (real
                    // m_blupiDynamite caps at 1) -- touching a second stick
                    // while already carrying one does nothing at all, not
                    // even removed (mobile-eggbert-reference/
                    // 13-object-pickups.md).
                    if (dynamiteCount_ == 0)
                    {
                        ++dynamiteCount_;
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel60);
                        obj.active = false;
                    }
                    break;
                case ObjectType::ObjectType29: // bullet pack
                    // Real gate: only picked up below the cap (real
                    // m_blupiBullet < 10) -- touching a pack while already
                    // at max ammo does nothing at all, not even removed.
                    if (bulletCount_ < kBulletCap)
                    {
                        bulletCount_ = kBulletCap;
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel54);
                        obj.active = false;
                    }
                    break;
                // Secret powers (plan.md E3D-MIG-170, real Decor.cpp
                // ~6014-6087) -- all 4 grant on contact here (the real
                // 2-stage delay/animation before Power/Cloud/Hide actually
                // activate is NOT modeled, see GEBlupiController::
                // TriggerPower()'s own comment). Invert/Mirror (ObjectType40,
                // plan.md PICKUP-011) is a separate real pickup family but
                // shares the same instant-grant-on-contact shape, handled
                // right below the 4 powers. Real per-pickup gates (minus
                // vehicle-mode clauses that don't exist here) are passed in
                // from the caller's own GEBlupiController state
                // (blupiCanGrantShield/Power/Cloud/Hide/Invert) since this
                // class has no access to GEBlupiController itself -- consistent
                // with the blupiBallooned/blupiCrouching precedent.
                case ObjectType::ObjectType25: // shield stick
                    if (blupiCanGrantShield)
                    {
                        obj.active = false;
                        shieldGrantedThisFrame_ = true;
                    }
                    break;
                case ObjectType::ObjectType26: // suction-cup ("Sucette" -> Power)
                    if (blupiCanGrantPower)
                    {
                        obj.active = false;
                        powerGrantedThisFrame_ = true;
                    }
                    break;
                case ObjectType::ObjectType30: // drink ("Drink" -> Hide)
                    if (blupiCanGrantHide)
                    {
                        obj.active = false;
                        hideGrantedThisFrame_ = true;
                    }
                    break;
                case ObjectType::ObjectType31: // charge ("Charge" -> Cloud)
                    if (blupiCanGrantCloud)
                    {
                        obj.active = false;
                        cloudGrantedThisFrame_ = true;
                    }
                    break;
                case ObjectType::ObjectType40: // mirror/invert
                    if (blupiCanGrantInvert)
                    {
                        obj.active = false;
                        invertGrantedThisFrame_ = true;
                    }
                    break;
                default:
                    break;
            }
        }

        if (!touchingExitThisFrame)
        {
            exitContactActive_ = false;
        }

        // Key-gated doors (plan.md E3D-MIG-160/161, real `Decor::IsDoor`
        // ~7360): probes Blupi's own cell AND one cell further in his
        // facing direction (blupiFacingDX/DZ) for a door icon (334-336),
        // matching the real "trigger a step before actually reaching it"
        // detection exactly. Opens automatically if the matching key is
        // currently held -- no action-button gate in the real source,
        // unlike switches/dynamite. Real key flags are NOT consumed on
        // pickup, only on use (`Decor.cpp` ~5619) -- modeled here as
        // clearing the whole count to 0 (this engine's keys1_/keys2_/
        // keys3_ are plain pickup counters, not a persisted bitmask, but
        // real levels only ever grant one of each key before it must be
        // re-collected, so "count > 0" / "clear to 0" behaves identically
        // to the real boolean flag for the realistic case). Real voyage-
        // deferred key-flag-setting (pickup consumed from the world
        // immediately, but not "held" until a HUD-fly animation completes)
        // is NOT modeled -- same simplification as every other pickup this
        // session, applied immediately instead.
        {
            auto& terrain = worldRuntime.GetWorldMutable();
            const int axis = static_cast<int>(terrain.blocksPerAxis());
            const int probeGX[2] = {
                static_cast<int>(std::lround(blupiX)) + GEWorldRuntime::kWorldCenterX,
                static_cast<int>(std::lround(blupiX + static_cast<float>(blupiFacingDX))) +
                    GEWorldRuntime::kWorldCenterX,
            };
            const int probeGZ[2] = {
                static_cast<int>(std::lround(blupiZ)) + GEWorldRuntime::kWorldCenterZ,
                static_cast<int>(std::lround(blupiZ + static_cast<float>(blupiFacingDZ))) +
                    GEWorldRuntime::kWorldCenterZ,
            };
            const int probeGY = static_cast<int>(std::lround(blupiY));
            for (int p = 0; p < 2; ++p)
            {
                const int gx = probeGX[p];
                const int gz = probeGZ[p];
                if (gx < 0 || gx >= axis || gz < 0 || gz >= axis || probeGY < 0 || probeGY >= axis)
                {
                    continue;
                }
                const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(probeGY),
                                                    static_cast<std::uint16_t>(gz))
                                       .type();
                if (!GalaxyEggbert::BlockTypes::isDoor(icon))
                {
                    continue;
                }
                const int keyType = GalaxyEggbert::BlockTypes::doorKeyType(icon);
                const bool hasKey = (keyType == 49 && keys1_ > 0) || (keyType == 50 && keys2_ > 0) ||
                                     (keyType == 51 && keys3_ > 0);
                if (hasKey)
                {
                    OpenDoorAt(worldRuntime, gx, probeGY, gz, sound);
                    if (keyType == 49) keys1_ = 0;
                    else if (keyType == 50) keys2_ = 0;
                    else if (keyType == 51) keys3_ = 0;
                }
            }
        }

        // Treasure-gated doors (plan.md E3D-MIG-162, real
        // Decor::OpenDoorsTresor ~11642): a door needing N treasures uses
        // icon 420+N -- scans the whole grid and opens every one whose
        // requirement is now met, all at once, the same moment a
        // qualifying treasure pickup completed above (not just the
        // nearest door). Shared with CheatAllTreasure() below.
        if (treasureDoorScanNeeded)
        {
            ScanAndOpenTreasureDoors(worldRuntime, treasuresCollected_, sound);
        }

        // Bridge construction trigger (ObjectType52, plan.md PICKUP-064, real
        // Decor::IsBridge ~7334, called every tick from Decor.cpp ~5608-5612
        // while Blupi has focus -- not action-button gated, unlike switches/
        // dynamite). Real source probes 2 candidate foot-height offsets;
        // this engine uses a single stance height (round(blupiY)-1, same
        // convention as TryActivateSwitch's own standing-cell check) --
        // a documented simplification, not a transcribed real detail.
        // Spawns exactly one ObjectType52 at the Bridge cell, guarded so a
        // second never stacks on top of an already-in-progress one.
        {
            int bridgeGX, bridgeGY, bridgeGZ;
            ToGridCell(world, blupiX, blupiY - 1.0f, blupiZ, bridgeGX, bridgeGY, bridgeGZ);
            if (world.getBlock(static_cast<std::uint16_t>(bridgeGX), static_cast<std::uint16_t>(bridgeGY),
                                static_cast<std::uint16_t>(bridgeGZ))
                    .type() == GalaxyEggbert::BlockTypes::Bridge)
            {
                bool alreadyBuilding = false;
                for (const auto& obj : objects)
                {
                    if (obj.active && obj.type == ObjectType::ObjectType52 &&
                        std::lround(obj.currentX) == std::lround(blupiX) &&
                        std::lround(obj.currentZ) == std::lround(blupiZ))
                    {
                        alreadyBuilding = true;
                        break;
                    }
                }
                if (!alreadyBuilding)
                {
                    // currentY stays in world-space "standing height" (grid
                    // Y + 1), the same convention every other MobileObjSpec
                    // uses -- the per-tick terrain write below converts back
                    // to a grid index the same way TryActivateSwitch's own
                    // standing-cell check does.
                    MobileObjSpec bridge{};
                    bridge.type = ObjectType::ObjectType52;
                    bridge.posStartX = bridge.posEndX = bridge.currentX = static_cast<float>(std::lround(blupiX));
                    bridge.posStartY = bridge.posEndY = bridge.currentY = static_cast<float>(bridgeGY) + 1.0f;
                    bridge.posStartZ = bridge.posEndZ = bridge.currentZ = static_cast<float>(std::lround(blupiZ));
                    bridge.phase = 0.0f;
                    bridge.active = true;
                    pendingSpawns.push_back(bridge);
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel72);
                }
            }
        }

        // Player-fired Tank bullet (2026-07-13, plan.md BULLET-001, real
        // Decor.cpp:4308-4344) -- see Update()'s own class comment for the
        // full real-behavior citation. Cooldown ticks down every frame
        // regardless of input; only reset on an actual shot.
        fireCooldownTimer_ = std::max(0.0f, fireCooldownTimer_ - dt);
        if (blupiFirePressed && blupiCanFire && fireCooldownTimer_ <= 0.0f)
        {
            if (bulletCount_ > 0)
            {
                int gx, gy, gz;
                ToGridCell(world, blupiX, blupiY, blupiZ, gx, gy, gz);
                const int dist = SearchAirDistance(world, gx, gy, gz, blupiFacingDX, 0, blupiFacingDZ);
                --bulletCount_;
                fireCooldownTimer_ = kFireCooldownSeconds;
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel52);
                if (dist > 0)
                {
                    MobileObjSpec blupiPos{};
                    blupiPos.currentX = blupiX;
                    blupiPos.currentY = blupiY;
                    blupiPos.currentZ = blupiZ;
                    pendingSpawns.push_back(MakeBullet(blupiPos, static_cast<float>(blupiFacingDX), 0.0f,
                                                        static_cast<float>(blupiFacingDZ), dist));
                }
            }
            else
            {
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel53);
            }
        }

        // Flush this frame's blupih/blupit shots into the live object list
        // (deferred from the loop above, see pendingSpawns' own comment) --
        // reuse an inactive slot first, matching the real fixed-pool
        // MoveObjectFree()'s slot-reuse semantics rather than growing
        // `objects` unboundedly every time a shooter fires.
        for (auto& spec : pendingSpawns)
        {
            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = spec;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(spec);
            }
        }
    }

    void GEInteractionSystem::LoseLife()
    {
        --lives_;
        if (lives_ <= 0)
        {
            // Real DoorsLost() (Decor.cpp:11716) resets m_nbVies back to 3
            // on game-over rather than a permanent depletion.
            lives_ = 3;
            ++gameOverCount_;
        }
    }

    bool GEInteractionSystem::PlaceDynamite(GEWorldRuntime& worldRuntime, float x, float y, float z, bool grounded)
    {
        // Real gate (Decor.cpp ~4792-4812): carrying at least one, and
        // solid ground under both feet -- approximated here as `grounded`
        // (this engine's single-point collision has no separate left/right
        // foot check). Real "not in any vehicle mode, not lift-transported"
        // clauses aren't modeled -- neither concept exists yet.
        if (dynamiteCount_ <= 0 || !grounded)
        {
            return false;
        }
        --dynamiteCount_;

        MobileObjSpec fuse;
        fuse.type = ObjectType::ObjectType56;
        fuse.posStartX = fuse.posEndX = fuse.currentX = x;
        fuse.posStartY = fuse.posEndY = fuse.currentY = y;
        fuse.posStartZ = fuse.posEndZ = fuse.currentZ = z;
        fuse.phase = 0.0f;
        fuse.active = true;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = fuse;
                return true;
            }
        }
        objects.push_back(fuse);
        return true;
    }

    bool GEInteractionSystem::TryPerso(GEWorldRuntime& worldRuntime, float x, float y, float z, bool grounded)
    {
        auto& objects = worldRuntime.GetMobileObjectsMutable();

        // Real pickup (Decor.cpp ~6088-6101): standing near an already-
        // placed decoy takes priority over placing a new one (matching the
        // real source's own MoveObjectDetect check before the placement
        // branch). Proximity radius matches every other pickup/interaction
        // check in this file.
        constexpr float kPersoRadius = 1.1f;
        for (auto& obj : objects)
        {
            if (!obj.active || obj.type != ObjectType::ObjectType200)
            {
                continue;
            }
            const float dx = obj.currentX - x;
            const float dy = obj.currentY - y;
            const float dz = obj.currentZ - z;
            if (dx * dx + dy * dy + dz * dz < kPersoRadius * kPersoRadius)
            {
                if (persoCount_ >= kPersoCap)
                {
                    return false;
                }
                obj.active = false;
                ++persoCount_;
                return true;
            }
        }

        // Real placement (Decor.cpp ~4818-4841): carrying at least one,
        // and solid ground under both feet -- approximated as `grounded`,
        // same simplification as PlaceDynamite() above.
        if (persoCount_ <= 0 || !grounded)
        {
            return false;
        }
        --persoCount_;

        MobileObjSpec decoy;
        decoy.type = ObjectType::ObjectType200;
        decoy.posStartX = decoy.posEndX = decoy.currentX = x;
        decoy.posStartY = decoy.posEndY = decoy.currentY = y;
        decoy.posStartZ = decoy.posEndZ = decoy.currentZ = z;
        decoy.phase = 0.0f;
        decoy.active = true;

        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = decoy;
                return true;
            }
        }
        objects.push_back(decoy);
        return true;
    }

    void GEInteractionSystem::CheatOpenDoors(GEWorldRuntime& worldRuntime, GESound& sound)
    {
        auto& terrain = worldRuntime.GetWorldMutable();
        const int axis = static_cast<int>(terrain.blocksPerAxis());
        for (int gx = 0; gx < axis; ++gx)
        {
            for (int gy = 0; gy < axis; ++gy)
            {
                for (int gz = 0; gz < axis; ++gz)
                {
                    const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                        static_cast<std::uint16_t>(gz))
                                           .type();
                    // Both real door families, opened regardless of
                    // whether Blupi actually holds the matching key/
                    // treasure count -- see this method's own header
                    // comment for the real-vs-adapted reasoning (440 is
                    // this session's own confirmed real total icon
                    // ceiling, a safe upper bound for the treasure-door
                    // family's icon range).
                    if (GalaxyEggbert::BlockTypes::isDoor(icon) || (icon >= 421 && icon <= 440))
                    {
                        OpenDoorAt(worldRuntime, gx, gy, gz, sound);
                    }
                }
            }
        }
    }

    bool GEInteractionSystem::CheatCleanAll(GEWorldRuntime& worldRuntime)
    {
        bool anyDestroyed = false;
        for (auto& obj : worldRuntime.GetMobileObjectsMutable())
        {
            if (!obj.active)
            {
                continue;
            }
            if (IsGenericHazard(obj.type) || obj.type == ObjectType::ObjectType32 ||
                obj.type == ObjectType::ObjectType33 || obj.type == ObjectType::ObjectType44 ||
                obj.type == ObjectType::ObjectType54)
            {
                obj.active = false;
                anyDestroyed = true;
            }
        }
        return anyDestroyed;
    }

    void GEInteractionSystem::SpawnInvertBurst(GEWorldRuntime& worldRuntime, float blupiX, float blupiY,
                                                float blupiZ, bool isGrant)
    {
        // Real behavior confirmed via direct Decor.cpp read (the general
        // `Decor::ObjectStart()` sets posStart=the spawn point itself,
        // posEnd=spawn point + the bucketed direction's real 500px reach,
        // step=2, stepAdvance=ScaleTime(|speedMagnitude*500/64|)=78 at this
        // build's 20Hz reference rate -- NOT an instant static burst; the
        // object is meant to visibly slide from posStart to posEnd over 78
        // ticks, though it always self-deletes (phase>=16, see below) long
        // before actually arriving). Grant spawns each instance exactly AT
        // Blupi (no pre-offset), reaching a full 500px-out posEnd. Expiry
        // additionally pre-offsets each spawn point 100px in the OPPOSITE
        // direction before the same 500px push (`Decor.cpp:5145-5156`,
        // e.g. the "up" instance spawns 100px below Blupi then pushes up
        // 500px, netting a 400px-above final posEnd but sweeping back
        // through Blupi's own position along the way) -- both use the same
        // 64px-per-tile scale used throughout this engine's own atlas/
        // movement math.
        constexpr float kReach = 500.0f / 64.0f;
        constexpr float kExpiryPreOffset = 100.0f / 64.0f;
        const ObjectType type = isGrant ? ObjectType::ObjectType41 : ObjectType::ObjectType42;

        // Real screen-Y (up/down) maps to this engine's world-Y (height),
        // same convention already established for camera shake and Ghost
        // mode's vertical flight -- real screen-up (the real `speed=-60`
        // case) is world +Y here, real screen-down (`speed=60`) is world
        // -Y. Real screen-X (left/right) maps directly to world X with no
        // sign flip, same convention used everywhere else this session.
        const float dirs[4][3] = {
            {0.0f, 1.0f, 0.0f},  // up
            {0.0f, -1.0f, 0.0f}, // down
            {1.0f, 0.0f, 0.0f},  // +X (real "right")
            {-1.0f, 0.0f, 0.0f}, // -X (real "left")
        };

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (const auto& dir : dirs)
        {
            const float startOffset = isGrant ? 0.0f : -kExpiryPreOffset;
            const float endOffset = isGrant ? kReach : (kReach - kExpiryPreOffset);

            MobileObjSpec spec;
            spec.type = type;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = blupiX + dir[0] * startOffset;
            spec.currentY = spec.posStartY = blupiY + dir[1] * startOffset;
            spec.currentZ = spec.posStartZ = blupiZ + dir[2] * startOffset;
            spec.posEndX = blupiX + dir[0] * endOffset;
            spec.posEndY = blupiY + dir[1] * endOffset;
            spec.posEndZ = blupiZ + dir[2] * endOffset;

            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = spec;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(spec);
            }
        }
    }

    void GEInteractionSystem::SpawnFanHitFlash(GEWorldRuntime& worldRuntime, float x, float y, float z)
    {
        MobileObjSpec spec;
        spec.type = ObjectType::ObjectType11;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = x;
        spec.currentY = spec.posStartY = spec.posEndY = y;
        spec.currentZ = spec.posStartZ = spec.posEndZ = z;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    void GEInteractionSystem::CheatAllTreasure(GEWorldRuntime& worldRuntime, GESound& sound)
    {
        bool anyCollected = false;
        for (auto& obj : worldRuntime.GetMobileObjectsMutable())
        {
            if (obj.active && obj.type == ObjectType::ObjectType5)
            {
                obj.active = false;
                ++treasuresCollected_;
                anyCollected = true;
            }
        }
        if (anyCollected)
        {
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
            ScanAndOpenTreasureDoors(worldRuntime, treasuresCollected_, sound);
        }
    }

    bool GEInteractionSystem::CheatFindExit(const GEWorldRuntime& worldRuntime, float& outX, float& outY,
                                             float& outZ) const
    {
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType7)
            {
                outX = obj.currentX;
                outY = obj.currentY;
                outZ = obj.currentZ;
                return true;
            }
        }
        return false;
    }
}
