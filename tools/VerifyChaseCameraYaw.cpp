#include <GalaxyEggbert/Game/ChaseCameraYaw.hpp>
#include <random>

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
        const auto tunnel = [](float e) { return e <= 0.05f ? 4.0f : 0.4f; }; // a low ceiling, open along it
        const float inTunnel = ChaseCameraElevation(0.29f, 4.0f, tunnel);
        check(inTunnel <= 0.05f && inTunnel >= -0.35f, "a low ceiling over him: it drops toward the level");
    }

    // The look point drops toward his middle as the camera comes in (play pass 2026-11-03).
    {
        using GalaxyEggbert::Game::ChaseLookHeight;
        check(near(ChaseLookHeight(4.0f), 1.0f, 1e-5f) && near(ChaseLookHeight(6.0f), 1.0f, 1e-5f),
              "from the usual distance or further it looks a block over his feet");
        check(near(ChaseLookHeight(0.0f), 0.45f, 1e-5f), "against him it looks at his middle");
        check(ChaseLookHeight(1.0f) < ChaseLookHeight(2.0f) && ChaseLookHeight(2.0f) < ChaseLookHeight(3.0f),
              "the closer, the lower");
        // His feet in the picture from two blocks: the eye stands 2 back at the usual 0.29 rad
        // elevation from the point a block over his feet (the game places it so) and looks at the
        // lowered look point; his feet within the 45-degree picture's lower half (22.5 degrees).
        const float d = 2.0f, e = 0.29f;
        const float eyeHeight = 1.0f + d * std::sin(e);
        const float feetAngle = std::atan2(eyeHeight, d * std::cos(e));
        const float lowered = feetAngle - std::atan2(eyeHeight - ChaseLookHeight(d), d * std::cos(e));
        const float unlowered = feetAngle - std::atan2(eyeHeight - 1.0f, d * std::cos(e));
        check(lowered < 22.5f * kPi / 180.0f && unlowered > 22.5f * kPi / 180.0f,
              "two blocks off, his feet stay in the picture (looking a block over them, they left it)");
    }

    // Camera audit regressions: the old 0.1-block march missed a short diagonal corner
    // crossing and failed to recognise a look point already inside its wall.
    {
        const auto block = [](int x, int y, int z) { return x == 5 && y == 5 && z == 5; };
        const auto room = [&](float x, float y, float z, float dx, float dy, float dz, float distance)
        { return ChaseCameraWallDistance(10, x, y, z, dx, dy, dz, distance, block); };
        check(std::fabs(room(2, 5, 5, 1, 0, 0, 6) - 2.35f) < 1e-5f, "camera stops before the exact wall face");
        check(room(5, 5, 5, 1, 0, 0, 4) == 0, "blocked origin has no safe arm");
        check(room(2, 3, 5, 1, 0, 0, 6) == 6, "open ray keeps the chase distance");
        check(room(4.48f, 5.47f, 5, 1, 1, 0, 1) == 0,
              "short grazing intersection is not skipped");
        check(std::fabs(room(8, 5, 5, -1, 0, 0, 6) - 2.35f) < 1e-5f, "negative ray uses the same margin");
        // Compare the traversal against independent analytic slab intersections.
        std::mt19937 random(1701);
        std::uniform_real_distribution<float> coordinate(1.0f, 9.0f);
        bool agrees = true;
        for (int sample = 0; sample < 2000; ++sample)
        {
            float origin[3], direction[3], length2 = 0.0f;
            for (int axis = 0; axis < 3; ++axis)
            {
                origin[axis] = coordinate(random);
                direction[axis] = coordinate(random) - origin[axis];
                length2 += direction[axis] * direction[axis];
            }
            const float length = std::sqrt(length2);
            float enter = 0.0f, leave = length;
            for (int axis = 0; axis < 3; ++axis)
            {
                direction[axis] /= length;
                if (std::fabs(direction[axis]) < 1e-7f)
                {
                    if (origin[axis] < 4.5f || origin[axis] > 5.5f) leave = -1.0f;
                    continue;
                }
                const float a = (4.5f - origin[axis]) / direction[axis];
                const float b = (5.5f - origin[axis]) / direction[axis];
                enter = std::max(enter, std::min(a, b));
                leave = std::min(leave, std::max(a, b));
            }
            const float expected = enter <= leave ? std::max(0.0f, enter - 0.15f) : length;
            const float actual = room(origin[0], origin[1], origin[2], direction[0], direction[1], direction[2], length);
            agrees &= std::fabs(expected - actual) < 2e-5f;
        }
        check(agrees, "2000 camera rays agree with independent slab intersections");
    }

    std::cout << (allOk ? "ALL PASS" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
