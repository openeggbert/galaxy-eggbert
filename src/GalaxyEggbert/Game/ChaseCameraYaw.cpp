#include "ChaseCameraYaw.hpp"

#include <cmath>

namespace GalaxyEggbert::Game
{
    namespace
    {
        constexpr float kPi = 3.14159265358979f;

        float SideYaw(ChaseCameraSide side, float blupiYaw) noexcept
        {
            switch (side)
            {
                case ChaseCameraSide::Right: return blupiYaw - kPi / 2.0f;
                case ChaseCameraSide::Left: return blupiYaw + kPi / 2.0f;
                case ChaseCameraSide::Behind: break;
            }
            return blupiYaw;
        }
    }

    float ChaseCameraYaw::Wrap(float angle) noexcept
    {
        angle = std::remainder(angle, 2.0f * kPi);
        return angle <= -kPi ? angle + 2.0f * kPi : angle;
    }

    void ChaseCameraYaw::Reset(float blupiYaw) noexcept
    {
        m_yaw = Wrap(blupiYaw);
        m_idleSeconds = 0.0f;
        m_side = ChaseCameraSide::Behind;
    }

    void ChaseCameraYaw::Update(const ChaseCameraInput& input, float dt) noexcept
    {
        m_idleSeconds = input.idle ? m_idleSeconds + dt : 0.0f;
        if (input.facesViewer)
        {
            // Always his right, even against a wall: the wall pull-in then brings the camera
            // closer instead (user decision 2026-10-24).
            m_side = ChaseCameraSide::Right;
        }
        else if (m_idleSeconds >= kIdleDelaySeconds)
        {
            const bool keep = (m_side == ChaseCameraSide::Right && input.rightOpen) ||
                              (m_side == ChaseCameraSide::Left && input.leftOpen);
            if (!keep)
            {
                // The side nearer to the camera; his right, the sprites' side, on a tie (straight
                // behind him).
                constexpr float kTie = 1e-3f;
                const float toRight = std::fabs(Wrap(SideYaw(ChaseCameraSide::Right, input.blupiYaw) - m_yaw));
                const float toLeft = std::fabs(Wrap(SideYaw(ChaseCameraSide::Left, input.blupiYaw) - m_yaw));
                if (input.rightOpen && (toRight <= toLeft + kTie || !input.leftOpen))
                {
                    m_side = ChaseCameraSide::Right;
                }
                else if (input.leftOpen)
                {
                    m_side = ChaseCameraSide::Left;
                }
                else
                {
                    m_side = ChaseCameraSide::Behind;
                }
            }
        }
        else
        {
            m_side = ChaseCameraSide::Behind;
        }

        // Framerate-independent exponential approach, like the camera's position damping.
        const float rate = m_side == ChaseCameraSide::Behind ? kFollowPerSecond : kOrbitPerSecond;
        const float remaining = Wrap(SideYaw(m_side, input.blupiYaw) - m_yaw);
        m_yaw = Wrap(m_yaw + remaining * (1.0f - std::exp(-rate * dt)));
    }
}
