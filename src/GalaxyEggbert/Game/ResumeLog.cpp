#include "ResumeLog.hpp"
#include "UserFolder.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        constexpr std::uint64_t kMaxTicks = 10000000;
    }

    std::uint64_t ResumeLog::HashBytes(std::string_view bytes)
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (unsigned char c : bytes) hash = (hash ^ c) * 1099511628211ull;
        return hash;
    }

    std::optional<std::uint64_t> ResumeLog::HashFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return std::nullopt;
        std::uint64_t hash = 14695981039346656037ull;
        char buffer[65536];
        while (in.read(buffer, sizeof(buffer)) || in.gcount() != 0)
            for (std::streamsize i = 0; i < in.gcount(); ++i)
                hash = (hash ^ static_cast<unsigned char>(buffer[i])) * 1099511628211ull;
        if (in.bad() || !in.eof()) return std::nullopt;
        return hash;
    }

    bool ResumeLog::MatchesWorld(const std::string& path, std::uint64_t hash) const
    {
        return !path.empty() && start.worldPath == path && start.worldHash == hash;
    }

    void ResumeLog::Append(const ResumeInput& input)
    {
        if (!runs.empty() && runs.back().input == input && runs.back().count < UINT32_MAX)
            ++runs.back().count;
        else
            runs.push_back({1, input});
    }

    std::uint64_t ResumeLog::TickCount() const
    {
        std::uint64_t count = 0;
        for (const auto& run : runs) count += run.count;
        return count;
    }

    std::uint64_t ResumeLog::DropLastTicks(std::uint64_t n)
    {
        std::uint64_t removed = 0;
        while (n != 0 && !runs.empty())
        {
            const auto take = std::min<std::uint64_t>(n, runs.back().count);
            runs.back().count -= static_cast<std::uint32_t>(take);
            removed += take;
            n -= take;
            if (runs.back().count == 0) runs.pop_back();
        }
        return removed;
    }

    void ResumeLog::WriteBody(std::ostream& out) const
    {
        out << std::quoted(start.worldPath) << ' ' << start.worldHash << '\n';
        out << start.mission << ' ' << start.lives << ' ' << start.speed << ' ' << start.seed << ' '
            << start.x << ' ' << start.y << ' ' << start.z << ' ' << start.yaw << ' '
            << start.quickCheat << ' ' << start.openDoors << ' ' << start.showSecret << ' ' << start.compass << ' '
            << start.cheatsLocked << ' ' << start.debug << ' '
            << start.cameraMode << '\n' << std::quoted(start.saveData) << '\n';
        // Optional named start metadata: old v7 resumes and shipped demos retain false defaults.
        // Only non-default values add a record, keeping ordinary recordings byte-compatible.
        if (start.photo || start.freeze) out << "OBJECT_CHEATS " << start.photo << ' ' << start.freeze << '\n';
        if (start.xray || start.hitbox) out << "DISPLAY_CHEATS " << start.xray << ' ' << start.hitbox << '\n';
        if (start.trail || start.ids || start.sounds || start.chunks)
            out << "DIAGNOSTIC_CHEATS " << start.trail << ' ' << start.ids << ' ' << start.sounds << ' ' << start.chunks << '\n';
        if (start.step || start.practice) out << "TIMELINE_CHEATS " << start.step << ' ' << start.practice << '\n';
        if (start.perf || start.cascades) out << "RENDER_CHEATS " << start.perf << ' ' << start.cascades << '\n';
        out << runs.size() << ' ' << TickCount() << '\n';
        for (const auto& run : runs)
        {
            const auto& f = run.input;
            out << run.count << ' ' << f.keys.size();
            for (int key : f.keys) out << ' ' << key;
            out << ' ' << f.mouseX << ' ' << f.mouseY << ' ' << f.wheel << ' ' << f.buttons
                << ' ' << f.viewportWidth << ' ' << f.viewportHeight << ' ' << f.touches.size();
            for (const auto& touch : f.touches)
                out << ' ' << touch.id << ' ' << touch.x << ' ' << touch.y << ' ' << touch.state;
            out << ' ' << f.touchControlsEnabled << ' ' << f.accelValid << ' ' << f.accelReset << ' ' << f.accelY
                << ' ' << f.cheatLetter << '\n';
        }
    }

    void ResumeLog::ImbueStream(std::ios& stream)
    {
        stream.imbue(std::locale::classic());
        stream.precision(std::numeric_limits<float>::max_digits10);
    }

    std::string ResumeLog::Path() { return UserPathString("resume.txt"); }

    std::string ResumeLog::AppendChecksum(const std::string& body)
    {
        return body + "END " + std::to_string(HashBytes(body)) + "\n";
    }

    std::string ResumeLog::Serialize() const
    {
        std::ostringstream out;
        ImbueStream(out);
        out << "GALAXY_RESUME 7\n" << std::quoted(start.build) << '\n';
        WriteBody(out);
        return AppendChecksum(out.str());
    }

    std::optional<std::string> ResumeLog::VerifiedBody(const std::string& text)
    {
        if (text.empty() || text.size() > 128 * 1024 * 1024) return std::nullopt;
        const auto footer = text.rfind("\nEND ");
        if (footer == std::string::npos || text.back() != '\n') return std::nullopt;
        auto body = text.substr(0, footer + 1);
        std::istringstream end(text.substr(footer + 5));
        end.imbue(std::locale::classic());
        std::uint64_t checksum;
        if (!(end >> checksum) || checksum != HashBytes(body)) return std::nullopt;
        end >> std::ws;
        if (!end.eof()) return std::nullopt;
        return body;
    }

    bool ResumeLog::Parse(const std::string& text, const std::string& build, ResumeLog& result)
    {
        const auto body = VerifiedBody(text);
        if (!body) return false;
        std::istringstream in(*body);
        in.imbue(std::locale::classic());
        ResumeLog parsed;
        std::string magic;
        int version;
        if (!(in >> magic >> version >> std::quoted(parsed.start.build)) || magic != "GALAXY_RESUME" ||
            version != 7 || parsed.start.build != build) return false;
        if (!ReadBody(in, parsed)) return false;
        in >> std::ws;
        if (!in.eof()) return false;
        result = std::move(parsed);
        return true;
    }

    bool ResumeLog::ReadBody(std::istream& in, ResumeLog& parsed)
    {
        auto& s = parsed.start;
        if (!(in >> std::quoted(s.worldPath) >> s.worldHash) || s.worldPath.empty() ||
            s.worldPath.size() > 4096 || s.worldPath.find('\0') != std::string::npos) return false;
        if (!(in >> s.mission >> s.lives >> s.speed >> s.seed >> s.x >> s.y >> s.z >> s.yaw
                 >> s.quickCheat >> s.openDoors >> s.showSecret >> s.compass >> s.cheatsLocked >> s.debug
                 >> s.cameraMode >> std::quoted(s.saveData)))
            return false;
        if (s.mission < 1 || s.mission > 999 || s.lives < 0 || s.lives > 1000000 ||
            (s.speed != 0 && s.speed != 1 && s.speed != 2 && s.speed != 4 && s.speed != 8) ||
            s.cameraMode < 0 || s.cameraMode > 1 || s.cheatsLocked < 0 || s.cheatsLocked > 2 || s.saveData.size() > 16384 ||
            !std::isfinite(s.x) || !std::isfinite(s.y) || !std::isfinite(s.z) || !std::isfinite(s.yaw))
            return false;
        s.photo = s.freeze = false;
        in >> std::ws;
        if (in.peek() == 'O')
        {
            std::string tag;
            if (!(in >> tag >> s.photo >> s.freeze) || tag != "OBJECT_CHEATS") return false;
        }
        s.xray = s.hitbox = false;
        s.trail = s.ids = s.sounds = s.chunks = false;
        in >> std::ws;
        if (in.peek() == 'D')
        {
            std::string tag;
            if (!(in >> tag)) return false;
            if (tag == "DISPLAY_CHEATS")
            {
                if (!(in >> s.xray >> s.hitbox)) return false;
                in >> std::ws;
                if (in.peek() == 'D') { if (!(in >> tag)) return false; }
                else tag.clear();
            }
            if (tag == "DIAGNOSTIC_CHEATS")
            {
                if (!(in >> s.trail >> s.ids >> s.sounds >> s.chunks)) return false;
            }
            else if (!tag.empty()) return false;
        }
        s.step = s.practice = false;
        in >> std::ws;
        if (in.peek() == 'T')
        {
            std::string tag;
            if (!(in >> tag >> s.step >> s.practice) || tag != "TIMELINE_CHEATS") return false;
        }
        s.perf = s.cascades = false;
        in >> std::ws;
        if (in.peek() == 'R')
        {
            std::string tag;
            if (!(in >> tag >> s.perf >> s.cascades) || tag != "RENDER_CHEATS") return false;
        }
        std::uint64_t runCount, ticks, actualTicks = 0;
        if (!(in >> runCount >> ticks) || ticks > kMaxTicks || runCount > ticks) return false;
        parsed.runs.clear();
        for (std::uint64_t i = 0; i < runCount; ++i)
        {
            ResumeRun run;
            auto& f = run.input;
            unsigned keys, touches;
            if (!(in >> run.count >> keys) || run.count == 0 || keys > 256) return false;
            actualTicks += run.count;
            if (actualTicks > ticks) return false;
            for (unsigned k = 0; k < keys; ++k)
            {
                int key;
                if (!(in >> key) || key < 0 || key > 255) return false;
                if (!f.keys.empty() && key <= f.keys.back()) return false;
                f.keys.push_back(key);
            }
            if (!(in >> f.mouseX >> f.mouseY >> f.wheel >> f.buttons >> f.viewportWidth
                     >> f.viewportHeight >> touches) || f.buttons > 31 || touches > 32 ||
                f.viewportWidth < 1 || f.viewportHeight < 1 || f.viewportWidth > 65536 || f.viewportHeight > 65536)
                return false;
            for (unsigned t = 0; t < touches; ++t)
            {
                ResumeTouch touch;
                if (!(in >> touch.id >> touch.x >> touch.y >> touch.state) || touch.state < 0 || touch.state > 3 ||
                    !std::isfinite(touch.x) || !std::isfinite(touch.y)) return false;
                f.touches.push_back(touch);
            }
            if (!(in >> f.touchControlsEnabled >> f.accelValid >> f.accelReset >> f.accelY >> f.cheatLetter) ||
                !std::isfinite(f.accelY) || (f.cheatLetter != -1 && f.cheatLetter != 0 &&
                    (f.cheatLetter < 'a' || f.cheatLetter > 'z'))) return false;
            parsed.runs.push_back(std::move(run));
        }
        return actualTicks == ticks;
    }
}
