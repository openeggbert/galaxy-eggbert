#pragma once

#include "GEEditorPaletteLayout.hpp"
#include "GEPaletteCategories.hpp"

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>

#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA
{
    class GEEditorPaletteRenderer
    {
    public:
        struct State
        {
            const std::vector<PaletteCategory>& categories;
            const std::vector<int>& contentBlockIds;
            const std::vector<int>& contentObjectTypeIds;
            const std::vector<int>& contentButtonIconIds;
            int openCategory = -1;
            int selectedBlockType = 0;
            int selectedObjectType = 0;
            bool objectMode = false;
            bool noticeVisible = false;
            bool stopConfirmArmed = false;
            bool hasPlacementPreview = false;
        };

        GEEditorPaletteRenderer();
        ~GEEditorPaletteRenderer();
        GEEditorPaletteRenderer(const GEEditorPaletteRenderer&) = delete;
        GEEditorPaletteRenderer& operator=(const GEEditorPaletteRenderer&) = delete;

        void Draw(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            const GEEditorPaletteLayout& layout, const State& state,
            int viewportWidth, int viewportHeight);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
