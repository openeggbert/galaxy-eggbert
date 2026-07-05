#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <cstdint>

namespace GalaxyEggbert::CNA
{
    // Invisible, collision-only movement placeholder for early debug testing
    // of 3D navigation (plan.md E3D-MIG-060). Grid-based collision against
    // the loaded World: stands on/climbs terrain up to a 1-block step
    // (step-up traversal — one of CLAUDE.md's allowed natural 3D
    // adaptations), falls under gravity otherwise, and blocks movement into
    // taller obstacles. Tank controls (turn + forward/back), matching
    // GalaxyEggbertSimple3D's already-shipped scheme — see Step() below.
    //
    // No 3D Blupi model exists yet (2026-07-05) — the real in-world sprite
    // (E3D-MIG-061..064) waits for it. In the meantime this tracks just
    // enough state (facing yaw, a coarse Stop/March/Jump/Down/Up animation
    // state) to drive a first-person camera and a 2D screen-corner
    // animation indicator — see GalaxyEggbertCnaGame. The frame tables
    // mirror GalaxyEggbertSimple3D's already-approved GEBlupiController.cpp
    // (galaxy-eggbert's own code, not a fresh mobile-eggbert transcription).
    //
    // Deliberately engine-agnostic (only depends on GalaxyEggbert::Worlds)
    // so Step() can be scripted/tested without a live window or keyboard —
    // see tools/VerifyBlupiMovement.cpp. The CNA target reads Keyboard
    // state itself and calls Step().
    class GEBlupiController
    {
    public:
        static constexpr float kMoveSpeed = 5.5f; // matches Simple3D::GEBlupiController
        static constexpr float kTurnSpeed = 3.14159265f; // rad/s (180 deg/s, matches Simple3D)
        static constexpr float kJumpSpeed = 12.0f;
        static constexpr float kGravity   = 25.0f;
        static constexpr float kFallLimit = -10.0f;
        static constexpr float kStepLimit = 1.0f;
        static constexpr float kAnimFps   = 8.0f; // matches Simple3D::GEBlupiController

        enum class AnimState : std::uint8_t { Stop, March, Jump, Down, Up };

        void SetPosition(float x, float y, float z) noexcept;

        // Sets facing directly — useful for tests/tools that need to face a
        // specific direction without stepping turnInput to get there (see
        // tools/VerifyBlupiMovement.cpp). Not used by normal gameplay input.
        void SetYaw(float yaw) noexcept { m_yaw = yaw; }

        [[nodiscard]] float GetX() const noexcept { return m_x; }
        [[nodiscard]] float GetY() const noexcept { return m_y; }
        [[nodiscard]] float GetZ() const noexcept { return m_z; }
        [[nodiscard]] bool IsOnGround() const noexcept { return m_onGround; }

        // Facing angle in radians, 0 = looking toward -Z. Updated every Step()
        // by turnInput (see below) — unlike a strafe-style controller, yaw is
        // driven directly by turning, not derived from movement direction.
        [[nodiscard]] float GetYaw() const noexcept { return m_yaw; }

        // Current coarse animation state and the blupi.png icon index to
        // display for it right now (10 columns, 60x60 px tiles — see
        // GalaxyEggbertSimple3D::GEBlupiController for the same convention).
        [[nodiscard]] AnimState GetAnimState() const noexcept { return m_animState; }
        [[nodiscard]] int GetAnimIcon() const noexcept;

        // Tank controls, matching GalaxyEggbertSimple3D's already-shipped
        // scheme (GalaxyEggbertSimpleGame::SetupInput's "Move" axis) and
        // mobile-eggbert's own control feel: turnInput (-1/0/+1, Left/Right)
        // rotates facing; moveInput (-1/0/+1, Down/Up) moves forward/back
        // along the current facing direction — arrows are not a strafe pad.
        // crouchHeld (LShift)/lookUpHeld (RShift) mirror Simple3D's Down/Up
        // BlupiState and don't affect collision, only animation state and
        // (via the CNA game's camera) eye height/look pitch.
        void Step(const Worlds::World& world, float turnInput, float moveInput,
                  bool jumpPressed, bool crouchHeld, bool lookUpHeld, float dt);

    private:
        [[nodiscard]] static bool IsSolidAt(const Worlds::World& world, int gx, int gy, int gz);
        [[nodiscard]] static int GroundHeightAt(const Worlds::World& world, int gx, int gz);
        void TryMoveAxis(const Worlds::World& world, float ddx, float ddz);
        void UpdateAnim(bool moving, bool crouchHeld, bool lookUpHeld, float dt);

        float m_x = 0.0f;
        float m_y = 1.0f;
        float m_z = 0.0f;
        float m_velocityY = 0.0f;
        bool m_onGround = false;

        float m_yaw = 0.0f;
        AnimState m_animState = AnimState::Stop;
        int m_animPhase = 0;
        float m_animTimer = 0.0f;
    };
}
