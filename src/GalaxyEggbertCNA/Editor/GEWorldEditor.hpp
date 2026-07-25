#pragma once

#include "GEEditCommandStack.hpp"
#include "GEEditorBrowserScreen.hpp"
#include "GEEditorHighlightRenderer.hpp"
#include "GEEditorPalette.hpp"

#include <Easy3D/Camera3D.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>

namespace GalaxyEggbert::CNA
{
    // Owns one in-game 3D editing session: camera and placement input,
    // palette interaction, undo history, object editing, persistence and
    // transitions to the world browser or play-test mode.
    class GEWorldEditor
    {
    public:
        // Enters the browser and rescans the selected gamer's custom worlds.
        void EnterBrowser(int gamerSlot);

        [[nodiscard]] bool IsBrowsing() const noexcept { return browsing_; }

        // One request the browser screen made this frame -- the caller
        // (GalaxyEggbertCnaGame) still owns actually loading/creating the
        // .vwr file (this class has no GEWorldRuntime access), matching
        // this project's existing "pending signal consumed by the owning
        // class" idiom (see GEInteractionSystem's own class comment).
        struct BrowserRequest
        {
            bool shouldOpen = false;
            std::filesystem::path openPath; // valid when shouldOpen
            bool shouldCreateNew = false;    // GECustomWorldStorage::NextNewWorldPath(gamerSlot) is the target
        };

        // Drives the browser screen while IsBrowsing() is true -- no World
        // needed (there isn't one loaded yet).
        [[nodiscard]] BrowserRequest UpdateBrowsing(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportWidth, int viewportHeight);

        void DrawBrowsing(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          int viewportWidth, int viewportHeight);

        // Called once the caller has actually loaded/created the requested
        // world -- leaves browsing mode so the next frame's Update()/Draw()
        // dispatch (IsBrowsing() now false) drives real editing instead.
        void ExitBrowser() noexcept { browsing_ = false; }

        // Called once when entering editing (after ExitBrowser(), or
        // temporarily from the F9 debug entry) -- resets the free-fly
        // camera to a sensible starting point/orientation above the loaded
        // world.
        void EnterEditing(float startX, float startY, float startZ) noexcept;

        // Sets the path used by keyboard save and play-test.
        void SetWorldPath(std::filesystem::path path) noexcept { worldPath_ = std::move(path); }

        // The path currently being edited (plan.md EDITOR-108) -- read by
        // GalaxyEggbertCnaGame after ConsumePlayTestRequested() fires, to
        // know which file to load for the play-test session.
        [[nodiscard]] const std::filesystem::path& GetWorldPath() const noexcept { return worldPath_; }

        // Consumes the request raised by the dice button after it saves.
        [[nodiscard]] bool ConsumePlayTestRequested() noexcept;

        // Updates one editor frame. RMB looks, WASD/Space/Ctrl fly and the
        // wheel zooms. Left click or PLACE creates the selected block or
        // object at the red preview; middle click removes a block. NumPad
        // 4/6, 7/9 and 8/2 move the preview on X/Y/Z; NumPad 5 resets it.
        // Enter saves, U/R undo/redo, F/Escape starts/cancels box fill,
        // arrows select a background, and G/T/Tab/+/-/Delete edit objects.
        // Palette presses are consumed before world editing. The visible
        // Stop sign returns to the browser, with a second press required
        // while unsaved changes exist; the dice saves and starts play-test.
        void Update(const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
                    const Microsoft::Xna::Framework::Input::MouseState& mouse,
                    float dt, int viewportWidth, int viewportHeight,
                    Easy3D::Camera3D& camera,
                    Worlds::World& world);

        // True exactly once, right after an Update() call that placed or
        // removed a block -- the caller (GalaxyEggbertCnaGame) should
        // respond by calling its own RebuildWorldPresentation(), the same
        // "rebuild the whole mesh on change" convention GETerrainRenderer
        // already uses for LoadMission(). Clears back to false once read,
        // same "*ThisFrame()"/"Consume*()" idiom already established by
        // GEInteractionSystem/GEBlupiController.
        [[nodiscard]] bool ConsumeNeedsPresentationRebuild() noexcept;

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const Easy3D::Camera3D& camera,
                  int viewportWidth, int viewportHeight);

    private:
        struct FrameInput
        {
            GEEditorPalette::UpdateResult palette;
            bool leftHeld = false;
            bool middleHeld = false;
            bool enterHeld = false;
            bool undoHeld = false;
            bool redoHeld = false;
            bool boxHeld = false;
            bool escapeHeld = false;
            bool selectHeld = false;
            bool targetHeld = false;
            bool cycleFieldHeld = false;
            bool increaseFieldHeld = false;
            bool decreaseFieldHeld = false;
            bool deleteObjectHeld = false;
            bool skyPreviousHeld = false;
            bool skyNextHeld = false;
        };

        [[nodiscard]] Easy3D::Camera3D::Vector3 UpdateCamera(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            float dt, Easy3D::Camera3D& camera);
        [[nodiscard]] bool UpdatePlacementOffset(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
            GEEditorPalette::Action paletteAction);
        void UpdatePlacementPreview(
            const Worlds::World& world, const Easy3D::Camera3D::Vector3& forward);
        [[nodiscard]] FrameInput ReadFrameInput(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            GEEditorPalette::UpdateResult paletteResult) const;
        void UpdateBoxPreview(const Worlds::World& world);
        [[nodiscard]] bool HandlePlacement(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleRemoval(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleSessionAndHistory(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleBoxFill(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleSkyRegion(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleObjectEditing(
            const FrameInput& input, Easy3D::Camera3D& camera,
            int viewportWidth, int viewportHeight, Worlds::World& world);
        void RemoveObjectWithHistory(
            Worlds::World& world, const MoveObjectRecord& record,
            std::uint16_t anchorX, std::uint16_t anchorY, std::uint16_t anchorZ);
        void StoreInputEdges(const FrameInput& input, bool placementOffsetKeyHeld) noexcept;

        float camX_ = 50.0f;
        float camY_ = 15.0f;
        float camZ_ = 50.0f;
        float yaw_ = 0.0f;   // radians; matches GEBlupiController's own convention:
                             // forward = (sin(yaw), -cos(yaw)) in the XZ plane, yaw 0 = facing -Z.
        float pitch_ = -0.35f; // radians, negative = looking down toward the terrain
        float flySpeed_ = 15.0f; // units/second

        bool mouseLookHeldLastFrame_ = false;
        int lastScrollWheelValue_ = 0;
        bool hasLastScrollWheelValue_ = false;

        GEEditorHighlightRenderer highlightRenderer_;
        bool hasHighlight_ = false;
        // A fresh custom world has no solid voxels for the DDA raycast to
        // hit. In that case Update() projects the aim ray onto y=0 and
        // exposes that valid first-placement cell through hasHighlight_,
        // while this flag preserves whether middle-click may remove a real
        // hit voxel.
        bool hasRaycastHit_ = false;
        float highlightX_ = 0.0f;
        float highlightY_ = 0.0f;
        float highlightZ_ = 0.0f;
        // Raw-grid cell hit by the current ray, retained for removal and
        // box-fill. Placement uses placementCell* below.
        std::uint16_t hitCellX_ = 0;
        std::uint16_t hitCellY_ = 0;
        std::uint16_t hitCellZ_ = 0;

        // The ray supplies a base adjacent cell. The author can move the
        // pending placement from that base with NumPad 4/6 (X), 8/2 (Z),
        // and 7/9 (Y), or with the matching on-screen X/Y/Z touch buttons;
        // NumPad 5 resets it. Keeping base and resulting cell separate
        // makes preview, mouse/touch placement and box-fill use exactly the
        // same destination.
        int placementOffsetX_ = 0;
        int placementOffsetY_ = 0;
        int placementOffsetZ_ = 0;
        std::uint16_t placementCellX_ = 0;
        std::uint16_t placementCellY_ = 0;
        std::uint16_t placementCellZ_ = 0;
        bool placementOffsetKeyHeldLastFrame_ = false;

        bool leftHeldLastFrame_ = false;
        bool middleHeldLastFrame_ = false;
        bool enterHeldLastFrame_ = false;
        bool undoKeyHeldLastFrame_ = false;
        bool redoKeyHeldLastFrame_ = false;
        bool boxKeyHeldLastFrame_ = false;
        bool escapeKeyHeldLastFrame_ = false;
        bool skyRegionPrevKeyHeldLastFrame_ = false; // EDITOR-111
        bool skyRegionNextKeyHeldLastFrame_ = false;
        bool needsPresentationRebuild_ = false;
        bool playTestRequested_ = false;

        // Unsaved changes make the visible Stop action require confirmation.
        bool dirty_ = false;
        bool stopConfirmArmed_ = false;

        // Centralizes the state changed by every undoable world mutation.
        void MarkMutated() noexcept
        {
            needsPresentationRebuild_ = true;
            dirty_ = true;
            stopConfirmArmed_ = false;
        }

        GEEditCommandStack commandStack_;
        GEEditorPalette palette_;

        // Box-fill tool (plan.md EDITOR-105).
        bool boxFirstCornerPlaced_ = false;
        std::uint16_t boxCorner0X_ = 0;
        std::uint16_t boxCorner0Y_ = 0;
        std::uint16_t boxCorner0Z_ = 0;
        // Render-space box bounds for Draw() to show, recomputed each
        // Update() call while boxFirstCornerPlaced_ is true.
        bool showingBox_ = false;
        float boxMinRenderX_ = 0.0f, boxMinRenderY_ = 0.0f, boxMinRenderZ_ = 0.0f;
        float boxMaxRenderX_ = 0.0f, boxMaxRenderY_ = 0.0f, boxMaxRenderZ_ = 0.0f;

        // MoveObject select/edit tool (plan.md EDITOR-110). selectedObject_
        // mirrors the record actually stored in the world -- kept in sync
        // across edits so each new edit's undo command captures a correct
        // "before" state, and re-synced from the world after any Undo/Redo
        // (RefreshSelectedObjectAfterHistoryChange()) so undoing/redoing an
        // edit doesn't leave this stale. selectedAnchor* is
        // floor(selectedObject_.posStart*) -- this milestone never moves
        // posStart, so the anchor cell never changes once selected.
        bool hasSelectedObject_ = false;
        MoveObjectRecord selectedObject_;
        std::uint16_t selectedAnchorX_ = 0;
        std::uint16_t selectedAnchorY_ = 0;
        std::uint16_t selectedAnchorZ_ = 0;
        // Render-space position for Draw()'s selection highlight -- set
        // once at selection time (see selectedAnchor*'s own comment for why
        // that's safe to cache).
        float selectedRenderX_ = 0.0f;
        float selectedRenderY_ = 0.0f;
        float selectedRenderZ_ = 0.0f;
        GEEditorHighlightRenderer selectionHighlightRenderer_;

        // Which field Tab cycles through and OemPlus/OemMinus adjust --
        // order matches MoveObjectRecord's own declared field order.
        enum class EditableField
        {
            Speed,
            StepAdvanceTicks,
            StepRecedeTicks,
            TimeStopStartTicks,
            TimeStopEndTicks,
        };
        static constexpr int kEditableFieldCount = 5;
        EditableField activeField_ = EditableField::Speed;

        // Returns @p record with activeField_'s value nudged by @p sign
        // (+1.0f/-1.0f), clamped to a sane floor -- or std::nullopt if the
        // clamp left the field unchanged (so Update() can skip pushing a
        // no-op undo entry, same "don't record a no-op" convention the
        // box-fill tool already follows).
        [[nodiscard]] std::optional<MoveObjectRecord> ApplyActiveFieldDelta(
            const MoveObjectRecord& record, float sign) const noexcept;

        // Re-reads whichever MoveObjectRecord is now anchored at
        // selectedAnchorX/Y/Z_ from @p world and refreshes selectedObject_
        // (or clears hasSelectedObject_ if the anchor is empty, e.g. an
        // undone placement) -- called after every successful Undo()/Redo()
        // so a later G/T/Tab/OemPlus/OemMinus edit's "before" state can't
        // go stale relative to the world it's actually about to mutate.
        void RefreshSelectedObjectAfterHistoryChange(const Worlds::World& world);

        bool selectKeyHeldLastFrame_ = false;
        bool targetKeyHeldLastFrame_ = false;
        bool cycleFieldKeyHeldLastFrame_ = false;
        bool increaseFieldKeyHeldLastFrame_ = false;
        bool decreaseFieldKeyHeldLastFrame_ = false;
        bool deleteObjectKeyHeldLastFrame_ = false;

        std::filesystem::path worldPath_;

        bool browsing_ = true;
        int gamerSlot_ = 0;
        GEEditorBrowserScreen browserScreen_;
    };
}
