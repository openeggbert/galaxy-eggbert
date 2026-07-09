#include "GEInnerFlatPlateTiles.hpp"

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
            default:
                return Easy3D::PlateAxis::Z;
        }
    }
}
