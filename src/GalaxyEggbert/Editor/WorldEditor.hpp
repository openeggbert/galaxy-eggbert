#pragma once
#include <vector>
#include <string>
#include <array>

#include "Clipboard.hpp"
#include "EditCommandStack.hpp"
#include "EditorBrowserScreen.hpp"
#include "EditorHighlightRenderer.hpp"
#include "EditorPalette.hpp"
#include "LiftEndpointRenderer.hpp"
#include "EditorVariants.hpp"

#include <CnaEngine/Graphics/Camera.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>

#include <random>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <utility>

namespace GalaxyEggbert::Editor
{
    struct EditorNavigation
    {
        float forward = 0, strafe = 0, vertical = 0;
        float yaw = 0, pitch = 0;
        bool centeredAim = false, keepAim = false;
    };
    // A lift's speed per cell and rests by its menu entry (20 Hz ticks; the PC editor's PlaceDecor4,
    // blupi_recon decdesign.cpp: slow 20/20/20, normal 10/10/10, fast 6/6/3, fast/slow 5 up, 22
    // back, 2). Keyed by the entry's button.png glyph (PaletteCategories: 21, 20, 19, 28; the
    // conveyor lift 121 as the normal one).
    struct LiftTiming
    {
        float advance;
        float recede;
        float rest;
    };
    [[nodiscard]] inline bool IsLiftType(GalaxyEggbert::Def::ObjectType type) noexcept
    {
        using T = GalaxyEggbert::Def::ObjectType;
        return type == T::ObjectType1 || type == T::ObjectType47 || type == T::ObjectType48;
    }
    [[nodiscard]] inline std::optional<LiftTiming> LiftPreset(GalaxyEggbert::Def::ObjectType type, int buttonIcon) noexcept
    {
        if (!IsLiftType(type))
        {
            return std::nullopt;
        }
        switch (buttonIcon)
        {
        case 21: return LiftTiming{20.0f, 20.0f, 20.0f};
        case 20: return LiftTiming{10.0f, 10.0f, 10.0f};
        case 19: return LiftTiming{6.0f, 6.0f, 3.0f};
        case 28: return LiftTiming{5.0f, 22.0f, 2.0f};
        case 121: return LiftTiming{10.0f, 10.0f, 10.0f};
        default: return std::nullopt;
        }
    }

    // Owns one in-game 3D editing session: camera and placement input,
    // palette interaction, undo history, object editing, persistence and
    // transitions to the world browser or play-test mode.
    class WorldEditor
    {
    public:
        // The palette (scripted checks select from it as a click would).
        [[nodiscard]] EditorPalette& Palette() noexcept { return palette_; }
        // Enters the browser and rescans the selected gamer's custom worlds.
        void EnterBrowser(int gamerSlot);

        [[nodiscard]] bool IsBrowsing() const noexcept { return browsing_; }

        // The copy C made (ED-0010), nullptr before one; whether P's pasting is on.
        [[nodiscard]] const Clipboard* CurrentClipboard() const noexcept
        {
            return clipboard_ ? &*clipboard_ : nullptr;
        }
        [[nodiscard]] bool IsPasting() const noexcept { return pasting_; }

        // One request the browser screen made this frame -- the caller
        // (GalaxyEggbertGame) still owns actually loading/creating the
        // .vwr file (this class has no WorldRuntime access), matching
        // this project's existing "pending signal consumed by the owning
        // class" idiom (see InteractionSystem's own class comment).
        struct BrowserRequest
        {
            bool shouldOpen = false;
            std::filesystem::path openPath; // valid when shouldOpen
            bool shouldCreateNew = false;    // a new world in the empty slot createPath
            std::filesystem::path createPath;
            bool shouldReturnToMenu = false;
        };

        // Drives the browser screen while IsBrowsing() is true -- no World
        // needed (there isn't one loaded yet).
        [[nodiscard]] BrowserRequest UpdateBrowsing(const Microsoft::Xna::Framework::Input::MouseState& mouse,
                                                    int viewportWidth, int viewportHeight,
                                                    bool backPressed = false);

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
        // GalaxyEggbertGame after ConsumePlayTestRequested() fires, to
        // know which file to load for the play-test session.
        [[nodiscard]] const std::filesystem::path& GetWorldPath() const noexcept { return worldPath_; }

        // Consumes the request raised by the dice button after it saves -- once the save's thumbnail
        // was taken (a frame later), so that it shows the world, not the play-test.
        [[nodiscard]] bool ConsumePlayTestRequested() noexcept;

        // The thumbnail a save asks for (ED-0013): the file the caller writes the editor's view to
        // after drawing this frame (CustomWorldStorage ThumbnailPath), once; empty when none.
        [[nodiscard]] std::filesystem::path ConsumeThumbnailRequest() noexcept
        {
            return std::exchange(thumbnailRequest_, {});
        }

        using Navigation = EditorNavigation;

        // Updates one editor frame. RMB looks, WASD/Ctrl/Z fly and the
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
                    CnaEngine::Graphics::Camera& camera,
                    Worlds::World& world, const Navigation& navigation = {});

        // True exactly once, right after an Update() call that placed or
        // removed a block -- the caller (GalaxyEggbertGame) should
        // respond by calling its own RebuildWorldPresentation(), the same
        // "rebuild the whole mesh on change" convention TerrainRenderer
        // already uses for LoadMission(). Clears back to false once read,
        // same "*ThisFrame()"/"Consume*()" idiom already established by
        // InteractionSystem/BlupiController.
        [[nodiscard]] bool ConsumeNeedsPresentationRebuild() noexcept;

        // The point the last Update() aimed the camera at (one unit ahead of
        // it): the camera's view is XNA's CreateLookAt of its position and
        // this point.
        [[nodiscard]] const Microsoft::Xna::Framework::Vector3& CameraTarget() const noexcept
        {
            return cameraTarget_;
        }

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const CnaEngine::Graphics::Camera& camera,
                  int viewportWidth, int viewportHeight, const Worlds::World& world);

    private:
        struct FrameInput
        {
            EditorPalette::UpdateResult palette;
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
            bool ctrlHeld = false; // place without cycling (ED-0001)
            bool musicNextHeld = false; // M: the world's next background music (OR-0014)
            bool turnHeld = false;      // O: turn the aimed tile 90 degrees (M3D-0005)
            bool variantHeld = false;   // V: the aimed block's next variant (OR-0070)
            bool helpHeld = false;      // H: the editor's keys on screen (OR-0070-0072)
            bool copyHeld = false;      // C: a box's corners, copied (ED-0010)
            bool pasteHeld = false;     // P: pasting the copy on and off
            bool clipTurnHeld = false;  // Q: the copy turned a quarter
            bool clipMirrorHeld = false; // X: the copy mirrored
            bool placeHeld = false;      // Space: the visible PLACE action
            bool stopHeld = false;
        };

        [[nodiscard]] Microsoft::Xna::Framework::Vector3 UpdateCamera(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            float dt, CnaEngine::Graphics::Camera& camera, const Navigation& navigation);
        [[nodiscard]] bool UpdatePlacementOffset(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
            EditorPalette::Action paletteAction);
        // The ray through the cursor (the mouse, or the touch point: SDL turns a touch into mouse
        // events). While the right button turns the view the cursor is captured, so the centre aims.
        [[nodiscard]] Microsoft::Xna::Framework::Vector3 AimDirection(
            const Microsoft::Xna::Framework::Vector3& forward,
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            int viewportWidth, int viewportHeight, const CnaEngine::Graphics::Camera& camera) const;
        void AimAt(const Worlds::World& world, const Microsoft::Xna::Framework::Vector3& direction);
        void UpdatePlacementPreview(const Worlds::World& world);
        [[nodiscard]] FrameInput ReadFrameInput(
            const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
            const Microsoft::Xna::Framework::Input::MouseState& mouse,
            EditorPalette::UpdateResult paletteResult) const;
        void UpdateBoxPreview(const Worlds::World& world);
        [[nodiscard]] bool HandlePlacement(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleExtendedSelection(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleRemoval(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleSessionAndHistory(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleBoxFill(const FrameInput& input, Worlds::World& world);
        // Pasting (ED-0010): while on, a click stamps the copy with its corner at the red preview
        // (CTRL+click keeps what is under its empty cells), Q turns it, X mirrors it, P or ESC stops.
        [[nodiscard]] bool HandleClipboard(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleSkyRegion(const FrameInput& input, Worlds::World& world);
        [[nodiscard]] bool HandleMusic(const FrameInput& input, Worlds::World& world);
        // O turns the aimed tile 90 degrees about the vertical (the plate-rotation metadata): a side
        // fan or wind tile then blows along Z instead of X (M3D-0005), a saw or sign faces the
        // other way. Undoable.
        [[nodiscard]] bool HandleTurn(const FrameInput& input, Worlds::World& world);
        bool turnKeyHeldLastFrame_ = false;
        bool musicKeyHeldLastFrame_ = false;
        // V cycles the aimed block through its variants (OR-0070, owner 2026-11-04: changing a
        // variant failed for many icons): the original's variant family (EditorVariants.hpp), else
        // the auto-tile family it belongs to (AutoTile.hpp kAdaptDecor: the rock's, slime's, ...
        // edge pieces), so an edge the re-tiling chose can be changed by hand. Undoable.
        [[nodiscard]] bool HandleVariant(const FrameInput& input, Worlds::World& world);
        bool variantKeyHeldLastFrame_ = false;
        // H shows the editor's keys (helpVisible_).
        bool helpKeyHeldLastFrame_ = false;
        bool helpVisible_ = false;
        bool placeKeyHeldLastFrame_ = false;
        bool stopKeyHeldLastFrame_ = false;
        bool extendedSelection_ = false;
        int selectionAnchorX_ = 0, selectionAnchorY_ = 0, selectionAnchorZ_ = 0;
        // A moving object (a lift, a creature on a patrol) just placed waits for its end: the next
        // click elsewhere sets it, a click on it again leaves it still (the original editor's
        // m_liftEnd, decdesign.cpp:479-506; OR-0072, owner 2026-11-04: placing a moving platform
        // seemed impossible, its end was only on the G and T keys).
        std::optional<std::array<std::uint16_t, 3>> pendingEnd_;
        // The record with its end at (x, y, z), its step scaled as the T key and the original's
        // PlaceDecor6 do.
        [[nodiscard]] MoveObjectRecord WithEndAt(const MoveObjectRecord& record, std::uint16_t x, std::uint16_t y,
                                                 std::uint16_t z) const;
        // The status lines over the view: the pending end's prompt, the keys (H).
        [[nodiscard]] std::vector<std::string> StatusLines() const;
        [[nodiscard]] bool HandleObjectEditing(
            const FrameInput& input, CnaEngine::Graphics::Camera& camera,
            int viewportWidth, int viewportHeight, Worlds::World& world);
        void RemoveObjectWithHistory(
            Worlds::World& world, const MoveObjectRecord& record,
            std::uint16_t anchorX, std::uint16_t anchorY, std::uint16_t anchorZ);
        void RemoveBigDecorWithHistory(
            Worlds::World& world, const BigDecorRecord& record);
        void StoreInputEdges(const FrameInput& input, bool placementOffsetKeyHeld) noexcept;

        float camX_ = 50.0f;
        float camY_ = 15.0f;
        float camZ_ = 50.0f;
        Microsoft::Xna::Framework::Vector3 cameraTarget_{0.0f, 0.0f, 0.0f};
        float yaw_ = 0.0f;   // radians; matches BlupiController's own convention:
                             // forward = (sin(yaw), -cos(yaw)) in the XZ plane, yaw 0 = facing -Z.
        float pitch_ = -0.35f; // radians, negative = looking down toward the terrain
        float flySpeed_ = 15.0f; // units/second

        bool mouseLookHeldLastFrame_ = false;
        int lastScrollWheelValue_ = 0;
        bool hasLastScrollWheelValue_ = false;

        EditorHighlightRenderer highlightRenderer_;
        bool hasHighlight_ = false;
        // A fresh custom world has no solid voxels for the DDA raycast to
        // hit. In that case Update() projects the aim ray onto y=0 and
        // exposes that valid first-placement cell through hasHighlight_,
        // while this flag preserves whether middle-click may remove a real
        // hit voxel.
        bool hasRaycastHit_ = false;
        // What the aim ray last found, kept so a cursor resting on the editor's UI (PLACE, the
        // offset buttons) still places at the last cell aimed in the view.
        enum class AimKind { None, Voxel, Ground };
        AimKind aimKind_ = AimKind::None;
        int aimCellX_ = 0;
        int aimCellY_ = 0;
        int aimCellZ_ = 0;
        int aimNormalX_ = 0;
        int aimNormalY_ = 0;
        int aimNormalZ_ = 0;
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

        EditCommandStack commandStack_;
        EditorPalette palette_;
        LiftEndpointRenderer liftEndpoints_;
        VariantCycler variants_; // the original's variant families (ED-0001)
        VariantCycler bigDecorVariants_{BigDecorVariantFamilies()}; // ED-0006
        // The auto-tiler's dice (AutoTile.hpp: the original's random twins of a variant).
        std::minstd_rand autoTileRandom_{20261102u};

        // Box-fill tool (plan.md EDITOR-105); the same box copies when C started it (ED-0010).
        bool boxFirstCornerPlaced_ = false;
        bool boxForCopy_ = false;
        std::optional<Clipboard> clipboard_; // kept across the worlds opened in one session
        bool pasting_ = false;
        bool copyKeyHeldLastFrame_ = false;
        bool pasteKeyHeldLastFrame_ = false;
        bool clipTurnKeyHeldLastFrame_ = false;
        bool clipMirrorKeyHeldLastFrame_ = false;
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
        EditorHighlightRenderer selectionHighlightRenderer_;

        // Which field Tab cycles through and OemPlus/OemMinus adjust --
        // order matches MoveObjectRecord's own declared field order. Not the
        // record's speed: since D43 every patrol keeps its authored ticks and
        // nothing reads speed (it stays in the record for the file format).
        enum class EditableField
        {
            StepAdvanceTicks,
            StepRecedeTicks,
            TimeStopStartTicks,
            TimeStopEndTicks,
        };
        static constexpr int kEditableFieldCount = 4;
        EditableField activeField_ = EditableField::StepAdvanceTicks;

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
        // The world backed up since it was opened from the browser (ED-0011): its first save keeps
        // the file as it was (CustomWorldStorage BackUpWorld), the next ones of that visit do not.
        std::filesystem::path backedUpPath_;
        std::filesystem::path thumbnailRequest_;
        // Saves @p world to worldPath_, backed up first when this visit has not yet.
        void SaveWorld(const Worlds::World& world);

        bool browsing_ = true;
        int gamerSlot_ = 0;
        EditorBrowserScreen browserScreen_;
    };
}
