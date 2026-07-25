#pragma once

namespace GalaxyEggbert::Game
{
    // Real screen-shake/forced-pan camera effect (plan.md CAM-008..013),
    // ported directly from `Decor::DecorNextAction()`/`Tables::
    // table_decor_action` (mobile-eggbert `Decor.cpp:1353-1397`,
    // `Tables.cpp:2027-2081`; data-table transcription approved by the
    // user 2026-07-14). Real mechanism: `m_decorAction` selects one of 3
    // fixed per-frame `(dx,dy)` tables (Small/Big/Electric); each real
    // tick adds `3 * (dx,dy)` (the real, confirmed ×3 multiplier) to the
    // scroll/camera position for that frame only (not accumulated into
    // any persistent state) and advances one frame; once the table is
    // exhausted the effect self-clears back to `None`. A new `Trigger()`
    // unconditionally restarts from frame 0 regardless of whatever was
    // previously active (matches the real trigger sites -- a plain
    // `m_decorAction = X; m_decorPhase = 0;` assignment, no priority
    // gating found anywhere).
    //
    // Values returned by GetOffsetX()/GetOffsetY() are in real PIXEL
    // units (the real table's own "sub-pixel Decor units" already
    // multiplied by 3, matching `DecorNextAction()`'s own math exactly)
    // -- converting this to a 3D camera-space perturbation is the
    // caller's job (GalaxyEggbertGame), keeping this class a pure,
    // engine-agnostic port of the real per-frame math, same precedent as
    // BlupiController/TerrainAnimDivisor.
    //
    // Real tick rate is `Config::CURRENT_FPS` == 20 (confirmed,
    // `Config.hpp`), the same shared 20Hz tick domain as every other
    // per-frame real system ported this session.
    enum class CameraShakeType
    {
        None,
        Small,
        Big,
        Electric,
    };

    class CameraShake
    {
    public:
        static constexpr float kTickSeconds = 1.0f / 20.0f;

        void Trigger(CameraShakeType type) noexcept;
        void Update(float dt) noexcept;

        [[nodiscard]] float GetOffsetX() const noexcept { return m_offsetX; }
        [[nodiscard]] float GetOffsetY() const noexcept { return m_offsetY; }
        [[nodiscard]] CameraShakeType GetActiveType() const noexcept { return m_active; }
        [[nodiscard]] int GetPhase() const noexcept { return m_phase; }

    private:
        CameraShakeType m_active = CameraShakeType::None;
        int m_phase = 0;
        float m_tickTimer = 0.0f;
        float m_offsetX = 0.0f;
        float m_offsetY = 0.0f;
    };
}
