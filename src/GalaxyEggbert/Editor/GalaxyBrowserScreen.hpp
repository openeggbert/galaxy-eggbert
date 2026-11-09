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
    // The editor's screen of one gamer's own galaxy (BD-0119; CustomWorldStorage GalaxyDir()),
    // reached from the world screen's Galaxy button: the main hub and the bonus world on top, the
    // fourteen worlds below, and the selected world's hub and eight missions at the bottom. A world
    // that exists opens, one that does not is created from its starter (CreateGalaxyStarterWorld)
    // and opened; a click on a world's number only selects it. Back returns to the world screen.
    // As on the world screen, a place that exists has a bin on its corner: the first click arms it
    // (a tick, the caption "Delete?"), a second click on it deletes the file into the galaxy's
    // backups (DeleteSlotWorld); Backups switches to bringing a place's newest backup back
    // (RestoreBackup; again steps further back, round to where it began). Export writes the whole
    // galaxy as one file (ExportGalaxy(); downloaded on the web); Import lists the galaxies in
    // SharedWorldsDir() where the help is (on the web it opens the file picker first) and a row
    // clicked twice replaces this galaxy with it (ImportGalaxy(), the places there now kept in their
    // backups). The list pages by eight (arrows under it), and a row's bin deletes the shared file
    // (two clicks; on the web the browser's storage is the only place a player could reach it).
    // Raw 800x480 pixels, as EditorBrowserScreen.
    class GalaxyBrowserScreen
    {
    public:
        enum class Action { None, Open, New, Back };
        struct UpdateResult
        {
            Action action = Action::None;
            int mission = 0;            // Open/New: the galaxy mission
            std::filesystem::path path; // its file
            bool imported = false;      // another galaxy replaced this one (its progress starts over)
        };

        // Rescans which of the gamer's galaxy worlds exist (on entering, and after a creation).
        void Refresh(int gamerSlot);
        UpdateResult Update(const Microsoft::Xna::Framework::Input::MouseState& mouse, bool backPressed = false);
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int viewportWidth, int viewportHeight);

        // The world whose hub and missions the bottom row shows (1-14).
        [[nodiscard]] int SelectedWorld() const noexcept { return selectedWorld_; }

        // Whether Backups is on (a click on a place restores instead of opening), and the line
        // under the places (what the last delete or restore did).
        [[nodiscard]] bool RestoreMode() const noexcept { return restoreMode_; }
        [[nodiscard]] bool ImportMode() const noexcept { return importMode_; }
        [[nodiscard]] const std::string& Message() const noexcept { return message_; }

        // Rects, public for the tests: the main hub, the bonus world, world n's number (1-14), the
        // bottom row's place p (0 the hub, 1-8 the missions), the bin on a place's corner, Backups,
        // Back.
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect MainHubRect() noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect BonusRect() noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect WorldRect(int world) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect PlaceRect(int place) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect DeleteRect(
            const GalaxyEggbert::Game::QuadBatch::Rect& place) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect BackupsRect() noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect ExportRect() noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect ImportRect() noexcept;
        // Row @p index (0-7) of a page of the galaxies to import, where the help is; the bin on its
        // right end (two clicks delete the file); the pages' arrows under the rows.
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect ImportRowRect(int index) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect ImportDeleteRect(int index) noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect ImportPrevRect() noexcept;
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect ImportNextRect() noexcept;
        static constexpr int kImportRowsShown = 8;
        [[nodiscard]] int ImportPage() const noexcept { return importPage_; }
        [[nodiscard]] std::size_t ImportCount() const noexcept { return imports_.size(); }
        [[nodiscard]] static GalaxyEggbert::Game::QuadBatch::Rect BackButtonRect() noexcept;

    private:
        struct ShownPlace
        {
            int mission;
            GalaxyEggbert::Game::QuadBatch::Rect rect;
        };
        void EnsureLoaded(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        void Restore(int mission);
        void Export();
        void ToggleImport();
        [[nodiscard]] bool Exists(int mission) const noexcept;
        [[nodiscard]] int Backups(int mission) const noexcept;
        [[nodiscard]] bool WorldHasAny(int world) const noexcept;
        // The places on the screen now: the main hub, the bonus world, the selected world's nine.
        [[nodiscard]] std::vector<ShownPlace> Shown() const;

        int gamerSlot_ = 0;
        int selectedWorld_ = 1;
        std::vector<bool> exists_ = std::vector<bool>(200, false); // per mission number
        std::vector<int> backups_ = std::vector<int>(200, 0);      // their backups (0-3)
        int armedDelete_ = 0;     // the mission whose bin was clicked once, 0 none
        bool restoreMode_ = false;
        int restoreMission_ = 0;  // the place the last restores stepped through
        int restoreSteps_ = 0;
        int restoreCycle_ = 0;    // its backups and the world it had: restores come round to it
        std::string message_;
        bool importMode_ = false;
        std::vector<std::filesystem::path> imports_; // the galaxies listed to import
        int armedImport_ = -1;                       // the file (index in imports_) clicked once, -1 none
        int armedImportDelete_ = -1;                 // the file whose bin was clicked once, -1 none
        int importPage_ = 0;                         // kImportRowsShown files a page
        bool mouseWasDown_ = false;
        float mouseX_ = -1.0f, mouseY_ = -1.0f;
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
