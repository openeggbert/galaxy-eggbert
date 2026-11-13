#include <GalaxyEggbert/Game/Autoplay.hpp>
#include <GalaxyEggbert/Game/ResumeLog.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
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
        check(autoplay.Parse("until blitz = 52\nuntil temp = 0\nuntil crush = 3\n", error),
              "the lightning's, the saucers' and the crushers' cycles are fields (blitz, temp, crush)");
        check(autoplay.Parse("# a comment\n\nwait 3 # trailing\ngo 1 2 jump 0.5 tol 0.1 max 99\n"
                             "until phase = Win max 30\nexpect treasures >= 2\nlog hello\nend\n",
                             error),
              "every command parses");
    }

    {
        Autoplay typing;
        std::string error;
        check(typing.Parse("type ghost\nend\n", error), "typed cheat parses");
        AutoplayState state;
        const std::string word = "ghost";
        for (int tick = 0; tick < 10; ++tick)
        {
            state.tick = tick;
            const auto keys = typing.Step(state);
            check(keys.letter == (tick % 2 == 0 ? word[tick / 2] : 0),
                  "typed cheat emits a letter press then release");
        }
        state.tick = 10;
        typing.Step(state);
        check(typing.Finished() && typing.Passed(), "typing completes after the last release");
        Autoplay bad;
        check(!bad.Parse("type ghost extra", error), "typed cheat rejects extra words");
    }

    {
        // shots N: the host saves the frame of each of the N steps from the one that read it.
        Autoplay autoplay;
        std::string error;
        check(!autoplay.Parse("shots 0\n", error) && !autoplay.Parse("shots\n", error),
              "shots needs a positive tick count");
        check(autoplay.Parse("wait 2\nshots 3\nwait 10\n", error), "shots parses");
        std::string wanted;
        AutoplayState state;
        for (int t = 0; t < 10; ++t)
        {
            state.tick = t;
            autoplay.Step(state);
            wanted += autoplay.WantsShot() ? '1' : '0';
        }
        check(wanted == "0011100000", "shots 3 wants the frames of three ticks, from the step that read it (" +
                                          wanted + ")");
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
        const Run run = Play("go 2 0 tol 0.05\n", 600);
        check(std::hypot(run.state.x - 2.0f, run.state.z) <= 0.05f && Contains(run.log, "RESULT pass"),
              "go ... tol 0.05 still walks right up to its point");
    }

    {
        // Held at a point it cannot pass (the stand-in has no walls: it reaches it), with no
        // tolerance: it stops turning there instead of spinning on the spot.
        const Run run = Play("go 3 0 tol 0 until tick >= 600\n", 700);
        check(std::hypot(run.state.x - 3.0f, run.state.z) < 0.2f && std::fabs(run.state.yawDegrees - 90.0f) < 30.0f,
              "go ... tol 0 settles at its point without spinning (yaw " + std::to_string(run.state.yawDegrees) + ")");
    }

    {
        const Run run = Play("go 5 0 until x >= 2\nwait 1\n", 2000);
        check(run.state.x >= 2.0f && run.state.x < 2.2f && Contains(run.log, "RESULT pass"),
              "go ... until COND stops as soon as COND holds");
    }

    {
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("go 0 -3 with crouch with lookup\n", error);
        AutoplayState state;
        const AutoplayKeys keys = autoplay.Step(state);
        check(keys.up && keys.crouch && keys.lookUp && !keys.jump, "go ... with KEY holds the keys while it walks");
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
        // mtick counts from the mission's start: in a chain a mission's own timing still holds.
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("until mission = 12\nuntil mtick >= 3\nlog three\nuntil tick >= 9\nlog nine\n", error);
        AutoplayState state{.mission = 11};
        for (int t = 0; t < 20 && !autoplay.Finished(); ++t)
        {
            state.tick = t;
            state.mission = t >= 5 ? 12 : 11;
            autoplay.Step(state);
        }
        const std::string log = autoplay.TakeLog();
        check(autoplay.Passed() && Contains(log, "t=8 log three") && Contains(log, "t=9 log nine"),
              "mtick counts from the mission's start, tick from the script's");
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
                  Contains(log, "t=70 treasures 1 eggs 0 lives 3 keys 0 dynamite 0") && Contains(log, "t=100 phase Win") &&
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

    {
        Autoplay autoplay;
        std::string error;
        // Crates move continuously and rest anywhere (BD-0014): from 33 to 32 over ticks 10-20.
        autoplay.Parse("crate 32.5 10 45 tol 0.06 max 100\ncrate 32 10 45 max 100\nwait 5\n", error);
        AutoplayState state;
        std::string log;
        int halfWay = -1;
        for (int t = 0; t < 50 && !autoplay.Finished(); ++t)
        {
            state.tick = t;
            const float x = t < 10 ? 33.0f : t < 20 ? 33.0f - static_cast<float>(t - 10) * 0.1f : 32.0f;
            state.crates = {{x, 10.0f, 45.0f}};
            autoplay.Step(state);
            const std::string taken = autoplay.TakeLog();
            if (halfWay < 0 && Contains(taken, "crate 32 10 45 max 100"))
            {
                halfWay = t;
            }
            log += taken;
        }
        check(autoplay.Passed() && halfWay == 15 && Contains(log, "t=21 crate (33.00 10.00 45.00) -> (32.00 10.00 45.00)") &&
                  !Contains(log, "-> (32.50") && Contains(log, "t=0 crates 1") &&
                  Contains(log, "PASS script complete"),
              "crate waits for a crate at a point (tol along X and Z); the trace records where each crate comes "
              "to rest, not its moving positions");
    }

    {
        // include: a scratch tree in the working directory (the build directory under ctest).
        namespace fs = std::filesystem;
        const fs::path root = "autoplay_include_check";
        fs::create_directories(root / "sub");
        std::ofstream(root / "main.txt") << "wait 2\ninclude sub/part.txt\nend\n";
        std::ofstream(root / "sub" / "part.txt") << "hold 3 up\ninclude more.txt\n";
        std::ofstream(root / "sub" / "more.txt") << "log deep\n";
        std::ofstream(root / "bad.txt") << "include sub/broken.txt\n";
        std::ofstream(root / "sub" / "broken.txt") << "wait 1\njump 3\n";
        Autoplay autoplay;
        std::string error;
        const bool loaded = autoplay.Load((root / "main.txt").string(), error);
        AutoplayState state;
        int up = 0;
        for (int t = 0; t < 20 && !autoplay.Finished(); ++t)
        {
            state.tick = t;
            up += autoplay.Step(state).up ? 1 : 0;
        }
        const std::string log = autoplay.TakeLog();
        check(loaded && up == 3 && Contains(log, "part.txt:1: hold 3 up") && Contains(log, "log deep") &&
                  autoplay.Passed(),
              "include runs another script's commands in place, its path relative to the including file");
        Autoplay bad;
        const bool badLoaded = bad.Load((root / "bad.txt").string(), error);
        check(!badLoaded && Contains(error, "broken.txt:2"), "an error in an included file names it (" + error + ")");
        fs::remove_all(root);
    }

    {
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("clear 30 17 46 2\nwait 1\n", error);
        AutoplayState state;
        int cleared = -1;
        for (int t = 0; t < 60 && cleared < 0; ++t)
        {
            state.tick = t;
            // A fish swimming along +X past (30, 17, 46) at 0.1 a tick: more than 2 beyond it from
            // tick 30.
            state.enemies = {{29.05f + 0.1f * t, 17.0f, 46.0f}};
            autoplay.Step(state);
            if (Contains(autoplay.TakeLog(), "line 2: wait 1"))
            {
                cleared = t;
            }
        }
        check(cleared == 30, "clear waits until no enemy is within R (tick " + std::to_string(cleared) + ")");
    }

    {
        Autoplay autoplay;
        std::string error;
        autoplay.Parse("near 65 19 50 0.12\nwait 1\n", error);
        AutoplayState state;
        int caught = -1;
        for (int t = 0; t < 100 && caught < 0; ++t)
        {
            state.tick = t;
            // A mine bobbing up from y 16 at 0.05 a tick: within 0.12 of y 19 from tick 58.
            state.enemies = {{65.0f, 16.0f + 0.05f * t, 50.0f}};
            autoplay.Step(state);
            if (Contains(autoplay.TakeLog(), "line 2: wait 1"))
            {
                caught = t;
            }
        }
        check(caught == 58, "near waits until an enemy is within R (tick " + std::to_string(caught) + ")");
    }

    {
        Autoplay autoplay;
        std::string error;
        check(autoplay.Parse("clear 50 20 67 12 lane x max 10\n", error), "clear ... lane x parses");
        AutoplayState state;
        // A mine of the next lane (z 69) right beside the point does not count; one in the lane
        // 13 away neither.
        state.enemies = {{50.0f, 20.0f, 69.0f}, {63.0f, 20.0f, 67.0f}};
        autoplay.Step(state);
        check(autoplay.Passed(), "clear ... lane x counts only the enemies in that row");
        Autoplay busy;
        busy.Parse("clear 50 20 67 12 lane x max 10\n", error);
        AutoplayState inLane;
        inLane.enemies = {{55.0f, 20.0f, 67.0f}};
        for (int t = 0; t < 12; ++t)
        {
            inLane.tick = t;
            busy.Step(inLane);
        }
        check(busy.Finished() && !busy.Passed(), "an enemy 5 along the lane keeps it waiting");
    }

    {
        using namespace GalaxyEggbert::Game;
        ResumeLog log;
        log.start.build = "test build";
        log.start.worldPath = "worlds3d/a quoted \"world\".vwr";
        log.start.worldHash = 0xfedcba9876543210ull;
        log.start.seed = 4294967295u;
        log.start.x = 0.123456791f;
        log.start.compass = true;
        log.start.debug = true;
        log.start.cheatsLocked = true;
        log.start.saveData = "selectedGamer=2\nquoted=\"value\"\n";
        ResumeInput input;
        input.keys = {32, 37, 162};
        input.mouseX = -40; input.mouseY = 570; input.wheel = 120; input.buttons = 21;
        input.viewportWidth = 1280; input.viewportHeight = 720;
        input.touches = {{7, 123.45678f, 300.0f, 1}, {8, 700.0f, 250.0f, 3}};
        input.accelValid = true; input.accelY = -0.123456791f;
        input.touchControlsEnabled = true;
        input.cheatLetter = 'e';
        log.Append(input); log.Append(input);
        input.keys.clear(); input.touchControlsEnabled = false; input.cheatLetter = 0; log.Append(input);
        check(log.runs.size() == 2 && log.runs[0].count == 2 && log.TickCount() == 3,
              "resume RLE merges only consecutive identical ticks");
        ResumeLog parsed;
        const auto text = log.Serialize();
        check(ResumeLog::Parse(text, "test build", parsed) && parsed.start == log.start && parsed.runs == log.runs,
              "resume v7 round trip retains world identity, compass, debug, cheat lock and all raw inputs");
        check(text.starts_with("GALAXY_RESUME 7\n"), "resume writes format version 7");
        {
            auto withCheats = log;
            withCheats.start.photo = withCheats.start.freeze = true;
            withCheats.start.xray = withCheats.start.hitbox = true;
            ResumeLog restored;
            check(ResumeLog::Parse(withCheats.Serialize(), "test build", restored) &&
                  restored.start == withCheats.start && restored.runs == withCheats.runs,
                  "optional object/display start metadata round-trips for Continue and demos");
            check(text.find("OBJECT_CHEATS") == std::string::npos &&
                  !parsed.start.photo && !parsed.start.freeze && !parsed.start.xray && !parsed.start.hitbox,
                  "existing v7 logs without object-cheat metadata keep false defaults");
            auto bad = withCheats.Serialize();
            const auto body = ResumeLog::VerifiedBody(bad);
            bad = *body;
            bad.replace(bad.find("OBJECT_CHEATS"),13,"UNKNOWN_FLAGS");
            check(!ResumeLog::Parse(ResumeLog::AppendChecksum(bad), "test build", restored),
                  "unknown start metadata is refused transactionally");
        }

        {
            ResumeLog original = log;
            original.runs = {{5, log.runs[0].input}, {3, log.runs[1].input}, {7, log.runs[0].input}};
            auto trimmed = original;
            check(trimmed.DropLastTicks(0) == 0 && trimmed.runs == original.runs && trimmed.TickCount() == 15,
                  "trimming zero ticks leaves all runs unchanged");
            check(trimmed.DropLastTicks(2) == 2 && trimmed.runs.back().count == 5 && trimmed.TickCount() == 13,
                  "trim inside the last run shortens it");
            check(ResumeLog::Parse(trimmed.Serialize(), "test build", parsed) &&
                  parsed.start == trimmed.start && parsed.runs == trimmed.runs && parsed.TickCount() == 13,
                  "a trimmed non-empty log round trips exactly");
            trimmed = original;
            check(trimmed.DropLastTicks(7) == 7 && trimmed.runs.size() == 2 && trimmed.TickCount() == 8,
                  "trim exactly at a boundary removes the empty run");
            trimmed = original;
            check(trimmed.DropLastTicks(12) == 12 && trimmed.runs.size() == 1 &&
                  trimmed.runs[0].count == 3 && trimmed.TickCount() == 3,
                  "trim across several runs reaches the first run");
            check(trimmed.DropLastTicks(3) == 3 && trimmed.runs.empty() && trimmed.TickCount() == 0,
                  "trim the exact tick count leaves no runs");
            trimmed = original;
            check(trimmed.DropLastTicks(UINT64_MAX) == 15 && trimmed.runs.empty() && trimmed.TickCount() == 0 &&
                  trimmed.DropLastTicks(1) == 0, "trim beyond the log (or an empty log) returns only ticks removed");
            ResumeLog::Parse(text, "test build", parsed);
        }
        check(parsed.runs[0].input.cheatLetter=='e' && parsed.runs[1].input.cheatLetter==0,
              "resume retains ordered cheat letters independently of held gameplay keys");
        check(parsed.runs[0].input.touchControlsEnabled && !parsed.runs[1].input.touchControlsEnabled,
              "resume replays the captured touch capability without querying the current device");
        auto oldBody = text.substr(0, text.rfind("\nEND ") + 1);
        oldBody.replace(0, std::string("GALAXY_RESUME 7").size(), "GALAXY_RESUME 6");
        check(!ResumeLog::Parse(oldBody + "END " + std::to_string(ResumeLog::HashBytes(oldBody)) + "\n",
                               "test build", parsed) && parsed.start == log.start,
              "version 6 without the initial debug flag is refused transactionally");
        check(parsed.MatchesWorld(log.start.worldPath, log.start.worldHash), "same world is accepted");
        check(!parsed.MatchesWorld(log.start.worldPath, log.start.worldHash ^ 1), "changed world hash is refused");
        check(!parsed.MatchesWorld("worlds3d/world045.vwr", log.start.worldHash), "different world path is refused");
        // A complete v1 file with a correct checksum: refusal must be because of its version.
        const std::string v1 = "GALAXY_RESUME 1\n\"test build\"\n1 3 1 0 0 0 0 0 0 0 0 1\n\"\"\n0 0\n";
        check(!ResumeLog::Parse(v1 + "END " + std::to_string(ResumeLog::HashBytes(v1)) + "\n", "test build", parsed)
                  && parsed.start == log.start, "version 1 is refused transactionally even with a valid checksum");
        check(ResumeLog::HashBytes("") == 0xcbf29ce484222325ull &&
              ResumeLog::HashBytes("foobar") == 0x85944171f73967e8ull &&
              ResumeLog::HashBytes(std::string_view("\0", 1)) == 0xaf63bd4c8601b7dfull,
              "FNV-1a matches known vectors including binary NUL");
        const auto hashPath = std::filesystem::path("verify-autoplay-resume-hash.bin");
        std::string binary(65537, '\0');
        binary[0] = '\xff'; binary[65535] = '\x80'; binary.back() = 'x';
        { std::ofstream out(hashPath, std::ios::binary); out.write(binary.data(), binary.size()); }
        check(ResumeLog::HashFile(hashPath) == ResumeLog::HashBytes(binary),
              "streamed file hash includes binary bytes across buffer boundaries");
        std::filesystem::remove(hashPath);
        check(!ResumeLog::HashFile(hashPath), "unreadable file has no hash");
        bool rejected = true;
        for (std::size_t length = 0; length < text.size(); ++length)
            rejected = rejected && !ResumeLog::Parse(text.substr(0, length), "test build", parsed);
        check(rejected && parsed.runs == log.runs, "every truncated resume is refused without changing destination");
        check(!ResumeLog::Parse(text, "another build", parsed), "foreign build resume is refused");
        auto corrupt = text; corrupt[corrupt.find("1280")] = '9';
        check(!ResumeLog::Parse(corrupt, "test build", parsed), "resume checksum detects damaged input");
        check(!ResumeLog::Parse(text + "trailing", "test build", parsed), "resume trailing garbage is refused");
        log.runs[0].count = 0;
        check(!ResumeLog::Parse(log.Serialize(), "test build", parsed), "zero-length run is refused");
        log.runs[0].count = 10000001;
        check(!ResumeLog::Parse(log.Serialize(), "test build", parsed), "unbounded replay is refused");
        log.runs[0].count = 2; log.runs[0].input.keys = {999};
        check(!ResumeLog::Parse(log.Serialize(), "test build", parsed), "out-of-range input is refused");
        ResumeLog empty; empty.start.build = "test build";
        empty.start.worldPath = "worlds3d/world001.vwr";
        check(ResumeLog::Parse(empty.Serialize(), "test build", parsed) && parsed.TickCount() == 0,
              "a mission saved before its first tick can resume");
    }

    {
        Autoplay autoplay;
        std::string error;
        check(autoplay.Parse("pointer 2 400 180 down\npointer 1 70 100\nhold 2 lt\nhold 1 confirm\nend\n",error),
              "pointer and LT/confirm autoplay commands parse");
        AutoplayState state;
        auto a = autoplay.Step(state); ++state.tick;
        auto b = autoplay.Step(state); ++state.tick;
        auto c = autoplay.Step(state); ++state.tick;
        auto d = autoplay.Step(state); ++state.tick;
        auto e = autoplay.Step(state); ++state.tick;
        auto f = autoplay.Step(state);
        check(a.pointer && a.pointerDown && a.pointerX == 400 && a.pointerY == 180 && b.pointerDown &&
              c.pointer && !c.pointerDown && c.pointerX == 70 && c.pointerY == 100 &&
              d.cheatKeyboard && e.cheatKeyboard && f.confirm,
              "pointer coordinates/button, LT and confirm hold exactly their tick counts");
        for (const auto* bad : {"pointer 0 1 2", "pointer 2 1", "pointer 2 1 2 up", "pointer 2 1 2 down extra"})
            check(!autoplay.Parse(bad,error), "malformed pointer command is refused");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
