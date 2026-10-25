// The terrain's tile coverage (src/GalaxyEggbert/Game/TileCoverage.hpp): which object-m.png
// tiles are drawn opaque, cut out (alpha test) or blended, read from the sheet's alpha.
#include <GalaxyEggbert/Game/TileCoverage.hpp>

#include <cstdint>
#include <iostream>
#include <vector>

namespace
{
    using GalaxyEggbert::Game::TileCoverage;

    constexpr int kTile = GalaxyEggbert::BlockTypes::kTileSize;
    constexpr int kGap = GalaxyEggbert::BlockTypes::kSheetGap;
    constexpr int kPitch = kTile + kGap;

    // A sheet of `columns` x `rows` icons laid out as object-m.png, every texel opaque, the gaps
    // clear.
    struct Sheet
    {
        int width;
        int height;
        std::vector<std::uint8_t> alpha;

        Sheet(int columns, int rows)
            : width(kGap + columns * kPitch), height(kGap + rows * kPitch),
              alpha(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0)
        {
            for (int icon = 0; icon < columns * rows; ++icon)
            {
                Fill(icon, columns, 0, kTile, 255);
            }
        }

        // Sets the alpha of icon's texel rows [rowFrom, rowTo).
        void Fill(int icon, int columns, int rowFrom, int rowTo, std::uint8_t value)
        {
            const int x0 = kGap + (icon % columns) * kPitch;
            const int y0 = kGap + (icon / columns) * kPitch;
            for (int y = y0 + rowFrom; y < y0 + rowTo; ++y)
            {
                for (int x = x0; x < x0 + kTile; ++x)
                {
                    alpha[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)] =
                        value;
                }
            }
        }
    };
}

int main()
{
    using GalaxyEggbert::Game::ClassifyTileSheet;
    using GalaxyEggbert::Game::CoverageOf;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* what)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << what << std::endl;
        allOk = allOk && condition;
    };

    constexpr int kColumns = 3;
    Sheet sheet(kColumns, 2);
    sheet.Fill(1, kColumns, 0, 13, 0);              // ~20 % clear: a stone wall's gaps
    sheet.Fill(2, kColumns, 0, 20, 0);              // ~31 % clear: a door on clear texels
    sheet.Fill(3, kColumns, 0, 64, 0);              // a cobweb: clear, its strands faint
    sheet.Fill(3, kColumns, 0, 10, 60);
    sheet.Fill(3, kColumns, 10, 12, 255);
    sheet.Fill(4, kColumns, 0, 40, 0);              // a pillar: faint edges round a solid shape
    sheet.Fill(4, kColumns, 40, 44, 60);
    sheet.Fill(5, kColumns, 0, 64, 127);            // all just under the reference: clear
    const auto coverage = ClassifyTileSheet(sheet.alpha.data(), sheet.width, sheet.height);

    check(coverage.size() == 6, "Every icon of the sheet is classified");
    check(CoverageOf(coverage, 0) == TileCoverage::Opaque, "A fully opaque tile stays opaque");
    check(CoverageOf(coverage, 1) == TileCoverage::Opaque,
          "Under a quarter clear stays opaque (a wall's gaps read as mortar, never as holes)");
    check(CoverageOf(coverage, 2) == TileCoverage::Cutout, "A sprite on clear texels is cut out");
    check(CoverageOf(coverage, 3) == TileCoverage::Translucent,
          "A tile mostly of faint texels is blended (a cut would lose it)");
    check(CoverageOf(coverage, 4) == TileCoverage::Cutout, "Faint edges round a solid sprite are still a cutout");
    check(CoverageOf(coverage, 5) == TileCoverage::Translucent,
          "Alpha below the reference counts as clear, and faint");
    check(CoverageOf(coverage, -1) == TileCoverage::Opaque && CoverageOf(coverage, 6) == TileCoverage::Opaque &&
              CoverageOf({}, 2) == TileCoverage::Opaque,
          "An icon the table does not have is opaque");
    check(ClassifyTileSheet(nullptr, 10, 10).empty(), "No sheet, no table");

    // The real sheet's layout: 1301 x 1431 texels hold 20 x 22 icons.
    const std::vector<std::uint8_t> real(1301u * 1431u, 255);
    check(ClassifyTileSheet(real.data(), 1301, 1431).size() == 440, "object-m.png's 1301 x 1431 hold 440 icons");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
