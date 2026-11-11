// Every world shipped in worlds3d/ (and any further directory or .vwr file given on the
// command line) must load through the game's own reader, carry the mission number its file
// name says, give WorldRuntime its objects, and survive load -> save -> load unchanged: the
// native format's round-trip gate (plans/GALAXY_EGGBERT_COMPLETION.md §6). The re-saved copy
// is written to the working directory (the build directory under ctest) and removed again.
//
//   VerifyShippedWorlds <worlds3d dir or .vwr> [...]

#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/Game/BlupiController.hpp"
#include "GalaxyEggbert/Game/WorldRuntime.hpp"
#include "GalaxyEggbert/Game/LegacySawFloors.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    using GalaxyEggbert::Worlds::World;

    // Every sparse metadata record of every chunk, whatever its type (1-8 today, and any added
    // later): the format keeps them in order, so a round trip must give the same list.
    bool SameMetadata(const World& a, const World& b)
    {
        const std::uint8_t n = a.chunksPerAxis();
        for (std::uint8_t z = 0; z < n; ++z)
            for (std::uint8_t y = 0; y < n; ++y)
                for (std::uint8_t x = 0; x < n; ++x)
                    if (a.chunk(x, y, z).extraMetadata() != b.chunk(x, y, z).extraMetadata())
                        return false;
        return true;
    }

    bool SameWorld(const World& a, const World& b)
    {
        if (a.chunksPerAxis() != b.chunksPerAxis() || a.missionNumber() != b.missionNumber() ||
            a.skyRegion() != b.skyRegion() || a.musicTrack() != b.musicTrack() ||
            a.hasLegacySawFloors() != b.hasLegacySawFloors() || a.hasSpawnPoint() != b.hasSpawnPoint() ||
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
        return SameMetadata(a, b);
    }

    // The comparison itself must see a record of a type no code knows (0x1234), and such a record
    // must survive save -> load in a real world. Returns the problem, or "".
    std::string UnknownMetadataProblem(const World& world, const std::filesystem::path& scratch)
    {
        World marked = world;
        marked.setBlockExtraMetadata(1, 2, 3, 0x1234, {0xCA, 0xFE});
        if (SameWorld(world, marked))
            return "the round-trip comparison does not see a metadata record of type 0x1234";
        marked.saveToFile(scratch);
        if (!SameWorld(marked, World::loadFromFile(scratch)))
            return "a metadata record of type 0x1234 does not survive save -> load";
        return "";
    }

    // The exhibition worlds (998 gallery, 999 showcase): a switch 384/385 turns the saws of its own
    // row within 20 cells along X (Decor.cpp:7138-7147), so each one needs a saw 378/379 there, or
    // pressing it looks like nothing happens (owner 2026-11-06). Returns the first switch without.
    std::string SwitchWithoutSaw(const World& world)
    {
        const int n = world.blocksPerAxis();
        for (int z = 0; z < n; ++z)
            for (int y = 0; y < n; ++y)
                for (int x = 0; x < n; ++x)
                {
                    const auto type = world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                     static_cast<std::uint16_t>(z)).type();
                    if (type != 384 && type != 385)
                        continue;
                    bool sawInRow = false;
                    for (int sx = std::max(0, x - 20); sx <= std::min(n - 1, x + 20) && !sawInRow; ++sx)
                    {
                        const auto other = world.getBlock(static_cast<std::uint16_t>(sx),
                                                          static_cast<std::uint16_t>(y),
                                                          static_cast<std::uint16_t>(z)).type();
                        sawInRow = other == 378 || other == 379;
                    }
                    if (!sawInRow)
                        return "the switch " + std::to_string(type) + " at (" + std::to_string(x) + ", " +
                               std::to_string(y) + ", " + std::to_string(z) + ") has no saw in its row";
                }
        return {};
    }

    // Pressing each exhibit switch (Blupi walked up to it from the south, his body against it, at its height) must turn the saws of its
    // row: some 378 become 379 or the other way round. Returns the first switch that changed none.
    std::string SwitchThatTurnsNothing(const std::filesystem::path& path)
    {
        // Walked up to it (his body against the block) and walked into it (the switch is passable, he
        // stands in its cell: the case that did nothing, OR-0078). A fresh runtime for each press: the
        // exhibit switches stand in opposite states, so pressing both in one world could undo itself.
        for (const float offsetZ : {0.5f + GalaxyEggbert::Game::BlupiController::kBodyHalfWidth, 0.0f})
        {
            std::vector<std::array<int, 3>> switches;
            {
                GalaxyEggbert::Game::WorldRuntime runtime;
                if (!runtime.LoadFromVwrFile(path.string()))
                    return "WorldRuntime::LoadFromVwrFile() refused it";
                const World& world = runtime.GetWorld();
                const int n = world.blocksPerAxis();
                for (int z = 0; z < n; ++z)
                    for (int y = 1; y < n; ++y) // y 0: floor switches (stood on) are the south hall's
                        for (int x = 0; x < n; ++x)
                        {
                            const auto type = world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                             static_cast<std::uint16_t>(z)).type();
                            if (type == 384 || type == 385)
                                switches.push_back({x, y, z});
                        }
            }
            for (const auto& [x, y, z] : switches)
            {
                GalaxyEggbert::Game::WorldRuntime runtime;
                if (!runtime.LoadFromVwrFile(path.string()))
                    return "WorldRuntime::LoadFromVwrFile() refused it";
                const World& world = runtime.GetWorld();
                const int n = world.blocksPerAxis();
                int sawY=y;
                if (const auto* row=world.findBlockExtraMetadata(x,y,z,GalaxyEggbert::Game::kLegacySwitchSawRowMetadataType);
                    row && *row==std::vector<std::uint8_t>{1}) ++sawY;
                const auto turning = [&]
                {
                    int count = 0;
                    for (int sx = 0; sx < n; ++sx)
                        count += world.getBlock(static_cast<std::uint16_t>(sx), static_cast<std::uint16_t>(sawY),
                                                static_cast<std::uint16_t>(z)).type() == 378;
                    return count;
                };
                const int before = turning();
                const auto turned = runtime.TryActivateSwitch(
                    static_cast<float>(x - GalaxyEggbert::Game::WorldRuntime::kWorldCenterX),
                    static_cast<float>(y),
                    static_cast<float>(z - GalaxyEggbert::Game::WorldRuntime::kWorldCenterZ) + offsetZ, true);
                if (!turned || turning() == before)
                    return "pressing the switch at (" + std::to_string(x) + ", " + std::to_string(y) + ", " +
                           std::to_string(z) + ") " + (offsetZ == 0.0f ? "standing in it" : "against it") +
                           " turns no saw";
            }
        }
        return {};
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
            if (problem.empty() && (expected == 998 || expected == 999))
            {
                problem = SwitchWithoutSaw(world);
                if (problem.empty())
                    problem = SwitchThatTurnsNothing(path);
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
            if (problem.empty() && path == worlds.front())
            {
                problem = UnknownMetadataProblem(world, scratch);
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
