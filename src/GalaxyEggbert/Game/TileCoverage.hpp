#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>

#include <cstddef>
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
    [[nodiscard]] inline std::vector<TileCoverage> ClassifyTileSheet(const std::uint8_t* alpha, int width, int height,
                                                                    int scale = 1)
    {
        std::vector<TileCoverage> coverage;
        if (alpha == nullptr || width <= 0 || height <= 0)
        {
            return coverage;
        }
        const int kTileSize = BlockTypes::kTileSize * scale;
        const int kSheetGap = BlockTypes::kSheetGap * scale;
        const int kPitch = kTileSize + kSheetGap;
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

    // Tiles whose clear texels are holes to see through, though under a quarter of them is clear: the
    // cheese with holes 339 (owner 2026-11-04, Q-010: see-through holes). Drawn as a cutout, so its
    // neighbours' faces stay and a hole shows what lies behind; left out of the clear-texel fill.
    inline constexpr int kSeeThroughTiles[] = {339};
    inline void MarkSeeThroughTiles(std::vector<TileCoverage>& coverage)
    {
        for (const int icon : kSeeThroughTiles)
        {
            if (icon >= 0 && static_cast<std::size_t>(icon) < coverage.size())
            {
                coverage[static_cast<std::size_t>(icon)] = TileCoverage::Cutout;
            }
        }
    }

    // The clear texels of the tiles drawn opaque take the colour of the solid texels round them
    // (2026-11-03, the second play pass). Those tiles (under a quarter clear) are drawn as they are,
    // the alpha ignored, so their clear texels showed whatever colour the sheet stores there -- mostly
    // black: black slashes on the slime edge pieces (347-363), slits in the brick edge pieces (262,
    // 263, 283), dots in the cheese with holes (339). In the original's 2D the backdrop showed
    // through them; on a 3D cube the nearest colour of the tile itself is the closest to that. Each
    // clear texel takes the mean of its solid (or already filled) 8 neighbours within the same tile,
    // ring by ring inward, and becomes solid (the terrain's opaque pass draws with the blend state
    // the frame left, so a clear texel let the black behind the face show through). Only tiles
    // classified opaque are touched: the cutout and blended ones keep their shapes; their texels all
    // end wholly opaque. `rgba`: 4 bytes a texel, row by row, the sheet as ClassifyTileSheet's.
    inline void FillOpaqueTilesClearTexels(std::uint8_t* rgba, int width, int height,
                                           const std::vector<TileCoverage>& coverage, int scale = 1)
    {
        if (rgba == nullptr || width <= 0 || height <= 0)
        {
            return;
        }
        const int kTileSize = BlockTypes::kTileSize * scale;
        const int kSheetGap = BlockTypes::kSheetGap * scale;
        const int kPitch = kTileSize + kSheetGap;
        const int columns = width / kTileSize;
        std::vector<std::uint8_t> filled(static_cast<std::size_t>(kTileSize) * kTileSize);
        std::vector<std::uint8_t> next;
        for (std::size_t icon = 0; icon < coverage.size(); ++icon)
        {
            if (coverage[icon] != TileCoverage::Opaque || columns <= 0)
            {
                continue;
            }
            const int x0 = kSheetGap + static_cast<int>(icon % static_cast<std::size_t>(columns)) * kPitch;
            const int y0 = kSheetGap + static_cast<int>(icon / static_cast<std::size_t>(columns)) * kPitch;
            if (x0 + kTileSize > width || y0 + kTileSize > height)
            {
                continue;
            }
            const auto at = [&](int x, int y) { return rgba + (static_cast<std::size_t>(y0 + y) * width + (x0 + x)) * 4; };
            int clear = 0;
            for (int y = 0; y < kTileSize; ++y)
            {
                for (int x = 0; x < kTileSize; ++x)
                {
                    const bool solid = at(x, y)[3] >= kCutoutReferenceAlpha;
                    filled[static_cast<std::size_t>(y) * kTileSize + x] = solid ? 1 : 0;
                    clear += solid ? 0 : 1;
                }
            }
            while (clear > 0)
            {
                next = filled;
                int done = 0;
                for (int y = 0; y < kTileSize; ++y)
                {
                    for (int x = 0; x < kTileSize; ++x)
                    {
                        if (filled[static_cast<std::size_t>(y) * kTileSize + x] != 0)
                        {
                            continue;
                        }
                        int sum[3] = {0, 0, 0};
                        int count = 0;
                        for (int dy = -1; dy <= 1; ++dy)
                        {
                            for (int dx = -1; dx <= 1; ++dx)
                            {
                                const int nx = x + dx, ny = y + dy;
                                if ((dx == 0 && dy == 0) || nx < 0 || ny < 0 || nx >= kTileSize || ny >= kTileSize ||
                                    filled[static_cast<std::size_t>(ny) * kTileSize + nx] == 0)
                                {
                                    continue;
                                }
                                const std::uint8_t* n = at(nx, ny);
                                sum[0] += n[0];
                                sum[1] += n[1];
                                sum[2] += n[2];
                                ++count;
                            }
                        }
                        if (count > 0)
                        {
                            std::uint8_t* texel = at(x, y);
                            for (int k = 0; k < 3; ++k)
                            {
                                texel[k] = static_cast<std::uint8_t>(sum[k] / count);
                            }
                            texel[3] = 255;
                            next[static_cast<std::size_t>(y) * kTileSize + x] = 1;
                            ++done;
                        }
                    }
                }
                if (done == 0)
                {
                    break; // a tile with no solid texel at all
                }
                filled.swap(next);
                clear -= done;
            }
            // And every texel wholly opaque (2026-11-10, the owner's report: light lines round every
            // cell of a wall with the 2x textures): a sheet rendered with a soft mask (the 2x one) has
            // its opaque tiles' edges at alpha 140-250, which blended the light behind into them.
            for (int y = 0; y < kTileSize; ++y)
            {
                for (int x = 0; x < kTileSize; ++x)
                {
                    at(x, y)[3] = 255;
                }
            }
        }
    }

    // The brick wall's edge pieces (262, 263, 283) where they are whole cubes (Q-011, the owner
    // 2026-11-09: whole bricks except at a wall's open end, BrickEdges.hpp): their clear texels take
    // the whole brick 261's texel at the same place, so the bricks run on to the tile's edge (the
    // mean of the texels round them, FillOpaqueTilesClearTexels(), made pale bands of mortar colour
    // in the walls the worlds scatter them in). Call before FillOpaqueTilesClearTexels(); the same
    // sheet layout.
    inline constexpr int kWholeBrick = 261;
    inline constexpr int kBrickEdgePieces[] = {262, 263, 283};
    inline void FillBrickEdgesFromWholeBrick(std::uint8_t* rgba, int width, int height, int scale = 1)
    {
        if (rgba == nullptr || width <= 0 || height <= 0) return;
        const int kTileSize = BlockTypes::kTileSize * scale;
        const int kSheetGap = BlockTypes::kSheetGap * scale;
        const int kPitch = kTileSize + kSheetGap;
        const int columns = width / kTileSize;
        if (columns <= 0) return;
        const auto origin = [&](int icon, int& x0, int& y0) {
            x0 = kSheetGap + (icon % columns) * kPitch;
            y0 = kSheetGap + (icon / columns) * kPitch;
            return x0 + kTileSize <= width && y0 + kTileSize <= height;
        };
        int sx = 0, sy = 0;
        if (!origin(kWholeBrick, sx, sy)) return;
        for (const int icon : kBrickEdgePieces)
        {
            int x0 = 0, y0 = 0;
            if (!origin(icon, x0, y0)) continue;
            for (int y = 0; y < kTileSize; ++y)
            {
                for (int x = 0; x < kTileSize; ++x)
                {
                    std::uint8_t* texel = rgba + (static_cast<std::size_t>(y0 + y) * width + (x0 + x)) * 4;
                    const std::uint8_t* whole = rgba + (static_cast<std::size_t>(sy + y) * width + (sx + x)) * 4;
                    if (texel[3] >= kCutoutReferenceAlpha || whole[3] < kCutoutReferenceAlpha) continue;
                    for (int k = 0; k < 4; ++k) texel[k] = whole[k];
                }
            }
        }
    }

    // An icon's coverage; Opaque for an icon the table does not have (no sheet classified).
    [[nodiscard]] inline TileCoverage CoverageOf(const std::vector<TileCoverage>& coverage, int icon) noexcept
    {
        return icon >= 0 && static_cast<std::size_t>(icon) < coverage.size() ? coverage[static_cast<std::size_t>(icon)]
                                                                             : TileCoverage::Opaque;
    }
}
