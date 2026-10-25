// Every world shipped in worlds3d/ (and any further directory or .vwr file given on the
// command line) must load through the game's own reader, carry the mission number its file
// name says, give WorldRuntime its objects, and survive load -> save -> load unchanged: the
// native format's round-trip gate (plans/GALAXY_EGGBERT_COMPLETION.md §6). The re-saved copy
// is written to the working directory (the build directory under ctest) and removed again.
//
//   VerifyShippedWorlds <worlds3d dir or .vwr> [...]

#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/Game/WorldRuntime.hpp"

#include <algorithm>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    using GalaxyEggbert::Worlds::World;

    bool SameMetadata(const World& a, const World& b, std::uint16_t type)
    {
        auto ra = a.collectExtraMetadata(type);
        auto rb = b.collectExtraMetadata(type);
        const auto key = [](const World::BlockExtraMetadataRecord& r)
        {
            return std::tuple(r.z, r.y, r.x, r.payload);
        };
        const auto less = [&](const auto& l, const auto& r) { return key(l) < key(r); };
        std::sort(ra.begin(), ra.end(), less);
        std::sort(rb.begin(), rb.end(), less);
        return std::equal(ra.begin(), ra.end(), rb.begin(), rb.end(),
                          [&](const auto& l, const auto& r) { return key(l) == key(r); });
    }

    bool SameWorld(const World& a, const World& b)
    {
        if (a.missionNumber() != b.missionNumber() || a.skyRegion() != b.skyRegion() ||
            a.hasSpawnPoint() != b.hasSpawnPoint() ||
            (a.hasSpawnPoint() && (a.spawnX() != b.spawnX() || a.spawnY() != b.spawnY() || a.spawnZ() != b.spawnZ())))
        {
            return false;
        }
        const std::uint16_t n = a.blocksPerAxis();
        for (std::uint16_t z = 0; z < n; ++z)
            for (std::uint16_t y = 0; y < n; ++y)
                for (std::uint16_t x = 0; x < n; ++x)
                    if (a.getBlock(x, y, z) != b.getBlock(x, y, z))
                        return false;
        for (std::uint16_t type = 1; type <= 3; ++type)
            if (!SameMetadata(a, b, type))
                return false;
        return true;
    }

    // worldNNN.vwr -> NNN, or -1 for any other name (the title scene, the editor's starter).
    int MissionFromName(const std::filesystem::path& path)
    {
        const std::string stem = path.stem().string();
        if (stem.size() == 8 && stem.rfind("world", 0) == 0 &&
            std::all_of(stem.begin() + 5, stem.end(), [](char c) { return c >= '0' && c <= '9'; }))
        {
            return std::stoi(stem.substr(5));
        }
        return -1;
    }
}

int main(int argc, char** argv)
{
    std::vector<std::filesystem::path> worlds;
    for (int i = 1; i < argc; ++i)
    {
        const std::filesystem::path arg(argv[i]);
        if (std::filesystem::is_directory(arg))
        {
            for (const auto& entry : std::filesystem::directory_iterator(arg))
                if (entry.path().extension() == ".vwr")
                    worlds.push_back(entry.path());
        }
        else
        {
            worlds.push_back(arg);
        }
    }
    std::sort(worlds.begin(), worlds.end());
    if (worlds.empty())
    {
        std::cout << "FAIL: no .vwr worlds given" << std::endl;
        return 1;
    }

    int failures = 0;
    const std::filesystem::path scratch = "VerifyShippedWorlds-roundtrip.vwr";
    for (const auto& path : worlds)
    {
        std::string problem;
        try
        {
            const World world = World::loadFromFile(path);
            const int expected = MissionFromName(path);
            if (expected >= 0 && static_cast<int>(world.missionNumber()) != expected)
            {
                problem = "header mission " + std::to_string(world.missionNumber()) + ", file name says " +
                          std::to_string(expected);
            }
            GalaxyEggbert::Game::WorldRuntime runtime;
            if (problem.empty() && !runtime.LoadFromVwrFile(path.string()))
            {
                problem = "WorldRuntime::LoadFromVwrFile() refused it";
            }
            if (problem.empty())
            {
                world.saveToFile(scratch);
                const World again = World::loadFromFile(scratch);
                if (!SameWorld(world, again))
                {
                    problem = "load -> save -> load changed it";
                }
            }
        }
        catch (const std::exception& e)
        {
            problem = std::string("does not load: ") + e.what();
        }
        if (problem.empty())
        {
            std::cout << "PASS: " << path.filename().string() << std::endl;
        }
        else
        {
            std::cout << "FAIL: " << path.filename().string() << ": " << problem << std::endl;
            ++failures;
        }
    }
    std::error_code ec;
    std::filesystem::remove(scratch, ec);
    std::cout << worlds.size() - static_cast<std::size_t>(failures) << "/" << worlds.size() << " worlds pass"
              << std::endl;
    return failures == 0 ? 0 : 1;
}
