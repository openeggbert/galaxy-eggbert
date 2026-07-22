// INFRA-003 (plan.md §7, "Correctness Infrastructure" vision): a
// data-integrity regression test for GEObjectIcons::GetObjIcon()'s hand-
// transcribed frame arrays and divisors -- the file this function lives in
// (GEObjectIcons.cpp) carries 34 "Fixed 2026-..." annotations documenting
// historical wrong-divisor/wrong-sequence bugs, each found only by a human
// looking at a screenshot (REMAKE-ANALYSIS.md's own diagnosis, RC-1/RC-2).
//
// This is a REGRESSION test, not a fresh independent re-verification
// against mobile-eggbert (which would require reading ../mobile-eggbert
// source directly -- out of scope, needs separate approval): every expected
// sequence/divisor/period below is transcribed from GEObjectIcons.cpp's own
// already-committed, already-cross-referenced-against-Tables.cpp code (see
// that file's own per-case comments for the real Tables.cpp/Decor.cpp
// citations). The value here is locking today's confirmed-correct values in
// as an explicit, checked contract, so a future edit can't silently
// re-introduce one of the 34 historical bugs without failing this test.
//
// Frame-count (period) cross-checked against mobile-eggbert-reference/
// 08-animations.md §3.1/3.2 wherever that doc documents one, noted per case
// below -- this is the specific "frame-array lengths match the documented
// counts" check plan.md's own INFRA-003 entry asks for.
#include "Game/GEObjectIcons.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

using GalaxyEggbert::ObjectType;
using namespace GalaxyEggbert::CNA;

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

    // Verifies GetObjIcon(type, p) == sequence[(p / divisor) % period] for
    // p across 2 full periods (so both the per-frame HOLD, `divisor`, and
    // the wrap-around, `period`, are exercised, not just the first cycle).
    void checkSequence(ObjectType type, int divisor, const std::vector<int>& sequence, const std::string& name)
    {
        const int period = static_cast<int>(sequence.size());
        for (int p = 0; p < divisor * period * 2; ++p)
        {
            const int expected = sequence[static_cast<std::size_t>((p / divisor) % period)];
            const int actual = GetObjIcon(type, p);
            if (actual != expected)
            {
                check(false, name + ": p=" + std::to_string(p) + " expected=" + std::to_string(expected) +
                                  " actual=" + std::to_string(actual));
                return; // one mismatch per sequence is enough signal; avoid a wall of repeats
            }
        }
        check(true, name);
    }

    // Verifies GetObjIcon(type, p) == base + (p / divisor) % period for a
    // plain ascending run (no table needed) -- same 2-full-periods coverage
    // as checkSequence() above.
    void checkAscending(ObjectType type, int divisor, int base, int period, const std::string& name)
    {
        for (int p = 0; p < divisor * period * 2; ++p)
        {
            const int expected = base + (p / divisor) % period;
            const int actual = GetObjIcon(type, p);
            if (actual != expected)
            {
                check(false, name + ": p=" + std::to_string(p) + " expected=" + std::to_string(expected) +
                                  " actual=" + std::to_string(actual));
                return;
            }
        }
        check(true, name);
    }

    // Verifies GetObjIcon(type, p) returns the same fixed icon regardless of
    // p -- for the confirmed single-static-icon ObjectTypes.
    void checkStatic(ObjectType type, int expectedIcon, const std::string& name)
    {
        for (int p : {0, 1, 2, 5, 37, 1000})
        {
            const int actual = GetObjIcon(type, p);
            if (actual != expectedIcon)
            {
                check(false, name + ": p=" + std::to_string(p) + " expected=" + std::to_string(expectedIcon) +
                                  " actual=" + std::to_string(actual));
                return;
            }
        }
        check(true, name);
    }
}

int main()
{
    // -- Static single-icon types (no animation) --
    checkStatic(ObjectType::ObjectType23, 176, "ObjectType23 (secret-level marker) static icon");
    checkStatic(ObjectType::ObjectType28, 167, "ObjectType28 static icon");
    checkStatic(ObjectType::ObjectType29, 177, "ObjectType29 static icon");
    checkStatic(ObjectType::ObjectType56, 253, "ObjectType56 static icon (100 real frames would overflow the sheet)");
    checkStatic(ObjectType::ObjectType52, 365, "ObjectType52 static icon (157 real frames would overflow the sheet)");
    checkStatic(ObjectType::ObjectType1, 29, "ObjectType1 (platform lift) static icon");
    checkStatic(ObjectType::ObjectType12, 32, "ObjectType12 (crate) static icon");
    checkStatic(ObjectType::ObjectType13, 68, "ObjectType13 static icon");
    checkStatic(ObjectType::ObjectType30, 178, "ObjectType30 static icon");
    checkStatic(ObjectType::ObjectType19, 89, "ObjectType19 (jeep) static icon");
    checkStatic(ObjectType::ObjectType46, 208, "ObjectType46 (balloon) static icon");
    checkStatic(ObjectType::ObjectType55, 252, "ObjectType55 (dynamite) static icon");

    // -- Plain ascending-range types (divisor + base + period only, no table) --
    // ObjectType2/3: 9 frames each, matches mobile-eggbert-reference/08-animations.md §3.1
    // ("patrol enemy A/B", 9 frames).
    checkAscending(ObjectType::ObjectType2, 2, 12, 9, "ObjectType2 (patrol enemy A)");
    checkAscending(ObjectType::ObjectType3, 2, 48, 9, "ObjectType3 (patrol enemy B)");
    checkAscending(ObjectType::ObjectType36, 2, 179, 8, "ObjectType36 (pollution puff)");
    checkAscending(ObjectType::ObjectType41, 2, 179, 8, "ObjectType41 (invert-start burst)");
    checkAscending(ObjectType::ObjectType31, 2, 238, 6, "ObjectType31 (charge power-up)");
    // ObjectType16: 9 frames, matches 08-animations.md §3.1 ("spider", 9 frames).
    checkAscending(ObjectType::ObjectType16, 1, 69, 9, "ObjectType16 (spider)");
    // ObjectType6/7: 8 frames each, matches 08-animations.md §3.1 ("egg"/"exit goal", 8 frames).
    checkAscending(ObjectType::ObjectType6, 4, 21, 8, "ObjectType6 (extra-life egg)");
    checkAscending(ObjectType::ObjectType7, 3, 29, 8, "ObjectType7 (level-exit goal)");
    checkAscending(ObjectType::ObjectType98, 1, 90, 10, "ObjectType98 (bullet-splat, table_sploutch1)");
    checkAscending(ObjectType::ObjectType200, 1, 257, 6, "ObjectType200 (Blupi-skin)");
    checkAscending(ObjectType::ObjectType201, 1, 257, 6, "ObjectType201 (Blupi-skin)");
    checkAscending(ObjectType::ObjectType202, 1, 257, 6, "ObjectType202 (Blupi-skin)");
    checkAscending(ObjectType::ObjectType203, 1, 257, 6, "ObjectType203 (Blupi-skin)");

    // ObjectType42: descending range (186 down to 179).
    {
        constexpr int kDivisor = 2;
        constexpr int kPeriod = 8;
        for (int p = 0; p < kDivisor * kPeriod * 2; ++p)
        {
            const int expected = 186 - (p / kDivisor) % kPeriod;
            check(GetObjIcon(ObjectType::ObjectType42, p) == expected,
                  "ObjectType42 (invert-stop burst): p=" + std::to_string(p));
        }
    }

    // ObjectType5: real asymmetric 0..11..1 triangle wave (Decor.cpp:8297-8307),
    // period 22 -- matches 08-animations.md §3.1 ("treasure sparkle", "22 (11-icon ping-pong)").
    {
        constexpr int kDivisor = 3;
        constexpr int kPeriod = 22;
        for (int p = 0; p < kDivisor * kPeriod * 2; ++p)
        {
            const int q = (p / kDivisor) % kPeriod;
            const int expected = (q < 11) ? q : (kPeriod - q);
            check(GetObjIcon(ObjectType::ObjectType5, p) == expected,
                  "ObjectType5 (treasure sparkle): p=" + std::to_string(p));
        }
    }

    // -- Table-driven types (exact sequence transcribed from GEObjectIcons.cpp) --
    checkSequence(ObjectType::ObjectType27, 1,
                  {152, 153, 154, 155, 156, 152, 153, 154, 155, 156, 157, 158, 159, 160, 157, 158,
                   159, 160, 161, 162, 163, 164, 165, 166},
                  "ObjectType27 (kMagicTrack, Shield/Power magic trail)");
    checkSequence(ObjectType::ObjectType34, 1,
                  {168, 168, 169, 169, 170, 170, 171, 171, 170, 170, 169, 169, 168, 168, 169, 169,
                   169, 168, 168, 169, 169, 170, 170, 169, 168},
                  "ObjectType34 (kGlu, real table_glu)");
    checkSequence(ObjectType::ObjectType37, 1,
                  {40, 40, 40, 40, 41, 41, 41, 41, 40, 40, 40, 40, 40, 40, 40, 41, 41, 41, 40, 40,
                   40, 40, 40, 40, 40, 41, 41, 41, 40, 40, 42, 42, 42, 43, 43, 43, 44, 44, 44, 45,
                   45, 45, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46,
                   47, 47, 46, 46, 47, 47, 46, 46, 47, 47},
                  "ObjectType37 (kClear, real table_clear)");
    checkSequence(ObjectType::ObjectType39, 1, {166, 165, 164, 163, 162, 161, 162, 163, 164, 165, 166},
                  "ObjectType39 (kTresorTrack, real table_tresortrack)");
    checkSequence(ObjectType::ObjectType57, 1,
                  {274, 275, 276, 277, 278, 274, 275, 276, 277, 278, 279, 280, 281, 282, 283, 284,
                   285, 286, 287, 288},
                  "ObjectType57 (kShieldTrack, real table_shieldtrack, 20 frames)");
    checkSequence(ObjectType::ObjectType97, 1, {256, 258, 260, 262, 264},
                  "ObjectType97 (kFollow2, real table_follow2, step of 2; matches 08-animations.md's \"5 frames\")");
    // ObjectType14 (real table_plouf, 7-frame ripple) -- 08-animations.md doesn't
    // list this one directly (Category B, added later), but the array itself is
    // cited against Tables.cpp:1508-1519 in GEObjectIcons.cpp.
    checkSequence(ObjectType::ObjectType14, 2, {99, 100, 101, 102, 101, 100, 99}, "ObjectType14 (kPlouf)");
    checkSequence(ObjectType::ObjectType15, 2,
                  {103, 104, 105, 106, 104, 103, 106, 105, 103, 104, 103, 105, 106, 103, 105, 106,
                   103, 104, 106, 105},
                  "ObjectType15 (kBlup)");
    checkSequence(ObjectType::ObjectType35, 2, {244, 99, 244}, "ObjectType35 (kTiplouf)");
    // ObjectType4/17/20/33: 8 frames each, matches 08-animations.md §3.1.
    checkSequence(ObjectType::ObjectType4, 9, {66, 66, 67, 67, 66, 66, 65, 65},
                  "ObjectType4 (kBulldozer, generic phase-indexed fallback)");
    checkSequence(ObjectType::ObjectType17, 6, {82, 82, 81, 81, 82, 82, 83, 83},
                  "ObjectType17 (kFish, generic phase-indexed fallback)");
    checkSequence(ObjectType::ObjectType20, 6, {98, 99, 100, 101, 102, 103, 104, 105},
                  "ObjectType20 (kBird, generic phase-indexed fallback)");
    checkSequence(ObjectType::ObjectType33, 6, {249, 249, 250, 250, 249, 249, 248, 248},
                  "ObjectType33 (kBlupit, generic phase-indexed fallback)");
    // ObjectType49/50/51: 12 frames each, matches 08-animations.md §3.1 ("key", 12 frames).
    checkSequence(ObjectType::ObjectType49, 3, {209, 210, 211, 212, 213, 214, 215, 214, 213, 212, 211, 210},
                  "ObjectType49 (kCle1, Key1)");
    checkSequence(ObjectType::ObjectType50, 3, {220, 221, 222, 221, 220, 219, 218, 217, 216, 217, 218, 219},
                  "ObjectType50 (kCle2, Key2)");
    checkSequence(ObjectType::ObjectType51, 3, {229, 228, 227, 226, 225, 224, 223, 224, 225, 226, 227, 228},
                  "ObjectType51 (kCle3, Key3)");
    // ObjectType25: 16 frames -- 08-animations.md §3.1 lists "8" for this row, but that
    // row's own text explicitly flags itself as stale ("kShield[8] is a known-incomplete
    // port... real 16-frame table_shield") -- 16 is the corrected, current value, not 8.
    checkSequence(ObjectType::ObjectType25, 2,
                  {144, 145, 146, 147, 148, 149, 150, 151, 266, 267, 268, 269, 270, 271, 272, 273},
                  "ObjectType25 (kShield, real 16-entry table_shield, NOT the stale 8-entry doc row)");
    // ObjectType21: 12 frames, matches 08-animations.md §3.2 ("secret-level exit", 12 frames).
    checkSequence(ObjectType::ObjectType21, 3, {122, 123, 124, 125, 126, 127, 128, 127, 126, 125, 124, 123},
                  "ObjectType21 (kCleGeneric)");
    // ObjectType24: 34 frames, matches 08-animations.md §3.2 ("skateboard", 34 frames).
    checkSequence(ObjectType::ObjectType24, 1,
                  {129, 129, 129, 129, 130, 130, 130, 131, 131, 132, 132, 133, 133, 134, 134, 134,
                   135, 135, 135, 135, 134, 134, 134, 133, 133, 132, 132, 131, 131, 131, 130, 130,
                   130, 130},
                  "ObjectType24 (kSkate)");
    // ObjectType26: 8 frames, matches 08-animations.md §3.2 ("suction-cup", 8 frames).
    checkSequence(ObjectType::ObjectType26, 2, {136, 137, 138, 139, 140, 141, 142, 143}, "ObjectType26 (kPower)");
    // ObjectType40: 20 frames, matches 08-animations.md §3.2 ("mirror/invert", 20 frames).
    checkSequence(ObjectType::ObjectType40, 2,
                  {187, 187, 187, 188, 189, 190, 191, 192, 193, 194, 187, 187, 187, 194, 193, 192,
                   191, 190, 189, 188},
                  "ObjectType40 (kInvert)");
    // ObjectType47/48: 6 frames each, matches 08-animations.md §3.2 ("platform lift track", 6 frames).
    checkSequence(ObjectType::ObjectType47, 1, {311, 312, 313, 314, 315, 316}, "ObjectType47 (kChenille)");
    checkSequence(ObjectType::ObjectType48, 1, {316, 315, 314, 313, 312, 311}, "ObjectType48 (kChenillei, reverse)");
    // ObjectType32/44/54: generic phase-indexed fallback (distinct from the newer
    // patrol-aware GetBlupihIcon()/GetWaspIcon()/GetCreatureIcon() below).
    checkSequence(ObjectType::ObjectType32, 6, {66, 67, 68, 67, 66, 69, 70, 69},
                  "ObjectType32 (kBlupihLeft, generic phase-indexed fallback)");
    checkSequence(ObjectType::ObjectType44, 6, {195, 196, 197, 198, 197, 196},
                  "ObjectType44 (kGuepeLeft, generic phase-indexed fallback; matches 08-animations.md's \"6 frames\")");
    checkSequence(ObjectType::ObjectType54, 6, {247, 248, 249, 250, 251, 250, 249, 248},
                  "ObjectType54 (kCreature, generic phase-indexed fallback)");
    // ObjectType96: 26 frames, matches 08-animations.md §3.2 ("follower, dormant", 26 frames).
    checkSequence(ObjectType::ObjectType96, 1,
                  {256, 256, 256, 257, 257, 258, 259, 260, 261, 262, 263, 264, 264, 265, 265, 265,
                   264, 264, 263, 262, 261, 260, 259, 258, 257, 257},
                  "ObjectType96 (kFollow1, follower dormant)");

    // -- explo.png tables (ObjectType8-11/53/90-93/98-100) --
    checkSequence(ObjectType::ObjectType8, 1,
                  {0, 0, 1, 1, 2, 2, 3, 3, 4, 3, 4, 4, 3, 4, 3, 3, 4, 4, 5, 5, 4, 5, 6, 5, 6, 6, 5,
                   5, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11},
                  "ObjectType8 (kExplo1, real table_explo1, 39 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType9, 1,
                  {12, -1, 13, 14, -1, 15, 13, -1, 14, 15, 12, -1, 13, 15, 14, 14, -1, 14, 15, 13},
                  "ObjectType9 (kExplo2, real table_explo2, 20 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType10, 1,
                  {32, 32, 34, 34, 32, 32, 34, 34, 32, 32, 34, 34, 32, 32, 35, 35, 32, 32, 35, 35},
                  "ObjectType10 (kExplo3, real table_explo3, 20 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType11, 1, {12, 13, 14, 15, 7, 8, 9, 10, 11},
                  "ObjectType11 (kExplo4, real table_explo4, 9 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType90, 1, {54, -1, 55, -1, 56, -1, 57, -1, 58, -1, 59, -1},
                  "ObjectType90 (kExplo5, real table_explo5, 12 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType91, 1, {54, 55, 56, 57, 58, 59},
                  "ObjectType91 (kExplo6, real table_explo6, 6 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType93, 1, {7, 8, 9, 10, 11},
                  "ObjectType93 (kExplo8, real table_explo8, 5 frames per 08-animations.md §4)");
    checkSequence(ObjectType::ObjectType99, 1, {-1, -1, -1, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99},
                  "ObjectType99 (kSploutch2, real table_sploutch2, real leading -1 delay)");
    checkSequence(ObjectType::ObjectType100, 1,
                  {-1, -1, -1, -1, -1, -1, -1, -1, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99},
                  "ObjectType100 (kSploutch3, real table_sploutch3, real leading -1 delay)");
    // ObjectType53 (kTentacule, 45 frames per 08-animations.md's own VISUAL-021 finding).
    checkSequence(ObjectType::ObjectType53, 1,
                  {86, 85, 84, 83, 84, 85, 86, -1, 86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75,
                   74, 73, 72, 71, 70, 70, 70, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82,
                   83, 84, 85, 86, -1},
                  "ObjectType53 (kTentacule, real table_tentacule)");
    // ObjectType92 (kExplo7, real table_explo7, 128 frames per 08-animations.md §4).
    checkSequence(ObjectType::ObjectType92, 1,
                  {60, 61, -1, 63, 64, 65, 62, 64, 62, 60, 62, -1, 65, -1, 60, 65, 63, 61, 62, -1,
                   64, 65, -1, 62, 64, 61, 62, 63, -1, 65, 60, -1, 65, -1, 63, 65, -1, 61, 60, 65,
                   62, 63, 64, -1, 62, 63, -1, 62, 62, 60, 62, -1, 65, -1, 60, 65, 64, 61, 62, 63,
                   -1, 65, 60, -1, 63, 61, 62, -1, 64, 65, -1, 62, 62, 60, 62, -1, 65, -1, 60, 65,
                   60, 61, -1, 63, 64, 65, 62, 64, 63, 61, 62, -1, 64, 65, -1, 62, 60, 61, -1, 63,
                   64, 65, 62, 64, -1, 60, -1, -1, 65, -1, 60, -1, 63, -1, 62, -1, -1, 65, -1, -1,
                   -1, 61, -1, -1, -1, 60, -1, -1},
                  "ObjectType92 (kExplo7, real table_explo7)");

    // ObjectType38 (kElectro, real table_electro, 90-tick two-channel cycle:
    // icons 266/267 for the first 30 ticks (blupi1.png), 40-47 afterward
    // (element.png) -- GetObjIcon() itself just returns the icon; the channel
    // switch is the caller's job via IsBlupiPngSourcedAtPhase()).
    checkSequence(ObjectType::ObjectType38, 1,
                  {266, 267, 266, 267, 266, 267, 266, 267, 266, 267, 266, 267, 266, 267, 266, 267,
                   266, 267, 266, 267, 266, 267, 266, 267, 266, 267, 266, 267, 266, 267, 40, 40,
                   40, 40, 41, 41, 41, 41, 40, 40, 40, 40, 40, 40, 40, 41, 41, 41, 40, 40, 40, 40,
                   40, 40, 40, 41, 41, 41, 40, 40, 42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 45,
                   46, 46, 47, 47, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46},
                  "ObjectType38 (kElectro, real table_electro)");

    // Genuinely no icon exists in mobile-eggbert source data for these 4 --
    // default: return 0 is correct, not a gap (GEObjectIcons.hpp's own header comment).
    checkStatic(ObjectType::ObjectType0, 0, "ObjectType0 (no real icon -- confirmed gap, not a bug)");
    checkStatic(ObjectType::ObjectType18, 0, "ObjectType18 (no real icon -- confirmed gap, not a bug)");
    checkStatic(ObjectType::ObjectType22, 0, "ObjectType22 (no real icon -- confirmed gap, not a bug)");
    checkStatic(ObjectType::ObjectType58, 0, "ObjectType58 (no real icon -- confirmed gap, not a bug)");

    // Texture-atlas-selection predicates (2026-07-23, found via a fresh
    // code audit to have zero test coverage anywhere in tools/):
    // IsUniformCubeObject/IsObjectMPngSourced/IsExploPngSourced/
    // IsBlupiPngSourced gate which of 5 real sheets (object-m.png,
    // element.png [the implicit "none of the above" default],
    // explo.png, blupi.png/blupi1.png) a type's billboard actually
    // samples from -- used by both real gameplay rendering and the
    // editor palette (GEEditorPalette.cpp). A type accidentally moved
    // between lists renders with a completely wrong/garbage texture --
    // visually broken, silent, no assertion elsewhere would catch it,
    // only a live screenshot would. The 4 predicates must be mutually
    // EXCLUSIVE (each type samples exactly one sheet); UsesBlupi1Texture
    // must be a subset of IsBlupiPngSourced (blupi1.png selection is
    // only meaningful for types already on the blupi-png path).
    {
        // Regression lock: exact current membership, transcribed directly
        // from GEObjectIcons.cpp's own switch statements (not re-derived
        // from mobile-eggbert -- see this file's own top comment on scope).
        check(IsUniformCubeObject(ObjectType::ObjectType1) && IsUniformCubeObject(ObjectType::ObjectType12) &&
                  IsUniformCubeObject(ObjectType::ObjectType47) && IsUniformCubeObject(ObjectType::ObjectType48),
              "IsUniformCubeObject() matches its real membership list (1/12/47/48)");
        check(IsObjectMPngSourced(ObjectType::ObjectType14) && IsObjectMPngSourced(ObjectType::ObjectType15) &&
                  IsObjectMPngSourced(ObjectType::ObjectType31) && IsObjectMPngSourced(ObjectType::ObjectType35) &&
                  IsObjectMPngSourced(ObjectType::ObjectType52),
              "IsObjectMPngSourced() matches its real membership list (14/15/31/35/52)");
        check(IsExploPngSourced(ObjectType::ObjectType8) && IsExploPngSourced(ObjectType::ObjectType53) &&
                  IsExploPngSourced(ObjectType::ObjectType100),
              "IsExploPngSourced() matches its real membership list (spot-checked: 8/53/100)");
        check(IsBlupiPngSourced(ObjectType::ObjectType200) && IsBlupiPngSourced(ObjectType::ObjectType38),
              "IsBlupiPngSourced() matches its real membership list (spot-checked: 200/38)");
        check(!IsUniformCubeObject(ObjectType::ObjectType6) && !IsObjectMPngSourced(ObjectType::ObjectType6) &&
                  !IsExploPngSourced(ObjectType::ObjectType6) && !IsBlupiPngSourced(ObjectType::ObjectType6),
              "the extra-life egg (ObjectType6) is on none of the 4 special sheets -- falls to element.png");

        // Exhaustive mutual-exclusivity check across ObjectType's full
        // real range (a uint8_t enum, 0-255) -- no type may be claimed by
        // more than one of the 4 predicates at once.
        bool exclusive = true;
        int overlapType = -1;
        for (int i = 0; i <= 255; ++i)
        {
            const ObjectType t = GalaxyEggbert::ToObjectType(i);
            const int claims = (IsUniformCubeObject(t) ? 1 : 0) + (IsObjectMPngSourced(t) ? 1 : 0) +
                               (IsExploPngSourced(t) ? 1 : 0) + (IsBlupiPngSourced(t) ? 1 : 0);
            if (claims > 1)
            {
                exclusive = false;
                overlapType = i;
                break;
            }
        }
        check(exclusive, "the 4 texture-sheet predicates are mutually exclusive across every ObjectType (0-255)" +
                              (exclusive ? std::string() : ", first overlap at type " + std::to_string(overlapType)));

        // UsesBlupi1Texture must never claim a type IsBlupiPngSourced()
        // itself doesn't -- blupi1.png selection only makes sense for
        // types already routed onto the blupi-png path.
        bool subsetHolds = true;
        int violatingType = -1;
        for (int i = 0; i <= 255; ++i)
        {
            const ObjectType t = GalaxyEggbert::ToObjectType(i);
            if (UsesBlupi1Texture(t) && !IsBlupiPngSourced(t))
            {
                subsetHolds = false;
                violatingType = i;
                break;
            }
        }
        check(subsetHolds, "UsesBlupi1Texture() is always a subset of IsBlupiPngSourced()" +
                                (subsetHolds ? std::string() : ", violated at type " + std::to_string(violatingType)));
    }

    std::cout << checksRun << " checks run, " << checksFailed << " failed." << std::endl;
    std::cout << (checksFailed == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return checksFailed == 0 ? 0 : 1;
}
