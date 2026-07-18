#pragma once

#include "GEEditorHighlightRenderer.hpp"

#include <Easy3D/Camera3D.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Input/Keyboard.hpp>
#include <Microsoft/Xna/Framework/Input/Mouse.hpp>

namespace GalaxyEggbert::CNA
{
    // In-game 3D world editor (plan.md section 6, EDITOR-1xx tasks) -- lets a
    // player create/edit/save/play-test their own .vwr worlds from a new
    // GamePhase::Editor mode. Not a mobile-eggbert feature: content-creation
    // tooling, explicitly exempt from the project's faithful-remake rule
    // (see plan.md section 6 / CLAUDE.md).
    //
    // EDITOR-102 (this milestone): free-fly camera (EDITOR-101) + a voxel
    // raycast each frame, tracked with a translucent highlight cube. Block/
    // object editing land in later milestones.
    class GEWorldEditor
    {
    public:
        // Called once when entering the Editor phase (temporarily from the
        // F9 debug entry, later from EDITOR-107's real menu flow) -- resets
        // the free-fly camera to a sensible starting point/orientation
        // above the loaded world.
        void EnterEditing(float startX, float startY, float startZ) noexcept;

        // Reads keyboard/mouse and flies camera around, then raycasts from
        // the (possibly just-moved) camera into @p world to find whichever
        // block it's currently aiming at (stored for Draw() to visualize).
        // While the right mouse button is held, mouse deltas drive
        // yaw/pitch (relative mouse mode + cursor capture) and the cursor
        // is hidden from the OS; releasing it frees the cursor again for
        // future tool-click handling (EDITOR-103+). WASD move along the
        // camera's own forward/right axes, Space/Left Ctrl move along
        // world up/down, the scroll wheel adjusts fly speed.
        //
        // @p world is in its own RAW GRID space (see GEVoxelRaycast.hpp);
        // the camera's own position/direction (render space, shifted by
        // -GEWorldRuntime::kWorldCenterX/Z from that) is converted
        // internally -- callers never need to apply this shift themselves.
        void Update(const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
                    const Microsoft::Xna::Framework::Input::MouseState& mouse,
                    float dt, int viewportWidth, int viewportHeight,
                    Easy3D::Camera3D& camera,
                    const Worlds::World& world);

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
    };
}
