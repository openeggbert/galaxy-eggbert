#include "GEBlupiController.hpp"
#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr int kWorldCenterX = GEWorldRuntime::kWorldCenterX;
        constexpr int kWorldCenterZ = GEWorldRuntime::kWorldCenterZ;

        int ClampGrid(int v, int blocksPerAxis)
        {
            return std::clamp(v, 0, blocksPerAxis - 1);
        }

        // blupi.png icon indices, ported from GalaxyEggbertSimple3D's own
        // already-approved GEBlupiController.cpp (kStopFrames/kMarchFrames/
        // kJumpFrames) — not a fresh mobile-eggbert transcription.
        constexpr int kStopFrames[]  = {0};
        constexpr int kMarchFrames[] = {5, 6, 7, 8, 9, 10};
        constexpr int kJumpFrames[]  = {17, 18, 19};
    }

    void GEBlupiController::SetPosition(float x, float y, float z) noexcept
    {
        m_x = x;
        m_y = y;
        m_z = z;
        m_velocityY = 0.0f;
        m_onGround = false;
    }

    bool GEBlupiController::IsSolidAt(const Worlds::World& world, int gx, int gy, int gz)
    {
        return !world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                               static_cast<std::uint16_t>(gz)).isAir();
    }

    int GEBlupiController::GroundHeightAt(const Worlds::World& world, int gx, int gz)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int y = blocksPerAxis - 1; y >= 0; --y)
        {
            if (IsSolidAt(world, gx, y, gz))
            {
                return y + 1;
            }
        }
        return 0;
    }

    void GEBlupiController::TryMoveAxis(const Worlds::World& world, float ddx, float ddz)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const float candidateX = m_x + ddx;
        const float candidateZ = m_z + ddz;

        const int gx = ClampGrid(static_cast<int>(std::lround(candidateX + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(candidateZ + kWorldCenterZ)), blocksPerAxis);

        const int targetGroundY = GroundHeightAt(world, gx, gz);

        // Allow the move if the destination column's ground is at most
        // kStepLimit above the current feet Y (step-up); any amount lower
        // is always fine (falling is handled by the vertical pass in Step()).
        if (static_cast<float>(targetGroundY) <= m_y + kStepLimit)
        {
            m_x = candidateX;
            m_z = candidateZ;
            if (m_onGround && static_cast<float>(targetGroundY) > m_y)
            {
                m_y = static_cast<float>(targetGroundY);
            }
        }
    }

    void GEBlupiController::Step(const Worlds::World& world, float dx, float dz, bool jumpPressed, float dt)
    {
        const bool moving = (dx != 0.0f || dz != 0.0f);

        if (dx != 0.0f)
        {
            TryMoveAxis(world, dx * kMoveSpeed * dt, 0.0f);
        }
        if (dz != 0.0f)
        {
            TryMoveAxis(world, 0.0f, dz * kMoveSpeed * dt);
        }

        if (moving)
        {
            // 0 rad = facing -Z, matching the forward vector the CNA camera
            // derives from GetYaw() (sin(yaw), 0, -cos(yaw)).
            m_yaw = std::atan2(dx, -dz);
        }

        if (m_onGround && jumpPressed)
        {
            m_velocityY = kJumpSpeed;
            m_onGround = false;
        }

        m_velocityY = std::max(m_velocityY - kGravity * dt, kFallLimit);
        float newY = m_y + m_velocityY * dt;

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        const int groundY = GroundHeightAt(world, gx, gz);

        if (newY <= static_cast<float>(groundY))
        {
            newY = static_cast<float>(groundY);
            m_velocityY = 0.0f;
            m_onGround = true;
        }
        else
        {
            m_onGround = false;
        }
        m_y = newY;

        UpdateAnim(moving, dt);
    }

    void GEBlupiController::UpdateAnim(bool moving, float dt)
    {
        const AnimState newState = !m_onGround ? AnimState::Jump
                                  : moving      ? AnimState::March
                                                : AnimState::Stop;
        if (newState != m_animState)
        {
            m_animState = newState;
            m_animPhase = 0;
            m_animTimer = 0.0f;
            return;
        }

        m_animTimer += dt;
        const float frameDuration = 1.0f / kAnimFps;
        if (m_animTimer >= frameDuration)
        {
            m_animTimer -= frameDuration;
            ++m_animPhase;
        }
    }

    int GEBlupiController::GetAnimIcon() const noexcept
    {
        switch (m_animState)
        {
            case AnimState::March:
                return kMarchFrames[m_animPhase % (sizeof(kMarchFrames) / sizeof(kMarchFrames[0]))];
            case AnimState::Jump:
                return kJumpFrames[m_animPhase % (sizeof(kJumpFrames) / sizeof(kJumpFrames[0]))];
            case AnimState::Stop:
            default:
                return kStopFrames[m_animPhase % (sizeof(kStopFrames) / sizeof(kStopFrames[0]))];
        }
    }
}
