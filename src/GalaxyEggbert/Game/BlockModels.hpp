#pragma once

#include <GalaxyEggbert/BlockTypes.hpp>

#include <array>
#include <cstdint>
#include <string_view>

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

    // Animated terrain drawn as its original scene's model frame by frame: the cell's animation
    // icon (ResolveBlockAnimationIcon) picks the pose its model draws for that icon; a frame with
    // no icon (-1: gone) draws nothing. The vanishing saucer 324-329 (Decor/temporaire.rd4, its
    // six icons at IoU 0.985-0.992; owner 2026-11-02: a billboard, wrong from above) and the
    // crusher 317-323 hanging in its cell (Decor/aplatisseur.rd4, seven icons at IoU 0.948-0.99:
    // the blue spring and the red block striking down).
    // The slime drip 404-410 (Decor/goutte/goutte01-04.rds, 404-408; the two smallest drops 409-410
    // have no scene and are not drawn; BD-0033).
    inline constexpr std::array<std::uint16_t, 3> kAnimatedModelBases{GalaxyEggbert::BlockTypes::Temp,
                                                                      GalaxyEggbert::BlockTypes::Crusher,
                                                                      GalaxyEggbert::BlockTypes::Drip};

    // Animated terrain whose cell is the floor (Galaxy's convention: the space over it is deadly)
    // and whose model stands on it, one cell up: the saw 378-383 and the stopped saw 379
    // (Decor/scie.rd4, its six icons at IoU 0.986-0.990; owner 2026-11-02: a rock cube showed in
    // its place). The renderer draws the cell as the floor around it.
    inline constexpr std::array<std::uint16_t, 2> kModelOverFloorBases{GalaxyEggbert::BlockTypes::Saw,
                                                                       GalaxyEggbert::BlockTypes::SawStopped};

    // Still terrain icons drawn as their original scene's model when avatars3d/objects has one for
    // their object-m icon (VIS-0077/0078/0378/0400): the stone column 76 (Decor/support1.rd4), the
    // golden support 77 (Decor/support2.rd4), the barrel stack 377 (Decor/pile1.rd4) and the white
    // column 399 (Decor/palais/big0108.rds). Their roles and collision are unchanged.
    // The hanging bar's two icons too (Decor/barre.rd4, owner 2026-11-02: in 3D, not a plate).
    // The teleporters 330-333 (Decor/teleporteur1-4.rd4, one scene a letter; owner 2026-11-02: in
    // one cell as the tile shows it, the device its top third, the tip under it), the yellow
    // trestle 199 (Decor/support3.rd4), and the posts 182 and 183 (Decor/porte1.rd4, porte2.rd4).
    // The coloured doors 334-336 (Decor/porte-r/v/b.rd4, IoU 0.983; owner 2026-11-02: the red, green
    // and blue posts in 3D), until a key opens them. The spring 211 (Decor/ressort.rd4, IoU 0.98;
    // owner 2026-11-02: a cube with a sprite), its frame per tick (SpringFrameIcon below).
    // The clay bridge 364 and its shaking 365-366 (Decor/grotte-fragile1.rd4; its crumbling pieces
    // 367-372 are the ObjectType52's, drawn in the same cell).
    // The lightning's emitter 304 (Decor/eclair.rd4, IoU 0.94), its front toward the open side.
    // The saws' switch 384/385 (Decor/interrupteur.rd4, IoU 1.0, front toward the open side) and the
    // conveyor 311-316 (Decor/chenille.rds, 0.95-0.96) (OR-0050's sweep). Not the column 375:
    // Decor/colonne1.rds is a cube carrying a painted column (an embedded image and mask), so the
    // original was already a picture on a box, which its sprite on the cell's faces reproduces.
    // The palace's balustrade 398 (Decor/palais/mur06.rds, IoU 0.99; a plate before), a balcony's
    // edge, its front toward the open side: the drop it rails.
    inline constexpr std::array<std::uint16_t, 31> kStillModelIcons{76, 77, 377, 399, 398, 201, 138, 202,
                                                                     330, 331, 332, 333, 199, 182, 183,
                                                                     334, 335, 336, 211, 364, 365, 366, 304,
                                                                     384, 385, 311, 312, 313, 314, 315, 316};

    // The spring 211 is drawn as Decor/ressort.rd4 always bouncing as the original draws it:
    // Tables::table_ressort[(m_time / 2 + column * 7) % 8] (Decor.cpp:926-929; the column's 7
    // offsets each spring's phase; in 3D the cell's X + Z).
    inline constexpr std::array<std::uint16_t, 8> kSpringFrames{209, 210, 211, 212, 213, 212, 211, 210};
    [[nodiscard]] inline std::uint16_t SpringFrameIcon(int animPhase, int gridX, int gridZ) noexcept
    {
        const int k = ((animPhase / 2 + (gridX + gridZ) * 7) % 8 + 8) % 8;
        return kSpringFrames[static_cast<std::size_t>(k)];
    }

    // The conveyor 311-316 (Decor/chenille.rds) is a tread only 0.206 of a cell deep (Z -0.103 to
    // 0.103 as its frames pose it), a rail in 3D where Blupi walks the whole of the cell's top; it
    // is stretched along Z to the cell's depth, its wheels becoming rollers across it (a belt
    // stretches where the clay bridge's lumps would not; OR-0055).
    inline constexpr float kConveyorDepth = 0.206f;
    [[nodiscard]] constexpr float ModelDepthScale(int objectMIcon) noexcept
    {
        return objectMIcon >= 311 && objectMIcon <= 316 ? 1.0f / kConveyorDepth : 1.0f;
    }
    // Its scene models the tread's belt only on the side the sprite's camera saw (Z -0.02 to 0.08
    // of the base's -0.10 to 0.10): stretched, the belt runs along the front half and the back
    // shows the bare base (owner 2026-11-03: the belt belongs on the other side too). So it is
    // drawn a second time mirrored along Z, the belt's run along X unchanged.
    [[nodiscard]] constexpr bool ModelMirroredInDepth(int objectMIcon) noexcept
    {
        return objectMIcon >= 311 && objectMIcon <= 316;
    }

    // The clay bridge (364-372, Decor/grotte-fragile1.rd4) covers its cell's whole top in 3D (owner
    // 2026-11-02: a slab of clay lumps over the whole cell, not a strip along its edge). The scene
    // is one row of five lumps along X, only 0.284 of a cell deep (Z -0.142 to 0.142 as its frames
    // pose it): the side view needed no more. So the row is drawn kBridgeRows times across the
    // cell, every other one turned round so that the lumps do not line up; the outer rows' edges
    // lie on the cell's, the rows overlapping a little. The slab keeps the scene's height, the
    // lumps from 0.13 to 0.55 over the cell's centre (the sprite's top 24 of 64 rows).
    inline constexpr int kBridgeRows = 4;
    inline constexpr float kBridgeRowDepth = 0.284f;
    inline constexpr float kBridgeRowCentreZ = 0.0f; // the row's centre along Z, posed
    [[nodiscard]] constexpr bool IsClayBridgeIcon(int objectMIcon) noexcept
    {
        return objectMIcon >= 364 && objectMIcon <= 372;
    }
    struct BridgeRow
    {
        float dz;  // the model's origin from the cell's centre, along Z
        float yaw; // its turn about Y (0 or pi)
    };
    [[nodiscard]] constexpr BridgeRow BridgeRowPlacement(int row) noexcept
    {
        constexpr float kPi = 3.14159265359f;
        const float pitch = (1.0f - kBridgeRowDepth) / static_cast<float>(kBridgeRows - 1);
        const float centre = -0.5f + kBridgeRowDepth / 2.0f + pitch * static_cast<float>(row);
        const bool turned = row % 2 == 1;
        // Turned round, the row's centre lies at -kBridgeRowCentreZ from the origin.
        return {turned ? centre + kBridgeRowCentreZ : centre - kBridgeRowCentreZ, turned ? kPi : 0.0f};
    }

    // The signs as their Ray Dream models (owner's decision MR-0006): the missions' 174-181
    // (Decor/world1.rd4), the worlds' 158-165, 309, 411-415 and the return sign 184 (world2a.rd4;
    // 184's red arrow its decal, 2026-11-03) and the completed worlds'
    // 166-173, 310, 416-420 (world2b.rd4), IoU 0.995; what was painted on them (the digit, the
    // world's picture) is a decal (avatars3d/objects/<sign>/decals.txt, tools/rd4_to_gltf/
    // sign_decals.py). Turned toward the cell's open side like the front models below.
    // The treasure doors 421-439 too: the golden post 183's scene (Decor/porte2.rd4) with the grey
    // plate of the number of treasures that opens it as the decal (sign_decals.py --plate).
    inline constexpr std::array<std::uint16_t, 56> kSignModelIcons{
        174, 175, 176, 177, 178, 179, 180, 181,
        158, 159, 160, 161, 162, 163, 164, 165, 309, 411, 412, 413, 414, 415, 184,
        166, 167, 168, 169, 170, 171, 172, 173, 310, 416, 417, 418, 419, 420,
        421, 422, 423, 424, 425, 426, 427, 428, 429, 430, 431, 432, 433, 434, 435, 436, 437, 438, 439};
    [[nodiscard]] constexpr bool IsSignModelIcon(std::uint16_t icon) noexcept
    {
        for (const auto sign : kSignModelIcons)
        {
            if (sign == icon)
            {
                return true;
            }
        }
        return false;
    }

    // Still icons whose model has a front (the original is a front view: the teleporters' lights
    // and letter, the trestle's frame), turned toward the cell's open side
    // (TerrainRenderer::ModelCell::openSide).
    inline constexpr std::array<std::uint16_t, 9> kFrontModelIcons{330, 331, 332, 333, 199, 304, 384, 385, 398};

    // The teleporters 330-333 (Decor/teleporteur1-4.rd4) are a blue box the size of the cell with
    // the letter plate and two lights on its front only: the original's sprite shows that front
    // whichever way Blupi comes, but in 3D three sides were blank and, where the open side the
    // front turns to is not the one Blupi comes from, the letter could not be seen (owner
    // 2026-11-02, OR-0056). The plate and lights are drawn on all four sides, as a sign's painting
    // is drawn front and back; the box once.
    [[nodiscard]] constexpr bool IsTeleporterIcon(int objectMIcon) noexcept
    {
        return objectMIcon >= 330 && objectMIcon <= 333;
    }
    // How far each copy of the plate stands out to sit on its side of the box as the front's sits on
    // the front: the box is not centred (posed, X -0.508 to 0.497, Z -0.552 to 0.488, the front at
    // +Z), so a copy turned a quarter (to +X) moves out 0.009, a half (to -Z) 0.064 -- else the
    // back's plate lies inside the box -- and three quarters (to -X) 0.020.
    inline constexpr std::array<float, 4> kBoothSideOffsets{0.0f, 0.497f - 0.488f, 0.552f - 0.488f, 0.508f - 0.488f};
    // A teleporter model's body: the part of a plain colour (`_rd_RRGGBB`) whose blue is its
    // strongest component, as against the letter plate (a texture, `_rd_tex_...`) and the red and
    // yellow lights.
    [[nodiscard]] inline bool IsBoothBodyPart(std::string_view name) noexcept
    {
        const auto at = name.rfind("_rd_");
        if (at == std::string_view::npos || name.size() != at + 4 + 6)
        {
            return false;
        }
        const auto hex = [&](std::size_t i) -> int
        {
            int v = 0;
            for (std::size_t k = i; k < i + 2; ++k)
            {
                const char c = name[k];
                const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10
                            : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
                if (d < 0)
                {
                    return -1;
                }
                v = v * 16 + d;
            }
            return v;
        };
        const int r = hex(at + 4), g = hex(at + 6), b = hex(at + 8);
        return r >= 0 && g >= 0 && b >= 0 && b > r && b > g;
    }

    // The turn about Y (radians) putting a model's front, +Z at no turn, toward side 0 +X, 1 -X,
    // 2 +Z, 3 -Z: CreateRotationY(a) turns +Z to (sin a, 0, cos a).
    [[nodiscard]] constexpr float FrontModelYaw(int side) noexcept
    {
        constexpr float kHalfPi = 1.57079632679f;
        return side == 0 ? kHalfPi : side == 1 ? -kHalfPi : side == 3 ? 2.0f * kHalfPi : 0.0f;
    }

    // The hanging bar's icons, both rendered from Decor/barre.rd4: 138 with the two mounts that hold
    // it to a ceiling, 202 the pipe alone (its model leaves the mounts out, objects.txt drop=).
    inline constexpr std::uint16_t kBarMountedIcon = 138;
    inline constexpr std::uint16_t kBarPlainIcon = 202;
    [[nodiscard]] constexpr bool IsBarIcon(std::uint16_t icon) noexcept
    {
        return icon == kBarMountedIcon || icon == kBarPlainIcon;
    }

    // The icons under which a bar has no mounts: those for which Decor::IsRightBorder (Decor.cpp:10367)
    // answers false seen from the bar below (dx 0, dy 1), found by running that function over every
    // icon (2026-11-02); empty, Galaxy's air, too. Passable decor not listed (the girder 201) holds
    // them all the same.
    inline constexpr std::array<std::uint16_t, 86> kNoMountsUnderIcons{
        15,  16,  17,  18,  19,  21,  25,  26,  28,  32,  33,  34,  45,  46,  47,  74,  75,  87,
        88,  89,  90,  107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
        122, 123, 124, 125, 126, 129, 135, 138, 139, 140, 141, 142, 143, 154, 155, 157, 202, 250,
        251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 264, 265, 266, 267, 268, 269, 270, 271,
        272, 273, 274, 275, 276, 277, 278, 279, 280, 281, 282, 317, 378, 404};

    // The bar a cell shows, by the icon above it (0: air). The original's editor keeps a bar so
    // (Decor::AdaptMidBorder, the "above" bit of Tables::table_adapt_decor's bar row: 138 with it, 202
    // without); 301 of the 303 bars of its levels follow it (two 138 under cheese 250/252, left from
    // an edit). Galaxy's worlds place either icon anywhere, so the cell's own icon is not used.
    [[nodiscard]] constexpr std::uint16_t ShownBarIcon(std::uint16_t above) noexcept
    {
        if (above == 0)
        {
            return kBarPlainIcon;
        }
        for (const auto icon : kNoMountsUnderIcons)
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
