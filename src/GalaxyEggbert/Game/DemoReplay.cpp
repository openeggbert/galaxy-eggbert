#include "DemoReplay.hpp"
#include "FreeFileName.hpp"
#include "UserFolder.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
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
        // Beside savedata.txt, in the player's folder (UserFolder.hpp): IDBFS's /save on the web.
        return UserPathString("demos");
    }

    std::string UserDemoPath(int number)
    {
        char name[32];
        std::snprintf(name, sizeof(name), "/demo%03d.gdemo", number);
        return UserDemoDir() + name;
    }

    std::string SharedDemoDir()
    {
        return UserDemoDir() + "/shared";
    }

    std::string DemoExportName(int gamer, int slot)
    {
        const std::string player = gamer >= 0 && gamer <= 2 ? std::string(1, static_cast<char>('A' + gamer))
                                                            : "player" + std::to_string(gamer);
        return player + "-demo-" + std::to_string(slot) + ".gdemo";
    }

    std::string ExportDemo(const std::string& source, int gamer, int slot)
    {
        std::error_code error;
        if (!std::filesystem::is_regular_file(source, error)) return {};
        std::filesystem::create_directories(SharedDemoDir(), error);
        // An earlier export stays: A-demo-5.gdemo, A-demo-5-2.gdemo ...
        const std::string name = DemoExportName(gamer, slot);
        const auto target = FreeFileName(SharedDemoDir(), std::filesystem::path(name).stem().string(), ".gdemo");
        if (!std::filesystem::copy_file(source, target, std::filesystem::copy_options::none, error))
            return {};
        return target.generic_string();
    }

    std::vector<std::string> ListSharedDemos()
    {
        std::vector<std::pair<std::filesystem::file_time_type, std::string>> found;
        std::error_code error;
        for (const auto& entry : std::filesystem::directory_iterator(SharedDemoDir(), error))
        {
            if (entry.is_regular_file(error) && entry.path().extension() == ".gdemo")
                found.emplace_back(entry.last_write_time(error), entry.path().generic_string());
        }
        std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        std::vector<std::string> paths;
        for (auto& [time, path] : found) paths.push_back(std::move(path));
        return paths;
    }

    DemoLoadStatus ImportDemo(const std::string& source, int slot)
    {
        std::ifstream in(source, std::ios::binary);
        if (!in) return DemoLoadStatus::Missing;
        const std::string text((std::istreambuf_iterator<char>(in)), {});
        DemoRecording demo;
        const auto status = ParseDemo(text, kDemoSimulationRevision, demo);
        if (status != DemoLoadStatus::Ok) return status;
        std::error_code error;
        std::filesystem::create_directories(UserDemoDir(), error);
        const std::string target = UserDemoPath(slot);
        const std::string temporary = target + ".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            out << text;
            if (!out) return DemoLoadStatus::Missing;
        }
        std::filesystem::rename(temporary, target, error);
        if (error)
        {
            std::filesystem::remove(temporary, error);
            return DemoLoadStatus::Missing;
        }
        return DemoLoadStatus::Ok;
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
