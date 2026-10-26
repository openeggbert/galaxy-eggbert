#include <GalaxyEggbert/Game/Autoplay.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

// Scripted verification of the QA autoplay (Autoplay.hpp): its script parses, and its commands
// steer a simple stand-in for Blupi (turning 135 degrees/s, walking 2.5 cells/s, as
// BlupiController's kTurnSpeed/kMoveSpeed) to where the script says, jump where it says, stop on
// a timeout or a death, and count expectations.
namespace
{
    using GalaxyEggbert::Game::Autoplay;
    using GalaxyEggbert::Game::AutoplayKeys;
    using GalaxyEggbert::Game::AutoplayState;

    struct Run
    {
        AutoplayState state;
        int jumpTicks = 0;
        int firstJumpTick = -1;
        float firstJumpDistanceTo5 = -1.0f;
        std::string log;
    };

    // Plays `script` against the stand-in for at most `ticks` ticks.
    Run Play(const std::string& script, int ticks, AutoplayState start = {})
    {
        Autoplay autoplay;
        std::string error;
        Run run;
        run.state = start;
        if (!autoplay.Parse(script, error))
        {
            run.log = "parse error: " + error;
            return run;
        }
        for (int t = 0; t < ticks && !autoplay.Finished(); ++t)
        {
            run.state.tick = t;
            const AutoplayKeys keys = autoplay.Step(run.state);
            constexpr float kTurnPerTick = 135.0f / 60.0f;
            constexpr float kWalkPerTick = 2.5f / 60.0f;
            if (keys.right) run.state.yawDegrees += kTurnPerTick;
            if (keys.left) run.state.yawDegrees -= kTurnPerTick;
            if (keys.up)
            {
                const float yaw = run.state.yawDegrees * 3.14159265f / 180.0f;
                run.state.x += std::sin(yaw) * kWalkPerTick;
                run.state.z -= std::cos(yaw) * kWalkPerTick;
            }
            if (keys.jump)
            {
                if (run.firstJumpTick < 0)
                {
                    run.firstJumpTick = t;
                    run.firstJumpDistanceTo5 = std::hypot(run.state.x - 5.0f, run.state.z);
                }
                ++run.jumpTicks;
            }
        }
        run.log += autoplay.TakeLog();
        run.log += autoplay.Passed() ? "RESULT pass\n" : (autoplay.Finished() ? "RESULT fail\n" : "RESULT running\n");
        return run;
    }

    bool Contains(const std::string& text, const std::string& part)
    {
        return text.find(part) != std::string::npos;
    }
}

int main()
{
    bool allOk = true;
    const auto check = [&allOk](bool cond, const std::string& what) {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    {
        Autoplay autoplay;
        std::string error;
        const bool parsed = autoplay.Parse("wait 10\nfly 3 4\n", error);
        check(!parsed && Contains(error, "line 2"), "an unknown command fails to parse, naming its line (" + error + ")");
        check(!autoplay.Parse("hold 5 up sideways\n", error), "an unknown key fails to parse");
        check(!autoplay.Parse("until phase = Flying\n", error), "an unknown phase fails to parse");
        check(!autoplay.Parse("go 3\n", error), "go without its Z fails to parse");
        check(autoplay.Parse("# a comment\n\nwait 3 # trailing\ngo 1 2 jump 0.5 tol 0.1 max 99\n"
                             "until phase = Win max 30\nexpect treasures >= 2\nlog hello\nend\n",
                             error),
              "every command parses");
    }

    {
        const Run run = Play("face 90\nend\n", 200);
        check(std::fabs(run.state.yawDegrees - 90.0f) <= 1.5f, "face 90 turns him right to +X");
        check(Contains(run.log, "RESULT pass"), "face finishes");
        const Run back = Play("face -45\n", 200, AutoplayState{.yawDegrees = 170.0f});
        check(std::fabs(back.state.yawDegrees - 315.0f) <= 1.5f,
              "face takes the short way round (170 to -45 turns right through 180)");
    }

    {
        const Run run = Play("go 5 0\ngo 5 -4\ngo 0 -4\n", 2000);
        check(std::hypot(run.state.x - 0.0f, run.state.z + 4.0f) <= 0.25f && Contains(run.log, "RESULT pass"),
              "go walks a route of three legs and stops within its tolerance");
    }

    {
        const Run run = Play("go 5 0 until x >= 2\nwait 1\n", 2000);
        check(run.state.x >= 2.0f && run.state.x < 2.2f && Contains(run.log, "RESULT pass"),
              "go ... until COND stops as soon as COND holds");
    }

    {
        const Run run = Play("go 5 0 jump 1.5\n", 2000);
        check(run.jumpTicks == 6, "go ... jump D jumps once (6 ticks)");
        check(run.firstJumpDistanceTo5 <= 1.5f && run.firstJumpDistanceTo5 > 1.4f,
              "the jump starts at D from the target");
    }

    {
        const Run run = Play("until phase = Win max 20\nlog unreachable\n", 100);
        check(Contains(run.log, "FAIL timeout") && !Contains(run.log, "unreachable") &&
                  Contains(run.log, "RESULT fail"),
              "until gives up after max ticks and ends the run as a failure");
    }

    {
        const Run run = Play("expect mission = 11\nexpect treasures >= 1\n", 10,
                             AutoplayState{.mission = 11, .treasures = 0});
        check(Contains(run.log, "PASS expect mission = 11") && Contains(run.log, "FAIL expect treasures >= 1") &&
                  Contains(run.log, "RESULT fail"),
              "expect records a pass and a failure; a failure fails the run");
    }

    {
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("wait 100\n", error);
        AutoplayState state;
        for (int t = 0; t < 5; ++t)
        {
            state.tick = t;
            state.dead = t >= 3;
            autoplay.Step(state);
        }
        const std::string log = autoplay.TakeLog();
        check(autoplay.Finished() && !autoplay.Passed() && Contains(log, "t=3 FAIL died"),
              "a death ends the run as a failure at once");
    }

    {
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("hold 3 up jump\nend\nwait 50\n", error);
        AutoplayState state;
        int held = 0;
        for (int t = 0; t < 20 && !autoplay.Finished(); ++t)
        {
            state.tick = t;
            const AutoplayKeys keys = autoplay.Step(state);
            held += keys.up && keys.jump ? 1 : 0;
        }
        check(held == 3 && autoplay.Passed(), "hold holds its keys for N ticks; end stops the script");
    }

    {
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("wait 200\n", error);
        AutoplayState state{.phase = GalaxyEggbert::Def::GamePhase::Play, .mission = 11, .lives = 3};
        for (int t = 0; t < 130; ++t)
        {
            state.tick = t;
            if (t == 70) state.treasures = 1;
            if (t == 100) state.phase = GalaxyEggbert::Def::GamePhase::Win;
            autoplay.Step(state);
        }
        const std::string log = autoplay.TakeLog();
        check(Contains(log, "t=0 phase Play") && Contains(log, "t=0 mission 11") &&
                  Contains(log, "t=70 treasures 1 eggs 0 lives 3") && Contains(log, "t=100 phase Win") &&
                  Contains(log, "t=60 at "),
              "the trace records phases, the mission, pickups and the position every second");
    }

    {
        Autoplay autoplay;
        std::string error;
        check(autoplay.Parse("lift 58 10 34 max 300\nwait 1\n", error), "lift parses");
        AutoplayState state;
        int arrived = -1;
        for (int t = 0; t < 100 && arrived < 0; ++t)
        {
            state.tick = t;
            // A lift rising from feet height 13 to 10 at 0.1 a tick (arrives on tick 30).
            state.lifts = {{50.0f, 12.0f, 30.0f}, {58.0f, std::max(10.0f, 13.0f - 0.1f * t), 34.0f}};
            autoplay.Step(state);
            const std::string log = autoplay.TakeLog();
            if (Contains(log, "line 2: wait 1"))
            {
                arrived = t;
            }
        }
        check(arrived == 30, "lift waits until a lift stands at its point (tick " + std::to_string(arrived) + ")");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
