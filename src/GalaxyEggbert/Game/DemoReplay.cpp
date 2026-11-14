#include "DemoReplay.hpp"
#include "FreeFileName.hpp"
#include "Localization.hpp"
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

        constexpr std::size_t kMaxWorldBytes = 64 * 1024 * 1024;
        constexpr std::string_view kBase64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string EncodeBase64(std::string_view bytes)
        {
            std::string encoded;
            encoded.reserve((bytes.size() + 2) / 3 * 4);
            for (std::size_t i = 0; i < bytes.size(); i += 3)
            {
                const unsigned a = static_cast<unsigned char>(bytes[i]);
                const unsigned b = i + 1 < bytes.size() ? static_cast<unsigned char>(bytes[i + 1]) : 0;
                const unsigned c = i + 2 < bytes.size() ? static_cast<unsigned char>(bytes[i + 2]) : 0;
                encoded += kBase64[a >> 2];
                encoded += kBase64[((a & 3) << 4) | (b >> 4)];
                encoded += i + 1 < bytes.size() ? kBase64[((b & 15) << 2) | (c >> 6)] : '=';
                encoded += i + 2 < bytes.size() ? kBase64[c & 63] : '=';
            }
            return encoded;
        }

        std::optional<std::string> DecodeBase64(std::string_view text, std::size_t length)
        {
            if (length > kMaxWorldBytes || text.size() != (length + 2) / 3 * 4) return std::nullopt;
            std::string bytes;
            bytes.reserve(length);
            for (std::size_t i = 0; i < text.size(); i += 4)
            {
                unsigned value = 0;
                for (std::size_t j = 0; j < 4; ++j)
                {
                    const auto digit = kBase64.find(text[i + j]);
                    if (digit == std::string_view::npos && text[i + j] != '=') return std::nullopt;
                    value = (value << 6) | (digit == std::string_view::npos ? 0 : static_cast<unsigned>(digit));
                }
                bytes += static_cast<char>((value >> 16) & 255);
                if (bytes.size() < length) bytes += static_cast<char>((value >> 8) & 255);
                if (bytes.size() < length) bytes += static_cast<char>(value & 255);
            }
            // Also rejects misplaced padding, nonzero unused bits and noncanonical encodings.
            if (bytes.size() != length || EncodeBase64(bytes) != text) return std::nullopt;
            return bytes;
        }

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

    std::string DemoExportName(int gamer, int slot, const std::string& title)
    {
        if (std::string named = SanitizeDemoName(title); !named.empty() && !IsAutomaticDemoTitle(title))
        {
            std::replace(named.begin(), named.end(), ' ', '-');
            std::replace(named.begin(), named.end(), '\'', '-');
            return named + ".gdemo";
        }
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
        const std::string name = DemoExportName(gamer, slot, DemoTitle(source));
        const auto target = FreeFileName(SharedDemoDir(), std::filesystem::path(name).stem().string(), ".gdemo");
        if (!std::filesystem::copy_file(source, target, std::filesystem::copy_options::none, error))
            return {};
        return target.generic_string();
    }

    std::string SanitizeDemoName(const std::string& name)
    {
        std::string out;
        for (const char c : name)
        {
            const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == ' ' ||
                            c == '-' || c == '\'';
            if (!ok || (c == ' ' && (out.empty() || out.back() == ' '))) continue;
            if (out.size() < kDemoNameMax) out += c;
        }
        while (!out.empty() && out.back() == ' ') out.pop_back();
        return out;
    }

    bool IsAutomaticDemoTitle(const std::string& title) noexcept
    {
        return title.empty() || title.rfind("Recorded by player ", 0) == 0;
    }

    std::string DemoTitle(const std::string& path)
    {
        std::ifstream in(path, std::ios::binary);
        if (!in) return {};
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        DemoRecording demo;
        return ParseDemo(text, kDemoSimulationRevision, demo) == DemoLoadStatus::Ok ? demo.title : std::string();
    }

    bool RenameDemo(const std::string& path, const std::string& name, bool acceptOtherRevision)
    {
        const std::string clean = SanitizeDemoName(name);
        if (clean.empty()) return false;
        std::string text;
        {
            std::ifstream in(path, std::ios::binary);
            if (!in) return false;
            text.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        }
        DemoRecording demo;
        if (ParseDemo(text, kDemoSimulationRevision, demo, acceptOtherRevision) != DemoLoadStatus::Ok) return false;
        // Replace only the quoted title and checksum: keep even format-1 files byte-identical
        // everywhere else (including their original spacing and embedded world bytes).
        std::string body = *ResumeLog::VerifiedBody(text);
        std::istringstream header(body);
        std::string magic, oldTitle;
        int format, revision;
        header >> magic >> format >> revision >> std::ws;
        const auto begin = static_cast<std::size_t>(header.tellg());
        header >> std::quoted(oldTitle);
        const auto end = static_cast<std::size_t>(header.tellg());
        std::ostringstream title;
        title << std::quoted(clean);
        body.replace(begin, end - begin, title.str());
        const std::string temporary = path + ".tmp";
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            out << ResumeLog::AppendChecksum(body);
            if (!out) return false;
        }
        std::error_code error;
        std::filesystem::rename(temporary, path, error);
        if (!error) return true;
        std::filesystem::remove(temporary, error);
        return false;
    }

    std::string RenameDemoFile(const std::string& path, const std::string& name)
    {
        const auto clean = SanitizeDemoName(name);
        if (clean.empty()) return {};
        namespace fs = std::filesystem;
        const fs::path source(path);
        std::string stem = clean;
        std::replace(stem.begin(), stem.end(), ' ', '-');
        const auto wanted = source.parent_path() / (stem + ".gdemo");
        std::error_code error;
        const auto time = fs::last_write_time(source, error);
        if (error) return {};
        if (wanted == source)
        {
            if (!RenameDemo(path, clean, true)) return {};
            fs::last_write_time(source, time, error);
            return path;
        }
        const auto target = FreeFileName(source.parent_path(), stem, ".gdemo");
        if (!fs::copy_file(source, target, fs::copy_options::none, error)) return {};
        if (!RenameDemo(target.generic_string(), clean, true))
        {
            fs::remove(target, error);
            return {};
        }
        if (!fs::remove(source, error))
        {
            fs::remove(target, error);
            return {};
        }
        fs::last_write_time(target, time, error); // naming does not reorder the collection
        return target.generic_string();
    }

    bool DeleteDemoFile(const std::string& path)
    {
        std::error_code error;
        return std::filesystem::remove(path, error);
    }

    std::vector<DemoCollectionEntry> ListDemoCollection(
        const std::function<std::string(DemoRecording&)>& resolveWorld)
    {
        std::vector<DemoCollectionEntry> entries;
        for (const auto& path : ListSharedDemos())
        {
            DemoCollectionEntry entry;
            entry.path = path;
            entry.name = std::filesystem::path(path).filename().string();
            std::ifstream in(path, std::ios::binary);
            DemoRecording demo;
            entry.status = in ? ParseDemo(std::string((std::istreambuf_iterator<char>(in)), {}),
                                         kDemoSimulationRevision, demo, true) : DemoLoadStatus::Missing;
            if (entry.status == DemoLoadStatus::Ok)
            {
                if (!IsAutomaticDemoTitle(demo.title)) entry.name = demo.title;
                entry.olderRevision = demo.simulationRevision != kDemoSimulationRevision;
                entry.galaxy = demo.log.start.worldPath.find("/galaxy/world") != std::string::npos;
                entry.mission = demo.log.start.mission;
                entry.seconds = demo.log.TickCount() / 60;
                entry.metadata = demo.gameVersion;
                // "2026-11-14T07:29:14Z" shown as "2026-11-14 07:29 UTC".
                if (!demo.recordedAt.empty())
                    entry.metadata += (entry.metadata.empty() ? "" : "  ") +
                                      (demo.recordedAt.size() >= 16 && demo.recordedAt[10] == 'T'
                                           ? demo.recordedAt.substr(0, 10) + ' ' + demo.recordedAt.substr(11, 5) + " UTC"
                                           : demo.recordedAt);
                const auto worldPath = resolveWorld(demo);
                std::ifstream world(worldPath, std::ios::binary);
                std::optional<std::string> bytes;
                if (world) bytes = std::string((std::istreambuf_iterator<char>(world)), {});
                entry.status = DemoWorldStatus(demo, worldPath, bytes);
            }
            entries.push_back(std::move(entry));
        }
        return entries;
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
            << std::quoted(title) << '\n' << std::quoted(log.start.build) << '\n'
            << "VERSION " << std::quoted(gameVersion) << '\n'
            << "RECORDED_AT " << std::quoted(recordedAt) << '\n'
            << "WORLD " << worldBytes.has_value();
        if (worldBytes) out << ' ' << worldBytes->size() << ' ' << std::quoted(EncodeBase64(*worldBytes));
        out << '\n' << endSignature.size() << '\n';
        for (const auto& [name, hash] : endSignature) out << std::quoted(name) << ' ' << hash << '\n';
        log.WriteBody(out);
        return ResumeLog::AppendChecksum(out.str());
    }

    DemoLoadStatus ParseDemo(const std::string& text, int expectedRevision, DemoRecording& out, bool acceptOtherRevision)
    {
        const auto body = ResumeLog::VerifiedBody(text);
        if (!body) return DemoLoadStatus::Corrupt;
        std::istringstream in(*body);
        in.imbue(std::locale::classic());
        DemoRecording parsed;
        std::string magic;
        int format = 0;
        if (!(in >> magic >> format) || magic != kMagic) return DemoLoadStatus::Corrupt;
        if (format != 1 && format != kDemoFormatVersion) return DemoLoadStatus::IncompatibleFormat;
        std::size_t entries = 0;
        if (!(in >> parsed.simulationRevision >> std::quoted(parsed.title) >> std::quoted(parsed.log.start.build)) ||
            parsed.title.size() > kMaxTitle || parsed.log.start.build.size() > 4096)
            return DemoLoadStatus::Corrupt;
        if (format == 2)
        {
            std::string tag;
            bool hasWorld = false;
            if (!(in >> tag >> std::quoted(parsed.gameVersion)) || tag != "VERSION" ||
                parsed.gameVersion.size() > 128 || parsed.gameVersion.find_first_of("\r\n\0", 0, 3) != std::string::npos)
                return DemoLoadStatus::Corrupt;
            if (!(in >> tag >> std::quoted(parsed.recordedAt)) || tag != "RECORDED_AT" ||
                parsed.recordedAt.size() > 20 || parsed.recordedAt.find_first_of("\r\n\0", 0, 3) != std::string::npos)
                return DemoLoadStatus::Corrupt;
            if (!(in >> tag >> hasWorld) || tag != "WORLD") return DemoLoadStatus::Corrupt;
            if (hasWorld)
            {
                std::size_t length = 0;
                std::string encoded;
                if (!(in >> length) || length > kMaxWorldBytes || !(in >> std::quoted(encoded)))
                    return DemoLoadStatus::Corrupt;
                parsed.worldBytes = DecodeBase64(encoded, length);
                if (!parsed.worldBytes) return DemoLoadStatus::Corrupt;
            }
        }
        if (!(in >> entries) || entries > kMaxSignatureEntries) return DemoLoadStatus::Corrupt;
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
        if (parsed.worldBytes && ResumeLog::HashBytes(*parsed.worldBytes) != parsed.log.start.worldHash)
            return DemoLoadStatus::Corrupt;
        // Checked last: a damaged file is reported as damaged whatever revision it claims.
        if (!acceptOtherRevision && parsed.simulationRevision != expectedRevision) return DemoLoadStatus::IncompatibleRevision;
        out = std::move(parsed);
        return DemoLoadStatus::Ok;
    }

    DemoLoadStatus DemoWorldStatus(const DemoRecording& demo, const std::string& path,
                                   const std::optional<std::string>& worldBytes)
    {
        if (worldBytes && demo.log.MatchesWorld(path, ResumeLog::HashBytes(*worldBytes))) return DemoLoadStatus::Ok;
        if (demo.worldBytes)
            return ResumeLog::HashBytes(*demo.worldBytes) == demo.log.start.worldHash
                       ? DemoLoadStatus::OwnWorld : DemoLoadStatus::Corrupt;
        return worldBytes ? DemoLoadStatus::WorldChanged : DemoLoadStatus::WorldMissing;
    }

    std::optional<std::string> CacheDemoWorld(const DemoRecording& demo)
    {
        if (!demo.worldBytes || demo.worldBytes->size() > kMaxWorldBytes ||
            ResumeLog::HashBytes(*demo.worldBytes) != demo.log.start.worldHash) return std::nullopt;
        const auto target = std::filesystem::path(UserDemoDir()) / "worlds" /
                            (std::to_string(demo.log.start.worldHash) + ".vwr");
        if (ResumeLog::HashFile(target) == demo.log.start.worldHash) return target.generic_string();
        std::error_code error;
        std::filesystem::create_directories(target.parent_path(), error);
        if (error) return std::nullopt;
        const auto temporary = target.string() + ".tmp";
        {
            std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
            file.write(demo.worldBytes->data(), static_cast<std::streamsize>(demo.worldBytes->size()));
            file.close();
            if (!file)
            {
                std::filesystem::remove(temporary, error);
                return std::nullopt;
            }
        }
        std::filesystem::rename(temporary, target, error);
        if (error)
        {
            std::filesystem::remove(temporary, error);
            return std::nullopt;
        }
        return target.generic_string();
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
