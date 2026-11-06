#include "ResumeLog.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        std::uint64_t Checksum(const std::string& text)
        {
            std::uint64_t hash = 14695981039346656037ull;
            for (unsigned char c : text) hash = (hash ^ c) * 1099511628211ull;
            return hash;
        }
        constexpr std::uint64_t kMaxTicks = 10000000;
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

    std::string ResumeLog::Serialize() const
    {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::setprecision(std::numeric_limits<float>::max_digits10);
        out << "GALAXY_RESUME 1\n" << std::quoted(start.build) << '\n';
        out << start.mission << ' ' << start.lives << ' ' << start.speed << ' ' << start.seed << ' '
            << start.x << ' ' << start.y << ' ' << start.z << ' ' << start.yaw << ' '
            << start.quickCheat << ' ' << start.openDoors << ' ' << start.showSecret << ' '
            << start.cameraMode << '\n' << std::quoted(start.saveData) << '\n';
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
            out << ' ' << f.accelValid << ' ' << f.accelReset << ' ' << f.accelY << '\n';
        }
        const auto body = out.str();
        out << "END " << Checksum(body) << '\n';
        return out.str();
    }

    bool ResumeLog::Parse(const std::string& text, const std::string& build, ResumeLog& result)
    {
        if (text.size() > 128 * 1024 * 1024) return false;
        const auto footer = text.rfind("\nEND ");
        if (footer == std::string::npos || text.back() != '\n') return false;
        const auto body = text.substr(0, footer + 1);
        std::istringstream end(text.substr(footer + 5));
        end.imbue(std::locale::classic());
        std::uint64_t checksum;
        if (!(end >> checksum) || checksum != Checksum(body)) return false;
        end >> std::ws;
        if (!end.eof()) return false;
        std::istringstream in(body);
        in.imbue(std::locale::classic());
        ResumeLog parsed;
        auto& s = parsed.start;
        std::string magic;
        int version;
        if (!(in >> magic >> version >> std::quoted(s.build)) || magic != "GALAXY_RESUME" ||
            version != 1 || s.build != build) return false;
        if (!(in >> s.mission >> s.lives >> s.speed >> s.seed >> s.x >> s.y >> s.z >> s.yaw
                 >> s.quickCheat >> s.openDoors >> s.showSecret >> s.cameraMode >> std::quoted(s.saveData)))
            return false;
        if (s.mission < 1 || s.mission > 999 || s.lives < 0 || s.lives > 1000000 ||
            (s.speed != 0 && s.speed != 1 && s.speed != 2 && s.speed != 4 && s.speed != 8) ||
            s.cameraMode < 0 || s.cameraMode > 1 || s.saveData.size() > 16384 ||
            !std::isfinite(s.x) || !std::isfinite(s.y) || !std::isfinite(s.z) || !std::isfinite(s.yaw))
            return false;
        std::uint64_t runCount, ticks, actualTicks = 0;
        if (!(in >> runCount >> ticks) || ticks > kMaxTicks || runCount > ticks) return false;
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
            if (!(in >> f.accelValid >> f.accelReset >> f.accelY) || !std::isfinite(f.accelY)) return false;
            parsed.runs.push_back(std::move(run));
        }
        in >> std::ws;
        if (!in.eof() || actualTicks != ticks) return false;
        result = std::move(parsed);
        return true;
    }
}
