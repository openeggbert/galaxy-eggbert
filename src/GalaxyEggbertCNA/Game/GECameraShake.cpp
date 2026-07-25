#include "GECameraShake.hpp"

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Verbatim transcription of the real `Tables::table_decor_action[519]`
        // (mobile-eggbert `Tables.cpp:2027-2081`, data-table transcription
        // approved by the user 2026-07-14) -- a flat record list, each record
        // `{actionId, frameCount, dx1,dy1, dx2,dy2, ..., dxN,dyN}`, terminated
        // by a 0. actionId matches the real `GalaxyEggbert::Def::DecorAction` enum's raw values:
        // 1=Small, 2=Big, 5=Electric (3/4 are unused gaps in the real enum
        // too). Kept as one flat array (not pre-split into 3 named tables)
        // specifically so the walk below can mirror the real
        // `DecorNextAction()`'s own record-stride loop exactly, rather than
        // risk a transcription error re-grouping the 192-entry Electric
        // segment by hand.
        constexpr short kTableDecorAction[519] = {
            1, 32, -4, 4, 4, -3, -4, 2, 4, 5,
            -4, -1, 4, 2, -4, -4, 4, -3, -3, 2,
            3, 2, -3, -5, 3, 4, -3, 5, 3, -2,
            -3, 5, 3, 4, -2, -2, 2, 4, -2, -2,
            2, -2, -2, -4, 2, 2, -2, -2, 2, -3,
            -1, -3, 1, -2, -1, -1, 1, 2, -1, -2,
            1, -1, -1, 1, 1, 2, 2, 32, -4, 0,
            4, 0, -4, 0, 4, 0, -4, 0, 4, 0,
            -4, 0, 4, 0, -3, 0, 3, 0, -3, 0,
            3, 0, -3, 0, 3, 0, -3, 0, 3, 0,
            -2, 0, 2, 0, -2, 0, 2, 0, -2, 0,
            2, 0, -2, 0, 2, 0, -1, 0, 1, 0,
            -1, 0, 1, 0, -1, 0, 1, 0, -1, 0,
            1, 0, 5, 192, 0, -32, 0, 32, 0, -16,
            0, 6, 0, -8, 0, 8, 0, -4, 0, 4,
            0, -2, 0, 2, -7, 0, -6, 0, -5, 0,
            -4, 0, -2, 0, 0, 0, 2, 0, 4, 0,
            5, 0, 6, 0, 7, 0, 7, 0, 6, 0,
            5, 0, 4, 0, 2, 0, 0, 0, -2, 0,
            -4, 0, -5, 0, -6, 0, -7, 0, -7, 0,
            -6, 0, -5, 0, -4, 0, -2, 0, 0, 0,
            2, 0, 4, 0, 5, 0, 6, 0, 7, 0,
            7, 0, 6, 0, 5, 0, 4, 0, 2, 0,
            0, 0, -2, 0, -4, 0, -5, 0, -6, 0,
            -7, 0, -7, 0, -6, 0, -5, 0, -4, 0,
            -2, 0, 0, 0, 2, 0, 4, 0, 5, 0,
            6, 0, 7, 0, 7, 0, 6, 0, 5, 0,
            4, 0, 2, 0, 0, 0, -2, 0, -4, 0,
            -5, 0, -6, 0, -7, 0, -7, 0, -6, 0,
            -5, 0, -4, 0, -2, 0, 0, 0, 2, 0,
            4, 0, 5, 0, 6, 0, 7, 0, 7, 0,
            6, 0, 5, 0, 4, 0, 2, 0, 0, 0,
            -2, 0, -4, 0, -5, 0, -6, 0, -7, 0,
            -7, 0, -6, 0, -5, 0, -4, 0, -2, 0,
            0, 0, 2, 0, 4, 0, 5, 0, 6, 0,
            7, 0, 7, 0, 6, 0, 5, 0, 4, 0,
            2, 0, 0, 0, -2, 0, -4, 0, -5, 0,
            -6, 0, -7, 0, -7, 0, -6, 0, -5, 0,
            -4, 0, -2, 0, 0, 0, 2, 0, 4, 0,
            5, 0, 6, 0, 7, 0, 7, 0, 6, 0,
            5, 0, 4, 0, 2, 0, 0, 0, -2, 0,
            -4, 0, -5, 0, -6, 0, -7, 0, -7, 0,
            -6, 0, -5, 0, -4, 0, -2, 0, 0, 0,
            2, 0, 4, 0, 5, 0, 6, 0, 7, 0,
            7, 0, 6, 0, 5, 0, 4, 0, 2, 0,
            0, 0, -2, 0, -4, 0, -5, 0, -6, 0,
            -7, 0, -7, 0, -6, 0, -5, 0, -4, 0,
            -2, 0, 0, 0, 2, 0, 4, 0, 5, 0,
            6, 0, 7, 0, 7, 0, 6, 0, 5, 0,
            4, 0, 2, 0, 0, 0, -2, 0, -4, 0,
            -5, 0, -6, 0, -7, 0, -7, 0, -6, 0,
            -5, 0, -4, 0, -2, 0, -1, 0, 0
        };

        int RawActionId(CameraShakeType type)
        {
            switch (type)
            {
                case CameraShakeType::Small:    return 1;
                case CameraShakeType::Big:       return 2;
                case CameraShakeType::Electric: return 5;
                default:                        return 0;
            }
        }
    }

    void GECameraShake::Trigger(CameraShakeType type) noexcept
    {
        m_active = type;
        m_phase = 0;
        m_tickTimer = 0.0f;
        m_offsetX = 0.0f;
        m_offsetY = 0.0f;
    }

    void GECameraShake::Update(float dt) noexcept
    {
        if (m_active == CameraShakeType::None)
        {
            m_offsetX = 0.0f;
            m_offsetY = 0.0f;
            return;
        }

        m_tickTimer += dt;
        while (m_tickTimer >= kTickSeconds && m_active != CameraShakeType::None)
        {
            m_tickTimer -= kTickSeconds;

            const int wantedId = RawActionId(m_active);
            int i = 0;
            for (; kTableDecorAction[i] != 0; i += 2 + kTableDecorAction[i + 1] * 2)
            {
                if (kTableDecorAction[i] != wantedId)
                {
                    continue;
                }
                const int frameCount = kTableDecorAction[i + 1];
                if (m_phase < frameCount)
                {
                    // Real x3 multiplier (Decor.cpp:1367-1368), confirmed
                    // via direct source read -- not an approximation.
                    m_offsetX = 3.0f * static_cast<float>(kTableDecorAction[i + 2 + m_phase * 2]);
                    m_offsetY = 3.0f * static_cast<float>(kTableDecorAction[i + 2 + m_phase * 2 + 1]);
                    ++m_phase;
                }
                else
                {
                    m_active = CameraShakeType::None;
                    m_offsetX = 0.0f;
                    m_offsetY = 0.0f;
                }
                break;
            }
        }
    }
}
