#include "Game/GEInteractionSystem.hpp"
#include "Game/GESound.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <cmath>
#include <iostream>

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
    // far end and ping-ponged back to the exact start value.
    float liftStartY = -1.0f;
    for (const auto& obj : world.GetMobileObjects())
    {
        if (obj.type == ObjectType::ObjectType1)
        {
            liftStartY = obj.currentY;
            break;
        }
    }
    check(liftStartY >= 0.0f, "found the platform lift (ObjectType1) in the sample world");

    for (int i = 0; i < 30; ++i)
    {
        interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound); // Blupi far away
    }
    float liftYAfter = liftStartY;
    for (const auto& obj : world.GetMobileObjects())
    {
        if (obj.type == ObjectType::ObjectType1)
        {
            liftYAfter = obj.currentY;
            break;
        }
    }
    std::cout << "Lift Y after 0.5s: " << liftYAfter << " (started at " << liftStartY << ")" << std::endl;
    check(std::fabs(liftYAfter - liftStartY) > 0.05f, "platform lift patrols (currentY changed)");

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
        // (IsBalloonPoppableHazard()): while ballooned, contact pops the
        // balloon instead of killing, and does NOT destroy the follower
        // either (the real Decor.cpp:5766-5781 pop branch has no
        // ObjectDelete call at all).
        MobileObjSpec follower;
        follower.type = ObjectType::ObjectType96;
        follower.posStartX = follower.posEndX = follower.currentX = 15.0f;
        follower.posStartY = follower.posEndY = follower.currentY = 1.0f;
        follower.posStartZ = follower.posEndZ = follower.currentZ = 15.0f;
        world.GetMobileObjectsMutable().push_back(follower);

        const int livesBeforeFollower = interaction.Lives();
        interaction.Update(dt, world, 15.0f, 1.0f, 15.0f, 0.0f, sound, /*blupiCrouching=*/false, /*blupiBallooned=*/true);
        check(interaction.BalloonPoppedThisFrame(), "BalloonPoppedThisFrame() is true touching a follower while ballooned");
        check(!interaction.DiedThisFrame(), "the pop happens instead of a kill while ballooned");
        check(interaction.Lives() == livesBeforeFollower, "a popped balloon costs no life");
        bool followerStillActive = false;
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == ObjectType::ObjectType96) followerStillActive = obj.active;
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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
