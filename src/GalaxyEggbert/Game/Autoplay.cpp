#include "Autoplay.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        constexpr int kDefaultMaxTicks = 1800;
        constexpr int kJumpTicks = 6;
        constexpr float kFaceToleranceDegrees = 1.5f;
        // Farther off course than this he turns on the spot instead of walking.
        constexpr float kWalkConeDegrees = 30.0f;
        constexpr float kSteerDeadZoneDegrees = 2.0f;
        constexpr float kSteerRadius = 0.15f;
        constexpr int kPositionEveryTicks = 60;

        constexpr const char* kPhaseNames[] = {"None",     "First",     "Wait",   "Init",    "Play",
                                               "Pause",    "Lost",      "Win",    "Trial",   "MainSetup",
                                               "PlaySetup", "Resume",   "Ranking", "Editor"};

        const char* PhaseName(Def::GamePhase phase)
        {
            const auto index = static_cast<std::size_t>(Def::ToRaw(phase));
            return index < std::size(kPhaseNames) ? kPhaseNames[index] : "?";
        }

        // -180..180
        float WrapDegrees(float degrees)
        {
            degrees = std::fmod(degrees + 180.0f, 360.0f);
            if (degrees < 0.0f)
            {
                degrees += 360.0f;
            }
            return degrees - 180.0f;
        }

        bool SetKey(AutoplayKeys& keys, const std::string& name)
        {
            if (name == "up") keys.up = true;
            else if (name == "down") keys.down = true;
            else if (name == "left") keys.left = true;
            else if (name == "right") keys.right = true;
            else if (name == "jump") keys.jump = true;
            else if (name == "action") keys.action = true;
            else if (name == "crouch") keys.crouch = true;
            else if (name == "lookup") keys.lookUp = true;
            else return false;
            return true;
        }
    }

    bool Autoplay::Parse(std::string_view text, std::string& error)
    {
        commands_.clear();
        return ParseInto(text, "", std::filesystem::current_path(), 0, error);
    }

    bool Autoplay::Load(const std::string& path, std::string& error)
    {
        commands_.clear();
        std::string text;
        if (!ReadFile(path, text))
        {
            error = "cannot read " + path;
            return false;
        }
        const std::filesystem::path file(path);
        return ParseInto(text, file.filename().string(), file.parent_path(), 0, error);
    }

    bool Autoplay::ReadFile(const std::filesystem::path& path, std::string& text)
    {
        std::ifstream in(path);
        if (!in)
        {
            return false;
        }
        std::stringstream buffer;
        buffer << in.rdbuf();
        text = buffer.str();
        return true;
    }

    bool Autoplay::ParseInto(std::string_view text, const std::string& source, const std::filesystem::path& directory,
                             int depth, std::string& error)
    {
        std::istringstream lines{std::string(text)};
        std::string raw;
        int lineNumber = 0;
        while (std::getline(lines, raw))
        {
            ++lineNumber;
            const auto hash = raw.find('#');
            std::string body = raw.substr(0, hash);
            std::istringstream words(body);
            std::string verb;
            if (!(words >> verb))
            {
                continue;
            }
            Command command;
            command.line = lineNumber;
            command.source = source;
            command.text = body.substr(body.find_first_not_of(" \t"));
            while (!command.text.empty() && (command.text.back() == ' ' || command.text.back() == '\t' ||
                                             command.text.back() == '\r'))
            {
                command.text.pop_back();
            }
            command.ticks = kDefaultMaxTicks;
            const auto fail = [&](const std::string& why) {
                error = command.Where() + ": " + why + ": " + command.text;
                return false;
            };
            const auto readCondition = [&](Condition& condition) {
                std::string field, op, value;
                if (!(words >> field >> op >> value))
                {
                    return false;
                }
                static const std::pair<const char*, Field> kFields[] = {
                    {"x", Field::X},         {"y", Field::Y},         {"z", Field::Z},
                    {"yaw", Field::Yaw},     {"tick", Field::Tick},   {"mission", Field::Mission},
                    {"treasures", Field::Treasures}, {"eggs", Field::Eggs}, {"lives", Field::Lives},
                    {"ground", Field::Ground}, {"dead", Field::Dead}, {"exit", Field::Exit},
                    {"phase", Field::Phase}};
                static const std::pair<const char*, Op> kOps[] = {{"=", Op::Eq},  {"!=", Op::Ne}, {"<", Op::Lt},
                                                                  {"<=", Op::Le}, {">", Op::Gt},  {">=", Op::Ge}};
                const auto fieldIt = std::find_if(std::begin(kFields), std::end(kFields),
                                                  [&](const auto& entry) { return field == entry.first; });
                const auto opIt = std::find_if(std::begin(kOps), std::end(kOps),
                                               [&](const auto& entry) { return op == entry.first; });
                if (fieldIt == std::end(kFields) || opIt == std::end(kOps))
                {
                    return false;
                }
                condition.field = fieldIt->second;
                condition.op = opIt->second;
                if (condition.field == Field::Phase)
                {
                    for (std::size_t i = 0; i < std::size(kPhaseNames); ++i)
                    {
                        if (value == kPhaseNames[i])
                        {
                            condition.value = static_cast<float>(i);
                            return true;
                        }
                    }
                    return false;
                }
                try
                {
                    condition.value = std::stof(value);
                }
                catch (...)
                {
                    return false;
                }
                return true;
            };
            // The optional trailing words of go/until: jump D, tol T, max N, until COND.
            const auto readOptions = [&](bool allowGo) {
                std::string option;
                while (words >> option)
                {
                    if (allowGo && option == "with")
                    {
                        std::string key;
                        if (!(words >> key) || !SetKey(command.keys, key))
                        {
                            return false;
                        }
                        continue;
                    }
                    if (allowGo && option == "until")
                    {
                        command.hasCondition = true;
                        if (!readCondition(command.condition))
                        {
                            return false;
                        }
                        continue;
                    }
                    float value = 0.0f;
                    if (!(words >> value))
                    {
                        return false;
                    }
                    if (option == "max") command.ticks = static_cast<int>(value);
                    else if (allowGo && option == "jump") command.jumpAt = value;
                    else if (allowGo && option == "tol") command.tolerance = value;
                    else return false;
                }
                return true;
            };

            if (verb == "wait" || verb == "hold")
            {
                command.kind = verb == "wait" ? Kind::Wait : Kind::Hold;
                if (!(words >> command.ticks) || command.ticks <= 0)
                {
                    return fail("needs a tick count");
                }
                std::string key;
                while (words >> key)
                {
                    if (command.kind == Kind::Wait || !SetKey(command.keys, key))
                    {
                        return fail("unknown key '" + key + "'");
                    }
                }
            }
            else if (verb == "face")
            {
                command.kind = Kind::Face;
                command.ticks = 400;
                if (!(words >> command.yaw))
                {
                    return fail("needs a yaw in degrees");
                }
            }
            else if (verb == "go")
            {
                command.kind = Kind::Go;
                if (!(words >> command.x >> command.z) || !readOptions(true))
                {
                    return fail("needs X Z [jump D] [tol T] [max N] [until COND] [with KEY]...");
                }
            }
            else if (verb == "lift" || verb == "crate")
            {
                command.kind = verb == "lift" ? Kind::Lift : Kind::Crate;
                if (!(words >> command.x >> command.y >> command.z) || !readOptions(false))
                {
                    return fail("needs X Y Z [max N]");
                }
            }
            else if (verb == "clear" || verb == "near")
            {
                command.kind = verb == "clear" ? Kind::Clear : Kind::Near;
                if (!(words >> command.x >> command.y >> command.z >> command.radius) || !readOptions(false))
                {
                    return fail("needs X Y Z R [max N]");
                }
            }
            else if (verb == "until" || verb == "expect")
            {
                command.kind = verb == "until" ? Kind::Until : Kind::Expect;
                if (!readCondition(command.condition) || (command.kind == Kind::Until && !readOptions(false)))
                {
                    return fail("needs FIELD OP VALUE");
                }
            }
            else if (verb == "trace")
            {
                command.kind = Kind::Trace;
                if (!(words >> command.ticks) || command.ticks <= 0)
                {
                    return fail("needs a tick count");
                }
            }
            else if (verb == "log")
            {
                command.kind = Kind::Log;
            }
            else if (verb == "end")
            {
                command.kind = Kind::End;
            }
            else if (verb == "include")
            {
                // Another script's commands in place of this line, its path relative to this one.
                std::string name;
                std::string included;
                if (!(words >> name))
                {
                    return fail("needs a file");
                }
                const std::filesystem::path path = directory / name;
                if (depth >= 8 || !ReadFile(path, included))
                {
                    return fail(depth >= 8 ? "includes nested too deep" : "cannot read " + path.string());
                }
                if (!ParseInto(included, path.filename().string(), path.parent_path(), depth + 1, error))
                {
                    return false;
                }
                continue;
            }
            else
            {
                return fail("unknown command '" + verb + "'");
            }
            if ((command.kind == Kind::Face || command.kind == Kind::End) && (words >> verb))
            {
                return fail("unexpected '" + verb + "'");
            }
            commands_.push_back(std::move(command));
        }
        return true;
    }

    bool Autoplay::Holds(const Condition& condition, const AutoplayState& state)
    {
        float value = 0.0f;
        switch (condition.field)
        {
        case Field::X: value = state.x; break;
        case Field::Y: value = state.y; break;
        case Field::Z: value = state.z; break;
        case Field::Yaw: value = state.yawDegrees; break;
        case Field::Tick: value = static_cast<float>(state.tick); break;
        case Field::Mission: value = static_cast<float>(state.mission); break;
        case Field::Treasures: value = static_cast<float>(state.treasures); break;
        case Field::Eggs: value = static_cast<float>(state.eggs); break;
        case Field::Lives: value = static_cast<float>(state.lives); break;
        case Field::Ground: value = state.onGround ? 1.0f : 0.0f; break;
        case Field::Dead: value = state.dead ? 1.0f : 0.0f; break;
        case Field::Exit: value = state.exitReached ? 1.0f : 0.0f; break;
        case Field::Phase: value = static_cast<float>(Def::ToRaw(state.phase)); break;
        }
        switch (condition.op)
        {
        case Op::Eq: return value == condition.value;
        case Op::Ne: return value != condition.value;
        case Op::Lt: return value < condition.value;
        case Op::Le: return value <= condition.value;
        case Op::Gt: return value > condition.value;
        case Op::Ge: return value >= condition.value;
        }
        return false;
    }

    void Autoplay::Write(const AutoplayState& state, const std::string& text)
    {
        char prefix[32];
        std::snprintf(prefix, sizeof(prefix), "t=%d ", state.tick);
        log_ += prefix;
        log_ += text;
        log_ += '\n';
    }

    void Autoplay::Fail(const AutoplayState& state, const std::string& text)
    {
        ++failures_;
        finished_ = true;
        Write(state, "FAIL " + text);
    }

    void Autoplay::LogChanges(const AutoplayState& state)
    {
        char line[160];
        if (!havePrevious_ || state.phase != previous_.phase)
        {
            Write(state, std::string("phase ") + PhaseName(state.phase));
        }
        if (!havePrevious_ || state.mission != previous_.mission)
        {
            Write(state, "mission " + std::to_string(state.mission));
        }
        if (havePrevious_ && (state.treasures != previous_.treasures || state.eggs != previous_.eggs ||
                              state.lives != previous_.lives))
        {
            std::snprintf(line, sizeof(line), "treasures %d eggs %d lives %d", state.treasures, state.eggs,
                          state.lives);
            Write(state, line);
        }
        if (!state.crates.empty() && (!havePrevious_ || state.crates != previous_.crates))
        {
            std::string crates = "crates";
            for (const auto& crate : state.crates)
            {
                std::snprintf(line, sizeof(line), " (%.2f %.2f %.2f)", crate[0], crate[1], crate[2]);
                crates += line;
            }
            Write(state, crates);
        }
        if (!havePrevious_ || state.tick % kPositionEveryTicks == 0 || traceTicksLeft_ > 0)
        {
            std::snprintf(line, sizeof(line), "at %.2f %.2f %.2f yaw %.0f%s", state.x, state.y, state.z,
                          state.yawDegrees, state.onGround ? "" : " air");
            Write(state, line);
        }
        traceTicksLeft_ = std::max(0, traceTicksLeft_ - 1);
        previous_ = state;
        havePrevious_ = true;
    }

    AutoplayKeys Autoplay::Step(const AutoplayState& state)
    {
        AutoplayKeys keys;
        if (finished_)
        {
            return keys;
        }
        const bool justDied = state.dead && !(havePrevious_ && previous_.dead);
        LogChanges(state);
        if (justDied)
        {
            char line[96];
            std::snprintf(line, sizeof(line), "died at %.2f %.2f %.2f", state.x, state.y, state.z);
            Fail(state, line);
            return keys;
        }

        while (current_ < commands_.size())
        {
            const Command& command = commands_[current_];
            if (commandStartTick_ < 0)
            {
                commandStartTick_ = state.tick;
                jumped_ = false;
                jumpTicksLeft_ = 0;
                if (command.kind != Kind::Log && command.kind != Kind::Expect && command.kind != Kind::Trace)
                {
                    Write(state, command.Where() + ": " + command.text);
                }
            }
            const int elapsed = state.tick - commandStartTick_;
            bool done = false;
            switch (command.kind)
            {
            case Kind::Wait:
            case Kind::Hold:
                done = elapsed >= command.ticks;
                if (!done)
                {
                    keys = command.keys;
                }
                break;
            case Kind::Face:
            {
                const float error = WrapDegrees(command.yaw - state.yawDegrees);
                done = std::fabs(error) <= kFaceToleranceDegrees;
                keys.right = !done && error > 0.0f;
                keys.left = !done && error < 0.0f;
                break;
            }
            case Kind::Go:
            {
                const float dx = command.x - state.x;
                const float dz = command.z - state.z;
                const float distance = std::sqrt(dx * dx + dz * dz);
                if (command.jumpAt >= 0.0f && !jumped_ && distance <= command.jumpAt)
                {
                    jumped_ = true;
                    jumpTicksLeft_ = kJumpTicks;
                }
                // The jump is played out even when he arrives before it ends.
                done = (distance <= command.tolerance && jumpTicksLeft_ == 0) ||
                       (command.hasCondition && Holds(command.condition, state));
                if (!done)
                {
                    keys = command.keys;
                    constexpr float kDegrees = 57.2957795f;
                    // Right at the point of a `go ... tol 0 ... until` (held against a wall) he
                    // neither turns nor walks: the bearing to it is noise there. With a tolerance
                    // he steers until it is met, as ever.
                    const float steerRadius =
                        command.tolerance > 0.0f ? std::min(kSteerRadius, command.tolerance) : kSteerRadius;
                    if (distance > steerRadius)
                    {
                        const float error = WrapDegrees(std::atan2(dx, -dz) * kDegrees - state.yawDegrees);
                        keys.up = distance > command.tolerance && std::fabs(error) <= kWalkConeDegrees;
                        keys.right = error > kSteerDeadZoneDegrees;
                        keys.left = error < -kSteerDeadZoneDegrees;
                    }
                    if (jumpTicksLeft_ > 0)
                    {
                        keys.jump = true;
                        --jumpTicksLeft_;
                    }
                }
                break;
            }
            case Kind::Until:
                done = Holds(command.condition, state);
                break;
            case Kind::Clear:
            case Kind::Near:
            {
                const bool any = std::any_of(state.enemies.begin(), state.enemies.end(), [&](const std::array<float, 3>& at) {
                    const float dx = at[0] - command.x, dy = at[1] - command.y, dz = at[2] - command.z;
                    return dx * dx + dy * dy + dz * dz < command.radius * command.radius;
                });
                done = command.kind == Kind::Clear ? !any : any;
                break;
            }
            case Kind::Lift:
            case Kind::Crate:
            {
                const auto& where = command.kind == Kind::Lift ? state.lifts : state.crates;
                done = std::any_of(where.begin(), where.end(), [&](const std::array<float, 3>& at) {
                    constexpr float kAt = 0.05f;
                    return std::fabs(at[0] - command.x) <= kAt && std::fabs(at[1] - command.y) <= kAt &&
                           std::fabs(at[2] - command.z) <= kAt;
                });
                break;
            }
            case Kind::Expect:
                if (Holds(command.condition, state))
                {
                    Write(state, "PASS " + command.text);
                }
                else
                {
                    ++failures_;
                    Write(state, "FAIL " + command.text);
                }
                done = true;
                break;
            case Kind::Log:
                Write(state, command.text);
                done = true;
                break;
            case Kind::Trace:
                traceTicksLeft_ = command.ticks;
                done = true;
                break;
            case Kind::End:
                current_ = commands_.size();
                continue;
            }
            if (done)
            {
                ++current_;
                commandStartTick_ = -1;
                continue;
            }
            if (command.kind != Kind::Wait && command.kind != Kind::Hold && elapsed >= command.ticks)
            {
                Fail(state, "timeout after " + std::to_string(elapsed) + " ticks: " + command.Where() + ": " +
                                command.text);
                return AutoplayKeys{};
            }
            return keys;
        }

        finished_ = true;
        Write(state, failures_ == 0 ? "PASS script complete" : "FAIL script complete with " +
                                                                  std::to_string(failures_) + " failure(s)");
        return AutoplayKeys{};
    }

    std::string Autoplay::TakeLog()
    {
        std::string out;
        out.swap(log_);
        return out;
    }
}
