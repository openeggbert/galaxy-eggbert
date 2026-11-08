#pragma once

// A gamepad in play (owner 2026-11-02: Galaxy must support gamepads). Mobile Eggbert has none (a
// touch screen and the Windows Phone keyboard), so this mapping is Galaxy's own, chosen to read as
// the keyboard does: the left stick or the D-pad turns and walks (down: the turn-around, as the
// keyboard's Down), A jumps (and climbs the helicopter), B or X is Action, RB backs up (the
// keyboard's Right Ctrl), LB crouches (Left Shift), Y looks up (Right Shift), Start pauses.
// Engine-independent: the host fills GamepadFrame from CNA's GamePadState each frame and presses
// the keys GamepadKeys() names, so the gamepad drives every phase the keyboard does (the menus'
// Escape, the cheat words aside).

namespace GalaxyEggbert::Game
{
    struct GamepadFrame
    {
        bool connected = false;
        float leftX = 0.0f, leftY = 0.0f; // the left stick, -1..1, Y up positive (XNA's convention)
        float rightX = 0.0f, rightY = 0.0f;
        float leftTrigger = 0.0f, rightTrigger = 0.0f;
        bool dpadUp = false, dpadDown = false, dpadLeft = false, dpadRight = false;
        bool a = false, b = false, x = false, y = false;
        bool leftShoulder = false, rightShoulder = false;
        bool start = false, back = false;
    };

    // The stick's dead zone and the share past which an axis counts (the game's moves are -1/0/+1,
    // as the keyboard's and the on-screen pad's).
    inline constexpr float kGamepadAxisThreshold = 0.5f;

    // The keyboard keys a gamepad frame stands for (the Keys enum's values are XNA's, so the host
    // passes them straight to KeyboardState): bits of GamepadKey.
    enum GamepadKey : unsigned
    {
        kPadUp = 1u << 0, kPadDown = 1u << 1, kPadLeft = 1u << 2, kPadRight = 1u << 3,
        kPadJump = 1u << 4,     // Left Ctrl
        kPadAction = 1u << 5,   // Space
        kPadBackUp = 1u << 6,   // Right Ctrl
        kPadCrouch = 1u << 7,   // Left Shift
        kPadLookUp = 1u << 8,   // Right Shift
        kPadPause = 1u << 9,    // Escape
        kPadFire = 1u << 11,    // F, right trigger
        kPadConfirm = 1u << 10, // Enter (A in the menus too)
    };
    [[nodiscard]] inline unsigned GamepadKeys(const GamepadFrame& now) noexcept
    {
        if (!now.connected)
        {
            return 0u;
        }
        const float t = kGamepadAxisThreshold;
        unsigned keys = 0u;
        if (now.dpadUp || now.leftY > t) keys |= kPadUp;
        if (now.dpadDown || now.leftY < -t) keys |= kPadDown;
        if (now.dpadLeft || now.leftX < -t) keys |= kPadLeft;
        if (now.dpadRight || now.leftX > t) keys |= kPadRight;
        if (now.a) keys |= kPadJump | kPadConfirm;
        if (now.b || now.x) keys |= kPadAction;
        if (now.rightTrigger > kGamepadAxisThreshold) keys |= kPadFire;
        if (now.rightShoulder) keys |= kPadBackUp;
        if (now.leftShoulder) keys |= kPadCrouch;
        if (now.y) keys |= kPadLookUp;
        if (now.start || now.back) keys |= kPadPause;
        return keys;
    }
}
