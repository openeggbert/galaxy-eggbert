#pragma once

#include "GEEditorHighlightRenderer.hpp"

#include <Easy3D/Camera3D.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

#include <cstdint>
#include <filesystem>

namespace GalaxyEggbert::CNA
{
    // In-game 3D world editor (plan.md section 6, EDITOR-1xx tasks) -- lets a
    // player create/edit/save/play-test their own .vwr worlds from a new
    // GamePhase::Editor mode. Not a mobile-eggbert feature: content-creation
    // tooling, explicitly exempt from the project's faithful-remake rule
    // (see plan.md section 6 / CLAUDE.md).
    //
    // EDITOR-103 (this milestone): free-fly camera (EDITOR-101) + voxel
    // raycast/highlight (EDITOR-102), now with single block place/remove
    // and save/load. Undo/redo, box-fill, the real palette UI, and object
    // editing land in later milestones.
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
        //   - Left click: places BlockTypes::RockPile (a temporary fixed
        //     type until EDITOR-106's real palette exists) at the cell
        //     adjacent to the aimed-at face.
        //   - Middle click: removes the aimed-at block entirely.
        //   - Enter: saves @p world to the path set via SetWorldPath().
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

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  const Easy3D::Camera3D& camera);

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
        bool needsPresentationRebuild_ = false;

        std::filesystem::path worldPath_;
    };
}
