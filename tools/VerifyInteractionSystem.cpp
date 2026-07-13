#include "Game/GEInteractionSystem.hpp"
#include "Game/GESound.hpp"
#include "Game/GETrainingHints.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <cmath>
#include <iostream>
#include <string>

// Scripted, non-interactive verification of GEInteractionSystem (2026-07-10)
// against the real worlds3d/world001.vwr sample world -- proves platform
// lift patrol, crate push, and pickup collection (treasure/egg/key/exit)
// actually work, not just "compiles and doesn't crash". GESound is
// constructed but never LoadContent()-ed, so every Play() call is a no-op
// against an unloaded channel (no audio device needed for this scripted
// check).
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::CNA;

    const std::string worldPath = (argc > 1) ? argv[1] : "worlds3d/world001.vwr";

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

    if (const auto* egg = findFirst(ObjectType::ObjectType6))
    {
        const float ex = egg->currentX, ey = egg->currentY, ez = egg->currentZ;
        for (int i = 0; i < 5; ++i)
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
        for (int i = 0; i < 5; ++i)
        {
            interaction.Update(dt, world, cx, cy, cz, 0.0f, sound);
        }
        check(interaction.TreasuresCollected() == 1, "chest collected exactly once (TreasuresCollected == 1)");
    }
    else
    {
        check(false, "found a chest (ObjectType5) in the sample world");
    }

    if (const auto* key = findFirst(ObjectType::ObjectType49))
    {
        const float kx = key->currentX, ky = key->currentY, kz = key->currentZ;
        for (int i = 0; i < 5; ++i)
        {
            interaction.Update(dt, world, kx, ky, kz, 0.0f, sound);
        }
        check(interaction.Key1Count() == 1, "key collected exactly once (Key1Count == 1)");
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
        check(interaction.Lives() == livesBeforeHazard - 1, "generic hazard contact costs exactly 1 life");
        const auto* afterHazard = findFirst(ObjectType::ObjectType2);
        check(afterHazard == nullptr || !afterHazard->active,
              "the hazard that killed Blupi is destroyed (no longer active)");
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
        for (int i = 0; i < 30; ++i)
        {
            const float moveDX = 0.05f; // walking east this frame
            interaction.Update(dt, world, blupiX, crate->currentY, blupiZ, moveDX, sound);
            blupiX += moveDX;
        }
        const auto* after = findFirst(ObjectType::ObjectType12);
        std::cout << "Crate X after push attempt: " << (after ? after->currentX : -999.0f)
                  << " (started at " << startX << ")" << std::endl;
        check(after != nullptr && after->currentX > startX, "crate was pushed east (currentX increased)");
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
        interaction.Update(dt, world, ddx, ddy, ddz, 0.0f, sound);
        check(interaction.DynamiteCount() == 1, "dynamite pickup increments DynamiteCount() to 1");

        check(!interaction.PlaceDynamite(world, 0.0f, 1.0f, 0.0f, /*grounded=*/false),
              "PlaceDynamite() is a no-op while not grounded");
        check(interaction.DynamiteCount() == 1, "DynamiteCount() unchanged after the failed placement");

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
        for (int i = 0; i < 300; ++i) // 300 * (1/60)s = 5s of simulated time
        {
            world.Update(dt);
            interaction.Update(dt, world, placeX, placeY, placeZ, 0.0f, sound);
        }

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

        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            true, true);
        check(interaction.BulletCount() == bulletsBeforeFiring - 1,
              "firing while in a Tank consumes exactly 1 bullet");

        interaction.Update(dt, world, fireTestX, fireTestY, fireTestZ, 0.0f, sound,
                            false, false, 1, 0, false, true, true, true, true,
                            true, true);
        check(interaction.BulletCount() == bulletsBeforeFiring - 1,
              "holding Fire within the real 0.5s cooldown does not fire again");

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
            treasureDoorInteraction.Update(dt, world, freshChests[0]->currentX, freshChests[0]->currentY,
                                            freshChests[0]->currentZ, 0.0f, sound);
            check(treasureDoorTileType() == 422,
                  "the door stays closed after only 1 of the 2 required treasures is collected");
            treasureDoorInteraction.Update(dt, world, freshChests[1]->currentX, freshChests[1]->currentY,
                                            freshChests[1]->currentZ, 0.0f, sound);
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
        interaction.Update(dt, world, 5.0f, 1.0f, 5.0f, 0.0f, sound);
        check(interaction.DiedThisFrame(), "DiedThisFrame() is true touching an injected spider (ObjectType16)");
        check(interaction.Lives() == livesBeforeSpider - 1, "spider contact costs exactly 1 life, same as ObjectType2/3");

        bool spiderStillActive = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType16)
            {
                spiderStillActive = obj.active;
            }
        }
        check(!spiderStillActive, "the spider that killed Blupi is destroyed, same as ObjectType2/3");
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
        const bool bulletCostALife =
            (interaction.Lives() == livesBeforeBullet - 1) || (interaction.Lives() == 3 && livesBeforeBullet <= 1);
        check(bulletCostALife, "blupih's projectile contact costs exactly 1 life");

        const int bulletCountAfterContact = countBulletsAt(bhX, bhZ);
        check(bulletCountAfterContact == bulletCountBefore, "the projectile that killed Blupi is destroyed (no longer active)");

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
        for (int i = 0; i < 250 && !selfDestructed; ++i)
        {
            interaction.Update(dt, world, beyondWallX, fY, fZ, 0.0f, sound);
            const auto* current = findHoming();
            if (current != nullptr && !current->active)
            {
                selfDestructed = true;
                framesToDestruct = i;
            }
        }
        std::cout << "Follower self-destructed after " << framesToDestruct << " frame(s) approaching the wall" << std::endl;
        check(selfDestructed, "a homing follower self-destructs when its next step would land inside solid terrain");
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

                cheatInteraction.CheatCleanAll(cheatWorld);

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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
