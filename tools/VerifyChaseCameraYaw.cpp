#include <GalaxyEggbert/Game/ChaseCameraYaw.hpp>

#include <cmath>
#include <iostream>

// Scripted verification of ChaseCameraYaw (the real-Blupi plan's phase E): the camera lags
// behind a turn, orbits to Blupi's nearer open side when he stands idle, stays behind when
// both sides are walled, and goes to his right side for clips that face the sprites' viewer.
// No graphics context needed.
int main()
{
    using namespace GalaxyEggbert::Game;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };
    constexpr float kPi = 3.14159265358979f;
    constexpr float dt = 1.0f / 60.0f;
    const auto near = [](float a, float b, float tolerance)
    { return std::fabs(ChaseCameraYaw::Wrap(a - b)) <= tolerance; };
    const auto run = [](ChaseCameraYaw& camera, const ChaseCameraInput& input, float seconds)
    {
        for (float t = 0.0f; t < seconds; t += dt)
        {
            camera.Update(input, dt);
        }
    };

    check(near(ChaseCameraYaw::Wrap(3.0f * kPi), kPi, 1e-5f), "Wrap(3 pi) is pi");
    check(near(ChaseCameraYaw::Wrap(-kPi), kPi, 1e-5f) && ChaseCameraYaw::Wrap(-kPi) > 0.0f,
          "Wrap(-pi) is +pi, the range is (-pi, pi]");

    // Moving: behind him, lagging on a turn.
    {
        ChaseCameraYaw camera;
        camera.Reset(0.3f);
        check(near(camera.GetYaw(), 0.3f, 1e-6f) && camera.GetSide() == ChaseCameraSide::Behind,
              "Reset puts the camera straight behind");
        ChaseCameraInput moving;
        moving.blupiYaw = 0.3f;
        run(camera, moving, 1.0f);
        check(near(camera.GetYaw(), 0.3f, 1e-4f), "walking straight keeps it behind");
        moving.blupiYaw = 0.3f + kPi / 2.0f;
        run(camera, moving, 0.2f);
        check(!near(camera.GetYaw(), moving.blupiYaw, kPi / 4.0f),
              "0.2 s after a 90-degree turn the camera still trails by over 45 degrees (side view)");
        run(camera, moving, 2.0f);
        check(near(camera.GetYaw(), moving.blupiYaw, 0.01f), "2 s later it is behind him again");
    }

    check(std::fabs(ChaseCameraYaw::kIdleDelaySeconds - 3.0f) < 1e-6f,
          "it waits 3 s of standing still before leaving his back");

    // Idle: the nearer side after the delay, his right on a tie.
    {
        ChaseCameraYaw camera;
        camera.Reset(1.0f);
        ChaseCameraInput idle;
        idle.blupiYaw = 1.0f;
        idle.idle = true;
        run(camera, idle, ChaseCameraYaw::kIdleDelaySeconds * 0.9f);
        check(camera.GetSide() == ChaseCameraSide::Behind && near(camera.GetYaw(), 1.0f, 1e-4f),
              "before the idle delay it stays behind");
        run(camera, idle, 4.0f);
        check(camera.GetSide() == ChaseCameraSide::Right && near(camera.GetYaw(), 1.0f - kPi / 2.0f, 0.01f),
              "idle straight behind him: it orbits to his right");
        ChaseCameraInput moving;
        moving.blupiYaw = 1.0f;
        run(camera, moving, 3.0f);
        check(camera.GetSide() == ChaseCameraSide::Behind && near(camera.GetYaw(), 1.0f, 0.01f),
              "walking again brings it back behind");
    }
    {
        ChaseCameraYaw camera;
        camera.Reset(0.0f);
        ChaseCameraInput moving;
        moving.blupiYaw = -0.6f; // he turned right; the camera trails on his left for a moment
        run(camera, moving, 0.1f);
        ChaseCameraInput idle = moving;
        idle.idle = true;
        idle.rightOpen = false;
        run(camera, idle, ChaseCameraYaw::kIdleDelaySeconds * 0.9f);
        check(camera.GetSide() == ChaseCameraSide::Behind && near(camera.GetYaw(), -0.6f, 0.01f),
              "during the 3 s wait it has caught up behind him");
        run(camera, idle, 5.0f);
        check(camera.GetSide() == ChaseCameraSide::Left && near(camera.GetYaw(), -0.6f + kPi / 2.0f, 0.01f),
              "idle with a wall on his right: it takes his left side");
        idle.rightOpen = true;
        idle.leftOpen = false;
        run(camera, idle, 5.0f);
        check(camera.GetSide() == ChaseCameraSide::Right, "a wall on the left sends it to his right");
        idle.leftOpen = true;
        run(camera, idle, 1.0f);
        check(camera.GetSide() == ChaseCameraSide::Right, "once on a side it stays while that side is open");
        idle.leftOpen = false;
        idle.rightOpen = false;
        run(camera, idle, 5.0f);
        check(camera.GetSide() == ChaseCameraSide::Behind && near(camera.GetYaw(), -0.6f, 0.01f),
              "walls on both sides: it stays behind");
    }

    // Clips facing the viewer: his right side, moving or not, walled or not.
    {
        ChaseCameraYaw camera;
        camera.Reset(2.0f);
        ChaseCameraInput squashed;
        squashed.blupiYaw = 2.0f;
        squashed.facesViewer = true;
        squashed.rightOpen = false;
        run(camera, squashed, 4.0f);
        check(camera.GetSide() == ChaseCameraSide::Right && near(camera.GetYaw(), 2.0f - kPi / 2.0f, 0.01f),
              "a clip facing the viewer puts it on his right, even against a wall");
    }

    // The camera rises over what blocks it behind him (D18).
    {
        using GalaxyEggbert::Game::ChaseCameraElevation;
        const auto open = [](float) { return 4.0f; };
        check(near(ChaseCameraElevation(0.29f, 4.0f, open), 0.29f, 1e-5f), "open behind him: the usual elevation");
        const auto pillar = [](float e) { return e < 0.7f ? 0.3f : 4.0f; }; // blocked low, clear above
        const float overPillar = ChaseCameraElevation(0.29f, 4.0f, pillar);
        check(overPillar >= 0.7f && overPillar < 0.9f, "blocked low behind him: the lowest elevation that clears it");
        const auto closed = [](float) { return 0.3f; };
        check(near(ChaseCameraElevation(0.29f, 4.0f, closed), 0.29f, 1e-5f),
              "blocked at every height (a tunnel): stays where it was");
        const auto slightly = [](float e) { return 0.3f + 0.2f * e; }; // barely better higher up
        check(near(ChaseCameraElevation(0.29f, 4.0f, slightly), 0.29f, 1e-5f),
              "rising gains little room: stays where it was");
        const auto ceiling = [](float e) { return e < 0.6f ? 0.5f : 2.0f; }; // clearer above, never enough
        check(ChaseCameraElevation(0.29f, 4.0f, ceiling) >= 0.6f, "clearly more room higher up: it rises");
    }

    std::cout << (allOk ? "ALL PASS" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
