#include "GETileAtlas.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"

#include <string>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr std::string_view kTilePrefix = "tile";
    }

    GETileAtlas::GETileAtlas()
        : m_atlas(BlockTypes::kSheetW, BlockTypes::kSheetH)
    {
        // Row-major grid registration matches BlockTypes' own
        // icon = row * kSheetCols + col convention exactly, so the AddGrid
        // frame index equals the block type / icon index directly.
        // startX/startY/spacingX/spacingY = kSheetGap: object-m.png is packed on a
        // 65px pitch (64px icon + 1px gap), not a flat 64px grid — see
        // BlockTypes::kSheetGap / tileUV()'s comment for how this was found.
        const int rows = (BlockTypes::kSheetH - BlockTypes::kSheetGap) /
                         (BlockTypes::kTileSize + BlockTypes::kSheetGap);
        m_atlas.AddGrid(kTilePrefix, BlockTypes::kTileSize, BlockTypes::kTileSize,
                         BlockTypes::kSheetCols, rows,
                         BlockTypes::kSheetGap, BlockTypes::kSheetGap,
                         BlockTypes::kSheetGap, BlockTypes::kSheetGap);
    }

    Easy3D::UvRect GETileAtlas::GetTileUv(int blockType) const
    {
        if (blockType <= 0)
        {
            return {};
        }
        return m_atlas.GetUvOrDefault(std::string(kTilePrefix) + "_" + std::to_string(blockType));
    }
}
