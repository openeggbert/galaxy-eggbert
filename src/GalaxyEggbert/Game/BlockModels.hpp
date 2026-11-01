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

    // Still terrain icons drawn as their original scene's model when avatars3d/objects has one for
    // their object-m icon (VIS-0077/0078/0378/0400): the stone column 76 (Decor/support1.rd4), the
    // golden support 77 (Decor/support2.rd4), the barrel stack 377 (Decor/pile1.rd4) and the white
    // column 399 (Decor/palais/big0108.rds). Their roles and collision are unchanged.
    inline constexpr std::array<std::uint16_t, 5> kStillModelIcons{76, 77, 377, 399, 201};

    // Still icons whose model is a flat frame drawn on each side of the cell (a lattice box), left
    // out on a side shared with the same icon: the steel girder 201 (Decor/structure1.rd4, its
    // truss frame). The sides mask bits: 1 +X, 2 -X, 4 +Z, 8 -Z (that neighbour is the same icon).
    inline constexpr std::array<std::uint16_t, 1> kLatticeModelIcons{201};

    struct LatticeSide
    {
        float dx, dz;    // the frame's offset from the cell's centre
        float yaw;       // its turn about Y (the model's frame faces +Z)
        std::uint8_t bit; // the neighbour bit that hides it
    };
    inline constexpr std::array<LatticeSide, 4> kLatticeSides{{
        {0.0f, 0.5f, 0.0f, 4},
        {0.0f, -0.5f, 3.14159265f, 8},
        {0.5f, 0.0f, 1.57079632f, 1},
        {-0.5f, 0.0f, -1.57079632f, 2},
    }};

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
