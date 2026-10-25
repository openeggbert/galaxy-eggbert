#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>

#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    // How a terrain tile's own texels cover it (2026-10-26). object-m.png is a sheet of 2D
    // sprites, and many tiles are a shape on clear texels: doors, treasure doors, pillars,
    // girders, bars, balustrades, the flat-plate decor, cobwebs. Drawn opaque as a cube or a
    // plate they showed black wherever the sprite is clear. The terrain renderer sends each
    // non-animated tile to the pass its texels call for.
    enum class TileCoverage : std::uint8_t
    {
        Opaque,      // under a quarter clear: drawn as it is (the gaps of a stone wall read as mortar,
                     // where holes would let the camera see through the wall)
        Cutout,      // a hard-edged sprite: the clear texels are cut away (alpha test), the rest opaque
        Translucent, // mostly faint texels (cobwebs, splashes): a cut would lose them, so alpha-blended
    };

    // Texels with alpha below this are clear: the cutout pass discards them.
    inline constexpr int kCutoutReferenceAlpha = 128;

    struct TileAlphaCounts
    {
        int clear = 0; // alpha < kCutoutReferenceAlpha
        int faint = 0; // 0 < alpha < kCutoutReferenceAlpha (counted in clear as well)
        int solid = 0; // alpha >= kCutoutReferenceAlpha
    };

    [[nodiscard]] inline TileCoverage ClassifyTileCoverage(const TileAlphaCounts& counts) noexcept
    {
        const int total = counts.clear + counts.solid;
        if (total == 0 || counts.clear * 4 < total)
        {
            return TileCoverage::Opaque;
        }
        return counts.faint > counts.solid ? TileCoverage::Translucent : TileCoverage::Cutout;
    }

    // Classifies every icon of a sheet laid out as object-m.png (BlockTypes.hpp: kTileSize
    // texels with a kSheetGap-texel gap before each), from its alpha channel: one byte per
    // texel, row by row, width * height bytes. Icons that do not fit the sheet are left out.
    [[nodiscard]] inline std::vector<TileCoverage> ClassifyTileSheet(const std::uint8_t* alpha, int width, int height)
    {
        std::vector<TileCoverage> coverage;
        if (alpha == nullptr || width <= 0 || height <= 0)
        {
            return coverage;
        }
        constexpr int kTileSize = BlockTypes::kTileSize;
        constexpr int kSheetGap = BlockTypes::kSheetGap;
        constexpr int kPitch = kTileSize + kSheetGap;
        const int columns = width / kTileSize;
        const int rows = height / kTileSize;
        for (int row = 0; row < rows; ++row)
        {
            for (int column = 0; column < columns; ++column)
            {
                const int x0 = kSheetGap + column * kPitch;
                const int y0 = kSheetGap + row * kPitch;
                if (x0 + kTileSize > width || y0 + kTileSize > height)
                {
                    return coverage;
                }
                TileAlphaCounts counts;
                for (int y = y0; y < y0 + kTileSize; ++y)
                {
                    for (int x = x0; x < x0 + kTileSize; ++x)
                    {
                        const int a = alpha[static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                            static_cast<std::size_t>(x)];
                        if (a >= kCutoutReferenceAlpha)
                        {
                            ++counts.solid;
                        }
                        else
                        {
                            ++counts.clear;
                            counts.faint += a > 0 ? 1 : 0;
                        }
                    }
                }
                coverage.push_back(ClassifyTileCoverage(counts));
            }
        }
        return coverage;
    }

    // An icon's coverage; Opaque for an icon the table does not have (no sheet classified).
    [[nodiscard]] inline TileCoverage CoverageOf(const std::vector<TileCoverage>& coverage, int icon) noexcept
    {
        return icon >= 0 && static_cast<std::size_t>(icon) < coverage.size() ? coverage[static_cast<std::size_t>(icon)]
                                                                             : TileCoverage::Opaque;
    }
}
