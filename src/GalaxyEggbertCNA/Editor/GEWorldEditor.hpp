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
    // In-game 3D world editor (plan.md section 6, EDITOR-1xx tasks) -- lets a
    // player create/edit/save/play-test their own .vwr worlds from a new
    // GamePhase::Editor mode. Not a mobile-eggbert feature: content-creation
    // tooling, explicitly exempt from the project's faithful-remake rule
    // (see plan.md section 6 / CLAUDE.md).
    //
    // EDITOR-108 (an earlier milestone): free-fly camera (EDITOR-101) + voxel
    // raycast/highlight (EDITOR-102) + single block place/remove/save
    // (EDITOR-103) + undo/redo (EDITOR-104) + box-fill (EDITOR-105) + a
    // real block palette (EDITOR-106) + a real per-gamer-slot world
    // browser (EDITOR-107), now with a Play-Test toolbar button that saves
    // and hands off to GalaxyEggbertCnaGame for a real gameplay session
    // against the just-saved world. EDITOR-109 added stationary MoveObject
    // placement in Objects mode; EDITOR-110 (this milestone) adds selecting
    // an already-placed one and editing it -- see Update()'s own comment
    // for the G/T/Tab/OemPlus/OemMinus/Delete bindings, keyboard-only, same
    // "no toolbar button, no text label" precedent already set by the
    // box-fill tool (F/Escape).
    class GEWorldEditor
    {
    public:
        // Enters (or re-enters) the browser for @p gamerSlot -- called from
        // the Init screen's new Editor button, and again internally
        // whenever the toolbar's Back button is pressed while editing.
        // Rescans disk (GECustomWorldStorage::ListCustomWorlds) fresh each
        // time. IsBrowsing() reports true afterward, until the caller
        // (GalaxyEggbertCnaGame) actually loads/creates a world and calls
        // ExitBrowser().
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

        // Sets the path Save() (see Update()'s Enter-key handling below)
        // and the toolbar's Save button write to -- set by
        // GalaxyEggbertCnaGame right after it loads/creates the world this
        // BrowserRequest pointed at.
        void SetWorldPath(std::filesystem::path path) noexcept { worldPath_ = std::move(path); }

        // The path currently being edited (plan.md EDITOR-108) -- read by
        // GalaxyEggbertCnaGame after ConsumePlayTestRequested() fires, to
        // know which file to load for the play-test session.
        [[nodiscard]] const std::filesystem::path& GetWorldPath() const noexcept { return worldPath_; }

        // True exactly once, right after an Update() call whose Play-Test
        // toolbar button was clicked -- Update() already saved @p world to
        // worldPath_ itself before setting this (same "save before
        // testing" behavior as pressing Save first, then Play-Test). The
        // caller (GalaxyEggbertCnaGame) still owns actually switching to
        // GamePhase::Play and loading the saved file fresh (this class has
        // no GEWorldRuntime/phase access), same "*ThisFrame()"/"Consume*()"
        // idiom as ConsumeNeedsPresentationRebuild() above.
        [[nodiscard]] bool ConsumePlayTestRequested() noexcept;

        // Reads keyboard/mouse and flies camera around, then raycasts from
        // the (possibly just-moved) camera into @p world to find whichever
        // block it's currently aiming at (stored for Draw() to visualize).
        // While the right mouse button is held, mouse deltas drive
        // yaw/pitch (relative mouse mode + cursor capture) and the cursor
        // is hidden from the OS; releasing it frees the cursor again for
        // left/middle-click tool handling. WASD move along the camera's
        // own forward/right axes, Space/Left Ctrl move along world
        // up/down, the scroll wheel adjusts fly speed.
        //
        // Editing tools (edge-triggered, once per press -- RMB is already
        // taken by camera look, so this deliberately isn't the usual
        // Minecraft-style left=break/right=place binding):
        //   - Left click: places the palette's currently selected block
        //     type (GEEditorPalette::SelectedBlockType(), clicked from the
        //     on-screen palette grid -- see Draw()) at the cell adjacent
        //     to the aimed-at face. While the palette is in Objects mode
        //     (its 6th toolbar button toggles), the same click instead
        //     places a stationary MoveObjectRecord (posEnd == posStart) of
        //     the selected ObjectType there -- an enemy/pickup/lift, which
        //     GEWorldRuntime::ResyncFromWorld() picks up on the caller's
        //     next RebuildWorldPresentation(), so it shows up as a real
        //     billboard immediately without a disk round-trip.
        //   - Middle click: removes the aimed-at block entirely.
        //   - Enter: saves @p world to the path set via SetWorldPath().
        //   - U: undoes the most recent block edit; R: redoes it (plain
        //     keys, not Ctrl-modified -- Left Ctrl already flies downward).
        //     The palette's own Undo/Redo/Save toolbar buttons trigger the
        //     exact same actions via a mouse click, for players who don't
        //     know the keybindings; its 4th (Back) button returns to the
        //     browser (EnterBrowser() again, for the same gamer slot); its
        //     5th (Play-Test) button saves @p world then requests a real
        //     gameplay session -- see ConsumePlayTestRequested() below.
        //   - F: box-fill tool. First press marks the aimed-at cell as
        //     corner A; while a corner is marked, the highlight tracks a
        //     live box between corner A and wherever the raycast currently
        //     aims (fly anywhere in between -- the two corners can come
        //     from completely different camera angles/distances, which is
        //     what gives a true 3D cuboid, not just a flat footprint). A
        //     second F press marks corner B and immediately fills the
        //     whole box with the palette's selected block type as ONE undo
        //     command (only the cells that actually changed); Escape
        //     cancels back to single-cell picking with no world change.
        //   - G: selects whichever already-placed MoveObject's billboard
        //     projects closest to screen center (plan.md EDITOR-110) --
        //     objects aren't voxel-grid raycast targets like blocks, so
        //     picking is nearest-screen-space instead, reusing
        //     GEHud::ProjectWorldToHudSpace() (no new projection math).
        //     Pressing G with nothing within the pick radius clears the
        //     current selection. The selection is highlighted (a distinct
        //     color from the aim-crosshair/box-fill overlay).
        //   - T: while an object is selected, sets its posEnd to wherever
        //     the crosshair currently aims -- gives it a real patrol path
        //     (posStart != posEnd is the existing "this object moves"
        //     signal already used by GEEditCommandStack/GEWorldRuntime).
        //   - Tab: cycles which of the selected object's 5 numeric fields
        //     (speed, then the 4 real patrol-turn-timing fields, in
        //     MoveObjectRecord's own declared order) OemPlus/OemMinus
        //     below adjust.
        //   - OemPlus/OemMinus (the +/- keys): nudge the active field by a
        //     fixed editor-UX step (not a transcribed real constant, same
        //     category as MoveObjectRecord's own placeholder timing
        //     defaults), clamped so speed/ticks never go negative.
        //   - Delete: removes the selected object entirely and clears the
        //     selection.
        //   All five are edge-triggered on the press, same as every other
        //   tool key above, and each is one MoveObjectEdit undo command
        //   (U/R undo/redo them like any other edit) -- no new toolbar
        //   button exists for any of them, matching the box-fill tool's own
        //   keyboard-only precedent (every tool already has a working
        //   binding; toolbar buttons are a discoverability convenience on
        //   top, not a functional requirement -- see GEEditorPalette's own
        //   class comment).
        //   - Left/Right arrow keys (EDITOR-111): step @p world's
        //     skyRegion() down/up by 1, wrapping 0<->31 (world.hpp's own
        //     documented valid range) -- one SkyRegionEdit undo command per
        //     press, same edge-triggered/undoable shape as every tool
        //     above. No live visual feedback needed beyond the real
        //     background itself: ConsumeNeedsPresentationRebuild() fires
        //     the same way a block edit does, so the caller's
        //     RebuildWorldPresentation() reloads Content/backgrounds/
        //     decorNNN.png (or falls back to a flat clear color for the 4
        //     ids with no real art, exactly as it already does for a
        //     freshly-loaded world) immediately -- the player SEES the sky
        //     change, no text readout needed (this class draws no text at
        //     all, see GEEditorPalette's own class comment). The palette's
        //     own SkyRegionPrev/Next toolbar buttons trigger the identical
        //     action via a mouse click, same "keyboard-first, toolbar
        //     button as discoverability convenience" precedent as
        //     Undo/Redo/Save/Back/PlayTest/BoxFill above.
        // Call ConsumeNeedsPresentationRebuild() after Update() returns to
        // find out whether @p world was actually mutated this frame.
        //
        // @p world is in its own RAW GRID space (see GEVoxelRaycast.hpp);
        // the camera's own position/direction (render space, shifted by
        // -GEWorldRuntime::kWorldCenterX/Z from that) is converted
        // internally -- callers never need to apply this shift themselves.
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

        // @p terrainTexture/elementTexture/exploTexture/blupiTexture/
        // blupi1Texture are the same already-loaded object-m.png/
        // element.png/explo.png/blupi.png/blupi1.png textures
        // GalaxyEggbertCnaGame's own terrain/object rendering uses -- lent
        // to the palette so its Objects-mode icon grid can draw each
        // type's real sprite (GEEditorPalette::Draw()'s own comment).
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const Easy3D::Camera3D& camera,
                  Microsoft::Xna::Framework::Graphics::Texture2D& terrainTexture,
                  Microsoft::Xna::Framework::Graphics::Texture2D& elementTexture,
                  Microsoft::Xna::Framework::Graphics::Texture2D& exploTexture,
                  Microsoft::Xna::Framework::Graphics::Texture2D& blupiTexture,
                  Microsoft::Xna::Framework::Graphics::Texture2D& blupi1Texture,
                  int viewportWidth, int viewportHeight);

    private:
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
        float highlightX_ = 0.0f;
        float highlightY_ = 0.0f;
        float highlightZ_ = 0.0f;
        // Raw-grid-space hit cell + outward face normal from this frame's
        // raycast -- kept alongside the render-space highlight* fields
        // above so Update()'s place/remove handling doesn't need to
        // reverse the render-space shift a second time.
        std::uint16_t hitCellX_ = 0;
        std::uint16_t hitCellY_ = 0;
        std::uint16_t hitCellZ_ = 0;
        std::int8_t hitNormalX_ = 0;
        std::int8_t hitNormalY_ = 0;
        std::int8_t hitNormalZ_ = 0;

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
