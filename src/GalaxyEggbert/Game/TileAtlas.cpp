#include "TileAtlas.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"

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

    Easy3D::UvRect TileAtlas::GetTileUv(int blockType) const
    {
        if (blockType <= 0 || blockType >= kIconCount)
        {
            return {};
        }

        float uOff = 0.0f;
        float vOff = 0.0f;
        float uScale = 0.0f;
        float vScale = 0.0f;
        BlockTypes::tileUV(blockType, uOff, vOff, uScale, vScale);
        return Easy3D::UvRect{uOff, vOff, uOff + uScale, vOff + vScale};
    }
}
