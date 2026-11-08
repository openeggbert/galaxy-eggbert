#include <GalaxyEggbert/Game/WorldRuntime.hpp>
#include <GalaxyEggbert/Game/ObjectIcons.hpp>

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// Scripted verification that GalaxyEggbert::Game::WorldRuntime's
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

    {
        const auto palmUv = GalaxyEggbert::Game::GetBigDecorIconUv(16);
        const bool usesExploTile16 =
            palmUv.U0 == 0.6f && palmUv.V0 == 0.1f &&
            palmUv.U1 == 0.7f && palmUv.V1 == 0.2f;
        std::cout << (usesExploTile16 ? "PASS" : "FAIL")
                  << ": Palmtree 16 resolves in the explo.png BigDecor atlas"
                  << std::endl;
        if (!usesExploTile16)
        {
            allOk = false;
        }
    }
    for (const auto& c : kCases)
    {
        GalaxyEggbert::Game::WorldRuntime runtime;
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

        GalaxyEggbert::Game::WorldRuntime runtime;
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

    {
        constexpr const char* path = "big_decor_runtime_test.vwr";
        GalaxyEggbert::Worlds::World world;
        GalaxyEggbert::PlaceBigDecor(world, {87, 62, 7, 31});
        world.saveToFile(path);

        GalaxyEggbert::Game::WorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile(path);
        const auto& cells = runtime.GetBigDecorCells();
        const bool bigDecorMatches =
            loaded && cells.size() == 1 &&
            cells[0].worldX == 12.0f && cells[0].worldY == 7.0f &&
            cells[0].worldZ == -19.0f && cells[0].icon == 87;
        std::cout << (bigDecorMatches ? "PASS" : "FAIL")
                  << ": .vwr BigDecor converts to a 3D runtime billboard"
                  << std::endl;
        if (!bigDecorMatches)
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
        GalaxyEggbert::Game::WorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile("worlds3d/world999.vwr");
        const bool emptyAsExpected = loaded && runtime.GetBigDecor().empty();
        std::cout << (emptyAsExpected ? "PASS" : "FAIL")
                  << ": .vwr world source has empty BigDecor grid as expected" << std::endl;
        if (!emptyAsExpected)
        {
            allOk = false;
        }
    }

    // Q-014, owner 2026-11-05: the BigDecor (but the slime and 203) stand in Blupi's way as boxes
    // fitted to their models; without a measured shape a column of two cells over the anchor. World
    // 041 has the house (26) at (40, 10, 70) among plants (23-25), all solid now.
    {
        GalaxyEggbert::Game::WorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile("worlds3d/world041.vwr");
        int solid = 0;
        for (const auto& decor : runtime.GetBigDecorCells())
        {
            solid += GalaxyEggbert::IsSolidBigDecorIcon(decor.icon) ? 1 : 0;
        }
        const auto columns = runtime.SolidBigDecorBoxes();
        const auto holds = [](const std::vector<GalaxyEggbert::Game::SolidBox>& boxes, float x, float y, float z)
        {
            for (const auto& box : boxes)
            {
                if (box.Contains(x, y, z))
                {
                    return true;
                }
            }
            return false;
        };
        const bool columnOk = loaded && solid > 0 && columns.size() == static_cast<std::size_t>(solid) &&
                              holds(columns, 40.0f, 10.5f, 70.0f) && holds(columns, 40.0f, 11.5f, 70.0f) &&
                              !holds(columns, 40.0f, 12.5f, 70.0f) && !holds(columns, 41.0f, 10.5f, 70.0f);
        std::cout << (columnOk ? "PASS" : "FAIL") << ": without a shape each solid BigDecor is a column of two cells ("
                  << solid << " in world 041)" << std::endl;
        allOk = allOk && columnOk;
        // The house measured two blocks square (its model): solid across both neighbours' halves.
        GalaxyEggbert::Game::BigDecorShapes shapes;
        shapes[26] = {{-1.0f, 1.0f, 0.0f, 1.7f, -1.0f, 1.0f}};
        runtime.SetBigDecorShapes(shapes);
        const auto fitted = runtime.SolidBigDecorBoxes();
        const bool fittedOk = holds(fitted, 40.8f, 10.5f, 70.0f) && holds(fitted, 40.0f, 11.6f, 70.8f) &&
                              !holds(fitted, 41.2f, 10.5f, 70.0f) && !holds(fitted, 40.0f, 11.8f, 70.0f);
        std::cout << (fittedOk ? "PASS" : "FAIL")
                  << ": with its model's shape the house is solid over its whole footprint, no higher" << std::endl;
        allOk = allOk && fittedOk;
    }

    // Owner 2026-11-08: the large green columns/vase in the editor gallery must be solid.
    {
        constexpr const char* path = "green_decor_collision_test.vwr";
        GalaxyEggbert::Worlds::World world;
        GalaxyEggbert::Game::BigDecorShapes shapes;
        for (int icon = 66; icon <= 69; ++icon)
        {
            GalaxyEggbert::PlaceBigDecor(world, {static_cast<std::uint16_t>(icon),
                static_cast<std::uint16_t>(40 + 4 * (icon - 66)), 1, 50});
            shapes[icon] = {{-.7f,.7f,0.f,3.5f,-.6f,.6f}};
        }
        world.saveToFile(path);
        GalaxyEggbert::Game::WorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile(path);
        std::remove(path);
        runtime.SetBigDecorShapes(shapes);
        const auto boxes = runtime.SolidBigDecorBoxes();
        bool correct = loaded && boxes.size() == 4;
        for (int i = 0; i < 4; ++i)
        {
            bool inside = false;
            for (const auto& box : boxes) inside |= box.Contains(40.f + 4 * i, 4.f, 50.f);
            correct &= inside;
        }
        std::cout << (correct ? "PASS" : "FAIL")
                  << ": green decor66-69 produces full model-height collision boxes" << std::endl;
        allOk &= correct;
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
        GalaxyEggbert::Game::WorldRuntime runtime;
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
