#include "GEObjectIcons.hpp"

namespace GalaxyEggbert::CNA
{
    int GetObjIcon(ObjectType type, int p)
    {
        static const int kCle1[12]    = {209,210,211,212,213,214,215,214,213,212,211,210};
        static const int kCle2[12]    = {220,221,222,221,220,219,218,217,216,217,218,219};
        static const int kCle3[12]    = {229,228,227,226,225,224,223,224,225,226,227,228};
        static const int kShield[8]   = {144,145,146,147,148,149,150,151};
        static const int kBulldozer[8]= {66,66,67,67,66,66,65,65};
        static const int kBird[8]     = {98,99,100,101,102,103,104,105};
        static const int kFish[8]     = {82,82,81,81,82,82,83,83};
        static const int kBlupit[8]   = {249,249,250,250,249,249,248,248};
        // Real table_tresortrack (Tables.cpp:1792-1795, Invert-burst-adjacent
        // fix, 2026-07-14) -- an 11-frame oscillating shimmer (166 down to
        // 161 and back), NOT a simple ascending range like the old wrong
        // formula here assumed.
        static const int kTresorTrack[11] = {166,165,164,163,162,161,162,163,164,165,166};
        // Real table_explo4 (Tables.cpp:1391, fan-hit shockwave fix,
        // 2026-07-14) -- NOT a simple ascending range (jumps from 15 back
        // to 7 partway through), same category of bug as kTresorTrack
        // above.
        static const int kExplo4[9] = {12,13,14,15,7,8,9,10,11};
        // Real table_explo3 (Tables.cpp:1384-1388, fish/bird explosion
        // flash fix, 2026-07-14) -- a 20-frame repeating oscillation
        // (32,32,34,34 x3, then 32,32,35,35 x2), NOT a simple ascending
        // range, same category of bug as kExplo4/kTresorTrack above.
        static const int kExplo3[20] = {
            32,32,34,34,32,32,34,34,32,32,
            34,34,32,32,35,35,32,32,35,35
        };
        // Real table_explo2 (Tables.cpp:1377-1381, follower-blocked-path
        // debris flash fix, 2026-07-14) -- 20-frame scattered debris with
        // real `-1` "no sprite this tick" entries interspersed throughout
        // (reuses the same renderer `-1`-skip support added for the
        // bullet-splat effect), NOT a simple ascending range.
        static const int kExplo2[20] = {
            12,-1,13,14,-1,15,13,-1,14,15,
            12,-1,13,15,14,14,-1,14,15,13
        };
        // Real table_explo1 (Tables.cpp:1368-1374, dynamite-blast flash
        // fix, 2026-07-14) -- the real 39-frame primary blast sequence
        // repeatedly bounces back and forth between adjacent values
        // (e.g. ...,4,3,4,3,3,4,4,...) rather than advancing monotonically,
        // same category of bug as kExplo4/kTresorTrack above.
        static const int kExplo1[39] = {
            0,0,1,1,2,2,3,3,4,3,
            4,4,3,4,3,3,4,4,5,5,
            4,5,6,5,6,6,5,5,6,7,
            7,8,8,9,9,10,10,11,11
        };
        // Real table_explo7 (Tables.cpp:1407-1422, teleporter arc fix,
        // 2026-07-14) -- a 128-frame "large multi-particle scatter" with
        // `-1` blanks interspersed throughout (not just a leading/trailing
        // delay like table_sploutch2/3) for a staggered, flickering look;
        // the renderer's existing `-1`-skip support (added for the
        // bullet-splat effect) already handles this.
        static const int kExplo7[128] = {
            60,61,-1,63,64,65,62,64,62,60,
            62,-1,65,-1,60,65,63,61,62,-1,
            64,65,-1,62,64,61,62,63,-1,65,
            60,-1,65,-1,63,65,-1,61,60,65,
            62,63,64,-1,62,63,-1,62,62,60,
            62,-1,65,-1,60,65,64,61,62,63,
            -1,65,60,-1,63,61,62,-1,64,65,
            -1,62,62,60,62,-1,65,-1,60,65,
            60,61,-1,63,64,65,62,64,63,61,
            62,-1,64,65,-1,62,60,61,-1,63,
            64,65,62,64,-1,60,-1,-1,65,-1,
            60,-1,63,-1,62,-1,-1,65,-1,-1,
            -1,61,-1,-1,-1,60,-1,-1
        };
        // Real table_explo5/6/8 (Tables.cpp:1395-1425, found 2026-07-16 -- these 3 were the only
        // remaining explo tables still using an approximation formula instead of an exact
        // transcription after the 2026-07-14 pass fixed explo1/2/3/4/7). Same "wrong divisor"
        // bug as every array above (`(p/6) % N` instead of `p % N`, `Config::ScaleDiv(1)==1`).
        // table_explo5 has real `-1` "no sprite this tick" entries producing an alternating
        // visible/invisible strobe (the renderer's existing `-1`-skip support already handles
        // it); explo6/8 have no blanks, plain ascending ranges.
        static const int kExplo5[12] = {54,-1,55,-1,56,-1,57,-1,58,-1,59,-1};
        static const int kExplo6[6]  = {54,55,56,57,58,59};
        static const int kExplo8[5]  = {7,8,9,10,11};
        // Real table_plouf/table_tiplouf/table_blup (Tables.cpp:1508-1519,
        // found 2026-07-17 during water-splash research) -- same "wrong
        // approximation formula" bug class as every array above. Plouf is a
        // 7-frame ripple oscillating 99->102->99 (NOT a monotonic range up
        // to 105); Tiplouf is only 3 frames, {244,99,244} -- icon 244 is the
        // ambient background, 99 the visible droplet on the middle frame
        // (NOT a monotonic range up to 246); Blup is a 20-frame shuffled
        // reuse of just 4 icons (103-106), NOT a growing range up to 122.
        static const int kPlouf[7]   = {99,100,101,102,101,100,99};
        static const int kTiplouf[3] = {244,99,244};
        static const int kBlup[20]   = {
            103,104,105,106,104,103,106,105,103,104,
            103,105,106,103,105,106,103,104,106,105
        };
        // Real table_magictrack (Tables.cpp:1754-1759, Shield/Power magic
        // trail fix, 2026-07-14) -- NOT a simple ascending range: the real
        // 24-frame "loop departs Blupi" animation repeats icons 152-156
        // TWICE (frames 0-9) before continuing upward, same category of
        // bug as kExplo1/kExplo4/kTresorTrack above.
        static const int kMagicTrack[24] = {
            152,153,154,155,156,152,153,154,155,156,
            157,158,159,160,157,158,159,160,161,162,
            163,164,165,166
        };
        // Real table_shieldtrack (Tables.cpp:1769-1773) -- same
        // repeats-twice-then-continues shape as kMagicTrack above. Also
        // fixes a second bug: the previous static "274 (first-frame only)"
        // return assumed a naive ascending 274..293 range would overflow
        // this engine's element.png sheet, but the REAL table only reaches
        // 288 -- comfortably within bounds, so the full animation fits.
        static const int kShieldTrack[20] = {
            274,275,276,277,278,274,275,276,277,278,
            279,280,281,282,283,284,285,286,287,288
        };
        // Real table_sploutch2/3 (Tables.cpp:1436-1448, bullet-splat
        // effect fix, 2026-07-14) -- -1 means "no sprite this tick" (a
        // real leading delay before the splash icons begin: 3 ticks for
        // table_sploutch2, 8 for table_sploutch3, modeling debris that
        // fell from progressively greater heights).
        static const int kSploutch2[13] = {-1,-1,-1,90,91,92,93,94,95,96,97,98,99};
        static const int kSploutch3[18] = {-1,-1,-1,-1,-1,-1,-1,-1,90,91,92,93,94,95,96,97,98,99};
        static const int kGuepeLeft[6]    = {195,196,197,198,197,196};
        static const int kCreature[8]     = {247,248,249,250,251,250,249,248};
        static const int kBlupihLeft[8]   = {66,67,68,67,66,69,70,69};
        static const int kFollow1[26]     = {256,256,256,257,257,258,259,260,261,262,
                                              263,264,264,265,265,265,264,264,263,262,
                                              261,260,259,258,257,257};
        static const int kChenille[6]     = {311,312,313,314,315,316};
        // table_chenillei (ObjectType48's "leftward carry" reverse-direction
        // track, mobile-eggbert-reference/03-objects.md: "table_chenillei[0]
        // =316") -- same 6 icons as table_chenille, reverse order.
        static const int kChenillei[6]    = {316,315,314,313,312,311};
        static const int kCleGeneric[12]  = {122,123,124,125,126,127,128,127,126,125,124,123};
        static const int kSkate[34]       = {129,129,129,129,130,130,130,131,131,132,
                                              132,133,133,134,134,134,135,135,135,135,
                                              134,134,134,133,133,132,132,131,131,131,
                                              130,130,130,130};
        static const int kPower[8]        = {136,137,138,139,140,141,142,143};
        static const int kInvert[20]      = {187,187,187,188,189,190,191,192,193,194,
                                              187,187,187,194,193,192,191,190,189,188};
        // table_electro (ObjectType38, electric arc) -- transcribed from
        // mobile-eggbert's Tables.cpp with explicit user approval
        // (2026-07-09, NEXT.md §3), the real 90-tick table: ticks 0-29
        // alternate 266/267 (blupi1.png channel), ticks 30-89 cycle through
        // element.png icons 40-47 (Decor.cpp confirms the channel switch at
        // tick 30). Real mobile-eggbert behavior is a one-shot (the object
        // despawns once phase>=90) -- simplified here to a continuous
        // `% 90` loop, matching this function's existing convention for
        // every other multi-frame type (e.g. kFollow1/kSkate above).
        static const int kElectro[90] = {
            266,267,266,267,266,267,266,267,266,267,
            266,267,266,267,266,267,266,267,266,267,
            266,267,266,267,266,267,266,267,266,267,
             40, 40, 40, 40, 41, 41, 41, 41, 40, 40,
             40, 40, 40, 40, 40, 41, 41, 41, 40, 40,
             40, 40, 40, 40, 40, 41, 41, 41, 40, 40,
             42, 42, 42, 43, 43, 43, 44, 44, 44, 45,
             45, 45, 46, 46, 47, 47, 46, 46, 47, 47,
             46, 46, 47, 47, 46, 46, 47, 47, 46, 46};
        switch (type)
        {
            // Category B fill-in (2026-07-09, NEXT.md §3): real behavior
            // confirmed in mobile-eggbert but never level-placed, so these
            // had no icon at all before (fell through to default: return 0,
            // a wrong/placeholder icon). Sourced from
            // mobile-eggbert-reference/03-objects.md's Category B table
            // (icon + frame count only -- table_X[0]=N notation -- not a
            // Tables.cpp per-frame array transcription). Where the
            // documented frame count stays within element.png's 290-icon
            // grid (29 rows x 10 cols), a consecutive-icon cycle is used,
            // matching this function's existing convention for simple
            // animations (e.g. ObjectType2/6/7 above). Where it doesn't
            // (56), the real per-frame layout isn't known without reading
            // Tables.cpp, so only the documented first-frame icon is
            // returned (no animation) rather than guessing a cycle that
            // would run off the sheet. (57 was in this category too until
            // 2026-07-14, when Tables.cpp confirmed its real table fits
            // the sheet after all -- see its own case below.)
            case ObjectType::ObjectType23: return 176;
            // Fixed 2026-07-14 (plan.md VISUAL-011-adjacent, Shield/Power
            // magic trail): wrong divisor (6 instead of the real
            // `Config::ScaleDiv(1)==1`) and wrong ascending-arithmetic
            // assumption -- real table_magictrack repeats icons 152-156
            // twice before continuing (see kMagicTrack above).
            case ObjectType::ObjectType27: return kMagicTrack[p % 24];
            case ObjectType::ObjectType28: return 167;
            case ObjectType::ObjectType29: return 177;
            case ObjectType::ObjectType34: return 168 + (p / 6) % 25;
            // Fixed 2026-07-14 (plan.md VISUAL-013, Pollution puff): wrong
            // divisor (6 instead of the real `Config::ScaleDiv(2)==2`) --
            // real `table_pollution` (`Tables.cpp:1494`) is a plain
            // ascending range (179..186), so only the divisor needed
            // fixing, unlike the non-monotonic tables fixed elsewhere.
            case ObjectType::ObjectType36: return 179 + (p / 2) % 8;
            case ObjectType::ObjectType37: return 40 + (p / 6) % 70;
            // Fixed 2026-07-14 (plan.md VISUAL-012): was a wrong "166 +
            // ascending" arithmetic formula; the real table_tresortrack is
            // an oscillating shimmer (see kTresorTrack above), and the real
            // divisor is Config::ScaleDiv(1)==1, not 6.
            case ObjectType::ObjectType39: return kTresorTrack[p % 11];
            // Fixed 2026-07-14 (plan.md VISUAL-014/015): divisor was 6, real
            // is Config::ScaleDiv(2)==2 at this build's 20Hz reference rate
            // (Tables.cpp:table_invertstart/table_invertstop, confirmed via
            // direct source read); ObjectType42 also wrongly ASCENDED past
            // 186 (out of the real icon range) instead of matching the
            // real table's exact reverse order (186 down to 179).
            case ObjectType::ObjectType41: return 179 + (p / 2) % 8;
            case ObjectType::ObjectType42: return 186 - (p / 2) % 8;
            case ObjectType::ObjectType56: return 253; // 100 frames would exceed the sheet (253+99=352 > 289) -- first-frame only
            // Fixed 2026-07-14 (plan.md VISUAL-011-adjacent, Shield/Power
            // magic trail): the real table only reaches 288 (see
            // kShieldTrack above), not the previously-assumed 293 -- it
            // fits the sheet fine, so the full 20-frame animation is now
            // modeled instead of a static first-frame return.
            case ObjectType::ObjectType57: return kShieldTrack[p % 20];
            case ObjectType::ObjectType97: return 256 + (p / 6) % 5;
            // object-m.png-sourced Category B types (2026-07-09) -- these
            // are NOT element.png icons; GetElementIconUv() would compute
            // the wrong UV rect for them. The icon numbers below are only
            // meaningful when looked up via GETileAtlas::GetTileUv()
            // (object-m.png's 440-icon grid) -- see IsObjectMPngSourced()
            // and GalaxyEggbertCnaGame.cpp's dedicated dispatch, mirroring
            // the existing IsUniformCubeObject() precedent.
            case ObjectType::ObjectType14: return kPlouf[p % 7];
            case ObjectType::ObjectType15: return kBlup[p % 20];
            case ObjectType::ObjectType31: return 238 + (p / 6) % 6;
            case ObjectType::ObjectType35: return kTiplouf[p % 3];
            case ObjectType::ObjectType52: return 365; // 157 frames would exceed the sheet (365+156=521 > 439) -- first-frame only
            case ObjectType::ObjectType1:  return 29;
            case ObjectType::ObjectType2:  return 12 + (p / 6) % 9;
            case ObjectType::ObjectType3:  return 48 + (p / 6) % 9;
            case ObjectType::ObjectType4:  return kBulldozer[(p / 9) % 8];
            case ObjectType::ObjectType12: return 32;
            case ObjectType::ObjectType13: return 68;
            case ObjectType::ObjectType16: return 69 + (p / 3) % 9;
            case ObjectType::ObjectType17: return kFish[(p / 6) % 8];
            case ObjectType::ObjectType20: return kBird[(p / 6) % 8];
            case ObjectType::ObjectType30: return 178;
            case ObjectType::ObjectType33: return kBlupit[(p / 6) % 8];
            // ObjectType5/6/7's real divisors (Decor.cpp: ScaleDiv(3), ScaleDiv(4),
            // ScaleDiv(3) respectively) were mistranscribed as 9/12/9 (each real
            // value x3) -- fixed 2026-07-10, reported live as "truhla" (the
            // ObjectType5 treasure chest) animating 3x too slowly.
            case ObjectType::ObjectType5: { int q = (p / 3) % 22; return (q < 11) ? q : (21 - q); }
            case ObjectType::ObjectType6:  return 21 + (p / 4) % 8;
            case ObjectType::ObjectType7:  return 29 + (p / 3) % 8;
            case ObjectType::ObjectType49: return kCle1[(p / 9) % 12];
            case ObjectType::ObjectType50: return kCle2[(p / 9) % 12];
            case ObjectType::ObjectType51: return kCle3[(p / 9) % 12];
            case ObjectType::ObjectType25: return kShield[(p / 6) % 8];
            case ObjectType::ObjectType19: return 89;
            case ObjectType::ObjectType46: return 208;
            case ObjectType::ObjectType55: return 252;
            case ObjectType::ObjectType21: return kCleGeneric[(p / 9) % 12];
            case ObjectType::ObjectType24: return kSkate[(p / 3) % 34];
            case ObjectType::ObjectType26: return kPower[(p / 6) % 8];
            case ObjectType::ObjectType40: return kInvert[(p / 4) % 20];
            case ObjectType::ObjectType47: return kChenille[(p / 6) % 6];
            case ObjectType::ObjectType48: return kChenillei[(p / 6) % 6];
            case ObjectType::ObjectType32: return kBlupihLeft[(p / 6) % 8];
            case ObjectType::ObjectType44: return kGuepeLeft[(p / 6) % 6];
            case ObjectType::ObjectType54: return kCreature[(p / 6) % 8];
            case ObjectType::ObjectType96: return kFollow1[(p / 3) % 26];

            // explo.png-sourced Category B types (2026-07-09) -- explosions/
            // visual effects, 100-icon grid (0-99). Icon numbers are only
            // meaningful via GetExploIconUv() -- see IsExploPngSourced().
            // 53/92 return their first-frame icon only (documented frame
            // count would exceed the 100-icon grid under a naive
            // consecutive-icon assumption, same reasoning as 56/57/52
            // above); 98 cycles normally (90+9=99 fits exactly); 99/100
            // also return their first REAL-frame icon only (both have real
            // leading invisible ticks in mobile-eggbert that a static
            // return can't represent -- see GetObjIcon's header comment).
            // Fixed 2026-07-14 (plan.md VISUAL-008, dynamite-blast flash):
            // wrong divisor (6 instead of the real Config::ScaleDiv(1)==1)
            // and wrong ascending-arithmetic assumption -- real
            // table_explo1 bounces back and forth (see kExplo1 above).
            case ObjectType::ObjectType8:   return kExplo1[p % 39];
            // Fixed 2026-07-14 (plan.md VISUAL-008, follower-blocked-path
            // debris flash): wrong divisor (6 instead of the real
            // `Config::ScaleDiv(1)==1`) and wrong ascending-arithmetic
            // assumption -- real table_explo2 has real `-1` blank-frame
            // sentinels interspersed throughout (see kExplo2 above).
            case ObjectType::ObjectType9:   return kExplo2[p % 20];
            // Fixed 2026-07-14 (plan.md VISUAL-008, fish/bird explosion
            // flash): wrong divisor (6 instead of the real
            // `Config::ScaleDiv(1)==1`) and wrong ascending-arithmetic
            // assumption -- real table_explo3 oscillates (see kExplo3
            // above).
            case ObjectType::ObjectType10:  return kExplo3[p % 20];
            // Fixed 2026-07-14 (plan.md VISUAL-008-adjacent, the Fan-hit
            // shockwave completing CAM-009's BigShake): wrong divisor (6
            // instead of the real Config::ScaleDiv(1)==1) and wrong
            // ascending-arithmetic assumption -- real table_explo4 jumps
            // non-monotonically (see kExplo4 above).
            case ObjectType::ObjectType11:  return kExplo4[p % 9];
            case ObjectType::ObjectType53:  return 86; // 45 frames would exceed the sheet (86+44=130 > 99)
            // Fixed 2026-07-16 -- exact table_explo5 transcription (see kExplo5 above), same
            // wrong-divisor bug as every other explo case here.
            case ObjectType::ObjectType90:  return kExplo5[p % 12];
            // Fixed 2026-07-16 -- exact table_explo6 transcription (see kExplo6 above).
            case ObjectType::ObjectType91:  return kExplo6[p % 6];
            // Fixed 2026-07-14 (plan.md VISUAL-010, teleporter arc): the
            // real table_explo7 stays within icons 60-65 (see kExplo7
            // above), well within the sheet -- the previous "would exceed
            // the sheet" assumption was based on a wrong naive-ascending
            // guess, same mistake as ObjectType57's own fix.
            case ObjectType::ObjectType92:  return kExplo7[p % 128];
            // Fixed 2026-07-16 -- exact table_explo8 transcription (see kExplo8 above).
            case ObjectType::ObjectType93:  return kExplo8[p % 5];
            // Fixed 2026-07-14 (plan.md VISUAL-009, bullet-splat effect):
            // wrong divisor (6 instead of the real `Config::ScaleDiv(1)==1`)
            // -- real `table_sploutch1` is a plain ascending range (90..99),
            // so only the divisor needed fixing.
            case ObjectType::ObjectType98:  return 90 + p % 10;
            // Fixed 2026-07-14: previously a static "first real frame"
            // return -- the real tables (`table_sploutch2/3`) are now
            // transcribed verbatim (see kSploutch2/3 below) and fully
            // animated, including their real leading `-1` "invisible frame"
            // delay (3 ticks for 99, 8 for 100) -- the renderer skips
            // drawing this tick when the icon is negative (see
            // GalaxyEggbertCnaGame.cpp's explo.png billboard pass).
            case ObjectType::ObjectType99:  return kSploutch2[p % 13];
            case ObjectType::ObjectType100: return kSploutch3[p % 18];

            // blupi.png/blupi1.png-sourced Blupi-skin types (2026-07-09) --
            // 340-icon grid (0-339). Icon numbers are only meaningful via
            // GetBlupiIconUv() -- see IsBlupiPngSourced()/UsesBlupi1Texture().
            case ObjectType::ObjectType200: return 257 + (p / 6) % 6;
            case ObjectType::ObjectType201: return 257 + (p / 6) % 6;
            case ObjectType::ObjectType202: return 257 + (p / 6) % 6;
            case ObjectType::ObjectType203: return 257 + (p / 6) % 6;

            // ObjectType38 (electric arc, 2026-07-09) -- now animated with
            // real per-instance phase (MobileObjSpec::phase,
            // GEWorldRuntime::Update()): returns the real table_electro
            // icon for whichever channel is active at this tick (icon 266/
            // 267 while p%90<30 on blupi1.png, 40-47 afterward on
            // element.png) -- see IsBlupiPngSourcedAtPhase() below for the
            // matching per-instance channel dispatch used by the renderer.
            case ObjectType::ObjectType38:  return kElectro[p % 90];
            default:                       return 0;
        }
    }

    int GetBulldozerIcon(bool patrolGoesLeftFromStart, int patrolStep, int patrolTimeTicks)
    {
        // Real Decor.cpp:8628-8668 -- ObjectType4's own 4-state turn/walk
        // step machine (`patrolStep` here matches its real `step` field
        // exactly: 1=dwell@start, 2=advance, 3=dwell@end, 4=recede; see
        // AdvancePatrolStep()'s own comment), NOT the generic phase-indexed
        // cycle GetObjIcon()'s ObjectType4 case above uses (that one stays,
        // for callers with no patrol-state context, e.g. the editor
        // palette). Real direction split is a static per-object property
        // (posStart.X > posEnd.X, i.e. whether this bulldozer's own patrol
        // range runs right-to-left), not a per-frame check -- step 1/3's
        // turn pose always telegraphs the upcoming step 2/4 walk direction.
        static const int kLeft[8]        = {66,66,67,67,66,66,65,65}; // table_bulldozer_left
        static const int kRight[8]       = {58,58,57,57,58,58,59,59}; // table_bulldozer_right
        static const int kTurnToLeft[22] = {
            58,59,59,59,60,60,60,61,61,62,
            62,63,63,64,64,64,65,65,65,66,
            66,66}; // table_bulldozer_turn2l
        static const int kTurnToRight[22] = {
            66,65,65,65,64,64,64,63,63,62,
            62,61,61,60,60,60,59,59,59,58,
            58,58}; // table_bulldozer_turn2r

        const int t = patrolTimeTicks < 0 ? 0 : patrolTimeTicks;
        if (patrolGoesLeftFromStart)
        {
            switch (patrolStep)
            {
                case 1:  return kTurnToLeft[t % 22];
                case 3:  return kTurnToRight[t % 22];
                case 4:  return kRight[t % 8];
                default: return kLeft[t % 8]; // step 2 (and any other value, defensively)
            }
        }
        switch (patrolStep)
        {
            case 1:  return kTurnToRight[t % 22];
            case 3:  return kTurnToLeft[t % 22];
            case 4:  return kLeft[t % 8];
            default: return kRight[t % 8]; // step 2
        }
    }

    bool IsUniformCubeObject(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::ObjectType1:
            case ObjectType::ObjectType12:
            case ObjectType::ObjectType47:
            case ObjectType::ObjectType48:
                return true;
            default:
                return false;
        }
    }

    bool IsObjectMPngSourced(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::ObjectType14:
            case ObjectType::ObjectType15:
            case ObjectType::ObjectType31:
            case ObjectType::ObjectType35:
            case ObjectType::ObjectType52:
                return true;
            default:
                return false;
        }
    }

    ObjectIconUv GetElementIconUv(int icon)
    {
        constexpr int kTilePx = 60;
        constexpr int kCols = 10;
        constexpr float kSheetW = 600.0f;
        constexpr float kSheetH = 1740.0f;
        const int col = icon % kCols;
        const int row = icon / kCols;
        const float u0 = static_cast<float>(col * kTilePx) / kSheetW;
        const float v0 = static_cast<float>(row * kTilePx) / kSheetH;
        const float u1 = static_cast<float>(col * kTilePx + kTilePx) / kSheetW;
        const float v1 = static_cast<float>(row * kTilePx + kTilePx) / kSheetH;
        return ObjectIconUv{u0, v0, u1, v1};
    }

    bool IsExploPngSourced(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::ObjectType8:
            case ObjectType::ObjectType9:
            case ObjectType::ObjectType10:
            case ObjectType::ObjectType11:
            case ObjectType::ObjectType53:
            case ObjectType::ObjectType90:
            case ObjectType::ObjectType91:
            case ObjectType::ObjectType92:
            case ObjectType::ObjectType93:
            case ObjectType::ObjectType98:
            case ObjectType::ObjectType99:
            case ObjectType::ObjectType100:
                return true;
            default:
                return false;
        }
    }

    ObjectIconUv GetExploIconUv(int icon)
    {
        constexpr int kTilePx = 144;
        constexpr int kCols = 10;
        constexpr float kSheetW = 1440.0f;
        constexpr float kSheetH = 1440.0f;
        const int col = icon % kCols;
        const int row = icon / kCols;
        const float u0 = static_cast<float>(col * kTilePx) / kSheetW;
        const float v0 = static_cast<float>(row * kTilePx) / kSheetH;
        const float u1 = static_cast<float>(col * kTilePx + kTilePx) / kSheetW;
        const float v1 = static_cast<float>(row * kTilePx + kTilePx) / kSheetH;
        return ObjectIconUv{u0, v0, u1, v1};
    }

    bool IsBlupiPngSourced(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::ObjectType200:
            case ObjectType::ObjectType201:
            case ObjectType::ObjectType202:
            case ObjectType::ObjectType203:
            case ObjectType::ObjectType38:
                return true;
            default:
                return false;
        }
    }

    bool UsesBlupi1Texture(ObjectType type)
    {
        switch (type)
        {
            case ObjectType::ObjectType201:
            case ObjectType::ObjectType202:
            case ObjectType::ObjectType203:
            case ObjectType::ObjectType38:
                return true;
            default:
                return false;
        }
    }

    bool IsBlupiPngSourcedAtPhase(ObjectType type, int phase)
    {
        if (type == ObjectType::ObjectType38)
        {
            // Matches Decor.cpp's channel switch (~line 8997): blupi1.png
            // for the first 30 of the 90-tick cycle, element.png after.
            return (phase % 90) < 30;
        }
        return IsBlupiPngSourced(type);
    }

    ObjectIconUv GetBlupiIconUv(int icon)
    {
        constexpr int kTilePx = 60;
        constexpr int kCols = 10;
        constexpr float kSheetW = 600.0f;
        constexpr float kSheetH = 2040.0f;
        const int col = icon % kCols;
        const int row = icon / kCols;
        const float u0 = static_cast<float>(col * kTilePx) / kSheetW;
        const float v0 = static_cast<float>(row * kTilePx) / kSheetH;
        const float u1 = static_cast<float>(col * kTilePx + kTilePx) / kSheetW;
        const float v1 = static_cast<float>(row * kTilePx + kTilePx) / kSheetH;
        return ObjectIconUv{u0, v0, u1, v1};
    }
}
