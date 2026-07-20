#include "GEInnerFlatPlateTiles.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

#include <algorithm>
#include <iterator>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Source: mobile-eggbert-reference/questionnaire-all-remaining-tiles.md,
        // every icon whose "Render mód?" answer is `InnerFlatPlate`,
        // confirmed 2026-07-08.
        constexpr int kInnerFlatPlateIcons[] = {
            77, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121,
            122, 123, 124, 125, 138, 199,
            264, 265, 266, 267, 268, 269, 270, 271, 272, 273, 274, 275, 276,
            277, 278, 279, 280, 281, 282, 285, 286, 287, 288, 289, 290, 291,
            292, 293, 294, 295, 296, 297, 298, 299, 300, 302, 303,
            367, 368, 369, 370, 371, 372,
            398,
            // Saw/SawStopped (378/379) is NOT in this list -- moved out
            // 2026-07-20 (3rd round of live user feedback) into its own
            // dedicated ground-anchored-overlay system in
            // GETerrainRenderer.cpp (IsGroundAnchoredPlateIcon(), handled
            // the same additive way as IsGrassTopIcon() -- a normal solid
            // floor cube PLUS a horizontal blade plate on top), since an
            // exclusive InnerFlatPlate (hollow block, no solid cube at all)
            // read as a hole/pit in the floor once the blade itself was
            // correctly reoriented to lie flat. See that file's own comment
            // for the full history (2 earlier rounds, 2026-07-11, only
            // repositioned a VERTICAL plate before this axis correction).
        };
    }

    bool IsInnerFlatPlateIcon(int icon)
    {
        return std::find(std::begin(kInnerFlatPlateIcons), std::end(kInnerFlatPlateIcons), icon) !=
               std::end(kInnerFlatPlateIcons);
    }

    Easy3D::PlateAxis GetInnerFlatPlateAxis(int icon, bool rotated)
    {
        Easy3D::PlateAxis axis;
        switch (icon)
        {
            case 368:
            case 369:
            case 370:
            case 371:
            case 372:
                axis = Easy3D::PlateAxis::Y;
                break;
            default:
                axis = Easy3D::PlateAxis::Z;
                break;
        }
        // 90-degree rotation (2026-07-11, user feedback) -- e.g. Saw needs
        // to face whichever way its OWN corridor runs, which varies per
        // placement, not per icon (real mobile-eggbert's 2D sprite has no
        // axis concept at all). PlateAxis::Y is left unchanged -- no
        // rotation-eligible horizontal-plate icon exists yet, and rotating
        // one 90 degrees around its own vertical normal isn't a simple
        // axis-enum swap the way X<->Z is.
        if (rotated && axis == Easy3D::PlateAxis::X)
        {
            axis = Easy3D::PlateAxis::Z;
        }
        else if (rotated && axis == Easy3D::PlateAxis::Z)
        {
            axis = Easy3D::PlateAxis::X;
        }
        return axis;
    }
}
