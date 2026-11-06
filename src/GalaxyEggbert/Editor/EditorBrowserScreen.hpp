#pragma once

#include <GalaxyEggbert/Game/QuadBatch.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <filesystem>
#include <memory>
#include <string>
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
    //
    // Backups (ED-0011): the Backups button turns the slots into restore buttons -- a click
    // brings back the slot's newest backup, again an older one, round to where it began
    // (CustomWorldStorage RestoreBackup); a deleted slot's world is its newest backup.
    class EditorBrowserScreen
    {
    public:
        enum class Action { None, Open, New, Back };
        // What a click on a slot does.
        enum class Mode { Open, Restore };

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
        // trash button arms it (drawn as a tick, the caption asks "Delete?");
        // a second click on the SAME button actually deletes and
        // re-Refresh()es; clicking anything else cancels the armed state.
        UpdateResult Update(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                           int viewportWidth, int viewportHeight,
                           bool backPressed = false);

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                 int viewportWidth, int viewportHeight);

        // Screen rects, public for the tests (raw 800x480 pixels like the rest of
        // the editor UI): slots 0-9 (a grid of five by two, each the numbered button and
        // its caption), each occupied slot's trash button, the older worlds' rows, Back.
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect SlotRect(int slotIndex) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect DeleteButtonRect(int slotIndex) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect OlderRect(int index) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect BackButtonRect() noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect RestoreModeRect() noexcept;

        [[nodiscard]] Mode CurrentMode() const noexcept { return mode_; }
        // Back to opening worlds, no message (entering the browser).
        void ResetMode() noexcept
        {
            mode_ = Mode::Open;
            message_.clear();
            restoreSlot_ = -1;
        }
        // The line under the slots: what the last click did.
        [[nodiscard]] const std::string& Message() const noexcept { return message_; }

    private:
        void EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        void Restore(int slotIndex);

        int gamerSlot_ = 0;
        std::vector<bool> slotUsed_;                 // per slot 1-10 (index 0-9)
        std::vector<int> backupCount_;               // per slot
        Mode mode_ = Mode::Open;
        std::string message_;
        int restoreSlot_ = -1;          // the slot the last restores stepped through
        int restoreSteps_ = 0;
        int restoreCycle_ = 0;          // its backups and the world it had: restores round to it
        std::vector<std::filesystem::path> older_;
        int armedDeleteIndex_ = -1; // the slot index whose delete is armed, -1 none
        bool slotsFull_ = false;    // an older world was clicked with no slot free
        bool mouseWasDown_ = false;
        float mouseX_ = -1.0f; // where the mouse was at the last Update(), for the held button
        float mouseY_ = -1.0f;
        bool backWasDown_ = false;

        bool loaded_ = false;
        Microsoft::Xna::Framework::Graphics::Texture2D textTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> textEffect_;
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> textRenderer_;
        Microsoft::Xna::Framework::Graphics::Texture2D padTexture_;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> padEffect_;
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> padRenderer_;
    };
}
