#pragma once

#include <GalaxyEggbert/Game/QuadBatch.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <filesystem>
#include <memory>
#include <vector>

namespace GalaxyEggbert::Editor
{
    // The editor's world screen for one gamer (plan.md EDITOR-107), entered
    // from the main menu's 3D World Editor button: the gamer's ten slots
    // (CustomWorldStorage, owner 2026-10-25) as numbered rows -- an occupied
    // slot opens, an empty one creates a new world there, and an occupied one
    // can be deleted with two clicks -- and, beside them, the gamer's older
    // worlds (files outside the ten slots), which are never deleted or hidden:
    // clicking one copies it into the first free slot and opens the copy. No
    // free-text renaming (no text-input widget exists in this codebase yet).
    class EditorBrowserScreen
    {
    public:
        enum class Action { None, Open, New, Back };

        struct UpdateResult
        {
            Action action = Action::None;
            std::filesystem::path path; // the world to open (Open) or create (New)
        };

        // Rescans the gamer's slots and older worlds from disk -- call once
        // when entering the browser and again after any create/delete, not
        // every frame.
        void Refresh(int gamerSlot);

        // Edge-triggered on release, matching InputPad's own click idiom.
        // Deleting is a real two-tap confirm: the first click on a row's
        // "X" arms it (drawn highlighted); a second click on the SAME "X"
        // actually deletes and re-Refresh()es; clicking anything else
        // cancels the armed state without deleting.
        UpdateResult Update(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                           int viewportWidth, int viewportHeight,
                           bool backPressed = false);

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                 int viewportWidth, int viewportHeight);

        // Screen rects, public for the tests (raw 800x480 pixels like the rest of
        // the editor UI): slot rows 0-9, the older worlds' rows, Back.
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect SlotRect(int slotIndex) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect DeleteButtonRect(int slotIndex) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect OlderRect(int index) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect BackButtonRect() noexcept;

    private:
        void EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);

        int gamerSlot_ = 0;
        std::vector<bool> slotUsed_;                 // per slot 1-10 (index 0-9)
        std::vector<std::filesystem::path> older_;
        int armedDeleteIndex_ = -1; // the slot index whose delete is armed, -1 none
        bool slotsFull_ = false;    // an older world was clicked with no slot free
        bool mouseWasDown_ = false;
        bool backWasDown_ = false;

        bool loaded_ = false;
        Microsoft::Xna::Framework::Graphics::Texture2D textTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> textRenderer_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> flatEffect_;
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> flatRenderer_;
    };
}
