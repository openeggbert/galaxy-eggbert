#include "Game/GEBlupiController.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cmath>
#include <filesystem>
#include <iostream>

// Scripted, non-interactive verification of GEBlupiController's grid
// collision (plan.md E3D-MIG-060): loads a world and drives Step() with
// scripted input instead of live keyboard input, so this proves step-up
// traversal, wall blocking, and gravity/landing actually work against the
// real worlds3d/world999.vwr geometry (renamed from world001.vwr 2026-07-17
// when the real 78-world hub structure took over world001.vwr as the
// genuine global hub) — not just "compiles and doesn't crash".
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::CNA;

    const std::filesystem::path worldPath = (argc > 1) ? argv[1] : "worlds3d/world999.vwr";
    const Worlds::World world = Worlds::World::loadFromFile(worldPath);

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    // INFRA-007 (plan.md §7): GEBlupiController's 3 one-shot sound cues
    // moved from parallel *ThisFrame() booleans to one typed event queue
    // (EventsThisFrame()) -- this helper mirrors what the old individual
    // getters used to check, so the assertions below read the same.
    const auto hasEvent = [](const GEBlupiController& controller, GEBlupiController::EventKind kind)
    {
        for (const auto& event : controller.EventsThisFrame())
        {
            if (event.kind == kind) return true;
        }
        return false;
    };

    constexpr float dt = 1.0f / 60.0f;

    // 1. Spawn on the ground floor (world (0,1,0) == grid (50,*,50)); the
    // controller should recognize it is already standing on solid ground.
    GEBlupiController blupi;
    blupi.SetPosition(0.0f, 1.0f, 0.0f);
    blupi.Step(world, 0.0f, 0.0f, false, false, false, dt);
    check(blupi.IsOnGround(), "spawns grounded on the ground floor");

    // 2. Walk west (-X) toward and up the 10-step staircase (world x from
    // -21 down to -30, world z band [-5,4] -- spawn z=0 is inside it).
    // Expect Y to climb via step-up traversal. Tank controls move along the
    // current facing direction, so face west (-X) first: yaw=0 faces -Z,
    // so -90 deg (-pi/2 rad) faces -X.
    constexpr float kFaceWest = -1.57079633f;
    blupi.SetYaw(kFaceWest);
    for (int i = 0; i < 400; ++i) // ~6.7 simulated seconds
    {
        blupi.Step(world, 0.0f, 1.0f, false, false, false, dt);
    }
    std::cout << "After walking west: x=" << blupi.GetX() << " y=" << blupi.GetY()
              << " z=" << blupi.GetZ() << " onGround=" << blupi.IsOnGround() << std::endl;
    check(blupi.GetY() >= 8.0f, "climbed the staircase via step-up traversal (Y >= 8)");
    check(blupi.IsOnGround(), "stands on top of the staircase/platform, not falling through it");

    // 3. Keep walking west into the room's far wall (world x=-45) -- should
    // block, not clip through.
    for (int i = 0; i < 300; ++i)
    {
        blupi.Step(world, 0.0f, 1.0f, false, false, false, dt);
    }
    std::cout << "After walking into wall: x=" << blupi.GetX() << std::endl;
    check(blupi.GetX() > -45.0f, "wall blocks horizontal movement (did not clip through x=-45 wall)");

    // 4. Drop from height over the tested corridor's real floor (world
    // (10,*,0), raw grid (60,*,50) -- inside fill(20,75,0,0,48,52,RockPile))
    // and confirm gravity + landing works. NOT world (40,*,40) (outside any
    // placed structure) as this test originally used -- plan.md
    // E3D-MIG-067's fall-off-world fix (2026-07-11) means a genuinely
    // floorless column now correctly free-falls forever rather than
    // "landing" at a fake Y=0 floor, so this test needs a location with a
    // REAL floor to verify actual gravity/landing physics; the empty-
    // column case is covered separately below (GroundHeightAt() returning
    // kNoGround).
    GEBlupiController faller;
    faller.SetPosition(10.0f, 20.0f, 0.0f);
    for (int i = 0; i < 200 && !faller.IsOnGround(); ++i)
    {
        faller.Step(world, 0.0f, 0.0f, false, false, false, dt);
    }
    std::cout << "Faller landed at y=" << faller.GetY() << std::endl;
    check(faller.IsOnGround(), "falls under gravity and lands");
    check(faller.GetY() < 5.0f, "lands far below the y=20 drop height (real gravity, not a snap)");
    check(faller.GetY() >= 0.0f, "lands on the real corridor floor, not the removed fake Y=0 fallback");

    // 4b. Fall-off-world (plan.md E3D-MIG-067, fixed 2026-07-11): drop over
    // a genuinely floorless column (world (40,*,40), raw grid (90,*,90) --
    // outside any placed structure in this world) and confirm Blupi falls
    // straight through, well past the old buggy Y=0 "floor", rather than
    // stopping there. This is a direct regression test for a real bug
    // found via live playtest: GroundHeightAt()'s old `return 0` fallback
    // for "no solid block in this column" was silently treated as solid
    // ground, making Blupi stop dead at Y=0.000 in open space with
    // nothing beneath him -- confirmed live before this fix (onGround
    // became true at exactly Y=0), and unreachable via any existing test
    // before this one, since every other drop test in this file lands on
    // a real floor.
    GEBlupiController fallsForever;
    fallsForever.SetPosition(40.0f, 20.0f, 40.0f);
    for (int i = 0; i < 180; ++i) // 3s -- comfortably past the y=20->y<0 fall time seen live (~2.25s)
    {
        fallsForever.Step(world, 0.0f, 0.0f, false, false, false, dt);
    }
    std::cout << "Fell-forever Y after 3s over an empty column: " << fallsForever.GetY() << std::endl;
    check(!fallsForever.IsOnGround(), "Blupi never lands over a genuinely floorless column (no fake Y=0 floor)");
    check(fallsForever.GetY() < 0.0f,
          "Blupi's Y drops below 0 over a floorless column, proving he's NOT clamped to a fake floor there");

    // 4c. Real bug found 2026-07-12 while live-testing vehicles (plan.md
    // E3D-MIG-171): TryMoveAxis() silently froze ALL horizontal movement
    // once Y fell far enough negative during a sustained fall (m_y below
    // roughly -2), because its step-up check compared the destination
    // column's ground height directly against m_y without accounting for
    // "I'm currently far below because I'm falling, not because that's an
    // unclimbable wall". Regression test: hold forward movement while
    // falling over the same genuinely floorless column above for long
    // enough that the old bug would have kicked in (well past y=-2), and
    // confirm Z keeps changing throughout, not just for the first couple
    // of frames.
    GEBlupiController fallingMover;
    fallingMover.SetPosition(40.0f, 20.0f, 40.0f);
    fallingMover.SetYaw(0.0f); // facing -Z
    float zSample1 = 0.0f, zSample2 = 0.0f;
    float minYSeen = 20.0f;
    for (int i = 0; i < 300; ++i) // 5s -- comfortably past y=-2 and well into the old bug's range
    {
        fallingMover.Step(world, 0.0f, 1.0f, false, false, false, dt);
        minYSeen = std::min(minYSeen, fallingMover.GetY());
        if (i == 30) zSample1 = fallingMover.GetZ();  // early in the fall (still above y=-2)
        if (i == 299) zSample2 = fallingMover.GetZ(); // 5s later (he may have drifted onto real ground by now)
    }
    std::cout << "Falling mover Z: early=" << zSample1 << ", late=" << zSample2 << ", min Y seen="
              << minYSeen << std::endl;
    check(minYSeen < -2.0f,
          "sanity: this drop genuinely goes deep enough (Y below -2) to trigger the old bug at some point");
    check(zSample2 != zSample1,
          "Z keeps changing over the course of a long fall while holding movement input (real bug: it used "
          "to freeze solid once Y fell below about -2)");

    // 4c. Animation state: Jump (ascending) vs Air (falling) split
    // (2026-07-11, plan.md E3D-MIG-064 -- expanding the bottom-right
    // animation indicator beyond its original Stop/March/Jump/Down/Up
    // debug-stopgap set). Real BlupiAction IDs 4 (Jump) and 5 (Air); Air's
    // frame data is ported from GalaxyEggbertSimple3D::GEBlupiController's
    // own already-approved kAirFrames, not a fresh mobile-eggbert
    // transcription. Verifies the velocity-sign-based split this class
    // uses (ascending = Jump, falling/apex = Air) in place of Simple3D's
    // own frame-counted trigger window.
    {
        GEBlupiController anim;
        anim.SetPosition(0.0f, 1.0f, 0.0f);
        anim.Step(world, 0.0f, 0.0f, false, false, false, dt);
        check(anim.GetAnimState() == GEBlupiController::AnimState::Stop,
              "grounded and idle starts in the Stop anim state");
        check(anim.GetAnimIcon() == 0, "Stop anim icon is the real icon 0");
        check(!anim.AnimIconUsesElementSheet(),
              "Stop's anim icon is on blupi.png, not element.png (the real BlupiSearchIcon() default)");

        anim.Step(world, 0.0f, 0.0f, true, false, false, dt); // jumpPressed
        check(!anim.IsOnGround(), "jump launches Blupi airborne");
        check(anim.GetAnimState() == GEBlupiController::AnimState::Jump,
              "freshly-launched jump (ascending, velocityY > 0) is the Jump anim state");
        check(anim.GetAnimIcon() == 17, "Jump anim icon starts at the real first jump frame (icon 17)");

        int stepsToApex = 0;
        while (anim.GetAnimState() == GEBlupiController::AnimState::Jump && stepsToApex < 200)
        {
            anim.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToApex;
        }
        check(stepsToApex < 200, "reaches the jump apex (velocityY turns non-positive) within a bounded time");
        check(anim.GetAnimState() == GEBlupiController::AnimState::Air,
              "past the apex, falling (velocityY <= 0) switches to the Air anim state, not still Jump");
        check(anim.GetAnimIcon() == 169, "Air anim icon starts at the real first air frame (icon 169)");

        int stepsToLand = 0;
        while (!anim.IsOnGround() && stepsToLand < 200)
        {
            anim.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToLand;
        }
        check(anim.IsOnGround(), "lands again after the jump arc completes");
        check(anim.GetAnimState() == GEBlupiController::AnimState::Stop,
              "back on the ground and idle returns to the Stop anim state, not stuck in Air");
    }

    // GetDisplayAnimIcon() (BLUPI-079, added 2026-07-20) -- real
    // table_mirror direction-substitution, approximated via sin(yaw) sign
    // since this engine has no discrete left/right facing. Jump (icon 17)
    // is a convenient known value: real table_mirror[17]=20.
    {
        GEBlupiController facingRight;
        facingRight.SetPosition(0.0f, 1.0f, 0.0f);
        facingRight.SetYaw(2.0f); // sin(2.0) > 0 -- "facing right" proxy
        facingRight.Step(world, 0.0f, 0.0f, false, false, false, dt); // settle onto ground first
        facingRight.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(facingRight.GetAnimIcon() == 17, "Jump base icon is still the real unmirrored first frame (icon 17)");
        check(facingRight.GetDisplayAnimIcon() == 17,
              "GetDisplayAnimIcon() matches GetAnimIcon() unmirrored while facing right");

        GEBlupiController facingLeft;
        facingLeft.SetPosition(0.0f, 1.0f, 0.0f);
        facingLeft.SetYaw(-2.0f); // sin(-2.0) < 0 -- "facing left" proxy
        facingLeft.Step(world, 0.0f, 0.0f, false, false, false, dt); // settle onto ground first
        facingLeft.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(facingLeft.GetAnimIcon() == 17, "Jump base icon is unaffected by yaw (GetAnimIcon() never mirrors)");
        check(facingLeft.GetDisplayAnimIcon() == 20,
              "GetDisplayAnimIcon() applies the real table_mirror substitution while facing left (icon 17 -> 20)");

        GEBlupiController invertedLeft;
        invertedLeft.SetPosition(0.0f, 1.0f, 0.0f);
        invertedLeft.SetYaw(-2.0f);
        check(invertedLeft.TriggerInvert(), "TriggerInvert() succeeds from a fresh, non-Hide state");
        invertedLeft.Step(world, 0.0f, 0.0f, false, false, false, dt); // settle onto ground first
        invertedLeft.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(invertedLeft.GetDisplayAnimIcon() == 17,
              "m_invert flips the facing sense back, matching the real m_blupiInvert twist "
              "(facing-left yaw + Invert active = no mirroring, same as facing right unmodified)");

        GEBlupiController gluLeft;
        gluLeft.SetPosition(0.0f, 1.0f, 0.0f);
        gluLeft.SetYaw(-2.0f);
        check(gluLeft.TriggerDeathLock(GEBlupiController::DeathCause::Glu, true),
              "TriggerDeathLock(Glu) succeeds from a fresh state");
        gluLeft.Step(world, 0.0f, 0.0f, false, false, false, dt);
        check(gluLeft.AnimIconUsesElementSheet(), "Glu is one of the real element.png-sourced death causes");
        check(gluLeft.GetAnimIcon() == 168, "Glu base icon starts at the real first frame (icon 168)");
        check(gluLeft.GetDisplayAnimIcon() == 172,
              "GetDisplayAnimIcon() applies the real +4 element.png special case for icons 168-171 "
              "while facing left (icon 168 -> 172), NOT the main table_mirror substitution");
    }

    // 4d. Animation state: Down (grounded crouch) icon cycling. Real
    // BlupiAction ID 6 has 3 real icon frames (33, 34, 35) per
    // table_blupi/mobile-eggbert-reference/08-animations.md -- this used
    // to be a single-frame array (icon 33 only), a transcription bug fixed
    // 2026-07-19 while cross-checking table_blupi for the skate/tank
    // animation wiring below.
    {
        GEBlupiController crouching;
        crouching.SetPosition(0.0f, 1.0f, 0.0f);
        crouching.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
        check(crouching.GetAnimState() == GEBlupiController::AnimState::Down,
              "grounded crouch input enters the Down anim state");
        check(crouching.GetAnimIcon() == 33, "Down anim icon starts at the real first frame (icon 33)");

        const int expectedCycle[] = {34, 35, 33};
        for (int expectedIcon : expectedCycle)
        {
            int steps = 0;
            const int previousIcon = crouching.GetAnimIcon();
            while (crouching.GetAnimIcon() == previousIcon && steps < 200)
            {
                crouching.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
                ++steps;
            }
            check(steps < 200, "Down anim icon advances to its next frame within a bounded time");
            check(crouching.GetAnimIcon() == expectedIcon,
                  "Down anim icon cycles through its real 3-frame sequence (33, 34, 35, then back to 33)");
        }
    }

    // 4e. Real crouch/look-up one-shot sound signals (SOUND-017/030/031,
    // added 2026-07-20, Decor.cpp:3278-3287/3628-3633): ch7/ch21 fire once,
    // real 0.2s (Config::ScaleTime(4)) after entering Down/Up; ch20 fires
    // once specifically on the Down->Stop release transition, NOT
    // Down->March (real gate: m_blupiSpeedX==0 && m_blupiSpeedY==0, i.e.
    // no movement input either -- this engine's own crouchHeld ternary
    // only reaches Stop, not March, under that same condition).
    {
        using EventKind = GEBlupiController::EventKind;

        GEBlupiController crouchSound;
        crouchSound.SetPosition(0.0f, 1.0f, 0.0f);
        crouchSound.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
        check(!hasEvent(crouchSound, EventKind::DownEntrySoundFired),
              "DownEntrySoundFired is not in EventsThisFrame() the instant Down is entered (real 0.2s delay)");

        int stepsToDownSound = 0;
        while (!hasEvent(crouchSound, EventKind::DownEntrySoundFired) && stepsToDownSound < 200)
        {
            crouchSound.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
            ++stepsToDownSound;
        }
        check(stepsToDownSound < 200, "DownEntrySoundFired fires within a bounded time");
        crouchSound.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
        check(!hasEvent(crouchSound, EventKind::DownEntrySoundFired),
              "DownEntrySoundFired is in EventsThisFrame() for exactly one Step() call, not every frame after");

        crouchSound.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/false, false, dt);
        check(crouchSound.GetAnimState() == GEBlupiController::AnimState::Stop,
              "releasing crouch with no movement input returns to Stop");
        check(hasEvent(crouchSound, EventKind::DownReleaseSoundFired),
              "DownReleaseSoundFired fires on the real Down->Stop release transition");

        GEBlupiController crouchToMarch;
        crouchToMarch.SetPosition(0.0f, 1.0f, 0.0f);
        crouchToMarch.Step(world, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
        crouchToMarch.Step(world, 1.0f, 1.0f, false, /*crouchHeld=*/false, false, dt);
        check(crouchToMarch.GetAnimState() == GEBlupiController::AnimState::March,
              "releasing crouch WITH movement input transitions to March, not Stop");
        check(!hasEvent(crouchToMarch, EventKind::DownReleaseSoundFired),
              "DownReleaseSoundFired does NOT fire on a Down->March transition (real gate "
              "requires speedX==0 too)");

        GEBlupiController lookingUp;
        lookingUp.SetPosition(0.0f, 1.0f, 0.0f);
        lookingUp.Step(world, 0.0f, 0.0f, false, false, /*lookUpHeld=*/true, dt);
        check(!hasEvent(lookingUp, EventKind::UpEntrySoundFired),
              "UpEntrySoundFired is not in EventsThisFrame() the instant Up is entered (real 0.2s delay)");
        int stepsToUpSound = 0;
        while (!hasEvent(lookingUp, EventKind::UpEntrySoundFired) && stepsToUpSound < 200)
        {
            lookingUp.Step(world, 0.0f, 0.0f, false, false, /*lookUpHeld=*/true, dt);
            ++stepsToUpSound;
        }
        check(stepsToUpSound < 200, "UpEntrySoundFired fires within a bounded time");
    }

    // 5. GetGroundBlockType() (plan.md E3D-MIG-140, lava-hazard detection) --
    // a small synthetic world (not worlds3d/world999.vwr, which has no lava
    // placed yet) with one lava block and one ordinary ground block,
    // isolated from anything else this tool tests against.
    {
        Worlds::World synthetic;
        constexpr std::uint16_t kGroundX = 10, kGroundZ = 10;
        constexpr std::uint16_t kLavaX = 20, kLavaZ = 20;
        synthetic.setBlock(kGroundX, 0, kGroundZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kLavaX, 0, kLavaZ, Worlds::Block::make(BlockTypes::Lava));

        GEBlupiController onGround;
        onGround.SetPosition(static_cast<float>(kGroundX) - 50.0f /* kWorldCenterX */,
                              1.0f, static_cast<float>(kGroundZ) - 50.0f /* kWorldCenterZ */);
        onGround.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onGround.GetGroundBlockType(synthetic) == BlockTypes::Ground,
              "GetGroundBlockType() identifies ordinary ground correctly");

        GEBlupiController onLava;
        onLava.SetPosition(static_cast<float>(kLavaX) - 50.0f, 1.0f, static_cast<float>(kLavaZ) - 50.0f);
        onLava.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onLava.IsOnGround(), "Blupi stands on a lava block rather than falling through it");
        check(onLava.GetGroundBlockType(synthetic) == BlockTypes::Lava,
              "GetGroundBlockType() identifies lava correctly (E3D-MIG-140 hazard detection)");

        GEBlupiController airborne;
        airborne.SetPosition(static_cast<float>(kLavaX) - 50.0f, 20.0f, static_cast<float>(kLavaZ) - 50.0f);
        check(airborne.GetGroundBlockType(synthetic) == BlockTypes::Air,
              "GetGroundBlockType() returns Air while airborne, even directly above lava");

        // Spikes (plan.md E3D-MIG-141) -- same synthetic world, one more block.
        constexpr std::uint16_t kSpikeX = 30, kSpikeZ = 30;
        synthetic.setBlock(kSpikeX, 0, kSpikeZ, Worlds::Block::make(BlockTypes::Spike));
        GEBlupiController onSpike;
        onSpike.SetPosition(static_cast<float>(kSpikeX) - 50.0f, 1.0f, static_cast<float>(kSpikeZ) - 50.0f);
        onSpike.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onSpike.IsOnGround(), "Blupi stands on a spike block rather than falling through it");
        check(onSpike.GetGroundBlockType(synthetic) == BlockTypes::Spike,
              "GetGroundBlockType() identifies spikes correctly (E3D-MIG-141 hazard detection)");

        // Water drip (plan.md TILE-032, real Decor::IsGoutte, icon 404,
        // confirmed 2026-07-14 via direct Decor.cpp read) -- same
        // recognition-only test shape as Lava/Spike above (the actual
        // kill trigger lives in GalaxyEggbertCnaGame::Update(), same as
        // every other hazard here, not separately unit-testable yet).
        constexpr std::uint16_t kDripX = 31, kDripZ = 31;
        synthetic.setBlock(kDripX, 0, kDripZ, Worlds::Block::make(BlockTypes::Drip));
        GEBlupiController onDrip;
        onDrip.SetPosition(static_cast<float>(kDripX) - 50.0f, 1.0f, static_cast<float>(kDripZ) - 50.0f);
        onDrip.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onDrip.IsOnGround(), "Blupi stands on a water-drip block rather than falling through it");
        check(onDrip.GetGroundBlockType(synthetic) == BlockTypes::Drip,
              "GetGroundBlockType() identifies the water-drip hazard correctly (real icon 404)");

        // Suspended/hanging bar-and-rope mode (plan.md TILE-045, real
        // Decor::GetTypeBarre()/m_blupiSuspend, icons 138/202 confirmed
        // 2026-07-14 via a direct Decor.cpp read). Own synthetic world,
        // well away from any other test's coordinates: a floor at gz=70,
        // a 3-cell gap (gz=71-73) spanned by a bar tile (icon 138) one row
        // above the floor (Hanging over open air), then floor resumes at
        // gz=74 with the SAME bar tile continuing over it (LandingAvailable,
        // since the cell below is now solid).
        {
            Worlds::World barreWorld;
            constexpr std::uint16_t kBarreX = 60;
            barreWorld.setBlock(kBarreX, 0, 70, Worlds::Block::make(BlockTypes::Ground));
            for (std::uint16_t z = 71; z <= 74; ++z)
            {
                barreWorld.setBlock(kBarreX, 1, z, Worlds::Block::make(static_cast<std::uint16_t>(138)));
            }
            barreWorld.setBlock(kBarreX, 0, 74, Worlds::Block::make(BlockTypes::Ground));
            const float barreWorldX = static_cast<float>(kBarreX) - 50.0f;

            // Grab: standing at a Hanging cell (gz=71, open air below).
            GEBlupiController grabber;
            grabber.SetPosition(barreWorldX, 1.0f, 71.0f - 50.0f);
            grabber.Step(barreWorld, 0.0f, 0.0f, false, false, false, dt);
            check(grabber.IsSuspended(), "standing at a bar cell over open air grabs it automatically");

            // Movement while hanging: face +Z (yaw=pi) and walk toward the
            // landing at gz=74.
            GEBlupiController climber;
            climber.SetPosition(barreWorldX, 1.0f, 71.0f - 50.0f);
            climber.SetYaw(3.14159265f); // facing +Z
            climber.Step(barreWorld, 0.0f, 0.0f, false, false, false, dt);
            check(climber.IsSuspended(), "sanity: grabbed the bar before attempting to climb it");
            for (int i = 0; i < 400 && climber.IsSuspended(); ++i) // generous bound, real move is slow/frame
            {
                climber.Step(barreWorld, 0.0f, 1.0f, false, false, false, dt);
            }
            check(!climber.IsSuspended() && climber.IsOnGround(),
                  "climbing to the far end (landing available) releases gracefully onto solid ground, "
                  "not a free-fall");
            check(climber.GetZ() > 71.0f - 50.0f,
                  "the climb actually moved Blupi forward along the bar (not stuck in place)");

            // Walking the OTHER way (back toward gz=70, off the bar
            // entirely) drops Blupi into free-fall instead.
            GEBlupiController dropper;
            dropper.SetPosition(barreWorldX, 1.0f, 71.0f - 50.0f);
            dropper.SetYaw(0.0f); // facing -Z
            dropper.Step(barreWorld, 0.0f, 0.0f, false, false, false, dt);
            check(dropper.IsSuspended(), "sanity: grabbed the bar before walking off the near end");
            bool sawFreefall = false;
            for (int i = 0; i < 400; ++i)
            {
                dropper.Step(barreWorld, 0.0f, 1.0f, false, false, false, dt);
                if (!dropper.IsSuspended() && !dropper.IsOnGround())
                {
                    sawFreefall = true;
                    break;
                }
            }
            check(sawFreefall, "walking off the bar's near end (no bar tile, gap below) drops Blupi "
                                "into free-fall instead of releasing gracefully");

            // Jump-to-release: immediate upward velocity, not a delayed
            // wind-up (no visible model exists to show one).
            GEBlupiController jumper;
            jumper.SetPosition(barreWorldX, 1.0f, 72.0f - 50.0f);
            jumper.Step(barreWorld, 0.0f, 0.0f, false, false, false, dt);
            check(jumper.IsSuspended(), "sanity: grabbed the bar before testing jump-release");
            jumper.Step(barreWorld, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
            check(!jumper.IsSuspended(), "pressing Jump while suspended releases the hang");
            check(jumper.GetVelocityY() > 0.0f,
                  "the jump-release gives Blupi a real upward launch (real fixed -11.0 equivalent)");

            // Grace timer (real m_blupiNoBarre, 5 ticks @ 20Hz = 0.25s):
            // force Blupi back to the exact bar cell right after release
            // (SetPosition() doesn't touch the grace timer) -- the very
            // next Step() must NOT instantly re-grab despite standing
            // exactly on a Hanging cell again.
            jumper.SetPosition(barreWorldX, 1.0f, 72.0f - 50.0f);
            jumper.Step(barreWorld, 0.0f, 0.0f, false, false, false, dt);
            check(!jumper.IsSuspended(),
                  "the grace timer blocks an instant re-grab even when forced back onto the same bar cell");

            // Past the real ~0.25s grace window, the same cell becomes
            // grabbable again (matches the real 5-tick expiry, not a
            // permanent lockout).
            for (int i = 0; i < 20 && !jumper.IsSuspended(); ++i) // 20 * (1/60)s > 0.25s
            {
                jumper.SetPosition(barreWorldX, 1.0f, 72.0f - 50.0f);
                jumper.Step(barreWorld, 0.0f, 0.0f, false, false, false, dt);
            }
            check(jumper.IsSuspended(), "the same bar cell becomes grabbable again once the grace timer expires");
        }

        // Crusher squash state (plan.md E3D-MIG-143) -- TriggerCrush()/
        // IsEcrased()/recovery, standing on the same ordinary ground block
        // used above (the trigger *condition* -- Crusher block + active
        // cycle -- is GalaxyEggbertCnaGame's job, tested separately in
        // GEWorldRuntime::IsCrusherActiveAtPhase(); this only tests
        // GEBlupiController's own state machine once triggered).
        GEBlupiController crushed;
        crushed.SetPosition(static_cast<float>(kGroundX) - 50.0f, 1.0f, static_cast<float>(kGroundZ) - 50.0f);
        crushed.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(crushed.TriggerCrush(), "TriggerCrush() returns true on a genuinely new trigger");
        check(crushed.IsEcrased(), "IsEcrased() is true immediately after TriggerCrush()");
        check(!crushed.TriggerCrush(), "TriggerCrush() is a no-op (returns false) while already squashed");

        // Animation indicator (plan.md E3D-MIG-064, 2026-07-11, real
        // BlupiAction IDs 72/73, `table_blupi` icon data transcribed with
        // explicit user approval): squashed+idle is StopEcrase, real icon
        // 320.
        crushed.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(crushed.GetAnimState() == GEBlupiController::AnimState::StopEcrase,
              "squashed and idle is the StopEcrase anim state");
        check(crushed.GetAnimIcon() == 320, "StopEcrase anim icon is the real icon 320");

        // Reduced move speed while squashed: same moveInput/dt, less
        // distance covered than an un-squashed Blupi over one Step(). At
        // yaw=0 (the default), forward movement changes Z, not X.
        const float zBeforeCrushedMove = crushed.GetZ();
        crushed.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
        const float crushedDelta = std::fabs(crushed.GetZ() - zBeforeCrushedMove);
        check(crushed.GetAnimState() == GEBlupiController::AnimState::MarchEcrase,
              "squashed and moving switches to the MarchEcrase anim state");
        check(crushed.GetAnimIcon() == 319, "MarchEcrase anim icon starts at the real first frame (icon 319)");

        GEBlupiController normal;
        normal.SetPosition(static_cast<float>(kGroundX) - 50.0f, 1.0f, static_cast<float>(kGroundZ) - 50.0f);
        normal.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        const float zBeforeNormalMove = normal.GetZ();
        normal.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
        const float normalDelta = std::fabs(normal.GetZ() - zBeforeNormalMove);
        check(crushedDelta > 0.0f && crushedDelta < normalDelta,
              "squashed Blupi moves slower than normal, but still moves (not fully immobilized)");

        // Jump blocked while squashed.
        crushed.Step(synthetic, 0.0f, 0.0f, true, false, false, dt);
        check(crushed.IsOnGround(), "jump input is ignored while squashed (still on ground, not launched)");

        // Auto-recovery after kEcraseDuration seconds.
        const int stepsToRecover = static_cast<int>(GEBlupiController::kEcraseDuration / dt) + 5;
        for (int i = 0; i < stepsToRecover && crushed.IsEcrased(); ++i)
        {
            crushed.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(!crushed.IsEcrased(), "squash state auto-recovers after kEcraseDuration seconds");

        // Wasp "balloon" status (plan.md E3D-MIG-135) -- TriggerBalloon()/
        // IsBallooned()/PopBalloon(), same synthetic world (falling
        // behavior only, doesn't need any particular ground block).
        GEBlupiController ballooned;
        ballooned.SetPosition(0.0f, 20.0f, 0.0f);
        check(ballooned.TriggerBalloon(), "TriggerBalloon() returns true on a genuinely new trigger");
        check(ballooned.IsBallooned(), "IsBallooned() is true immediately after TriggerBalloon()");
        check(!ballooned.TriggerBalloon(), "TriggerBalloon() is a no-op (returns false) while already ballooned");

        // Animation indicator (plan.md E3D-MIG-064, 2026-07-11, real
        // BlupiAction ID 66, `table_blupi` icon data transcribed with
        // explicit user approval): ballooned is always the Balloon anim
        // state, regardless of grounded/airborne (the real data has no
        // separate air variant).
        ballooned.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(ballooned.GetAnimState() == GEBlupiController::AnimState::Balloon,
              "ballooned is the Balloon anim state");
        check(ballooned.GetAnimIcon() == 291, "Balloon anim icon starts at the real first frame (icon 291)");

        // Ballooned Blupi RISES (2026-07-18, corrected twice: first from a
        // "falls slower" 20%-gravity approximation, then from a "frozen at a
        // fixed height" reading). The real dedicated balloon-movement block
        // (Decor.cpp:4039-4058, found only on the third pass) continuously
        // accelerates him upward to a terminal rise -- see
        // GEBlupiController::kBalloonRiseSpeed's own comment for the full
        // real-source citation and unit conversion.
        GEBlupiController falling;
        falling.SetPosition(0.0f, 20.0f, 0.0f);
        constexpr int kFallSteps = 30; // ~0.5s, short enough the falling instance doesn't reach the ground
        for (int i = 0; i < kFallSteps; ++i)
        {
            ballooned.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            falling.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(ballooned.GetY() > 20.0f, "ballooned Blupi RISES above the height he was stung at");
        check(ballooned.GetY() > falling.GetY(),
              "ballooned Blupi rises while a normal Blupi falls over the same span");
        check(!ballooned.IsOnGround(), "a risen ballooned Blupi is airborne, not grounded");

        // Real terminal rise speed: no input settles at kBalloonRiseSpeed,
        // Jump/Up held reaches the faster kBalloonRiseSpeedFast, Down held
        // decelerates back to a hover at 0 (never into a descent) --
        // Decor.cpp:4041-4057's own three branches.
        {
            GEBlupiController riser;
            riser.SetPosition(0.0f, 20.0f, 0.0f);
            riser.TriggerBalloon();
            // Well past the ~0.9s needed to reach terminal from 0.
            for (int i = 0; i < 180; ++i)
            {
                riser.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            const float coastY = riser.GetY();
            riser.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            const float coastRise = (riser.GetY() - coastY) / dt;
            check(std::fabs(coastRise - GEBlupiController::kBalloonRiseSpeed) < 0.02f,
                  "no-input ballooned rise settles at the real terminal kBalloonRiseSpeed");

            // Jump held -> the faster real terminal rise.
            for (int i = 0; i < 180; ++i)
            {
                riser.Step(synthetic, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
            }
            const float fastY = riser.GetY();
            riser.Step(synthetic, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
            const float fastRise = (riser.GetY() - fastY) / dt;
            check(std::fabs(fastRise - GEBlupiController::kBalloonRiseSpeedFast) < 0.02f,
                  "Jump held while ballooned reaches the faster real terminal rise");

            // Down (crouch) held -> decelerates to a hover, never a descent.
            for (int i = 0; i < 180; ++i)
            {
                riser.Step(synthetic, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
            }
            const float hoverY = riser.GetY();
            riser.Step(synthetic, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
            check(std::fabs(riser.GetY() - hoverY) < 0.001f,
                  "Down held while ballooned decelerates to a hover, never into a descent");
        }

        // Balloon HORIZONTAL drift (2026-07-18, second remaining gap closed:
        // kBalloonHorizontalSpeed's own comment) -- a held direction ramps
        // toward the real terminal drift speed, and releasing it decelerates
        // back to exactly 0, both at their own distinct real rates.
        {
            GEBlupiController drifter;
            drifter.SetPosition(0.0f, 20.0f, 0.0f);
            drifter.TriggerBalloon();
            for (int i = 0; i < 180; ++i)
            {
                drifter.Step(synthetic, 0.0f, /*moveInput=*/1.0f, false, false, false, dt);
            }
            // Default yaw=0 moves along -Z (this class's own sin/-cos
            // convention), not X -- measure total horizontal distance per
            // step instead of a specific axis, so this doesn't depend on
            // which axis "forward" happens to be.
            const float dxBefore = drifter.GetX(), dzBefore = drifter.GetZ();
            drifter.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            const float driftSpeed =
                std::hypot(drifter.GetX() - dxBefore, drifter.GetZ() - dzBefore) / dt;
            check(std::fabs(driftSpeed - GEBlupiController::kBalloonHorizontalSpeed) < 0.05f,
                  "held-direction balloon drift settles at the real terminal kBalloonHorizontalSpeed");

            for (int i = 0; i < 60; ++i)
            {
                drifter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            const float coastX = drifter.GetX(), coastZ = drifter.GetZ();
            drifter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            check(std::hypot(drifter.GetX() - coastX, drifter.GetZ() - coastZ) < 0.001f,
                  "releasing the direction decelerates balloon drift back to exactly 0");
        }

        // Getting stung force-exits any vehicle (real Decor.cpp:5826-5849:
        // ByeByeHelico() + every vehicle flag cleared) -- needed so the
        // balloon's own horizontal drift takes over immediately instead of
        // the vehicle's ramp system still holding priority via IsInVehicle().
        {
            GEBlupiController vehicleSting;
            vehicleSting.SetPosition(0.0f, 1.0f, 0.0f);
            vehicleSting.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
            check(vehicleSting.IsInVehicle(), "sanity: riding a Jeep before the sting");
            check(vehicleSting.TriggerBalloon(), "TriggerBalloon() succeeds while riding a vehicle");
            check(!vehicleSting.IsInVehicle(), "getting stung force-exits the vehicle (real ByeByeHelico())");
        }

        // Balloon rise stops against a real solid ceiling (2026-07-18, third
        // remaining gap closed: `Decor::TestPath()`'s general swept
        // collision applies to the balloon rise same as any other movement)
        // -- NOT a free clip-through-anything ascent.
        {
            constexpr std::uint16_t kCeilingX = 90, kCeilingZ = 90;
            synthetic.setBlock(kCeilingX, 0, kCeilingZ, Worlds::Block::make(BlockTypes::Ground));
            synthetic.setBlock(kCeilingX, 3, kCeilingZ, Worlds::Block::make(BlockTypes::Ground));
            GEBlupiController capped;
            capped.SetPosition(static_cast<float>(kCeilingX) - 50.0f, 1.0f, static_cast<float>(kCeilingZ) - 50.0f);
            capped.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            check(capped.IsOnGround(), "sanity: standing under the ceiling block before ballooning");
            capped.TriggerBalloon();
            for (int i = 0; i < 300; ++i) // ~5s -- comfortably past reaching the ceiling 2 units up
            {
                capped.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            // Boundary corrected 2026-07-21 (INFRA-005, plan.md §7): the old
            // CeilingHeightAt() used a "-0.5" convention here that turned out
            // to be INCONSISTENT with GroundHeightAt()'s own "+1" convention
            // for the exact same physical relationship (confirmed against
            // GetGroundBlockType()'s own `lround(m_y) - 1`) -- ground cells
            // are bottom-anchored (grid Y owns continuous [Y, Y+1)), so a
            // solid ceiling block at grid Y=3 has its OWN bottom at
            // continuous Y=3.0, not Y=2.5. ResolveMove() now uses one
            // consistent convention for both ground and ceiling (closing
            // this minor, previously-unnoticed inconsistency as a side
            // effect of unification), so the real stopping boundary moved
            // from ~2.5 to ~3.0.
            check(capped.GetY() < 3.0f + 0.01f,
                  "balloon rise stops at the real solid ceiling instead of clipping through it");
            check(capped.GetY() > 1.5f, "balloon rise actually reached up near the ceiling, not stuck low");
        }

        // PopBalloon() clears the status early and forces Blupi briefly
        // airborne, matching the real m_blupiAir=true on a hazard pop.
        ballooned.PopBalloon();
        check(!ballooned.IsBallooned(), "PopBalloon() clears the balloon status immediately");
        check(!ballooned.IsOnGround(), "PopBalloon() forces Blupi airborne (real m_blupiAir=true)");

        // PopBalloon() while NOT ballooned is a documented no-op.
        GEBlupiController notBallooned;
        notBallooned.SetPosition(static_cast<float>(kGroundX) - 50.0f, 1.0f, static_cast<float>(kGroundZ) - 50.0f);
        notBallooned.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        notBallooned.PopBalloon();
        check(notBallooned.IsOnGround(), "PopBalloon() is a no-op while not ballooned (still grounded)");

        // Auto-recovery after kBalloonDuration seconds.
        GEBlupiController recovering;
        recovering.SetPosition(0.0f, 20.0f, 0.0f);
        recovering.TriggerBalloon();
        const int stepsToRecoverBalloon = static_cast<int>(GEBlupiController::kBalloonDuration / dt) + 5;
        for (int i = 0; i < stepsToRecoverBalloon && recovering.IsBallooned(); ++i)
        {
            recovering.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(!recovering.IsBallooned(), "balloon status auto-recovers after kBalloonDuration seconds");

        // Spring bounce (plan.md E3D-MIG-145) -- same synthetic world, one
        // more block (icon 211 = BlockTypes::Spring). GetGroundBlockType()
        // detection first (the trigger *condition* -- GalaxyEggbertCnaGame's
        // job), then TriggerSpringBounce()'s own state machine in
        // isolation, same split as the Crusher tests above.
        constexpr std::uint16_t kSpringX = 40, kSpringZ = 40;
        synthetic.setBlock(kSpringX, 0, kSpringZ, Worlds::Block::make(BlockTypes::Spring));
        GEBlupiController onSpring;
        onSpring.SetPosition(static_cast<float>(kSpringX) - 50.0f, 1.0f, static_cast<float>(kSpringZ) - 50.0f);
        onSpring.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onSpring.IsOnGround(), "Blupi stands on a spring block rather than falling through it");
        check(onSpring.GetGroundBlockType(synthetic) == BlockTypes::Spring,
              "GetGroundBlockType() identifies a spring correctly (E3D-MIG-145 hazard detection)");

        check(onSpring.TriggerSpringBounce(/*jumpHeld=*/false),
              "TriggerSpringBounce() returns true while grounded");
        check(!onSpring.IsOnGround(), "the bounce launches Blupi airborne immediately");
        check(!onSpring.TriggerSpringBounce(/*jumpHeld=*/false),
              "TriggerSpringBounce() is a no-op (returns false) while already airborne");

        // Held-jump bounce launches noticeably higher than a not-held
        // bounce, matching the real source's two distinct magnitudes
        // (-19 held vs -10 not-held, both noPower).
        GEBlupiController bounceHeld;
        bounceHeld.SetPosition(static_cast<float>(kSpringX) - 50.0f, 1.0f, static_cast<float>(kSpringZ) - 50.0f);
        bounceHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        bounceHeld.TriggerSpringBounce(/*jumpHeld=*/true);

        GEBlupiController bounceNotHeld;
        bounceNotHeld.SetPosition(static_cast<float>(kSpringX) - 50.0f, 1.0f, static_cast<float>(kSpringZ) - 50.0f);
        bounceNotHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        bounceNotHeld.TriggerSpringBounce(/*jumpHeld=*/false);

        constexpr int kBounceSampleSteps = 10;
        for (int i = 0; i < kBounceSampleSteps; ++i)
        {
            bounceHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            bounceNotHeld.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        std::cout << "Spring bounce Y after " << kBounceSampleSteps << " steps: held=" << bounceHeld.GetY()
                  << " notHeld=" << bounceNotHeld.GetY() << std::endl;
        check(bounceHeld.GetY() > bounceNotHeld.GetY(),
              "holding Jump on contact launches Blupi noticeably higher than not holding it");

        // Vanishing/Temp tile (plan.md E3D-MIG-146) -- a real Ground block
        // (Y=0) with a Temp tile directly above it (Y=1), so
        // tempPassable=true has a genuine lower floor to fall through TO,
        // avoiding any ambiguity with GroundHeightAt's own "nothing solid
        // anywhere in this column" fallback.
        constexpr std::uint16_t kTempX = 45, kTempZ = 45;
        synthetic.setBlock(kTempX, 0, kTempZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kTempX, 1, kTempZ, Worlds::Block::make(BlockTypes::Temp));

        GEBlupiController onTemp;
        onTemp.SetPosition(static_cast<float>(kTempX) - 50.0f, 2.0f, static_cast<float>(kTempZ) - 50.0f);
        onTemp.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, /*tempPassable=*/false);
        check(onTemp.IsOnGround(), "Blupi stands on a Temp tile during its solid (non-passable) window");
        check(onTemp.GetGroundBlockType(synthetic) == BlockTypes::Temp,
              "GetGroundBlockType() identifies a Temp tile correctly (E3D-MIG-146 detection)");

        // During the real passable window (2 of 20 phase buckets), the SAME
        // Temp tile stops being solid ground -- Blupi should fall through
        // to the real Ground block one cell below instead of staying put.
        for (int i = 0; i < 60 && !(onTemp.IsOnGround() && onTemp.GetY() < 1.9f); ++i)
        {
            onTemp.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, /*tempPassable=*/true);
        }
        std::cout << "Blupi Y after the Temp tile turns passable: " << onTemp.GetY() << std::endl;
        check(std::fabs(onTemp.GetY() - 1.0f) < 0.1f,
              "Blupi falls through the Temp tile onto the real ground one cell below once it's passable");
        check(onTemp.GetGroundBlockType(synthetic) == BlockTypes::Ground,
              "Blupi now stands on the real Ground block beneath, not the Temp tile");

        // Teleporter (plan.md E3D-MIG-147) -- a real Ground floor (Y=0)
        // with a Teleport1 pillar FLOATING one cell above Blupi's standing
        // height (Y=2), matching the real "one tile above his feet"
        // detection exactly. Teleporter icons are always non-solid for
        // collision (GroundHeightAt's own IsTeleporterIcon() skip), so
        // Blupi must land on the REAL floor beneath it (Y=1), not be
        // blocked by or land on top of the floating pillar -- this is
        // also a direct regression test for the bug this redesign fixed
        // (an earlier attempt made teleporter pillars solid, which made
        // Blupi land ON the pillar instead of the floor beneath it).
        constexpr std::uint16_t kTeleX = 42, kTeleZ = 42;
        synthetic.setBlock(kTeleX, 0, kTeleZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kTeleX, 2, kTeleZ, Worlds::Block::make(BlockTypes::Teleport1));

        GEBlupiController onTeleporter;
        onTeleporter.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        onTeleporter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        std::cout << "Blupi Y beneath the floating teleporter pillar: " << onTeleporter.GetY() << std::endl;
        check(onTeleporter.IsOnGround() && std::fabs(onTeleporter.GetY() - 1.0f) < 0.01f,
              "Blupi stands on the real floor beneath the floating teleporter pillar, not on/blocked by it");
        check(onTeleporter.GetBlockTypeAbove(synthetic) == BlockTypes::Teleport1,
              "GetBlockTypeAbove() identifies the teleporter pillar one cell above Blupi (E3D-MIG-147 detection)");

        check(onTeleporter.TriggerTeleport(BlockTypes::Teleport1), "TriggerTeleport() returns true while grounded");
        check(onTeleporter.IsTeleporting(), "IsTeleporting() is true immediately after TriggerTeleport()");
        check(onTeleporter.GetTeleportIcon() == BlockTypes::Teleport1, "GetTeleportIcon() remembers which icon triggered it");
        check(!onTeleporter.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op (returns false) while already teleporting");

        // Animation indicator (plan.md E3D-MIG-064, 2026-07-11, real
        // BlupiAction ID 74, `table_blupi` icon data transcribed with
        // explicit user approval): teleporting is always the Teleporting
        // anim state. Real frame 0 is icon 1 (not -1), so no invisible-
        // frame substitution is exercised at this specific phase -- see
        // GetAnimIcon()'s own comment for that behavior.
        onTeleporter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        check(onTeleporter.GetAnimState() == GEBlupiController::AnimState::Teleporting,
              "teleporting is the Teleporting anim state");
        check(onTeleporter.GetAnimIcon() == 1, "Teleporting anim icon starts at the real first frame (icon 1)");

        // Fully frozen during transit: turning/moving input has no effect
        // at all (real m_blupiFocus=false blocks essentially every other
        // per-frame input block).
        const float xBeforeFrozenStep = onTeleporter.GetX();
        const float yawBeforeFrozenStep = onTeleporter.GetYaw();
        onTeleporter.Step(synthetic, 1.0f, 1.0f, true, false, false, dt);
        check(onTeleporter.GetX() == xBeforeFrozenStep && onTeleporter.GetYaw() == yawBeforeFrozenStep,
              "Blupi is fully frozen (no movement or turning) while teleporting");

        // Auto-completion after kTeleportDuration seconds.
        const int stepsToTeleport = static_cast<int>(GEBlupiController::kTeleportDuration / dt) + 5;
        for (int i = 0; i < stepsToTeleport && onTeleporter.IsTeleporting(); ++i)
        {
            onTeleporter.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        }
        check(!onTeleporter.IsTeleporting(), "teleport transit auto-completes after kTeleportDuration seconds");

        // Real gate: grounded, not ballooned, not squashed.
        GEBlupiController airborneTeleport;
        airborneTeleport.SetPosition(0.0f, 20.0f, 0.0f);
        check(!airborneTeleport.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while airborne (real !m_blupiAir gate)");

        GEBlupiController ballooned2;
        ballooned2.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        ballooned2.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        ballooned2.TriggerBalloon();
        check(!ballooned2.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while ballooned (real !m_blupiBalloon gate)");

        GEBlupiController crushed2;
        crushed2.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        crushed2.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        crushed2.TriggerCrush();
        check(!crushed2.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while squashed (real !m_blupiEcrase gate)");

        // Real gate also excludes every vehicle mount (Decor.cpp:5593-5594:
        // !m_blupiHelico/Over/Jeep/Tank/Skate) -- added 2026-07-16, this
        // engine's own gate previously missed this clause entirely.
        GEBlupiController mountedTeleport;
        mountedTeleport.SetPosition(static_cast<float>(kTeleX) - 50.0f, 1.0f, static_cast<float>(kTeleZ) - 50.0f);
        mountedTeleport.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        mountedTeleport.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
        check(!mountedTeleport.TriggerTeleport(BlockTypes::Teleport1),
              "TriggerTeleport() is a no-op while in a vehicle (real !m_blupiJeep/Tank/etc. gate)");

        // Death-lock + life-loss Voyage (death-VFX follow-up, verified
        // directly against Decor.cpp:6374-6392's shared per-cause duration
        // dispatch) -- mirrors TriggerTeleport()'s own freeze-timer shape
        // above, but chains into a SECOND fixed-duration frozen sub-state
        // (the life-loss-Voyage window) instead of auto-resuming directly.
        {
            GEBlupiController deathLocked;
            deathLocked.SetPosition(0.0f, 1.0f, 0.0f);
            check(deathLocked.TriggerDeathLock(GEBlupiController::DeathCause::Clear2, true),
                  "TriggerDeathLock() returns true when not already locked");
            check(deathLocked.IsDeathLocked(), "IsDeathLocked() is true immediately after TriggerDeathLock()");
            check(!deathLocked.IsDeathHidden(), "IsDeathHidden() is false during the lock itself (not yet Hide)");
            check(!deathLocked.TriggerDeathLock(GEBlupiController::DeathCause::Clear1, false),
                  "TriggerDeathLock() is a no-op (returns false) while already locked");

            // Fully frozen: same shape as the teleport freeze test above.
            // Also the first Step() call since TriggerDeathLock(), so this
            // is where GetAnimState() first reflects DeathLocked (set
            // inside UpdateAnim(), only called from within Step()).
            const float xBeforeFrozen = deathLocked.GetX();
            const float yawBeforeFrozen = deathLocked.GetYaw();
            deathLocked.Step(synthetic, 1.0f, 1.0f, true, false, false, dt);
            check(deathLocked.GetX() == xBeforeFrozen && deathLocked.GetYaw() == yawBeforeFrozen,
                  "Blupi is fully frozen (no movement or turning) while death-locked");
            check(deathLocked.GetAnimState() == GEBlupiController::AnimState::DeathLocked,
                  "the death lock is the DeathLocked anim state");
            // Real Clear2 has only a single, real "invisible" frame (table_blupi's own -1
            // sentinel) -- matches IsDeathHidden() never applying to Clear2 itself (no visible
            // Blupi to animate). GetAnimIcon() substitutes icon 0, same convention as Teleporting.
            check(deathLocked.GetAnimIcon() == 0,
                  "Clear2's DeathLocked anim icon substitutes icon 0 for its real invisible-only frame");
            // Real BlupiSearchIcon() channel rule (mobile-eggbert-reference/08-animations.md §2):
            // Clear2 is one of the 4 real element.png death causes, even though its own icon here
            // is the substituted "0" fallback, not a genuine Clear2 frame.
            check(deathLocked.AnimIconUsesElementSheet(),
                  "Clear2's DeathLocked anim icon is on element.png, not blupi.png");

            // Real per-cause hurt-sprite frames (plan.md `067`, added 2026-07-16) -- Clear1 has a
            // real distinct first frame (icon 40), unlike Clear2's invisible-only case above.
            GEBlupiController clear1Locked;
            clear1Locked.SetPosition(0.0f, 1.0f, 0.0f);
            clear1Locked.TriggerDeathLock(GEBlupiController::DeathCause::Clear1, true);
            clear1Locked.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            check(clear1Locked.GetAnimIcon() == 40,
                  "Clear1's DeathLocked anim icon starts at the real first frame (icon 40)");
            check(clear1Locked.AnimIconUsesElementSheet(),
                  "Clear1's DeathLocked anim icon is on element.png too (real BlupiSearchIcon() rule)");

            // Real Clear3/Glu causes (plan.md `067`) -- the other 2 of the 4 real element.png
            // death causes this engine models (Electro, the 5th, has no modeled mechanic yet).
            GEBlupiController clear3Locked;
            clear3Locked.SetPosition(0.0f, 1.0f, 0.0f);
            clear3Locked.TriggerDeathLock(GEBlupiController::DeathCause::Clear3, true);
            clear3Locked.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            check(clear3Locked.GetAnimIcon() == 40,
                  "Clear3's DeathLocked anim icon starts at the real first frame (icon 40)");
            check(clear3Locked.AnimIconUsesElementSheet(),
                  "Clear3's DeathLocked anim icon is on element.png too");

            GEBlupiController gluLocked;
            gluLocked.SetPosition(0.0f, 1.0f, 0.0f);
            gluLocked.TriggerDeathLock(GEBlupiController::DeathCause::Glu, true);
            gluLocked.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            check(gluLocked.GetAnimIcon() == 168,
                  "Glu's DeathLocked anim icon starts at the real first frame (icon 168)");
            check(gluLocked.AnimIconUsesElementSheet(),
                  "Glu's DeathLocked anim icon is on element.png too");

            // Real Clear2 lock duration = 100 ticks = 5.0s (Decor.cpp:6374-6392) --
            // advance to just under it (still locked), matching
            // kDeathLockTicks[Clear2]/20.0f exactly.
            constexpr float kClear2LockSeconds = 100.0f / 20.0f;
            const int ticksJustUnder = static_cast<int>(kClear2LockSeconds / dt) - 3;
            for (int i = 0; i < ticksJustUnder; ++i)
            {
                deathLocked.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(deathLocked.IsDeathLocked(), "still locked just under the real Clear2 duration (100 ticks=5.0s)");

            // Cross the threshold -- transitions into the life-loss-Voyage
            // window (still frozen, now ALSO Hide/invisible), and
            // ConsumeDeathLockResolved() fires exactly once.
            bool resolvedShouldRespawn = false;
            bool sawResolved = false;
            for (int i = 0; i < 10 && !sawResolved; ++i)
            {
                deathLocked.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
                if (deathLocked.ConsumeDeathLockResolved(resolvedShouldRespawn))
                {
                    sawResolved = true;
                }
            }
            check(sawResolved, "ConsumeDeathLockResolved() fires exactly once when the lock elapses");
            check(resolvedShouldRespawn, "ConsumeDeathLockResolved() echoes the real shouldRespawn=true passed to TriggerDeathLock()");
            check(!deathLocked.IsDeathLocked() && deathLocked.IsDeathHidden(),
                  "the lock ends and the life-loss-Voyage window (Hide) begins on the same transition");
            check(!deathLocked.ConsumeDeathLockResolved(resolvedShouldRespawn),
                  "ConsumeDeathLockResolved() does not fire again until the NEXT lock resolves");
            check(deathLocked.GetAnimState() == GEBlupiController::AnimState::DeathLocked,
                  "the life-loss-Voyage window is still the DeathLocked anim state");

            // Real fixed 40-tick(2.0s) life-loss Voyage auto-completes,
            // returning full control (no longer locked or hidden).
            constexpr float kLifeLossSeconds = 2.0f;
            const int stepsToLifeLoss = static_cast<int>(kLifeLossSeconds / dt) + 5;
            for (int i = 0; i < stepsToLifeLoss && deathLocked.IsDeathHidden(); ++i)
            {
                deathLocked.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(!deathLocked.IsDeathLocked() && !deathLocked.IsDeathHidden(),
                  "the life-loss-Voyage window auto-completes after kLifeLossVoyageDuration seconds");
            // moveInput moves along -cos(yaw) on Z / sin(yaw) on X -- at
            // the default yaw=0, that's pure Z movement (X stays put),
            // same axis convention used by Ghost mode/suspended movement
            // above.
            const float zBeforeUnlocked = deathLocked.GetZ();
            deathLocked.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            check(deathLocked.GetZ() != zBeforeUnlocked, "Blupi is fully controllable again once the death lock fully resolves");

            // Real per-cause durations (Decor.cpp:6374-6392): Clear1=70,
            // Clear3=70, Clear4=110, Glu=100, Drown=90 real ticks -- spot-
            // check 2 more (Clear4's real 110, the longest; Drown's real
            // 90) to confirm the lookup table itself, not just Clear2.
            GEBlupiController clear4Lock;
            clear4Lock.SetPosition(0.0f, 1.0f, 0.0f);
            clear4Lock.TriggerDeathLock(GEBlupiController::DeathCause::Clear4, false);
            constexpr float kClear4LockSeconds = 110.0f / 20.0f;
            for (int i = 0; i < static_cast<int>(kClear4LockSeconds / dt) - 3; ++i)
            {
                clear4Lock.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(clear4Lock.IsDeathLocked(), "still locked just under the real Clear4 duration (110 ticks=5.5s)");
            check(!clear4Lock.AnimIconUsesElementSheet(),
                  "Clear4's DeathLocked anim icon stays on blupi.png (only Clear1/2/3/Glu use element.png)");
            for (int i = 0; i < 10; ++i)
            {
                clear4Lock.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(!clear4Lock.IsDeathLocked(), "Clear4's real 110-tick duration elapses (distinct from Clear2's 100)");
            bool clear4ShouldRespawn = true;
            check(clear4Lock.ConsumeDeathLockResolved(clear4ShouldRespawn),
                  "ConsumeDeathLockResolved() fires for Clear4's own lock too");
            check(!clear4ShouldRespawn, "ConsumeDeathLockResolved() echoes the real shouldRespawn=false passed to TriggerDeathLock()");

            GEBlupiController drownLock;
            drownLock.SetPosition(0.0f, 1.0f, 0.0f);
            drownLock.TriggerDeathLock(GEBlupiController::DeathCause::Drown, true);
            constexpr float kDrownLockSeconds = 90.0f / 20.0f;
            for (int i = 0; i < static_cast<int>(kDrownLockSeconds / dt) - 3; ++i)
            {
                drownLock.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(drownLock.IsDeathLocked(), "still locked just under the real Drown duration (90 ticks=4.5s)");
            check(!drownLock.AnimIconUsesElementSheet(),
                  "Drown's DeathLocked anim icon also stays on blupi.png");
        }

        // Sucette/Drink/Charge real 2-stage pickup delay (plan.md `173`, verified directly
        // against Decor.cpp:6025-6087) -- a third application of the same freeze-timer template
        // as TriggerTeleport()/the death lock above.
        {
            GEBlupiController pickupFrozen;
            pickupFrozen.SetPosition(0.0f, 1.0f, 0.0f);
            check(pickupFrozen.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Sucette),
                  "TriggerPickupFreeze() returns true when not already frozen/locked");
            check(pickupFrozen.IsPickupFrozen(), "IsPickupFrozen() is true immediately after TriggerPickupFreeze()");
            check(!pickupFrozen.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Drink),
                  "TriggerPickupFreeze() is a no-op (returns false) while already frozen");

            // Fully frozen, same shape as the death lock/teleport tests above. Also the first
            // Step() call, so this is where GetAnimState() first reflects PickupBusy.
            const float xBeforeFrozen = pickupFrozen.GetX();
            const float yawBeforeFrozen = pickupFrozen.GetYaw();
            pickupFrozen.Step(synthetic, 1.0f, 1.0f, true, false, false, dt);
            check(pickupFrozen.GetX() == xBeforeFrozen && pickupFrozen.GetYaw() == yawBeforeFrozen,
                  "Blupi is fully frozen (no movement or turning) while pickup-busy");
            check(pickupFrozen.GetAnimState() == GEBlupiController::AnimState::PickupBusy,
                  "the pickup delay is the PickupBusy anim state");
            // Real per-kind pickup-freeze busy-animation frames (plan.md `173`, added 2026-07-16).
            check(pickupFrozen.GetAnimIcon() == 234,
                  "Sucette's PickupBusy anim icon starts at the real first frame (icon 234)");

            // Real Sucette duration = 32 ticks = 1.6s -- advance to just under it (still frozen).
            constexpr float kSucetteSeconds = 32.0f / 20.0f;
            for (int i = 0; i < static_cast<int>(kSucetteSeconds / dt) - 3; ++i)
            {
                pickupFrozen.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(pickupFrozen.IsPickupFrozen(), "still frozen just under the real Sucette duration (32 ticks=1.6s)");

            GEBlupiController::PickupFreezeKind resolvedKind{};
            bool sawResolved = false;
            for (int i = 0; i < 10 && !sawResolved; ++i)
            {
                pickupFrozen.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
                if (pickupFrozen.ConsumePickupFreezeResolved(resolvedKind))
                {
                    sawResolved = true;
                }
            }
            check(sawResolved, "ConsumePickupFreezeResolved() fires exactly once when the freeze elapses");
            check(resolvedKind == GEBlupiController::PickupFreezeKind::Sucette,
                  "ConsumePickupFreezeResolved() echoes the real kind passed to TriggerPickupFreeze()");
            check(!pickupFrozen.IsPickupFrozen(), "the freeze ends on the same transition");
            check(!pickupFrozen.ConsumePickupFreezeResolved(resolvedKind),
                  "ConsumePickupFreezeResolved() does not fire again until the NEXT freeze resolves");

            // Real controllability restored (movement along -cos(yaw)/Z at the default yaw=0,
            // same axis convention as the death-lock test above).
            const float zBeforeUnfrozen = pickupFrozen.GetZ();
            pickupFrozen.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            check(pickupFrozen.GetZ() != zBeforeUnfrozen, "Blupi is fully controllable again once the pickup freeze resolves");

            // Spot-check Drink's distinct 36-tick duration and Charge's 64-tick duration.
            GEBlupiController drinkFreeze;
            drinkFreeze.SetPosition(0.0f, 1.0f, 0.0f);
            drinkFreeze.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Drink);
            constexpr float kDrinkSeconds = 36.0f / 20.0f;
            for (int i = 0; i < static_cast<int>(kDrinkSeconds / dt) - 3; ++i)
            {
                drinkFreeze.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(drinkFreeze.IsPickupFrozen(), "still frozen just under the real Drink duration (36 ticks=1.8s)");
            for (int i = 0; i < 10; ++i)
            {
                drinkFreeze.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(!drinkFreeze.IsPickupFrozen(), "Drink's real 36-tick duration elapses (distinct from Sucette's 32)");

            GEBlupiController chargeFreeze;
            chargeFreeze.SetPosition(0.0f, 1.0f, 0.0f);
            chargeFreeze.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Charge);
            constexpr float kChargeSeconds = 64.0f / 20.0f;
            for (int i = 0; i < static_cast<int>(kChargeSeconds / dt) - 3; ++i)
            {
                chargeFreeze.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(chargeFreeze.IsPickupFrozen(), "still frozen just under the real Charge duration (64 ticks=3.2s)");
            for (int i = 0; i < 10; ++i)
            {
                chargeFreeze.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            }
            check(!chargeFreeze.IsPickupFrozen(), "Charge's real 64-tick duration elapses (the longest of the 3)");

            // A death lock always cancels a pending pickup freeze (real BlupiDead() unconditionally
            // overwrites whatever action was active).
            GEBlupiController canceledByDeath;
            canceledByDeath.SetPosition(0.0f, 1.0f, 0.0f);
            canceledByDeath.TriggerPickupFreeze(GEBlupiController::PickupFreezeKind::Charge);
            check(canceledByDeath.IsPickupFrozen(), "pickup freeze is active before a death lock interrupts it");
            canceledByDeath.TriggerDeathLock(GEBlupiController::DeathCause::Clear1, true);
            check(!canceledByDeath.IsPickupFrozen(), "TriggerDeathLock() cancels a pending pickup freeze outright");
            check(canceledByDeath.IsDeathLocked(), "the death lock itself starts normally despite the cancellation");

            // Real BlupiDead() (Decor.cpp:6547-6614) ALSO unconditionally clears vehicle mount/
            // Balloon/Ecrase/every secret power/Invert/Nage/Surf/Suspend/Ghost -- found 2026-07-16.
            // Death previously left every one of these completely untouched in this engine.
            GEBlupiController fullyLoaded;
            fullyLoaded.SetPosition(0.0f, 1.0f, 0.0f);
            fullyLoaded.Step(synthetic, 0.0f, 0.0f, false, false, false, dt); // settle grounded
            fullyLoaded.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
            fullyLoaded.TriggerShield();
            fullyLoaded.TriggerInvert();
            check(fullyLoaded.IsInVehicle() && fullyLoaded.GetSecretPower() == GEBlupiController::SecretPower::Shield &&
                      fullyLoaded.IsInverted(),
                  "sanity: vehicle/Shield/Invert are all active before the death lock clears them");
            fullyLoaded.TriggerDeathLock(GEBlupiController::DeathCause::Clear1, true);
            check(!fullyLoaded.IsInVehicle(), "TriggerDeathLock() clears the vehicle mount (real BlupiDead())");
            check(fullyLoaded.GetSecretPower() == GEBlupiController::SecretPower::None,
                  "TriggerDeathLock() clears the active secret power (real BlupiDead())");
            check(!fullyLoaded.IsInverted(), "TriggerDeathLock() clears Invert too (real BlupiDead())");

            GEBlupiController ballooned3;
            ballooned3.SetPosition(0.0f, 1.0f, 0.0f);
            ballooned3.TriggerBalloon();
            check(ballooned3.IsBallooned(), "sanity: ballooned before the death lock clears it");
            ballooned3.TriggerDeathLock(GEBlupiController::DeathCause::Clear2, true);
            check(!ballooned3.IsBallooned(), "TriggerDeathLock() clears Balloon too (real BlupiDead())");

            GEBlupiController crushed3;
            crushed3.SetPosition(0.0f, 1.0f, 0.0f);
            crushed3.TriggerCrush();
            check(crushed3.IsEcrased(), "sanity: squashed before the death lock clears it");
            crushed3.TriggerDeathLock(GEBlupiController::DeathCause::Drown, true);
            check(!crushed3.IsEcrased(), "TriggerDeathLock() clears Ecrase too (real BlupiDead())");
        }

        // Fan hazard collision (plan.md E3D-MIG-149) -- same non-solid
        // architecture as the teleporter above: a real Ground floor (Y=0)
        // with a FanLeft head FLOATING one cell above Blupi's standing
        // height (Y=2), matching the real placement convention already
        // established for the teleporter (GEWorldRuntime::TryConsumeFan()'s
        // own comment). Fan head icons are always non-solid for collision
        // (GroundHeightAt's own BlockTypes::isFan() skip), so Blupi must
        // land on the REAL floor beneath it (Y=1), not be blocked by or
        // land on top of the fan.
        constexpr std::uint16_t kFanX = 44, kFanZ = 44;
        synthetic.setBlock(kFanX, 0, kFanZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kFanX, 2, kFanZ, Worlds::Block::make(BlockTypes::FanLeft));

        GEBlupiController underFan;
        underFan.SetPosition(static_cast<float>(kFanX) - 50.0f, 1.0f, static_cast<float>(kFanZ) - 50.0f);
        underFan.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        std::cout << "Blupi Y beneath the floating fan: " << underFan.GetY() << std::endl;
        check(underFan.IsOnGround() && std::fabs(underFan.GetY() - 1.0f) < 0.01f,
              "Blupi stands on the real floor beneath the floating fan, not on/blocked by it");
        check(underFan.GetBlockTypeAbove(synthetic) == BlockTypes::FanLeft,
              "GetBlockTypeAbove() identifies the fan head one cell above Blupi (E3D-MIG-149 detection)");

        // Real 10-slot safe-position FIFO respawn (plan.md E3D-MIG-067) --
        // a 13-wide flat strip so Blupi can occupy 13 distinct safe grid
        // positions in a row (more than the 10-slot capacity), to prove
        // both that the FIFO tracks a lagged "safe" position and that it
        // shifts out its oldest entry once full.
        constexpr std::uint16_t kSafeStripZ = 60;
        for (std::uint16_t x = 60; x <= 72; ++x)
        {
            synthetic.setBlock(x, 0, kSafeStripZ, Worlds::Block::make(BlockTypes::Ground));
        }

        GEBlupiController defaultValid;
        check(defaultValid.GetValidX() == 0.0f && defaultValid.GetValidY() == 1.0f && defaultValid.GetValidZ() == 0.0f,
              "GetValidX/Y/Z() default to the spawn point before any safe frame is ever recorded");

        GEBlupiController airborneSafe;
        airborneSafe.SetPosition(0.0f, 20.0f, 0.0f);
        airborneSafe.UpdateSafePosition(/*externallySafe=*/true);
        check(airborneSafe.GetValidX() == 0.0f && airborneSafe.GetValidZ() == 0.0f,
              "UpdateSafePosition() is a no-op while airborne (still the spawn-point default)");

        GEBlupiController unsafeCaller;
        unsafeCaller.SetPosition(60.0f - 50.0f, 1.0f, static_cast<float>(kSafeStripZ) - 50.0f);
        unsafeCaller.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
        unsafeCaller.UpdateSafePosition(/*externallySafe=*/false);
        check(unsafeCaller.GetValidX() == 0.0f && unsafeCaller.GetValidZ() == 0.0f,
              "UpdateSafePosition() is a no-op when the caller reports externallySafe=false");

        // Walk across 13 distinct grid positions (one per frame), each a
        // real safe frame -- the FIFO holds only 10, so the tracked valid
        // position should lag behind Blupi's current position by roughly
        // that buffer, never equal to (or ahead of) wherever he currently
        // is.
        GEBlupiController safeWalker;
        for (int i = 0; i < 13; ++i)
        {
            safeWalker.SetPosition(static_cast<float>(60 + i) - 50.0f, 1.0f,
                                    static_cast<float>(kSafeStripZ) - 50.0f);
            safeWalker.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            safeWalker.UpdateSafePosition(/*externallySafe=*/true);
        }
        std::cout << "Safe walker at X=" << safeWalker.GetX() << ", valid respawn X=" << safeWalker.GetValidX()
                  << std::endl;
        check(safeWalker.GetValidX() < safeWalker.GetX(),
              "the tracked valid respawn position lags behind Blupi's current position (the real 'buffer'), "
              "not equal to or ahead of it");
        check(safeWalker.GetX() - safeWalker.GetValidX() >= 9.0f,
              "the lag is roughly the FIFO's 10-slot capacity, proving the oldest entry is what's used, "
              "not just the immediately-prior one");

        // Water Surf/Nage/drowning (plan.md E3D-MIG-148) -- a shallow
        // 1-layer pool (Ground at y=0, Water1 at y=1) and a deep 2-layer
        // pool (Ground at y=0, Water1 at y=1 AND y=2) elsewhere in the same
        // synthetic world. Water is now non-solid for collision (see
        // GroundHeightAt's own comment), so Blupi sinks through any depth
        // of water to the real floor beneath it -- the tile(s) at/above his
        // resting position then determine Surf vs Nage, matching real
        // mobile-eggbert's per-tile-independent collision well enough for
        // this purpose, the same technique already used for the teleporter
        // pillar and fan head.
        constexpr std::uint16_t kShallowX = 80, kShallowZ = 80;
        synthetic.setBlock(kShallowX, 0, kShallowZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kShallowX, 1, kShallowZ, Worlds::Block::make(BlockTypes::Water1));

        constexpr std::uint16_t kDeepX = 85, kDeepZ = 80;
        synthetic.setBlock(kDeepX, 0, kDeepZ, Worlds::Block::make(BlockTypes::Ground));
        synthetic.setBlock(kDeepX, 1, kDeepZ, Worlds::Block::make(BlockTypes::Water1));
        synthetic.setBlock(kDeepX, 2, kDeepZ, Worlds::Block::make(BlockTypes::Water1));

        const auto stepWithWaterDetection = [](GEBlupiController& c, const Worlds::World& w, float stepDt)
        {
            const auto blockAt = c.GetBlockTypeAt(w);
            const auto blockAbove = c.GetBlockTypeAbove(w);
            const bool surf = BlockTypes::isWater(blockAt) && !BlockTypes::isWater(blockAbove);
            const bool nage = BlockTypes::isWater(blockAt) && BlockTypes::isWater(blockAbove);
            c.Step(w, 0.0f, 0.0f, false, false, false, stepDt, false, surf, nage);
        };

        GEBlupiController inShallow;
        inShallow.SetPosition(static_cast<float>(kShallowX) - 50.0f, 20.0f, static_cast<float>(kShallowZ) - 50.0f);
        for (int i = 0; i < 200; ++i) stepWithWaterDetection(inShallow, synthetic, dt);
        check(inShallow.IsOnGround() && std::fabs(inShallow.GetY() - 1.0f) < 0.01f,
              "Blupi sinks through a shallow 1-layer water pool and rests on the real floor beneath it");
        check(inShallow.IsSurf() && !inShallow.IsNage(),
              "standing in a shallow 1-layer pool is Surf (dry above), not Nage");
        check(inShallow.GetWaterGaugeLevel() == GEBlupiController::kWaterGaugeMax,
              "the breath gauge stays full while merely Surf, not Nage");

        GEBlupiController inDeep;
        inDeep.SetPosition(static_cast<float>(kDeepX) - 50.0f, 20.0f, static_cast<float>(kDeepZ) - 50.0f);
        for (int i = 0; i < 200; ++i) stepWithWaterDetection(inDeep, synthetic, dt);
        check(inDeep.IsOnGround() && std::fabs(inDeep.GetY() - 1.0f) < 0.01f,
              "Blupi sinks through a deep 2-layer water pool and rests on the real floor beneath it");
        check(inDeep.IsNage() && !inDeep.IsSurf(),
              "standing beneath 2 layers of water is Nage (water above too), not merely Surf");

        // Gauge countdown + drowning -- run enough simulated time to
        // exhaust the full 100-level gauge (the real ~25s), confirming
        // JustDrowned() fires exactly at the moment it reaches 0.
        GEBlupiController drowning;
        drowning.SetPosition(static_cast<float>(kDeepX) - 50.0f, 20.0f, static_cast<float>(kDeepZ) - 50.0f);
        constexpr float kDrownDt = 0.05f;
        bool drownedOnce = false;
        int drownedFrame = -1;
        for (int i = 0; i < 700 && !drownedOnce; ++i) // 700 * 0.05s = 35s, comfortably past ~25s + fall time
        {
            stepWithWaterDetection(drowning, synthetic, kDrownDt);
            if (drowning.JustDrowned())
            {
                drownedOnce = true;
                drownedFrame = i;
            }
        }
        std::cout << "Drowned at simulated t=" << (drownedFrame * kDrownDt) << "s" << std::endl;
        check(drownedOnce, "JustDrowned() fires after prolonged Nage submersion (the real ~25s breath gauge)");
        check(drowning.GetWaterGaugeLevel() == 0, "the gauge is exactly 0 at the moment JustDrowned() fires");

        // Real Shield/Hide immunity (Decor.cpp:4615-4620, found 2026-07-16): the gauge does not
        // deplete AT ALL while either is active. 15s comfortably exceeds half the real ~25s
        // breath gauge (so an unprotected Blupi would have lost well over half by this point) while
        // staying safely inside Shield's own ~25s real duration (avoiding the coincidental overlap
        // where Shield expiring mid-test would let the gauge resume depleting).
        GEBlupiController shieldedSwimmer;
        shieldedSwimmer.SetPosition(static_cast<float>(kDeepX) - 50.0f, 20.0f, static_cast<float>(kDeepZ) - 50.0f);
        shieldedSwimmer.TriggerShield();
        for (int i = 0; i < 300; ++i) // 300 * 0.05s = 15s
        {
            stepWithWaterDetection(shieldedSwimmer, synthetic, kDrownDt);
        }
        check(!shieldedSwimmer.JustDrowned() && shieldedSwimmer.GetWaterGaugeLevel() == GEBlupiController::kWaterGaugeMax,
              "the water gauge never depletes while Shield is active (real !m_blupiShield gate)");

        // Resurfacing resets the gauge -- a few seconds of genuine Nage
        // (not to exhaustion), then Surf, confirms the gauge snaps back to
        // full instead of resuming from where it left off (matches the
        // real "gauge hidden on resurfacing" behavior, not a persisted
        // shared resource across dives).
        GEBlupiController resurfacer;
        resurfacer.SetPosition(static_cast<float>(kDeepX) - 50.0f, 1.0f, static_cast<float>(kDeepZ) - 50.0f);
        for (int i = 0; i < 60; ++i)
        {
            resurfacer.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, false, false, true);
        }
        check(resurfacer.GetWaterGaugeLevel() < GEBlupiController::kWaterGaugeMax,
              "the gauge has ticked down after a few seconds of genuine Nage");
        resurfacer.Step(synthetic, 0.0f, 0.0f, false, false, false, dt, false, true, false);
        check(resurfacer.GetWaterGaugeLevel() == GEBlupiController::kWaterGaugeMax,
              "the gauge resets to full the instant Nage ends (Surf), matching the real 'gauge hidden' behavior");

        // Secret powers (plan.md E3D-MIG-170) -- trigger gates, the shared
        // gauge's real per-power decrement rate, expiry, and the real
        // hazard-immunity gate (IsInvincible() == Shield || Hide).
        {
            GEBlupiController shielded;
            check(shielded.TriggerShield(), "TriggerShield() succeeds from None");
            check(shielded.IsShielded() && shielded.IsInvincible(),
                  "Shield grants IsShielded()/IsInvincible()");
            check(!shielded.TriggerPower(), "TriggerPower() fails while Shielded (real gate: != Shield)");
            check(!shielded.TriggerCloud(), "TriggerCloud() fails while Shielded (real gate: == None)");
            check(!shielded.TriggerHide(), "TriggerHide() fails while Shielded (real gate: != Shield/Cloud)");

            GEBlupiController hidden;
            check(hidden.TriggerHide(), "TriggerHide() succeeds from None");
            check(hidden.IsHidden() && hidden.IsInvincible(), "Hide grants IsHidden()/IsInvincible() too");

            GEBlupiController powered;
            check(powered.TriggerPower(), "TriggerPower() succeeds from None");
            check(!powered.IsInvincible(), "Power alone does NOT grant IsInvincible() (only Shield/Hide do)");
            check(powered.TriggerHide(), "TriggerHide() succeeds even while Power is active (real gate ignores Power)");

            // Real per-power decrement rates (Decor.cpp ~5071-5137): Shield
            // every ScaleTime(5)=0.25s/level, Power every ScaleTime(3)=
            // 0.15s/level, Cloud/Hide every ScaleTime(4)=0.2s/level.
            GEBlupiController shieldTiming;
            shieldTiming.TriggerShield();
            for (int i = 0; i < 30; ++i) // 30 * 0.25s = 7.5s = 30 levels
            {
                shieldTiming.Step(synthetic, 0.0f, 0.0f, false, false, false, GEBlupiController::kShieldTickSeconds);
            }
            check(shieldTiming.GetSecretPowerLevel() == GEBlupiController::kSecretPowerMax - 30,
                  "Shield's gauge ticks down at exactly the real 0.25s/level rate");

            GEBlupiController powerTiming;
            powerTiming.TriggerPower();
            for (int i = 0; i < 30; ++i)
            {
                powerTiming.Step(synthetic, 0.0f, 0.0f, false, false, false, GEBlupiController::kPowerTickSeconds);
            }
            check(powerTiming.GetSecretPowerLevel() == GEBlupiController::kSecretPowerMax - 30,
                  "Power's gauge ticks down at exactly the real 0.15s/level rate");

            // Expiry: the real ~25s Shield duration (100 levels * 0.25s).
            GEBlupiController expiring;
            expiring.TriggerShield();
            for (int i = 0; i < 500 && expiring.IsShielded(); ++i) // 500 * 0.05 = 25s
            {
                expiring.Step(synthetic, 0.0f, 0.0f, false, false, false, 0.05f);
            }
            check(expiring.GetSecretPower() == GEBlupiController::SecretPower::None,
                  "Shield expires back to None after its real ~25s duration");
            check(!expiring.IsInvincible(), "IsInvincible() is false again once Shield expires");

            // Real warning threshold: Shield warns at exactly level 10.
            GEBlupiController warning;
            warning.TriggerShield();
            bool sawWarning = false;
            for (int i = 0; i < 100 && !sawWarning; ++i)
            {
                warning.Step(synthetic, 0.0f, 0.0f, false, false, false, GEBlupiController::kShieldTickSeconds);
                if (warning.JustCrossedSecretPowerWarning())
                {
                    sawWarning = true;
                }
            }
            check(sawWarning, "JustCrossedSecretPowerWarning() fires once during Shield's real countdown");
            check(warning.GetSecretPowerLevel() == GEBlupiController::kShieldWarnLevel,
                  "the warning fires at exactly the real level-10 threshold, not some other level");
        }

        // Invert/Mirror (plan.md PICKUP-011) -- independent of the 4 powers
        // above (its own gauge, real gate is only !Hide), the real
        // ScaleTime(3)=0.15s/level tick rate (same as Power), no warning
        // stage, and the actual real "negate horizontal input speed" effect.
        {
            GEBlupiController inverted;
            check(inverted.TriggerInvert(), "TriggerInvert() succeeds from no state at all");
            check(inverted.IsInverted(), "IsInverted() reflects the new state");
            check(!inverted.TriggerInvert(),
                  "TriggerInvert() fails while already Invert (real: blocked while already active)");

            GEBlupiController shieldedThenInvert;
            shieldedThenInvert.TriggerShield();
            check(shieldedThenInvert.TriggerInvert(),
                  "TriggerInvert() succeeds while Shield is active (real gate ignores Shield/Power/Cloud)");

            GEBlupiController hiddenThenInvert;
            hiddenThenInvert.TriggerHide();
            check(!hiddenThenInvert.TriggerInvert(),
                  "TriggerInvert() fails while Hide is active (real gate: != Hide)");

            // Real tick rate: ScaleTime(3)=0.15s/level, same as Power.
            GEBlupiController invertTiming;
            invertTiming.TriggerInvert();
            for (int i = 0; i < 30; ++i)
            {
                invertTiming.Step(synthetic, 0.0f, 0.0f, false, false, false, GEBlupiController::kInvertTickSeconds);
            }
            check(invertTiming.GetInvertLevel() == GEBlupiController::kInvertMax - 30,
                  "Invert's gauge ticks down at exactly the real 0.15s/level rate");

            // Expiry: real ~15s duration (100 levels * 0.15s), no warning stage.
            GEBlupiController invertExpiring;
            invertExpiring.TriggerInvert();
            bool sawInvertExpiry = false;
            for (int i = 0; i < 400 && invertExpiring.IsInverted(); ++i) // 400 * 0.05 = 20s > 15s real duration
            {
                invertExpiring.Step(synthetic, 0.0f, 0.0f, false, false, false, 0.05f);
                if (invertExpiring.JustExpiredInvert())
                {
                    sawInvertExpiry = true;
                }
            }
            check(!invertExpiring.IsInverted(), "Invert expires back to false after its real ~15s duration");
            check(sawInvertExpiry, "JustExpiredInvert() fires exactly once when the gauge naturally reaches 0");

            // Real effect: negates horizontal movement input (Decor::
            // SetSpeedX: "if (m_blupiInvert) speed = -speed") -- walking
            // "forward" while Inverted should move Blupi BACKWARD relative
            // to a non-inverted control at the same facing/input.
            GEBlupiController normalWalker;
            normalWalker.SetYaw(0.0f); // facing -Z
            for (int i = 0; i < 60; ++i)
            {
                normalWalker.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            }
            GEBlupiController invertedWalker;
            invertedWalker.TriggerInvert();
            invertedWalker.SetYaw(0.0f);
            for (int i = 0; i < 60; ++i)
            {
                invertedWalker.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            }
            check(invertedWalker.GetZ() > 0.0f && normalWalker.GetZ() < 0.0f,
                  "the same forward input moves Blupi in opposite Z directions with/without Invert active");
        }

        // Vehicle mounts (plan.md E3D-MIG-171) -- trigger gates, Cloud/Hide
        // cancellation on mount (real: "if Cloud or Hide was active it is
        // silently cancelled... Shield/Power are left untouched"), and the
        // real per-mode horizontal accel/decel ramp + Helicopter's free
        // vertical flight.
        {
            GEBlupiController jeep;
            check(jeep.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false),
                  "TriggerMount(Jeep) succeeds from no vehicle, not Nage/Surf");
            check(jeep.IsInVehicle() && jeep.GetVehicleMode() == GEBlupiController::VehicleMode::Jeep,
                  "IsInVehicle()/GetVehicleMode() reflect the new Jeep mount");
            check(!jeep.TriggerMount(GEBlupiController::VehicleMode::Tank, false, false),
                  "TriggerMount() fails while already riding another vehicle (real: blocked while riding ANY vehicle)");

            // Real per-vehicle hazard immunity (Decor.cpp:5504-5528, found 2026-07-16): Over/Jeep/
            // Tank protect against Spike/Drip/Saw; Helicopter/Skateboard do NOT.
            check(jeep.HasVehicleHazardImmunity(),
                  "HasVehicleHazardImmunity() is true while riding a Jeep");
            GEBlupiController tankRider;
            tankRider.TriggerMount(GEBlupiController::VehicleMode::Tank, false, false);
            check(tankRider.HasVehicleHazardImmunity(),
                  "HasVehicleHazardImmunity() is true while riding a Tank");
            GEBlupiController overRider;
            overRider.TriggerMount(GEBlupiController::VehicleMode::Overcraft, false, false);
            check(overRider.HasVehicleHazardImmunity(),
                  "HasVehicleHazardImmunity() is true while riding an Overcraft");
            GEBlupiController heliRider;
            heliRider.TriggerMount(GEBlupiController::VehicleMode::Helicopter, false, false);
            check(!heliRider.HasVehicleHazardImmunity(),
                  "HasVehicleHazardImmunity() is FALSE while riding a Helicopter (real: not one of the "
                  "3 immune modes)");
            GEBlupiController skateRider;
            skateRider.TriggerMount(GEBlupiController::VehicleMode::Skateboard, false, false);
            check(!skateRider.HasVehicleHazardImmunity(),
                  "HasVehicleHazardImmunity() is FALSE while riding a Skateboard");
            GEBlupiController noVehicle;
            check(!noVehicle.HasVehicleHazardImmunity(),
                  "HasVehicleHazardImmunity() is false with no vehicle mounted");

            GEBlupiController nageRider;
            check(!nageRider.TriggerMount(GEBlupiController::VehicleMode::Jeep, /*inNage=*/true, false),
                  "TriggerMount() fails while Nage (real: blocked while swimming/surfing)");
            GEBlupiController surfRider;
            check(!surfRider.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, /*inSurf=*/true),
                  "TriggerMount() fails while Surf too");

            // Real gate also excludes Balloon/Ecrase (Decor.cpp:5649/5669/5687, found 2026-07-16)
            // -- previously missing from this engine's TriggerMount() entirely.
            GEBlupiController balloonedMounter;
            balloonedMounter.TriggerBalloon();
            check(balloonedMounter.IsBallooned(), "sanity: ballooned before attempting to mount");
            check(!balloonedMounter.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false),
                  "TriggerMount() fails while ballooned (real !m_blupiBalloon gate)");
            GEBlupiController crushedMounter;
            crushedMounter.TriggerCrush();
            check(crushedMounter.IsEcrased(), "sanity: squashed before attempting to mount");
            check(!crushedMounter.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false),
                  "TriggerMount() fails while squashed (real !m_blupiEcrase gate)");

            GEBlupiController cloudMounter;
            cloudMounter.TriggerCloud();
            check(cloudMounter.GetSecretPower() == GEBlupiController::SecretPower::Cloud,
                  "sanity: Cloud is active before mounting");
            cloudMounter.TriggerMount(GEBlupiController::VehicleMode::Skateboard, false, false);
            check(cloudMounter.GetSecretPower() == GEBlupiController::SecretPower::None,
                  "mounting a vehicle silently cancels an active Cloud (real behavior)");

            GEBlupiController shieldMounter;
            shieldMounter.TriggerShield();
            shieldMounter.TriggerMount(GEBlupiController::VehicleMode::Skateboard, false, false);
            check(shieldMounter.GetSecretPower() == GEBlupiController::SecretPower::Shield,
                  "mounting a vehicle does NOT cancel Shield (real: 'none of them check Shield or Power')");

            GEBlupiController dismounter;
            dismounter.TriggerMount(GEBlupiController::VehicleMode::Tank, false, false);
            dismounter.TriggerDismount();
            check(!dismounter.IsInVehicle(), "TriggerDismount() clears the vehicle mode");
            dismounter.TriggerDismount(); // no-op when not riding -- just confirming it doesn't crash
            check(!dismounter.IsInVehicle(), "TriggerDismount() stays a no-op when called again while not riding");

            // Vehicle motor sound accessors (plan.md SOUND-007/008, found 2026-07-17):
            // HasVehicleMotor() -- real source gives Helicopter/Jeep/Tank/Overcraft their own
            // motor sound set, but NOT Skateboard (confirmed via direct source read).
            GEBlupiController noMotorVehicle;
            check(!noMotorVehicle.HasVehicleMotor(), "HasVehicleMotor() is false with no vehicle mounted");
            GEBlupiController jeepMotor;
            jeepMotor.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
            check(jeepMotor.HasVehicleMotor(), "HasVehicleMotor() is true while riding a Jeep");
            GEBlupiController tankMotor;
            tankMotor.TriggerMount(GEBlupiController::VehicleMode::Tank, false, false);
            check(tankMotor.HasVehicleMotor(), "HasVehicleMotor() is true while riding a Tank");
            GEBlupiController overMotor;
            overMotor.TriggerMount(GEBlupiController::VehicleMode::Overcraft, false, false);
            check(overMotor.HasVehicleMotor(), "HasVehicleMotor() is true while riding an Overcraft");
            GEBlupiController heliMotor;
            heliMotor.TriggerMount(GEBlupiController::VehicleMode::Helicopter, false, false);
            check(heliMotor.HasVehicleMotor(), "HasVehicleMotor() is true while riding a Helicopter");
            GEBlupiController skateMotor;
            skateMotor.TriggerMount(GEBlupiController::VehicleMode::Skateboard, false, false);
            check(!skateMotor.HasVehicleMotor(),
                  "HasVehicleMotor() is FALSE while riding a Skateboard (real: no motor sound set for it)");

            // IsVehicleMotorHigh() -- real per-mode "m_blupiMotorHigh" pitch-select flag: false
            // at rest, true once genuinely moving (Jeep: nonzero horizontal speed after ramping
            // up; Helicopter: nonzero vertical velocity while ascending).
            GEBlupiController jeepIdle;
            jeepIdle.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
            check(!jeepIdle.IsVehicleMotorHigh(), "IsVehicleMotorHigh() is false for a Jeep at rest");
            jeepIdle.SetYaw(0.0f);
            for (int i = 0; i < 5; ++i)
            {
                jeepIdle.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            }
            check(jeepIdle.IsVehicleMotorHigh(), "IsVehicleMotorHigh() is true once the Jeep has ramped up speed");

            GEBlupiController heliIdle;
            heliIdle.TriggerMount(GEBlupiController::VehicleMode::Helicopter, false, false);
            check(!heliIdle.IsVehicleMotorHigh(), "IsVehicleMotorHigh() is false for a Helicopter at rest");
            for (int i = 0; i < 5; ++i)
            {
                heliIdle.Step(synthetic, 0.0f, 0.0f, false, /*crouchHeld=*/false, /*lookUpHeld=*/true, dt);
            }
            check(heliIdle.IsVehicleMotorHigh(),
                  "IsVehicleMotorHigh() is true once the Helicopter is ascending (nonzero vertical velocity)");

            // Real per-mode accel/decel ramp: Jeep's horizontal speed
            // should climb from 0 toward its own max, not snap instantly.
            // Verified by comparing the PER-FRAME delta near the start of
            // the ramp (frame 1) against a later frame's delta (frame 10)
            // -- a genuine accel ramp means the later delta is bigger
            // (still speeding up); an instant snap-to-max-speed (like
            // Blupi's own normal walk) would make every frame's delta
            // identical from the very first one.
            GEBlupiController jeepRamp;
            jeepRamp.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
            jeepRamp.SetYaw(0.0f); // facing -Z
            float zBefore = jeepRamp.GetZ();
            jeepRamp.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            const float firstFrameDelta = std::fabs(jeepRamp.GetZ() - zBefore);
            for (int i = 0; i < 8; ++i)
            {
                jeepRamp.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            }
            zBefore = jeepRamp.GetZ();
            jeepRamp.Step(synthetic, 0.0f, 1.0f, false, false, false, dt);
            const float laterFrameDelta = std::fabs(jeepRamp.GetZ() - zBefore);
            std::cout << "Jeep per-frame delta: frame 1 = " << firstFrameDelta << ", frame 10 = "
                      << laterFrameDelta << std::endl;
            check(laterFrameDelta > firstFrameDelta * 1.5f,
                  "Jeep's per-frame movement grows over the first several frames (a real accel ramp, "
                  "not an instant snap to max speed)");

            // Coasting: after releasing input, a vehicle should still be
            // moving next frame (decelerating), not stop dead instantly
            // like Blupi's own normal walk does.
            const float zBeforeRelease = jeepRamp.GetZ();
            jeepRamp.Step(synthetic, 0.0f, 0.0f, false, false, false, dt);
            check(jeepRamp.GetZ() != zBeforeRelease,
                  "a vehicle keeps coasting for at least one frame after input stops (real: decelerates, "
                  "never stops instantly)");

            // Helicopter flight: holding lookUp (real "Up") should ramp
            // m_velocityY toward the real ascend target, climbing Y over
            // several frames without needing to be grounded first.
            GEBlupiController helicopter;
            helicopter.SetPosition(0.0f, 5.0f, 0.0f);
            helicopter.TriggerMount(GEBlupiController::VehicleMode::Helicopter, false, false);
            const float yStart = helicopter.GetY();
            for (int i = 0; i < 30; ++i)
            {
                helicopter.Step(synthetic, 0.0f, 0.0f, false, /*crouchHeld=*/false, /*lookUpHeld=*/true, dt);
            }
            check(helicopter.GetY() > yStart, "holding lookUp while flying a Helicopter climbs Y over time");

            GEBlupiController helicopterDescend;
            helicopterDescend.SetPosition(0.0f, 12.0f, 0.0f);
            helicopterDescend.TriggerMount(GEBlupiController::VehicleMode::Helicopter, false, false);
            const float yStartDescend = helicopterDescend.GetY();
            for (int i = 0; i < 30; ++i)
            {
                helicopterDescend.Step(synthetic, 0.0f, 0.0f, false, /*crouchHeld=*/true, /*lookUpHeld=*/false, dt);
            }
            check(helicopterDescend.GetY() < yStartDescend,
                  "holding crouch (real 'Down') while flying a Helicopter descends Y over time");
        }
    }

    // Ghost mode (plan.md BLUPI-111) -- real absolute top-priority free
    // flight (Decor.cpp:2639-2705, confirmed via direct source research
    // 2026-07-14): no gravity, no collision, 4x normal speed, toggle-on
    // clears any active vehicle, toggle-off is rejected while standing
    // inside solid geometry.
    {
        Worlds::World ghostWorld;
        constexpr std::uint16_t kGhostGroundX = 30, kGhostGroundZ = 30;
        ghostWorld.setBlock(kGhostGroundX, 0, kGhostGroundZ, Worlds::Block::make(BlockTypes::Ground));

        GEBlupiController toggler;
        check(toggler.ToggleGhost(ghostWorld), "ToggleGhost() turns on from no state at all");
        check(toggler.IsGhost(), "IsGhost() reflects the new state");

        GEBlupiController vehicleThenGhost;
        vehicleThenGhost.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
        vehicleThenGhost.ToggleGhost(ghostWorld);
        check(!vehicleThenGhost.IsInVehicle(), "turning Ghost on clears any active vehicle mount (real behavior)");

        // No gravity: floating in open air, still airborne after several
        // frames of Step() with no input at all (a non-ghosted Blupi
        // would fall).
        GEBlupiController floater;
        floater.SetPosition(0.0f, 20.0f, 0.0f);
        floater.ToggleGhost(ghostWorld);
        const float yBeforeFloat = floater.GetY();
        for (int i = 0; i < 30; ++i)
        {
            floater.Step(ghostWorld, 0.0f, 0.0f, false, false, false, dt);
        }
        check(std::fabs(floater.GetY() - yBeforeFloat) < 0.01f,
              "Ghost mode has no gravity -- Y stays put with no vertical input, even far above any ground");

        // Free vertical flight via jumpPressed(up)/crouchHeld(down) --
        // this engine's adaptation of the real screen-vertical axis (see
        // kGhostSpeed's own comment).
        GEBlupiController riser;
        riser.SetPosition(0.0f, 20.0f, 0.0f);
        riser.ToggleGhost(ghostWorld);
        const float yBeforeRise = riser.GetY();
        riser.Step(ghostWorld, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(riser.GetY() > yBeforeRise, "holding Jump while ghosting flies upward");

        GEBlupiController sinker;
        sinker.SetPosition(0.0f, 20.0f, 0.0f);
        sinker.ToggleGhost(ghostWorld);
        const float yBeforeSink = sinker.GetY();
        sinker.Step(ghostWorld, 0.0f, 0.0f, false, /*crouchHeld=*/true, false, dt);
        check(sinker.GetY() < yBeforeSink, "holding crouch while ghosting flies downward");

        // Collision bypass: moving straight at/through the solid ground
        // block's own column (approached at the SAME height as the solid
        // block, which a normal walk would block outright). yaw=0 faces
        // -Z (this engine's own forward convention), so starting on the
        // +Z side of the block and walking forward crosses right through
        // its column and out the other side.
        const float ghostBlockWorldZ = static_cast<float>(kGhostGroundZ) - 50.0f;
        GEBlupiController flyer;
        flyer.SetPosition(static_cast<float>(kGhostGroundX) - 50.0f, 0.5f, ghostBlockWorldZ + 3.0f);
        flyer.ToggleGhost(ghostWorld);
        flyer.SetYaw(0.0f); // facing -Z, straight toward/through the solid block's own column
        for (int i = 0; i < 10; ++i)
        {
            flyer.Step(ghostWorld, 0.0f, 1.0f, false, false, false, dt);
        }
        check(flyer.GetZ() < ghostBlockWorldZ,
              "Ghost mode passes straight through solid geometry (no collision response at all)");

        // Real 4x speed: covers noticeably more ground per frame than a
        // normal (non-ghosted) walker given the identical input.
        GEBlupiController ghostMover;
        ghostMover.ToggleGhost(ghostWorld);
        ghostMover.SetYaw(0.0f);
        ghostMover.Step(ghostWorld, 0.0f, 1.0f, false, false, false, dt);
        GEBlupiController normalMover;
        normalMover.SetYaw(0.0f);
        normalMover.Step(ghostWorld, 0.0f, 1.0f, false, false, false, dt);
        check(std::fabs(ghostMover.GetZ()) > std::fabs(normalMover.GetZ()) * 3.0f,
              "Ghost mode's per-frame horizontal movement is markedly faster than normal walking "
              "(real exact 4x multiplier)");

        // Toggle-off gate: succeeds in open air, rejected inside solid
        // geometry (real `!DecorDetect(...)`, Decor.cpp:2065).
        GEBlupiController offInAir;
        offInAir.SetPosition(0.0f, 20.0f, 0.0f);
        offInAir.ToggleGhost(ghostWorld);
        check(!offInAir.ToggleGhost(ghostWorld), "ToggleGhost() off succeeds while standing in open air");
        check(!offInAir.IsGhost(), "IsGhost() reflects the toggle-off");

        GEBlupiController offInsideWall;
        offInsideWall.SetPosition(static_cast<float>(kGhostGroundX) - 50.0f, 0.3f,
                                   static_cast<float>(kGhostGroundZ) - 50.0f);
        offInsideWall.ToggleGhost(ghostWorld);
        check(offInsideWall.ToggleGhost(ghostWorld),
              "ToggleGhost() off is silently REJECTED while standing inside solid geometry (real behavior, "
              "avoids stranding Blupi mid-wall)");
        check(offInsideWall.IsGhost(), "IsGhost() stays true after the rejected toggle-off");
    }

    // Repro attempt for "grass-topped cubes reported walkable-through"
    // (NEXT.md §5/§8 task 4, reported live with no specific coordinates).
    // Icons 107/108/109 intentionally leave their PosY face un-rendered
    // (GETerrainRenderer's grass-top overlay draws a separate plate
    // instead -- see GEDirectionalCubeTiles.cpp) but must still be solid
    // for collision (IsSolidAt() only checks Block::isAir(), independent
    // of which faces a render mode chooses to draw). This walks Blupi
    // onto every icon-107/108/109 block actually present in the loaded
    // world and asserts he lands on top of it instead of falling through.
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        int grassTopBlocksTested = 0;
        for (int gx = 0; gx < blocksPerAxis; ++gx)
        {
            for (int gz = 0; gz < blocksPerAxis; ++gz)
            {
                for (int gy = blocksPerAxis - 1; gy >= 0; --gy)
                {
                    const auto block = world.getBlock(static_cast<std::uint16_t>(gx),
                                                        static_cast<std::uint16_t>(gy),
                                                        static_cast<std::uint16_t>(gz));
                    if (block.isAir()) continue;
                    const auto type = block.type();
                    if (type == 107 || type == 108 || type == 109)
                    {
                        const float worldX =
                            static_cast<float>(gx) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
                        const float worldZ =
                            static_cast<float>(gz) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
                        const float dropFromY = static_cast<float>(gy) + 3.0f;

                        GEBlupiController dropTest;
                        dropTest.SetPosition(worldX, dropFromY, worldZ);
                        for (int i = 0; i < 120; ++i) // ~2s, plenty to land and settle
                        {
                            dropTest.Step(world, 0.0f, 0.0f, false, false, false, dt);
                        }
                        std::cout << "Grass-top icon " << type << " at grid (" << gx << "," << gy
                                   << "," << gz << "): landed Y=" << dropTest.GetY()
                                   << " onGround=" << dropTest.IsOnGround() << std::endl;
                        check(dropTest.IsOnGround() &&
                                  dropTest.GetY() >= static_cast<float>(gy) + 1.0f - 0.01f,
                              "Blupi lands on top of a grass-topped block (icon 107/108/109), "
                              "not falling through it");
                        ++grassTopBlocksTested;
                    }
                }
            }
        }
        std::cout << grassTopBlocksTested
                   << " grass-topped (icon 107/108/109) block(s) tested for collision." << std::endl;
        check(grassTopBlocksTested > 0,
              "at least one grass-topped block exists in the world to test "
              "(otherwise this repro attempt tested nothing)");
    }

    // GroundHeightAt()'s roofed-interior fix (plan.md/NEXT.md §4/§5,
    // 2026-07-13): the south tunnel (tools/GenerateSampleWorld3D.cpp) is a
    // REAL enclosed interior already in this world -- floor at grid y=0,
    // BrickWall ceiling at grid y=3, open walkable interior at grid z 66-68
    // (world z 16-18) between BrickWall side walls at z=65/69. Before the
    // fix, GroundHeightAt() always scanned from the world's topmost Y down,
    // so the ceiling registered as this column's "floor" -- SetPosition()
    // into the tunnel's real floor got immediately overridden to the
    // ceiling's own top surface on the very next Step(). World x=-10 (grid
    // 40) is well inside the tunnel's x=20-75 footprint, away from any
    // other placed structure.
    {
        GEBlupiController tunnelWalker;
        tunnelWalker.SetPosition(-10.0f, 1.0f, 17.0f); // real tunnel floor height
        for (int i = 0; i < 30; ++i)
        {
            tunnelWalker.Step(world, 0.0f, 0.0f, false, false, false, dt);
        }
        std::cout << "South tunnel interior: y=" << tunnelWalker.GetY()
                   << " onGround=" << tunnelWalker.IsOnGround() << std::endl;
        check(tunnelWalker.IsOnGround(), "Blupi stays grounded on the tunnel's real floor");
        check(tunnelWalker.GetY() < 2.0f,
              "Blupi rests on the tunnel's real floor (y~1), not misresolved onto the "
              "BrickWall ceiling above it (y~4)");
    }

    // Jump-height headroom modulation (plan.md TILE-041, real
    // Decor::IsNormalJump()): a clipped ceiling within 2 tiles overhead
    // should reduce jump strength. Open sky (spawn (0,1,0)) vs. the same
    // south tunnel interior above (floor at grid y=0, BrickWall ceiling at
    // grid y=3 -- exactly 2 cells above Blupi's own standing height of 1,
    // triggering the "blocked" case).
    {
        // The jump-trigger assignment and gravity subtraction both happen
        // within the same Step() call (gravity runs later in that same
        // function body), so one call after the jump already reflects
        // exactly one frame of gravity -- subtract it here for an exact
        // comparison instead of a loose tolerance.
        const float kOneFrameGravity = GEBlupiController::kGravity * dt;

        GEBlupiController openJumper;
        openJumper.SetPosition(0.0f, 1.0f, 0.0f);
        for (int i = 0; i < 5; ++i) openJumper.Step(world, 0.0f, 0.0f, false, false, false, dt); // settle grounded
        openJumper.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        std::cout << "Open-sky jump velocityY=" << openJumper.GetVelocityY() << std::endl;
        check(std::fabs(openJumper.GetVelocityY() - (GEBlupiController::kJumpSpeed - kOneFrameGravity)) < 0.01f,
              "jumping with clear headroom uses the real full-height baseline (-16 equivalent)");

        GEBlupiController tunnelJumper;
        tunnelJumper.SetPosition(-10.0f, 1.0f, 17.0f);
        for (int i = 0; i < 5; ++i) tunnelJumper.Step(world, 0.0f, 0.0f, false, false, false, dt);
        tunnelJumper.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        std::cout << "Low-ceiling jump velocityY=" << tunnelJumper.GetVelocityY() << std::endl;
        check(std::fabs(tunnelJumper.GetVelocityY() -
                         (GEBlupiController::kJumpSpeedReduced - kOneFrameGravity)) < 0.01f,
              "jumping under the tunnel's low BrickWall ceiling uses the real reduced 'bumped head' "
              "height (-12 equivalent), not the full baseline");
        check(tunnelJumper.GetVelocityY() < openJumper.GetVelocityY(),
              "the low-ceiling jump is measurably weaker than the open-sky jump");
    }

    // Vehicle-mode ground-jump gate (real Decor.cpp:2913-2947, found 2026-07-16 while
    // researching E3D-MIG-065) -- Jeep/Tank must NOT respond to Jump via the normal ground-jump
    // path at all (previously this engine had no vehicle-mode check here whatsoever), and
    // Skateboard must use its OWN distinct real velocity (-13/-17), not the headroom-modulated
    // ordinary-Blupi values.
    {
        const float kOneFrameGravity = GEBlupiController::kGravity * dt;

        GEBlupiController jeepJumper;
        jeepJumper.SetPosition(0.0f, 1.0f, 0.0f);
        for (int i = 0; i < 5; ++i) jeepJumper.Step(world, 0.0f, 0.0f, false, false, false, dt);
        jeepJumper.TriggerMount(GEBlupiController::VehicleMode::Jeep, false, false);
        const float velocityYBeforeJeepJump = jeepJumper.GetVelocityY();
        jeepJumper.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(jeepJumper.GetVelocityY() < velocityYBeforeJeepJump + 0.01f,
              "pressing Jump while mounted in a Jeep does not launch a normal ground jump (real "
              "!m_blupiJeep gate) -- velocityY only reflects gravity, not a jump impulse");

        GEBlupiController tankJumper;
        tankJumper.SetPosition(0.0f, 1.0f, 0.0f);
        for (int i = 0; i < 5; ++i) tankJumper.Step(world, 0.0f, 0.0f, false, false, false, dt);
        tankJumper.TriggerMount(GEBlupiController::VehicleMode::Tank, false, false);
        const float velocityYBeforeTankJump = tankJumper.GetVelocityY();
        tankJumper.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(tankJumper.GetVelocityY() < velocityYBeforeTankJump + 0.01f,
              "pressing Jump while mounted in a Tank does not launch a normal ground jump (real "
              "!m_blupiTank gate)");

        GEBlupiController skateJumper;
        skateJumper.SetPosition(0.0f, 1.0f, 0.0f);
        for (int i = 0; i < 5; ++i) skateJumper.Step(world, 0.0f, 0.0f, false, false, false, dt);
        skateJumper.TriggerMount(GEBlupiController::VehicleMode::Skateboard, false, false);
        skateJumper.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        std::cout << "Skateboard jump velocityY=" << skateJumper.GetVelocityY() << std::endl;
        check(std::fabs(skateJumper.GetVelocityY() -
                         (GEBlupiController::kSkateboardJumpSpeed - kOneFrameGravity)) < 0.01f,
              "Skateboard jump uses its own real distinct velocity (-13 equivalent), not the "
              "headroom-modulated ordinary-Blupi values");
    }

    // JumpSkate/AirSkate (table_blupi actions 40/41, wired 2026-07-19) --
    // Skateboard is the only vehicle mode with its own real airborne icon
    // pair, split by velocity sign exactly like the base humanoid Jump/Air
    // (plan.md E3D-MIG-064).
    {
        GEBlupiController skateAirborne;
        skateAirborne.SetPosition(0.0f, 1.0f, 0.0f);
        for (int i = 0; i < 5; ++i) skateAirborne.Step(world, 0.0f, 0.0f, false, false, false, dt);
        skateAirborne.TriggerMount(GEBlupiController::VehicleMode::Skateboard, false, false);
        skateAirborne.Step(world, 0.0f, 0.0f, /*jumpPressed=*/true, false, false, dt);
        check(!skateAirborne.IsOnGround(), "Skateboard jump launches Blupi airborne");
        check(skateAirborne.GetAnimState() == GEBlupiController::AnimState::JumpSkate,
              "ascending on a Skateboard is the JumpSkate anim state, not the base Jump or StopSkate");
        check(skateAirborne.GetAnimIcon() == 210,
              "JumpSkate anim icon starts at the real first frame (icon 210)");

        int stepsToApex = 0;
        while (skateAirborne.GetAnimState() == GEBlupiController::AnimState::JumpSkate && stepsToApex < 200)
        {
            skateAirborne.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToApex;
        }
        check(stepsToApex < 200, "Skateboard jump reaches its apex within a bounded time");
        check(skateAirborne.GetAnimState() == GEBlupiController::AnimState::AirSkate,
              "past the apex, falling on a Skateboard switches to the AirSkate anim state, not "
              "MarchSkate/StopSkate");
        check(skateAirborne.GetAnimIcon() == 213,
              "AirSkate anim icon starts at the real first frame (icon 213)");

        int stepsToLand = 0;
        while (!skateAirborne.IsOnGround() && stepsToLand < 200)
        {
            skateAirborne.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToLand;
        }
        check(skateAirborne.IsOnGround(), "lands again after the Skateboard jump arc completes");
        check(skateAirborne.GetAnimState() == GEBlupiController::AnimState::StopSkate,
              "back on the ground and idle on a Skateboard returns to StopSkate, not stuck in AirSkate");
    }

    // TakeSkate/DeposeSkate one-shot mount/dismount anim (table_blupi actions
    // 42/43, wired 2026-07-19 in GalaxyEggbertCnaGame.cpp's mount/dismount
    // hook points) -- direct GEBlupiController-level coverage of the
    // TriggerOneShotAnim() mechanism itself, same "freeze, count down,
    // auto-resume" shape already used by Switch/TakeDynamite/PutDynamite.
    {
        GEBlupiController mounting;
        mounting.SetPosition(0.0f, 1.0f, 0.0f);
        check(mounting.TriggerOneShotAnim(GEBlupiController::AnimState::TakeSkate,
                                           GEBlupiController::kTakeSkateDuration),
              "TriggerOneShotAnim(TakeSkate) returns true when not already frozen/locked");
        check(mounting.IsOneShotAnimPlaying(), "IsOneShotAnimPlaying() is true immediately after triggering TakeSkate");
        mounting.Step(world, 0.0f, 0.0f, false, false, false, dt);
        check(mounting.GetAnimState() == GEBlupiController::AnimState::TakeSkate,
              "TakeSkate anim state takes precedence over Stop/StopSkate while playing");
        check(mounting.GetAnimIcon() == 17, "TakeSkate anim icon starts at the real first frame (icon 17)");

        int stepsToResume = 0;
        while (mounting.IsOneShotAnimPlaying() && stepsToResume < 200)
        {
            mounting.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToResume;
        }
        check(stepsToResume < 200, "TakeSkate one-shot anim resolves within a bounded time");
        check(!mounting.IsOneShotAnimPlaying(), "TakeSkate one-shot anim ends on its own after kTakeSkateDuration");

        GEBlupiController dismounting;
        dismounting.SetPosition(0.0f, 1.0f, 0.0f);
        check(dismounting.TriggerOneShotAnim(GEBlupiController::AnimState::DeposeSkate,
                                              GEBlupiController::kDeposeSkateDuration),
              "TriggerOneShotAnim(DeposeSkate) returns true when not already frozen/locked");
        dismounting.Step(world, 0.0f, 0.0f, false, false, false, dt);
        check(dismounting.GetAnimState() == GEBlupiController::AnimState::DeposeSkate,
              "DeposeSkate anim state takes precedence while playing");
        check(dismounting.GetAnimIcon() == 210, "DeposeSkate anim icon starts at the real first frame (icon 210)");

        int stepsToResumeDepose = 0;
        while (dismounting.IsOneShotAnimPlaying() && stepsToResumeDepose < 200)
        {
            dismounting.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToResumeDepose;
        }
        check(stepsToResumeDepose < 200, "DeposeSkate one-shot anim resolves within a bounded time");
        check(!dismounting.IsOneShotAnimPlaying(), "DeposeSkate one-shot anim ends on its own after kDeposeSkateDuration");
    }

    // FireTank one-shot recoil anim (table_blupi action 53, wired
    // 2026-07-19 alongside GEInteractionSystem::EventKind::TankFired in
    // GalaxyEggbertCnaGame.cpp) -- same direct GEBlupiController-level
    // TriggerOneShotAnim() coverage as TakeSkate/DeposeSkate above.
    {
        GEBlupiController firing;
        firing.SetPosition(0.0f, 1.0f, 0.0f);
        check(firing.TriggerOneShotAnim(GEBlupiController::AnimState::FireTank,
                                         GEBlupiController::kFireTankDuration),
              "TriggerOneShotAnim(FireTank) returns true when not already frozen/locked");
        firing.Step(world, 0.0f, 0.0f, false, false, false, dt);
        check(firing.GetAnimState() == GEBlupiController::AnimState::FireTank,
              "FireTank anim state takes precedence while playing");
        check(firing.GetAnimIcon() == 251, "FireTank anim icon starts at the real first frame (icon 251)");

        int stepsToResumeFiring = 0;
        while (firing.IsOneShotAnimPlaying() && stepsToResumeFiring < 200)
        {
            firing.Step(world, 0.0f, 0.0f, false, false, false, dt);
            ++stepsToResumeFiring;
        }
        check(stepsToResumeFiring < 200, "FireTank one-shot anim resolves within a bounded time");
        check(!firing.IsOneShotAnimPlaying(), "FireTank one-shot anim ends on its own after kFireTankDuration");
    }

    // Airborne horizontal wall collision (INFRA-005, plan.md §7) -- the
    // CONFIRMED bug this whole resolver refactor set out to fix:
    // TryMoveAxis() used to short-circuit its wall check entirely while
    // `!m_onGround`, so a jumping/floating Blupi passed straight through
    // any side wall. ResolveMove() now applies the same real wall check
    // unconditionally regardless of grounded state (matching real
    // Decor::BlupiStep()'s own unconditional TestPath() call, confirmed via
    // direct source research 2026-07-21) -- this test proves it directly:
    // launch a jump, then walk into a wall entirely while airborne (the
    // wall is tall enough that Blupi never lands on it mid-approach), and
    // confirm he's blocked, not clipped through.
    {
        Worlds::World airborneWallWorld;
        constexpr std::uint16_t kFloorX = 90, kFloorZ = 90;
        // A floor strip Blupi walks along, and a wall 10 tiles tall (kJumpSpeed=12,
        // kGravity=25 -> max real jump apex is 12*12/(2*25)=2.88 units, so 10 is
        // comfortably unreachable -- Blupi can never land ON TOP of this wall
        // during the jump arc below, which would otherwise let a reintroduced
        // bug hide by "landing" on the wall's top instead of genuinely clipping
        // past it in open air).
        for (std::uint16_t dx = 0; dx <= 5; ++dx)
        {
            airborneWallWorld.setBlock(static_cast<std::uint16_t>(kFloorX + dx), 0, kFloorZ,
                                        Worlds::Block::make(BlockTypes::Ground));
        }
        for (std::uint16_t dy = 0; dy <= 9; ++dy)
        {
            airborneWallWorld.setBlock(static_cast<std::uint16_t>(kFloorX + 5), dy, kFloorZ,
                                        Worlds::Block::make(BlockTypes::Ground));
        }
        // Starts RIGHT NEXT TO the wall (0.7 units away -- the wall's own
        // solid footprint is center-anchored, spanning [wallX-0.5,
        // wallX+0.5), so anything closer than 0.5 would already be
        // INSIDE it) so the jump's own brief airborne window is what
        // actually meets the wall -- placing him further back would let
        // him LAND again (back to normal, already-tested grounded wall
        // collision) well before ever reaching it, never exercising the
        // airborne case this test exists for at all.
        const float wallX = static_cast<float>(kFloorX + 5) - 50.0f;
        GEBlupiController jumper;
        jumper.SetPosition(wallX - 0.7f, 1.0f, static_cast<float>(kFloorZ) - 50.0f);
        jumper.SetYaw(0.0f);
        jumper.Step(airborneWallWorld, 0.0f, 0.0f, false, false, false, dt);
        check(jumper.IsOnGround(), "sanity: airborne-wall test's jumper starts grounded next to the wall");

        // Face toward the wall (+X, matching how the floor strip/wall were
        // placed) -- sin(yaw)=1 needs yaw=pi/2. Jump AND move toward the
        // wall in the same frame, so he's genuinely airborne for the very
        // first step that could contact it.
        jumper.SetYaw(1.57079633f);
        jumper.Step(airborneWallWorld, 0.0f, 1.0f, /*jumpPressed=*/true, false, false, dt);
        check(!jumper.IsOnGround(), "sanity: jumper is airborne after jumping");

        for (int i = 0; i < 10 && !jumper.IsOnGround(); ++i) // a handful of frames, well within the airborne window
        {
            jumper.Step(airborneWallWorld, 0.0f, 1.0f, false, false, false, dt);
        }
        check(jumper.GetX() < wallX - 0.45f,
              "airborne movement is blocked by a wall, same as grounded movement (the INFRA-005 fix)");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
