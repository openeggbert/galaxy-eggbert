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
        const int rows = (BlockTypes::kSheetH + BlockTypes::kTileSize - 1) / BlockTypes::kTileSize;
        m_atlas.AddGrid(kTilePrefix, BlockTypes::kTileSize, BlockTypes::kTileSize,
                         BlockTypes::kSheetCols, rows);
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
