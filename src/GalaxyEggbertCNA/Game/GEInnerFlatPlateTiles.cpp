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
            // Saw/SawStopped (378/379, plan.md E3D-MIG-149/142) added
            // 2026-07-11 per direct live user Q&A ("pila bude staticky
            // billboard tedy uprostred daneho bloku se textura nanese na
            // obe strany jakoby neviditelen desky v puli krychle") --
            // supersedes the questionnaire's earlier "ThinMechanical,
            // geometry not decided" placeholder categorization (§10.3, a
            // circular-blade shape flagged as thin/non-bulk but never given
            // a confirmed render mode), see mobile-eggbert-reference/
            // 02-tiles.md's own updated note. Was previously falling
            // through every special-geometry table into the generic
            // fully-textured UniformCube fallback, which the user reported
            // as visibly wrong.
            GalaxyEggbert::BlockTypes::Saw,
            GalaxyEggbert::BlockTypes::SawStopped,
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
