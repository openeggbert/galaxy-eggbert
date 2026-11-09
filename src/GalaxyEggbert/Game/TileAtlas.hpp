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

        // The padded sheet's layout at a texture scale (TextureScale.hpp): 1 is the constants above; 2 (the
        // 2x object-m.png, 128-texel tiles) keeps the border at 16 texels, so the sheet is 3200 x 3520 and
        // stays inside HiDef's 4096 (twice the 1x layout would be 3840 x 4224).
        struct PaddedLayout
        {
            int tile = 64;
            int border = kPaddedBorder;
            int pitch = kPaddedPitch;
            int width = kPaddedWidth;
            int height = kPaddedHeight;
        };
        [[nodiscard]] static constexpr PaddedLayout PaddedLayoutFor(int scale) noexcept
        {
            const int tile = 64 * (scale < 1 ? 1 : scale);
            const int pitch = tile + 2 * kPaddedBorder;
            return PaddedLayout{tile, kPaddedBorder, pitch, kPaddedColumns * pitch, kPaddedRows * pitch};
        }

        TileAtlas() = default;
        explicit TileAtlas(Layout layout, int scale = 1) : layout_(layout), scale_(scale < 1 ? 1 : scale) {}

        // UV rect for a block type. Returns an all-zero UvRect for Air (0),
        // negative values, or any type outside the sheet's grid.
        [[nodiscard]] CnaEngine::Graphics::UvRect GetTileUv(int blockType) const;
        [[nodiscard]] int Scale() const noexcept { return scale_; }
        [[nodiscard]] PaddedLayout Padded() const noexcept { return PaddedLayoutFor(scale_); }

    private:
        Layout layout_ = Layout::Sheet;
        int scale_ = 1;
    };

    // The padded terrain sheet (TileAtlas::Layout::Padded) from object-m.png's texels (RGBA, row by
    // row, width x height): each icon's 64 x 64 texels at (border + col * pitch, border + row *
    // pitch), the border round it filled with its nearest edge texel.
    // `scale`: the sheet's texture scale (TextureScale.hpp): 2 for the 2x object-m.png, laid out as
    // TileAtlas::PaddedLayoutFor(2).
    [[nodiscard]] std::vector<std::uint8_t> BuildPaddedSheet(const std::uint8_t* sheetRgba, int width, int height,
                                                             int scale = 1);

    // The next mip level of an RGBA image (each texel the mean of a 2 x 2 block; odd edges clamp).
    [[nodiscard]] std::vector<std::uint8_t> DownsampleHalf(const std::vector<std::uint8_t>& rgba, int width, int height);
}
