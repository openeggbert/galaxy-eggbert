#pragma once
#include "GalaxyEggbert/def/BlupiAction.hpp"

// Partial port of mobile-eggbert Tables.cpp — basic Blupi animations only.
// Full table_blupi[2911] + all other tables will be ported with Phase 8 (Decor).
class Tables {
public:
    Tables() = delete;

    // Returns the blupi.png icon index for the given action at the given scaled
    // animation phase.  scaledPhase = rawTick / 3  (matches original 20 fps
    // timing when running at 60 fps).  Returns 0 (standing frame) if unknown.
    static int GetBlupiIcon(GalaxyEggbert::BlupiAction action, int scaledPhase);
};
