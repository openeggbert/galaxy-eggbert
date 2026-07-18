#pragma once

#include "GEEditCommandStack.hpp"
#include "GEEditorHighlightRenderer.hpp"
#include "GEEditorPalette.hpp"

#include <Easy3D/Camera3D.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <filesystem>
#include <utility>

namespace GalaxyEggbert::CNA
{
    // In-game 3D world editor (plan.md section 6, EDITOR-1xx tasks) -- lets a
    // player create/edit/save/play-test their own .vwr worlds from a new
    // GamePhase::Editor mode. Not a mobile-eggbert feature: content-creation
    // tooling, explicitly exempt from the project's faithful-remake rule
    // (see plan.md section 6 / CLAUDE.md).
    //
    // EDITOR-106 (this milestone): free-fly camera (EDITOR-101) + voxel
    // raycast/highlight (EDITOR-102) + single block place/remove/save
    // (EDITOR-103) + undo/redo (EDITOR-104) + box-fill (EDITOR-105), now
    // with a real palette (GEEditorPalette) choosing what LMB/box-fill
    // place instead of a hardcoded block type. Object editing lands in
    // later milestones.
    class GEWorldEditor
    {
    public:
        // Called once when entering the Editor phase (temporarily from the
        // F9 debug entry, later from EDITOR-107's real menu flow) -- resets
        // the free-fly camera to a sensible starting point/orientation
        // above the loaded world.
        void EnterEditing(float startX, float startY, float startZ) noexcept;

        // Sets the path Save() (see Update()'s Enter-key handling below)
        // writes to. Temporary until EDITOR-107's real per-gamer-slot world
        // browser exists to choose this.
        void SetWorldPath(std::filesystem::path path) noexcept { worldPath_ = std::move(path); }

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
        //     to the aimed-at face.
        //   - Middle click: removes the aimed-at block entirely.
        //   - Enter: saves @p world to the path set via SetWorldPath().
        //   - U: undoes the most recent block edit; R: redoes it (plain
        //     keys, not Ctrl-modified -- Left Ctrl already flies downward).
        //     The palette's own Undo/Redo/Save toolbar buttons trigger the
        //     exact same actions via a mouse click, for players who don't
        //     know the keybindings.
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

        // @p terrainTexture is the same already-loaded object-m.png texture
        // GalaxyEggbertCnaGame's own terrain rendering uses -- lent to the
        // palette so its icon grid can sample the real terrain atlas
        // directly (GEEditorPalette::Draw()'s own comment).
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const Easy3D::Camera3D& camera,
                  Microsoft::Xna::Framework::Graphics::Texture2D& terrainTexture,
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
        bool needsPresentationRebuild_ = false;

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

        std::filesystem::path worldPath_;
    };
}
