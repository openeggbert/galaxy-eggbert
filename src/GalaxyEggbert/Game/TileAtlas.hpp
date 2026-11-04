#pragma once

#include <CnaEngine/Graphics/TextureAtlas.hpp>

#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Maps galaxy-eggbert block types (== icon index into mobile-eggbert's
    // object-m.png, see GalaxyEggbert::BlockTypes) to normalized UV rects.
    // Data-only: no texture pixels are loaded here and nothing is drawn —
    // see plan.md E3D-MIG-053.
    //
    // Uses GalaxyEggbert::BlockTypes::tileUV() directly (2026-07-09, NEXT.md
    // §8 task 2 -- seam-line artifact) rather than a plain
    // Easy3D::TextureAtlas grid registration: tileUV() applies a half-texel
    // UV inset documented in BlockTypes.hpp (without it, bilinear filtering at a
    // texel's exact edge samples into the next atlas tile or the 1px
    // packing gap, producing a visible seam at oblique/close angles -- this
    // was the root cause of the thin blue/dark seam lines reported
    // 2026-07-08). The previous Easy3D::TextureAtlas-based implementation
    // computed the *pitch* correctly (kSheetGap-aware column/row spacing,
    // so no cross-tile pixel drift) but never applied this additional
    // inset -- the seam regression is documented in missing.md and locked
    // by the atlas tests.
    class TileAtlas
    {
    public:
        // Sheet: object-m.png as it is (1 px between tiles). Padded: the terrain's own copy of it
        // (BuildPaddedSheet), each tile with a 16 px border of its own edge texels round it, so the
        // terrain can be mipmapped without tiles bleeding into each other (2026-11-04: without mips
        // distant floors shimmered into rings, 125, 105). The padding also makes the half-texel
        // inset unnecessary there: a sample past the tile's edge reads the same edge texel.
        enum class Layout
        {
            Sheet,
            Padded,
        };
        static constexpr int kPaddedBorder = 16;
        static constexpr int kPaddedPitch = 64 + 2 * kPaddedBorder; // 96: tiles stay aligned to mip level 4
        static constexpr int kPaddedColumns = 20;
        static constexpr int kPaddedRows = 22;
        static constexpr int kPaddedWidth = kPaddedColumns * kPaddedPitch;
        static constexpr int kPaddedHeight = kPaddedRows * kPaddedPitch;

        TileAtlas() = default;
        explicit TileAtlas(Layout layout) : layout_(layout) {}

        // UV rect for a block type. Returns an all-zero UvRect for Air (0),
        // negative values, or any type outside the sheet's grid.
        [[nodiscard]] CnaEngine::Graphics::UvRect GetTileUv(int blockType) const;

    private:
        Layout layout_ = Layout::Sheet;
    };

    // The padded terrain sheet (TileAtlas::Layout::Padded) from object-m.png's texels (RGBA, row by
    // row, width x height): each icon's 64 x 64 texels at (border + col * pitch, border + row *
    // pitch), the border round it filled with its nearest edge texel.
    [[nodiscard]] std::vector<std::uint8_t> BuildPaddedSheet(const std::uint8_t* sheetRgba, int width, int height);

    // The next mip level of an RGBA image (each texel the mean of a 2 x 2 block; odd edges clamp).
    [[nodiscard]] std::vector<std::uint8_t> DownsampleHalf(const std::vector<std::uint8_t>& rgba, int width, int height);
}
