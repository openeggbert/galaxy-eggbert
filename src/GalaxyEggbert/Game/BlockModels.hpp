#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>

#include <array>
#include <cstdint>

namespace GalaxyEggbert::Game
{
    // Terrain cells drawn as an original 3D model instead of a textured cube (VIS-0127, 2026-11-01).
    //
    // The fan: the four direction families (126 left, 129 right, 132 up, 135 down, three animation
    // frames each) were all rendered from one Ray Dream scene, Decor/ventillo-h.rd4, a fan on a stem
    // standing on a base plate and blowing up (its model avatars3d/objects/fan_ventillo-h matches the
    // up tiles 132-134 at IoU 0.993). The others are that model turned about the view axis (Z) so it
    // blows the way the cell's gameplay does (DirectionalCubeTiles, InteractionSystem): left -X,
    // right +X, down -Y; its base then sits on the wall, ceiling or floor behind it.
    inline constexpr std::array<std::uint16_t, 4> kFanModelBases{
        GalaxyEggbert::BlockTypes::FanLeft, GalaxyEggbert::BlockTypes::FanRight,
        GalaxyEggbert::BlockTypes::FanUp, GalaxyEggbert::BlockTypes::FanDown};
    // The icon whose model pose draws frame 0 (the up fan's; frame k is this + k).
    inline constexpr std::uint16_t kFanModelIcon = GalaxyEggbert::BlockTypes::FanUp;

    // The fan model's roll about Z (radians) for a fan cell of animation base `base`.
    [[nodiscard]] constexpr float FanModelRoll(std::uint16_t base) noexcept
    {
        constexpr float kHalfPi = 1.57079632679f;
        return base == GalaxyEggbert::BlockTypes::FanLeft    ? kHalfPi
               : base == GalaxyEggbert::BlockTypes::FanRight ? -kHalfPi
               : base == GalaxyEggbert::BlockTypes::FanUp    ? 0.0f
                                                             : 2.0f * kHalfPi;
    }
}
