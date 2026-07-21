// INFRA-004 (plan.md §7, "Correctness Infrastructure" vision):
// UnverifiedRenderMapping.hpp's table is bookkeeping, not a rendering
// change -- it marks the ~131 terrain icons whose 3D render-mode
// recommendation (mobile-eggbert-reference/15-3d-render-mapping-design.md
// §10.2-§10.5) is still only a first-pass agent visual guess, never
// confirmed by direct user identification the way §11's other 34 icons
// were. This test locks the table's current shape in as an explicit,
// checked contract so it can't silently shrink (an icon quietly treated as
// "confirmed" without anyone actually doing the confirmation work) or grow
// stale (duplicate/out-of-range entries) without failing here first.
#include "GalaxyEggbert/UnverifiedRenderMapping.hpp"

#include <iostream>
#include <string>

using namespace GalaxyEggbert::UnverifiedRenderMapping;

namespace
{
    int checksRun = 0;
    int checksFailed = 0;

    void check(bool condition, const std::string& message)
    {
        ++checksRun;
        if (!condition)
        {
            ++checksFailed;
            std::cout << "FAIL: " << message << std::endl;
        }
    }
}

int main()
{
    // Matches the count of "(§10.N)"-cited rows in
    // mobile-eggbert-reference/02-tiles.md as of 2026-07-21. If this
    // fails after an intentional identification pass resolved some icons
    // (moving them out of this table, mirroring §11's precedent), update
    // this constant deliberately -- don't just bump it to make the test
    // pass without checking the removed icons were genuinely resolved.
    check(kEntryCount == 131, "kEntryCount expected=131 actual=" + std::to_string(kEntryCount));

    // No duplicate icon IDs, and every ID falls inside the addressable
    // icon-ID space (0-440, per 02-tiles.md's own "all 441 addressable
    // icon IDs" framing).
    for (std::size_t i = 0; i < kEntryCount; ++i)
    {
        const int id = kEntries[i].iconId;
        check(id >= 0 && id <= 440, "icon " + std::to_string(id) + " out of the 0-440 addressable range");
        for (std::size_t j = i + 1; j < kEntryCount; ++j)
        {
            check(kEntries[j].iconId != id, "duplicate entry for icon " + std::to_string(id));
        }
    }

    // Spot-check IsUnverified(): icons resolved by direct user
    // identification must read as NOT unverified (they graduated out of
    // this table, or in Saw/SawStopped's case were never really a guess --
    // both were fully resolved through live back-and-forth, see NEXT.md).
    check(!IsUnverified(378), "icon 378 (Saw) resolved to InnerFlatPlate, must not read as unverified");
    check(!IsUnverified(379), "icon 379 (SawStopped) resolved to InnerFlatPlate, must not read as unverified");
    check(!IsUnverified(401), "icon 401 (cobweb) resolved to DirectionalCube in §11, must not read as unverified");
    check(!IsUnverified(61), "icon 61 (brick) resolved to Billboard by direct user ID in §11, must not read as unverified");
    check(!IsUnverified(10), "icon 10 (Ground) was never in the §10 finding at all, must not read as unverified");

    // Spot-check known members, one per render-mode category, still genuinely
    // unresolved first-pass guesses as of 2026-07-21.
    check(IsUnverified(201), "icon 201 (grate, ThinMechanical, most common flagged icon) should read as unverified");
    check(IsUnverified(391), "icon 391 (architectural-kit) should read as unverified");
    check(IsUnverified(92), "icon 92 (Water1, special-surface) should read as unverified");
    check(IsUnverified(182), "icon 182 (post/pillar, Billboard) should read as unverified");

    if (checksFailed == 0)
    {
        std::cout << "VerifyUnverifiedRenderMapping: all " << checksRun << " checks passed." << std::endl;
        return 0;
    }
    std::cout << "VerifyUnverifiedRenderMapping: " << checksFailed << "/" << checksRun << " checks FAILED."
               << std::endl;
    return 1;
}
