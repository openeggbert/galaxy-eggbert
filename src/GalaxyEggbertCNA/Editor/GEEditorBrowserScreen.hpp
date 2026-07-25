#pragma once

#include "Game/GEQuadBatch.hpp"

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Per-gamer-slot world list/create/open/delete screen (plan.md
    // EDITOR-107) -- the entry point into the editor from the Init menu's
    // new Editor button. Deliberately simple: no free-text renaming (no
    // text-input widget exists anywhere in this codebase yet, a documented
    // gap, not silently invented around); worlds are distinguished by
    // GECustomWorldStorage's own generated filename and a
    // std::filesystem::last_write_time-derived recency order (newest
    // first -- see Refresh()).
    class GEEditorBrowserScreen
    {
    public:
        enum class Action { None, Open, New, Back };

        struct UpdateResult
        {
            Action action = Action::None;
            std::filesystem::path path; // valid when action == Open
        };

        // Rescans GECustomWorldStorage::ListCustomWorlds(gamerSlot) from
        // disk -- call once when entering the browser and again after any
        // create/delete, not every frame.
        void Refresh(int gamerSlot);

        // Edge-triggered on release, matching GEInputPad's own click idiom.
        // Deleting is a real two-tap confirm: the first click on a row's
        // "X" arms it (drawn highlighted); a second click on the SAME "X"
        // actually deletes and re-Refresh()es; clicking anything else
        // cancels the armed state without deleting.
        UpdateResult Update(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                           int viewportWidth, int viewportHeight,
                           bool backPressed = false);

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                 int viewportWidth, int viewportHeight);

    private:
        [[nodiscard]] GEQuadBatch::Rect RowRect(int index) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect DeleteButtonRect(int index) const noexcept;
        [[nodiscard]] GEQuadBatch::Rect BackButtonRect(
            int viewportWidth, int viewportHeight) const noexcept;
        void EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);

        int gamerSlot_ = 0;
        std::vector<std::filesystem::path> worlds_;
        int armedDeleteIndex_ = -1; // -1 = no row's delete currently armed
        bool mouseWasDown_ = false;
        bool backWasDown_ = false;

        bool loaded_ = false;
        Microsoft::Xna::Framework::Graphics::Texture2D textTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> textRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> flatEffect_;
        std::unique_ptr<Easy3D::BillboardMeshRenderer> flatRenderer_;
    };
}
