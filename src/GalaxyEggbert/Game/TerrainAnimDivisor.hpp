#pragma once

#include <cstdint>

namespace GalaxyEggbert::Game
{
    // Real per-type tick divisor against the 20fps raw tick
    // (WorldRuntime::GetAnimPhase()) -- mobile-eggbert's real Decor.cpp
    // tile-animation logic (table_decor_scie/lave/eau1/eau2/ecraseur/
    // piege1/piege2/temp, table_marine) divides the same 20fps base tick
    // per type via Config::ScaleDiv(N), NOT a flat rate shared by every
    // animated tile -- confirmed by direct source line, e.g.
    // `table_decor_lave[... + m_time/ScaleDiv(2)]`. Water2/Marine's real
    // divisor also varies per-instance (3 + position%3, a visual
    // ripple-offset detail) -- not modeled here, every instance of a given
    // type shares one phase; only the base speed is fixed.
    //
    // Fan's divisor is NOT from mobile-eggbert source -- the FanLeft/
    // Right/Up/Down BLOCK icons (126-137) have no dedicated table_decor_*
    // animation entry in Decor.cpp at all (only a same-numbered-looking
    // but unrelated table_decor_ventg/ventd/venth/ventb exists, animating
    // icons 110-125, a separate wind "particle stream" decor effect, not
    // these cube blocks -- an initial 2026-07-09 fix wrongly matched fans
    // to that table's divisor 1/50ms and was reported live as "now too
    // fast"; reverted to divisor 3/150ms here, the same rate as the other
    // real mechanical/moving elements (Water1/Crusher/Marine) for lack of
    // a real per-type source value to match.
    //
    // Extracted out of TerrainRenderer.cpp's anonymous namespace
    // (plan.md TEST-007, 2026-07-14) so it can be exercised by a scripted
    // test without needing a graphics context -- behavior is unchanged.
    int AnimDivisor(std::uint16_t base) noexcept;
}
