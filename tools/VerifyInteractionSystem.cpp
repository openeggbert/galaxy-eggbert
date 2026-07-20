#include "Game/GEBlupiController.hpp"
#include "Game/GEHud.hpp"
#include "Game/GEInteractionSystem.hpp"
#include "Game/GEObjectIcons.hpp"
#include "Game/GESound.hpp"
#include "Game/GETrainingHints.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <Easy3D/Camera3D.hpp>

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <cmath>
#include <iostream>
#include <string>

// Scripted, non-interactive verification of GEInteractionSystem (2026-07-10)
// against worlds3d/world999.vwr, this engine's quarantined mechanics-showcase
// test world (renamed from world001.vwr 2026-07-17 when the real 78-world
// hub structure took over world001.vwr as the genuine global hub) -- proves
// platform lift patrol, crate push, and pickup collection (treasure/egg/key
// /exit) actually work, not just "compiles and doesn't crash". GESound is
// constructed but never LoadContent()-ed, so every Play() call is a no-op
// against an unloaded channel (no audio device needed for this scripted
// check).
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::CNA;

    const std::string worldPath = (argc > 1) ? argv[1] : "worlds3d/world999.vwr";

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    GEWorldRuntime world;
    if (!world.LoadFromVwrFile(worldPath))
    {
        std::cout << "FAIL: could not load " << worldPath << std::endl;
        return 1;
    }

    GESound sound; // never LoadContent()-ed -- every Play() call below is a silent no-op
    GEInteractionSystem interaction;
    constexpr float dt = 1.0f / 60.0f;

    // 1. Platform lift (ObjectType1, placed at world (0,4,-22)..(0,9,-22) per
    // tools/GenerateSampleWorld3D.cpp's "plateau -> crow's-nest lift") should
    // patrol: its currentY should move away from its start position after
    // enough Update() ticks, well before it could possibly have reached the
    // far end and ping-ponged back to the exact start value. Selected by
    // posStart != posEnd, not just by type -- the exhibition area
    // (2026-07-10) also places a STATIC ObjectType1 exhibit (posEnd ==
    // posStart, deliberately not patrolling), and CollectMoveObjects'
    // ordering is spatial (chunk order), so plain "first ObjectType1"
    // could find the stationary exhibit instead.
    const auto findPatrollingLift = [&world]() -> const MobileObjSpec*
    {
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType1 &&
                (obj.posStartX != obj.posEndX || obj.posStartY != obj.posEndY ||
                 obj.posStartZ != obj.posEndZ))
            {
                return &obj;
            }
        }
        return nullptr;
    };
    const auto* patrollingLift = findPatrollingLift();
    const float liftStartY = patrollingLift ? patrollingLift->currentY : -1.0f;
    check(patrollingLift != nullptr, "found the patrolling platform lift (ObjectType1) in the sample world");

    for (int i = 0; i < 30; ++i)
    {
        interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound); // Blupi far away
    }
    const auto* liftAfter = findPatrollingLift();
    const float liftYAfter = liftAfter ? liftAfter->currentY : liftStartY;
    std::cout << "Lift Y after 0.5s: " << liftYAfter << " (started at " << liftStartY << ")" << std::endl;
    check(std::fabs(liftYAfter - liftStartY) > 0.05f, "platform lift patrols (currentY changed)");

    // 1.5. Platform lift riding (plan.md E3D-MIG-152) -- position Blupi
    // exactly at the lift's current stand height (currentY + 2, see
    // GEInteractionSystem::IsRidingLift()'s own comment) and directly
    // above it in X/Z, then confirm one Update() call reports riding with
    // a RideStandY() matching the lift's (now patrolled) new position.
    if (liftAfter != nullptr)
    {
        const float rideBlupiX = liftAfter->currentX;
        const float rideBlupiZ = liftAfter->currentZ;
        const float rideBlupiY = liftAfter->currentY + 2.0f;
        interaction.Update(dt, world, rideBlupiX, rideBlupiY, rideBlupiZ, 0.0f, sound);
        check(interaction.IsRidingLift(), "IsRidingLift() is true while positioned on the lift's surface");
        const auto* liftNow = findPatrollingLift();
        check(liftNow != nullptr && std::fabs(interaction.RideStandY() - (liftNow->currentY + 2.0f)) < 0.01f,
              "RideStandY() matches the lift's own new stand height after its patrol step this frame");

        // Positioned far from the lift -- riding should not be reported.
        interaction.Update(dt, world, rideBlupiX + 20.0f, rideBlupiY, rideBlupiZ, 0.0f, sound);
        check(!interaction.IsRidingLift(), "IsRidingLift() is false when Blupi is nowhere near the lift");
    }
    else
    {
        check(false, "found the patrolling lift for the riding test");
    }

    // 2. Pickup collection -- walk Blupi's simulated position onto each of
    // the treasure/egg/key placed in the sample world and confirm the
    // interaction system collects it (counter increments, object goes
    // inactive) exactly once, not on every subsequent frame.
    const auto findFirst = [&world](ObjectType type) -> const MobileObjSpec*
    {
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == type) return &obj;
        }
        return nullptr;
    };

    // Voyage (plan.md `158`) test helper: GEInteractionSystem has no
    // camera access, so a real pickup only records a THIS-FRAME pending
    // request (GEInteractionSystem::Update() itself never starts the
    // actual voyage) -- real projection is GalaxyEggbertCnaGame's own
    // ResolvePendingVoyage() job. This helper stands in for that (no real
    // projection needed in a unit test -- just reuses the pending world
    // position directly as one endpoint, matching real behavior closely
    // enough for these tests, which only check the eventual REWARD, not
    // exact on-screen pixels), then fast-forwards world/interaction
    // updates (using the sentinel-position idiom already established
    // elsewhere in this file, so nothing else triggers meanwhile) until
    // the voyage completes. Returns false if no voyage was pending.
    const auto completePendingVoyage = [&sound](GEWorldRuntime& w, GEInteractionSystem& ir)
    {
        if (!ir.VoyagePendingThisFrame())
        {
            return false;
        }
        // Collapses start==end (ignoring the real pending positions --
        // unit tests only care that the reward eventually applies, not
        // exact on-screen distance/timing), so the voyage's real
        // `total=(|dx|+|dy|)/10` computes to 0 and the reward applies on
        // the very next tick. Deliberately avoids a long fast-forward:
        // advancing world time far enough for a REALISTIC screen distance
        // to complete would let other real periodic behavior (e.g.
        // blupih/blupit's own firing cooldown) fire too, an unwanted side
        // effect this helper's callers don't want.
        constexpr float kCollapsed = 0.0f;
        ir.BeginVoyage(w, ir.VoyagePendingKind(), ir.VoyagePendingIconId(), ir.VoyagePendingIsButtonChannel(),
                       kCollapsed, kCollapsed, kCollapsed, kCollapsed, sound);
        constexpr float voyageDt = 1.0f / 20.0f;
        w.Update(voyageDt);
        ir.Update(voyageDt, w, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        return true;
    };

    // Death-lock/life-loss-Voyage follow-up: real life loss/respawn is now
    // deferred behind a per-cause frozen "lock" duration (70-110 real
    // ticks) then a fixed 40-tick life-loss Voyage, mirroring
    // GalaxyEggbertCnaGame::ResolveDeathLock()'s own orchestration (which
    // this standalone tool has no access to, so it's replicated here).
    // Starts the lock if `ir.DeathLockRequestedThisFrame()` is set, then
    // fast-forwards `b`/`w`/`ir` together until it fully resolves (respawn
    // applied, LoseLife()/life-loss Voyage started). The real longest
    // duration is Clear4's 110-tick lock (5.5s) + the 40-tick (2.0s)
    // life-loss Voyage = 7.5s worst case -- 200 real-time-second iterations
    // at dt=1/20 comfortably covers every real cause with margin.
    const auto completeDeathLock = [&sound](GEWorldRuntime& w, GEInteractionSystem& ir, GEBlupiController& b)
    {
        if (ir.DeathLockRequestedThisFrame())
        {
            const auto kind = ir.DeathLockPendingKind();
            const auto cause = (kind == GEInteractionSystem::PendingDeathKind::Clear1) ? GEBlupiController::DeathCause::Clear1
                              : (kind == GEInteractionSystem::PendingDeathKind::Clear2) ? GEBlupiController::DeathCause::Clear2
                                                                                         : GEBlupiController::DeathCause::Glu;
            b.TriggerDeathLock(cause, ir.DeathLockShouldRespawn());
        }
        constexpr float lockDt = 1.0f / 20.0f;
        for (int i = 0; i < 200; ++i)
        {
            w.Update(lockDt);
            b.Step(w.GetWorld(), 0.0f, 0.0f, false, false, false, lockDt);
            bool shouldRespawn = false;
            if (b.ConsumeDeathLockResolved(shouldRespawn))
            {
                if (shouldRespawn)
                {
                    b.SetPosition(b.GetValidX(), b.GetValidY(), b.GetValidZ());
                }
                if (ir.Lives() <= 1)
                {
                    ir.LoseLife();
                }
                else
                {
                    ir.BeginVoyage(w, GEInteractionSystem::VoyageKind::LifeLoss, 48, false, 0.0f, 0.0f, 0.0f, 0.0f,
                                   sound);
                }
            }
            ir.Update(lockDt, w, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
            if (!b.IsDeathLocked() && !b.IsDeathHidden())
            {
                break;
            }
        }
    };

    if (const auto* egg = findFirst(ObjectType::ObjectType6))
    {
        const float ex = egg->currentX, ey = egg->currentY, ez = egg->currentZ;
        // Real reward is deferred to voyage completion (plan.md `158`) --
        // the first contact requests it; completePendingVoyage() stands in
        // for GalaxyEggbertCnaGame::ResolvePendingVoyage() + the flight
        // itself. The remaining frames of continued contact (the egg is
        // now inactive) must NOT request/grant a second reward.
        interaction.Update(dt, world, ex, ey, ez, 0.0f, sound);
        completePendingVoyage(world, interaction);
        for (int i = 0; i < 4; ++i)
        {
            interaction.Update(dt, world, ex, ey, ez, 0.0f, sound);
        }
        check(interaction.LifeEggCount() == 1, "egg collected exactly once (LifeEggCount == 1)");
        const auto* after = findFirst(ObjectType::ObjectType6);
        check(after == nullptr || !after->active, "collected egg is no longer active (stops rendering)");
        check(interaction.Lives() == 4, "egg pickup granted a life (Lives() == 4, started at 3)");
    }
    else
    {
        check(false, "found an egg (ObjectType6) in the sample world");
    }

    // 1.5 Lives -- LoseLife() decrements, and real DoorsLost() behavior
    // resets to 3 (not a permanent depletion) once it reaches 0, tracked
    // via GameOverCount() since no Lost-screen UI exists yet to observe it
    // any other way.
    {
        const int livesBefore = interaction.Lives(); // 4, from the egg above
        interaction.LoseLife();
        check(interaction.Lives() == livesBefore - 1, "LoseLife() decrements Lives() by exactly 1");
        check(interaction.GameOverCount() == 0, "GameOverCount() stays 0 while Lives() > 0");
        const int livesToLose = interaction.Lives(); // fixed snapshot -- Lives() itself changes below
        for (int i = 0; i < livesToLose; ++i)
        {
            interaction.LoseLife();
        }
        check(interaction.Lives() == 3, "Lives() resets to 3 on game-over, not a permanent depletion");
        check(interaction.GameOverCount() == 1, "GameOverCount() increments exactly once on game-over");
    }

    if (const auto* chest = findFirst(ObjectType::ObjectType5))
    {
        const float cx = chest->currentX, cy = chest->currentY, cz = chest->currentZ;
        // Real reward deferred to voyage completion (plan.md `158`) -- see
        // the egg test's own comment above.
        interaction.Update(dt, world, cx, cy, cz, 0.0f, sound);
        completePendingVoyage(world, interaction);
        for (int i = 0; i < 4; ++i)
        {
            interaction.Update(dt, world, cx, cy, cz, 0.0f, sound);
        }
        check(interaction.TreasuresCollected() == 1, "chest collected exactly once (TreasuresCollected == 1)");
    }
    else
    {
        check(false, "found a chest (ObjectType5) in the sample world");
    }

    // 2.1b. Treasure sparkle burst (plan.md VISUAL-012, ObjectType39) --
    // real 4-instance burst starting at the collected chest's own
    // position, sliding toward a real 500px/64-away posEnd over 78 ticks
    // (fixed 2026-07-14 -- was wrongly an instant static burst before) and
    // self-deleting at phase>=11, confirmed via direct Decor.cpp source
    // read.
    if (const auto* sparkleChest = findFirst(ObjectType::ObjectType5))
    {
        GEWorldRuntime sparkleWorld;
        check(sparkleWorld.LoadFromVwrFile(worldPath), "loaded a fresh world for the sparkle-burst test");
        GEInteractionSystem sparkleInteraction;
        constexpr float dt = 1.0f / 20.0f; // matches the real 20Hz tick rate obj.phase advances at
        constexpr float kDist = 500.0f / 64.0f;
        const float cx = sparkleChest->currentX, cy = sparkleChest->currentY, cz = sparkleChest->currentZ;

        // Filtered by proximity of posEnd (the real, fixed final target,
        // unaffected by the in-flight slide) to the chest, not a global
        // ObjectType39 count -- the sample world's own object-type
        // exhibition already places one static specimen of every
        // ObjectType (including 39) elsewhere in the map, so a blind
        // global count is always off by one (same false-positive shape as
        // the bridge-construction test above).
        const auto countNearbySparkles = [&sparkleWorld, cx, cy, cz, kDist]()
        {
            int count = 0;
            for (const auto& obj : sparkleWorld.GetMobileObjects())
            {
                if (!obj.active || obj.type != ObjectType::ObjectType39)
                {
                    continue;
                }
                const float edx = obj.posEndX - cx, edy = obj.posEndY - cy, edz = obj.posEndZ - cz;
                const float endDist = std::sqrt(edx * edx + edy * edy + edz * edz);
                if (std::fabs(endDist - kDist) < 0.01f)
                {
                    ++count;
                }
            }
            return count;
        };

        sparkleInteraction.Update(dt, sparkleWorld, cx, cy, cz, 0.0f, sound);
        check(countNearbySparkles() == 4,
              "collecting a treasure spawns exactly 4 ObjectType39 sparkle instances with a real 500px/64 posEnd");
        for (const auto& obj : sparkleWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType39 &&
                std::fabs(std::sqrt((obj.posEndX - cx) * (obj.posEndX - cx) + (obj.posEndY - cy) * (obj.posEndY - cy) +
                                     (obj.posEndZ - cz) * (obj.posEndZ - cz)) -
                          kDist) < 0.01f)
            {
                check(obj.currentX == cx && obj.currentY == cy && obj.currentZ == cz,
                      "sparkle instance starts exactly at the chest's own position (no pre-offset)");
            }
        }

        for (int i = 0; i < 10; ++i)
        {
            sparkleWorld.Update(dt);
        }
        sparkleInteraction.Update(dt, sparkleWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countNearbySparkles() == 4, "sparkle instances are still active just before their real phase-11 self-delete");

        sparkleWorld.Update(dt);
        sparkleInteraction.Update(dt, sparkleWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countNearbySparkles() == 0, "sparkle instances self-delete once phase reaches the real 11-tick lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-012, fixed
        // 2026-07-14 -- real table_tresortrack oscillates, was wrongly
        // ascending arithmetic before).
        check(GetObjIcon(ObjectType::ObjectType39, 0) == 166, "ObjectType39 icon at phase=0 is the real table_tresortrack[0]=166");
        check(GetObjIcon(ObjectType::ObjectType39, 5) == 161, "ObjectType39 icon at phase=5 is the real table_tresortrack[5]=161 (the shimmer's low point)");
        check(GetObjIcon(ObjectType::ObjectType39, 10) == 166, "ObjectType39 icon at phase=10 is the real table_tresortrack[10]=166 (back to the start)");
    }
    else
    {
        check(false, "found a chest (ObjectType5) in the sample world for the sparkle-burst test");
    }

    // GetBulldozerIcon() (ENEMY-013, added 2026-07-20) -- real ObjectType4
    // 4-state turn/walk step machine (Decor.cpp:8628-8668), direct
    // cross-check against the real table_bulldozer_left/right/turn2l/turn2r
    // values (Tables.cpp:1186-1205). patrolGoesLeftFromStart=true is the
    // real "posStart.X > posEnd.X" case.
    {
        // Left-from-start: step 1/3 are the turn poses telegraphing the
        // upcoming step 2 (walk left) / step 4 (walk right) respectively.
        check(GetBulldozerIcon(true, 1, 0) == 58, "left-from-start step 1 (turn-to-left) starts at the real first frame (icon 58)");
        check(GetBulldozerIcon(true, 1, 21) == 66, "left-from-start step 1 (turn-to-left) ends at the real last frame (icon 66)");
        check(GetBulldozerIcon(true, 2, 0) == 66, "left-from-start step 2 (walk left) starts at the real first frame (icon 66)");
        check(GetBulldozerIcon(true, 2, 2) == 67, "left-from-start step 2 (walk left) cycles through table_bulldozer_left (icon 67 at tick 2)");
        check(GetBulldozerIcon(true, 3, 0) == 66, "left-from-start step 3 (turn-to-right) starts at the real first frame (icon 66)");
        check(GetBulldozerIcon(true, 3, 21) == 58, "left-from-start step 3 (turn-to-right) ends at the real last frame (icon 58)");
        check(GetBulldozerIcon(true, 4, 0) == 58, "left-from-start step 4 (walk right) starts at the real first frame (icon 58)");
        check(GetBulldozerIcon(true, 4, 2) == 57, "left-from-start step 4 (walk right) cycles through table_bulldozer_right (icon 57 at tick 2)");

        // Right-from-start (posStart.X <= posEnd.X): every mapping mirrors.
        check(GetBulldozerIcon(false, 1, 0) == 66, "right-from-start step 1 (turn-to-right) starts at the real first frame (icon 66)");
        check(GetBulldozerIcon(false, 2, 0) == 58, "right-from-start step 2 (walk right) starts at the real first frame (icon 58)");
        check(GetBulldozerIcon(false, 3, 0) == 58, "right-from-start step 3 (turn-to-left) starts at the real first frame (icon 58)");
        check(GetBulldozerIcon(false, 4, 0) == 66, "right-from-start step 4 (walk left) starts at the real first frame (icon 66)");

        // Modulo wraparound past each table's real length.
        check(GetBulldozerIcon(true, 2, 8) == GetBulldozerIcon(true, 2, 0),
              "walk-left icon wraps around after the real 8-frame table_bulldozer_left length");
        check(GetBulldozerIcon(true, 1, 22) == GetBulldozerIcon(true, 1, 0),
              "turn-to-left icon wraps around after the real 22-frame table_bulldozer_turn2l length");
    }

    // GetFishIcon()/GetBirdIcon()/GetWaspIcon()/GetCreatureIcon() (ENEMY-016/018/024/026, added
    // 2026-07-20) -- same real 4-state step machine as the bulldozer above, direct cross-checks
    // against table_poisson_*/table_oiseau_*/table_guepe_*/table_creature_* (Tables.cpp:1212-1307).
    {
        check(GetFishIcon(true, 1, 0) == 79, "fish left-from-start step 1 (turn-to-left) starts at the real first frame (icon 79)");
        check(GetFishIcon(true, 1, 47) == 83, "fish left-from-start step 1 (turn-to-left) ends at the real last frame (icon 83)");
        check(GetFishIcon(true, 2, 0) == 82, "fish left-from-start step 2 (walk left) starts at the real first frame (icon 82)");
        check(GetFishIcon(false, 1, 0) == 82, "fish right-from-start step 1 (turn-to-right) starts at the real first frame (icon 82)");
        check(GetFishIcon(false, 2, 0) == 79, "fish right-from-start step 2 (walk right) starts at the real first frame (icon 79)");
        check(GetFishIcon(true, 1, 48) == GetFishIcon(true, 1, 0), "fish turn icon wraps around after the real 48-frame table length");

        check(GetBirdIcon(true, 1, 0) == 106, "bird left-from-start step 1 (turn-to-left) starts at the real first frame (icon 106)");
        check(GetBirdIcon(true, 1, 9) == 105, "bird left-from-start step 1 (turn-to-left) ends holding the real settled pose (icon 105)");
        check(GetBirdIcon(true, 2, 0) == 98, "bird left-from-start step 2 (fly left) starts at the real first frame (icon 98)");
        check(GetBirdIcon(false, 1, 0) == 114, "bird right-from-start step 1 (turn-to-right) starts at the real first frame (icon 114)");
        check(GetBirdIcon(false, 2, 0) == 90, "bird right-from-start step 2 (fly right) starts at the real first frame (icon 90)");
        check(GetBirdIcon(true, 1, 10) == GetBirdIcon(true, 1, 0), "bird turn icon wraps around after the real 10-frame table length");

        check(GetWaspIcon(true, 1, 0) == 207, "wasp left-from-start step 1 (turn-to-left) starts at the real first frame (icon 207)");
        check(GetWaspIcon(true, 1, 4) == 203, "wasp left-from-start step 1 (turn-to-left) ends at the real last frame (icon 203)");
        check(GetWaspIcon(true, 2, 0) == 195, "wasp left-from-start step 2 (fly left) starts at the real first frame (icon 195)");
        check(GetWaspIcon(false, 1, 0) == 203, "wasp right-from-start step 1 (turn-to-right) starts at the real first frame (icon 203)");
        check(GetWaspIcon(false, 2, 0) == 199, "wasp right-from-start step 2 (fly right) starts at the real first frame (icon 199)");
        check(GetWaspIcon(true, 1, 5) == GetWaspIcon(true, 1, 0), "wasp turn icon wraps around after the real 5-frame table length");

        check(GetCreatureIcon(1, 0) == 244, "creature step 1 (turn) starts at the real first frame (icon 244)");
        check(GetCreatureIcon(3, 0) == 244, "creature step 3 (turn) uses the SAME real single turn table as step 1 (icon 244)");
        check(GetCreatureIcon(1, 151) == 244, "creature turn ends at the real last frame (icon 244)");
        check(GetCreatureIcon(2, 0) == 247, "creature step 2 (walk) starts at the real first frame (icon 247)");
        check(GetCreatureIcon(4, 0) == 247, "creature step 4 (walk) is identical to step 2 (real table_creature_left/right are byte-identical)");
        check(GetCreatureIcon(1, 152) == GetCreatureIcon(1, 0), "creature turn icon wraps around after the real 152-frame table length");
    }

    // GetBlupihIcon()/GetBlupitIcon() (ENEMY-041/042, added 2026-07-20) -- same real 4-state step
    // machine, direct cross-checks against table_blupih_*/table_blupit_* (Tables.cpp:1314-1360).
    {
        check(GetBlupihIcon(true, 1, 0) == 71, "blupih left-from-start step 1 (turn-to-left) starts at the real first frame (icon 71)");
        check(GetBlupihIcon(true, 1, 25) == 275, "blupih left-from-start step 1 (turn-to-left) ends at the real last frame (icon 275)");
        check(GetBlupihIcon(true, 2, 0) == 66, "blupih left-from-start step 2 (fly left) starts at the real first frame (icon 66)");
        check(GetBlupihIcon(false, 1, 0) == 75, "blupih right-from-start step 1 (turn-to-right) starts at the real first frame (icon 75)");
        check(GetBlupihIcon(false, 2, 0) == 61, "blupih right-from-start step 2 (fly right) starts at the real first frame (icon 61)");
        check(GetBlupihIcon(true, 1, 26) == GetBlupihIcon(true, 1, 0), "blupih turn icon wraps around after the real 26-frame table length");

        check(GetBlupitIcon(true, 1, 0) == 238, "blupit left-from-start step 1 (turn-to-left) starts at the real first frame (icon 238)");
        check(GetBlupitIcon(true, 1, 23) == 249, "blupit left-from-start step 1 (turn-to-left) ends at the real last frame (icon 249)");
        check(GetBlupitIcon(true, 2, 0) == 249, "blupit left-from-start step 2 (drive left) starts at the real first frame (icon 249)");
        check(GetBlupitIcon(false, 1, 0) == 249, "blupit right-from-start step 1 (turn-to-right) starts at the real first frame (icon 249)");
        check(GetBlupitIcon(false, 2, 0) == 238, "blupit right-from-start step 2 (drive right) starts at the real first frame (icon 238)");
        check(GetBlupitIcon(true, 1, 24) == GetBlupitIcon(true, 1, 0), "blupit turn icon wraps around after the real 24-frame table length");
    }

    if (const auto* key = findFirst(ObjectType::ObjectType49))
    {
        const float kx = key->currentX, ky = key->currentY, kz = key->currentZ;
        // Real reward deferred to voyage completion (plan.md `158`) -- see
        // the egg test's own comment above.
        interaction.Update(dt, world, kx, ky, kz, 0.0f, sound);
        completePendingVoyage(world, interaction);
        for (int i = 0; i < 4; ++i)
        {
            interaction.Update(dt, world, kx, ky, kz, 0.0f, sound);
        }
        check(interaction.Key1Count() == 1, "key collected exactly once (Key1Count == 1)");

        // Real sparkle burst also fires for key pickups, not just treasure
        // (plan.md VISUAL-012, corrected 2026-07-14 -- ObjectType49/50/51
        // are the 3 key pickups themselves, not door tiles). Filtered by
        // posEnd proximity to the key, same false-positive-avoidance shape
        // as the treasure sparkle test's own `countNearbySparkles()`.
        constexpr float kKeySparkleDist = 500.0f / 64.0f;
        int keySparkleCount = 0;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (!obj.active || obj.type != ObjectType::ObjectType39)
            {
                continue;
            }
            const float edx = obj.posEndX - kx, edy = obj.posEndY - ky, edz = obj.posEndZ - kz;
            const float endDist = std::sqrt(edx * edx + edy * edy + edz * edz);
            if (std::fabs(endDist - kKeySparkleDist) < 0.01f)
            {
                ++keySparkleCount;
            }
        }
        check(keySparkleCount == 4, "collecting a key spawns the real 4-instance ObjectType39 sparkle burst too");
    }
    else
    {
        check(false, "found a key (ObjectType49) in the sample world");
    }

    // 2.5. Generic hazard contact (ObjectType2, plan.md E3D-MIG-132) -- the
    // sample world places one at world (41,1,67) and one at (18,11,49);
    // walking onto either should kill Blupi (lives lost, hazard destroyed,
    // DiedThisFrame() true for that one Update() call only).
    if (const auto* hazard = findFirst(ObjectType::ObjectType2))
    {
        const float hx = hazard->currentX, hy = hazard->currentY, hz = hazard->currentZ;
        const int livesBeforeHazard = interaction.Lives();
        interaction.Update(dt, world, hx, hy, hz, 0.0f, sound);
        check(interaction.DiedThisFrame(), "DiedThisFrame() is true the frame Blupi touches a generic hazard");
        check(interaction.SmallShakeTriggeredThisFrame(),
              "generic hazard contact-kill triggers SmallShake (plan.md CAM-008, real Decor.cpp behavior)");
        check(!interaction.BigShakeTriggeredThisFrame(),
              "a non-fish/bird hazard contact-kill does NOT trigger BigShake");
        // Real explosion flash (plan.md VISUAL-008) spawned at the same
        // site: ObjectType8 for this non-fish/bird hazard, exactly at the
        // hazard's own position.
        int explosionFlashCount = 0;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType8 && obj.currentX == hx && obj.currentY == hy &&
                obj.currentZ == hz)
            {
                ++explosionFlashCount;
            }
        }
        check(explosionFlashCount == 1,
              "generic hazard contact-kill spawns an ObjectType8 explosion flash at the hazard's position");
        // Matched on position, not just type -- `findFirst()` doesn't
        // filter by active state, and the real explosion-flash spawn
        // (plan.md VISUAL-008, added 2026-07-14) can now reuse this exact
        // now-inactive slot for its own ObjectType8, which would otherwise
        // make a blind type search find the sample world's OTHER real
        // ObjectType2 placement instead (still active), a false failure
        // (same false-positive shape flagged elsewhere this session).
        bool hazardStillActiveAtSamePos = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType2 && obj.currentX == hx && obj.currentY == hy &&
                obj.currentZ == hz)
            {
                hazardStillActiveAtSamePos = true;
                break;
            }
        }
        check(!hazardStillActiveAtSamePos, "the hazard that killed Blupi is destroyed (no longer active)");
        // Real life loss is now deferred behind the death-lock/life-loss-
        // Voyage (death-VFX follow-up) -- consumed BEFORE the "next frame"
        // check below, whose own interaction.Update() call would otherwise
        // wipe the still-pending DeathLockRequestedThisFrame() signal
        // first (a *ThisFrame() flag, reset every Update() call).
        GEBlupiController hazardDeathBlupi;
        completeDeathLock(world, interaction, hazardDeathBlupi);
        check(interaction.Lives() == livesBeforeHazard - 1, "generic hazard contact costs exactly 1 life");
        interaction.Update(dt, world, hx, hy, hz, 0.0f, sound);
        check(!interaction.DiedThisFrame(), "DiedThisFrame() is false again the very next frame");
    }
    else
    {
        check(false, "found a generic hazard (ObjectType2) in the sample world");
    }

    // 3. Crate push (ObjectType12) -- position Blupi immediately west of a
    // crate and simulate walking east into it (a positive per-frame X
    // delta); the crate's currentX should increase.
    if (const auto* crate = findFirst(ObjectType::ObjectType12))
    {
        const float startX = crate->currentX;
        const float blupiZ = crate->currentZ;
        float blupiX = startX - 0.8f;
        bool sawCratePushSignal = false;
        for (int i = 0; i < 30; ++i)
        {
            const float moveDX = 0.05f; // walking east this frame
            interaction.Update(dt, world, blupiX, crate->currentY, blupiZ, moveDX, sound);
            sawCratePushSignal = sawCratePushSignal || interaction.CrateBeingPushedThisFrame();
            blupiX += moveDX;
        }
        const auto* after = findFirst(ObjectType::ObjectType12);
        std::cout << "Crate X after push attempt: " << (after ? after->currentX : -999.0f)
                  << " (started at " << startX << ")" << std::endl;
        check(after != nullptr && after->currentX > startX, "crate was pushed east (currentX increased)");
        check(sawCratePushSignal,
              "CrateBeingPushedThisFrame() was true on at least one frame a push actually moved "
              "the crate (real crate-push loop sound, ch38, found 2026-07-16) -- checked across "
              "the whole approach since the crate outpaces Blupi's own per-frame step once pushed");

        // Real crate-push gate (Decor.cpp:6130-6132, found 2026-07-16) also excludes every
        // vehicle mode + Ecrase -- blupiCanPushCrate=false (e.g. while riding a vehicle) must
        // stop the push entirely, even while walking straight into the crate.
        const float gatedStartX = after->currentX;
        float gatedBlupiX = gatedStartX - 0.8f;
        for (int i = 0; i < 30; ++i)
        {
            const float moveDX = 0.05f;
            interaction.Update(dt, world, gatedBlupiX, after->currentY, blupiZ, moveDX, sound, false, false, 0, 0,
                                false, true, true, true, true, false, false, false, true, false,
                                /*blupiCanPushCrate=*/false);
            gatedBlupiX += moveDX;
        }
        const auto* stillGated = findFirst(ObjectType::ObjectType12);
        check(stillGated != nullptr && std::fabs(stillGated->currentX - gatedStartX) < 0.01f,
              "crate does NOT move when blupiCanPushCrate=false (real vehicle-mode gate)");
        check(!interaction.CrateBeingPushedThisFrame(),
              "CrateBeingPushedThisFrame() is false when the vehicle-mode gate blocks the push");
    }
    else
    {
        check(false, "found a crate (ObjectType12) in the sample world");
    }

    // 3.5. Linked crates (plan.md E3D-MIG-150) -- the sample world's own
    // linked-crate demo (tools/GenerateSampleWorld3D.cpp): 2 crates side by
    // side at grid (61,1,73)/(62,1,73) plus a 3rd stacked on top of the
    // first at (61,2,73) -- render space (grid - 50): (11,1,23)/(12,1,23)/
    // (11,2,23). Pushing the seed (the one at (11,1,23), closest to a Blupi
    // approaching from the west) should move all 3 atomically.
    const auto findByPos = [&world](float px, float py, float pz) -> const MobileObjSpec*
    {
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType12 && std::fabs(obj.posStartX - px) < 0.1f &&
                std::fabs(obj.posStartY - py) < 0.1f && std::fabs(obj.posStartZ - pz) < 0.1f)
            {
                return &obj;
            }
        }
        return nullptr;
    };
    const auto* seed = findByPos(11.0f, 1.0f, 23.0f);
    const auto* neighbor = findByPos(12.0f, 1.0f, 23.0f);
    const auto* stacked = findByPos(11.0f, 2.0f, 23.0f);
    check(seed != nullptr && neighbor != nullptr && stacked != nullptr,
          "found the linked-crate demo trio (side-by-side pair + stacked) in the sample world");
    if (seed != nullptr && neighbor != nullptr && stacked != nullptr)
    {
        const float seedStartX = seed->currentX;
        const float neighborStartX = neighbor->currentX;
        const float stackedStartX = stacked->currentX;
        const float pushZ = seed->currentZ;
        float blupiX = seedStartX - 0.8f;
        for (int i = 0; i < 30; ++i)
        {
            interaction.Update(dt, world, blupiX, seed->currentY, pushZ, 0.05f, sound);
            blupiX += 0.05f;
        }
        const auto* seedAfter = findByPos(11.0f, 1.0f, 23.0f);
        const auto* neighborAfter = findByPos(12.0f, 1.0f, 23.0f);
        const auto* stackedAfter = findByPos(11.0f, 2.0f, 23.0f);
        std::cout << "Linked crates after push -- seed X: " << (seedAfter ? seedAfter->currentX : -999.0f)
                  << " (started " << seedStartX << "), neighbor X: "
                  << (neighborAfter ? neighborAfter->currentX : -999.0f) << " (started " << neighborStartX
                  << "), stacked X: " << (stackedAfter ? stackedAfter->currentX : -999.0f) << " (started "
                  << stackedStartX << ")" << std::endl;
        check(seedAfter != nullptr && seedAfter->currentX > seedStartX,
              "the pushed (seed) crate moved east");
        // The neighbor keeps its original 1-unit offset from the seed (it
        // doesn't snap to the same X) -- what matters is that it moved the
        // SAME net distance as the seed, not left behind at its start
        // position.
        check(neighborAfter != nullptr &&
                  std::fabs((neighborAfter->currentX - neighborStartX) - (seedAfter->currentX - seedStartX)) < 0.1f,
              "the horizontally-linked neighbor crate moved the same distance as the seed, not left behind");
        check(stackedAfter != nullptr &&
                  std::fabs((stackedAfter->currentX - stackedStartX) - (seedAfter->currentX - seedStartX)) < 0.1f,
              "the vertically-stacked crate moved together with the seed too (real SearchLinkCaisse "
              "links vertically as well as horizontally)");
    }

    // 3.6. Dynamite (plan.md E3D-MIG-155) -- pickup, the placement gate,
    // and the full 9-blast sequence against a synthetic target crate
    // pushed directly into the loaded world's mobile-object list (kept
    // isolated from the real sample world's own crates/position).
    if (const auto* dynamite = findFirst(ObjectType::ObjectType55))
    {
        const float ddx = dynamite->currentX, ddy = dynamite->currentY, ddz = dynamite->currentZ;
        // Real reward deferred to voyage completion (plan.md `158`) -- see
        // the egg test's own comment above.
        interaction.Update(dt, world, ddx, ddy, ddz, 0.0f, sound);
        completePendingVoyage(world, interaction);
        check(interaction.DynamiteCount() == 1, "dynamite pickup increments DynamiteCount() to 1");

        check(!interaction.PlaceDynamite(world, 0.0f, 1.0f, 0.0f, /*grounded=*/false),
              "PlaceDynamite() is a no-op while not grounded");
        check(interaction.DynamiteCount() == 1, "DynamiteCount() unchanged after the failed placement");

        // Real vehicle-mode gate (Decor.cpp:4792-4794, found 2026-07-16) -- the caller (e.g. while
        // mounted in any vehicle) reports blupiCanUseHands=false; PlaceDynamite() must stay a
        // no-op even while grounded and carrying one.
        check(!interaction.PlaceDynamite(world, 0.0f, 1.0f, 0.0f, /*grounded=*/true,
                                          /*blupiCanUseHands=*/false),
              "PlaceDynamite() is a no-op when blupiCanUseHands=false (real vehicle-mode gate)");
        check(interaction.DynamiteCount() == 1, "DynamiteCount() unchanged after the vehicle-gated no-op");

        // A fresh, isolated spot -- the real center blast (tick 50) has a
        // (0,0) offset, so a target placed exactly here and Blupi standing
        // exactly here are both within its 2x2-tile radius.
        constexpr float placeX = 200.0f, placeY = 1.0f, placeZ = 200.0f;
        check(interaction.PlaceDynamite(world, placeX, placeY, placeZ, /*grounded=*/true),
              "PlaceDynamite() succeeds while carrying one and grounded");
        check(interaction.DynamiteCount() == 0, "DynamiteCount() drops to 0 after placing");

        auto& mutableObjects = world.GetMobileObjectsMutable();
        MobileObjSpec targetCrate;
        targetCrate.type = ObjectType::ObjectType12;
        targetCrate.active = true;
        targetCrate.currentX = targetCrate.posStartX = targetCrate.posEndX = placeX + 0.3f;
        targetCrate.currentY = targetCrate.posStartY = targetCrate.posEndY = placeY;
        targetCrate.currentZ = targetCrate.posStartZ = targetCrate.posEndZ = placeZ;
        mutableObjects.push_back(targetCrate);

        const int livesBeforeBlast = interaction.Lives();
        const int gameOversBeforeBlast = interaction.GameOverCount();
        // Advance the fuse through its full ~70-tick active timeline (well
        // under 4s at the real 20Hz reference rate) in small steps so each
        // blast tick is individually crossed, not skipped over. world.phase
        // only advances via GEWorldRuntime::Update() itself (the real game
        // loop calls this every frame before GEInteractionSystem::Update();
        // this test must too, or obj.phase never moves).
        bool smallShakeSeenDuringBlast = false;
        bool explosionFlashSeenAtCenter = false;
        // Real life loss/respawn for the blast's own death check is now
        // deferred behind the death-lock/life-loss-Voyage (death-VFX
        // follow-up) -- driven inline here (not completeDeathLock(), which
        // has its own loop) alongside the existing blast-sequence
        // fast-forward. 750 iterations (12.5s at dt=1/60) comfortably
        // covers the real ~70-tick(3.5s) blast sequence PLUS the worst-case
        // 110-tick lock(5.5s) + 40-tick(2.0s) life-loss Voyage that might
        // start near the end of it.
        GEBlupiController blastDeathBlupi;
        for (int i = 0; i < 750; ++i)
        {
            world.Update(dt);
            interaction.Update(dt, world, placeX, placeY, placeZ, 0.0f, sound);
            if (interaction.DeathLockRequestedThisFrame())
            {
                const auto kind = interaction.DeathLockPendingKind();
                const auto cause = (kind == GEInteractionSystem::PendingDeathKind::Clear1)
                                        ? GEBlupiController::DeathCause::Clear1
                                    : (kind == GEInteractionSystem::PendingDeathKind::Clear2)
                                        ? GEBlupiController::DeathCause::Clear2
                                        : GEBlupiController::DeathCause::Glu;
                blastDeathBlupi.TriggerDeathLock(cause, interaction.DeathLockShouldRespawn());
            }
            blastDeathBlupi.Step(world.GetWorld(), 0.0f, 0.0f, false, false, false, dt);
            bool blastShouldRespawn = false;
            if (blastDeathBlupi.ConsumeDeathLockResolved(blastShouldRespawn))
            {
                if (blastShouldRespawn)
                {
                    blastDeathBlupi.SetPosition(blastDeathBlupi.GetValidX(), blastDeathBlupi.GetValidY(),
                                                 blastDeathBlupi.GetValidZ());
                }
                if (interaction.Lives() <= 1)
                {
                    interaction.LoseLife();
                }
                else
                {
                    interaction.BeginVoyage(world, GEInteractionSystem::VoyageKind::LifeLoss, 48, false, 0.0f, 0.0f,
                                            0.0f, 0.0f, sound);
                }
            }
            smallShakeSeenDuringBlast = smallShakeSeenDuringBlast || interaction.SmallShakeTriggeredThisFrame();
            for (const auto& obj : world.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType8 &&
                    std::fabs(obj.currentX - placeX) < 0.01f && std::fabs(obj.currentY - placeY) < 0.01f &&
                    std::fabs(obj.currentZ - placeZ) < 0.01f)
                {
                    explosionFlashSeenAtCenter = true;
                }
            }
        }
        check(smallShakeSeenDuringBlast,
              "the dynamite blast's own center-tile tick triggers SmallShake (plan.md CAM-008, real "
              "Decor::DynamiteStart() behavior)");
        check(explosionFlashSeenAtCenter,
              "the dynamite blast's own center-tile tick also spawns an ObjectType8 explosion flash "
              "(plan.md VISUAL-008), at the exact blast-center position");

        bool crateStillActive = false;
        bool fuseStillActive = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType12 && obj.active &&
                std::fabs(obj.currentX - targetCrate.currentX) < 0.01f &&
                std::fabs(obj.currentZ - targetCrate.currentZ) < 0.01f)
            {
                crateStillActive = true;
            }
            if (obj.type == ObjectType::ObjectType56 && obj.active &&
                std::fabs(obj.currentX - placeX) < 0.01f && std::fabs(obj.currentZ - placeZ) < 0.01f)
            {
                fuseStillActive = true;
            }
        }
        check(!crateStillActive, "the dynamite blast destroyed the crate within its blast radius");
        check(interaction.Lives() < livesBeforeBlast || interaction.GameOverCount() > gameOversBeforeBlast,
              "standing in the blast radius cost Blupi a life (or triggered game-over)");
        check(!fuseStillActive, "the fuse object self-destructs once its sequence completes");
    }
    else
    {
        check(false, "found a dynamite stick (ObjectType55) in the sample world");
    }

    // 3.7. Bullet pack (plan.md E3D-MIG-175) -- the sample world's own
    // bullet-pack demo (tools/GenerateSampleWorld3D.cpp): automatic pickup
    // (no button), caps at 10, a second contact once already at the cap is
    // a genuine no-op (object stays active, count unchanged) per
    // mobile-eggbert-reference/13-object-pickups.md.
    float fireTestX = 0.0f, fireTestY = 0.0f, fireTestZ = 0.0f;
    if (const auto* bullets = findFirst(ObjectType::ObjectType29))
    {
        const float bx = bullets->currentX, by = bullets->currentY, bz = bullets->currentZ;
        fireTestX = bx;
        fireTestY = by;
        fireTestZ = bz;
        check(interaction.BulletCount() == 0, "BulletCount() starts at 0");
        interaction.Update(dt, world, bx, by, bz, 0.0f, sound);
        check(interaction.BulletCount() == 10, "bullet pack pickup tops BulletCount() up to 10");
        const auto* after = findFirst(ObjectType::ObjectType29);
        check(after == nullptr || !after->active, "collected bullet pack is no longer active");

        // Already at the cap: a second synthetic pack at the cap should be
        // a total no-op (stays active, count unchanged) -- exercised via a
        // fresh pack pushed directly into the mobile-object list so this
        // doesn't depend on the (now-collected) real one.
        auto& mutableObjects = world.GetMobileObjectsMutable();
        MobileObjSpec secondPack;
        secondPack.type = ObjectType::ObjectType29;
        secondPack.active = true;
        secondPack.currentX = secondPack.posStartX = secondPack.posEndX = bx;
        secondPack.currentY = secondPack.posStartY = secondPack.posEndY = by;
        secondPack.currentZ = secondPack.posStartZ = secondPack.posEndZ = bz;
        mutableObjects.push_back(secondPack);
        interaction.Update(dt, world, bx, by, bz, 0.0f, sound);
        check(interaction.BulletCount() == 10, "BulletCount() stays at the cap after touching a pack while full");
        check(mutableObjects.back().active, "a bullet pack touched while already at the cap is NOT removed");
    }
    else
    {
        check(false, "found a bullet pack (ObjectType29) in the sample world");
    }

    // 3.71. Player-fired Tank bullet (plan.md BULLET-001, real
    // Decor.cpp:4308-4344) -- BulletCount() is already 10 from 3.7 above.
    // Reuses the bullet pack's own real in-world position (fireTestX/Y/Z)
    // -- these tests only check ammo/cooldown bookkeeping, not the
    // raycast distance, so whatever terrain happens to be there doesn't
    // matter (a real shot fired straight into a wall still consumes ammo
    // and starts the cooldown, exactly like the real gate order: ammo
    // check happens before the raycast). 3.7 above left a still-ACTIVE
    // synthetic second bullet pack sitting at this exact position (its
    // own "no-op while at the cap" test) -- deactivated here first, or it
    // would silently re-top BulletCount() back to the cap every one of
    // these Update() calls and mask every assertion below.
    {
        for (auto& obj : world.GetMobileObjectsMutable())
        {
            if (obj.type == ObjectType::ObjectType29 && std::fabs(obj.currentX - fireTestX) < 0.01f &&
                std::fabs(obj.currentZ - fireTestZ) < 0.01f)
            {
                obj.active = false;
            }
        }

        const int bulletsBeforeFiring = interaction.BulletCount();

        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            /*blupiFirePressed=*/true, /*blupiCanFire=*/false);
        check(interaction.BulletCount() == bulletsBeforeFiring,
              "firing while not in a Tank (canFire=false) does not consume ammo");
        check(!interaction.TankFiredThisFrame(),
              "TankFiredThisFrame() is false when not in a Tank (real FireTank anim gate)");

        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            true, true);
        check(interaction.BulletCount() == bulletsBeforeFiring - 1,
              "firing while in a Tank consumes exactly 1 bullet");
        check(interaction.TankFiredThisFrame(),
              "TankFiredThisFrame() is true the exact frame a bullet actually launches");

        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            true, true);
        check(interaction.BulletCount() == bulletsBeforeFiring - 1,
              "holding Fire within the real 0.5s cooldown does not fire again");
        check(!interaction.TankFiredThisFrame(),
              "TankFiredThisFrame() is false while blocked by the real 0.5s cooldown");

        // Advance past the real 0.5s cooldown (Fire not held during the
        // wait, matching a real "tap" cadence) then fire again.
        for (int i = 0; i < 40; ++i)
        {
            interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                                false, false, 1, 0, false, true, true, true, true,
                                false, true);
        }
        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            true, true);
        check(interaction.BulletCount() == bulletsBeforeFiring - 2,
              "firing again after the cooldown elapses consumes a second bullet");

        // Drain to 0, then confirm firing with no ammo does not underflow.
        while (interaction.BulletCount() > 0)
        {
            for (int i = 0; i < 40; ++i)
            {
                interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                                    false, false, 1, 0, false, true, true, true, true,
                                    false, true);
            }
            interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                                false, false, 1, 0, false, true, true, true, true,
                                true, true);
        }
        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            true, true);
        check(interaction.BulletCount() == 0, "firing with no ammo left does not underflow BulletCount()");
        check(!interaction.TankFiredThisFrame(),
              "TankFiredThisFrame() is false on the empty-clip click, no real recoil pose then");
    }

    // 3.74. Types 201-203 (plan.md PICKUP-069, found 2026-07-16, real Decor.cpp:6088-6115) --
    // lethal decorative objects sharing ObjectType200's range but NOT Perso (200) itself. Real
    // contact: Clear1/Clear2 coinflip death (Shield/Hide immune), always channel 10 + SmallShake +
    // an ObjectType10 pop effect.
    {
        GEWorldRuntime lethalDecorWorld;
        GEInteractionSystem lethalDecorInteraction;
        MobileObjSpec decor201;
        decor201.type = ObjectType::ObjectType201;
        decor201.posStartX = decor201.posEndX = decor201.currentX = 400.0f;
        decor201.posStartY = decor201.posEndY = decor201.currentY = 1.0f;
        decor201.posStartZ = decor201.posEndZ = decor201.currentZ = 400.0f;
        lethalDecorWorld.GetMobileObjectsMutable().push_back(decor201);

        // Shield/Hide immunity (blupiInvincible=true) -- no death, object survives.
        lethalDecorInteraction.Update(dt, lethalDecorWorld, 400.0f, 1.0f, 400.0f, 0.0f, sound, false, false, 0, 0,
                                       /*blupiInvincible=*/true);
        check(!lethalDecorInteraction.DiedThisFrame(),
              "touching ObjectType201 with blupiInvincible=true does not kill Blupi (real Shield/Hide gate)");
        check(lethalDecorWorld.GetMobileObjects().front().active,
              "ObjectType201 survives contact while Blupi is invincible");

        // Now without invincibility -- lethal, real Clear1/Clear2 coinflip + effects.
        lethalDecorInteraction.Update(dt, lethalDecorWorld, 400.0f, 1.0f, 400.0f, 0.0f, sound);
        check(lethalDecorInteraction.DiedThisFrame(), "touching ObjectType201 kills Blupi (real BlupiDead(Clear1,Clear2))");
        const bool anyActive201 = std::any_of(lethalDecorWorld.GetMobileObjects().begin(),
                                               lethalDecorWorld.GetMobileObjects().end(),
                                               [](const auto& o) { return o.active && o.type == ObjectType::ObjectType201; });
        check(!anyActive201, "ObjectType201 is destroyed on lethal contact (real ObjectDelete) -- the destroyed "
                             "slot may be reused by the real ObjectType10 pop effect spawned the same frame");
        check(lethalDecorInteraction.DeathLockRequestedThisFrame(),
              "lethal contact requests a death lock");
        check(lethalDecorInteraction.DeathLockPendingKind() == GEInteractionSystem::PendingDeathKind::Clear1 ||
                  lethalDecorInteraction.DeathLockPendingKind() == GEInteractionSystem::PendingDeathKind::Clear2,
              "the death lock's pending kind is the real Clear1/Clear2 coinflip, nothing else");
        check(!lethalDecorInteraction.DeathLockShouldRespawn(),
              "shouldRespawn is false (real: no m_blupiRestart=true anywhere in this block)");
        check(lethalDecorInteraction.SmallShakeTriggeredThisFrame(),
              "lethal contact always triggers SmallShake (real: no fish/bird BigShake variant here)");
    }

    // 3.745. Perso-decoy/lethal-decor trap (found 2026-07-16, real Decor.cpp:7957-7975 +
    // Decor::MovePersoDetect() ~9835-9865) -- small enemies (4/32/33) that patrol into contact
    // with ANY real 200-203 object mutually destroy each other. Resolves the "what does placing a
    // Perso decoy actually DO" mystery HUD-017's own writeup left open.
    {
        GEWorldRuntime trapWorld;
        GEInteractionSystem trapInteraction;

        MobileObjSpec decoy;
        decoy.type = ObjectType::ObjectType200;
        decoy.posStartX = decoy.posEndX = decoy.currentX = 500.0f;
        decoy.posStartY = decoy.posEndY = decoy.currentY = 1.0f;
        decoy.posStartZ = decoy.posEndZ = decoy.currentZ = 500.0f;
        trapWorld.GetMobileObjectsMutable().push_back(decoy);

        MobileObjSpec bulldozer;
        bulldozer.type = ObjectType::ObjectType4;
        bulldozer.posStartX = bulldozer.posEndX = bulldozer.currentX = 500.0f;
        bulldozer.posStartY = bulldozer.posEndY = bulldozer.currentY = 1.0f;
        bulldozer.posStartZ = bulldozer.posEndZ = bulldozer.currentZ = 500.0f;
        trapWorld.GetMobileObjectsMutable().push_back(bulldozer);

        // Blupi is far away -- this mechanic doesn't involve his position at all.
        trapInteraction.Update(dt, trapWorld, 9999.0f, 9999.0f, 9999.0f, 0.0f, sound);
        check(trapInteraction.SmallShakeTriggeredThisFrame(),
              "the Perso-decoy trap triggers SmallShake when a small enemy touches a placed decoy");
        const bool anyActiveDecoyOrEnemy =
            std::any_of(trapWorld.GetMobileObjects().begin(), trapWorld.GetMobileObjects().end(),
                        [](const auto& o) {
                            return o.active && (o.type == ObjectType::ObjectType200 || o.type == ObjectType::ObjectType4);
                        });
        check(!anyActiveDecoyOrEnemy, "both the decoy and the enemy are destroyed by the trap (real mutual ObjectDelete)");

        // A small enemy far from any decoy is entirely unaffected.
        GEWorldRuntime noTrapWorld;
        GEInteractionSystem noTrapInteraction;
        MobileObjSpec farDecoy;
        farDecoy.type = ObjectType::ObjectType200;
        farDecoy.posStartX = farDecoy.posEndX = farDecoy.currentX = 500.0f;
        farDecoy.posStartY = farDecoy.posEndY = farDecoy.currentY = 1.0f;
        farDecoy.posStartZ = farDecoy.posEndZ = farDecoy.currentZ = 500.0f;
        noTrapWorld.GetMobileObjectsMutable().push_back(farDecoy);
        MobileObjSpec farBulldozer;
        farBulldozer.type = ObjectType::ObjectType4;
        farBulldozer.posStartX = farBulldozer.posEndX = farBulldozer.currentX = 700.0f;
        farBulldozer.posStartY = farBulldozer.posEndY = farBulldozer.currentY = 1.0f;
        farBulldozer.posStartZ = farBulldozer.posEndZ = farBulldozer.currentZ = 700.0f;
        noTrapWorld.GetMobileObjectsMutable().push_back(farBulldozer);
        noTrapInteraction.Update(dt, noTrapWorld, 9999.0f, 9999.0f, 9999.0f, 0.0f, sound);
        check(!noTrapInteraction.SmallShakeTriggeredThisFrame(),
              "no trap trigger when the enemy is far from any 200-203 object");
        const bool bothStillActive =
            noTrapWorld.GetMobileObjects()[0].active && noTrapWorld.GetMobileObjects()[1].active;
        check(bothStillActive, "both the decoy and the enemy survive when far apart");
    }

    // 3.75. Perso decoy (plan.md HUD-017) -- real m_blupiPerso starts at 0
    // (Decor.cpp:163/360/426) with no world pickup that grants it (only a
    // level-authored save-data field this engine doesn't model yet, and
    // retrieving an already-placed decoy) -- so this exercises the full
    // round trip entirely via synthetic objects, not the sample world's own
    // (nonexistent) demo placement.
    {
        constexpr float px = 300.0f, py = 1.0f, pz = 300.0f;
        check(interaction.PersoCount() == 0, "PersoCount() starts at 0, matching the real default");
        check(!interaction.TryPerso(world, px, py, pz, /*grounded=*/true),
              "TryPerso() placement is a no-op while PersoCount() == 0");

        auto& mutableObjects = world.GetMobileObjectsMutable();
        MobileObjSpec decoy;
        decoy.type = ObjectType::ObjectType200;
        decoy.active = true;
        decoy.currentX = decoy.posStartX = decoy.posEndX = px;
        decoy.currentY = decoy.posStartY = decoy.posEndY = py;
        decoy.currentZ = decoy.posStartZ = decoy.posEndZ = pz;
        mutableObjects.push_back(decoy);

        check(interaction.TryPerso(world, px, py, pz, /*grounded=*/true),
              "TryPerso() picks up a nearby placed decoy");
        // Real reward deferred to voyage completion (plan.md `158`) -- see
        // the egg test's own comment above.
        completePendingVoyage(world, interaction);
        check(interaction.PersoCount() == 1, "picking up the decoy increments PersoCount() to 1");
        check(!mutableObjects.back().active, "the picked-up decoy is no longer active");

        check(interaction.TryPerso(world, px, py, pz, /*grounded=*/true),
              "TryPerso() places a new decoy now that PersoCount() > 0");
        check(interaction.PersoCount() == 0, "placing the decoy decrements PersoCount() back to 0");
        bool foundNewDecoy = false;
        for (const auto& obj : mutableObjects)
        {
            if (obj.active && obj.type == ObjectType::ObjectType200 && std::fabs(obj.currentX - px) < 0.01f)
            {
                foundNewDecoy = true;
            }
        }
        check(foundNewDecoy, "TryPerso() actually spawned a new active decoy in the world");

        // Real vehicle-mode gate (Decor.cpp:4792-4794/6088-6101, found 2026-07-16): placement
        // excludes every vehicle mode, but pickup has NO such clause at all -- picking up the
        // decoy just placed above must still succeed even with blupiCanUseHands=false.
        check(interaction.TryPerso(world, px, py, pz, /*grounded=*/true, /*blupiCanUseHands=*/false),
              "TryPerso() pickup still succeeds with blupiCanUseHands=false (real: no vehicle "
              "clause on pickup)");
        completePendingVoyage(world, interaction);
        check(interaction.PersoCount() == 1,
              "picking up the decoy (blupiCanUseHands=false) still increments PersoCount()");

        check(!interaction.TryPerso(world, px, py, pz, /*grounded=*/true, /*blupiCanUseHands=*/false),
              "TryPerso() placement IS blocked with blupiCanUseHands=false (real vehicle-mode gate)");
        check(interaction.PersoCount() == 1, "PersoCount() unchanged after the vehicle-gated no-op");
    }

    // 3.76. Secret exit (ObjectType21, plan.md PICKUP-009/083, found 2026-07-16) -- shares the
    // exact same real exit-gate logic as the regular exit (ObjectType7, Decor.cpp:6158-6184).
    // Previously this engine only recognized ObjectType7, so touching a secret exit did nothing.
    {
        GEWorldRuntime secretExitWorld;
        GEInteractionSystem secretExitInteraction;
        MobileObjSpec secretExit;
        secretExit.type = ObjectType::ObjectType21;
        secretExit.active = true;
        secretExit.posStartX = secretExit.posEndX = secretExit.currentX = 600.0f;
        secretExit.posStartY = secretExit.posEndY = secretExit.currentY = 1.0f;
        secretExit.posStartZ = secretExit.posEndZ = secretExit.currentZ = 600.0f;
        secretExitWorld.GetMobileObjectsMutable().push_back(secretExit);

        // No treasure objects placed at all -- totalTreasures_ (lazily computed by scanning for
        // ObjectType5) is 0, trivially satisfying the real m_nbTresor>=m_totalTresor gate.
        secretExitInteraction.Update(dt, secretExitWorld, 600.0f, 1.0f, 600.0f, 0.0f, sound);
        check(secretExitInteraction.ExitReached(),
              "touching a secret exit (ObjectType21) with all treasure collected reaches the real "
              "exit-gate win condition, same as the regular exit");
    }

    // 3.8. Doors (plan.md E3D-MIG-160/161/162) -- the sample world's own
    // doors demo (tools/GenerateSampleWorld3D.cpp): a key-gated Door1 at
    // grid (48,1,90) with its Key1 at (46,1,88), and a treasure-gated
    // door (icon 421, needs 1) at (48,1,93). Fresh GEInteractionSystem
    // instances so their own key/treasure counters start at 0,
    // independent of the shared `interaction` used by every test above.
    {
        const auto doorTileType = [&]()
        { return world.GetWorld().getBlock(48, 1, 90).type(); };
        check(doorTileType() == BlockTypes::Door1, "the key-gated door demo tile starts as Door1 (closed)");

        GEInteractionSystem doorInteraction;
        // Approach without holding the key -- facing +Z (toward the door
        // at z=90 from z=89), render space (grid - 50).
        doorInteraction.Update(dt, world, 48.0f - 50.0f, 1.0f, 89.0f - 50.0f, 0.0f, sound, false, false, 0, 1);
        check(doorTileType() == BlockTypes::Door1, "the door stays closed while Blupi doesn't hold the matching key");

        doorInteraction.Update(dt, world, 46.0f - 50.0f, 1.0f, 88.0f - 50.0f, 0.0f, sound);
        // Real reward deferred to voyage completion (plan.md `158`) -- see
        // the egg test's own comment above. The door-OPENING itself is
        // NOT deferred (real key-flag clearing happens immediately, before
        // the door-unlock flourish's own voyage even starts -- see
        // RequestVoyage(VoyageKind::DoorUnlock, ...)'s own call site).
        completePendingVoyage(world, doorInteraction);
        check(doorInteraction.Key1Count() == 1, "picking up the key increments Key1Count() to 1");

        doorInteraction.Update(dt, world, 48.0f - 50.0f, 1.0f, 89.0f - 50.0f, 0.0f, sound, false, false, 0, 1);
        check(doorTileType() != BlockTypes::Door1,
              "the door opens (tile no longer Door1) once Blupi approaches while holding the key");
        check(doorInteraction.Key1Count() == 0, "the key is consumed on use (real behavior), not just on pickup");
    }

    {
        // Icon 422 needs 2 treasures -- deliberately more than the single
        // chest any earlier test in this file collects via the shared
        // `interaction`, so this tile is still genuinely closed here (not
        // already opened as a side effect of an earlier section).
        const auto treasureDoorTileType = [&]()
        { return world.GetWorld().getBlock(48, 1, 93).type(); };
        check(treasureDoorTileType() == 422,
              "the treasure-gated door demo tile starts as icon 422 (needs 2 treasures)");

        std::vector<const MobileObjSpec*> freshChests;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType5 && obj.active)
            {
                freshChests.push_back(&obj);
                if (freshChests.size() == 2) break;
            }
        }
        check(freshChests.size() == 2, "found 2 still-active chests for the treasure-gated door test");
        if (freshChests.size() == 2)
        {
            GEInteractionSystem treasureDoorInteraction;
            // Real reward (and the treasure-door rescan it triggers)
            // deferred to voyage completion (plan.md `158`) -- see the
            // egg test's own comment above.
            treasureDoorInteraction.Update(dt, world, freshChests[0]->currentX, freshChests[0]->currentY,
                                            freshChests[0]->currentZ, 0.0f, sound);
            completePendingVoyage(world, treasureDoorInteraction);
            check(treasureDoorTileType() == 422,
                  "the door stays closed after only 1 of the 2 required treasures is collected");
            treasureDoorInteraction.Update(dt, world, freshChests[1]->currentX, freshChests[1]->currentY,
                                            freshChests[1]->currentZ, 0.0f, sound);
            completePendingVoyage(world, treasureDoorInteraction);
            check(treasureDoorInteraction.TreasuresCollected() == 2,
                  "the fresh interaction system's own treasure count reaches 2");
            check(treasureDoorTileType() != 422,
                  "the treasure-gated door opens the instant the 2nd qualifying treasure pickup completes");
        }
    }

    // 3.9. Secret powers (plan.md E3D-MIG-170) -- the sample world's own
    // demo (tools/GenerateSampleWorld3D.cpp): Shield stick at (54,1,88).
    // Fresh GEInteractionSystem so its own signals aren't polluted by
    // earlier sections.
    if (const auto* shieldStick = findFirst(ObjectType::ObjectType25))
    {
        GEInteractionSystem shieldInteraction;
        shieldInteraction.Update(dt, world, shieldStick->currentX, shieldStick->currentY,
                                  shieldStick->currentZ, 0.0f, sound, false, false, 0, 0,
                                  /*blupiInvincible=*/false, /*canGrantShield=*/false);
        check(!shieldInteraction.ShieldGrantedThisFrame(),
              "ShieldGrantedThisFrame() is false when the caller reports canGrantShield=false");

        GEInteractionSystem shieldInteraction2;
        shieldInteraction2.Update(dt, world, shieldStick->currentX, shieldStick->currentY,
                                   shieldStick->currentZ, 0.0f, sound, false, false, 0, 0,
                                   /*blupiInvincible=*/false, /*canGrantShield=*/true);
        check(shieldInteraction2.ShieldGrantedThisFrame(),
              "ShieldGrantedThisFrame() is true on contact when canGrantShield=true");
    }
    else
    {
        check(false, "found the shield stick (ObjectType25) in the sample world");
    }

    // 3.9b. Invert/Mirror (plan.md PICKUP-011) -- the sample world's own
    // demo (tools/GenerateSampleWorld3D.cpp): mirror/invert at (58,1,90).
    if (const auto* invertPickup = findFirst(ObjectType::ObjectType40))
    {
        GEInteractionSystem invertInteraction;
        invertInteraction.Update(dt, world, invertPickup->currentX, invertPickup->currentY,
                                  invertPickup->currentZ, 0.0f, sound, false, false, 0, 0,
                                  /*blupiInvincible=*/false, /*canGrantShield=*/true, /*canGrantPower=*/true,
                                  /*canGrantCloud=*/true, /*canGrantHide=*/true, /*blupiFirePressed=*/false,
                                  /*blupiCanFire=*/false, /*blupiCloudActive=*/false, /*canGrantInvert=*/false);
        check(!invertInteraction.InvertGrantedThisFrame(),
              "InvertGrantedThisFrame() is false when the caller reports canGrantInvert=false");

        GEInteractionSystem invertInteraction2;
        invertInteraction2.Update(dt, world, invertPickup->currentX, invertPickup->currentY,
                                   invertPickup->currentZ, 0.0f, sound, false, false, 0, 0,
                                   /*blupiInvincible=*/false, /*canGrantShield=*/true, /*canGrantPower=*/true,
                                   /*canGrantCloud=*/true, /*canGrantHide=*/true, /*blupiFirePressed=*/false,
                                   /*blupiCanFire=*/false, /*blupiCloudActive=*/false, /*canGrantInvert=*/true);
        check(invertInteraction2.InvertGrantedThisFrame(),
              "InvertGrantedThisFrame() is true on contact when canGrantInvert=true");
    }
    else
    {
        check(false, "found the mirror/invert pickup (ObjectType40) in the sample world");
    }

    // 3.9c. Invert start/stop particle burst (plan.md VISUAL-014/015,
    // ObjectType41 on grant / ObjectType42 on expiry) -- 4 instances each,
    // real 500px/64 grant posEnd reach / 400px/64 expiry posEnd reach
    // (both computed from a real 100px/64 pre-offset for expiry, see
    // SpawnInvertBurst()'s own comment), a real slide from posStart to
    // posEnd over 78 ticks (fixed 2026-07-14 -- was wrongly an instant
    // static burst before), real screen-Y-to-world-Y sign flip, and the
    // real phase>=16 self-delete (long before the 78-tick slide actually
    // completes).
    {
        GEWorldRuntime burstWorld;
        GEInteractionSystem burstInteraction;
        constexpr float bx = 10.0f, by = 1.0f, bz = 10.0f;
        constexpr float kGrantDist = 500.0f / 64.0f;
        constexpr float kExpiryDist = 400.0f / 64.0f;
        constexpr float kExpiryPreOffset = 100.0f / 64.0f;
        constexpr float dt = 1.0f / 20.0f; // matches the real 20Hz tick rate obj.phase advances at

        burstInteraction.SpawnInvertBurst(burstWorld, bx, by, bz, /*isGrant=*/true);
        int grantCount = 0;
        for (const auto& obj : burstWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType41)
            {
                ++grantCount;
                check(obj.currentX == bx && obj.currentY == by && obj.currentZ == bz,
                      "grant burst instance starts exactly at Blupi's own position (no pre-offset)");
                const float edx = obj.posEndX - bx, edy = obj.posEndY - by, edz = obj.posEndZ - bz;
                const float endDist = std::sqrt(edx * edx + edy * edy + edz * edz);
                check(std::fabs(endDist - kGrantDist) < 0.01f,
                      "grant burst instance's real posEnd target is the real 500px/64 distance from Blupi");
                check(edz == 0.0f, "grant burst never offsets along world Z (only X/Y are used)");
            }
        }
        check(grantCount == 4, "SpawnInvertBurst(isGrant=true) spawns exactly 4 ObjectType41 instances");

        burstInteraction.SpawnInvertBurst(burstWorld, bx, by, bz, /*isGrant=*/false);
        int expiryCount = 0;
        for (const auto& obj : burstWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType42)
            {
                ++expiryCount;
                const float sdx = obj.currentX - bx, sdy = obj.currentY - by, sdz = obj.currentZ - bz;
                const float startDist = std::sqrt(sdx * sdx + sdy * sdy + sdz * sdz);
                check(std::fabs(startDist - kExpiryPreOffset) < 0.01f,
                      "expiry burst instance starts the real 100px/64 pre-offset away from Blupi, opposite its "
                      "final direction");
                const float edx = obj.posEndX - bx, edy = obj.posEndY - by, edz = obj.posEndZ - bz;
                const float endDist = std::sqrt(edx * edx + edy * edy + edz * edz);
                check(std::fabs(endDist - kExpiryDist) < 0.01f,
                      "expiry burst instance's real posEnd target is the real 400px/64 distance from Blupi "
                      "(closer than grant)");
            }
        }
        check(expiryCount == 4, "SpawnInvertBurst(isGrant=false) spawns exactly 4 ObjectType42 instances");

        // Real self-delete at phase>=16 (Decor.cpp:8575-8596) -- phase is
        // advanced by World::Update() itself (same convention as every
        // other phase-driven object this session), not by Update() here.
        for (int i = 0; i < 15; ++i)
        {
            burstWorld.Update(dt);
        }
        burstInteraction.Update(dt, burstWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        int stillActiveAt15 = 0;
        for (const auto& obj : burstWorld.GetMobileObjects())
        {
            if (obj.active && (obj.type == ObjectType::ObjectType41 || obj.type == ObjectType::ObjectType42))
            {
                ++stillActiveAt15;
            }
        }
        check(stillActiveAt15 == 8, "all 8 burst instances are still active just before their real phase-16 self-delete");

        burstWorld.Update(dt);
        burstInteraction.Update(dt, burstWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        int stillActiveAt16 = 0;
        for (const auto& obj : burstWorld.GetMobileObjects())
        {
            if (obj.active && (obj.type == ObjectType::ObjectType41 || obj.type == ObjectType::ObjectType42))
            {
                ++stillActiveAt16;
            }
        }
        check(stillActiveAt16 == 0, "all 8 burst instances self-delete once phase reaches the real 16-tick lifetime");

        // GetObjIcon()'s own formula (plan.md VISUAL-014/015, fixed
        // 2026-07-14 -- divisor was 6, real is 2; ObjectType42 wrongly
        // ascended past 186 instead of matching the real table's exact
        // reverse). table_invertstart={179..186}, table_invertstop=
        // {186..179} (Tables.cpp:1498/1502, confirmed via direct source
        // read) -- checked at phase=0 (first frame) and phase=14 (last
        // frame before the real phase-16 self-delete, (14/2)%8==7).
        check(GetObjIcon(ObjectType::ObjectType41, 0) == 179, "ObjectType41 icon at phase=0 is the real table_invertstart[0]=179");
        check(GetObjIcon(ObjectType::ObjectType41, 14) == 186, "ObjectType41 icon at phase=14 is the real table_invertstart[7]=186");
        check(GetObjIcon(ObjectType::ObjectType42, 0) == 186, "ObjectType42 icon at phase=0 is the real table_invertstop[0]=186");
        check(GetObjIcon(ObjectType::ObjectType42, 14) == 179, "ObjectType42 icon at phase=14 is the real table_invertstop[7]=179 (descending, not ascending past 186)");
    }

    // Hazard immunity: a fresh interaction system touching a STILL-ACTIVE
    // generic hazard (test 2.5 above already deactivated the first
    // ObjectType2 findFirst() would return, via the shared `interaction`
    // instance -- this looks past that one), with blupiInvincible=true,
    // should take no damage at all (real `!m_blupiShield && !m_blupiHide`
    // gate confirmed directly against Decor.cpp:5784).
    const MobileObjSpec* hazard2 = nullptr;
    for (const auto& obj : world.GetMobileObjects())
    {
        if (obj.type == ObjectType::ObjectType2 && obj.active)
        {
            hazard2 = &obj;
            break;
        }
    }
    if (hazard2 != nullptr)
    {
        GEInteractionSystem invincibleInteraction;
        const int livesBefore = invincibleInteraction.Lives();
        invincibleInteraction.Update(dt, world, hazard2->currentX, hazard2->currentY, hazard2->currentZ, 0.0f,
                                      sound, false, false, 0, 0, /*blupiInvincible=*/true);
        check(!invincibleInteraction.DiedThisFrame(),
              "DiedThisFrame() stays false touching a generic hazard while blupiInvincible=true");
        check(invincibleInteraction.Lives() == livesBefore,
              "no life is lost touching a generic hazard while invincible (real Shield/Hide immunity)");
        check(hazard2->active,
              "the hazard itself is untouched (not destroyed) by a contact that immunity blocked");
    }
    else
    {
        check(false, "found a generic hazard (ObjectType2) for the invincibility test");
    }

    // 4. GEWorldRuntime::IsBlitzActiveAtPhase() (plan.md E3D-MIG-144) -- real
    // BlitzActif() cycle: lethal only on even ticks within the first half of
    // a 100-tick cycle (num%2==0 && num<50).
    check(GEWorldRuntime::IsBlitzActiveAtPhase(0), "Blitz active at phase 0 (cycle start, even, first half)");
    check(!GEWorldRuntime::IsBlitzActiveAtPhase(1), "Blitz inactive at phase 1 (odd)");
    check(GEWorldRuntime::IsBlitzActiveAtPhase(48), "Blitz active at phase 48 (even, still first half)");
    check(!GEWorldRuntime::IsBlitzActiveAtPhase(50), "Blitz inactive at phase 50 (even, but second half)");
    check(!GEWorldRuntime::IsBlitzActiveAtPhase(99), "Blitz inactive at phase 99 (odd, second half)");
    check(GEWorldRuntime::IsBlitzActiveAtPhase(100), "Blitz active at phase 100 (cycle wraps back to 0)");

    // 5. GEWorldRuntime::IsCrusherActiveAtPhase() (plan.md E3D-MIG-143) --
    // real IsEcraseur() cycle: (phase/3)%10 <= 2, a 3-out-of-10 window.
    check(GEWorldRuntime::IsCrusherActiveAtPhase(0), "Crusher active at phase 0 (cycle start)");
    check(GEWorldRuntime::IsCrusherActiveAtPhase(8), "Crusher active at phase 8 (8/3=2, still in window)");
    check(!GEWorldRuntime::IsCrusherActiveAtPhase(9), "Crusher inactive at phase 9 (9/3=3, just past the window)");
    check(!GEWorldRuntime::IsCrusherActiveAtPhase(29), "Crusher inactive at phase 29 (29/3=9, end of the off window)");
    check(GEWorldRuntime::IsCrusherActiveAtPhase(30), "Crusher active at phase 30 (30/3=10, cycle wraps)");

    // 6. GEWorldRuntime::TryActivateSwitch() (plan.md E3D-MIG-142) -- the
    // sample world places a switch (starts SwitchOff) at grid (65,0,67) and
    // a linked saw (starts SawStopped) at grid (70,0,67), 5 cells apart
    // (within the real +-20 window). Grid (65,0,67) in raw grid space is
    // blupi position (15,1,67-50)=(15,1,17) in the render/camera space
    // TryActivateSwitch() (and blupi_.GetX/Y/Z()) actually use -- see
    // GEWorldRuntime::kWorldCenterX/Z and GenerateSampleWorld3D.cpp's own
    // "raw grid coordinates" comment.
    {
        constexpr float kSwitchBlupiX = 15.0f, kSwitchBlupiY = 1.0f, kSwitchBlupiZ = 17.0f;
        const auto getSwitchType = [&world]()
        { return world.GetWorld().getBlock(65, 0, 67).type(); };
        const auto getSawType = [&world]()
        { return world.GetWorld().getBlock(70, 0, 67).type(); };

        check(getSwitchType() == BlockTypes::SwitchOff, "switch starts SwitchOff, matching the saw's SawStopped start");
        check(getSawType() == BlockTypes::SawStopped, "linked saw starts SawStopped (safe)");

        check(!world.TryActivateSwitch(0.0f, 1.0f, 0.0f, true).has_value(),
              "TryActivateSwitch() is a no-op away from any switch tile");
        check(!world.TryActivateSwitch(kSwitchBlupiX, kSwitchBlupiY, kSwitchBlupiZ, false).has_value(),
              "TryActivateSwitch() is a no-op while airborne, even standing over a switch's column");

        const auto turnedOn = world.TryActivateSwitch(kSwitchBlupiX, kSwitchBlupiY, kSwitchBlupiZ, true);
        check(turnedOn.has_value() && *turnedOn, "TryActivateSwitch() turns the switch on (SwitchOff -> Switch)");
        check(getSwitchType() == BlockTypes::Switch, "switch tile itself is now Switch (on)");
        check(getSawType() == BlockTypes::Saw, "linked saw 5 cells away is now Saw (active/dangerous)");

        const auto turnedOff = world.TryActivateSwitch(kSwitchBlupiX, kSwitchBlupiY, kSwitchBlupiZ, true);
        check(turnedOff.has_value() && !*turnedOff, "TryActivateSwitch() toggles back off on a second press");
        check(getSwitchType() == BlockTypes::SwitchOff, "switch tile is SwitchOff again");
        check(getSawType() == BlockTypes::SawStopped, "linked saw is SawStopped again (safe)");
    }

    // 7. Shared kill list widened beyond ObjectType2/3 (plan.md E3D-MIG-132,
    // 2026-07-11) -- none of the other 6 real member types (4/16/17/20/
    // 96/97) are placed in the sample world, so a spider (16) is injected
    // directly into the loaded world's MobileObjSpec list to prove
    // GEInteractionSystem::Update() treats it exactly like ObjectType2/3
    // (contact kills Blupi, destroys the spider) without needing a real
    // level placement first.
    {
        MobileObjSpec spider;
        spider.type = ObjectType::ObjectType16;
        spider.posStartX = spider.posEndX = spider.currentX = 5.0f;
        spider.posStartY = spider.posEndY = spider.currentY = 1.0f;
        spider.posStartZ = spider.posEndZ = spider.currentZ = 5.0f;
        world.GetMobileObjectsMutable().push_back(spider);

        const int livesBeforeSpider = interaction.Lives();
        const int gameOverBeforeSpider = interaction.GameOverCount();
        interaction.Update(dt, world, 5.0f, 1.0f, 5.0f, 0.0f, sound);
        check(interaction.DiedThisFrame(), "DiedThisFrame() is true touching an injected spider (ObjectType16)");
        GEBlupiController spiderDeathBlupi;
        completeDeathLock(world, interaction, spiderDeathBlupi);
        // Lives() may already be down to 1 from earlier sections in this
        // same shared `interaction` instance -- a real life lost here can
        // therefore legitimately wrap back to 3 via game-over, same idiom
        // as the bulldozer/turn-dwell tests below.
        const bool spiderCostALife = (interaction.Lives() == livesBeforeSpider - 1) ||
                                     (interaction.GameOverCount() == gameOverBeforeSpider + 1 && interaction.Lives() == 3);
        check(spiderCostALife, "spider contact costs exactly 1 life, same as ObjectType2/3");

        // Matched on position, not just type -- the sample world has its
        // own real ObjectType16 placements elsewhere, and completeDeathLock()'s
        // fast-forward can let some other periodic spawn reuse this
        // injected spider's now-inactive slot, so a blind "last matching
        // type16 wins" search could find one of those real placements
        // instead (same false-positive shape flagged elsewhere this file).
        bool spiderStillActiveAtSamePos = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType16 && obj.currentX == 5.0f && obj.currentY == 1.0f &&
                obj.currentZ == 5.0f)
            {
                spiderStillActiveAtSamePos = true;
                break;
            }
        }
        check(!spiderStillActiveAtSamePos, "the spider that killed Blupi is destroyed, same as ObjectType2/3");
    }

    // 7.5. Fish (ObjectType17) contact-kill triggers BigShake, NOT
    // SmallShake -- the one real exception in the generic-hazard list
    // (plan.md CAM-008/009, Decor.cpp:5820-5823, confirmed via direct
    // source read: fish/bird specifically play BigShake, every other
    // hazard type plays SmallShake).
    {
        MobileObjSpec fish;
        fish.type = ObjectType::ObjectType17;
        fish.posStartX = fish.posEndX = fish.currentX = 6.0f;
        fish.posStartY = fish.posEndY = fish.currentY = 1.0f;
        fish.posStartZ = fish.posEndZ = fish.currentZ = 6.0f;
        world.GetMobileObjectsMutable().push_back(fish);

        interaction.Update(dt, world, 6.0f, 1.0f, 6.0f, 0.0f, sound);
        check(interaction.DiedThisFrame(), "DiedThisFrame() is true touching an injected fish (ObjectType17)");
        check(interaction.BigShakeTriggeredThisFrame(),
              "fish contact-kill triggers BigShake specifically, not SmallShake");
        check(!interaction.SmallShakeTriggeredThisFrame(),
              "fish contact-kill does NOT also trigger SmallShake the same frame");
        // Real explosion flash (plan.md VISUAL-008): ObjectType10 for
        // fish/bird specifically, matching the BigShake split exactly.
        int fishFlashCount = 0;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType10 && obj.currentX == 6.0f && obj.currentY == 1.0f &&
                obj.currentZ == 6.0f)
            {
                ++fishFlashCount;
            }
        }
        check(fishFlashCount == 1, "fish contact-kill spawns an ObjectType10 explosion flash, not ObjectType8");
    }

    // 8. Wasp (ObjectType44) balloon status and its hazard-pop interaction
    // (plan.md E3D-MIG-135) -- none of these types are placed in the real
    // sample world either, so all 3 are injected directly.
    {
        MobileObjSpec wasp;
        wasp.type = ObjectType::ObjectType44;
        wasp.posStartX = wasp.posEndX = wasp.currentX = 10.0f;
        wasp.posStartY = wasp.posEndY = wasp.currentY = 1.0f;
        wasp.posStartZ = wasp.posEndZ = wasp.currentZ = 10.0f;
        world.GetMobileObjectsMutable().push_back(wasp);

        const int livesBeforeWasp = interaction.Lives();
        interaction.Update(dt, world, 10.0f, 1.0f, 10.0f, 0.0f, sound);
        check(interaction.BalloonTouchedThisFrame(), "BalloonTouchedThisFrame() is true touching a wasp");
        check(!interaction.DiedThisFrame(), "touching a wasp does not kill Blupi");
        check(interaction.Lives() == livesBeforeWasp, "touching a wasp costs no life");
        bool waspStillActive = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType44) waspStillActive = obj.active;
        }
        check(waspStillActive, "the wasp itself is not destroyed by contact (unlike the shared kill list)");

        // Follower (96) -- one of the 4 real balloon-poppable types
        // (IsBalloonPoppableHazard(), which covers both the dormant 96 and
        // awake 97 state identically): while ballooned, contact pops the
        // balloon instead of killing, and does NOT destroy the follower
        // either (the real Decor.cpp:5766-5781 pop branch has no
        // ObjectDelete call at all). Placed at Y=20 -- well above the
        // sample world's real Y range [0,13] -- so it (and the touching
        // Blupi position below) can never coincide with real solid terrain;
        // since kFollowerWakeRadius > kHazardContactRadius, touching a
        // dormant follower always wakes it too (plan.md E3D-MIG-137), so
        // this contact is expected to also promote it to 97 before the pop
        // check runs, same frame.
        MobileObjSpec follower;
        follower.type = ObjectType::ObjectType96;
        follower.posStartX = follower.posEndX = follower.currentX = 15.0f;
        follower.posStartY = follower.posEndY = follower.currentY = 20.0f;
        follower.posStartZ = follower.posEndZ = follower.currentZ = 15.0f;
        world.GetMobileObjectsMutable().push_back(follower);

        const int livesBeforeFollower = interaction.Lives();
        interaction.Update(dt, world, 15.0f, 20.0f, 15.0f, 0.0f, sound, /*blupiCrouching=*/false, /*blupiBallooned=*/true);
        check(interaction.BalloonPoppedThisFrame(), "BalloonPoppedThisFrame() is true touching a follower while ballooned");
        check(!interaction.DiedThisFrame(), "the pop happens instead of a kill while ballooned");
        check(interaction.Lives() == livesBeforeFollower, "a popped balloon costs no life");
        bool followerStillActive = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if ((obj.type == ObjectType::ObjectType96 || obj.type == ObjectType::ObjectType97) &&
                obj.posStartX == 15.0f && obj.currentZ == 15.0f)
            {
                followerStillActive = obj.active;
            }
        }
        check(followerStillActive, "the follower that popped the balloon is NOT destroyed (real behavior has no ObjectDelete here)");

        // Bulldozer (4) -- NOT in IsBalloonPoppableHazard()'s 4-type subset
        // -- still kills even while ballooned, per the real source's
        // if/else-if chain (the pop check only ever matches 3/16/96/97).
        MobileObjSpec bulldozer;
        bulldozer.type = ObjectType::ObjectType4;
        bulldozer.posStartX = bulldozer.posEndX = bulldozer.currentX = 20.0f;
        bulldozer.posStartY = bulldozer.posEndY = bulldozer.currentY = 1.0f;
        bulldozer.posStartZ = bulldozer.posEndZ = bulldozer.currentZ = 20.0f;
        world.GetMobileObjectsMutable().push_back(bulldozer);

        // Lives() may already be down to 1 from earlier sections in this
        // same shared `interaction` instance -- a real life lost here can
        // therefore legitimately wrap back to 3 via the same game-over
        // reset already verified earlier, not just decrement by 1.
        const int livesBeforeBulldozer = interaction.Lives();
        const int gameOverCountBeforeBulldozer = interaction.GameOverCount();
        interaction.Update(dt, world, 20.0f, 1.0f, 20.0f, 0.0f, sound, /*blupiCrouching=*/false, /*blupiBallooned=*/true);
        check(interaction.DiedThisFrame(), "bulldozer (type 4) still kills Blupi even while ballooned");
        GEBlupiController bulldozerDeathBlupi;
        completeDeathLock(world, interaction, bulldozerDeathBlupi);
        const bool bulldozerCostALife =
            (interaction.Lives() == livesBeforeBulldozer - 1) ||
            (interaction.GameOverCount() == gameOverCountBeforeBulldozer + 1 && interaction.Lives() == 3);
        check(bulldozerCostALife, "bulldozer contact costs 1 life despite blupiBallooned=true (accounting for a possible game-over wrap)");
    }

    // 9. Real shared patrol-turn mechanic (plan.md E3D-MIG-131) -- a
    // synthetic patrol object with a symmetric 4-phase cycle (dwell 20
    // ticks / advance 20 ticks / dwell 20 ticks / recede 20 ticks, each
    // ~1s = 60 frames at dt=1/60, full cycle 240 frames) so both the
    // "resting at posEnd" dwell and the "resting at posStart" dwell give
    // wide, timing-forgiving sampling windows rather than needing a
    // frame-exact checkpoint on a instantaneous transition. Blupi is kept
    // far away (999,999,999) throughout so no contact/hazard/pickup check
    // ever fires and interferes with the position readings.
    {
        MobileObjSpec patroller;
        patroller.type = ObjectType::ObjectType17; // fish -- type doesn't matter, only its position does here
        patroller.posStartX = 0.0f; patroller.posEndX = 10.0f;
        patroller.posStartY = patroller.posEndY = 1.0f;
        patroller.posStartZ = patroller.posEndZ = 30.0f; // a Z not used by anything else in the sample world
        patroller.currentX = patroller.posStartX;
        patroller.currentY = patroller.posStartY;
        patroller.currentZ = patroller.posStartZ;
        patroller.stepAdvanceTicks = 20.0f;
        patroller.stepRecedeTicks = 20.0f;
        patroller.timeStopStartTicks = 20.0f;
        patroller.timeStopEndTicks = 20.0f;
        world.GetMobileObjectsMutable().push_back(patroller);

        const auto getPatrollerX = [&world]() -> float
        {
            for (const auto& obj : world.GetMobileObjects())
            {
                if (obj.type == ObjectType::ObjectType17 && obj.posStartX == 0.0f && obj.posEndX == 10.0f)
                {
                    return obj.currentX;
                }
            }
            return -999.0f;
        };

        check(getPatrollerX() == 0.0f, "patrol object starts at posStartX");

        // Frame 150 of a 240-frame cycle (dwell-start [0,60), advance
        // [60,120), dwell-end [120,180)) falls solidly inside the
        // dwell-end window -- comfortably past the moment it first
        // reaches posEndX, comfortably before it starts receding.
        for (int i = 0; i < 150; ++i)
        {
            interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound);
        }
        const float xAtDwellEnd = getPatrollerX();
        std::cout << "Patrol object X at frame 150 (mid dwell-end): " << xAtDwellEnd << " (posEnd=10)" << std::endl;
        check(xAtDwellEnd > 9.0f, "patrol object reaches and holds at posEndX during its dwell-end window");

        // Frame 300 total = frame 60 of a fresh 240-frame cycle -- just
        // past dwell-start [0,60) ending, so it's back near posStartX
        // (advance has barely begun).
        for (int i = 0; i < 150; ++i)
        {
            interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound);
        }
        const float xAtDwellStart = getPatrollerX();
        std::cout << "Patrol object X at frame 300 (back at dwell-start): " << xAtDwellStart << " (posStart=0)" << std::endl;
        check(xAtDwellStart < 1.0f, "patrol object completes the full cycle and returns to posStartX");
    }

    // 10. Blupih (ObjectType32) stationary shooter (plan.md E3D-MIG-134) --
    // verified against Decor.cpp:8878-8886. A hand-carved "ledge over a
    // pit" test column (rather than relying on the sample world's
    // incidental terrain shape) proves the real downward SearchDistRight
    // raycast: air from y=1..14, solid floor at y=0, blupih placed at
    // y=15 should drop a real ObjectType23 that travels 14 cells down
    // (posEndY == 1, one cell above the floor -- SearchDistRight stops
    // just short of the wall it found, never inside it).
    {
        constexpr int kGX = 95, kGZ = 95; // far corner, unused by anything else in the sample world
        auto& mutableWorld = world.GetWorldMutable();
        mutableWorld.setBlock(kGX, 0, kGZ, Worlds::Block::make(BlockTypes::Ground));
        for (int gy = 1; gy <= 20; ++gy)
        {
            mutableWorld.setBlock(kGX, static_cast<std::uint16_t>(gy), kGZ, Worlds::Block::make(BlockTypes::Air));
        }
        const float bhX = static_cast<float>(kGX) - GEWorldRuntime::kWorldCenterX;
        const float bhZ = static_cast<float>(kGZ) - GEWorldRuntime::kWorldCenterZ;

        MobileObjSpec blupih;
        blupih.type = ObjectType::ObjectType32;
        blupih.posStartX = bhX; blupih.posEndX = bhX + 0.001f; // nonzero delta only to satisfy the patrol gate
        blupih.posStartY = blupih.posEndY = 15.0f;
        blupih.posStartZ = blupih.posEndZ = bhZ;
        blupih.currentX = bhX; blupih.currentY = 15.0f; blupih.currentZ = bhZ;
        world.GetMobileObjectsMutable().push_back(blupih);

        // Matched on posStartX/Z, not just type -- the sample world now
        // also places a REAL blupih (plan.md E3D-MIG-134, tools/
        // GenerateSampleWorld3D.cpp's "turret perch") that can fire its own
        // projectile within this same ~70-frame window, so a global
        // ObjectType23 count would be flaky.
        const auto countBulletsAt = [&world](float x, float z)
        {
            int n = 0;
            for (const auto& obj : world.GetMobileObjects())
            {
                if (obj.type == ObjectType::ObjectType23 && obj.active && obj.posStartX == x && obj.posStartZ == z)
                {
                    ++n;
                }
            }
            return n;
        };
        const int bulletCountBefore = countBulletsAt(bhX, bhZ);

        // timeStopStartTicks defaults to 40 (> 21), so dwell-frame 21 is
        // crossed well before the object could ever leave patrolStep 1 --
        // 70 frames at dt=1/60 (20 ticks/s) covers ~23.3 ticks.
        for (int i = 0; i < 70; ++i)
        {
            interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound);
        }

        const MobileObjSpec* bullet = nullptr;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType23 && obj.active && obj.posStartZ == bhZ && obj.posStartX == bhX)
            {
                bullet = &obj;
            }
        }
        check(bullet != nullptr, "blupih dropped a real ObjectType23 projectile at dwell-frame 21");
        if (bullet)
        {
            std::cout << "Blupih bullet posEndY: " << bullet->posEndY << " (expected 1, floor at y=0)" << std::endl;
            check(std::fabs(bullet->posEndY - 1.0f) < 0.01f,
                  "blupih's projectile travel distance matches the real grid raycast (lands at y=1, just above the floor)");
        }

        // Contact-kill: the bullet is now a live patrolStep==2 object,
        // barely moved off Y=15 yet -- positioning Blupi right where it
        // spawned should register a kill this frame.
        const int livesBeforeBullet = interaction.Lives();
        interaction.Update(dt, world, bhX, 15.0f, bhZ, 0.0f, sound);
        check(interaction.DiedThisFrame(), "blupih's projectile is fatal on contact");

        const int bulletCountAfterContact = countBulletsAt(bhX, bhZ);
        check(bulletCountAfterContact == bulletCountBefore, "the projectile that killed Blupi is destroyed (no longer active)");

        // Bullet-hit splat effect (plan.md VISUAL-009) -- real
        // StartSploutchGlu() scatters 7 instances (1x ObjectType98, 4x99,
        // 2x100) within a few real px of the bullet's own position at the
        // moment of contact. Checked BEFORE completeDeathLock() below --
        // these transient splat objects self-delete well within its
        // multi-second fast-forward.
        int splatCount98 = 0, splatCount99 = 0, splatCount100 = 0;
        for (const auto& obj : world.GetMobileObjects())
        {
            const float sdx = obj.currentX - bhX, sdy = obj.currentY - 15.0f, sdz = obj.currentZ - bhZ;
            if (obj.active && sdx * sdx + sdy * sdy + sdz * sdz < 1.0f)
            {
                if (obj.type == ObjectType::ObjectType98) ++splatCount98;
                else if (obj.type == ObjectType::ObjectType99) ++splatCount99;
                else if (obj.type == ObjectType::ObjectType100) ++splatCount100;
            }
        }
        check(splatCount98 == 1 && splatCount99 == 4 && splatCount100 == 2,
              "the bullet contact-kill spawns the real 7-instance splat effect (1x ObjectType98, 4x99, 2x100)");

        // Real life loss is now deferred behind the death-lock/life-loss-
        // Voyage (death-VFX follow-up) -- checked last, see the generic-
        // hazard test above for why this must come after every immediate-
        // effect check.
        GEBlupiController bulletDeathBlupi;
        completeDeathLock(world, interaction, bulletDeathBlupi);
        const bool bulletCostALife =
            (interaction.Lives() == livesBeforeBullet - 1) || (interaction.Lives() == 3 && livesBeforeBullet <= 1);
        check(bulletCostALife, "blupih's projectile contact costs exactly 1 life");

        // "No room" cancellation: a second blupih placed directly on solid
        // ground (nothing but the floor immediately below it) should NOT
        // drop a visible projectile at all -- real ObjectStart still
        // returns a valid slot on this path (see SearchAirDistance's own
        // comment), it's just immediately voided.
        MobileObjSpec blupihFlush;
        blupihFlush.type = ObjectType::ObjectType32;
        blupihFlush.posStartX = bhX; blupihFlush.posEndX = bhX + 0.001f;
        blupihFlush.posStartY = blupihFlush.posEndY = 1.0f; // directly above the y=0 floor, no gap
        blupihFlush.posStartZ = blupihFlush.posEndZ = bhZ;
        blupihFlush.currentX = bhX; blupihFlush.currentY = 1.0f; blupihFlush.currentZ = bhZ;
        world.GetMobileObjectsMutable().push_back(blupihFlush);

        for (int i = 0; i < 70; ++i)
        {
            interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound);
        }
        // Matches on posStartX/Z too, not just Y -- the object exhibition
        // (tools/GenerateSampleWorld3D.cpp) places one static (posStart==
        // posEnd, non-fired) exhibit per real ObjectType with a non-zero
        // icon, and ObjectType23 (icon 176) is one of them, incidentally
        // also at Y=1 elsewhere in the world -- a Y-only filter would
        // wrongly match that unrelated static display item.
        int newBulletsFromFlush = 0;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType23 && obj.active &&
                obj.posStartY == 1.0f && obj.posStartX == bhX && obj.posStartZ == bhZ)
            {
                ++newBulletsFromFlush;
            }
        }
        check(newBulletsFromFlush == 0, "a blupih flush against solid ground drops no projectile (no room to fall)");
    }

    // 11. Blupit (ObjectType33) stationary shooter (plan.md E3D-MIG-134) --
    // verified against Decor.cpp:8928-8969. A hand-carved corridor with
    // asymmetric wall distances (14 cells left, 9 right) lets the two
    // real shots (dwell-frame 3 away from the upcoming walk direction,
    // dwell-frame 21 toward it) be told apart unambiguously by their
    // landing X.
    {
        constexpr int kCorridorGY = 1, kCorridorGZ = 40;
        constexpr int kMidGX = 85, kLeftWallGX = 70, kRightWallGX = 95;
        auto& mutableWorld = world.GetWorldMutable();
        for (int gx = kLeftWallGX + 1; gx < kRightWallGX; ++gx)
        {
            mutableWorld.setBlock(static_cast<std::uint16_t>(gx), kCorridorGY, kCorridorGZ,
                                   Worlds::Block::make(BlockTypes::Air));
        }
        mutableWorld.setBlock(kLeftWallGX, kCorridorGY, kCorridorGZ, Worlds::Block::make(BlockTypes::Ground));
        mutableWorld.setBlock(kRightWallGX, kCorridorGY, kCorridorGZ, Worlds::Block::make(BlockTypes::Ground));

        const float btX = static_cast<float>(kMidGX) - GEWorldRuntime::kWorldCenterX;
        const float btZ = static_cast<float>(kCorridorGZ) - GEWorldRuntime::kWorldCenterZ;

        MobileObjSpec blupit;
        blupit.type = ObjectType::ObjectType33;
        // posStartX < posEndX && patrolStep starts at 1 -> aboutToWalkRight
        // == true -> frame 3 fires LEFT (away), frame 21 fires RIGHT (toward).
        blupit.posStartX = btX; blupit.posEndX = btX + 0.001f;
        blupit.posStartY = blupit.posEndY = static_cast<float>(kCorridorGY);
        blupit.posStartZ = blupit.posEndZ = btZ;
        blupit.currentX = btX; blupit.currentY = static_cast<float>(kCorridorGY); blupit.currentZ = btZ;
        world.GetMobileObjectsMutable().push_back(blupit);

        for (int i = 0; i < 70; ++i)
        {
            interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound);
        }

        const MobileObjSpec* leftBullet = nullptr;
        const MobileObjSpec* rightBullet = nullptr;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType23 && obj.active && obj.posStartZ == btZ && obj.posStartX == btX)
            {
                if (obj.posEndX < btX) leftBullet = &obj; else rightBullet = &obj;
            }
        }
        check(leftBullet != nullptr, "blupit's dwell-frame-3 shot (away from upcoming travel) exists, fired LEFT");
        check(rightBullet != nullptr, "blupit's dwell-frame-21 shot (toward upcoming travel) exists, fired RIGHT");
        if (leftBullet)
        {
            std::cout << "Blupit left-shot posEndX: " << leftBullet->posEndX << " (expected " << (btX - 14.0f) << ")" << std::endl;
            check(std::fabs(leftBullet->posEndX - (btX - 14.0f)) < 0.01f,
                  "left-shot travels the real raycast distance to the left wall (14 cells)");
        }
        if (rightBullet)
        {
            std::cout << "Blupit right-shot posEndX: " << rightBullet->posEndX << " (expected " << (btX + 9.0f) << ")" << std::endl;
            check(std::fabs(rightBullet->posEndX - (btX + 9.0f)) < 0.01f,
                  "right-shot travels the real raycast distance to the right wall (9 cells)");
        }
    }

    // 12. Large creature (ObjectType54) turn-dwell-gated lethality (plan.md
    // E3D-MIG-136) -- verified against Decor.cpp:5867-5913. Injected
    // directly with patrolStep manually forced to each phase (rather than
    // waiting out a real patrol cycle), since only the phase at contact
    // time matters, not how it got there.
    {
        MobileObjSpec creature;
        creature.type = ObjectType::ObjectType54;
        creature.posStartX = creature.currentX = 60.0f;
        creature.posStartY = creature.currentY = 1.0f;
        creature.posStartZ = creature.currentZ = 60.0f;
        creature.posEndX = 60.0f; // posStart==posEnd is fine here -- this test
        creature.posEndY = 1.0f;  // drives patrolStep by hand, not via
        creature.posEndZ = 60.0f; // AdvancePatrolStep()'s own advance logic.
        world.GetMobileObjectsMutable().push_back(creature);

        const auto findCreature = [&world]() -> MobileObjSpec*
        {
            for (auto& obj : world.GetMobileObjectsMutable())
            {
                if (obj.type == ObjectType::ObjectType54 && obj.posStartX == 60.0f)
                {
                    return &obj;
                }
            }
            return nullptr;
        };

        // Mid-walk (patrolStep 2): contact is completely safe.
        if (auto* c = findCreature()) c->patrolStep = 2;
        int livesBefore = interaction.Lives();
        interaction.Update(dt, world, 60.0f, 1.0f, 60.0f, 0.0f, sound);
        check(!interaction.DiedThisFrame(), "large creature contact is safe while it's mid-walk (patrolStep 2)");
        check(interaction.Lives() == livesBefore, "no life lost touching the creature mid-walk");

        // Also safe mid-recede (patrolStep 4).
        if (auto* c = findCreature()) c->patrolStep = 4;
        livesBefore = interaction.Lives();
        interaction.Update(dt, world, 60.0f, 1.0f, 60.0f, 0.0f, sound);
        check(!interaction.DiedThisFrame(), "large creature contact is safe while it's mid-recede (patrolStep 4)");
        check(interaction.Lives() == livesBefore, "no life lost touching the creature mid-recede");

        // Turn-dwell (patrolStep 1): contact is lethal, and the creature
        // itself survives (unlike the shared kill-list types).
        if (auto* c = findCreature()) c->patrolStep = 1;
        livesBefore = interaction.Lives();
        const int gameOverBefore = interaction.GameOverCount();
        interaction.Update(dt, world, 60.0f, 1.0f, 60.0f, 0.0f, sound);
        check(interaction.DiedThisFrame(), "large creature contact is lethal during turn-dwell (patrolStep 1)");
        GEBlupiController turnDwellDeathBlupi;
        completeDeathLock(world, interaction, turnDwellDeathBlupi);
        const bool costALife =
            (interaction.Lives() == livesBefore - 1) ||
            (interaction.GameOverCount() == gameOverBefore + 1 && interaction.Lives() == 3);
        check(costALife, "turn-dwell contact costs exactly 1 life (accounting for a possible game-over wrap)");
        const auto* afterDwell = findCreature();
        check(afterDwell != nullptr && afterDwell->active,
              "the large creature is NOT destroyed by the contact that killed Blupi (unlike the shared kill list)");

        // Also lethal at the other dwell (patrolStep 3).
        if (auto* c = findCreature()) c->patrolStep = 3;
        livesBefore = interaction.Lives();
        interaction.Update(dt, world, 60.0f, 1.0f, 60.0f, 0.0f, sound);
        check(interaction.DiedThisFrame(), "large creature contact is also lethal at patrolStep 3 (the other dwell)");

        // Balloon immunity (real `!m_blupiBalloon` gate) -- while
        // ballooned, contact during turn-dwell does nothing at all (no
        // kill, no pop -- unlike the 4 balloon-poppable hazard types).
        if (auto* c = findCreature()) c->patrolStep = 1;
        livesBefore = interaction.Lives();
        interaction.Update(dt, world, 60.0f, 1.0f, 60.0f, 0.0f, sound, /*blupiCrouching=*/false, /*blupiBallooned=*/true);
        check(!interaction.DiedThisFrame(), "large creature contact is harmless during turn-dwell while ballooned");
        check(!interaction.BalloonPoppedThisFrame(), "large creature contact does not pop the balloon either (no pop path for type 54)");
        check(interaction.Lives() == livesBefore, "no life lost touching the creature during turn-dwell while ballooned");
    }

    // 13. Real playable placement -- the sample world's own large creature
    // (tools/GenerateSampleWorld3D.cpp's "walled room" guardian) has a real
    // posStart != posEnd patrol path, unlike the old zero-range placement
    // this replaced (same real guard the platform lift needed, see
    // AdvancePatrolStep()'s own comment). Selected by posStartX != posEndX,
    // NOT plain findFirst() -- the object exhibition area also places a
    // deliberately-static ObjectType54 specimen (posStart==posEnd), and
    // CollectMoveObjects' ordering is spatial, so "first ObjectType54"
    // could find that one instead (the exact pitfall findPatrollingLift's
    // own comment above already documents for ObjectType1).
    const MobileObjSpec* placedCreature = nullptr;
    for (const auto& obj : world.GetMobileObjects())
    {
        if (obj.type == ObjectType::ObjectType54 && obj.posStartX != obj.posEndX)
        {
            placedCreature = &obj;
            break;
        }
    }
    check(placedCreature != nullptr,
          "found a patrolling large creature (ObjectType54, posStartX != posEndX) in the sample world");

    // 14. Follower (ObjectType96/97) wake + homing + blocked-path self-
    // destruct (plan.md E3D-MIG-137) -- verified against Decor.cpp:
    // 9646-9678 (the wake box) and 8025-8064 (the homing step). Both
    // scenarios use grid Y=20, well above the sample world's real Y range
    // [0,13], and a Z unused by anything else, so nothing pre-existing can
    // interfere.
    {
        // 14a. Wake + gradual homing progress in open air: a dormant
        // follower placed 1.5 grid units from Blupi (within
        // kFollowerWakeRadius ~2.06, so it wakes on contact) should wake
        // (type -> 97) and creep toward Blupi over many small per-frame
        // steps -- NOT teleport there in one frame (real speed is a slow
        // 1 real px/tick).
        constexpr int kGX = 10, kGY = 20, kGZ = 95;
        const float fX = static_cast<float>(kGX) - GEWorldRuntime::kWorldCenterX;
        const float fY = static_cast<float>(kGY);
        const float fZ = static_cast<float>(kGZ) - GEWorldRuntime::kWorldCenterZ;
        const float targetX = fX + 1.5f; // within wake radius, open air the whole way

        MobileObjSpec dormant;
        dormant.type = ObjectType::ObjectType96;
        dormant.posStartX = dormant.posEndX = dormant.currentX = fX;
        dormant.posStartY = dormant.posEndY = dormant.currentY = fY;
        dormant.posStartZ = dormant.posEndZ = dormant.currentZ = fZ;
        world.GetMobileObjectsMutable().push_back(dormant);

        // Matched by currentZ alone (invariant -- homing never touches Z),
        // NOT posStartX, which the homing step itself overwrites every
        // frame it moves.
        const auto findFollowerAt = [&world](float z) -> const MobileObjSpec*
        {
            for (const auto& obj : world.GetMobileObjects())
            {
                if ((obj.type == ObjectType::ObjectType96 || obj.type == ObjectType::ObjectType97) &&
                    obj.currentZ == z)
                {
                    return &obj;
                }
            }
            return nullptr;
        };

        interaction.Update(dt, world, targetX, fY, fZ, 0.0f, sound);
        const auto* afterFirstFrame = findFollowerAt(fZ);
        check(afterFirstFrame != nullptr && afterFirstFrame->type == ObjectType::ObjectType97,
              "a dormant follower (96) wakes into the homing type (97) once Blupi is within its wake box");

        for (int i = 0; i < 59; ++i)
        {
            interaction.Update(dt, world, targetX, fY, fZ, 0.0f, sound);
        }
        const auto* afterOneSecond = findFollowerAt(fZ);
        std::cout << "Follower X after 1s homing: " << (afterOneSecond ? afterOneSecond->currentX : -999.0f)
                  << " (started at " << fX << ", target " << targetX << ")" << std::endl;
        check(afterOneSecond != nullptr && afterOneSecond->active,
              "the homing follower is still alive after 1s of unobstructed homing");
        if (afterOneSecond)
        {
            const float advanced = afterOneSecond->currentX - fX;
            check(advanced > 0.2f && advanced < 0.4f,
                  "the follower creeps toward Blupi at the real ~0.3125 grid-units/sec homing speed, not instantly");
            check(afterOneSecond->currentX < targetX,
                  "the follower has NOT yet reached Blupi after 1s (real speed is slow, not a teleport)");
        }
    }
    {
        // 14b. Blocked-path self-destruct: a follower already awake (97),
        // one grid unit from a solid wall with Blupi positioned beyond it,
        // should self-destruct (active -> false) the moment its next step
        // would land inside the wall, rather than passing through it.
        constexpr int kGX = 10, kGY = 20, kGZ = 90, kWallGX = 11;
        auto& mutableWorld = world.GetWorldMutable();
        mutableWorld.setBlock(static_cast<std::uint16_t>(kWallGX), static_cast<std::uint16_t>(kGY),
                               static_cast<std::uint16_t>(kGZ), Worlds::Block::make(BlockTypes::Ground));

        const float fX = static_cast<float>(kGX) - GEWorldRuntime::kWorldCenterX;
        const float fY = static_cast<float>(kGY);
        const float fZ = static_cast<float>(kGZ) - GEWorldRuntime::kWorldCenterZ;
        const float beyondWallX = static_cast<float>(kWallGX + 5) - GEWorldRuntime::kWorldCenterX;

        MobileObjSpec homing;
        homing.type = ObjectType::ObjectType97; // already awake
        homing.posStartX = homing.posEndX = homing.currentX = fX;
        homing.posStartY = homing.posEndY = homing.currentY = fY;
        homing.posStartZ = homing.posEndZ = homing.currentZ = fZ;
        world.GetMobileObjectsMutable().push_back(homing);

        // Identified by type97 + this scenario's unique fZ (distinct from
        // 14a's fZ above) -- posStartZ never changes for this object
        // (homing only ever touches X/Y), so this stays a valid match even
        // after self-destruct sets active=false.
        const auto findHoming = [&world, fZ]() -> const MobileObjSpec*
        {
            for (const auto& obj : world.GetMobileObjects())
            {
                if (obj.type == ObjectType::ObjectType97 && obj.posStartZ == fZ)
                {
                    return &obj;
                }
            }
            return nullptr;
        };

        bool selfDestructed = false;
        int framesToDestruct = -1;
        float destructX = 0.0f, destructY = 0.0f, destructZ = 0.0f;
        for (int i = 0; i < 250 && !selfDestructed; ++i)
        {
            interaction.Update(dt, world, beyondWallX, fY, fZ, 0.0f, sound);
            const auto* current = findHoming();
            if (current != nullptr && !current->active)
            {
                selfDestructed = true;
                framesToDestruct = i;
                destructX = current->currentX;
                destructY = current->currentY;
                destructZ = current->currentZ;
            }
        }
        std::cout << "Follower self-destructed after " << framesToDestruct << " frame(s) approaching the wall" << std::endl;
        check(selfDestructed, "a homing follower self-destructs when its next step would land inside solid terrain");
        check(interaction.SmallShakeTriggeredThisFrame(),
              "the follower's blocked-path self-destruct triggers SmallShake (plan.md CAM-008, a site the "
              "earlier camera-shake audit missed)");

        // Real debris flash (plan.md VISUAL-008, ObjectType9) spawned at
        // the follower's own position at the moment of self-destruct.
        int debrisFlashCount = 0;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType9 && obj.currentX == destructX &&
                obj.currentY == destructY && obj.currentZ == destructZ)
            {
                ++debrisFlashCount;
            }
        }
        check(debrisFlashCount == 1,
              "the follower's blocked-path self-destruct spawns an ObjectType9 debris flash at its position");
    }

    // 15. GEWorldRuntime::IsTempPassableAtPhase() (plan.md E3D-MIG-146) --
    // real IsPassIcon/IsBlocIcon(324) cycle: solid for buckets 0-17,
    // passable only for buckets 18-19 of a 20-value cycle at 4 phase
    // ticks/bucket (`m_time / 4 % 20 >= 18`), so the passable window is
    // phases 72-79 of every 80-phase cycle.
    check(!GEWorldRuntime::IsTempPassableAtPhase(0), "Temp solid at phase 0 (cycle start, bucket 0)");
    check(!GEWorldRuntime::IsTempPassableAtPhase(71), "Temp solid at phase 71 (bucket 17, just before the window)");
    check(GEWorldRuntime::IsTempPassableAtPhase(72), "Temp passable at phase 72 (bucket 18, window start)");
    check(GEWorldRuntime::IsTempPassableAtPhase(79), "Temp passable at phase 79 (bucket 19, window end)");
    check(!GEWorldRuntime::IsTempPassableAtPhase(80), "Temp solid at phase 80 (cycle wraps back to bucket 0)");

    // 16. GEWorldRuntime::FindTeleportDestination() (plan.md E3D-MIG-147) --
    // a hand-carved pair of pillars far apart (unused Y=20, well above the
    // sample world's real Y range [0,13]), plus a lone pillar with no
    // partner anywhere, to prove both the successful-match and no-match
    // (real "regains control in place") paths. Uses arbitrary non-real
    // icon values (998/999, outside both the real 330-333 teleporter
    // range and the tile exhibition's 1..440 coverage) rather than real
    // Teleport* constants -- the sample world now has a REAL matched
    // Teleport1 pair of its own (tools/GenerateSampleWorld3D.cpp's
    // teleporter rooms), which would otherwise be found instead of this
    // test's own synthetic pillars. FindTeleportDestination() itself
    // doesn't care whether the value is a real teleporter icon; it just
    // scans for another cell of the same type.
    {
        auto& mutableWorld = world.GetWorldMutable();
        constexpr std::uint16_t kTestIcon = 998;
        constexpr int kEntryGX = 85, kEntryGY = 20, kEntryGZ = 10;
        constexpr int kExitGX = 15, kExitGY = 20, kExitGZ = 60;
        mutableWorld.setBlock(kEntryGX, kEntryGY, kEntryGZ, Worlds::Block::make(kTestIcon));
        mutableWorld.setBlock(kExitGX, kExitGY, kExitGZ, Worlds::Block::make(kTestIcon));

        // Blupi's position when he triggered: one cell BELOW the entry
        // pillar (kEntryGY - 1), matching the real "stands in the open
        // space beneath it" relationship (GEBlupiController::
        // GetBlockTypeAbove()), not at the pillar's own height.
        const float entryX = static_cast<float>(kEntryGX) - GEWorldRuntime::kWorldCenterX;
        const float entryY = static_cast<float>(kEntryGY - 1);
        const float entryZ = static_cast<float>(kEntryGZ) - GEWorldRuntime::kWorldCenterZ;

        float destX = -999.0f, destY = -999.0f, destZ = -999.0f;
        const bool found = world.FindTeleportDestination(kTestIcon, entryX, entryY, entryZ,
                                                            destX, destY, destZ);
        check(found, "FindTeleportDestination() finds the paired pillar elsewhere in the grid");
        const float expectedDestX = static_cast<float>(kExitGX) - GEWorldRuntime::kWorldCenterX;
        const float expectedDestY = static_cast<float>(kExitGY - 1);
        const float expectedDestZ = static_cast<float>(kExitGZ) - GEWorldRuntime::kWorldCenterZ + 1.0f;
        std::cout << "Teleport destination: (" << destX << "," << destY << "," << destZ << ") expected ("
                  << expectedDestX << "," << expectedDestY << "," << expectedDestZ << ")" << std::endl;
        check(std::fabs(destX - expectedDestX) < 0.01f && std::fabs(destY - expectedDestY) < 0.01f &&
                  std::fabs(destZ - expectedDestZ) < 0.01f,
              "the destination is one cell BELOW the exit pillar in Y and one cell +Z away from directly "
              "beneath it, not the entry pillar and not an instant-re-trigger position");

        // No match anywhere -- a lone pillar with no partner. Uses an
        // arbitrary non-real icon value (999, well outside the real
        // 330-333 teleporter range and the tile exhibition's own 1..440
        // coverage, but still within Block's 12-bit type range) rather
        // than a real Teleport* constant, since the sample world's tile
        // exhibition already places exactly one specimen of every real
        // icon 1..440 (including all 4 real teleporter icons) -- a real
        // icon would never actually be "lone" in this loaded world.
        // FindTeleportDestination() itself doesn't care whether the value
        // is a real teleporter icon; it just scans for another cell of the
        // same type.
        constexpr std::uint16_t kLonelyIcon = 999;
        mutableWorld.setBlock(50, 20, 50, Worlds::Block::make(kLonelyIcon));
        float lonelyDestX, lonelyDestY, lonelyDestZ;
        const bool foundLonely = world.FindTeleportDestination(kLonelyIcon, 0.0f, 20.0f, 0.0f,
                                                                  lonelyDestX, lonelyDestY, lonelyDestZ);
        check(!foundLonely, "FindTeleportDestination() returns false for a lone teleporter with no partner anywhere");
    }

    // 17. GEWorldRuntime::TryConsumeFan() (plan.md E3D-MIG-149) -- a
    // synthetic FanLeft placed at an unused Y=20 height, well clear of the
    // sample world's own 2 real fan placements (embedded in the tunnel
    // wall as pure visual/render confirmation, not yet a reachable
    // hazard placement). Uses the real BlockTypes::FanLeft constant, unlike
    // FindTeleportDestination()'s type-agnostic scan above -- TryConsumeFan()
    // checks against BlockTypes::isFan() specifically, so a fake icon value
    // wouldn't exercise it.
    {
        auto& mutableWorld = world.GetWorldMutable();
        constexpr int kFanGX = 40, kFanGY = 20, kFanGZ = 40;
        mutableWorld.setBlock(kFanGX, kFanGY, kFanGZ, Worlds::Block::make(BlockTypes::FanLeft));

        const float fanX = static_cast<float>(kFanGX) - GEWorldRuntime::kWorldCenterX;
        const float fanZ = static_cast<float>(kFanGZ) - GEWorldRuntime::kWorldCenterZ;
        const float belowFanY = static_cast<float>(kFanGY - 1); // one cell below, matching GetBlockTypeAbove()'s convention

        check(!world.TryConsumeFan(0.0f, 1.0f, 0.0f).has_value(),
              "TryConsumeFan() is a no-op away from any fan");
        check(!world.TryConsumeFan(fanX, belowFanY - 5.0f, fanZ).has_value(),
              "TryConsumeFan() is a no-op checking a different (non-fan) cell in the same column");

        const auto consumed = world.TryConsumeFan(fanX, belowFanY, fanZ);
        check(consumed.has_value() && *consumed == BlockTypes::FanLeft,
              "TryConsumeFan() detects and returns the fan head icon one cell above Blupi");
        check(mutableWorld.getBlock(static_cast<std::uint16_t>(kFanGX), static_cast<std::uint16_t>(kFanGY),
                                     static_cast<std::uint16_t>(kFanGZ))
                      .type() == BlockTypes::Air,
              "the fan tile is consumed (cleared to Air) on contact, real ModifDecor(pos, -1)");

        check(!world.TryConsumeFan(fanX, belowFanY, fanZ).has_value(),
              "TryConsumeFan() is a no-op the second time -- the fan is already consumed");
    }

    // 17.5. Fan-hit shockwave flash (plan.md VISUAL-008-adjacent,
    // ObjectType11) -- a single instance spawned exactly at the given
    // position (no offset), real phase>=9 self-delete, corrected
    // table_explo4 icon values.
    {
        GEWorldRuntime flashWorld;
        GEInteractionSystem flashInteraction;
        constexpr float fx = 12.0f, fy = 1.0f, fz = 12.0f;
        constexpr float dt = 1.0f / 20.0f; // matches the real 20Hz tick rate obj.phase advances at

        flashInteraction.SpawnFanHitFlash(flashWorld, fx, fy, fz);
        const auto countFlashesAt = [&flashWorld, fx, fy, fz]()
        {
            int count = 0;
            for (const auto& obj : flashWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType11 && obj.currentX == fx &&
                    obj.currentY == fy && obj.currentZ == fz)
                {
                    ++count;
                }
            }
            return count;
        };
        check(countFlashesAt() == 1, "SpawnFanHitFlash() spawns exactly 1 instance, exactly at the given position");

        for (int i = 0; i < 8; ++i)
        {
            flashWorld.Update(dt);
        }
        flashInteraction.Update(dt, flashWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 1, "the flash is still active just before its real phase-9 self-delete");

        flashWorld.Update(dt);
        flashInteraction.Update(dt, flashWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 0, "the flash self-deletes once phase reaches the real 9-tick lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-008-adjacent,
        // fixed 2026-07-14 -- real table_explo4 is non-monotonic, was
        // wrongly ascending arithmetic before).
        check(GetObjIcon(ObjectType::ObjectType11, 0) == 12, "ObjectType11 icon at phase=0 is the real table_explo4[0]=12");
        check(GetObjIcon(ObjectType::ObjectType11, 3) == 15, "ObjectType11 icon at phase=3 is the real table_explo4[3]=15");
        check(GetObjIcon(ObjectType::ObjectType11, 4) == 7, "ObjectType11 icon at phase=4 is the real table_explo4[4]=7 (the non-monotonic jump)");
        check(GetObjIcon(ObjectType::ObjectType11, 8) == 11, "ObjectType11 icon at phase=8 is the real table_explo4[8]=11 (last frame before self-delete)");

        // GetObjIcon()'s corrected formula (VISUAL-021, fixed 2026-07-20 --
        // real table_tentacule oscillates within icons 70-86 and never
        // overflows explo.png, unlike the naive ascending-range assumption
        // the previous frozen-frame-86 case used).
        check(GetObjIcon(ObjectType::ObjectType53, 0) == 86, "ObjectType53 icon at phase=0 is the real table_tentacule[0]=86");
        check(GetObjIcon(ObjectType::ObjectType53, 3) == 83, "ObjectType53 icon at phase=3 is the real table_tentacule[3]=83 (rise peak)");
        check(GetObjIcon(ObjectType::ObjectType53, 7) == -1, "ObjectType53 icon at phase=7 is the real table_tentacule[7]=-1 (blank at the peak)");
        check(GetObjIcon(ObjectType::ObjectType53, 24) == 70, "ObjectType53 icon at phase=24 is the real table_tentacule[24]=70 (full extension)");
        check(GetObjIcon(ObjectType::ObjectType53, 44) == -1, "ObjectType53 icon at phase=44 is the real table_tentacule[44]=-1 (fully retracted)");
        check(GetObjIcon(ObjectType::ObjectType53, 45) == GetObjIcon(ObjectType::ObjectType53, 0),
              "ObjectType53 icon wraps around after the real 45-frame table length");
    }

    // 17.6. Dynamite-blast explosion flash self-delete timing (plan.md
    // VISUAL-008, ObjectType8) -- the actual spawn (via the real
    // 9-blast dynamite-fuse sequence) is already exercised in 3.6 above;
    // this isolates the self-delete-at-phase-39 logic on its own with a
    // directly-constructed instance, since there is no public single-shot
    // spawn method for this type (unlike SpawnFanHitFlash()).
    {
        GEWorldRuntime explo1World;
        GEInteractionSystem explo1Interaction;
        constexpr float dt = 1.0f / 20.0f; // matches the real 20Hz tick rate obj.phase advances at
        constexpr float ex = 30.0f, ey = 1.0f, ez = 30.0f;

        MobileObjSpec flash;
        flash.type = ObjectType::ObjectType8;
        flash.active = true;
        flash.phase = 0.0f;
        flash.currentX = flash.posStartX = flash.posEndX = ex;
        flash.currentY = flash.posStartY = flash.posEndY = ey;
        flash.currentZ = flash.posStartZ = flash.posEndZ = ez;
        explo1World.GetMobileObjectsMutable().push_back(flash);

        const auto countFlashesAt = [&explo1World, ex, ey, ez]()
        {
            int count = 0;
            for (const auto& obj : explo1World.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType8 && obj.currentX == ex &&
                    obj.currentY == ey && obj.currentZ == ez)
                {
                    ++count;
                }
            }
            return count;
        };

        for (int i = 0; i < 38; ++i)
        {
            explo1World.Update(dt);
        }
        explo1Interaction.Update(dt, explo1World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 1, "the dynamite-blast flash is still active just before its real phase-39 self-delete");

        explo1World.Update(dt);
        explo1Interaction.Update(dt, explo1World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 0, "the dynamite-blast flash self-deletes once phase reaches the real 39-tick lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-008, fixed
        // 2026-07-14 -- real table_explo1 bounces back and forth between
        // adjacent values, was wrongly ascending arithmetic before).
        check(GetObjIcon(ObjectType::ObjectType8, 0) == 0, "ObjectType8 icon at phase=0 is the real table_explo1[0]=0");
        check(GetObjIcon(ObjectType::ObjectType8, 8) == 4, "ObjectType8 icon at phase=8 is the real table_explo1[8]=4");
        check(GetObjIcon(ObjectType::ObjectType8, 9) == 3, "ObjectType8 icon at phase=9 is the real table_explo1[9]=3 (bounces back down from 4)");
        check(GetObjIcon(ObjectType::ObjectType8, 38) == 11, "ObjectType8 icon at phase=38 is the real table_explo1[38]=11 (last frame before self-delete)");
    }

    // 17.6b. Fish/bird explosion flash self-delete timing + icon formula
    // (plan.md VISUAL-008, ObjectType10) -- the real spawn (via the
    // generic-hazard-kill/fish-BigShake sites) is already exercised in
    // tests 2.5/7.5 above; this isolates the self-delete-at-phase-20
    // logic on its own with a directly-constructed instance.
    {
        GEWorldRuntime explo3World;
        GEInteractionSystem explo3Interaction;
        constexpr float dt = 1.0f / 20.0f;
        constexpr float ex = 35.0f, ey = 1.0f, ez = 35.0f;

        MobileObjSpec flash;
        flash.type = ObjectType::ObjectType10;
        flash.active = true;
        flash.phase = 0.0f;
        flash.currentX = flash.posStartX = flash.posEndX = ex;
        flash.currentY = flash.posStartY = flash.posEndY = ey;
        flash.currentZ = flash.posStartZ = flash.posEndZ = ez;
        explo3World.GetMobileObjectsMutable().push_back(flash);

        const auto countFlashesAt = [&explo3World, ex, ey, ez]()
        {
            int count = 0;
            for (const auto& obj : explo3World.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType10 && obj.currentX == ex &&
                    obj.currentY == ey && obj.currentZ == ez)
                {
                    ++count;
                }
            }
            return count;
        };

        for (int i = 0; i < 19; ++i)
        {
            explo3World.Update(dt);
        }
        explo3Interaction.Update(dt, explo3World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 1, "the fish/bird flash is still active just before its real phase-20 self-delete");

        explo3World.Update(dt);
        explo3Interaction.Update(dt, explo3World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 0, "the fish/bird flash self-deletes once phase reaches the real 20-tick lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-008, fixed
        // 2026-07-14 -- real table_explo3 oscillates between 32/34/35, was
        // wrongly ascending arithmetic before).
        check(GetObjIcon(ObjectType::ObjectType10, 0) == 32, "ObjectType10 icon at phase=0 is the real table_explo3[0]=32");
        check(GetObjIcon(ObjectType::ObjectType10, 2) == 34, "ObjectType10 icon at phase=2 is the real table_explo3[2]=34");
        check(GetObjIcon(ObjectType::ObjectType10, 14) == 35, "ObjectType10 icon at phase=14 is the real table_explo3[14]=35 (the second oscillation phase)");
        check(GetObjIcon(ObjectType::ObjectType10, 19) == 35, "ObjectType10 icon at phase=19 is the real table_explo3[19]=35 (last frame before self-delete)");
    }

    // 17.6c. Follower-blocked-path debris flash self-delete timing + icon
    // formula (plan.md VISUAL-008, ObjectType9) -- the real spawn (via
    // the follower blocked-path self-destruct) is already exercised in
    // test 14b above; this isolates the self-delete-at-phase-20 logic and
    // the `-1` blank-frame icon values on their own.
    {
        GEWorldRuntime explo2World;
        GEInteractionSystem explo2Interaction;
        constexpr float dt = 1.0f / 20.0f;
        constexpr float ex = 45.0f, ey = 1.0f, ez = 45.0f;

        MobileObjSpec flash;
        flash.type = ObjectType::ObjectType9;
        flash.active = true;
        flash.phase = 0.0f;
        flash.currentX = flash.posStartX = flash.posEndX = ex;
        flash.currentY = flash.posStartY = flash.posEndY = ey;
        flash.currentZ = flash.posStartZ = flash.posEndZ = ez;
        explo2World.GetMobileObjectsMutable().push_back(flash);

        const auto countFlashesAt = [&explo2World, ex, ey, ez]()
        {
            int count = 0;
            for (const auto& obj : explo2World.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType9 && obj.currentX == ex &&
                    obj.currentY == ey && obj.currentZ == ez)
                {
                    ++count;
                }
            }
            return count;
        };

        for (int i = 0; i < 19; ++i)
        {
            explo2World.Update(dt);
        }
        explo2Interaction.Update(dt, explo2World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 1, "the debris flash is still active just before its real phase-20 self-delete");

        explo2World.Update(dt);
        explo2Interaction.Update(dt, explo2World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countFlashesAt() == 0, "the debris flash self-deletes once phase reaches the real 20-tick lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-008, fixed
        // 2026-07-14 -- real table_explo2 has real `-1` blank-frame
        // sentinels interspersed throughout, was wrongly ascending
        // arithmetic before).
        check(GetObjIcon(ObjectType::ObjectType9, 0) == 12, "ObjectType9 icon at phase=0 is the real table_explo2[0]=12");
        check(GetObjIcon(ObjectType::ObjectType9, 1) == -1, "ObjectType9 icon at phase=1 is the real table_explo2[1]=-1 (a blank frame)");
        check(GetObjIcon(ObjectType::ObjectType9, 19) == 13, "ObjectType9 icon at phase=19 is the real table_explo2[19]=13 (last frame before self-delete)");
    }

    // 17.7. Pollution puff (plan.md VISUAL-013, ObjectType36) -- vehicle
    // exhaust smoke, 4 vehicle-gated emission schedules, real posStart->
    // posEnd slide via the existing generic AdvancePatrolStep() machinery
    // (not a hand-rolled interpolation block, unlike Invert/treasure's
    // earlier fix), real phase>=16 self-delete.
    {
        constexpr float px = 20.0f, py = 1.0f, pz = 20.0f;
        constexpr float dt = 1.0f / 20.0f;
        constexpr float kReach = 500.0f / 64.0f;

        GEWorldRuntime noneWorld;
        GEInteractionSystem noneInteraction;
        for (int i = 0; i < 60; ++i)
        {
            noneInteraction.TickPollutionPuff(noneWorld, px, py, pz, false, false, false, false,
                                               /*isMoving=*/false, /*ascending=*/false, /*facingDX=*/1);
        }
        int noneCount = 0;
        for (const auto& obj : noneWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType36)
            {
                ++noneCount;
            }
        }
        check(noneCount == 0, "TickPollutionPuff() never spawns while not in any of the 4 vehicles");

        // Jeep, stationary, facing right -- real schedule hits at
        // blupiPhase%50 in {0,12,20,35}; the tick counter used as the real
        // m_blupiPhase stand-in starts at 1 on the first call (see
        // TickPollutionPuff()'s own header comment), so ticks 12/20/35/50
        // of a fresh 50-call run each hit exactly once.
        GEWorldRuntime jeepWorld;
        GEInteractionSystem jeepInteraction;
        for (int i = 0; i < 50; ++i)
        {
            jeepInteraction.TickPollutionPuff(jeepWorld, px, py, pz, false, false, /*isJeep=*/true, false,
                                               /*isMoving=*/false, /*ascending=*/false, /*facingDX=*/1);
        }
        int jeepCount = 0;
        const MobileObjSpec* jeepPuff = nullptr;
        for (const auto& obj : jeepWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType36)
            {
                ++jeepCount;
                jeepPuff = &obj;
            }
        }
        check(jeepCount == 4,
              "Jeep (stationary) spawns exactly 4 puffs across one real 50-tick schedule cycle "
              "(blupiPhase%50 in {0,12,20,35})");
        if (jeepPuff != nullptr)
        {
            constexpr float kJeepOffsetX = (32.0f - 5.0f) / 64.0f;
            check(std::fabs(jeepPuff->posStartX - (px - kJeepOffsetX)) < 0.01f && jeepPuff->posStartY == py,
                  "Jeep puff spawns at the real facing-right offset from Blupi (32px nozzle, -5px trailing "
                  "adjustment)");
            check(std::fabs(jeepPuff->posEndX - (jeepPuff->posStartX - kReach)) < 0.01f,
                  "Jeep puff's real posEnd is 500px/64 further LEFT (facing right negates num, drifting "
                  "backward)");
            check(std::fabs(jeepPuff->stepAdvanceTicks - 156.0f) < 0.5f,
                  "Jeep puff's real stepAdvance is |20*500/64|=156 ticks (magnitude-20 bucket)");
            check(jeepPuff->patrolStep == 2,
                  "Jeep puff starts directly in patrolStep=2 (real step=2, skips the dwell phase)");
        }
        else
        {
            check(false, "found at least one spawned Jeep puff to inspect");
        }

        // Facing LEFT flips the drift direction (real: no negation, `num`
        // stays positive -> right bucket).
        GEWorldRuntime jeepLeftWorld;
        GEInteractionSystem jeepLeftInteraction;
        for (int i = 0; i < 12; ++i)
        {
            jeepLeftInteraction.TickPollutionPuff(jeepLeftWorld, px, py, pz, false, false, true, false, false, false,
                                                   /*facingDX=*/-1);
        }
        const MobileObjSpec* jeepLeftPuff = nullptr;
        for (const auto& obj : jeepLeftWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType36)
            {
                jeepLeftPuff = &obj;
            }
        }
        if (jeepLeftPuff != nullptr)
        {
            check(jeepLeftPuff->posEndX > jeepLeftPuff->posStartX + kReach - 0.01f,
                  "facing LEFT drifts the puff to the real RIGHT bucket instead (no negation)");
        }
        else
        {
            check(false, "found a spawned Jeep puff (facing left) to inspect");
        }

        // Overcraft, ascending -- real num=58 (>50 bucket, always downward
        // regardless of facing), Y offset always applied, X has a small
        // real random jitter (no RNG exists elsewhere in this engine, so a
        // deterministic stand-in is used -- see TickPollutionPuff()'s
        // comment).
        GEWorldRuntime overWorld;
        GEInteractionSystem overInteraction;
        for (int i = 0; i < 20; ++i)
        {
            overInteraction.TickPollutionPuff(overWorld, px, py, pz, false, /*isOvercraft=*/true, false, false,
                                               /*isMoving=*/false, /*ascending=*/true, /*facingDX=*/1);
        }
        const MobileObjSpec* overPuff = nullptr;
        for (const auto& obj : overWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType36)
            {
                overPuff = &obj;
            }
        }
        if (overPuff != nullptr)
        {
            check(std::fabs(overPuff->posEndY - (overPuff->posStartY - kReach)) < 0.01f,
                  "Overcraft (ascending) puff's real posEnd is 500px/64 further DOWN, regardless of facing");
            check(std::fabs(overPuff->stepAdvanceTicks - 62.0f) < 0.5f,
                  "Overcraft (ascending) puff's real stepAdvance is |8*500/64|=62 ticks (magnitude-8 bucket, "
                  "num=58)");
            check(std::fabs(overPuff->posStartY - (py - 22.0f / 64.0f)) < 0.01f,
                  "Overcraft (ascending) puff spawns at the real 22px/64 downward nozzle offset");
        }
        else
        {
            check(false, "found a spawned Overcraft (ascending) puff to inspect");
        }

        // Self-delete + real posStart->posEnd slide via the shared
        // AdvancePatrolStep() machinery (plan.md E3D-MIG-131), reused
        // rather than a fifth hand-rolled interpolation block.
        for (int i = 0; i < 15; ++i)
        {
            jeepWorld.Update(dt);
            jeepInteraction.Update(dt, jeepWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        int jeepStillActiveAt15 = 0;
        const MobileObjSpec* movedPuff = nullptr;
        for (const auto& obj : jeepWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType36)
            {
                ++jeepStillActiveAt15;
                movedPuff = &obj;
            }
        }
        check(jeepStillActiveAt15 == 4, "all 4 Jeep puffs are still active just before their real phase-16 self-delete");
        if (movedPuff != nullptr)
        {
            check(movedPuff->currentX != movedPuff->posStartX,
                  "a Jeep puff has visibly moved from posStart via the shared AdvancePatrolStep() slide "
                  "after 15 ticks");
        }

        jeepWorld.Update(dt);
        jeepInteraction.Update(dt, jeepWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        int jeepStillActiveAt16 = 0;
        for (const auto& obj : jeepWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType36)
            {
                ++jeepStillActiveAt16;
            }
        }
        check(jeepStillActiveAt16 == 0, "all 4 Jeep puffs self-delete once phase reaches the real 16-tick lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-013, fixed
        // 2026-07-14 -- divisor was 6, real is 2; table_pollution is a
        // plain ascending range so only the divisor needed fixing).
        check(GetObjIcon(ObjectType::ObjectType36, 0) == 179, "ObjectType36 icon at phase=0 is the real table_pollution[0]=179");
        check(GetObjIcon(ObjectType::ObjectType36, 2) == 180, "ObjectType36 icon at phase=2 is the real table_pollution[1]=180");
        check(GetObjIcon(ObjectType::ObjectType36, 14) == 186, "ObjectType36 icon at phase=14 is the real table_pollution[7]=186 (last frame before self-delete)");
    }

    // 17.8. Shield/Power magic trail (plan.md VISUAL-011-adjacent,
    // ObjectType57/27) -- a breadcrumb trail dropped every real 40px/64 of
    // Manhattan (X/Y-only) movement while Shield or Power is active, real
    // phase>=20/24 self-delete.
    {
        constexpr float dt = 1.0f / 20.0f;
        constexpr float kThreshold = 40.0f / 64.0f;

        GEWorldRuntime noneWorld;
        GEInteractionSystem noneInteraction;
        noneInteraction.ResetMagicTrail(0.0f, 1.0f, 0.0f);
        noneInteraction.TickMagicTrail(noneWorld, 5.0f, 1.0f, 0.0f, /*isShielded=*/false, /*isPowered=*/false);
        int noneCount = 0;
        for (const auto& obj : noneWorld.GetMobileObjects())
        {
            if (obj.active && (obj.type == ObjectType::ObjectType57 || obj.type == ObjectType::ObjectType27))
            {
                ++noneCount;
            }
        }
        check(noneCount == 0, "TickMagicTrail() never spawns while neither Shield nor Power is active, even after a large move");

        // Shield: below-threshold movement is a no-op; crossing it spawns
        // exactly at the current position and resets the tracker.
        GEWorldRuntime shieldWorld;
        GEInteractionSystem shieldInteraction;
        shieldInteraction.ResetMagicTrail(0.0f, 1.0f, 0.0f);
        shieldInteraction.TickMagicTrail(shieldWorld, kThreshold * 0.5f, 1.0f, 0.0f, /*isShielded=*/true, false);
        int shieldCountBelowThreshold = 0;
        for (const auto& obj : shieldWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType57)
            {
                ++shieldCountBelowThreshold;
            }
        }
        check(shieldCountBelowThreshold == 0, "moving less than the real 40px/64 threshold does not drop a Shield trail marker");

        shieldInteraction.TickMagicTrail(shieldWorld, kThreshold + 0.1f, 1.0f, 0.0f, true, false);
        const MobileObjSpec* shieldMarker = nullptr;
        int shieldCount = 0;
        for (const auto& obj : shieldWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType57)
            {
                ++shieldCount;
                shieldMarker = &obj;
            }
        }
        check(shieldCount == 1, "crossing the real 40px/64 threshold drops exactly 1 Shield trail marker (ObjectType57)");
        if (shieldMarker != nullptr)
        {
            constexpr float kSpawnX = kThreshold + 0.1f;
            check(shieldMarker->currentX == kSpawnX && shieldMarker->currentY == 1.0f && shieldMarker->currentZ == 0.0f,
                  "the Shield trail marker spawns exactly at Blupi's current position (static, no offset)");
            check(shieldMarker->posStartX == shieldMarker->posEndX && shieldMarker->posStartY == shieldMarker->posEndY,
                  "the Shield trail marker is static (real speed=0 -- posStart==posEnd, no slide)");
        }

        // The tracker is reset on spawn -- another below-threshold move
        // from the NEW marker position doesn't drop a second one.
        shieldInteraction.TickMagicTrail(shieldWorld, kThreshold + 0.1f + kThreshold * 0.5f, 1.0f, 0.0f, true, false);
        int shieldCountAfterSmallMove = 0;
        for (const auto& obj : shieldWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType57)
            {
                ++shieldCountAfterSmallMove;
            }
        }
        check(shieldCountAfterSmallMove == 1,
              "the tracker resets on spawn -- a second below-threshold move doesn't drop another marker yet");

        // Z is ignored by the real distance check -- moving only in Z
        // never drops a marker.
        GEWorldRuntime zOnlyWorld;
        GEInteractionSystem zOnlyInteraction;
        zOnlyInteraction.ResetMagicTrail(0.0f, 1.0f, 0.0f);
        zOnlyInteraction.TickMagicTrail(zOnlyWorld, 0.0f, 1.0f, 50.0f, true, false);
        int zOnlyCount = 0;
        for (const auto& obj : zOnlyWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType57)
            {
                ++zOnlyCount;
            }
        }
        check(zOnlyCount == 0, "the real distance check ignores world Z -- moving only in Z never drops a marker");

        // Power: same mechanic, different type/table.
        GEWorldRuntime powerWorld;
        GEInteractionSystem powerInteraction;
        powerInteraction.ResetMagicTrail(0.0f, 1.0f, 0.0f);
        powerInteraction.TickMagicTrail(powerWorld, kThreshold + 0.1f, 1.0f, 0.0f, /*isShielded=*/false,
                                        /*isPowered=*/true);
        int powerCount = 0;
        for (const auto& obj : powerWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType27)
            {
                ++powerCount;
            }
        }
        check(powerCount == 1, "crossing the real 40px/64 threshold drops exactly 1 Power trail marker (ObjectType27)");

        // Self-delete timing (phase>=20 Shield / phase>=24 Power).
        for (int i = 0; i < 19; ++i)
        {
            shieldWorld.Update(dt);
            shieldInteraction.Update(dt, shieldWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        int shieldActiveAt19 = 0;
        for (const auto& obj : shieldWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType57)
            {
                ++shieldActiveAt19;
            }
        }
        check(shieldActiveAt19 == 1, "the Shield trail marker is still active just before its real phase-20 self-delete");
        shieldWorld.Update(dt);
        shieldInteraction.Update(dt, shieldWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        int shieldActiveAt20 = 0;
        for (const auto& obj : shieldWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType57)
            {
                ++shieldActiveAt20;
            }
        }
        check(shieldActiveAt20 == 0, "Shield trail markers self-delete once phase reaches the real 20-tick lifetime");

        for (int i = 0; i < 23; ++i)
        {
            powerWorld.Update(dt);
            powerInteraction.Update(dt, powerWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        int powerActiveAt23 = 0;
        for (const auto& obj : powerWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType27)
            {
                ++powerActiveAt23;
            }
        }
        check(powerActiveAt23 == 1, "the Power trail marker is still active just before its real phase-24 self-delete");
        powerWorld.Update(dt);
        powerInteraction.Update(dt, powerWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        int powerActiveAt24 = 0;
        for (const auto& obj : powerWorld.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType27)
            {
                ++powerActiveAt24;
            }
        }
        check(powerActiveAt24 == 0, "the Power trail marker self-deletes once phase reaches the real 24-tick lifetime");

        // GetObjIcon()'s corrected formulas (plan.md VISUAL-011-adjacent,
        // fixed 2026-07-14 -- both tables repeat their first 5 icons twice
        // before continuing, not a simple ascending range).
        check(GetObjIcon(ObjectType::ObjectType57, 0) == 274, "ObjectType57 icon at phase=0 is the real table_shieldtrack[0]=274");
        check(GetObjIcon(ObjectType::ObjectType57, 5) == 274, "ObjectType57 icon at phase=5 is the real table_shieldtrack[5]=274 (the repeat)");
        check(GetObjIcon(ObjectType::ObjectType57, 10) == 279, "ObjectType57 icon at phase=10 is the real table_shieldtrack[10]=279 (continues past the repeat)");
        check(GetObjIcon(ObjectType::ObjectType27, 0) == 152, "ObjectType27 icon at phase=0 is the real table_magictrack[0]=152");
        check(GetObjIcon(ObjectType::ObjectType27, 5) == 152, "ObjectType27 icon at phase=5 is the real table_magictrack[5]=152 (the repeat)");
        check(GetObjIcon(ObjectType::ObjectType27, 10) == 157, "ObjectType27 icon at phase=10 is the real table_magictrack[10]=157 (continues past the repeat)");
    }

    // 17.9. Bullet-hit splat effect self-delete timing + icon formulas
    // (plan.md VISUAL-009, ObjectType98/99/100) -- the real spawn (via
    // the bullet-contact-kill site) is already exercised in test 10 above;
    // this isolates the 3 self-delete timings on their own with directly-
    // constructed instances, since there is no public single-shot spawn
    // method for these types (AppendSplatEffect() is a free function).
    {
        GEWorldRuntime splatWorld;
        GEInteractionSystem splatInteraction;
        constexpr float dt = 1.0f / 20.0f;
        constexpr float sx = 40.0f, sy = 1.0f, sz = 40.0f;

        const auto makeStatic = [](ObjectType type, float x, float y, float z)
        {
            MobileObjSpec spec;
            spec.type = type;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = spec.posEndX = x;
            spec.currentY = spec.posStartY = spec.posEndY = y;
            spec.currentZ = spec.posStartZ = spec.posEndZ = z;
            return spec;
        };
        splatWorld.GetMobileObjectsMutable().push_back(makeStatic(ObjectType::ObjectType98, sx, sy, sz));
        splatWorld.GetMobileObjectsMutable().push_back(makeStatic(ObjectType::ObjectType99, sx, sy, sz));
        splatWorld.GetMobileObjectsMutable().push_back(makeStatic(ObjectType::ObjectType100, sx, sy, sz));

        const auto countActiveOf = [&splatWorld](ObjectType type)
        {
            int n = 0;
            for (const auto& obj : splatWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == type) ++n;
            }
            return n;
        };

        for (int i = 0; i < 9; ++i)
        {
            splatWorld.Update(dt);
        }
        splatInteraction.Update(dt, splatWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOf(ObjectType::ObjectType98) == 1, "ObjectType98 is still active just before its real phase-10 self-delete");
        check(countActiveOf(ObjectType::ObjectType99) == 1, "ObjectType99 is still active just before its real phase-13 self-delete");
        check(countActiveOf(ObjectType::ObjectType100) == 1, "ObjectType100 is still active just before its real phase-18 self-delete");

        splatWorld.Update(dt);
        splatInteraction.Update(dt, splatWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOf(ObjectType::ObjectType98) == 0, "ObjectType98 self-deletes once phase reaches the real 10-tick lifetime");

        for (int i = 0; i < 3; ++i)
        {
            splatWorld.Update(dt);
        }
        splatInteraction.Update(dt, splatWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOf(ObjectType::ObjectType99) == 0, "ObjectType99 self-deletes once phase reaches the real 13-tick lifetime");

        for (int i = 0; i < 5; ++i)
        {
            splatWorld.Update(dt);
        }
        splatInteraction.Update(dt, splatWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOf(ObjectType::ObjectType100) == 0, "ObjectType100 self-deletes once phase reaches the real 18-tick lifetime");

        // GetObjIcon()'s corrected formulas (plan.md VISUAL-009, fixed
        // 2026-07-14 -- table_sploutch2/3's real leading `-1` "invisible
        // frame" delay is now modeled instead of a static first-frame
        // return).
        check(GetObjIcon(ObjectType::ObjectType98, 0) == 90, "ObjectType98 icon at phase=0 is the real table_sploutch1[0]=90");
        check(GetObjIcon(ObjectType::ObjectType98, 9) == 99, "ObjectType98 icon at phase=9 is the real table_sploutch1[9]=99 (last frame)");
        check(GetObjIcon(ObjectType::ObjectType99, 0) == -1, "ObjectType99 icon at phase=0 is the real table_sploutch2[0]=-1 (invisible delay)");
        check(GetObjIcon(ObjectType::ObjectType99, 2) == -1, "ObjectType99 icon at phase=2 is the real table_sploutch2[2]=-1 (still invisible)");
        check(GetObjIcon(ObjectType::ObjectType99, 3) == 90, "ObjectType99 icon at phase=3 is the real table_sploutch2[3]=90 (splash begins)");
        check(GetObjIcon(ObjectType::ObjectType100, 0) == -1, "ObjectType100 icon at phase=0 is the real table_sploutch3[0]=-1 (invisible delay)");
        check(GetObjIcon(ObjectType::ObjectType100, 7) == -1, "ObjectType100 icon at phase=7 is the real table_sploutch3[7]=-1 (still invisible)");
        check(GetObjIcon(ObjectType::ObjectType100, 8) == 90, "ObjectType100 icon at phase=8 is the real table_sploutch3[8]=90 (splash begins, longest delay)");
    }

    // 17.10. Teleporter arc (plan.md VISUAL-010, ObjectType92) -- despite
    // ObjectType.hpp's own "charged attack" doc comment, the real trigger
    // is the teleporter itself (already this engine's own TriggerTeleport()
    // call site); a single static instance, real phase>=128 self-delete.
    {
        GEWorldRuntime arcWorld;
        GEInteractionSystem arcInteraction;
        constexpr float dt = 1.0f / 20.0f;
        constexpr float ax = 25.0f, ay = 1.0f, az = 25.0f;

        arcInteraction.SpawnTeleportArc(arcWorld, ax, ay, az);
        const auto countArcsAt = [&arcWorld, ax, ay, az]()
        {
            int count = 0;
            for (const auto& obj : arcWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType92 && obj.currentX == ax &&
                    std::fabs(obj.currentY - (ay + 5.0f / 64.0f)) < 0.001f && obj.currentZ == az)
                {
                    ++count;
                }
            }
            return count;
        };
        check(countArcsAt() == 1,
              "SpawnTeleportArc() spawns exactly 1 instance, at Blupi's position with the real 5px/64 upward "
              "offset (screen Y- -> world Y+)");

        for (int i = 0; i < 127; ++i)
        {
            arcWorld.Update(dt);
        }
        arcInteraction.Update(dt, arcWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countArcsAt() == 1, "the arc is still active just before its real phase-128 self-delete");

        arcWorld.Update(dt);
        arcInteraction.Update(dt, arcWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countArcsAt() == 0, "the arc self-deletes once phase reaches the real 128-tick (6.4s) lifetime");

        // GetObjIcon()'s corrected formula (plan.md VISUAL-010, fixed
        // 2026-07-14 -- real table_explo7 is a 128-frame scatter with `-1`
        // blanks interspersed throughout, not a static first-frame return).
        check(GetObjIcon(ObjectType::ObjectType92, 0) == 60, "ObjectType92 icon at phase=0 is the real table_explo7[0]=60");
        check(GetObjIcon(ObjectType::ObjectType92, 1) == 61, "ObjectType92 icon at phase=1 is the real table_explo7[1]=61");
        check(GetObjIcon(ObjectType::ObjectType92, 2) == -1, "ObjectType92 icon at phase=2 is the real table_explo7[2]=-1 (a mid-sequence blank)");
        check(GetObjIcon(ObjectType::ObjectType92, 127) == -1, "ObjectType92 icon at phase=127 is the real table_explo7[127]=-1 (last frame before self-delete)");

        // GetObjIcon()'s corrected formula (found 2026-07-16 -- table_explo5/6/8 were the only
        // remaining explo tables still using an approximation formula (`(p/6) % N`) instead of an
        // exact transcription, after the 2026-07-14 pass fixed explo1/2/3/4/7).
        check(GetObjIcon(ObjectType::ObjectType90, 0) == 54, "ObjectType90 icon at phase=0 is the real table_explo5[0]=54");
        check(GetObjIcon(ObjectType::ObjectType90, 1) == -1, "ObjectType90 icon at phase=1 is the real table_explo5[1]=-1 (the strobe's blank tick)");
        check(GetObjIcon(ObjectType::ObjectType90, 2) == 55, "ObjectType90 icon at phase=2 is the real table_explo5[2]=55");
        check(GetObjIcon(ObjectType::ObjectType91, 0) == 54, "ObjectType91 icon at phase=0 is the real table_explo6[0]=54");
        check(GetObjIcon(ObjectType::ObjectType91, 5) == 59, "ObjectType91 icon at phase=5 is the real table_explo6[5]=59 (no blanks, plain ascending)");
        check(GetObjIcon(ObjectType::ObjectType93, 0) == 7, "ObjectType93 icon at phase=0 is the real table_explo8[0]=7");
        check(GetObjIcon(ObjectType::ObjectType93, 4) == 11, "ObjectType93 icon at phase=4 is the real table_explo8[4]=11 (no blanks, plain ascending)");
    }

    // 17.11. Voyage (plan.md `158`) mechanics in isolation -- the real
    // spawn sites are already exercised throughout this file above (each
    // pickup's own test, via completePendingVoyage()); this verifies the
    // state machine itself directly: linear interpolation, reward-at-
    // completion timing, and the real force-complete-on-new-voyage
    // interaction.
    {
        GEWorldRuntime voyageWorld;
        GEInteractionSystem voyageInteraction;
        constexpr float dt = 1.0f / 20.0f;

        voyageInteraction.BeginVoyage(voyageWorld, GEInteractionSystem::VoyageKind::Treasure, 6, false, 0.0f, 0.0f,
                                       100.0f, 0.0f, sound);
        check(voyageInteraction.VoyageActive(), "BeginVoyage() starts an active voyage");
        check(voyageInteraction.VoyageIconId() == 6 && !voyageInteraction.VoyageIsButtonChannel(),
              "the active voyage's icon/channel match what was passed in");
        check(std::fabs(voyageInteraction.VoyageDrawX() - 0.0f) < 0.01f,
              "VoyageDrawX() starts at the real start point (phase=0)");

        // Real total = (|100-0|+|0-0|)/10 = 10 ticks.
        for (int i = 0; i < 5; ++i)
        {
            voyageWorld.Update(dt);
            voyageInteraction.Update(dt, voyageWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(voyageInteraction.VoyageActive(), "the voyage is still active partway through (5 of 10 real ticks)");
        check(std::fabs(voyageInteraction.VoyageDrawX() - 50.0f) < 1.0f,
              "VoyageDrawX() linearly interpolates to the midpoint at phase=5/total=10");

        const int treasuresBefore = voyageInteraction.TreasuresCollected();
        for (int i = 0; i < 5; ++i) // the remaining 5 ticks to reach the real total=10
        {
            voyageWorld.Update(dt);
            voyageInteraction.Update(dt, voyageWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(!voyageInteraction.VoyageActive(), "the voyage completes once phase reaches the real total");
        check(voyageInteraction.TreasuresCollected() == treasuresBefore + 1,
              "the real reward applies exactly at completion (phase>=total), not before");

        // Force-complete-on-new-voyage (real `VoyageInit`'s own
        // `if (m_voyageIcon != -1) { phase=total; Step(); }`, Decor.cpp:
        // 10160-10164): starting a NEW voyage while another is still
        // mid-flight applies the OLD one's reward immediately.
        GEWorldRuntime forceWorld;
        GEInteractionSystem forceInteraction;
        forceInteraction.BeginVoyage(forceWorld, GEInteractionSystem::VoyageKind::Key1, 215, false, 0.0f, 0.0f,
                                      1000.0f, 0.0f, sound); // real total=100, won't complete on its own here
        check(forceInteraction.VoyageActive(), "voyage A (Key1) is active");
        const int key1Before = forceInteraction.Key1Count();
        forceInteraction.BeginVoyage(forceWorld, GEInteractionSystem::VoyageKind::Key2, 222, false, 0.0f, 0.0f, 10.0f,
                                      0.0f, sound);
        check(forceInteraction.Key1Count() == key1Before + 1,
              "starting voyage B (Key2) force-completes voyage A (Key1)'s real reward immediately");
        check(forceInteraction.VoyageActive() && forceInteraction.VoyageIconId() == 222,
              "voyage B (Key2) is now the active voyage");
    }

    // 17.12. GEHud::ProjectWorldToHudSpace() -- the new world->screen
    // projection utility (plan.md `158`), verified against a controlled
    // camera/viewport setup with hand-computed expected results (no such
    // projection utility existed anywhere in this codebase before this).
    {
        Easy3D::Camera3D camera;
        camera.SetPosition(Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 5.0f));
        camera.SetTarget(Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 0.0f));
        camera.SetUp(Microsoft::Xna::Framework::Vector3(0.0f, 1.0f, 0.0f));
        camera.SetFieldOfView(1.57079633f); // 90 degrees
        camera.SetAspectRatio(640.0f / 480.0f);
        camera.SetNearPlane(0.1f);
        camera.SetFarPlane(100.0f);

        // Viewport matches the 640x480 reference space exactly (scale=1,
        // no horizontal centering offset), so this isolates the
        // projection math itself from GEHud's own separate ref<->viewport
        // scale/offset conversion (already covered by GEHud.cpp's own
        // existing logic, reused verbatim here).
        float px = 0.0f, py = 0.0f;
        bool ok = GEHud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 0.0f),
                                                 camera.GetViewMatrix(), camera.GetProjectionMatrix(), 640, 480, px,
                                                 py);
        check(ok, "ProjectWorldToHudSpace() succeeds for a point in front of the camera");
        check(std::fabs(px - 320.0f) < 0.5f && std::fabs(py - 240.0f) < 0.5f,
              "a world point exactly on the camera's forward axis projects to the viewport center (320,240)");

        // A point to the world's +X (camera's own right, since the camera
        // looks down -Z with +Y up) must project to the RIGHT half of the
        // screen (px > 320); a point above center must project to the
        // TOP half (screen-space Y is down, so py < 240).
        ok = GEHud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3(1.0f, 0.0f, 0.0f), camera.GetViewMatrix(),
                                            camera.GetProjectionMatrix(), 640, 480, px, py);
        check(ok && px > 320.0f, "a world point to the camera's right projects to the right half of the screen");

        ok = GEHud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3(0.0f, 1.0f, 0.0f), camera.GetViewMatrix(),
                                            camera.GetProjectionMatrix(), 640, 480, px, py);
        check(ok && py < 240.0f, "a world point above center projects to the top half of the screen (screen-Y-down)");

        // Behind the camera (camera looks toward -Z from Z=5, so a point
        // further along +Z than the camera itself is behind it).
        ok = GEHud::ProjectWorldToHudSpace(Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 10.0f), camera.GetViewMatrix(),
                                            camera.GetProjectionMatrix(), 640, 480, px, py);
        check(!ok, "ProjectWorldToHudSpace() returns false for a point behind the camera");
    }

    // 17.13. Clear2Ascend/Clear3Ascend/Clear4 death VFX (plan.md `158`
    // death-VFX follow-up, verified directly against Decor.cpp:6547-6614
    // BlupiDead, 10141-10226 VoyageInit, 10228-10350 VoyageStep/
    // VoyageDraw, 8479-8481 ObjectType93 self-delete) -- the real
    // "soul ascends"/particle-burst VFX fired by 3 of Blupi's 8 death-
    // animation types (Clear1/5-8 have no VFX at all, Glu is a wholly
    // separate un-researched mechanic, all out of scope here).
    {
        GEWorldRuntime clear2World;
        GEInteractionSystem clear2Interaction;
        constexpr float dt = 1.0f / 20.0f;

        // Clear2Ascend: fixed total=100 (NOT distance-proportional, unlike
        // every pickup kind), even with a huge real HUD-space distance.
        clear2Interaction.BeginVoyage(clear2World, GEInteractionSystem::VoyageKind::Clear2Ascend, 230, false, 100.0f,
                                      500.0f, 100.0f, 200.0f, sound);
        check(clear2Interaction.VoyageActive() && clear2Interaction.VoyageIconId() == 230,
              "BeginVoyage(Clear2Ascend) starts an active voyage with the real icon 230");
        check(clear2Interaction.VoyageIconVisible(), "Clear2Ascend has no pre-move delay -- always visible");
        // Real total=(|dx|+|dy|)/10=(0+300)/10=30 WOULD be the generic
        // formula's answer here, but Clear2Ascend's fixed override (100)
        // must win instead -- verified by running fewer ticks than 30
        // would need and confirming it is NOT yet complete.
        for (int i = 0; i < 29; ++i)
        {
            clear2World.Update(dt);
            clear2Interaction.Update(dt, clear2World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(clear2Interaction.VoyageActive(),
              "Clear2Ascend is still active after 29 ticks -- the real fixed total=100 overrides the generic "
              "distance-based total=30 the endpoints alone would imply");

        // Real icon-cycle animation: 230->241 every 2 ticks while in
        // flight (Decor.cpp:10241-10252). After 29 ticks (all even-parity
        // boundaries crossed 14 times, `static_cast<int>(phase)%2==0`
        // checked at phase=2,4,...,28 -- 14 increments from the initial
        // 230), the icon should have advanced by exactly 14 (no wraparound
        // yet, since 230+14=244 would wrap at >241 -- 230+14=244>241, so
        // it DOES wrap: 244-12=232 after one wrap of the 12-value range
        // [230,241]). Rather than hand-deriving the exact wrapped value
        // (fragile to off-by-one drift), assert the real INVARIANT
        // instead: the icon always stays within the real animated range.
        check(clear2Interaction.VoyageIconId() >= 230 && clear2Interaction.VoyageIconId() <= 241,
              "Clear2Ascend's icon-cycle animation stays within the real [230,241] range (wraps, never escapes)");
        check(clear2Interaction.VoyageIconId() != 230,
              "Clear2Ascend's icon actually advanced from its initial 230 after 29 ticks (animation is running)");

        const int keys1Before = clear2Interaction.Key1Count(); // any counter neither kind touches
        for (int i = 0; i < 71; ++i) // remaining ticks (29 already elapsed) to reach the real fixed total=100
        {
            clear2World.Update(dt);
            clear2Interaction.Update(dt, clear2World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(!clear2Interaction.VoyageActive(), "Clear2Ascend completes once phase reaches its real fixed total=100");
        check(clear2Interaction.Key1Count() == keys1Before,
              "Clear2Ascend's real completion has no reward -- neither icon 230 nor 40 appears in VoyageStep's own "
              "completion if-chain");

        // Clear3Ascend: fixed total=50, real 30-tick pre-move delay (icon
        // hidden, position clamped to start) -- verified against a
        // controlled world-anchor position (BeginVoyage()'s trailing
        // worldAnchorX/Y/Z params) used by the puff-particle spawn.
        GEWorldRuntime clear3World;
        GEInteractionSystem clear3Interaction;
        clear3Interaction.BeginVoyage(clear3World, GEInteractionSystem::VoyageKind::Clear3Ascend, 40, false, 50.0f,
                                      100.0f, 50.0f, 0.0f, sound, 5.0f, 2.0f, -3.0f);
        check(clear3Interaction.VoyageActive() && clear3Interaction.VoyageIconId() == 40,
              "BeginVoyage(Clear3Ascend) starts an active voyage with the real icon 40");
        check(!clear3Interaction.VoyageIconVisible(),
              "Clear3Ascend's icon is hidden during the real 30-tick pre-move delay (phase=0)");
        check(std::fabs(clear3Interaction.VoyageDrawX() - 50.0f) < 0.01f,
              "Clear3Ascend's position stays clamped to the start point during the pre-move delay");

        // Real puff-particle spawn (Decor.cpp:10331-10348): a fresh
        // ObjectType93 appears each tick, anchored near the world-anchor
        // position (not the HUD-space start/end -- this engine has no 2D
        // decor-pixel space, see SpawnLavaAscendPuff()'s own comment).
        const int type93CountBefore =
            static_cast<int>(std::count_if(clear3World.GetMobileObjects().begin(), clear3World.GetMobileObjects().end(),
                                            [](const auto& o) { return o.active && o.type == ObjectType::ObjectType93; }));
        clear3World.Update(dt);
        clear3Interaction.Update(dt, clear3World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        const int type93CountAfter =
            static_cast<int>(std::count_if(clear3World.GetMobileObjects().begin(), clear3World.GetMobileObjects().end(),
                                            [](const auto& o) { return o.active && o.type == ObjectType::ObjectType93; }));
        check(type93CountAfter == type93CountBefore + 1,
              "Clear3Ascend's real puff spawn (ObjectType93) fires once per TickVoyage() call");
        const auto puff = std::find_if(clear3World.GetMobileObjects().begin(), clear3World.GetMobileObjects().end(),
                                        [](const auto& o) { return o.active && o.type == ObjectType::ObjectType93; });
        check(puff != clear3World.GetMobileObjects().end() && std::fabs(puff->currentZ - (-3.0f)) < 0.01f,
              "the spawned puff is anchored at the real world-anchor Z (no jitter applied to Z)");
        // This first spawn happens at phase=1, still within the real
        // 30-tick pre-move delay -- horizontal jitter is halved (max 4,
        // not 8) and vertical jitter quadrupled (max 40, not 10) during
        // that window (Decor.cpp:10339-10343).
        check(std::fabs(puff->currentX - 5.0f) <= (4.0f / 64.0f) + 0.01f &&
                  std::fabs(puff->currentY - 2.0f) <= (40.0f / 64.0f) + 0.01f,
              "the spawned puff's X/Y jitter stays within the real pre-move-delay table/random-range bounds around "
              "the anchor");

        // Real 30-tick delay elapses; icon becomes visible and position
        // starts advancing from the start point.
        for (int i = 0; i < 30; ++i)
        {
            clear3World.Update(dt);
            clear3Interaction.Update(dt, clear3World, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(clear3Interaction.VoyageIconVisible(),
              "Clear3Ascend's icon becomes visible once the real 30-tick pre-move delay elapses");

        // Real Clear4/Saw death VFX: 3 ObjectType41 particles (up/right/
        // left, no "down"), decoded from the real ObjectStart speeds
        // -70/20/-20 (Decor.cpp:6608-6613/7805-7869).
        GEWorldRuntime sawWorld;
        GEInteractionSystem sawInteraction;
        sawInteraction.SpawnSawDeathBurst(sawWorld, 10.0f, 1.0f, -5.0f, sound);
        int burstCount = 0;
        bool sawUp = false, sawRight = false, sawLeft = false, sawDown = false;
        constexpr float kSawReach = 500.0f / 64.0f;
        for (const auto& o : sawWorld.GetMobileObjects())
        {
            if (o.active && o.type == ObjectType::ObjectType41)
            {
                ++burstCount;
                check(std::fabs(o.stepAdvanceTicks - 156.0f) < 0.01f,
                      "each Clear4 burst particle uses the real stepAdvanceTicks=156 (magnitude 20, not Invert's 10)");
                if (std::fabs(o.posEndY - (1.0f + kSawReach)) < 0.01f) sawUp = true;
                if (std::fabs(o.posEndX - (10.0f + kSawReach)) < 0.01f) sawRight = true;
                if (std::fabs(o.posEndX - (10.0f - kSawReach)) < 0.01f) sawLeft = true;
                if (std::fabs(o.posEndY - (1.0f - kSawReach)) < 0.01f) sawDown = true;
            }
        }
        check(burstCount == 3, "SpawnSawDeathBurst() spawns exactly the real 3 particles, not Invert's 4");
        check(sawUp && sawRight && sawLeft && !sawDown,
              "Clear4's 3 real directions are up/right/left -- there is deliberately no 'down' direction");

        // Real 50/50 Clear1(nothing)/Clear2(ascend) coinflip
        // (Decor::BlupiDead(action1, action2), Decor.cpp:6551-6554) --
        // statistical: both outcomes must occur across enough trials
        // (P(all-same after 200 trials) is astronomically small for a
        // fair coin, so this is not a flaky test in practice).
        GEInteractionSystem coinInteraction;
        bool sawTrue = false, sawFalse = false;
        for (int i = 0; i < 200 && !(sawTrue && sawFalse); ++i)
        {
            (coinInteraction.RollClear2Coinflip() ? sawTrue : sawFalse) = true;
        }
        check(sawTrue && sawFalse, "RollClear2Coinflip() produces both outcomes over enough trials (a real 50/50)");
    }

    // 17.14. Sucette(26)/Drink(30)/Charge(31) real 2-stage pickup delay (plan.md `173`, verified
    // directly against Decor.cpp:6025-6087) -- GEInteractionSystem's OWN side of this: the
    // contact-time position getters GalaxyEggbertCnaGame::ResolvePickupFreeze() needs, and
    // RespawnPickupItem() itself. The deferred buff-grant/freeze timing lives entirely in
    // GEBlupiController (already covered directly in VerifyBlupiMovement) -- this class has no
    // access to it, matching the established decoupling.
    {
        GEWorldRuntime pickupWorld;
        GEInteractionSystem pickupInteraction;

        MobileObjSpec sucette;
        sucette.type = ObjectType::ObjectType26;
        sucette.posStartX = sucette.posEndX = sucette.currentX = 12.0f;
        sucette.posStartY = sucette.posEndY = sucette.currentY = 1.0f;
        sucette.posStartZ = sucette.posEndZ = sucette.currentZ = 34.0f;
        pickupWorld.GetMobileObjectsMutable().push_back(sucette);
        // Real Sucette gate requires the action button held at contact (plan.md `173`, found
        // 2026-07-14) -- touching it WITHOUT the button (blupiActionPressedEdge=false, the
        // default) must not grant anything.
        pickupInteraction.Update(dt, pickupWorld, 12.0f, 1.0f, 34.0f, 0.0f, sound);
        check(!pickupInteraction.PowerGrantedThisFrame(),
              "touching Sucette(26) WITHOUT the action button does not grant Power (real gate)");
        // Now with the button held -- blupiActionPressedEdge=true, every other trailing param
        // left at its real default.
        pickupInteraction.Update(dt, pickupWorld, 12.0f, 1.0f, 34.0f, 0.0f, sound, false, false, 0, 0, false, true,
                                  true, true, true, false, false, false, true, /*blupiActionPressedEdge=*/true);
        check(pickupInteraction.PowerGrantedThisFrame(), "touching Sucette(26) sets PowerGrantedThisFrame()");
        check(std::fabs(pickupInteraction.PowerPickupX() - 12.0f) < 0.01f &&
                  std::fabs(pickupInteraction.PowerPickupY() - 1.0f) < 0.01f &&
                  std::fabs(pickupInteraction.PowerPickupZ() - 34.0f) < 0.01f,
              "PowerPickupX/Y/Z() capture the real pickup's own contact position");

        // Vehicle-mode/Balloon/Ecrase gate (real Decor.cpp:6025-6087) -- caller
        // (GalaxyEggbertCnaGame::canGrantPower) reports false while Blupi is mounted/ballooned/
        // squashed, matching the real !m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate clause.
        // This class has no vehicle-state access itself, so this only exercises that it honors
        // blupiCanGrantPower=false even with the action button held -- the vehicle-state
        // computation itself is covered by GEBlupiController's own IsInVehicle()/IsBallooned()/
        // IsEcrased() (VerifyBlupiMovement). A fresh instance is used since the one above was
        // already consumed by the successful grant just checked.
        MobileObjSpec sucetteGated;
        sucetteGated.type = ObjectType::ObjectType26;
        sucetteGated.posStartX = sucetteGated.posEndX = sucetteGated.currentX = 50.0f;
        sucetteGated.posStartY = sucetteGated.posEndY = sucetteGated.currentY = 1.0f;
        sucetteGated.posStartZ = sucetteGated.posEndZ = sucetteGated.currentZ = 60.0f;
        pickupWorld.GetMobileObjectsMutable().push_back(sucetteGated);
        pickupInteraction.Update(dt, pickupWorld, 50.0f, 1.0f, 60.0f, 0.0f, sound, false, false, 0, 0, false, true,
                                  /*blupiCanGrantPower=*/false, true, true, false, false, false, true,
                                  /*blupiActionPressedEdge=*/true);
        check(!pickupInteraction.PowerGrantedThisFrame(),
              "touching Sucette(26) with the action button held but blupiCanGrantPower=false "
              "(vehicle/balloon/squash gate) does not grant Power");

        MobileObjSpec drink;
        drink.type = ObjectType::ObjectType30;
        drink.posStartX = drink.posEndX = drink.currentX = 20.0f;
        drink.posStartY = drink.posEndY = drink.currentY = 1.0f;
        drink.posStartZ = drink.posEndZ = drink.currentZ = 41.0f;
        pickupWorld.GetMobileObjectsMutable().push_back(drink);
        // Same real action-button gate as Sucette above -- without it, nothing grants.
        pickupInteraction.Update(dt, pickupWorld, 20.0f, 1.0f, 41.0f, 0.0f, sound);
        check(!pickupInteraction.HideGrantedThisFrame(),
              "touching Drink(30) WITHOUT the action button does not grant Hide (real gate)");
        pickupInteraction.Update(dt, pickupWorld, 20.0f, 1.0f, 41.0f, 0.0f, sound, false, false, 0, 0, false, true,
                                  true, true, true, false, false, false, true, /*blupiActionPressedEdge=*/true);
        check(pickupInteraction.HideGrantedThisFrame(), "touching Drink(30) sets HideGrantedThisFrame()");
        check(std::fabs(pickupInteraction.HidePickupX() - 20.0f) < 0.01f &&
                  std::fabs(pickupInteraction.HidePickupY() - 1.0f) < 0.01f &&
                  std::fabs(pickupInteraction.HidePickupZ() - 41.0f) < 0.01f,
              "HidePickupX/Y/Z() capture the real pickup's own contact position");

        MobileObjSpec charge;
        charge.type = ObjectType::ObjectType31;
        charge.posStartX = charge.posEndX = charge.currentX = 7.0f;
        charge.posStartY = charge.posEndY = charge.currentY = 1.0f;
        charge.posStartZ = charge.posEndZ = charge.currentZ = 9.0f;
        pickupWorld.GetMobileObjectsMutable().push_back(charge);
        // Deliberately NO action-button param here (defaults to false) -- real Charge grants
        // automatically on contact alone, unlike Sucette/Drink above (confirmed via direct source
        // read, Decor.cpp:6069-6087 has no getButtonPressedProperty() check at all).
        pickupInteraction.Update(dt, pickupWorld, 7.0f, 1.0f, 9.0f, 0.0f, sound);
        check(pickupInteraction.CloudGrantedThisFrame(),
              "touching Charge(31) sets CloudGrantedThisFrame() with NO action button needed (real gate)");
        check(std::fabs(pickupInteraction.CloudPickupX() - 7.0f) < 0.01f &&
                  std::fabs(pickupInteraction.CloudPickupY() - 1.0f) < 0.01f &&
                  std::fabs(pickupInteraction.CloudPickupZ() - 9.0f) < 0.01f,
              "CloudPickupX/Y/Z() capture the real pickup's own contact position");

        // RespawnPickupItem() -- real ObjectStart(pos, type, 0) at the freeze's own completion.
        const int type26CountBefore = static_cast<int>(std::count_if(
            pickupWorld.GetMobileObjects().begin(), pickupWorld.GetMobileObjects().end(),
            [](const auto& o) { return o.active && o.type == ObjectType::ObjectType26; }));
        pickupInteraction.RespawnPickupItem(pickupWorld, 12.0f, 1.0f, 34.0f, ObjectType::ObjectType26);
        const int type26CountAfter = static_cast<int>(std::count_if(
            pickupWorld.GetMobileObjects().begin(), pickupWorld.GetMobileObjects().end(),
            [](const auto& o) { return o.active && o.type == ObjectType::ObjectType26; }));
        check(type26CountAfter == type26CountBefore + 1,
              "RespawnPickupItem() spawns a new active instance of the real pickup type");
        const auto respawned =
            std::find_if(pickupWorld.GetMobileObjects().begin(), pickupWorld.GetMobileObjects().end(),
                          [](const auto& o) { return o.active && o.type == ObjectType::ObjectType26; });
        check(respawned != pickupWorld.GetMobileObjects().end() &&
                  std::fabs(respawned->currentX - 12.0f) < 0.01f && std::fabs(respawned->posStartX - 12.0f) < 0.01f &&
                  std::fabs(respawned->posEndX - 12.0f) < 0.01f,
              "the respawned item is static (posStart==posEnd==current, real speed=0)");
    }

    // 17.15. Water splash/bubble effects (plan.md PICKUP-078/079/080, found
    // 2026-07-17): SpawnWaterSplash() (ObjectType14 Plouf / ObjectType35
    // Tiplouf, static, real phase>=14/6 self-delete) and SpawnWaterBubble()
    // (ObjectType15 Blup, rises through a scanned water column, self-
    // deletes on arrival via the shared AdvancePatrolStep() ObjectType23
    // junction).
    {
        GEWorldRuntime splashWorld;
        GEInteractionSystem splashInteraction;
        constexpr float dt = 1.0f / 20.0f; // matches the real 20Hz tick rate obj.phase advances at
        constexpr float sx = 5.0f, sy = 2.0f, sz = 5.0f;

        const auto countActiveOfType = [&splashWorld](ObjectType type)
        {
            int count = 0;
            for (const auto& obj : splashWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == type)
                {
                    ++count;
                }
            }
            return count;
        };

        check(!splashInteraction.HasActiveObjectOfType(splashWorld, ObjectType::ObjectType14),
              "HasActiveObjectOfType() is false before any Plouf is spawned");
        splashInteraction.SpawnWaterSplash(splashWorld, ObjectType::ObjectType14, sx, sy, sz);
        check(countActiveOfType(ObjectType::ObjectType14) == 1,
              "SpawnWaterSplash(Plouf) spawns exactly 1 static instance");
        check(splashInteraction.HasActiveObjectOfType(splashWorld, ObjectType::ObjectType14),
              "HasActiveObjectOfType() is true once a Plouf is active");

        for (int i = 0; i < 13; ++i)
        {
            splashWorld.Update(dt);
        }
        splashInteraction.Update(dt, splashWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOfType(ObjectType::ObjectType14) == 1,
              "the Plouf splash is still active just before its real phase-14 self-delete");
        splashWorld.Update(dt);
        splashInteraction.Update(dt, splashWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOfType(ObjectType::ObjectType14) == 0,
              "the Plouf splash self-deletes once phase reaches the real 14-tick lifetime");

        // Tiplouf -- same shape, shorter real 6-tick lifetime.
        splashInteraction.SpawnWaterSplash(splashWorld, ObjectType::ObjectType35, sx, sy, sz);
        check(countActiveOfType(ObjectType::ObjectType35) == 1,
              "SpawnWaterSplash(Tiplouf) spawns exactly 1 static instance");
        for (int i = 0; i < 5; ++i)
        {
            splashWorld.Update(dt);
        }
        splashInteraction.Update(dt, splashWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOfType(ObjectType::ObjectType35) == 1,
              "the Tiplouf splash is still active just before its real phase-6 self-delete");
        splashWorld.Update(dt);
        splashInteraction.Update(dt, splashWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(countActiveOfType(ObjectType::ObjectType35) == 0,
              "the Tiplouf splash self-deletes once phase reaches the real 6-tick lifetime");

        // Corrected icon tables (real table_plouf/tiplouf/blup, Tables.cpp:1508-1519, previously
        // wrong monotonic-range approximations here).
        check(GetObjIcon(ObjectType::ObjectType14, 0) == 99, "Plouf icon at phase=0 is the real table_plouf[0]=99");
        check(GetObjIcon(ObjectType::ObjectType14, 3) == 102, "Plouf icon at phase=3 is the real table_plouf[3]=102 (ripple peak)");
        check(GetObjIcon(ObjectType::ObjectType14, 6) == 99, "Plouf icon at phase=6 is the real table_plouf[6]=99 (back down)");
        check(GetObjIcon(ObjectType::ObjectType35, 0) == 244, "Tiplouf icon at phase=0 is the real table_tiplouf[0]=244 (ambient)");
        check(GetObjIcon(ObjectType::ObjectType35, 1) == 99, "Tiplouf icon at phase=1 is the real table_tiplouf[1]=99 (the droplet)");
        check(GetObjIcon(ObjectType::ObjectType35, 2) == 244, "Tiplouf icon at phase=2 is the real table_tiplouf[2]=244 (back to ambient)");
        check(GetObjIcon(ObjectType::ObjectType15, 0) == 103, "Blup icon at phase=0 is the real table_blup[0]=103");
        check(GetObjIcon(ObjectType::ObjectType15, 6) == 106, "Blup icon at phase=6 is the real table_blup[6]=106 (shuffled, not a growing range)");

        // SpawnWaterBubble() -- a 4-tile water column above the spawn point.
        GEWorldRuntime bubbleWorld;
        GEInteractionSystem bubbleInteraction;
        auto& mutableBubbleWorld = bubbleWorld.GetWorldMutable();
        constexpr int kBGX = 20, kBGZ = 20, kBGY0 = 10;
        for (int i = 0; i < 5; ++i)
        {
            mutableBubbleWorld.setBlock(kBGX, static_cast<std::uint16_t>(kBGY0 + i), kBGZ,
                                          Worlds::Block::make(BlockTypes::Water1));
        }
        mutableBubbleWorld.setBlock(kBGX, static_cast<std::uint16_t>(kBGY0 + 5), kBGZ,
                                      Worlds::Block::make(BlockTypes::Air));
        const float bx = static_cast<float>(kBGX) - GEWorldRuntime::kWorldCenterX;
        const float bz = static_cast<float>(kBGZ) - GEWorldRuntime::kWorldCenterZ;
        const float by = static_cast<float>(kBGY0);

        bubbleInteraction.SpawnWaterBubble(bubbleWorld, bubbleWorld.GetWorld(), bx, by, bz);
        const auto findBubble = [&bubbleWorld]()
        {
            return std::find_if(bubbleWorld.GetMobileObjects().begin(), bubbleWorld.GetMobileObjects().end(),
                                  [](const auto& o) { return o.active && o.type == ObjectType::ObjectType15; });
        };
        auto bubbleIt = findBubble();
        check(bubbleIt != bubbleWorld.GetMobileObjects().end(),
              "SpawnWaterBubble() spawns an ObjectType15 instance when there's a clear water column above");
        check(bubbleIt->patrolStep == 2, "the bubble starts already advancing (real step=2, no dwell)");
        check(std::fabs(bubbleIt->posEndY - bubbleIt->posStartY - 4.0f) < 0.01f,
              "the bubble's posEnd is exactly the real water-column height (4 clear tiles) above its start");

        for (int i = 0; i < 40; ++i)
        {
            bubbleWorld.Update(dt);
            bubbleInteraction.Update(dt, bubbleWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(findBubble() == bubbleWorld.GetMobileObjects().end(),
              "the bubble self-deletes on reaching posEnd (real ObjectType15/ObjectType23 shared arrival branch), not dwelling there");

        // No-op guard: no water column above (dry tile immediately above the spawn point).
        GEWorldRuntime dryWorld;
        GEInteractionSystem dryInteraction;
        auto& mutableDryWorld = dryWorld.GetWorldMutable();
        mutableDryWorld.setBlock(kBGX, static_cast<std::uint16_t>(kBGY0), kBGZ, Worlds::Block::make(BlockTypes::Water1));
        mutableDryWorld.setBlock(kBGX, static_cast<std::uint16_t>(kBGY0 + 1), kBGZ, Worlds::Block::make(BlockTypes::Air));
        dryInteraction.SpawnWaterBubble(dryWorld, dryWorld.GetWorld(), bx, by, bz);
        check(std::none_of(dryWorld.GetMobileObjects().begin(), dryWorld.GetMobileObjects().end(),
                            [](const auto& o) { return o.active && o.type == ObjectType::ObjectType15; }),
              "SpawnWaterBubble() is a no-op with no clear water column above (real num<=0 guard)");
    }

    // 17.16. Blitz-emitter zap ambient sound (plan.md SOUND-079/VISUAL-024,
    // ch69, found 2026-07-17) -- a one-time lazy world scan for a
    // Blitz(305)-with-BlitzEmitter(304)-above pair, then a fixed 6-tick-per-
    // 100 trigger pattern reproduced via GetAnimPhase(). `sound` (this file's
    // shared, never-`LoadContent()`-ed instance) makes every `Play()` call a
    // silent, side-effect-free no-op with no playback state to query (see
    // its own declaration comment) -- these checks exercise the lazy-scan
    // and 100-tick lookup logic itself for regressions/crashes across both
    // the "no qualifying pair" and "qualifying pair present, every real
    // trigger tick" cases, rather than asserting audible playback (not
    // observable through this harness).
    {
        GEWorldRuntime noEmitterWorld;
        GEInteractionSystem noEmitterInteraction;
        auto& mutableNoEmitterWorld = noEmitterWorld.GetWorldMutable();
        mutableNoEmitterWorld.setBlock(10, 5, 10, Worlds::Block::make(BlockTypes::Blitz));
        mutableNoEmitterWorld.setBlock(10, 6, 10, Worlds::Block::make(BlockTypes::Ground)); // NOT the emitter icon
        noEmitterInteraction.Update(1.0f / 20.0f, noEmitterWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(!noEmitterInteraction.HasBlitzEmitterPair(),
              "the lazy world scan correctly finds no Blitz/BlitzEmitter pair when none exists");
        for (int i = 0; i < 99; ++i)
        {
            noEmitterWorld.Update(1.0f / 20.0f);
            noEmitterInteraction.Update(1.0f / 20.0f, noEmitterWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(true, "Blitz-emitter scan/lookup runs cleanly across a full 100-tick cycle with no qualifying pair");

        GEWorldRuntime emitterWorld;
        GEInteractionSystem emitterInteraction;
        auto& mutableEmitterWorld = emitterWorld.GetWorldMutable();
        mutableEmitterWorld.setBlock(10, 5, 10, Worlds::Block::make(BlockTypes::Blitz));
        mutableEmitterWorld.setBlock(10, 6, 10, Worlds::Block::make(BlockTypes::BlitzEmitter));
        emitterInteraction.Update(1.0f / 20.0f, emitterWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        check(emitterInteraction.HasBlitzEmitterPair(),
              "the lazy world scan correctly finds a Blitz/BlitzEmitter pair when one exists");
        for (int i = 0; i < 99; ++i)
        {
            emitterWorld.Update(1.0f / 20.0f);
            emitterInteraction.Update(1.0f / 20.0f, emitterWorld, 100000.0f, 100000.0f, 100000.0f, 0.0f, sound);
        }
        check(true, "Blitz-emitter scan/lookup runs cleanly across a full 100-tick cycle with a qualifying pair "
                    "present (real trigger ticks 0/7/18/25/33/44 all exercised)");
    }

    // 17.17. Hub/mission-progression mission-math (found/implemented
    // 2026-07-17, real `Decor.cpp`'s `Bye`/`Win`-action mission handlers
    // and `Game1::MissionBack()`) -- pure functions, table-driven.
    {
        using GalaxyEggbert::BlockTypes::isWorldSelect;
        using GalaxyEggbert::BlockTypes::worldSelectIndex;
        using GalaxyEggbert::BlockTypes::WorldSelect1;
        using GalaxyEggbert::BlockTypes::WorldSelect8;

        // From the global hub (mission 1): marker N -> world N*10.
        check(GEWorldRuntime::ComputeWorldSelectTarget(1, 1) == 10,
              "global hub marker 1 -> mission 10 (world 1's hub)");
        check(GEWorldRuntime::ComputeWorldSelectTarget(1, 5) == 50,
              "global hub marker 5 -> mission 50 (world 5's hub)");

        // From a world hub (mission X0): marker N -> sublevel X0+N.
        check(GEWorldRuntime::ComputeWorldSelectTarget(10, 1) == 11,
              "world-10 hub marker 1 -> mission 11 (its own first sublevel)");
        check(GEWorldRuntime::ComputeWorldSelectTarget(10, 2) == 12,
              "world-10 hub marker 2 -> mission 12 (its own second sublevel)");
        check(GEWorldRuntime::ComputeWorldSelectTarget(30, 4) == 34,
              "world-30 hub marker 4 -> mission 34");

        // ComputeMissionBack(): a sublevel returns to its own world's hub;
        // a hub already (mission%10==0) returns to the global hub (1).
        check(GEWorldRuntime::ComputeMissionBack(11) == 10, "sublevel 11 back -> hub 10");
        check(GEWorldRuntime::ComputeMissionBack(15) == 10, "sublevel 15 back -> hub 10");
        check(GEWorldRuntime::ComputeMissionBack(10) == 1, "hub 10 back -> global hub 1");
        check(GEWorldRuntime::ComputeMissionBack(50) == 1, "hub 50 back -> global hub 1");

        // BlockTypes::isWorldSelect()/worldSelectIndex() -- icons 158-169,
        // indices 1-12 (extended 2026-07-17 from the original 8, to cover
        // all 12 real world hubs -- renamed from Sp0-Sp7, plan.md `170`).
        using GalaxyEggbert::BlockTypes::WorldSelect12;
        check(isWorldSelect(WorldSelect1) && isWorldSelect(WorldSelect12), "WorldSelect1/12 are recognized markers");
        check(!isWorldSelect(157) && !isWorldSelect(170), "icons just outside 158-169 are NOT world-select markers");
        check(worldSelectIndex(WorldSelect1) == 1 && worldSelectIndex(WorldSelect12) == 12,
              "worldSelectIndex() recovers the real 1-12 marker index");
        check(worldSelectIndex(GalaxyEggbert::BlockTypes::Ground) == -1,
              "worldSelectIndex() returns -1 for a non-marker icon");

        // BlockTypes::isProgressDoor()/progressDoorIndex() -- real per-
        // world sublevel-select door gate (icons 170-176, indices 2-8,
        // found 2026-07-17, `Decor::AdaptDoors()`'s hub-level branch).
        using GalaxyEggbert::BlockTypes::isProgressDoor;
        using GalaxyEggbert::BlockTypes::progressDoorIndex;
        using GalaxyEggbert::BlockTypes::ProgressDoor2;
        using GalaxyEggbert::BlockTypes::ProgressDoor8;
        check(isProgressDoor(ProgressDoor2) && isProgressDoor(ProgressDoor8), "ProgressDoor2/8 are recognized doors");
        check(!isProgressDoor(169) && !isProgressDoor(177), "icons just outside 170-176 are NOT progress doors");
        check(progressDoorIndex(ProgressDoor2) == 2 && progressDoorIndex(ProgressDoor8) == 8,
              "progressDoorIndex() recovers the real 2-8 gated marker index");

        // GEWorldRuntime::ComputeWinExitTarget() -- the real win-exit
        // formula (`Decor.cpp:6411-6434`), distinct from ComputeMissionBack()
        // above: mission 1's own exit goes to the real final bonus world
        // (199); mission 199's own exit loops back to the global hub (this
        // engine's simplification of the real true-ending sentinel); every
        // other mission falls through to the same ComputeMissionBack().
        check(GEWorldRuntime::ComputeWinExitTarget(1) == 199, "mission 1's own exit -> mission 199 (final bonus world)");
        check(GEWorldRuntime::ComputeWinExitTarget(199) == 1, "mission 199's own exit -> loops back to mission 1");
        check(GEWorldRuntime::ComputeWinExitTarget(11) == 10, "sublevel 11's exit -> hub 10 (falls through to ComputeMissionBack)");
        check(GEWorldRuntime::ComputeWinExitTarget(10) == 1, "hub 10's own exit -> global hub 1 (falls through)");
    }

    // 17.18. New hub/mission-progression worlds (plan.md hub/mission
    // system) -- confirm world010/011/012.vwr each load cleanly and report
    // the right real missionNumber, same sanity level as the existing
    // world999.vwr load at the top of this file.
    {
        GEWorldRuntime hubWorld;
        check(hubWorld.LoadFromVwrFile("worlds3d/world010.vwr"), "world010.vwr loads cleanly");
        check(hubWorld.GetMissionNumber() == 10, "world010.vwr reports missionNumber() == 10");

        GEWorldRuntime sub11;
        check(sub11.LoadFromVwrFile("worlds3d/world011.vwr"), "world011.vwr loads cleanly");
        check(sub11.GetMissionNumber() == 11, "world011.vwr reports missionNumber() == 11");

        GEWorldRuntime sub12;
        check(sub12.LoadFromVwrFile("worlds3d/world012.vwr"), "world012.vwr loads cleanly");
        check(sub12.GetMissionNumber() == 12, "world012.vwr reports missionNumber() == 12");
    }

    // 18. GESound::FootstepChannelFor() (plan.md E3D-MIG-084) -- the real
    // Decor::SoundEnviron() terrain-specific footstep/landing remap, one
    // representative icon per range plus a generic fallback. A pure
    // function, no LoadContent()/audio device needed.
    {
        using GalaxyEggbert::SoundChannel;
        check(GESound::FootstepChannelFor(41) == SoundChannel::SoundChannel78,
              "icon 41 (obstacle range 41-47) remaps to channel 78");
        check(GESound::FootstepChannelFor(139) == SoundChannel::SoundChannel78,
              "icon 139 (obstacle range 139-143) remaps to channel 78 too");
        check(GESound::FootstepChannelFor(15) == SoundChannel::SoundChannel80,
              "icon 15 (obstacle range 1-28) remaps to channel 80");
        check(GESound::FootstepChannelFor(325) == SoundChannel::SoundChannel80,
              "icon 325 (obstacle range 324-329) remaps to channel 80 too");
        check(GESound::FootstepChannelFor(338) == SoundChannel::SoundChannel82,
              "icon 338 remaps to channel 82");
        check(GESound::FootstepChannelFor(350) == SoundChannel::SoundChannel84,
              "icon 350 (obstacle range 341-363) remaps to channel 84");
        check(GESound::FootstepChannelFor(220) == SoundChannel::SoundChannel86,
              "icon 220 (obstacle range 215-234) remaps to channel 86");
        check(GESound::FootstepChannelFor(247) == SoundChannel::SoundChannel88,
              "icon 247 (obstacle range 246-249) remaps to channel 88");
        check(GESound::FootstepChannelFor(108) == SoundChannel::SoundChannel90,
              "icon 108 (obstacle range 107-109) remaps to channel 90");
        check(GESound::FootstepChannelFor(BlockTypes::RockPile) == SoundChannel::SoundChannel3,
              "an icon outside all 7 remap ranges falls back to the generic channel 3");
    }

    // 19. FindTrainingHint() (plan.md HUD-024) -- the real
    // Decor::DrawInfo/Tables::table_training1..4 lookup, one representative
    // check per mission plus the real gate semantics (exact-treasure-count,
    // vehicle, dynamite) and the "matched but empty text" case.
    {
        using std::string;
        // Mission 11: unconditional hint in range [1,3].
        {
            const char* hint = FindTrainingHint(11, 2, 10, 0, false, false);
            check(hint != nullptr && string(hint) == "Use the directional wheel [Move].",
                  "mission 11, grid (2,10): unconditional hint matches");
        }
        // Mission 11: exact-treasure-count gate (record at col 4 needs
        // treasuresCollected == 0).
        {
            check(FindTrainingHint(11, 4, 10, 0, false, false) != nullptr,
                  "mission 11, grid (4,10), 0 treasures: gate (==0) passes");
            check(FindTrainingHint(11, 4, 10, 1, false, false) == nullptr,
                  "mission 11, grid (4,10), 1 treasure: gate (==0) now fails, no hint");
        }
        // Mission 11: a real record whose text is genuinely empty (col 16)
        // -- matches but shows nothing, same as the real source.
        {
            check(FindTrainingHint(11, 16, 10, 0, false, false) == nullptr,
                  "mission 11, grid (16,10): matches a real but intentionally-empty hint slot");
        }
        // Mission 12: unconditional.
        {
            const char* hint = FindTrainingHint(12, 10, 50, 0, false, false);
            check(hint != nullptr && string(hint) == "Push the box forward until the red dot with [Action].",
                  "mission 12, grid (10,50): unconditional hint matches");
        }
        // Mission 13: same rect, opposite vehicle gate -> different text.
        {
            const char* noVehicleHint = FindTrainingHint(13, 20, 38, 0, false, false);
            const char* vehicleHint = FindTrainingHint(13, 20, 38, 0, true, false);
            check(noVehicleHint != nullptr && string(noVehicleHint) == "Take a helicopter with [Action].",
                  "mission 13, grid (20,38), no vehicle: the -2 (noVehicle) record");
            check(vehicleHint != nullptr &&
                      string(vehicleHint) == "Use [Move] or [Jump] to take off. Direct with [Move] and [Move].",
                  "mission 13, grid (20,38), in a vehicle: the -3 (anyVehicle) record, different text");
        }
        // Mission 14: dynamite gate.
        {
            const char* noDynamite = FindTrainingHint(14, 10, 50, 0, false, false);
            const char* hasDynamite = FindTrainingHint(14, 10, 50, 0, false, true);
            check(noDynamite != nullptr && string(noDynamite) == "Take the dynamite sticks with [Action].",
                  "mission 14, grid (10,50), no dynamite: the -4 (noDynamite) record");
            check(hasDynamite != nullptr && string(hasDynamite) == "Do not put down the dynamite here!",
                  "mission 14, grid (10,50), carrying dynamite: the -5 (hasDynamite) record, different text");
        }
        // Outside every real rect, and outside missions 11-14 entirely.
        check(FindTrainingHint(11, 99, 99, 0, false, false) == nullptr,
              "mission 11, a grid position outside every real rect: no hint");
        check(FindTrainingHint(0, 2, 10, 0, false, false) == nullptr,
              "mission 0 (no mission): no hint anywhere, matching the real array==nullptr early-out");
    }

    // 5. Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13) -- each
    // test below uses its OWN fresh GEWorldRuntime/GEInteractionSystem
    // pair (not the heavily-mutated shared `world`/`interaction` above)
    // so the exact before/after deltas are unambiguous.
    {
        GEWorldRuntime cheatWorld;
        if (!cheatWorld.LoadFromVwrFile(worldPath))
        {
            check(false, "cheat tests: could not load a fresh copy of the sample world");
        }
        else
        {
            GEInteractionSystem cheatInteraction;

            // 5.1 CheatOpenDoors: opens both real door families (key-gated
            // Door1/2/3 AND treasure-gated icon>=421) regardless of
            // whether Blupi actually holds the matching key/treasure.
            {
                auto& terrain = cheatWorld.GetWorldMutable();
                const int axis = static_cast<int>(terrain.blocksPerAxis());
                bool foundKeyDoor = false, foundTreasureDoor = false;
                for (int gx = 0; gx < axis && !(foundKeyDoor && foundTreasureDoor); ++gx)
                {
                    for (int gy = 0; gy < axis && !(foundKeyDoor && foundTreasureDoor); ++gy)
                    {
                        for (int gz = 0; gz < axis && !(foundKeyDoor && foundTreasureDoor); ++gz)
                        {
                            const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                                static_cast<std::uint16_t>(gz))
                                                   .type();
                            if (BlockTypes::isDoor(icon)) foundKeyDoor = true;
                            if (icon >= 421 && icon <= 440) foundTreasureDoor = true;
                        }
                    }
                }
                check(foundKeyDoor, "cheat tests: sample world has at least one key-gated door before CheatOpenDoors()");
                check(foundTreasureDoor,
                      "cheat tests: sample world has at least one treasure-gated door before CheatOpenDoors()");

                cheatInteraction.CheatOpenDoors(cheatWorld, sound);

                bool stillHasKeyDoor = false, stillHasTreasureDoor = false;
                for (int gx = 0; gx < axis; ++gx)
                {
                    for (int gy = 0; gy < axis; ++gy)
                    {
                        for (int gz = 0; gz < axis; ++gz)
                        {
                            const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                                static_cast<std::uint16_t>(gz))
                                                   .type();
                            if (BlockTypes::isDoor(icon)) stillHasKeyDoor = true;
                            if (icon >= 421 && icon <= 440) stillHasTreasureDoor = true;
                        }
                    }
                }
                check(!stillHasKeyDoor, "CheatOpenDoors(): opens every key-gated door, even without holding the key");
                check(!stillHasTreasureDoor,
                      "CheatOpenDoors(): opens every treasure-gated door, even without enough treasures");
            }

            // 5.2 CheatCleanAll: deactivates every active instance of the
            // 12-type hazard/enemy list, leaves everything else (e.g. the
            // exit marker) untouched.
            {
                int hazardsBefore = 0;
                bool exitActiveBefore = false;
                for (const auto& obj : cheatWorld.GetMobileObjects())
                {
                    if (!obj.active) continue;
                    switch (obj.type)
                    {
                        case ObjectType::ObjectType2: case ObjectType::ObjectType3: case ObjectType::ObjectType4:
                        case ObjectType::ObjectType16: case ObjectType::ObjectType17: case ObjectType::ObjectType20:
                        case ObjectType::ObjectType32: case ObjectType::ObjectType33: case ObjectType::ObjectType44:
                        case ObjectType::ObjectType54: case ObjectType::ObjectType96: case ObjectType::ObjectType97:
                            ++hazardsBefore;
                            break;
                        case ObjectType::ObjectType7:
                            exitActiveBefore = true;
                            break;
                        default:
                            break;
                    }
                }
                check(hazardsBefore > 0, "cheat tests: sample world has at least one CleanAll-eligible hazard/enemy");
                check(exitActiveBefore, "cheat tests: sample world has an active exit marker (ObjectType7) before CheatCleanAll()");

                const bool cleanAllDestroyedAnything = cheatInteraction.CheatCleanAll(cheatWorld);
                check(cleanAllDestroyedAnything,
                      "CheatCleanAll() returns true when it actually destroyed something (plan.md CAM-008 "
                      "camera-shake signal)");
                check(!cheatInteraction.CheatCleanAll(cheatWorld),
                      "CheatCleanAll() returns false when called again with nothing left to destroy");

                int hazardsAfter = 0;
                bool exitActiveAfter = false;
                for (const auto& obj : cheatWorld.GetMobileObjects())
                {
                    if (!obj.active) continue;
                    switch (obj.type)
                    {
                        case ObjectType::ObjectType2: case ObjectType::ObjectType3: case ObjectType::ObjectType4:
                        case ObjectType::ObjectType16: case ObjectType::ObjectType17: case ObjectType::ObjectType20:
                        case ObjectType::ObjectType32: case ObjectType::ObjectType33: case ObjectType::ObjectType44:
                        case ObjectType::ObjectType54: case ObjectType::ObjectType96: case ObjectType::ObjectType97:
                            ++hazardsAfter;
                            break;
                        case ObjectType::ObjectType7:
                            exitActiveAfter = true;
                            break;
                        default:
                            break;
                    }
                }
                check(hazardsAfter == 0, "CheatCleanAll(): deactivates every hazard/enemy of the real 12-type list");
                check(exitActiveAfter, "CheatCleanAll(): leaves unrelated objects (the exit marker) untouched");
            }

            // 5.3 CheatAllTreasure: collects every active treasure at
            // once, incrementing TreasuresCollected() by exactly that
            // many.
            {
                int treasuresInWorld = 0;
                for (const auto& obj : cheatWorld.GetMobileObjects())
                {
                    if (obj.active && obj.type == ObjectType::ObjectType5) ++treasuresInWorld;
                }
                check(treasuresInWorld > 0, "cheat tests: sample world has at least one uncollected treasure");
                const int before = cheatInteraction.TreasuresCollected();

                cheatInteraction.CheatAllTreasure(cheatWorld, sound);

                check(cheatInteraction.TreasuresCollected() == before + treasuresInWorld,
                      "CheatAllTreasure(): TreasuresCollected() increases by exactly the number of treasures in the world");
                int treasuresRemaining = 0;
                for (const auto& obj : cheatWorld.GetMobileObjects())
                {
                    if (obj.active && obj.type == ObjectType::ObjectType5) ++treasuresRemaining;
                }
                check(treasuresRemaining == 0, "CheatAllTreasure(): every treasure is deactivated");
            }

            // 5.4 CheatFindExit: returns the real exit marker's position.
            {
                const MobileObjSpec* realExit = nullptr;
                for (const auto& obj : cheatWorld.GetMobileObjects())
                {
                    if (obj.active && obj.type == ObjectType::ObjectType7)
                    {
                        realExit = &obj;
                        break;
                    }
                }
                check(realExit != nullptr, "cheat tests: sample world has an active exit marker");
                if (realExit != nullptr)
                {
                    float ex = 0.0f, ey = 0.0f, ez = 0.0f;
                    const bool found = cheatInteraction.CheatFindExit(cheatWorld, ex, ey, ez);
                    check(found, "CheatFindExit(): finds the exit marker");
                    check(std::fabs(ex - realExit->currentX) < 0.01f && std::fabs(ey - realExit->currentY) < 0.01f &&
                              std::fabs(ez - realExit->currentZ) < 0.01f,
                          "CheatFindExit(): returned position matches the real exit marker's position exactly");
                }
            }
        }
    }

    // 6. Cloud secret-power electric aura (plan.md `068`, `Decor::
    // BlupiElectro`) -- while active, instantly destroys small enemies
    // (ObjectType4/32/33) within a real 40px aura around Blupi. Own
    // fresh, isolated world/interaction pair (matches the cheat tests'
    // own reasoning above).
    {
        GEWorldRuntime auraWorld;
        if (!auraWorld.LoadFromVwrFile(worldPath))
        {
            check(false, "aura test: could not load sample world");
        }
        else
        {
            GEInteractionSystem auraInteraction;
            // Real blupih (ObjectType32) sample-world instance sits at
            // its posStart, (85,4,80) -- see GenerateSampleWorld3D.cpp.
            const MobileObjSpec* blupih = nullptr;
            for (const auto& obj : auraWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType32)
                {
                    blupih = &obj;
                    break;
                }
            }
            check(blupih != nullptr, "aura test: sample world has an active blupih (ObjectType32)");
            if (blupih != nullptr)
            {
                const float bx = blupih->currentX, by = blupih->currentY, bz = blupih->currentZ;

                // Standing right on top of it with the aura OFF must not
                // destroy it.
                auraInteraction.Update(1.0f / 60.0f, auraWorld, bx, by, bz, 0.0f, sound, false, false, 0, 0, false,
                                       true, true, true, true, false, false, /*blupiCloudActive=*/false);
                bool stillActive = false;
                for (const auto& obj : auraWorld.GetMobileObjects())
                {
                    if (&obj == blupih) { stillActive = obj.active; break; }
                }
                check(stillActive, "Cloud aura OFF: standing on a blupih does not destroy it");

                // Same position, aura ON: destroys it.
                auraInteraction.Update(1.0f / 60.0f, auraWorld, bx, by, bz, 0.0f, sound, false, false, 0, 0, false,
                                       true, true, true, true, false, false, /*blupiCloudActive=*/true);
                bool destroyed = true;
                for (const auto& obj : auraWorld.GetMobileObjects())
                {
                    if (&obj == blupih) { destroyed = !obj.active; break; }
                }
                check(destroyed, "Cloud aura ON: standing on a blupih destroys it (BlupiElectro)");
            }
        }
        {
            // Far away (well beyond the aura radius), the aura must not
            // reach even with Cloud active.
            GEWorldRuntime farWorld;
            farWorld.LoadFromVwrFile(worldPath);
            GEInteractionSystem farInteraction;
            const MobileObjSpec* blupit = nullptr;
            for (const auto& obj : farWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType33)
                {
                    blupit = &obj;
                    break;
                }
            }
            check(blupit != nullptr, "aura test: sample world has an active blupit (ObjectType33)");
            if (blupit != nullptr)
            {
                farInteraction.Update(1.0f / 60.0f, farWorld, blupit->currentX + 50.0f, blupit->currentY,
                                      blupit->currentZ, 0.0f, sound, false, false, 0, 0, false, true, true, true,
                                      true, false, false, /*blupiCloudActive=*/true);
                bool stillActive = false;
                for (const auto& obj : farWorld.GetMobileObjects())
                {
                    if (&obj == blupit) { stillActive = obj.active; break; }
                }
                check(stillActive, "Cloud aura ON but far away: a blupit 50 units away is untouched");
            }
        }
    }

    // Bridge construction (ObjectType52, plan.md PICKUP-064) -- the sample
    // world's own demo (tools/GenerateSampleWorld3D.cpp): a single Bridge
    // (icon 364) tile at grid (88,0,97), world (38,1,47), spanning a real
    // gap with nothing beneath it. Fresh GEWorldRuntime so this test's own
    // many Update() calls don't affect earlier sections' object state.
    {
        GEWorldRuntime bridgeWorld;
        check(bridgeWorld.LoadFromVwrFile(worldPath), "loaded the world for the bridge test");
        GEInteractionSystem bridgeInteraction;
        constexpr float bridgeX = 38.0f, bridgeY = 1.0f, bridgeZ = 47.0f;
        constexpr float dt = 1.0f / 20.0f; // matches the real 20Hz tick rate obj.phase advances at

        check(bridgeWorld.GetWorld().getBlock(88, 0, 97).type() == GalaxyEggbert::BlockTypes::Bridge,
              "the sample world's Bridge demo tile is at the expected grid cell");

        // Counts only ObjectType52 near the bridge cell itself -- the
        // sample world's own object-type exhibition (tools/
        // GenerateSampleWorld3D.cpp) already places one static specimen of
        // every ObjectType (including 52) far away, so a blind global type
        // count would always be off by that one pre-existing exhibit.
        const auto countBridgeObjsHere = [&bridgeWorld, bridgeX, bridgeZ]()
        {
            int count = 0;
            for (const auto& obj : bridgeWorld.GetMobileObjects())
            {
                if (obj.active && obj.type == ObjectType::ObjectType52 &&
                    std::fabs(obj.currentX - bridgeX) < 0.5f && std::fabs(obj.currentZ - bridgeZ) < 0.5f)
                {
                    ++count;
                }
            }
            return count;
        };

        bridgeWorld.Update(dt); // advances obj.phase for the object spawned by the call just below
        bridgeInteraction.Update(dt, bridgeWorld, bridgeX, bridgeY, bridgeZ, 0.0f, sound);
        check(countBridgeObjsHere() == 1, "standing on the Bridge tile spawns exactly one ObjectType52");

        // Advance to well within the documented 112-tick hollow window
        // (ticks 28-139) -- the cell must have genuinely lost its ground
        // collision (Air), not just changed its render icon. obj.phase only
        // advances via GEWorldRuntime::Update() itself (the real game loop
        // calls this every frame before GEInteractionSystem::Update(), same
        // convention as the dynamite-fuse test above), not a no-op skip.
        for (int i = 0; i < 80; ++i)
        {
            bridgeWorld.Update(dt);
            bridgeInteraction.Update(dt, bridgeWorld, bridgeX, bridgeY, bridgeZ, 0.0f, sound);
        }
        check(bridgeWorld.GetWorld().getBlock(88, 0, 97).isAir(),
              "mid-construction (tick ~80), the bridge cell is genuinely non-solid (real ground-collision "
              "toggle, not a purely cosmetic overlay)");

        // A second, independent GEBlupiController standing on that same
        // now-hollow cell must fall (nothing exists beneath this demo's
        // real chasm) -- proves the collision change is actually consumed
        // by movement, not just visible in the raw block data.
        GEBlupiController fallingBlupi;
        fallingBlupi.SetPosition(bridgeX, bridgeY, bridgeZ);
        fallingBlupi.Step(bridgeWorld.GetWorld(), 0.0f, 0.0f, false, false, false, dt);
        check(!fallingBlupi.IsOnGround(),
              "Blupi is no longer grounded standing on the hollowed-out bridge cell (real fall-through)");

        // Advance to completion (self-deletes at phase 157) -- the cell
        // must be restored to the original Bridge icon, not left hollow.
        for (int i = 0; i < 80; ++i)
        {
            bridgeWorld.Update(dt);
            bridgeInteraction.Update(dt, bridgeWorld, bridgeX + 20.0f, bridgeY, bridgeZ, 0.0f, sound);
        }
        check(bridgeWorld.GetWorld().getBlock(88, 0, 97).type() == GalaxyEggbert::BlockTypes::Bridge,
              "after the full 157-tick sequence, the bridge cell is restored to the original Bridge icon");
        check(countBridgeObjsHere() == 0, "the construction object self-deletes once its phase reaches 157");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
