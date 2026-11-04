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
    // `table_decor_lave[... + m_time/ScaleDiv(2)]`. The per-cell offsets
    // (i * 13 + j * 7) and the second water's per-cell pace are
    // BlockDefinitionRegistry::ResolveAnimationIconAt's (BD-0065).
    //
    // The fans' divisor is the original's 2 (Decor.cpp:1104-1122: the block icons 126/129/132/135
    // draw table_decor_ventillog/d/h/b[m_time / ScaleDiv(2) % 3], three frames each). It was
    // believed to have no table (only the wind's table_decor_ventg/d/h/b, divisor 1, which a
    // 2026-07-09 fix took and was "too fast") and ran at 3 until 2026-11-02 (owner: too slow).
    //
    // Extracted out of TerrainRenderer.cpp's anonymous namespace
    // (plan.md TEST-007, 2026-07-14) so it can be exercised by a scripted
    // test without needing a graphics context -- behavior is unchanged.
    int AnimDivisor(std::uint16_t base) noexcept;
}
