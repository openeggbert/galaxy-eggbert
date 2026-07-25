#pragma once

namespace GalaxyEggbert::Game
{
    inline constexpr float kGroundObjectCenterY = 1.0f;

    [[nodiscard]] constexpr float ObjectVisualCenterY(float objectCenterY) noexcept
    {
        return objectCenterY;
    }

    [[nodiscard]] constexpr float LiftRiderCenterY(float liftCenterY) noexcept
    {
        return liftCenterY + 1.0f;
    }
}
