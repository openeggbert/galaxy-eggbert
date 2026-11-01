#pragma once

#include <cmath>

namespace GalaxyEggbert::Game::CratePush
{
    // How Blupi moves a crate (ObjectType12), from Mobile Eggbert's Decor.cpp (2026-11-01, the
    // fidelity audit's GP-crate rows, plans/audit/behavior_differences.md BD-0001):
    //
    // - While pushing (action Push) he walks CaisseGetMove(5) pixels a 20 Hz tick
    //   (Decor.cpp:3388/3446); while pulling (Pop) he backs away CaisseGetMove(3) (:3410/3468).
    // - Every tick the crate is put against him: its left edge at his pos + 55 (facing right)
    //   or pos - 59 (facing left), Decor.cpp:6137-6152, 6200-6248: its centre 57 px from his.
    //   If the crate group cannot take that place (TestPushCaisse) he goes back where he was:
    //   the crate never leaves his hands and never jumps.
    // - CaisseGetMove (Decor.cpp:9487): the speed drops by one pixel for every two crates
    //   linked to the one he holds (at least 1), doubles under Power, and ramps up over the
    //   first 20 ticks of the action (max * phase / 20, at least 1).
    //
    // In 3D the crate moves along X or Z (the axis Blupi faces), its group linked within the push's
    // lane, and it rests wherever it stops, as the original's (the owner's reports OR-0021..0023
    // overturned the whole-cell moves of M3D-0001, 2026-11-01).

    // The original's 64-pixel block is one Galaxy cell.
    inline constexpr float kPixelsPerCell = 64.0f;
    inline constexpr float kOriginalTicksPerSecond = 20.0f;

    // Centre-to-centre distance between Blupi and the crate he pushes or pulls: the crate's left
    // edge at his pos + 55, a 64 px crate, a 60 px Blupi: 55 + 32 - 30 = 57 px (59 - 32 + 30 to
    // the left).
    inline constexpr float kContactGap = 57.0f / kPixelsPerCell;

    // Walking into a crate becomes a push once his centre is this close to the crate's (his
    // collision point stops at half a cell from it, so he always gets there).
    inline constexpr float kEngageGap = kContactGap + 1.0f / kPixelsPerCell;

    // A 3D adaptation (M3D-0002): a crate that cannot go only because it lies off the lane of its
    // move by up to this much across (a pixel past a cell after a push along the other axis, say)
    // first slides into the lane, at the same speed. The original is 1D and needs no such rule;
    // aligning a crate to the pixel in 3D is not humanly possible.
    inline constexpr float kLaneSlide = 16.0f / kPixelsPerCell;

    inline constexpr int kPushMaxPixels = 5; // CaisseGetMove(5), Decor.cpp:3388/3446
    inline constexpr int kPullMaxPixels = 3; // CaisseGetMove(3), Decor.cpp:3410/3468

    // Decor::CaisseGetMove(max) (Decor.cpp:9487-9507), the pixels a 20 Hz tick, for `linked`
    // crates moving together (m_nbLinkCaisse, the held one included) and `phaseTicks` 20 Hz
    // ticks into the action (m_blupiPhase).
    [[nodiscard]] constexpr int PixelsPerTick(int max, int linked, bool power, int phaseTicks) noexcept
    {
        max -= (linked - 1) / 2;
        if (max < 1)
        {
            max = 1;
        }
        if (power)
        {
            max *= 2;
        }
        if (phaseTicks < 20)
        {
            max = max * phaseTicks / 20;
            if (max == 0)
            {
                max++;
            }
        }
        return max;
    }

    // The same in cells a second, `seconds` into the action.
    [[nodiscard]] inline float CellsPerSecond(int max, int linked, bool power, float seconds) noexcept
    {
        const int phaseTicks = static_cast<int>(std::floor(seconds * kOriginalTicksPerSecond + 1e-4f));
        return static_cast<float>(PixelsPerTick(max, linked, power, phaseTicks)) * kOriginalTicksPerSecond /
               kPixelsPerCell;
    }
}
