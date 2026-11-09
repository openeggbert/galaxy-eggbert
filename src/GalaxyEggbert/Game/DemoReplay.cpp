#include "DemoReplay.hpp"

#include <algorithm>
#include <cstdio>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        constexpr const char* kMagic = "GALAXY_DEMO";
        constexpr std::size_t kMaxTitle = 200;
        constexpr std::size_t kMaxSignatureEntries = 64;

        bool Contains(const std::vector<int>& sorted, int value)
        {
            return std::binary_search(sorted.begin(), sorted.end(), value);
        }
    }

    std::string DemoPath(int number)
    {
        char path[64];
        std::snprintf(path, sizeof(path), "Content/demos/demo%03d.gdemo", number);
        return path;
    }

    std::string UserDemoDir()
    {
        // The save file's platform path (SaveData::kSavePath): IDBFS's /save on the web.
#if defined(__EMSCRIPTEN__)
        return "/save/demos";
#else
        return "demos";
#endif
    }

    std::string UserDemoPath(int number)
    {
        char name[32];
        std::snprintf(name, sizeof(name), "/demo%03d.gdemo", number);
        return UserDemoDir() + name;
    }

    std::string DemoRecording::Serialize() const
    {
        std::ostringstream out;
        ResumeLog::ImbueStream(out);
        out << kMagic << ' ' << kDemoFormatVersion << '\n' << simulationRevision << '\n'
            << std::quoted(title) << '\n' << std::quoted(log.start.build) << '\n' << endSignature.size() << '\n';
        for (const auto& [name, hash] : endSignature) out << std::quoted(name) << ' ' << hash << '\n';
        log.WriteBody(out);
        return ResumeLog::AppendChecksum(out.str());
    }

    const char* DemoLoadStatusText(DemoLoadStatus status) noexcept
    {
        switch (status)
        {
            case DemoLoadStatus::Ok: return "ok";
            case DemoLoadStatus::Missing: return "missing";
            case DemoLoadStatus::Corrupt: return "damaged or not a demo";
            case DemoLoadStatus::IncompatibleFormat: return "another demo format";
            case DemoLoadStatus::IncompatibleRevision: return "recorded with another simulation revision";
            case DemoLoadStatus::WorldMissing: return "its world is missing";
            case DemoLoadStatus::WorldChanged: return "its world has changed";
        }
        return "unknown";
    }

    DemoLoadStatus ParseDemo(const std::string& text, int expectedRevision, DemoRecording& out)
    {
        const auto body = ResumeLog::VerifiedBody(text);
        if (!body) return DemoLoadStatus::Corrupt;
        std::istringstream in(*body);
        in.imbue(std::locale::classic());
        DemoRecording parsed;
        std::string magic;
        int format = 0;
        if (!(in >> magic >> format) || magic != kMagic) return DemoLoadStatus::Corrupt;
        if (format != kDemoFormatVersion) return DemoLoadStatus::IncompatibleFormat;
        std::size_t entries = 0;
        if (!(in >> parsed.simulationRevision >> std::quoted(parsed.title) >> std::quoted(parsed.log.start.build)
                 >> entries) ||
            parsed.title.size() > kMaxTitle || parsed.log.start.build.size() > 4096 || entries > kMaxSignatureEntries)
            return DemoLoadStatus::Corrupt;
        std::set<std::string> names;
        for (std::size_t i = 0; i < entries; ++i)
        {
            std::pair<std::string, std::uint64_t> entry;
            if (!(in >> std::quoted(entry.first) >> entry.second) || entry.first.empty() ||
                entry.first.size() > 64 || !names.insert(entry.first).second)
                return DemoLoadStatus::Corrupt;
            parsed.endSignature.push_back(std::move(entry));
        }
        if (!ResumeLog::ReadBody(in, parsed.log)) return DemoLoadStatus::Corrupt;
        in >> std::ws;
        if (!in.eof()) return DemoLoadStatus::Corrupt;
        // A demo plays at the normal speed (GameSpeed::Normal) and is short; an empty one shows nothing.
        if (parsed.log.start.speed != 1 || parsed.log.runs.empty() || parsed.log.TickCount() > kDemoMaxTicks)
            return DemoLoadStatus::Corrupt;
        // Checked last: a damaged file is reported as damaged whatever revision it claims.
        if (parsed.simulationRevision != expectedRevision) return DemoLoadStatus::IncompatibleRevision;
        out = std::move(parsed);
        return DemoLoadStatus::Ok;
    }

    DemoLoadStatus DemoWorldStatus(const DemoRecording& demo, const std::string& path,
                                   const std::optional<std::string>& worldBytes)
    {
        if (!worldBytes) return DemoLoadStatus::WorldMissing;
        return demo.log.MatchesWorld(path, ResumeLog::HashBytes(*worldBytes)) ? DemoLoadStatus::Ok
                                                                             : DemoLoadStatus::WorldChanged;
    }

    std::vector<std::pair<std::string, std::uint64_t>> HashDemoSignature(
        const std::map<std::string, std::string>& signature)
    {
        std::vector<std::pair<std::string, std::uint64_t>> result;
        for (const auto& [name, value] : signature) result.emplace_back(name, ResumeLog::HashBytes(value));
        return result;
    }

    std::vector<std::string> DemoSignatureDifferences(
        const std::vector<std::pair<std::string, std::uint64_t>>& expected,
        const std::vector<std::pair<std::string, std::uint64_t>>& actual)
    {
        std::map<std::string, std::uint64_t> want(expected.begin(), expected.end());
        std::map<std::string, std::uint64_t> have(actual.begin(), actual.end());
        std::vector<std::string> different;
        for (const auto& [name, hash] : want)
        {
            const auto it = have.find(name);
            if (it == have.end() || it->second != hash) different.push_back(name);
        }
        for (const auto& [name, hash] : have)
            if (!want.count(name)) different.push_back(name);
        return different;
    }

    std::vector<int> DemoOrderAfter(int current, int count)
    {
        std::vector<int> order;
        if (count <= 0) return order;
        const int start = (current >= 1 && current <= count) ? current : 0;
        for (int i = 1; i <= count; ++i) order.push_back((start + i - 1) % count + 1);
        return order;
    }

    bool DemoActivity(const DemoLiveInput& previous, const DemoLiveInput& now) noexcept
    {
        return !now.keys.empty() || !now.keyEvents.empty() || now.mouseButtons != 0 || now.padButtons != 0 ||
               now.touches > 0 || now.touchPresses > 0 || now.wheel != previous.wheel ||
               now.mouseX != previous.mouseX || now.mouseY != previous.mouseY;
    }

    bool DemoNewPress(const DemoLiveInput& previous, const DemoLiveInput& now) noexcept
    {
        for (int key : now.keys)
            if (!Contains(previous.keys, key)) return true;
        for (int key : now.keyEvents)
            if (!Contains(previous.keys, key)) return true;
        return (now.mouseButtons & ~previous.mouseButtons) != 0 || (now.padButtons & ~previous.padButtons) != 0 ||
               now.wheel != previous.wheel || now.touchPresses > 0 || now.touches > previous.touches;
    }

    bool DemoInputHeld(const DemoLiveInput& now) noexcept
    {
        return !now.keys.empty() || now.mouseButtons != 0 || now.padButtons != 0 || now.touches > 0;
    }

    bool DemoIdleTimer::Update(float elapsedSeconds, bool eligible, bool activity) noexcept
    {
        if (!eligible || activity)
        {
            elapsed_ = 0.0f;
            return false;
        }
        elapsed_ += std::max(0.0f, elapsedSeconds);
        if (elapsed_ < seconds_) return false;
        elapsed_ = 0.0f;
        return true;
    }
}
