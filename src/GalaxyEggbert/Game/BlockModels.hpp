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
    // The hanging bar's two icons too (Decor/barre.rd4, owner 2026-11-02: in 3D, not a plate).
    inline constexpr std::array<std::uint16_t, 7> kStillModelIcons{76, 77, 377, 399, 201, 138, 202};

    // The hanging bar's icons, both rendered from Decor/barre.rd4: 138 with the two mounts that hold
    // it to a ceiling, 202 the pipe alone (its model leaves the mounts out, objects.txt drop=).
    inline constexpr std::uint16_t kBarMountedIcon = 138;
    inline constexpr std::uint16_t kBarPlainIcon = 202;
    [[nodiscard]] constexpr bool IsBarIcon(std::uint16_t icon) noexcept
    {
        return icon == kBarMountedIcon || icon == kBarPlainIcon;
    }

    // Icons whose underside is no edge for the mounts to hold: Decor::IsRightBorder (Decor.cpp:10367)
    // answers false for them seen from below (dy == 1).
    inline constexpr std::array<std::uint16_t, 18> kNoUndersideEdgeIcons{
        15, 16, 17, 18, 19, 21, 25, 26, 28, 74, 75, 87, 88, 89, 90, 154, 155, 317};

    // The bar a cell shows, by the icon above it and whether that one is solid (a quarter-cell
    // collision mask). The original's editor keeps a bar so (Decor::AdaptMidBorder, the "above" bit of
    // Tables::table_adapt_decor's bar row: 138 with it, 202 without): all 303 bars of its levels
    // follow this. Galaxy's worlds place either icon anywhere, so the cell's own icon is not used.
    [[nodiscard]] constexpr std::uint16_t ShownBarIcon(std::uint16_t above, bool aboveSolid) noexcept
    {
        if (!aboveSolid || IsBarIcon(above))
        {
            return kBarPlainIcon;
        }
        for (const auto icon : kNoUndersideEdgeIcons)
        {
            if (icon == above)
            {
                return kBarPlainIcon;
            }
        }
        return kBarMountedIcon;
    }

    // The icon a model cell continues a row of: the two bars are one row.
    [[nodiscard]] constexpr std::uint16_t ModelRowFamily(std::uint16_t icon) noexcept
    {
        return icon == kBarPlainIcon ? kBarMountedIcon : icon;
    }

    // Still icons whose model runs along X and is turned to run along Z where its row does (no
    // neighbour of its row on X, one on Z): the hanging bars.
    inline constexpr std::array<std::uint16_t, 2> kRowModelIcons{kBarMountedIcon, kBarPlainIcon};

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
