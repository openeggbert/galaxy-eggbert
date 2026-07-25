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
            const std::vector<int>& contentSkyRegionIds;
            const std::vector<int>& contentSpawnPointIds;
            const std::vector<int>& contentBigDecorIconIds;
            int openCategory = -1;
            int selectedBlockType = 0;
            int selectedObjectType = 0;
            int selectedSkyRegion = 0;
            bool objectMode = false;
            bool spawnPointMode = false;
            bool bigDecorMode = false;
            bool noticeVisible = false;
            bool stopConfirmArmed = false;
            bool hasPlacementPreview = false;
            int placementX = 0;
            int placementY = 0;
            int placementZ = 0;
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
