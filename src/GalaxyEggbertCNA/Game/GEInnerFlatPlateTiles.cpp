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

    Easy3D::PlateAxis GetInnerFlatPlateAxis(int icon)
    {
        switch (icon)
        {
            case 368:
            case 369:
            case 370:
            case 371:
            case 372:
                return Easy3D::PlateAxis::Y;
            // Saw/SawStopped (378/379): unlike the other confirmed
            // InnerFlatPlate icons (small decorations with no reliable
            // orientation cue, hence the harmless Z-axis tie-break below),
            // Saw's one real gameplay placement (plan.md E3D-MIG-142's
            // switch+saw pair, worlds3d/world001.vwr) sits in a corridor
            // Blupi walks along X, not Z -- a Z-axis plate is invisible
            // edge-on from that approach (confirmed live, 2026-07-11).
            // Real mobile-eggbert's own 2D sprite has no axis concept at
            // all (always face-on to the player), so this is a genuine 3D
            // placement adaptation, not a faithfulness question -- X is
            // chosen because it's the one axis that's actually correct for
            // the one real placement that exists today.
            case GalaxyEggbert::BlockTypes::Saw:
            case GalaxyEggbert::BlockTypes::SawStopped:
                return Easy3D::PlateAxis::X;
            default:
                return Easy3D::PlateAxis::Z;
        }
    }
}
