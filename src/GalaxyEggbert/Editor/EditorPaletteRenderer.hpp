#pragma once
#include <string>

#include "EditorPaletteLayout.hpp"
#include "PaletteCategories.hpp"

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>

#include <memory>
#include <vector>

namespace GalaxyEggbert::Editor
{
    class EditorPaletteRenderer
    {
    public:
        struct State
        {
            const std::vector<PaletteCategory>& categories;
            const std::vector<int>& contentBlockIds;
            const std::vector<int>& contentObjectTypeIds;
            const std::vector<int>& contentButtonIconIds;
            const std::vector<int>& contentSkyRegionIds;
            const std::vector<int>& contentMusicTrackIds;
            const std::vector<int>& contentSpawnPointIds;
            const std::vector<int>& contentBigDecorIconIds;
            int openCategory = -1;
            int selectedBlockType = 0;
            int selectedObjectType = 0;
            int selectedSkyRegion = 0;
            int selectedMusicTrack = 0;
            int selectedBigDecorIcon = 0;
            int selectedButtonIcon = 0;
            bool objectMode = false;
            bool spawnPointMode = false;
            bool bigDecorMode = false;
            bool noticeVisible = false;
            bool stopConfirmArmed = false;
            bool hasPlacementPreview = false;
            int placementX = 0;
            int placementY = 0;
            int placementZ = 0;
            // Lines over the view, top left of it (WorldEditor::StatusLines: a prompt, the keys).
            std::vector<std::string> statusLines;
            // Words placed in the view (the world's axes' names), centred on their screen points.
            struct ViewLabel
            {
                float x = 0.0f;
                float y = 0.0f;
                std::string text;
                Microsoft::Xna::Framework::Vector3 colour;
            };
            std::vector<ViewLabel> viewLabels;
        };

        EditorPaletteRenderer();
        ~EditorPaletteRenderer();
        EditorPaletteRenderer(const EditorPaletteRenderer&) = delete;
        EditorPaletteRenderer& operator=(const EditorPaletteRenderer&) = delete;

        void Draw(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            const EditorPaletteLayout& layout, const State& state,
            int viewportWidth, int viewportHeight);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
