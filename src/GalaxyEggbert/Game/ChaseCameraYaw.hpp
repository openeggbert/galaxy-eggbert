#pragma once

namespace GalaxyEggbert::Game
{
    // Where the third-person camera orbits Blupi (the real-Blupi plan's phase E, user decisions
    // 2026-10-24). The sprites showed Blupi from his side; the 3D chase camera sits behind him
    // while he moves and brings that side view back when it can:
    //
    // - Moving: the camera follows his yaw with a lag (kFollowPerSecond), so on a turn he is seen
    //   from the side for a moment before it straightens out behind him.
    // - Idle (standing with no input for kIdleDelaySeconds): it orbits to whichever of his sides is
    //   nearer to where it is, skipping a side a wall blocks, and stays behind if both are.
    // - A clip that turns him toward the sprites' viewer (clips.txt "viewer", e.g. the squashed or
    //   inflated Blupi): it always goes to his right side, the side the original profile camera
    //   saw, so he faces the camera as he faced the player.
    //
    // Yaws use BlupiController's convention (he moves along (sin yaw, 0, -cos yaw)); GetYaw() is
    // the camera's orbit yaw: the camera sits at (-sin, 0, cos) of it from Blupi, so his own yaw
    // puts it behind him and his yaw - pi/2 on his right. Engine-agnostic like CameraShake, so
    // tools/VerifyChaseCameraYaw.cpp checks it without a graphics context.
    enum class ChaseCameraSide
    {
        Behind,
        Right,
        Left,
    };

    struct ChaseCameraInput
    {
        float blupiYaw = 0.0f;
        bool idle = false;        // standing (AnimState::Stop) with no movement or turn input
        bool facesViewer = false; // the active clip turns him toward the sprites' viewer
        bool rightOpen = true;    // no wall between Blupi and the camera on his right
        bool leftOpen = true;     // ... on his left
    };

    class ChaseCameraYaw
    {
    public:
        // Three seconds standing still before it leaves his back (user request 2026-10-24).
        static constexpr float kIdleDelaySeconds = 3.0f;
        static constexpr float kFollowPerSecond = 2.5f;
        static constexpr float kOrbitPerSecond = 1.5f;

        // Puts the camera straight behind a Blupi facing `blupiYaw` (first frame, first person).
        void Reset(float blupiYaw) noexcept;
        void Update(const ChaseCameraInput& input, float dt) noexcept;

        [[nodiscard]] float GetYaw() const noexcept { return m_yaw; }
        [[nodiscard]] ChaseCameraSide GetSide() const noexcept { return m_side; }

        // `angle` wrapped into (-pi, pi].
        [[nodiscard]] static float Wrap(float angle) noexcept;

    private:
        float m_yaw = 0.0f;
        float m_idleSeconds = 0.0f;
        ChaseCameraSide m_side = ChaseCameraSide::Behind;
    };

    // How high the chase camera stands (D18, 2026-10-26). When something behind Blupi (a wall, a
    // pillar over the cell he stands in) pulls the camera in onto his head, it rises over it
    // instead, looking down at him; in a low tunnel, whose ceiling the usual elevation meets, it
    // drops toward the level instead. The elevation nearest to `elevation` (up before down,
    // within kMinElevation..kMaxElevation) that leaves it kEnough of `distance`, else the one
    // with the most room if that is clearly more (kGain) than at `elevation`, else `elevation`.
    // `room(e)` is how far the camera can stand back at elevation e (radians above his look
    // point).
    template <typename Room>
    [[nodiscard]] float ChaseCameraElevation(float elevation, float distance, Room&& room)
    {
        constexpr float kStep = 0.15f;
        constexpr float kMaxElevation = 1.3f;
        constexpr float kMinElevation = -0.35f;
        constexpr float kEnough = 0.85f;
        constexpr float kGain = 0.5f;
        const float base = room(elevation);
        if (base >= distance * kEnough)
        {
            return elevation;
        }
        float best = elevation;
        float bestRoom = base;
        for (int k = 1;; ++k)
        {
            const float up = elevation + static_cast<float>(k) * kStep;
            const float down = elevation - static_cast<float>(k) * kStep;
            const bool canUp = up <= kMaxElevation + 1e-4f;
            const bool canDown = down >= kMinElevation - 1e-4f;
            if (!canUp && !canDown)
            {
                break;
            }
            for (const float e : {up, down})
            {
                if ((e == up && !canUp) || (e == down && !canDown))
                {
                    continue;
                }
                const float r = room(e);
                if (r >= distance * kEnough)
                {
                    return e;
                }
                if (r > bestRoom)
                {
                    best = e;
                    bestRoom = r;
                }
            }
        }
        return bestRoom > base + kGain ? best : elevation;
    }
}
