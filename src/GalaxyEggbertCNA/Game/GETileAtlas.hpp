#pragma once

#include <Easy3D/TextureAtlas.hpp>

namespace GalaxyEggbert::CNA
{
    // Maps galaxy-eggbert block types (== icon index into mobile-eggbert's
    // object-m.png, see GalaxyEggbert::BlockTypes) to normalized UV rects.
    // Data-only: no texture pixels are loaded here and nothing is drawn —
    // see plan.md E3D-MIG-053.
    //
    // Uses GalaxyEggbert::BlockTypes::tileUV() directly (2026-07-09, NEXT.md
    // §8 task 2 -- seam-line artifact) rather than a plain
    // Easy3D::TextureAtlas grid registration: tileUV() applies a half-texel
    // UV inset that GalaxyEggbertSimple3D's own GETerrainRenderer.cpp
    // already needed and documented (without it, bilinear filtering at a
    // texel's exact edge samples into the next atlas tile or the 1px
    // packing gap, producing a visible seam at oblique/close angles -- this
    // was the root cause of the thin blue/dark seam lines reported
    // 2026-07-08). The previous Easy3D::TextureAtlas-based implementation
    // computed the *pitch* correctly (kSheetGap-aware column/row spacing,
    // so no cross-tile pixel drift) but never applied this additional
    // inset -- a real, if narrow, functional gap versus the historical
    // Simple3D reference this class was ported from.
    class GETileAtlas
    {
    public:
        GETileAtlas() = default;

        // UV rect for a block type. Returns an all-zero UvRect for Air (0),
        // negative values, or any type outside the sheet's grid.
        [[nodiscard]] Easy3D::UvRect GetTileUv(int blockType) const;
    };
}
