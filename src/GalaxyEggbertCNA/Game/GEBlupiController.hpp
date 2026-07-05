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
    // taller obstacles.
    //
    // No 3D Blupi model exists yet (2026-07-05) — the real in-world sprite
    // (E3D-MIG-061..064) waits for it. In the meantime this tracks just
    // enough state (facing yaw, a coarse idle/walk/jump animation state) to
    // drive a first-person camera and a 2D screen-corner animation
    // indicator — see GalaxyEggbertCnaGame. The frame tables mirror
    // GalaxyEggbertSimple3D's already-approved GEBlupiController.cpp
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
        static constexpr float kJumpSpeed = 12.0f;
        static constexpr float kGravity   = 25.0f;
        static constexpr float kFallLimit = -10.0f;
        static constexpr float kStepLimit = 1.0f;
        static constexpr float kAnimFps   = 8.0f; // matches Simple3D::GEBlupiController

        enum class AnimState : std::uint8_t { Stop, March, Jump };

        void SetPosition(float x, float y, float z) noexcept;

        [[nodiscard]] float GetX() const noexcept { return m_x; }
        [[nodiscard]] float GetY() const noexcept { return m_y; }
        [[nodiscard]] float GetZ() const noexcept { return m_z; }
        [[nodiscard]] bool IsOnGround() const noexcept { return m_onGround; }

        // Facing angle in radians, 0 = looking toward -Z. Only updated while
        // dx/dz input is non-zero; holds its last value while idle/airborne.
        [[nodiscard]] float GetYaw() const noexcept { return m_yaw; }

        // Current coarse animation state and the blupi.png icon index to
        // display for it right now (10 columns, 60x60 px tiles — see
        // GalaxyEggbertSimple3D::GEBlupiController for the same convention).
        [[nodiscard]] AnimState GetAnimState() const noexcept { return m_animState; }
        [[nodiscard]] int GetAnimIcon() const noexcept;

        // dx/dz: movement intent along world axes (e.g. -1/0/+1 from input).
        void Step(const Worlds::World& world, float dx, float dz, bool jumpPressed, float dt);

    private:
        [[nodiscard]] static bool IsSolidAt(const Worlds::World& world, int gx, int gy, int gz);
        [[nodiscard]] static int GroundHeightAt(const Worlds::World& world, int gx, int gz);
        void TryMoveAxis(const Worlds::World& world, float ddx, float ddz);
        void UpdateAnim(bool moving, float dt);

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
