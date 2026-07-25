#include "Game/GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Scripted verification that GalaxyEggbert::CNA::GEWorldRuntime's
// LoadFromMobileEggbertFile() correctly parses BigDecor: sections into
// GetBigDecor() (NEXT.md §8 task 3). Mirrors VerifyMoveObjectTypesCna.cpp's
// pattern: each case is a real mobile-eggbert level file confirmed (by
// inspecting ../mobile-eggbert/worlds/*.txt directly) to place a specific
// non-air BigDecor icon at a specific row/col, so this is grounded in real
// level data, not synthetic.
int main()
{
    struct Case { const char* file; int row; int col; int icon; const char* name; };
    static const std::vector<Case> kCases = {
        {"../mobile-eggbert/worlds/world013.txt", 17, 33, 25, "world013 bubble/decor icon 25"},
        {"../mobile-eggbert/worlds/world013.txt", 33, 21, 18, "world013 decor icon 18"},
        {"../mobile-eggbert/worlds/world013.txt", 38, 21, 16, "world013 decor icon 16"},
    };

    bool allOk = true;
    for (const auto& c : kCases)
    {
        GalaxyEggbert::CNA::GEWorldRuntime runtime;
        if (!runtime.LoadFromMobileEggbertFile(c.file))
        {
            std::cout << "FAIL: could not load " << c.file << std::endl;
            allOk = false;
            continue;
        }

        const auto& bigDecor = runtime.GetBigDecor();
        constexpr int kGridSize = 100;
        bool ok = false;
        if (bigDecor.size() == static_cast<std::size_t>(kGridSize) * kGridSize)
        {
            const std::uint16_t actual = bigDecor[
                static_cast<std::size_t>(c.row) * kGridSize + static_cast<std::size_t>(c.col)];
            ok = (actual == static_cast<std::uint16_t>(c.icon));
        }

        std::cout << (ok ? "PASS" : "FAIL") << ": icon=" << c.icon << " (" << c.name
                  << ") at row=" << c.row << " col=" << c.col << " from " << c.file << std::endl;
        if (!ok)
        {
            allOk = false;
        }
    }

    // EDITOR-125: .vwr spawn cells are raw-grid world metadata. The
    // runtime exposes the X/Z-shifted render position and keeps Y intact.
    {
        constexpr const char* path = "spawn_runtime_test.vwr";
        GalaxyEggbert::Worlds::World world;
        world.setSpawnPoint(62, 7, 31);
        world.saveToFile(path);

        GalaxyEggbert::CNA::GEWorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile(path);
        const bool spawnMatches =
            loaded && runtime.HasExplicitSpawnPoint() &&
            runtime.GetSpawnRenderX() == 12.0f &&
            runtime.GetSpawnRenderY() == 7.0f &&
            runtime.GetSpawnRenderZ() == -19.0f;
        std::cout << (spawnMatches ? "PASS" : "FAIL")
                  << ": .vwr spawn converts from raw grid to runtime render space"
                  << std::endl;
        if (!spawnMatches)
        {
            allOk = false;
        }
        std::remove(path);
    }

    // Sanity check: the default .vwr world source has no BigDecor concept
    // (LoadFromVwrFile() clears bigDecor_ to empty) -- confirms the
    // 2026-07-09 segfault fix (guarding the grid-size check before scanning)
    // by exercising the empty-grid path explicitly. world999.vwr (renamed
    // from world001.vwr 2026-07-17) is this engine's quarantined mechanics
    // -showcase/test world.
    {
        GalaxyEggbert::CNA::GEWorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile("worlds3d/world999.vwr");
        const bool emptyAsExpected = loaded && runtime.GetBigDecor().empty();
        std::cout << (emptyAsExpected ? "PASS" : "FAIL")
                  << ": .vwr world source has empty BigDecor grid as expected" << std::endl;
        if (!emptyAsExpected)
        {
            allOk = false;
        }
    }

    // Malformed-cell regression (found via a fresh code audit, 2026-07-23):
    // std::stoi() used to run directly on a Decor:/BigDecor: cell with no
    // try/catch anywhere in LoadFromMobileEggbertFile() or its callers --
    // a non-numeric cell threw std::invalid_argument, uncaught, which
    // would terminate the process. Not reachable via any real gameplay
    // path today (this function's only real callers, all in tools/, pass
    // known-good ../mobile-eggbert reference files) but it stays a real,
    // callable API (see this function's own comment) -- a genuinely
    // malformed file, hand-edited or from a future caller, is a real
    // input here. Writes a synthetic file with a non-numeric Decor: cell
    // directly (not a ../mobile-eggbert file -- that tree is read-only)
    // and confirms Load() completes without crashing.
    {
        const char* path = "malformed_decor_test.txt";
        {
            std::ofstream out(path);
            out << "blupiPos=0;0\n";
            out << "Decor:\n";
            out << "abc,1,2\n"; // non-numeric first cell -- used to throw uncaught
        }
        GalaxyEggbert::CNA::GEWorldRuntime runtime;
        const bool loadedWithoutCrashing = runtime.LoadFromMobileEggbertFile(path);
        std::cout << (loadedWithoutCrashing ? "PASS" : "FAIL")
                  << ": a malformed (non-numeric) Decor: cell doesn't crash the loader" << std::endl;
        if (!loadedWithoutCrashing)
        {
            allOk = false;
        }
        std::remove(path);
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
