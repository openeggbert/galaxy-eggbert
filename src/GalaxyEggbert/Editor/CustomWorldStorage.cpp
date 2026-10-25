#include "CustomWorldStorage.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>

#include <algorithm>
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
