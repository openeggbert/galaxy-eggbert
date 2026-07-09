#include "Game/GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <cstdint>
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

    // Sanity check: the default .vwr world source has no BigDecor concept
    // (LoadFromVwrFile() clears bigDecor_ to empty) -- confirms the
    // 2026-07-09 segfault fix (guarding the grid-size check before scanning)
    // by exercising the empty-grid path explicitly.
    {
        GalaxyEggbert::CNA::GEWorldRuntime runtime;
        const bool loaded = runtime.LoadFromVwrFile("worlds3d/world001.vwr");
        const bool emptyAsExpected = loaded && runtime.GetBigDecor().empty();
        std::cout << (emptyAsExpected ? "PASS" : "FAIL")
                  << ": .vwr world source has empty BigDecor grid as expected" << std::endl;
        if (!emptyAsExpected)
        {
            allOk = false;
        }
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
