#include "TileAtlas.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include "GalaxyEggbert/BlockTypes.hpp"

#include <algorithm>
#include <cstddef>

namespace GalaxyEggbert::Game
{
    namespace
    {
        // Total registered icon slots in object-m.png's grid (20 cols x 22
        // rows = 440, icons 0..439) -- same row computation TileAtlas's
        // old Easy3D::TextureAtlas-based constructor used, kept here purely
        // as a bounds check now that tileUV() itself doesn't validate icon
        // range.
        const int kIconRows = (BlockTypes::kSheetH - BlockTypes::kSheetGap) /
                              (BlockTypes::kTileSize + BlockTypes::kSheetGap);
        const int kIconCount = BlockTypes::kSheetCols * kIconRows;
    }

    CnaEngine::Graphics::UvRect TileAtlas::GetTileUv(int blockType) const
    {
        if (blockType <= 0 || blockType >= kIconCount ||
            !GalaxyEggbert::IsSupportedBlockType(static_cast<std::uint16_t>(blockType)))
        {
            return {};
        }

        const auto& definition =
            GalaxyEggbert::GetBlockDefinition(static_cast<std::uint16_t>(blockType));
        if (definition.textureSource != GalaxyEggbert::BlockTextureSource::ObjectM)
        {
            return {};
        }

        if (layout_ == Layout::Padded)
        {
            const int icon = static_cast<int>(definition.textureIcon);
            const int col = icon % kPaddedColumns;
            const int row = icon / kPaddedColumns;
            const float u0 = static_cast<float>(kPaddedBorder + col * kPaddedPitch) / kPaddedWidth;
            const float v0 = static_cast<float>(kPaddedBorder + row * kPaddedPitch) / kPaddedHeight;
            return CnaEngine::Graphics::UvRect{u0, v0, u0 + 64.0f / kPaddedWidth, v0 + 64.0f / kPaddedHeight};
        }
        float uOff = 0.0f;
        float vOff = 0.0f;
        float uScale = 0.0f;
        float vScale = 0.0f;
        BlockTypes::tileUV(definition.textureIcon, uOff, vOff, uScale, vScale);
        return CnaEngine::Graphics::UvRect{uOff, vOff, uOff + uScale, vOff + vScale};
    }

    std::vector<std::uint8_t> BuildPaddedSheet(const std::uint8_t* sheetRgba, int width, int height)
    {
        constexpr int W = TileAtlas::kPaddedWidth;
        constexpr int H = TileAtlas::kPaddedHeight;
        std::vector<std::uint8_t> out(static_cast<std::size_t>(W) * H * 4, 0);
        if (sheetRgba == nullptr)
        {
            return out;
        }
        constexpr int kTile = BlockTypes::kTileSize;
        constexpr int kGap = BlockTypes::kSheetGap;
        for (int row = 0; row < TileAtlas::kPaddedRows; ++row)
        {
            for (int col = 0; col < TileAtlas::kPaddedColumns; ++col)
            {
                const int sx0 = kGap + col * (kTile + kGap);
                const int sy0 = kGap + row * (kTile + kGap);
                if (sx0 + kTile > width || sy0 + kTile > height)
                {
                    continue;
                }
                const int dx0 = col * TileAtlas::kPaddedPitch;
                const int dy0 = row * TileAtlas::kPaddedPitch;
                for (int y = 0; y < TileAtlas::kPaddedPitch; ++y)
                {
                    const int ty = std::clamp(y - TileAtlas::kPaddedBorder, 0, kTile - 1);
                    for (int x = 0; x < TileAtlas::kPaddedPitch; ++x)
                    {
                        const int tx = std::clamp(x - TileAtlas::kPaddedBorder, 0, kTile - 1);
                        const std::uint8_t* src =
                            sheetRgba + (static_cast<std::size_t>(sy0 + ty) * width + (sx0 + tx)) * 4;
                        std::uint8_t* dst = out.data() + (static_cast<std::size_t>(dy0 + y) * W + (dx0 + x)) * 4;
                        dst[0] = src[0];
                        dst[1] = src[1];
                        dst[2] = src[2];
                        dst[3] = src[3];
                    }
                }
            }
        }
        return out;
    }

    std::vector<std::uint8_t> DownsampleHalf(const std::vector<std::uint8_t>& rgba, int width, int height)
    {
        const int w = std::max(1, width / 2);
        const int h = std::max(1, height / 2);
        std::vector<std::uint8_t> out(static_cast<std::size_t>(w) * h * 4);
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                for (int k = 0; k < 4; ++k)
                {
                    int sum = 0;
                    for (int dy = 0; dy < 2; ++dy)
                    {
                        for (int dx = 0; dx < 2; ++dx)
                        {
                            const int sx = std::min(2 * x + dx, width - 1);
                            const int sy = std::min(2 * y + dy, height - 1);
                            sum += rgba[(static_cast<std::size_t>(sy) * width + sx) * 4 + k];
                        }
                    }
                    out[(static_cast<std::size_t>(y) * w + x) * 4 + k] = static_cast<std::uint8_t>((sum + 2) / 4);
                }
            }
        }
        return out;
    }
}
