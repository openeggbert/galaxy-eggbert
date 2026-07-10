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
            // (56/57), the real per-frame layout isn't known without reading
            // Tables.cpp, so only the documented first-frame icon is
            // returned (no animation) rather than guessing a cycle that
            // would run off the sheet.
            case ObjectType::ObjectType23: return 176;
            case ObjectType::ObjectType27: return 152 + (p / 6) % 24;
            case ObjectType::ObjectType28: return 167;
            case ObjectType::ObjectType29: return 177;
            case ObjectType::ObjectType34: return 168 + (p / 6) % 25;
            case ObjectType::ObjectType36: return 179 + (p / 6) % 8;
            case ObjectType::ObjectType37: return 40 + (p / 6) % 70;
            case ObjectType::ObjectType39: return 166 + (p / 6) % 11;
            case ObjectType::ObjectType41: return 179 + (p / 6) % 8;
            case ObjectType::ObjectType42: return 186 + (p / 6) % 8;
            case ObjectType::ObjectType56: return 253; // 100 frames would exceed the sheet (253+99=352 > 289) -- first-frame only
            case ObjectType::ObjectType57: return 274; // 20 frames would exceed the sheet (274+19=293 > 289) -- first-frame only
            case ObjectType::ObjectType97: return 256 + (p / 6) % 5;
            // object-m.png-sourced Category B types (2026-07-09) -- these
            // are NOT element.png icons; GetElementIconUv() would compute
            // the wrong UV rect for them. The icon numbers below are only
            // meaningful when looked up via GETileAtlas::GetTileUv()
            // (object-m.png's 440-icon grid) -- see IsObjectMPngSourced()
            // and GalaxyEggbertCnaGame.cpp's dedicated dispatch, mirroring
            // the existing IsUniformCubeObject() precedent.
            case ObjectType::ObjectType14: return 99 + (p / 6) % 7;
            case ObjectType::ObjectType15: return 103 + (p / 6) % 20;
            case ObjectType::ObjectType31: return 238 + (p / 6) % 6;
            case ObjectType::ObjectType35: return 244 + (p / 6) % 3;
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
            case ObjectType::ObjectType8:   return 0 + (p / 6) % 39;
            case ObjectType::ObjectType9:   return 12 + (p / 6) % 20;
            case ObjectType::ObjectType10:  return 32 + (p / 6) % 20;
            case ObjectType::ObjectType11:  return 12 + (p / 6) % 9;
            case ObjectType::ObjectType53:  return 86; // 45 frames would exceed the sheet (86+44=130 > 99)
            case ObjectType::ObjectType90:  return 54 + (p / 6) % 12;
            case ObjectType::ObjectType91:  return 54 + (p / 6) % 6;
            case ObjectType::ObjectType92:  return 60; // 128 frames would exceed the sheet (60+127=187 > 99)
            case ObjectType::ObjectType93:  return 7 + (p / 6) % 5;
            case ObjectType::ObjectType98:  return 90 + (p / 6) % 10;
            case ObjectType::ObjectType99:  return 90; // first real frame after 3 documented invisible ticks
            case ObjectType::ObjectType100: return 90; // first real frame after 8 documented invisible ticks

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
