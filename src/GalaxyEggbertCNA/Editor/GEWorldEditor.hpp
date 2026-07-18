#pragma once

#include <Easy3D/Camera3D.hpp>
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
    // EDITOR-101 (this milestone): free-fly camera only. Raycasting
    // (EDITOR-102) and block/object editing land in later milestones.
    class GEWorldEditor
    {
    public:
        // Called once when entering the Editor phase (temporarily from the
        // F9 debug entry, later from EDITOR-107's real menu flow) -- resets
        // the free-fly camera to a sensible starting point/orientation
        // above the loaded world.
        void EnterEditing(float startX, float startY, float startZ) noexcept;

        // Reads keyboard/mouse and flies camera around. While the right
        // mouse button is held, mouse deltas drive yaw/pitch (relative
        // mouse mode + cursor capture) and the cursor is hidden from the
        // OS; releasing it frees the cursor again for future tool-click
        // handling (EDITOR-103+). WASD move along the camera's own
        // forward/right axes, Space/Left Ctrl move along world up/down,
        // the scroll wheel adjusts fly speed.
        void Update(const Microsoft::Xna::Framework::Input::KeyboardState& keyboard,
                    const Microsoft::Xna::Framework::Input::MouseState& mouse,
                    float dt, int viewportWidth, int viewportHeight,
                    Easy3D::Camera3D& camera);

        void Draw();

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
    };
}
