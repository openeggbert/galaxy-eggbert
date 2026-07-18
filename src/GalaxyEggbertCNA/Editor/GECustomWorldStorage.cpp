#include "GECustomWorldStorage.hpp"

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
}
