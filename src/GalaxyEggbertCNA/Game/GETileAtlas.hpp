#pragma once

#include <Easy3D/TextureAtlas.hpp>

namespace GalaxyEggbert::CNA
{
    // Maps galaxy-eggbert block types (== icon index into mobile-eggbert's
    // object-m.png, see GalaxyEggbert::BlockTypes) to normalized UV rects via
    // an Easy3D::TextureAtlas grid registration. Data-only: no texture pixels
    // are loaded here and nothing is drawn — see plan.md E3D-MIG-053.
    class GETileAtlas
    {
    public:
        GETileAtlas();

        // UV rect for a block type. Returns an all-zero UvRect for Air (0),
        // negative values, or any type outside the registered sheet grid.
        [[nodiscard]] Easy3D::UvRect GetTileUv(int blockType) const;

    private:
        Easy3D::TextureAtlas m_atlas;
    };
}
