#include "InnerFlatPlateTiles.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::Game
{
    CnaEngine::Graphics::PlateAxis GetInnerFlatPlateAxis(int icon, bool rotated)
    {
        CnaEngine::Graphics::PlateAxis axis;
        switch (icon)
        {
            case 368:
            case 369:
            case 370:
            case 371:
            case 372:
                axis = CnaEngine::Graphics::PlateAxis::Y;
                break;
            default:
                axis = CnaEngine::Graphics::PlateAxis::Z;
                break;
        }
        // 90-degree rotation (2026-07-11, user feedback) -- e.g. Saw needs
        // to face whichever way its OWN corridor runs, which varies per
        // placement, not per icon (real mobile-eggbert's 2D sprite has no
        // axis concept at all). PlateAxis::Y is left unchanged -- no
        // rotation-eligible horizontal-plate icon exists yet, and rotating
        // one 90 degrees around its own vertical normal isn't a simple
        // axis-enum swap the way X<->Z is.
        if (rotated && axis == CnaEngine::Graphics::PlateAxis::X)
        {
            axis = CnaEngine::Graphics::PlateAxis::Z;
        }
        else if (rotated && axis == CnaEngine::Graphics::PlateAxis::Z)
        {
            axis = CnaEngine::Graphics::PlateAxis::X;
        }
        return axis;
    }
}
