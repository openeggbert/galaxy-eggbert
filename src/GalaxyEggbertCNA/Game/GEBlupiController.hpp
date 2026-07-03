#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

namespace GalaxyEggbert::CNA
{
    // Invisible, collision-only movement placeholder for early debug testing
    // of 3D navigation (plan.md E3D-MIG-060) — no sprite, no animation state
    // machine yet (that's E3D-MIG-061..064). Grid-based collision against
    // the loaded World: stands on/climbs terrain up to a 1-block step
    // (step-up traversal — one of CLAUDE.md's allowed natural 3D
    // adaptations), falls under gravity otherwise, and blocks movement into
    // taller obstacles.
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

        void SetPosition(float x, float y, float z) noexcept;

        [[nodiscard]] float GetX() const noexcept { return m_x; }
        [[nodiscard]] float GetY() const noexcept { return m_y; }
        [[nodiscard]] float GetZ() const noexcept { return m_z; }
        [[nodiscard]] bool IsOnGround() const noexcept { return m_onGround; }

        // dx/dz: movement intent along world axes (e.g. -1/0/+1 from input).
        void Step(const Worlds::World& world, float dx, float dz, bool jumpPressed, float dt);

    private:
        [[nodiscard]] static bool IsSolidAt(const Worlds::World& world, int gx, int gy, int gz);
        [[nodiscard]] static int GroundHeightAt(const Worlds::World& world, int gx, int gz);
        void TryMoveAxis(const Worlds::World& world, float ddx, float ddz);

        float m_x = 0.0f;
        float m_y = 1.0f;
        float m_z = 0.0f;
        float m_velocityY = 0.0f;
        bool m_onGround = false;
    };
}
