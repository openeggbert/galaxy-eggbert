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
//   hold N KEY...              hold the keys for N ticks: up down left right jump action crouch lookup fire
//                              pause (Escape: Pause and back; 2026-11-11, the network tests)
//   pointer N X Y [down]       pointer at viewport pixels for N ticks (released unless down)
//   hold N lt|confirm          LT mapped key / A-or-Enter keyboard confirmation
//   type WORD                  type lowercase letters, each followed by a release tick (cheats)
//   face DEG                   turn on the spot until facing DEG
//   go X Z [jump D] [tol T] [max N] [until COND] [with KEY]...
//                              turn towards (X, Z) and walk until within T (0.25) of it, or until
//                              COND holds; with `jump D`, jump once on the way when D or less from
//                              it; `with KEY` holds KEY too (crouch dives, lookup swims up)
//   fly X Y Z [tol T] [max N] [until COND] [with KEY]...
//                              a go that also holds the height Y (helicopter, overcraft): lookup
//                              below it, crouch above it, judged half a second ahead; done at the
//                              point with the height within 0.3 (2026-11-04: the helicopter sinks
//                              when let be, BD-0070, so a held climb no longer stays put)
//   until COND [max N]         no keys until COND holds (at most N ticks, default 1800)
//   lift X Y Z [max N]         no keys until a lift would carry him standing at (X, Y, Z): its
//                              platform is there (within 0.05), Y being his feet's height on it
//   crate X Y Z [tol T] [max N] no keys until a crate stands at (X, Y, Z) (within T along X and Z,
//                              default 0.05; crates rest anywhere, not on whole cells)
//   clear X Y Z R [lane x|z] [max N]
//                              no keys until no enemy (fish, mine, bird, ...) is within R of
//                              (X, Y, Z): time a dive past a patrol; with `lane x` only the enemies
//                              in that row along X count (within 0.6 of Y and Z), R along it
//   near X Y Z R [lane x|z] [max N]
//                              no keys until an enemy is within R of (X, Y, Z): catch a patrol at
//                              one end of its run, to go as it turns away
//   expect COND                record a pass or a failure
//   log TEXT                   write TEXT to the trace
//   trace N                    write the position every tick for the next N ticks (else every 60)
//   shots N                    have the game save the frame of each of the next N ticks (the host's
//                              autoplay_shots/, tick-numbered): a moment caught without AUTOPLAY_SHOTS
//   mortal                     from here on a death is written to the trace, not a failure (to look at
//                              the deaths themselves: the lock, the life-loss window, the respawn)
//   wander N [seed S] [until COND]
//                              play at random for N ticks (or until COND holds; no failure when the
//                              ticks run out): every half second or so new keys, walking and turning
//                              mostly, now and then a jump, the action key or a shot (a long network
//                              match's play, Q-022); S picks the sequence (1 by default)
//   hash                       have the game write a hash of its world to its log (the network tests:
//                              every machine's world the same after a long match)
//   say TEXT                   in a network match, send TEXT as a chat line (as typed after Enter)
//   end                        stop here
//   include FILE               FILE's commands here (its path relative to this script's directory)
//
// COND is FIELD OP VALUE, FIELD one of x y z yaw tick mtick mission treasures eggs lives keys dynamite
// ground dead ghost anim (BlupiController::AnimState: Stop=0, March=1) exit (1 once the mission's exit is reached) blitz (the lightning's place in its 100-tick cycle:
// it strikes on the even ticks of 0-49) temp (the vanishing saucers' place in their 20-step cycle:
// gone at 18-19) switches (the switches he has pressed this mission) saws (the saws 378 turning now: a press of a
// switch turns those of its row off or on) crush (the crushers' place in their 10-step cycle: they squash at 0-2) phase, OP one of = != < <= > >=; tick counts from the script's start, mtick from
// the current mission's (a mission's own timing holds when it is played in a chain); phase takes a name
// (Play, Win, Lost, Pause, Init, ...); power (0 none, 1 shield, 2 power, 3 cloud, 4 hidden); in a network
// game netpause (1 while another player's pause holds the game) and netplayers (the other players still
// in the match). A
// command that runs out of ticks, or Blupi dying (before `mortal`), ends the run as a failure.

#include <GalaxyEggbert/Def/GamePhase.hpp>

#include <array>
#include <filesystem>
#include <string>
#include <utility>
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
        bool ghost = false;
        int anim = 0; // BlupiController::AnimState: Stop=0, March=1
        int switchPresses = 0; // the switches he has pressed (384/385 toggled) since the mission began
        int sawsTurning = 0;   // the saws 378 now turning in the world

        bool exitReached = false;
        int blitz = 0;
        int temp = 0;
        int crush = 0;
        Def::GamePhase phase = Def::GamePhase::None;
        int mission = 0;
        int treasures = 0;
        int eggs = 0;
        int lives = 0;
        int keys = 0; // the coloured keys held
        int dynamite = 0; // the stick carried (0 or 1)
        // The vehicle he rides: 0 none, 1 helicopter, 2 jeep, 3 tank, 4 skateboard, 5 overcraft
        // (BlupiController::VehicleMode's order). The trace records each change.
        int vehicle = 0;
        // His secret power: 0 none, 1 shield, 2 power, 3 cloud, 4 hidden (Def::SecretPower).
        int power = 0;
        // The network game: 1 while another player's pause holds this one's game (PK_PAUSE); the
        // other players still in the match (present and not knocked out).
        int netPaused = 0;
        int netPlayers = 0;
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
        bool fire = false; // F: the tank's shot, the helicopter's glue (2026-11-10, the network tests)
        bool pause = false; // Escape: Pause, and back to Play (2026-11-11, the network tests)
        bool cheatKeyboard = false, confirm = false;
        bool pointer = false, pointerDown = false;
        int pointerX = 0, pointerY = 0;
        char letter = 0;
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
        // Whether the frame of the tick just stepped should be saved (`shots N`).
        [[nodiscard]] bool WantsShot() const noexcept { return shotTicksLeft_ > 0; }
        // Whether `hash` asked for the world's hash since the last call (once).
        [[nodiscard]] bool TakeHashRequest() noexcept
        {
            const bool wanted = hashWanted_;
            hashWanted_ = false;
            return wanted;
        }
        // The chat line `say` asked for since the last call (empty: none).
        [[nodiscard]] std::string TakeSayRequest() { return std::exchange(said_, std::string()); }

    private:
        enum class Kind { Wait, Hold, Type, Face, Go, Until, Lift, Crate, Clear, Near, Expect, Log, Trace, Shots, Mortal, Wander, Hash, Say, End };
        enum class Field { X, Y, Z, Yaw, Tick, MissionTick, Mission, Treasures, Eggs, Lives, Keys, Dynamite, Vehicle, Ground, Dead, Ghost, Anim, Exit, Blitz, Temp, Crush, Switches, Saws, Phase, Power, NetPause, NetPlayers };
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
            std::string typedWord;
            int ticks = 0;
            AutoplayKeys keys;
            float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f;
            float jumpAt = -1.0f;
            // `fly X Y Z`: a go that also holds the height Y (the helicopter, the overcraft).
            bool fly = false;
            float radius = 0.0f;
            char lane = 0; // 'x', 'z' or 0 (a sphere)
            float tolerance = 0.25f;
            std::uint32_t seed = 1; // wander
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

        [[nodiscard]] bool Holds(const Condition& condition, const AutoplayState& state) const;
        void Write(const AutoplayState& state, const std::string& text);
        void Fail(const AutoplayState& state, const std::string& text);
        void LogChanges(const AutoplayState& state);

        std::vector<Command> commands_;
        std::size_t current_ = 0;
        int commandStartTick_ = -1;
        int missionStartTick_ = 0; // the tick the current mission was first seen
        int jumpTicksLeft_ = 0;
        int traceTicksLeft_ = 0;
        int shotTicksLeft_ = 0; // counted down as each Step() begins: N steps from the one that set it
        bool hashWanted_ = false;
        std::string said_;
        // wander: its random sequence, the keys held and the tick they change.
        std::uint32_t wanderState_ = 1;
        AutoplayKeys wanderKeys_;
        int wanderChangeTick_ = 0;
        bool jumped_ = false;
        // A go's last position and how many ticks he has not moved from it (pressed against a wall).
        float goLastX_ = 0.0f;
        float goLastZ_ = 0.0f;
        float goLastYaw_ = 0.0f;
        float goLastY_ = 0.0f;
        int goStillTicks_ = 0;
        bool finished_ = false;
        int failures_ = 0;
        bool havePrevious_ = false;
        bool mortal_ = false; // `mortal`: deaths are logged, not failures
        std::vector<std::array<float, 3>> crateRest_; // where each crate last came to rest
        AutoplayState previous_;
        std::string log_;
    };
}
