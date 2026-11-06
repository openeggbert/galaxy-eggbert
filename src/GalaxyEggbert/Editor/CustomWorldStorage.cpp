#include "CustomWorldStorage.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdio>
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

    bool BackUpWorld(const std::filesystem::path& world)
    {
        std::error_code ec;
        if (!std::filesystem::exists(world, ec))
        {
            return false;
        }
        std::filesystem::create_directories(BackupPath(world, 1).parent_path(), ec);
        std::filesystem::remove(BackupPath(world, kBackupGenerations), ec);
        for (int generation = kBackupGenerations - 1; generation >= 1; --generation)
        {
            if (std::filesystem::exists(BackupPath(world, generation), ec))
            {
                std::filesystem::rename(BackupPath(world, generation), BackupPath(world, generation + 1), ec);
            }
        }
        return std::filesystem::copy_file(world, BackupPath(world, 1), std::filesystem::copy_options::overwrite_existing,
                                          ec);
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
}
