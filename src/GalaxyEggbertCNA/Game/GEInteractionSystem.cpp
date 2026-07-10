#include "GEInteractionSystem.hpp"

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
    }

    void GEInteractionSystem::Update(float dt, GEWorldRuntime& worldRuntime,
                                      float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                                      GESound& sound, bool blupiCrouching, bool blupiBallooned)
    {
        diedThisFrame_ = false;
        balloonTouchedThisFrame_ = false;
        balloonPoppedThisFrame_ = false;
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        const Worlds::World& world = worldRuntime.GetWorld();

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
                        const float destX = obj.currentX + pushDir;
                        const int wx = static_cast<int>(std::round(destX)) + GEWorldRuntime::kWorldCenterX;
                        const int wz = static_cast<int>(std::round(obj.currentZ)) + GEWorldRuntime::kWorldCenterZ;
                        const int axis = static_cast<int>(world.blocksPerAxis());
                        if (wx >= 0 && wx < axis && wz >= 0 && wz < axis)
                        {
                            const bool hasFloor = !world.getBlock(static_cast<std::uint16_t>(wx), 0,
                                                                   static_cast<std::uint16_t>(wz))
                                                        .isAir();
                            if (hasFloor)
                            {
                                bool occupied = false;
                                for (const auto& other : objects)
                                {
                                    if (&other == &obj || !IsCrate(other.type))
                                    {
                                        continue;
                                    }
                                    if (std::fabs(other.currentX - destX) < 0.5f &&
                                        std::fabs(other.currentZ - obj.currentZ) < 0.5f)
                                    {
                                        occupied = true;
                                        break;
                                    }
                                }
                                if (!occupied)
                                {
                                    obj.currentX = destX;
                                }
                            }
                        }
                    }
                }
                continue;
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
            // Real shield/hide/superblupi/win-and-death-action immunity
            // gates are NOT modeled -- none of those concepts exist in this
            // engine yet (same simplification already applied to every
            // other hazard above). Real death has no distinct sound call of
            // its own (StartSploutchGlu only spawns silent splash-effect
            // debris) -- channel 74 reused here for consistency with this
            // class's existing hazard-death sound approximation.
            if (obj.type == ObjectType::ObjectType23)
            {
                const float bdx = obj.currentX - blupiX;
                const float bdy = obj.currentY - blupiY;
                const float bdz = obj.currentZ - blupiZ;
                if (bdx * bdx + bdy * bdy + bdz * bdz < kHazardContactRadius * kHazardContactRadius)
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
            // (idempotent there, not here -- see the class comment).
            if (obj.type == ObjectType::ObjectType44)
            {
                const float wdx = obj.currentX - blupiX;
                const float wdy = obj.currentY - blupiY;
                const float wdz = obj.currentZ - blupiZ;
                if (wdx * wdx + wdy * wdy + wdz * wdz < kHazardContactRadius * kHazardContactRadius)
                {
                    balloonTouchedThisFrame_ = true;
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
            // comment) -- 2/4/17/20 still kill even while ballooned.
            if (IsGenericHazard(obj.type))
            {
                if (obj.type == ObjectType::ObjectType3 && blupiCrouching)
                {
                    continue;
                }
                const float hdx = obj.currentX - blupiX;
                const float hdy = obj.currentY - blupiY;
                const float hdz = obj.currentZ - blupiZ;
                if (hdx * hdx + hdy * hdy + hdz * hdz < kHazardContactRadius * kHazardContactRadius)
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
                obj.type != ObjectType::ObjectType50 && obj.type != ObjectType::ObjectType51)
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
                default:
                    break;
            }
        }

        if (!touchingExitThisFrame)
        {
            exitContactActive_ = false;
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
}
