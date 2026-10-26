#pragma once

// QA autoplay (milestone F, 2026-10-27): a script that plays a mission through, tick by tick, so a
// whole run is reproducible and can be replayed after any change. `GalaxyEggbertCNA --mission N
// --autoplay <script>` feeds the keys this returns to the game instead of the keyboard and writes
// what happened (phase, mission, pickups, lives, deaths, positions) to `autoplay_trace.txt`. Not a
// player-facing feature; engine-independent so `VerifyAutoplay` checks it without CNA.
//
// One command per line ("#" starts a comment). Positions are raw grid coordinates (the ones `.vwr`
// files and `--spawn` use; a cell's centre is at its integer coordinates), yaw in degrees (0 faces
// -Z, 90 +X):
//
//   wait N                     no keys for N ticks (1/60 s each)
//   hold N KEY...              hold the keys for N ticks: up down left right jump action crouch lookup
//   face DEG                   turn on the spot until facing DEG
//   go X Z [jump D] [tol T] [max N] [until COND] [with KEY]...
//                              turn towards (X, Z) and walk until within T (0.25) of it, or until
//                              COND holds; with `jump D`, jump once on the way when D or less from
//                              it; `with KEY` holds KEY too (crouch dives, lookup swims up)
//   until COND [max N]         no keys until COND holds (at most N ticks, default 1800)
//   lift X Y Z [max N]         no keys until a lift would carry him standing at (X, Y, Z): its
//                              platform is there (within 0.05), Y being his feet's height on it
//   crate X Y Z [max N]        no keys until a crate stands in cell (X, Y, Z) (within 0.05)
//   clear X Y Z R [max N]      no keys until no enemy (fish, mine, bird, ...) is within R of
//                              (X, Y, Z): time a dive past a patrol
//   expect COND                record a pass or a failure
//   log TEXT                   write TEXT to the trace
//   trace N                    write the position every tick for the next N ticks (else every 60)
//   end                        stop here
//   include FILE               FILE's commands here (its path relative to this script's directory)
//
// COND is FIELD OP VALUE, FIELD one of x y z yaw tick mission treasures eggs lives ground dead
// exit (1 once the mission's exit is reached) phase, OP one of = != < <= > >=; phase takes a name
// (Play, Win, Lost, Pause, Init, ...). A
// command that runs out of ticks, or Blupi dying, ends the run as a failure.

#include <GalaxyEggbert/Def/GamePhase.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace GalaxyEggbert::Game
{
    struct AutoplayState
    {
        int tick = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        float yawDegrees = 0.0f;
        bool onGround = false;
        bool dead = false;
        bool exitReached = false;
        Def::GamePhase phase = Def::GamePhase::None;
        int mission = 0;
        int treasures = 0;
        int eggs = 0;
        int lives = 0;
        // Every active lift, as where a rider stands on it: raw x, feet y, raw z.
        std::vector<std::array<float, 3>> lifts;
        // Every crate: raw x, y (its cell), raw z. The trace records them whenever one moves.
        std::vector<std::array<float, 3>> crates;
        // Every active enemy and projectile: raw x, y, raw z of its cell centre.
        std::vector<std::array<float, 3>> enemies;
    };

    struct AutoplayKeys
    {
        bool up = false, down = false, left = false, right = false;
        bool jump = false, action = false, crouch = false, lookUp = false;
    };

    class Autoplay
    {
    public:
        // Reads a script; false with `error` naming the line when it does not parse. Parse()'s
        // includes resolve against the current directory, Load()'s against the file's own.
        bool Parse(std::string_view text, std::string& error);
        bool Load(const std::string& path, std::string& error);

        // One tick: the keys to hold now, given the game's state after the previous tick.
        AutoplayKeys Step(const AutoplayState& state);

        [[nodiscard]] bool Finished() const noexcept { return finished_; }
        [[nodiscard]] bool Passed() const noexcept { return finished_ && failures_ == 0; }
        // The trace lines written since the last call.
        std::string TakeLog();

    private:
        enum class Kind { Wait, Hold, Face, Go, Until, Lift, Crate, Clear, Expect, Log, Trace, End };
        enum class Field { X, Y, Z, Yaw, Tick, Mission, Treasures, Eggs, Lives, Ground, Dead, Exit, Phase };
        enum class Op { Eq, Ne, Lt, Le, Gt, Ge };
        struct Condition
        {
            Field field = Field::Tick;
            Op op = Op::Ge;
            float value = 0.0f;
        };
        struct Command
        {
            Kind kind = Kind::Wait;
            int line = 0;
            std::string source;
            std::string text;
            int ticks = 0;
            AutoplayKeys keys;
            float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f;
            float jumpAt = -1.0f;
            float radius = 0.0f;
            float tolerance = 0.25f;
            Condition condition;
            bool hasCondition = false;

            [[nodiscard]] std::string Where() const
            {
                return (source.empty() ? "line " : source + ":") + std::to_string(line);
            }
        };

        static bool ReadFile(const std::filesystem::path& path, std::string& text);
        bool ParseInto(std::string_view text, const std::string& source, const std::filesystem::path& directory,
                       int depth, std::string& error);

        [[nodiscard]] static bool Holds(const Condition& condition, const AutoplayState& state);
        void Write(const AutoplayState& state, const std::string& text);
        void Fail(const AutoplayState& state, const std::string& text);
        void LogChanges(const AutoplayState& state);

        std::vector<Command> commands_;
        std::size_t current_ = 0;
        int commandStartTick_ = -1;
        int jumpTicksLeft_ = 0;
        int traceTicksLeft_ = 0;
        bool jumped_ = false;
        bool finished_ = false;
        int failures_ = 0;
        bool havePrevious_ = false;
        AutoplayState previous_;
        std::string log_;
    };
}
