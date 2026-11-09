#include "CustomWorldStorage.hpp"

#include <GalaxyEggbert/Game/DemoReplay.hpp>

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>

namespace GalaxyEggbert::Editor
{
    namespace
    {
        // Same real Emscripten/IDBFS persistence difference SaveData::
        // kSavePath already documents -- a platform path, not an engine-API
        // difference, so it doesn't fall under CLAUDE.md's "no #ifdef for
        // engine differences" rule.
#if defined(__EMSCRIPTEN__)
        constexpr const char* kCustomWorldsRoot = "/save/customworlds";
#else
        constexpr const char* kCustomWorldsRoot = "customworlds";
#endif
        // A file's bytes; nothing when it cannot be read (defined with the galaxy packs below).
        std::optional<std::string> ReadBytes(const std::filesystem::path& path);
    }

    std::filesystem::path CustomWorldsDir(int gamerSlot)
    {
        return std::filesystem::path(kCustomWorldsRoot) / ("gamer" + std::to_string(gamerSlot));
    }

    std::vector<std::filesystem::path> ListCustomWorlds(int gamerSlot)
    {
        std::vector<std::filesystem::path> worlds;
        const std::filesystem::path dir = CustomWorldsDir(gamerSlot);
        std::error_code ec;
        if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec))
        {
            return worlds;
        }
        for (const auto& entry : std::filesystem::directory_iterator(dir, ec))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".vwr")
            {
                worlds.push_back(entry.path());
            }
        }
        std::sort(worlds.begin(), worlds.end());
        return worlds;
    }

    std::filesystem::path SlotWorldPath(int gamerSlot, int slot)
    {
        char name[32];
        std::snprintf(name, sizeof(name), "custom_%03d.vwr", slot);
        return CustomWorldsDir(gamerSlot) / name;
    }

    std::vector<std::filesystem::path> ListOlderCustomWorlds(int gamerSlot)
    {
        std::vector<std::filesystem::path> older;
        for (const auto& path : ListCustomWorlds(gamerSlot))
        {
            bool isSlot = false;
            for (int slot = 1; slot <= kCustomWorldSlotCount && !isSlot; ++slot)
            {
                isSlot = path.filename() == SlotWorldPath(gamerSlot, slot).filename();
            }
            if (!isSlot)
            {
                older.push_back(path);
            }
        }
        return older;
    }

    std::filesystem::path NextNewWorldPath(int gamerSlot)
    {
        std::error_code ec;
        std::filesystem::create_directories(CustomWorldsDir(gamerSlot), ec);
        for (int slot = 1; slot <= kCustomWorldSlotCount; ++slot)
        {
            const std::filesystem::path candidate = SlotWorldPath(gamerSlot, slot);
            if (!std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }
        // All ten slots are taken: there is no new world to create (never
        // reuse or overwrite one).
        return {};
    }

    std::filesystem::path CopyIntoFreeSlot(int gamerSlot, const std::filesystem::path& source)
    {
        const std::filesystem::path target = NextNewWorldPath(gamerSlot);
        if (target.empty())
        {
            return {};
        }
        std::error_code ec;
        if (!std::filesystem::copy_file(source, target, std::filesystem::copy_options::none, ec))
        {
            return {};
        }
        return target;
    }

    std::filesystem::path BackupPath(const std::filesystem::path& world, int generation)
    {
        return world.parent_path() / "backups" /
               (world.stem().string() + "." + std::to_string(generation) + world.extension().string());
    }

    int BackupCount(const std::filesystem::path& world)
    {
        std::error_code ec;
        int count = 0;
        while (count < kBackupGenerations && std::filesystem::exists(BackupPath(world, count + 1), ec))
        {
            ++count;
        }
        return count;
    }

    namespace
    {
        // Keeps @p world as generation 1 of the backups at path(generation), the older ones one
        // generation further, the oldest dropped.
        template <typename Path>
        bool RotateBackups(const std::filesystem::path& world, const Path& path)
        {
            std::error_code ec;
            if (!std::filesystem::exists(world, ec))
            {
                return false;
            }
            std::filesystem::create_directories(path(1).parent_path(), ec);
            std::filesystem::remove(path(kBackupGenerations), ec);
            for (int generation = kBackupGenerations - 1; generation >= 1; --generation)
            {
                if (std::filesystem::exists(path(generation), ec))
                {
                    std::filesystem::rename(path(generation), path(generation + 1), ec);
                }
            }
            return std::filesystem::copy_file(world, path(1), std::filesystem::copy_options::overwrite_existing, ec);
        }
    }

    bool BackUpWorld(const std::filesystem::path& world)
    {
        return RotateBackups(world, [&](int generation) { return BackupPath(world, generation); });
    }

    bool BackUpWorldTo(const std::filesystem::path& world, const std::filesystem::path& backupDir)
    {
        return RotateBackups(world, [&](int generation) {
            return backupDir / (world.stem().string() + "." + std::to_string(generation) + world.extension().string());
        });
    }

    std::filesystem::path CustomWorldsRoot()
    {
        return kCustomWorldsRoot;
    }

    std::filesystem::path CampaignBackupsDir()
    {
        return CustomWorldsRoot() / "campaign-backups";
    }

    std::filesystem::path CampaignEditsDir()
    {
        return CustomWorldsRoot() / "campaign-edits";
    }

    std::filesystem::path CampaignOriginalsDir()
    {
#if defined(__EMSCRIPTEN__)
        return "worlds3d-shipped";
#else
        return CustomWorldsRoot() / "campaign-originals";
#endif
    }

    bool HasCampaignEdit(const std::filesystem::path& world)
    {
        if (world.filename().empty()) return false;
        const auto edit = ReadBytes(CampaignEditsDir() / world.filename());
        const auto now = edit ? ReadBytes(world) : std::nullopt;
        return edit && now && *edit == *now;
    }

    void KeepCampaignOriginal(const std::filesystem::path& world, const std::filesystem::path& originalsDir)
    {
        std::error_code ec;
        if (originalsDir.empty() || !std::filesystem::exists(world, ec)) return;
        const auto kept = originalsDir / world.filename();
        if (HasCampaignEdit(world) && std::filesystem::exists(kept, ec)) return; // the original before the edit
        std::filesystem::create_directories(originalsDir, ec);
        std::filesystem::copy_file(world, kept, std::filesystem::copy_options::overwrite_existing, ec);
    }

    bool RevertCampaignEdit(const std::filesystem::path& world, const std::filesystem::path& originalsDir)
    {
        std::error_code ec;
        std::filesystem::remove(CampaignEditsDir() / world.filename(), ec);
        const auto kept = originalsDir / world.filename();
        if (originalsDir.empty() || !std::filesystem::exists(kept, ec)) return false;
        const bool restored = std::filesystem::copy_file(kept, world, std::filesystem::copy_options::overwrite_existing, ec);
        if (restored) std::filesystem::remove(kept, ec);
        return restored;
    }

    int ApplyCampaignEdits(const std::filesystem::path& worldsDir, const std::filesystem::path& originalsDir)
    {
        int copied = 0;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(CampaignEditsDir(), ec))
        {
            const std::string name = entry.path().filename().string();
            const bool world = name.size() == 12 && name.rfind("world", 0) == 0 &&
                               name.compare(8, 4, ".vwr") == 0 &&
                               std::all_of(name.begin() + 5, name.begin() + 8, [](char c) { return c >= '0' && c <= '9'; });
            if (!world || !entry.is_regular_file(ec)) continue;
            if (!originalsDir.empty()) KeepCampaignOriginal(worldsDir / name, originalsDir);
            if (std::filesystem::copy_file(entry.path(), worldsDir / name, std::filesystem::copy_options::overwrite_existing, ec))
                ++copied;
        }
        return copied;
    }

    bool RestoreBackup(const std::filesystem::path& world)
    {
        const int count = BackupCount(world);
        if (count == 0)
        {
            return false;
        }
        std::error_code ec;
        const bool present = std::filesystem::exists(world, ec);
        const std::filesystem::path swap = world.parent_path() / "backups" / (world.stem().string() + ".swap.vwr");
        if (present)
        {
            std::filesystem::rename(world, swap, ec);
            if (ec)
            {
                return false;
            }
        }
        std::filesystem::rename(BackupPath(world, 1), world, ec);
        for (int generation = 2; generation <= count; ++generation)
        {
            std::filesystem::rename(BackupPath(world, generation), BackupPath(world, generation - 1), ec);
        }
        if (present)
        {
            std::filesystem::rename(swap, BackupPath(world, count), ec);
        }
        std::filesystem::remove(ThumbnailPath(world), ec);
        return true;
    }

    bool DeleteSlotWorld(const std::filesystem::path& world)
    {
        std::error_code ec;
        (void)BackUpWorld(world);
        std::filesystem::remove(ThumbnailPath(world), ec);
        return std::filesystem::remove(world, ec);
    }

    std::filesystem::path ThumbnailPath(const std::filesystem::path& world)
    {
        std::filesystem::path thumbnail = world;
        thumbnail.replace_extension(".png");
        return thumbnail;
    }

    std::filesystem::path SharedWorldsDir()
    {
        return std::filesystem::path(kCustomWorldsRoot) / "shared";
    }

    std::vector<std::filesystem::path> ListSharedWorlds()
    {
        std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> found;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(SharedWorldsDir(), ec))
        {
            if (entry.is_regular_file(ec) && entry.path().extension() == ".vwr")
            {
                found.emplace_back(entry.last_write_time(ec), entry.path());
            }
        }
        std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        std::vector<std::filesystem::path> worlds;
        for (auto& [time, path] : found)
        {
            worlds.push_back(std::move(path));
        }
        return worlds;
    }

    std::string ExportName(int gamerSlot, int slot)
    {
        const std::string player = gamerSlot >= 0 && gamerSlot <= 2 ? std::string(1, static_cast<char>('A' + gamerSlot))
                                                                    : "player" + std::to_string(gamerSlot);
        return player + "-world-" + std::to_string(slot) + ".vwr";
    }

    std::filesystem::path ExportWorld(int gamerSlot, int slot)
    {
        const std::filesystem::path source = SlotWorldPath(gamerSlot, slot);
        std::error_code ec;
        if (!std::filesystem::exists(source, ec))
        {
            return {};
        }
        std::filesystem::create_directories(SharedWorldsDir(), ec);
        const std::filesystem::path target = SharedWorldsDir() / ExportName(gamerSlot, slot);
        if (!std::filesystem::copy_file(source, target, std::filesystem::copy_options::overwrite_existing, ec))
        {
            return {};
        }
        return target;
    }

    bool SeedShowcaseSlot(int gamerSlot, const std::filesystem::path& showcase)
    {
        const std::filesystem::path dir = CustomWorldsDir(gamerSlot);
        const std::filesystem::path marker = dir / ".demo-seeded";
        std::error_code ec;
        if (std::filesystem::exists(marker, ec) || !std::filesystem::exists(showcase, ec))
        {
            return false;
        }
        std::filesystem::create_directories(dir, ec);
        const std::filesystem::path slotOne = SlotWorldPath(gamerSlot, 1);
        bool copied = false;
        if (!std::filesystem::exists(slotOne, ec))
        {
            copied = std::filesystem::copy_file(showcase, slotOne, std::filesystem::copy_options::none, ec);
        }
        if (std::FILE* file = std::fopen(marker.string().c_str(), "w"))
        {
            std::fputs(copied ? "slot 1 seeded from the showcase world\n" : "slot 1 was taken; not seeded\n", file);
            std::fclose(file);
        }
        return copied;
    }

    namespace
    {
        // FNV-1a over a file's bytes (0 when it cannot be read).
        std::uint64_t FileHash(const std::filesystem::path& path)
        {
            std::FILE* file = std::fopen(path.string().c_str(), "rb");
            if (file == nullptr)
            {
                return 0;
            }
            std::uint64_t hash = 1469598103934665603ULL;
            int c = 0;
            while ((c = std::fgetc(file)) != EOF)
            {
                hash = (hash ^ static_cast<std::uint64_t>(c)) * 1099511628211ULL;
            }
            std::fclose(file);
            return hash;
        }
    }

    bool SeedGallerySlot(int gamerSlot, const std::filesystem::path& gallery)
    {
        std::error_code ec;
        if (!std::filesystem::exists(gallery, ec))
        {
            return false;
        }
        const std::filesystem::path dir = CustomWorldsDir(gamerSlot);
        std::filesystem::create_directories(dir, ec);
        const std::filesystem::path marker = dir / ".gallery-seeded";
        const std::uint64_t shipped = FileHash(gallery);
        int slot = 0;
        unsigned long long copiedHash = 0;
        if (std::FILE* file = std::fopen(marker.string().c_str(), "r"))
        {
            if (std::fscanf(file, "slot %d hash %llu", &slot, &copiedHash) != 2)
            {
                slot = 0;
            }
            std::fclose(file);
            if (slot < 1 || slot > kCustomWorldSlotCount)
            {
                return false; // no free slot when first offered, or edited away
            }
            const std::filesystem::path copy = SlotWorldPath(gamerSlot, slot);
            if (!std::filesystem::exists(copy, ec) || FileHash(copy) != copiedHash || copiedHash == shipped)
            {
                return false; // deleted, edited, or already current
            }
        }
        else
        {
            for (int candidate = 1; candidate <= kCustomWorldSlotCount && slot == 0; ++candidate)
            {
                if (!std::filesystem::exists(SlotWorldPath(gamerSlot, candidate), ec))
                {
                    slot = candidate;
                }
            }
        }
        bool copied = false;
        if (slot != 0)
        {
            copied = std::filesystem::copy_file(gallery, SlotWorldPath(gamerSlot, slot),
                                                std::filesystem::copy_options::overwrite_existing, ec);
        }
        if (std::FILE* file = std::fopen(marker.string().c_str(), "w"))
        {
            std::fprintf(file, "slot %d hash %llu\n", copied ? slot : 0,
                         static_cast<unsigned long long>(copied ? shipped : 0));
            std::fclose(file);
        }
        return copied;
    }

    GalaxyEggbert::Worlds::World CreateEditorStarterWorld()
    {
        using GalaxyEggbert::MoveObjectRecord;
        using GalaxyEggbert::Def::ObjectType;
        using GalaxyEggbert::PlaceMoveObject;
        using GalaxyEggbert::Worlds::Block;

        GalaxyEggbert::Worlds::World world;
        constexpr std::uint16_t kCenter = 50;
        for (std::uint16_t x = kCenter - 1; x <= kCenter + 1; ++x)
        {
            for (std::uint16_t z = kCenter - 1; z <= kCenter + 1; ++z)
            {
                world.setBlock(x, 0, z, Block::make(GalaxyEggbert::BlockTypes::RockPile));
            }
        }

        const auto placeStationary = [&world](GalaxyEggbert::Def::ObjectType type, std::uint16_t x, std::uint16_t y, std::uint16_t z)
        {
            MoveObjectRecord record;
            record.type = type;
            record.posStartX = static_cast<float>(x);
            record.posStartY = static_cast<float>(y);
            record.posStartZ = static_cast<float>(z);
            record.posEndX = record.posStartX;
            record.posEndY = record.posStartY;
            record.posEndZ = record.posStartZ;
            PlaceMoveObject(world, record);
        };
        placeStationary(GalaxyEggbert::Def::ObjectType::ObjectType5, kCenter, 1, kCenter);       // chest in the centre
        placeStationary(GalaxyEggbert::Def::ObjectType::ObjectType7, kCenter - 1, 1, kCenter);   // exit arrow at one edge
        return world;
    }

    std::filesystem::path GalaxyDir(int gamerSlot)
    {
        return CustomWorldsDir(gamerSlot) / "galaxy";
    }

    std::filesystem::path GalaxyWorldPath(int gamerSlot, int mission)
    {
        char name[32];
        std::snprintf(name, sizeof(name), "world%03d.vwr", mission);
        return GalaxyDir(gamerSlot) / name;
    }

    bool IsGalaxyMission(int mission) noexcept
    {
        if (mission == 1 || mission == kGalaxyBonusMission) return true;
        const int world = mission / 10;
        const int place = mission % 10;
        return world >= 1 && world <= kGalaxyWorldCount && place <= kGalaxyMissionsPerWorld;
    }

    std::optional<std::pair<int, int>> ParseGalaxyWorldPath(const std::string& path)
    {
        for (int gamer = 0; gamer < 3; ++gamer)
        {
            const std::string prefix = GalaxyDir(gamer).generic_string() + "/world";
            if (path.size() != prefix.size() + 7 || path.compare(0, prefix.size(), prefix) != 0 ||
                path.compare(path.size() - 4, 4, ".vwr") != 0)
                continue;
            int mission = 0;
            for (std::size_t i = prefix.size(); i < prefix.size() + 3; ++i)
            {
                if (path[i] < '0' || path[i] > '9') return std::nullopt;
                mission = mission * 10 + (path[i] - '0');
            }
            if (!IsGalaxyMission(mission)) return std::nullopt;
            return std::make_pair(gamer, mission);
        }
        return std::nullopt;
    }

    std::string GalaxyMissionName(int mission)
    {
        if (mission == 1) return "Main hub";
        if (mission == kGalaxyBonusMission) return "Bonus world";
        const std::string world = "World " + std::to_string(mission / 10);
        return mission % 10 == 0 ? world : world + ", mission " + std::to_string(mission % 10);
    }

    GalaxyEggbert::Worlds::World CreateGalaxyStarterWorld(int mission)
    {
        using GalaxyEggbert::Worlds::Block;
        if (mission != 1 && mission % 10 != 0)
        {
            auto world = CreateEditorStarterWorld(); // a mission: the chest and the exit
            world.setSpawnPoint(51, 1, 50);
            world.setMissionNumber(static_cast<std::uint32_t>(mission));
            return world;
        }
        GalaxyEggbert::Worlds::World world;
        for (std::uint16_t x = 49; x <= 51; ++x)
            for (std::uint16_t z = 49; z <= 51; ++z)
                world.setBlock(x, 0, z, Block::make(GalaxyEggbert::BlockTypes::RockPile));
        // Blupi starts facing -Z (no player start object: yaw 0) with the sign two cells ahead, so
        // walking forward takes him there; a world hub's way back is beside him.
        if (mission == 1)
        {
            world.setBlock(50, 1, 49, Block::make(GalaxyEggbert::BlockTypes::WorldSelect1));
        }
        else
        {
            world.setBlock(50, 1, 49, Block::make(GalaxyEggbert::BlockTypes::MissionSign1));
            world.setBlock(49, 1, 51, Block::make(GalaxyEggbert::BlockTypes::ReturnSign));
        }
        world.setSpawnPoint(50, 1, 51);
        world.setMissionNumber(static_cast<std::uint32_t>(mission));
        return world;
    }

    namespace
    {
        constexpr const char* kGalaxyPackHeader = "GALAXY_PACK 1\n";
        constexpr std::size_t kGalaxyPackMaxWorld = 64u * 1024u * 1024u;

        std::uint64_t BytesHash(const std::string& bytes)
        {
            std::uint64_t hash = 1469598103934665603ULL;
            for (const unsigned char c : bytes) hash = (hash ^ c) * 1099511628211ULL;
            return hash;
        }

        std::string HashText(std::uint64_t hash)
        {
            char text[17];
            std::snprintf(text, sizeof(text), "%016llx", static_cast<unsigned long long>(hash));
            return text;
        }

        std::optional<std::string> ReadBytes(const std::filesystem::path& path)
        {
            std::ifstream in(path, std::ios::binary);
            if (!in) return std::nullopt;
            std::string bytes((std::istreambuf_iterator<char>(in)), {});
            if (in.bad()) return std::nullopt;
            return bytes;
        }
    }

    std::vector<int> GalaxyRecordings(int gamerSlot)
    {
        using namespace GalaxyEggbert::Game;
        std::vector<int> positions;
        for (int position = 1; position <= kDemoSlotCount; ++position)
        {
            const auto text = ReadBytes(UserDemoPath(position));
            DemoRecording demo;
            if (!text || ParseDemo(*text, kDemoSimulationRevision, demo) != DemoLoadStatus::Ok) continue;
            const auto galaxy = ParseGalaxyWorldPath(demo.log.start.worldPath);
            if (galaxy && galaxy->first == gamerSlot) positions.push_back(position);
        }
        return positions;
    }

    std::string PackGalaxy(int gamerSlot)
    {
        std::string body = kGalaxyPackHeader;
        int places = 0;
        for (int mission = 1; mission < 200; ++mission)
        {
            if (!IsGalaxyMission(mission)) continue;
            const auto bytes = ReadBytes(GalaxyWorldPath(gamerSlot, mission));
            if (!bytes) continue;
            body += "world " + std::to_string(mission) + " " + std::to_string(bytes->size()) + "\n";
            body += *bytes;
            ++places;
        }
        if (places == 0) return {};
        for (const int position : GalaxyRecordings(gamerSlot))
        {
            const auto bytes = ReadBytes(GalaxyEggbert::Game::UserDemoPath(position));
            if (!bytes) continue;
            body += "demo " + std::to_string(position) + " " + std::to_string(bytes->size()) + "\n";
            body += *bytes;
        }
        return body + "END " + HashText(BytesHash(body)) + "\n";
    }

    std::optional<GalaxyPack> UnpackGalaxy(const std::string& pack)
    {
        const std::string header = kGalaxyPackHeader;
        // The footer: "END " + 16 hex digits + "\n", over everything before it.
        constexpr std::size_t kFooter = 4 + 16 + 1;
        if (pack.size() < header.size() + kFooter || pack.compare(0, header.size(), header) != 0) return std::nullopt;
        const std::size_t bodyEnd = pack.size() - kFooter;
        const std::string body = pack.substr(0, bodyEnd);
        if (pack.compare(bodyEnd, 4, "END ") != 0 || pack.back() != '\n' ||
            pack.substr(bodyEnd + 4, 16) != HashText(BytesHash(body)))
            return std::nullopt;
        GalaxyPack unpacked;
        std::set<int> demoPositions;
        std::size_t at = header.size();
        while (at < bodyEnd)
        {
            const std::size_t lineEnd = body.find('\n', at);
            if (lineEnd == std::string::npos) return std::nullopt;
            std::istringstream line(body.substr(at, lineEnd - at));
            std::string word;
            int number = 0;
            std::size_t size = 0;
            std::string rest;
            if (!(line >> word >> number >> size) || (word != "world" && word != "demo") || (line >> rest) ||
                size > kGalaxyPackMaxWorld || size > bodyEnd - (lineEnd + 1))
                return std::nullopt;
            std::string bytes = body.substr(lineEnd + 1, size);
            at = lineEnd + 1 + size;
            if (word == "demo")
            {
                using namespace GalaxyEggbert::Game;
                if (number < 1 || number > kDemoSlotCount || !demoPositions.insert(number).second) return std::nullopt;
                DemoRecording demo;
                if (ParseDemo(bytes, kDemoSimulationRevision, demo) == DemoLoadStatus::Ok)
                    unpacked.demos.emplace(number, std::move(bytes));
                continue;
            }
            if (!IsGalaxyMission(number) || unpacked.worlds.count(number) != 0) return std::nullopt;
            try
            {
                std::istringstream stream(bytes, std::ios::in | std::ios::binary);
                (void)GalaxyEggbert::Worlds::World::loadFromStream(stream);
            }
            catch (const std::exception&)
            {
                return std::nullopt;
            }
            unpacked.worlds.emplace(number, std::move(bytes));
        }
        if (unpacked.worlds.empty()) return std::nullopt;
        return unpacked;
    }

    std::string GalaxyExportName(int gamerSlot)
    {
        const std::string player = gamerSlot >= 0 && gamerSlot <= 2 ? std::string(1, static_cast<char>('A' + gamerSlot))
                                                                    : "player" + std::to_string(gamerSlot);
        return player + "-galaxy" + kGalaxyPackExtension;
    }

    std::filesystem::path ExportGalaxy(int gamerSlot)
    {
        const std::string pack = PackGalaxy(gamerSlot);
        if (pack.empty()) return {};
        std::error_code ec;
        std::filesystem::create_directories(SharedWorldsDir(), ec);
        const auto target = SharedWorldsDir() / GalaxyExportName(gamerSlot);
        const auto temporary = std::filesystem::path(target.string() + ".tmp");
        {
            std::ofstream out(temporary, std::ios::binary | std::ios::trunc);
            out << pack;
            if (!out) return {};
        }
        std::filesystem::rename(temporary, target, ec);
        if (ec)
        {
            std::filesystem::remove(temporary, ec);
            return {};
        }
        return target;
    }

    std::vector<std::filesystem::path> ListSharedGalaxies()
    {
        std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> found;
        std::error_code ec;
        for (const auto& entry : std::filesystem::directory_iterator(SharedWorldsDir(), ec))
        {
            if (entry.is_regular_file(ec) && entry.path().extension() == kGalaxyPackExtension)
                found.emplace_back(entry.last_write_time(ec), entry.path());
        }
        std::sort(found.begin(), found.end(), [](const auto& a, const auto& b) {
            return a.first != b.first ? a.first > b.first : a.second < b.second;
        });
        std::vector<std::filesystem::path> paths;
        for (auto& [time, path] : found) paths.push_back(std::move(path));
        return paths;
    }

    int ImportGalaxy(int gamerSlot, const std::filesystem::path& pack, int* demos)
    {
        if (demos != nullptr) *demos = 0;
        const auto bytes = ReadBytes(pack);
        const auto unpacked = bytes ? UnpackGalaxy(*bytes) : std::nullopt;
        if (!unpacked) return -1;
        const auto& worlds = unpacked->worlds;
        std::error_code ec;
        for (int mission = 1; mission < 200; ++mission)
        {
            const auto path = GalaxyWorldPath(gamerSlot, mission);
            if (IsGalaxyMission(mission) && std::filesystem::exists(path, ec)) (void)DeleteSlotWorld(path);
        }
        std::filesystem::create_directories(GalaxyDir(gamerSlot), ec);
        int written = 0;
        for (const auto& [mission, world] : worlds)
        {
            std::ofstream out(GalaxyWorldPath(gamerSlot, mission), std::ios::binary | std::ios::trunc);
            out << world;
            if (out) ++written;
        }
        // Its demos among the shared ones, to be placed from the demo positions' Import.
        if (!unpacked->demos.empty()) std::filesystem::create_directories(GalaxyEggbert::Game::SharedDemoDir(), ec);
        for (const auto& [position, demo] : unpacked->demos)
        {
            const auto target = std::filesystem::path(GalaxyEggbert::Game::SharedDemoDir()) /
                                (pack.stem().string() + "-demo-" + std::to_string(position) + ".gdemo");
            std::ofstream out(target, std::ios::binary | std::ios::trunc);
            out << demo;
            if (out && demos != nullptr) ++*demos;
        }
        return written;
    }
}
