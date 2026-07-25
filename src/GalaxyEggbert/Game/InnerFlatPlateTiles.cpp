#include "InnerFlatPlateTiles.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::Game
{
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
