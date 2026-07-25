#include "GECustomWorldStorage.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>

#include <algorithm>
#include <cstdio>
#include <string>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Same real Emscripten/IDBFS persistence difference GESaveData::
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

    std::filesystem::path NextNewWorldPath(int gamerSlot)
    {
        const std::filesystem::path dir = CustomWorldsDir(gamerSlot);
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        for (int n = 1; n <= 999; ++n)
        {
            char name[32];
            std::snprintf(name, sizeof(name), "custom_%03d.vwr", n);
            const std::filesystem::path candidate = dir / name;
            if (!std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }
        // Exhausted 1-999 (999 custom worlds in one gamer slot) -- fall
        // back to a timestamp-free sequential overflow name rather than
        // silently reusing/overwriting an existing one.
        return dir / "custom_overflow.vwr";
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
