#include "GEBlupiController.hpp"
#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr int kWorldCenterX = GEWorldRuntime::kWorldCenterX;
        constexpr int kWorldCenterZ = GEWorldRuntime::kWorldCenterZ;

        int ClampGrid(int v, int blocksPerAxis)
        {
            return std::clamp(v, 0, blocksPerAxis - 1);
        }

        // blupi.png icon indices, ported from GalaxyEggbertSimple3D's own
        // already-approved GEBlupiController.cpp (kMarchFrames/kJumpFrames/
        // kDownFrames/kUpFrames/kAirFrames) — not a fresh mobile-eggbert
        // transcription.
        //
        // kStopFrames replaced 2026-07-18 (user question: "does Blupi get
        // bored and tap his foot after standing still for a while?") --
        // the OLD single-frame `{0}` here was a Simple3D-era placeholder,
        // never the real data. The real `table_blupi` Stop record (id=1)
        // is 330 frames long, not 1 -- confirmed this is NOT a separate
        // "boredom timer" mechanism, just the natural consequence of Stop's
        // own real animation being a long (16.5s at the 20Hz reference
        // rate) idle-fidget cycle: mostly icon 0 (plain idle pose) with
        // periodic short twitches (23, 133) and one longer gesture
        // (135-138) baked directly into the table. Extracted via the same
        // table_blupi parser as every other array in this file (see
        // kChargeFrames' own comment for the validation method).
        constexpr int kStopFrames[] = {
            0, 0, 0, 0, 0, 23, 23, 23, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 23, 23, 23, 23, 23, 23, 0, 0, 0, 0, 0, 0, 0, 0, 23,
            23, 23, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 23, 23,
            23, 23, 0, 0, 0, 23, 23, 23, 0, 0, 0, 0, 0, 0, 133, 133,
            0, 0, 0, 133, 133, 0, 0, 0, 0, 0, 0, 0, 133, 133, 0, 0,
            0, 0, 23, 23, 23, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 23, 23, 0, 0, 0, 0, 0, 23, 23, 23, 0, 0, 0, 135, 135,
            136, 136, 137, 137, 137, 137, 137, 137, 137, 137, 138, 138, 137, 137, 137, 138,
            138, 137, 137, 137, 138, 138, 137, 137, 137, 138, 138, 137, 137, 137, 137, 137,
            137, 136, 136, 135, 135, 135, 0, 0, 0, 0, 0, 23, 23, 23, 0, 0,
            133, 133, 0, 0, 0, 23, 23, 23, 23, 0, 0, 0, 0, 0, 0, 0,
            23, 23, 0, 0, 0, 0, 0, 0, 0, 0, 133, 133, 0, 0, 0, 0,
            0, 23, 23, 23, 23, 0, 0, 0, 135, 135, 136, 136, 137, 137, 137, 137,
            137, 137, 137, 137, 138, 138, 137, 137, 137, 138, 138, 137, 137, 137, 138, 138,
            137, 137, 137, 138, 138, 137, 137, 137, 138, 138, 137, 137, 137, 138, 138, 137,
            137, 137, 138, 138, 137, 137, 137, 138, 138, 137, 137, 137, 138, 138, 137, 137,
            137, 138, 138, 137, 137, 137, 138, 138, 137, 137, 137, 138, 138, 137, 137, 137,
            137, 137, 137, 136, 136, 135, 135, 135, 0, 0, 0, 0, 0, 23, 23, 23,
            0, 0, 133, 133, 0, 0, 0, 23, 23, 23, 23, 0, 0, 0, 0, 0,
            0, 0, 23, 23, 0, 0, 0, 0, 0, 0, 0, 0, 133, 133, 0, 0,
            0, 0, 0, 23, 23, 23, 23, 0, 0, 0};
        constexpr int kMarchFrames[] = {5, 6, 7, 8, 9, 10};
        constexpr int kJumpFrames[]  = {17, 18, 19};
        constexpr int kAirFrames[]   = {169, 26, 170, 170, 27};
        constexpr int kDownFrames[]  = {33, 34, 35};
        constexpr int kUpFrames[]    = {44};

        // blupi.png icon indices for StopEcrase(72)/MarchEcrase(73)/
        // Balloon(66)/Teleporting(74) -- a fresh, narrowly-scoped
        // transcription of exactly these 4 mobile-eggbert
        // Tables::table_blupi records, added 2026-07-11 with explicit user
        // approval (plan.md E3D-MIG-064; unlike the 5 arrays above, no
        // pre-approved in-repo source existed for these). -1 is the real
        // "invisible frame" sentinel (same convention as the Temp tile,
        // 02-tiles.md) -- GetAnimIcon() below substitutes icon 0 (the real
        // idle/Stop pose) for it, a deliberate simplification since this
        // debug HUD indicator has no "draw nothing this frame" mechanism,
        // not a claim that icon 0 is what real mobile-eggbert shows there.
        constexpr int kStopEcraseFrames[]  = {320};
        constexpr int kMarchEcraseFrames[] = {
            319, 319, 318, 318, 317, 317, 318, 318, 319, 319, 320, 320,
            321, 321, 322, 322, 323, 323, 322, 322, 321, 321, 320, 320};
        constexpr int kBalloonFrames[] = {
            291, 291, 292, 292, 293, 293, 294, 294,
            295, 295, 294, 294, 293, 293, 292, 292};
        constexpr int kTeleportingFrames[] = {
            1, 1, 2, 2, 3, 3, 4, 4, 270, 270, 269, 269, 268, 268, 0, 0,
            1, 2, 3, 4, 270, 269, 268, 0, 1, 2, 3, 4, 270, 269, 268, 0,
            1, 3, 270, 268, 2, 4, 269, 0, 1, 3, 270, 268, 2, 4, 269, 0,
            -1, 3, 270, -1, 2, -1, 269, 0, 1, -1, -1, 268, -1, -1, 269, -1,
            -1, -1, -1, 270, -1, -1, 2, -1, -1, -1, -1, -1, -1, 29, 46, 47,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

        // blupi.png icon indices for the 6 real DeathCause hurt-sprite states
        // (Clear1/2/3/4/Glu/Drown, plan.md `067`/`158`) and the 3 real
        // PickupFreezeKind states (Sucette/Drink/Charge, plan.md `173`) --
        // parsed directly out of ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/
        // Tables.cpp's own table_blupi (a small script mirroring
        // Decor.cpp:2393-2400's own [actionID, frameCount, threshold,
        // frame0..frameN-1] record layout, not by hand), added 2026-07-16
        // under the user's standing blanket approval (2026-07-14) for small
        // real data-table transcriptions -- validated by cross-checking the
        // same script against BlupiAction::Teleporte(74) first, which
        // reproduced kTeleportingFrames above byte-for-byte. -1 is the real
        // "invisible frame" sentinel (same convention as kTeleportingFrames);
        // GetAnimIcon() substitutes icon 0 for it, same simplification.
        constexpr int kClear1Frames[] = {
            40, 40, 40, 40, 41, 41, 41, 41, 40, 40, 40, 40, 40, 40, 40, 41,
            41, 41, 40, 40, 40, 40, 40, 40, 40, 41, 41, 41, 40, 40, 42, 42,
            42, 43, 43, 43, 44, 44, 44, 45, 45, 45, 46, 46, 47, 47, 46, 46,
            47, 47, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46, 47, 47, 46, 46,
            47, 47, 46, 46, 47, 47};
        constexpr int kClear2Frames[] = {-1};
        constexpr int kClear3Frames[] = {
            40, 40, 40, 40, 41, 41, 41, 41, 40, 40, 40, 40, 40, 40, 40, 41,
            41, 41, 40, 40, 40, 40, 40, 40, 40, 41, 41, 41, 40, 40, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1};
        constexpr int kClear4Frames[] = {
            324, 325, 324, 325, 324, 325, 324, 325, 324, 325, 324, 325, 324, 325,
            324, 325, 324, 325, 324, 325, 324, 324, 325, 325, 326, 326, 327, 327,
            328, 328, 329, 329, 330, 330, 331, 331, 332, 332, 333, 333, 334, 334,
            333, 333, 332, 332, 331, 331, 330, 330, 329, 329, 329, 330, 330, 330,
            331, 331, 331, 332, 332, 332, 333, 333, 333, 334, 334, 334, 333, 333,
            333, 332, 332, 332, 331, 331, 331, 330, 330, 330, 329, 329, 329, 329,
            329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329, 329,
            329, 329, 329, 329, 329, 329, 329, 329, 329, 329};
        constexpr int kGluFrames[] = {
            168, 168, 169, 169, 170, 170, 171, 171, 170, 170, 169, 169, 168, 168,
            169, 169, 169, 168, 168, 169, 169, 170, 170, 169, 168};
        constexpr int kDrownFrames[] = {
            93, 96, 98, 94, 95, 93, 95, 98, 93, 94, 96, 96, 94, 94, 98, 98,
            93, 93, 97, 97, 94, 94, 94, 96, 96, 96, 93, 93, 93, 93, 94, 94,
            94, 94, 94, 97, 97, 97, 97, 97, 76, 76, 76, 76, 76, 76, 76, 76,
            76, 76, 79, 79, 76, 76, 76, 76, 79, 79, 76, 76, 76, 76, 76, 76,
            76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76,
            76, 76, 76, 76, 76, 76, 76, 76, 76, 76};
        constexpr int kSucetteFrames[] = {
            234, 234, 235, 235, 236, 236, 235, 235, 234, 234, 235, 235, 236, 236,
            235, 235, 234, 234, 235, 235, 236, 236, 235, 235, 234, 234, 235, 235,
            236, 236, 235, 235};
        constexpr int kDrinkFrames[] = {253, 253, 254, 254};
        constexpr int kChargeFrames[] = {
            1, 3, 270, 268, 1, 3, 270, 268, 1, 3, 270, 268, 1, 3, 270, 268,
            1, 3, 270, 268, 1, 3, 270, 268, 1, 3, 270, 268, 1, 3, 270, 268,
            1, 2, 3, 4, 270, 269, 268, 0, 1, 2, 3, 4, 270, 269, 268, 0,
            1, 1, 2, 2, 3, 3, 4, 4, 270, 270, 269, 269, 268, 268, 0, 0};

        // blupi.png icon indices for vehicle Stop/March, Swim/Surf Stop/
        // March, and Hide (plan.md BLUPI-037/038/047/058/069/084/088/091/
        // 094/040/041/043/044/051, found 2026-07-18 while auditing the HUD
        // animation icon's real scope) -- same extraction script as the
        // block above (validated against it byte-for-byte via kChargeFrames
        // and kTeleportingFrames), same user data-transcription approval.
        constexpr int kStopHelicoFrames[]  = {61};
        constexpr int kMarchHelicoFrames[] = {61, 62, 63, 62, 61, 64, 65, 64};
        constexpr int kStopJeepFrames[]  = {111, 111, 110, 110, 111, 111, 112, 112};
        constexpr int kMarchJeepFrames[] = {111, 111, 110, 110, 111, 111, 112, 112};
        constexpr int kStopTankFrames[] = {
            238, 238, 239, 239, 240, 240, 241, 241, 241, 241, 241, 241, 241, 241,
            241, 241, 240, 240, 241, 241, 240, 240, 241, 241, 241, 241, 241, 241,
            241, 241, 241, 241, 241, 241, 240, 240, 241, 241, 240, 240, 241, 241,
            241, 241, 241, 241, 241, 241, 240, 240, 239, 239, 238, 238, 238, 238,
            238, 238, 237, 237, 238, 238, 237, 237};
        constexpr int kMarchTankFrames[] = {238, 238, 237, 237, 238, 238, 239, 239};
        constexpr int kStopSkateFrames[] = {
            182, 182, 182, 182, 208, 208, 208, 208, 208, 182, 182, 182, 182, 182,
            182, 208, 208, 208, 208, 208, 194, 194, 194, 182, 182, 182, 182, 208,
            208, 208, 208, 208, 182, 182, 182, 182, 182, 182, 208, 208, 208, 208,
            182, 182, 182, 182, 182, 182, 182, 182, 194, 194, 194, 195, 195, 195,
            196, 196, 196, 197, 197, 197, 198, 198, 198, 198, 197, 197, 197, 196,
            196, 196, 195, 195, 195, 194, 194, 194, 182, 182, 182, 182, 208, 208,
            208, 208, 182, 182, 182, 182, 194, 194, 194, 195, 195, 195, 196, 196,
            196, 197, 197, 197, 198, 198, 198, 198, 197, 197, 197, 196, 196, 196,
            195, 195, 195, 194, 194, 194, 182, 182, 182, 182, 208, 208, 208, 208,
            182, 182, 182, 182, 210, 210, 211, 211, 211, 211, 211, 211, 210, 210};
        constexpr int kMarchSkateFrames[] = {
            182, 183, 184, 185, 186, 187, 182, 183, 184, 185, 186, 187, 182, 183,
            184, 185, 186, 187, 182, 182, 182, 182, 182, 182, 182, 183, 184, 185,
            186, 187, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182,
            182, 183, 184, 185, 186, 187, 182, 182, 182, 182, 182, 182, 182, 182,
            182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 183, 184, 185,
            186, 187, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182,
            182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182, 182};
        constexpr int kJumpSkateFrames[] = {210, 211, 212};
        constexpr int kAirSkateFrames[]  = {213, 213, 214, 214, 215, 215, 214, 214};
        constexpr int kStopOverFrames[]  = {315};
        constexpr int kMarchOverFrames[] = {
            296, 297, 298, 299, 300, 301, 302, 301, 300, 299, 298, 297};
        constexpr int kStopNageFrames[] = {76, 76, 76, 76, 76, 76, 77, 77, 77, 77};
        constexpr int kMarchNageFrames[] = {
            76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 81, 39, 39};
        constexpr int kStopSurfFrames[] = {
            93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 98};
        constexpr int kMarchSurfFrames[] = {
            93, 93, 94, 94, 95, 95, 96, 96, 97, 97, 98, 98};
        constexpr int kHideFrames[] = {276, 277, 278, 279, 280, 281, 282, 283, 284};
        constexpr int kPushFrames[] = {49, 50, 51, 52, 53, 54};
        constexpr int kSwitchFrames[] = {0, 268, 268, 269, 269, 269, 269, 268, 268, 0};
        constexpr int kFireTankFrames[] = {251, 251, 238, 238, 238, 238};
        constexpr int kTakeDynamiteFrames[] = {
            1, 1, 2, 2, 41, 41, 42, 42, 43, 43, 42, 42, 41, 41, 2, 2, 1, 1};
        constexpr int kPutDynamiteFrames[] = {
            135, 135, 137, 137, 231, 231, 231, 231, 230, 230, 231, 231, 231, 231,
            230, 230, 231, 231, 231, 231, 230, 230, 137, 137, 135, 135};
        constexpr int kTakeSkateFrames[] = {
            17, 17, 18, 18, 19, 19, 1, 1, 215, 215, 214, 214, 213, 213, 212, 212,
            211, 211, 210, 210};
        constexpr int kDeposeSkateFrames[] = {
            210, 210, 211, 211, 212, 212, 213, 213, 214, 214, 215, 215, 1, 1, 19,
            19, 18, 18, 17, 17};
        constexpr int kMockeryFrames[] = {
            263, 264, 265, 264, 263, 264, 265, 264, 263, 264, 265, 264, 263, 264,
            265, 264, 263, 264, 265, 264, 263, 264, 265, 264, 263, 264, 265, 264,
            263, 264, 265, 264, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23, 23, 23, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 23, 23, 0, 0, 0, 0, 0, 0, 0, 133, 133, 0,
            0, 0, 133, 133, 0, 0, 0, 0, 0, 0, 0, 133, 133, 0, 0, 0, 0, 23, 23,
            23, 23, 0, 0, 0, 0};
        constexpr int kMockeryiFrames[] = {
            1, 1, 2, 2, 3, 3, 285, 286, 287, 286, 285, 286, 287, 286, 285, 286,
            287, 286, 285, 286, 287, 286, 285, 286, 287, 286, 285, 286, 287, 286,
            285, 286, 287, 286, 285, 286, 287, 286, 3, 3, 2, 2, 1, 1, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 23, 23, 23, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 23,
            23, 0, 0, 0, 0, 0, 0, 0, 133, 133, 0, 0, 0, 133, 133, 0, 0, 0, 0,
            0, 0, 0, 133, 133, 0, 0, 0, 0, 23, 23, 23, 23, 0, 0, 0, 0};
        constexpr int kMockerypFrames[] = {
            1, 1, 288, 288, 289, 289, 290, 290, 290, 289, 288, 288, 289, 289,
            290, 290, 290, 289, 288, 288, 289, 289, 290, 290, 290, 289, 288,
            288, 289, 289, 290, 290, 290, 289, 288, 288, 289, 289, 290, 290,
            290, 289, 288, 288, 289, 289, 290, 290, 290, 290, 290, 290, 290,
            290, 290, 290, 290, 290, 1, 1};

        // Teleporter pillars (plan.md E3D-MIG-147, icons 330-333) are
        // ALWAYS non-solid for collision purposes (unlike Temp, this isn't
        // phase-gated) -- see GroundHeightAt's own comment for why.
        bool IsTeleporterIcon(std::uint16_t type)
        {
            return type == GalaxyEggbert::BlockTypes::Teleport1 || type == GalaxyEggbert::BlockTypes::Teleport2 ||
                   type == GalaxyEggbert::BlockTypes::Teleport3 || type == GalaxyEggbert::BlockTypes::Teleport4;
        }
    }

    void GEBlupiController::SetPosition(float x, float y, float z) noexcept
    {
        m_x = x;
        m_y = y;
        m_z = z;
        m_velocityY = 0.0f;
        m_onGround = false;
    }

    void GEBlupiController::RideLift(float x, float y, float z) noexcept
    {
        m_x = x;
        m_y = y;
        m_z = z;
        m_velocityY = 0.0f;
        m_onGround = true;
    }

    bool GEBlupiController::IsSolidAt(const Worlds::World& world, int gx, int gy, int gz)
    {
        return !world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                               static_cast<std::uint16_t>(gz)).isAir();
    }

    int GEBlupiController::GroundHeightAt(const Worlds::World& world, int gx, int gz, bool tempPassable,
                                           float referenceY)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        // See this method's own header comment (GEBlupiController.hpp) for
        // why the scan no longer unconditionally starts at the world's
        // topmost Y.
        const int startY = std::min(blocksPerAxis - 1, static_cast<int>(std::floor(referenceY)) + 1);
        for (int y = startY; y >= 0; --y)
        {
            if (IsSolidAt(world, gx, y, gz))
            {
                const auto blockType = world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(y),
                                                        static_cast<std::uint16_t>(gz))
                                            .type();
                // Vanishing/Temp tile (plan.md E3D-MIG-146): during its
                // real passable window, it is NOT solid ground -- keep
                // scanning downward instead of stopping here, so Blupi
                // genuinely falls through to whatever (if anything) is
                // beneath it, matching the real IsBlocIcon(324) becoming
                // false for those 2 of 20 phase buckets.
                if (tempPassable && blockType == GalaxyEggbert::BlockTypes::Temp)
                {
                    continue;
                }
                // Teleporter pillar (plan.md E3D-MIG-147): ALWAYS non-solid
                // for collision, not phase-gated like Temp. Real
                // mobile-eggbert's collision is genuinely per-tile-
                // independent (a tile's solidity has no bearing on the
                // tile below it in the same column), which is how Blupi
                // can walk directly beneath a solid-LOOKING teleporter
                // pillar in the real game. This engine's simplified
                // column-based collision (topmost solid block = that
                // column's floor) would otherwise make Blupi land ON a
                // floating pillar instead of standing in the open space
                // beneath it -- confirmed empirically during this task.
                // Excluding teleporter icons from ground-height resolution
                // entirely reproduces the real walk-under behavior without
                // a general per-cell-occupancy collision rewrite.
                if (IsTeleporterIcon(blockType))
                {
                    continue;
                }
                // Fan head icons (plan.md E3D-MIG-149): ALWAYS non-solid,
                // same reasoning and same real per-tile-independent-
                // collision precedent as the teleporter pillar above --
                // real mobile-eggbert's IsVentillo() check requires Blupi
                // to actually be AT the fan's own tile, which is
                // unreachable here unless the fan (and anything else
                // occupying its column) stops blocking the column's
                // ground-height resolution.
                if (GalaxyEggbert::BlockTypes::isFan(blockType))
                {
                    continue;
                }
                // Water (plan.md E3D-MIG-148): ALWAYS non-solid, same
                // reasoning/precedent as teleporter pillars and fan heads
                // above -- real mobile-eggbert's water is genuinely
                // passable (Blupi swims/sinks through it, resting on
                // whatever solid floor is beneath), unlike this engine's
                // default "any non-air block is solid ground" rule. Without
                // this, Blupi would always rest ON TOP of the topmost water
                // layer (same as standing on land), making a multi-layer
                // deep pool -- and therefore Nage/drowning -- structurally
                // unreachable, the same category of bug already fixed for
                // the teleporter/fan.
                if (GalaxyEggbert::BlockTypes::isWater(blockType))
                {
                    continue;
                }
                return y + 1;
            }
        }
        // No solid block anywhere in this column -- see kNoGround's own
        // comment for why this must not be treated as solid ground at
        // Y=0.
        return kNoGround;
    }

    int GEBlupiController::CeilingHeightAt(const Worlds::World& world, int gx, int gz, float referenceY)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        // +1: start one cell ABOVE referenceY, mirroring GroundHeightAt()'s
        // own "+1" (there, one cell above referenceY scanning down; here,
        // one cell above referenceY scanning up) -- excludes whatever cell
        // referenceY itself is currently embedded in, which must already be
        // open (that's where the caller is standing/rising through).
        const int startY = std::max(0, static_cast<int>(std::floor(referenceY)) + 1);
        for (int y = startY; y < blocksPerAxis; ++y)
        {
            if (IsSolidAt(world, gx, y, gz))
            {
                const auto blockType = world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(y),
                                                        static_cast<std::uint16_t>(gz))
                                            .type();
                // Same non-solid exclusions as GroundHeightAt() -- real
                // per-tile-independent collision applies the same way
                // scanning up as scanning down (Temp always solid here since
                // a rising Blupi isn't the "fall through" case tempPassable
                // exists for; teleporter/fan/water stay non-solid).
                if (IsTeleporterIcon(blockType) || GalaxyEggbert::BlockTypes::isFan(blockType) ||
                    GalaxyEggbert::BlockTypes::isWater(blockType))
                {
                    continue;
                }
                return y;
            }
        }
        return kNoCeiling;
    }

    bool GEBlupiController::HasJumpHeadroom(const Worlds::World& world) const
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        // "One tile above Blupi's center, probes two stacked tiles overhead"
        // -- the 2 grid cells directly above his current standing height.
        const int gy0 = static_cast<int>(std::lround(m_y)) + 1;
        for (int gy = gy0; gy < gy0 + 2; ++gy)
        {
            if (gy < 0 || gy >= blocksPerAxis)
            {
                continue; // off the top of the world -- open sky, not blocked
            }
            if (IsSolidAt(world, gx, gy, gz))
            {
                return false;
            }
        }
        return true;
    }

    GEBlupiController::BarreCellType GEBlupiController::GetBarreCellType(const Worlds::World& world,
                                                                          int gx, int gy, int gz)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        if (gx < 0 || gx >= blocksPerAxis || gy < 0 || gy >= blocksPerAxis || gz < 0 || gz >= blocksPerAxis)
        {
            return BarreCellType::None;
        }
        const auto icon = world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                          static_cast<std::uint16_t>(gz))
                               .type();
        if (icon != 138 && icon != 202)
        {
            return BarreCellType::None;
        }
        const bool belowSolid = gy > 0 && IsSolidAt(world, gx, gy - 1, gz);
        return belowSolid ? BarreCellType::LandingAvailable : BarreCellType::Hanging;
    }

    bool GEBlupiController::TriggerCrush() noexcept
    {
        if (m_ecrase)
        {
            return false;
        }
        m_ecrase = true;
        m_ecraseTimer = kEcraseDuration;
        m_velocityY = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerBalloon() noexcept
    {
        if (m_balloon)
        {
            return false;
        }
        m_balloon = true;
        m_balloonTimer = kBalloonDuration;
        m_velocityY = 0.0f;
        m_balloonHorizontalSpeed = 0.0f;
        // Real trigger (Decor.cpp:5826-5849) force-exits any vehicle mode
        // (`ByeByeHelico()` + m_blupiHelico/Over/Jeep/Tank/Skate all set
        // false) the instant Blupi is stung -- needed here so the balloon
        // branch's own horizontal-drift momentum in Step() takes over
        // immediately instead of the vehicle's own ramp system still
        // holding priority via IsInVehicle().
        m_vehicleMode = VehicleMode::None;
        m_vehicleSpeed = 0.0f;
        return true;
    }

    void GEBlupiController::PopBalloon() noexcept
    {
        if (!m_balloon)
        {
            return;
        }
        m_balloon = false;
        m_balloonTimer = 0.0f;
        m_onGround = false; // real m_blupiAir = true
    }

    bool GEBlupiController::TriggerShield() noexcept
    {
        if (m_secretPower == SecretPower::Shield || m_secretPower == SecretPower::Hide ||
            m_secretPower == SecretPower::Power)
        {
            return false;
        }
        m_secretPower = SecretPower::Shield;
        m_secretPowerLevel = kSecretPowerMax;
        m_secretPowerTimer = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerPower() noexcept
    {
        if (m_secretPower == SecretPower::Shield)
        {
            return false;
        }
        m_secretPower = SecretPower::Power;
        m_secretPowerLevel = kSecretPowerMax;
        m_secretPowerTimer = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerCloud() noexcept
    {
        if (m_secretPower != SecretPower::None)
        {
            return false;
        }
        m_secretPower = SecretPower::Cloud;
        m_secretPowerLevel = kSecretPowerMax;
        m_secretPowerTimer = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerHide() noexcept
    {
        if (m_secretPower == SecretPower::Shield || m_secretPower == SecretPower::Cloud)
        {
            return false;
        }
        m_secretPower = SecretPower::Hide;
        m_secretPowerLevel = kSecretPowerMax;
        m_secretPowerTimer = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerInvert() noexcept
    {
        if (m_invert || m_secretPower == SecretPower::Hide)
        {
            return false;
        }
        m_invert = true;
        m_invertLevel = kInvertMax;
        m_invertTimer = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerMount(VehicleMode mode, bool inNage, bool inSurf) noexcept
    {
        // Real gate also excludes Balloon/Ecrase (Decor.cpp:5649/5669/5687: !m_blupiBalloon &&
        // !m_blupiEcrase), found missing here 2026-07-16 -- this method's own header comment
        // claimed "Ecrase already blocks separately via its own state elsewhere", which was never
        // actually true (no other check anywhere blocks mounting while squashed/ballooned).
        if (IsInVehicle() || inNage || inSurf || m_suspended || m_balloon || m_ecrase)
        {
            return false;
        }
        m_vehicleMode = mode;
        m_vehicleSpeed = 0.0f;
        if (m_secretPower == SecretPower::Cloud || m_secretPower == SecretPower::Hide)
        {
            m_secretPower = SecretPower::None;
            m_secretPowerLevel = 0;
            m_secretPowerTimer = 0.0f;
        }
        return true;
    }

    void GEBlupiController::TriggerDismount() noexcept
    {
        if (!IsInVehicle())
        {
            return;
        }
        m_vehicleMode = VehicleMode::None;
        m_vehicleSpeed = 0.0f;
    }

    bool GEBlupiController::ToggleGhost(const Worlds::World& world) noexcept
    {
        if (!m_ghost)
        {
            m_ghost = true;
            m_vehicleMode = VehicleMode::None;
            m_vehicleSpeed = 0.0f;
            m_suspended = false;
            m_velocityY = 0.0f;
            return m_ghost;
        }

        // Toggle-off gate: real `!DecorDetect(BlupiRect(m_blupiPos))`
        // (Decor.cpp:2065) -- silently rejected if the current position
        // is inside solid geometry, rather than stranding Blupi mid-wall.
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gy = ClampGrid(static_cast<int>(std::lround(m_y)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        if (!IsSolidAt(world, gx, gy, gz))
        {
            m_ghost = false;
        }
        return m_ghost;
    }

    bool GEBlupiController::TriggerSpringBounce(bool jumpHeld) noexcept
    {
        if (!m_onGround)
        {
            return false;
        }
        m_velocityY = jumpHeld ? kSpringBounceHeld : kSpringBounceNotHeld;
        m_onGround = false;
        return true;
    }

    bool GEBlupiController::TriggerTeleport(std::uint16_t icon) noexcept
    {
        // Real gate also excludes every vehicle mount (Decor.cpp:5593-5594:
        // !m_blupiHelico/Over/Jeep/Tank/Skate) -- this class's own comment
        // above once said "vehicles aren't modeled, so those clauses don't
        // apply", which stopped being true once VehicleMode was added; fixed
        // 2026-07-16 alongside the same gap in the Sucette/Drink/Charge
        // pickup gates (GalaxyEggbertCnaGame.cpp).
        if (m_teleporting || !m_onGround || m_balloon || m_ecrase || m_vehicleMode != VehicleMode::None)
        {
            return false;
        }
        m_teleporting = true;
        m_teleportTimer = kTeleportDuration;
        m_teleportIcon = icon;
        m_velocityY = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerBye() noexcept
    {
        if (m_bye || m_teleporting || m_deathLocked || m_deathLossVoyageActive || m_pickupFrozen ||
            m_balloon || m_ecrase)
        {
            return false;
        }
        m_bye = true;
        m_byeTimer = kByeDuration;
        m_velocityY = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerOneShotAnim(AnimState state, float durationSeconds) noexcept
    {
        if (m_oneShotAnimActive || m_bye || m_teleporting || m_deathLocked || m_deathLossVoyageActive ||
            m_pickupFrozen || m_balloon || m_ecrase)
        {
            return false;
        }
        m_oneShotAnimActive = true;
        m_oneShotAnimTimer = durationSeconds;
        m_oneShotAnimState = state;
        m_velocityY = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerMockery(AnimState variant) noexcept
    {
        // Real gate is implicitly satisfied by the caller only calling this
        // while GetAnimState()==Stop (every vehicle/swim/surf/hide/ecrase/
        // balloon mode already has its OWN AnimState taking precedence over
        // Stop in UpdateAnim(), so a real Stop reading already means "none
        // of those modes are active" -- no extra checks needed here beyond
        // the freezes/already-mocking/cooldown guards below).
        if (m_mockeryActive || !IsMockeryCooldownElapsed() || m_bye || m_oneShotAnimActive || m_teleporting ||
            m_deathLocked || m_deathLossVoyageActive || m_pickupFrozen)
        {
            return false;
        }
        m_mockeryActive = true;
        m_mockeryVariant = variant;
        m_mockeryCooldownTimer = kMockeryCooldown;
        switch (variant)
        {
            case AnimState::Mockery:  m_mockeryTimer = kMockeryDuration;  break;
            case AnimState::Mockeryi: m_mockeryTimer = kMockeryiDuration; break;
            case AnimState::Mockeryp:
            default:                  m_mockeryTimer = kMockerypDuration; break;
        }
        return true;
    }

    bool GEBlupiController::TriggerDeathLock(DeathCause cause, bool shouldRespawn) noexcept
    {
        if (m_deathLocked || m_deathLossVoyageActive)
        {
            return false;
        }
        // Real Decor.cpp:6374-6392 duration table (Config::ScaleTime(N), N in real ticks at this
        // build's 20Hz reference rate -- /20.0f to seconds, same conversion used throughout this
        // session).
        constexpr float kDeathLockTicks[] = {
            70.0f, // Clear1
            100.0f, // Clear2
            70.0f, // Clear3
            110.0f, // Clear4
            100.0f, // Glu
            90.0f, // Drown
        };
        // Real BlupiDead() unconditionally overwrites whatever action was active (Decor.cpp:
        // 6547-6614) -- a death always cancels a pending pickup freeze outright, the same
        // "always wins" precedence Step() already gives the death lock over pickup-freeze below.
        m_pickupFrozen = false;
        m_pickupFreezeTimer = 0.0f;
        m_pickupFreezeResolvedPending = false;
        // Real BlupiDead() (Decor.cpp:6547-6614) ALSO unconditionally clears vehicle mount/
        // Balloon/Ecrase/every secret power/Invert/Nage/Surf/Suspend/Ghost (found 2026-07-16 while
        // researching the large-creature ObjectType54 contact, which has its own manual
        // equivalent of this same reset because it bypasses BlupiDead() -- see
        // GEInteractionSystem.cpp's own comment). This engine's death-lock previously left every
        // one of these completely untouched across death/respawn -- a real, significant gap, not
        // just the narrow single-mechanic gates fixed earlier this session. Unlike voluntary/
        // spring-forced dismount, real BlupiDead() does NOT redeposit the vehicle pickup back into
        // the world (no such ObjectStart call in it) -- dying with one mounted just loses it, so
        // this class does nothing world-facing here, matching that (this class has no world
        // access regardless).
        m_vehicleMode = VehicleMode::None;
        m_vehicleSpeed = 0.0f;
        m_balloon = false;
        m_ecrase = false;
        m_secretPower = SecretPower::None;
        m_secretPowerLevel = 0;
        m_secretPowerTimer = 0.0f;
        m_invert = false;
        m_invertLevel = 0;
        m_invertTimer = 0.0f;
        m_nage = false;
        m_surf = false;
        m_suspended = false;
        m_ghost = false;
        m_deathLocked = true;
        m_deathLockTimer = kDeathLockTicks[static_cast<std::size_t>(cause)] / 20.0f;
        m_deathLockShouldRespawn = shouldRespawn;
        m_deathCause = cause;
        m_velocityY = 0.0f;
        return true;
    }

    bool GEBlupiController::TriggerPickupFreeze(PickupFreezeKind kind) noexcept
    {
        if (m_pickupFrozen || m_deathLocked || m_deathLossVoyageActive)
        {
            return false;
        }
        // Real Decor.cpp:6025-6087's own 3 independent durations (Config::ScaleTime(N)) -- /20.0f
        // to seconds, same conversion used throughout this session.
        constexpr float kPickupFreezeTicks[] = {
            32.0f, // Sucette
            36.0f, // Drink
            64.0f, // Charge
        };
        m_pickupFrozen = true;
        m_pickupFreezeTimer = kPickupFreezeTicks[static_cast<std::size_t>(kind)] / 20.0f;
        m_pickupFreezeKind = kind;
        m_velocityY = 0.0f;
        return true;
    }

    bool GEBlupiController::ConsumePickupFreezeResolved(PickupFreezeKind& outKind) noexcept
    {
        if (!m_pickupFreezeResolvedPending)
        {
            return false;
        }
        m_pickupFreezeResolvedPending = false;
        outKind = m_pickupFreezeKind;
        return true;
    }

    bool GEBlupiController::ConsumeDeathLockResolved(bool& outShouldRespawn) noexcept
    {
        if (!m_deathLockResolvedPending)
        {
            return false;
        }
        m_deathLockResolvedPending = false;
        outShouldRespawn = m_deathLockShouldRespawn;
        return true;
    }

    void GEBlupiController::UpdateSafePosition(bool externallySafe) noexcept
    {
        if (!(m_onGround && !m_balloon && !m_ecrase && externallySafe))
        {
            return;
        }
        // Real order (Decor.cpp:6474-6477): m_blupiValidPos is set to the
        // FIFO's oldest entry BEFORE the current position is pushed, not
        // after -- this is what gives the "don't respawn exactly where
        // you died" buffer, since the just-computed valid position always
        // lags at least one FIFO slot behind wherever Blupi currently is.
        if (m_safeFifoCount > 0)
        {
            m_validX = m_safeFifo[0][0];
            m_validY = m_safeFifo[0][1];
            m_validZ = m_safeFifo[0][2];
        }
        const bool isDuplicate = m_safeFifoCount > 0 &&
                                  m_safeFifo[m_safeFifoCount - 1][0] == m_x &&
                                  m_safeFifo[m_safeFifoCount - 1][1] == m_y &&
                                  m_safeFifo[m_safeFifoCount - 1][2] == m_z;
        if (isDuplicate)
        {
            return;
        }
        if (m_safeFifoCount < kSafeFifoCapacity)
        {
            m_safeFifo[static_cast<std::size_t>(m_safeFifoCount)] = {m_x, m_y, m_z};
            ++m_safeFifoCount;
        }
        else
        {
            for (int i = 0; i < kSafeFifoCapacity - 1; ++i)
            {
                m_safeFifo[static_cast<std::size_t>(i)] = m_safeFifo[static_cast<std::size_t>(i) + 1];
            }
            m_safeFifo[kSafeFifoCapacity - 1] = {m_x, m_y, m_z};
        }
    }

    std::uint16_t GEBlupiController::GetGroundBlockType(const Worlds::World& world) const noexcept
    {
        if (!m_onGround)
        {
            return GalaxyEggbert::BlockTypes::Air;
        }
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        const int gy = static_cast<int>(std::lround(m_y)) - 1;
        if (gy < 0 || gy >= blocksPerAxis)
        {
            return GalaxyEggbert::BlockTypes::Air;
        }
        return world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                               static_cast<std::uint16_t>(gz))
            .type();
    }

    std::uint16_t GEBlupiController::GetBlockTypeAbove(const Worlds::World& world) const noexcept
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        const int gy = static_cast<int>(std::lround(m_y)) + 1;
        if (gy < 0 || gy >= blocksPerAxis)
        {
            return GalaxyEggbert::BlockTypes::Air;
        }
        return world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                               static_cast<std::uint16_t>(gz))
            .type();
    }

    std::uint16_t GEBlupiController::GetBlockTypeAt(const Worlds::World& world) const noexcept
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        const int gy = static_cast<int>(std::lround(m_y));
        if (gy < 0 || gy >= blocksPerAxis)
        {
            return GalaxyEggbert::BlockTypes::Air;
        }
        return world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                               static_cast<std::uint16_t>(gz))
            .type();
    }

    void GEBlupiController::TryMoveAxis(const Worlds::World& world, float ddx, float ddz, bool tempPassable)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const float candidateX = m_x + ddx;
        const float candidateZ = m_z + ddz;

        const int gx = ClampGrid(static_cast<int>(std::lround(candidateX + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(candidateZ + kWorldCenterZ)), blocksPerAxis);

        // referenceY = m_y + kStepLimit: allows detecting legitimate
        // step-up ground within reach, while a real ceiling higher than
        // that stays irrelevant (see GroundHeightAt's own comment).
        const int targetGroundY = GroundHeightAt(world, gx, gz, tempPassable, m_y + kStepLimit);

        // Allow the move if the destination column's ground is at most
        // kStepLimit above the current feet Y (step-up); any amount lower
        // is always fine (falling is handled by the vertical pass in
        // Step()). This step-up restriction only makes sense while
        // GROUNDED (climbing a curb while walking) -- while airborne
        // (`!m_onGround`, mid-jump or mid-fall), it's skipped entirely.
        // Real bug found 2026-07-12 while live-testing vehicles
        // (E3D-MIG-171): while falling through a floorless column, m_y
        // keeps dropping below any OTHER nearby column's real ground
        // height (or the kNoGround sentinel itself, once m_y drops below
        // about -2) -- the plain `targetGroundY <= m_y + kStepLimit`
        // comparison then flips false and silently blocks ALL further
        // horizontal movement for the rest of the fall, misreading "I'm
        // currently far below that column's ground because I'm falling"
        // as "that's an unclimbable wall". Not a vehicle-specific bug --
        // any sustained directional input held while falling off the
        // world edge hits this; previous fall-death tests only ever
        // dropped straight down with no horizontal input during the fall,
        // so it went uncaught until now.
        if (!m_onGround || static_cast<float>(targetGroundY) <= m_y + kStepLimit)
        {
            m_x = candidateX;
            m_z = candidateZ;
            if (m_onGround && static_cast<float>(targetGroundY) > m_y)
            {
                m_y = static_cast<float>(targetGroundY);
            }
        }
    }

    void GEBlupiController::Step(const Worlds::World& world, float turnInput, float moveInput,
                                  bool jumpPressed, bool crouchHeld, bool lookUpHeld, float dt,
                                  bool tempPassable, bool inSurfWater, bool inDeepWater, bool pushingCrate)
    {
        // Ghost mode (plan.md BLUPI-111) -- real absolute top priority:
        // `Decor::BlupiStep()`'s very first statement is `if (m_blupiGhost)
        // { BlupiGhostStep(); return; }`, before even teleporting. Real
        // `BlupiGhostStep()` (`Decor.cpp:2639-2705`) moves directly along
        // both real screen axes at 4x the normal per-frame speed, no
        // gravity, no collision, world-bounds clamp only -- see
        // kGhostSpeed's own comment for how the real X/Y-axis free flight
        // is adapted to this engine's tank-control scheme (forward/back
        // along facing + turn for horizontal, reusing jumpPressed/
        // crouchHeld for vertical since this engine has no other
        // real-mapped use for them once every other Step() branch below
        // is skipped).
        if (m_ghost)
        {
            if (turnInput != 0.0f)
            {
                m_yaw += turnInput * kTurnSpeed * dt;
            }
            m_x += std::sin(m_yaw) * kGhostSpeed * moveInput * dt;
            m_z += -std::cos(m_yaw) * kGhostSpeed * moveInput * dt;
            if (jumpPressed)
            {
                m_y += kGhostSpeed * dt;
            }
            if (crouchHeld)
            {
                m_y -= kGhostSpeed * dt;
            }

            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            m_x = std::clamp(m_x, static_cast<float>(-kWorldCenterX),
                              static_cast<float>(blocksPerAxis - 1 - kWorldCenterX));
            m_z = std::clamp(m_z, static_cast<float>(-kWorldCenterZ),
                              static_cast<float>(blocksPerAxis - 1 - kWorldCenterZ));
            m_y = std::clamp(m_y, 0.0f, static_cast<float>(blocksPerAxis));

            m_velocityY = 0.0f;
            m_onGround = false;

            const bool moving = moveInput != 0.0f;
            UpdateAnim(moving, false, false, dt);
            return;
        }

        // Water Surf/Nage status (plan.md E3D-MIG-148) -- set directly from
        // the caller's own per-frame terrain determination, same split as
        // tempPassable. The gauge only ticks while genuinely Nage, and
        // resets to full the instant Nage ends (matches the real "gauge
        // hidden" behavior on resurfacing/leaving the water).
        m_surf = inSurfWater;
        m_nage = inDeepWater;
        m_justDrowned = false;
        if (!m_nage)
        {
            m_waterGaugeLevel = kWaterGaugeMax;
            m_waterGaugeTimer = 0.0f;
        }
        else
        {
            // Real gate (Decor.cpp:4615-4620, found 2026-07-16): Shield/Hide grant immunity to the
            // gauge depleting AT ALL while submerged (`!m_blupiShield && !m_blupiHide` around the
            // decrement itself) -- this engine previously decremented unconditionally, a
            // previously-documented "not modeled" gap, now closed.
            const bool wasAboveZero = m_waterGaugeLevel > 0;
            const bool shieldOrHideImmune =
                m_secretPower == SecretPower::Shield || m_secretPower == SecretPower::Hide;
            m_waterGaugeTimer += dt;
            while (m_waterGaugeTimer >= kWaterGaugeTickSeconds && m_waterGaugeLevel > 0)
            {
                m_waterGaugeTimer -= kWaterGaugeTickSeconds;
                if (!shieldOrHideImmune)
                {
                    --m_waterGaugeLevel;
                }
            }
            m_justDrowned = wasAboveZero && m_waterGaugeLevel <= 0;
        }

        // Death-lock + life-loss Voyage (see TriggerDeathLock()'s own comment) -- real
        // `BlupiDead()` sets `m_blupiFocus=false` immediately (Decor.cpp:6547-6614), the same
        // mechanism Teleporte uses, so this takes the same top-priority early-return shape,
        // checked BEFORE teleporting since a death always overrides any other frozen state. Two
        // chained timers: the lock itself, then (once it elapses) the life-loss-Voyage window --
        // ConsumeDeathLockResolved() lets the caller know exactly once when to apply the real
        // `m_blupiRestart`-gated respawn, call LoseLife(), and start the icon-48 Voyage.
        if (m_deathLocked)
        {
            m_deathLockTimer -= dt;
            if (m_deathLockTimer <= 0.0f)
            {
                m_deathLocked = false;
                m_deathLockTimer = 0.0f;
                m_deathLossVoyageActive = true;
                m_deathLossVoyageTimer = kLifeLossVoyageDuration;
                m_deathLockResolvedPending = true;
            }
            UpdateAnim(false, false, false, dt);
            return;
        }
        if (m_deathLossVoyageActive)
        {
            m_deathLossVoyageTimer -= dt;
            if (m_deathLossVoyageTimer <= 0.0f)
            {
                m_deathLossVoyageActive = false;
                m_deathLossVoyageTimer = 0.0f;
            }
            UpdateAnim(false, false, false, dt);
            return;
        }

        // Real "Bye" farewell freeze (see TriggerBye()'s own comment) -- same freeze shape as
        // Teleporte, checked after death (a death always takes precedence over a pending Bye,
        // same reasoning as every other freeze here) but before pickup-freeze/teleport (mutually
        // exclusive in practice -- TriggerBye()'s own guard already refuses to fire over either).
        if (m_bye)
        {
            m_byeTimer -= dt;
            if (m_byeTimer <= 0.0f)
            {
                m_bye = false;
                m_byeTimer = 0.0f;
            }
            UpdateAnim(false, false, false, dt);
            return;
        }

        // Real one-shot action-animation freeze (Switch/TakeDynamite/PutDynamite, see
        // TriggerOneShotAnim()'s own comment) -- same freeze shape as Bye just above.
        if (m_oneShotAnimActive)
        {
            m_oneShotAnimTimer -= dt;
            if (m_oneShotAnimTimer <= 0.0f)
            {
                m_oneShotAnimActive = false;
                m_oneShotAnimTimer = 0.0f;
            }
            UpdateAnim(false, false, false, dt);
            return;
        }

        // Real Sucette/Drink/Charge 2-stage pickup delay (see TriggerPickupFreeze()'s own
        // comment) -- same freeze shape as the death lock/Teleporte, checked after both (a death
        // always cancels a pending pickup freeze, per TriggerDeathLock()'s own comment) but before
        // teleporting (mutually exclusive in practice -- real BlupiFocus-gated pickups can't be
        // touched mid-teleport anyway).
        if (m_pickupFrozen)
        {
            m_pickupFreezeTimer -= dt;
            if (m_pickupFreezeTimer <= 0.0f)
            {
                m_pickupFrozen = false;
                m_pickupFreezeTimer = 0.0f;
                m_pickupFreezeResolvedPending = true;
            }
            UpdateAnim(false, false, false, dt);
            return;
        }

        // Teleport transit (plan.md E3D-MIG-147): real BlupiAction::
        // Teleporte zeroes velocity once at trigger and drops m_blupiFocus,
        // which gates essentially every other per-frame input/gravity
        // block in the real source -- since he's grounded (not
        // m_blupiAir) when a teleport starts and nothing sets m_blupiAir
        // during it, he stays fully motionless for the whole transit. This
        // early-return reproduces that exactly: no turning, movement,
        // jumping, or gravity while teleporting, just the countdown.
        if (m_teleporting)
        {
            m_teleportTimer -= dt;
            if (m_teleportTimer <= 0.0f)
            {
                m_teleporting = false;
                m_teleportTimer = 0.0f;
            }
            // The Teleporting anim state (plan.md E3D-MIG-064) still needs
            // to advance during the freeze -- moving/crouchHeld/lookUpHeld
            // don't matter here since m_teleporting takes top precedence in
            // UpdateAnim()'s own cascade regardless of their values.
            UpdateAnim(false, false, false, dt);
            return;
        }

        // Suspended/hanging bar-and-rope mode (plan.md TILE-045, see
        // kSuspendMoveSpeed's own comment for the full real-behavior
        // citation). Grace timer ticks down regardless of state (real
        // m_blupiNoBarre decrements every tick unconditionally).
        if (m_suspendGraceTimer > 0.0f)
        {
            m_suspendGraceTimer = std::max(0.0f, m_suspendGraceTimer - dt);
        }

        // Grab trigger: automatic (no button), matches the real gate
        // exactly -- blocked while already suspended, in ANY vehicle,
        // ballooned, crushed, or genuinely in water (Nage/Surf), and during
        // the post-release grace window.
        if (!m_suspended && m_suspendGraceTimer <= 0.0f && !IsInVehicle() && !m_balloon && !m_ecrase &&
            !m_nage && !m_surf)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
            const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
            const int gy = ClampGrid(static_cast<int>(std::lround(m_y)), blocksPerAxis);
            if (GetBarreCellType(world, gx, gy, gz) == BarreCellType::Hanging)
            {
                // m_y stays exactly at the bar's own grid row while hanging
                // (not the usual "+1 standing height" convention) -- this
                // state bypasses normal ground physics entirely, so the
                // only requirement is internal self-consistency with the
                // round(m_y) reads used throughout the hang below.
                m_suspended = true;
                m_y = static_cast<float>(gy);
                m_velocityY = 0.0f;
                m_onGround = false;
                m_suspendDropHoldTimer = 0.0f;
            }
        }

        if (m_suspended)
        {
            // Real Turn animation while suspended has its own timing (this
            // engine has no visible model to show it, so plain yaw update
            // suffices, same simplification as everywhere else in Step()).
            if (turnInput != 0.0f)
            {
                m_yaw += turnInput * kTurnSpeed * dt;
            }

            // Real "speedX*5, no ramp" -- reuses the same instant
            // target-speed shape this engine's normal walk already uses
            // (see kSuspendMoveSpeed's own comment).
            const bool moving = std::fabs(moveInput) > 0.001f;
            if (moving)
            {
                m_x += std::sin(m_yaw) * kSuspendMoveSpeed * moveInput * dt;
                m_z += -std::cos(m_yaw) * kSuspendMoveSpeed * moveInput * dt;
            }

            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
            const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
            const int gy = ClampGrid(static_cast<int>(std::lround(m_y)), blocksPerAxis);
            const BarreCellType cellHere = GetBarreCellType(world, gx, gy, gz);

            // Real jump-to-release (Decor.cpp:4769-4789): instant here, not
            // the real 10-tick wind-up (no visible model to show it, see
            // kSuspendReleaseSpeed's own comment).
            if (jumpPressed)
            {
                m_suspended = false;
                m_onGround = false;
                m_velocityY = kSuspendReleaseSpeed;
                m_suspendGraceTimer = kSuspendNoRegrabSeconds;
            }
            // Real: holding Down for >5 ticks, or walking off the bar
            // entirely (no bar tile at the new position), drops Blupi into
            // free-fall.
            else if (crouchHeld)
            {
                m_suspendDropHoldTimer += dt;
                if (m_suspendDropHoldTimer > kSuspendDropHoldSeconds || cellHere == BarreCellType::None)
                {
                    m_suspended = false;
                    m_onGround = false;
                    m_velocityY = 0.0f;
                    m_suspendGraceTimer = kSuspendNoRegrabSeconds;
                }
            }
            else if (cellHere == BarreCellType::None)
            {
                m_suspended = false;
                m_onGround = false;
                m_velocityY = 0.0f;
                m_suspendGraceTimer = kSuspendNoRegrabSeconds;
                m_suspendDropHoldTimer = 0.0f;
            }
            else
            {
                m_suspendDropHoldTimer = 0.0f;
                // Real: reaching a cell where the landing below is clear
                // ends the hang gracefully, stepping down onto solid
                // ground instead of free-falling.
                if (cellHere == BarreCellType::LandingAvailable)
                {
                    m_suspended = false;
                    m_onGround = true;
                    m_y = static_cast<float>(gy);
                    m_velocityY = 0.0f;
                }
            }

            UpdateAnim(moving, false, false, dt);
            return;
        }

        // Tank controls (matches GalaxyEggbertSimple3D's "Move" axis
        // handling): turning changes yaw directly; movement is always along
        // the current facing direction, never a free strafe. 0 rad = facing
        // -Z, matching the forward vector the CNA camera derives from
        // GetYaw() (sin(yaw), 0, -cos(yaw)).
        if (turnInput != 0.0f)
        {
            m_yaw += turnInput * kTurnSpeed * dt;
        }

        // Crusher squash state: real behavior allows movement at reduced
        // speed but blocks jump entirely while squashed.
        const float effectiveMoveSpeed = m_ecrase ? kMoveSpeed * kEcraseSpeedMultiplier : kMoveSpeed;

        // Vehicle horizontal speed (plan.md E3D-MIG-171): unlike normal
        // walking (instant target speed, no ramp -- this engine's own
        // existing simplification, not a real transcription), vehicles
        // ramp `m_vehicleSpeed` toward the target at their own real
        // accel/decel rate, matching "a neutral input always decelerates
        // back toward zero, never instantly" -- so a vehicle keeps
        // coasting after input stops, unlike Blupi's own instant-stop walk.
        // Already signed (carries moveInput's own sign), so it's used
        // directly below instead of being multiplied by moveInput again.
        float horizontalSpeed;
        if (m_balloon)
        {
            // Balloon horizontal drift (kBalloonHorizontalSpeed's own
            // comment, direct port of Decor.cpp:4059-4106's real 3-branch
            // shape): a held direction accelerates toward a signed target of
            // +/-kBalloonHorizontalSpeed at kBalloonHorizontalAccel; no
            // input decelerates back toward exactly 0 at the real, faster
            // kBalloonHorizontalDecel rate (never overshooting past 0 into
            // the opposite sign) -- two visibly different rates, so this is
            // written as the real 3-way branch on moveInput's sign rather
            // than force-fit into the single-rate m_vehicleSpeed shape above.
            if (moveInput < 0.0f)
            {
                const float target = moveInput * kBalloonHorizontalSpeed;
                if (m_balloonHorizontalSpeed > target)
                {
                    m_balloonHorizontalSpeed =
                        std::max(m_balloonHorizontalSpeed - kBalloonHorizontalAccel * dt, target);
                }
            }
            else if (moveInput > 0.0f)
            {
                const float target = moveInput * kBalloonHorizontalSpeed;
                if (m_balloonHorizontalSpeed < target)
                {
                    m_balloonHorizontalSpeed =
                        std::min(m_balloonHorizontalSpeed + kBalloonHorizontalAccel * dt, target);
                }
            }
            else
            {
                if (m_balloonHorizontalSpeed > 0.0f)
                {
                    m_balloonHorizontalSpeed =
                        std::max(m_balloonHorizontalSpeed - kBalloonHorizontalDecel * dt, 0.0f);
                }
                else if (m_balloonHorizontalSpeed < 0.0f)
                {
                    m_balloonHorizontalSpeed =
                        std::min(m_balloonHorizontalSpeed + kBalloonHorizontalDecel * dt, 0.0f);
                }
            }
            horizontalSpeed = m_balloonHorizontalSpeed;
        }
        else if (IsInVehicle())
        {
            float maxSpeed = 0.0f;
            float decel = kVehicleAccel;
            switch (m_vehicleMode)
            {
                case VehicleMode::Jeep:        maxSpeed = kJeepMaxSpeed;       decel = kJeepDecel;       break;
                case VehicleMode::Tank:        maxSpeed = kTankMaxSpeed;       decel = kTankDecel;       break;
                case VehicleMode::Overcraft:   maxSpeed = kOvercraftMaxSpeed;  decel = kOvercraftDecel;  break;
                case VehicleMode::Skateboard:  maxSpeed = kSkateboardMaxSpeed; decel = kSkateboardDecel; break;
                case VehicleMode::Helicopter:  maxSpeed = kHelicopterMaxSpeed; decel = kHelicopterDecel; break;
                default: break;
            }
            const float target = moveInput * maxSpeed;
            const float rate = (std::fabs(target) > std::fabs(m_vehicleSpeed)) ? kVehicleAccel : decel;
            if (m_vehicleSpeed < target)
            {
                m_vehicleSpeed = std::min(m_vehicleSpeed + rate * dt, target);
            }
            else if (m_vehicleSpeed > target)
            {
                m_vehicleSpeed = std::max(m_vehicleSpeed - rate * dt, target);
            }
            horizontalSpeed = m_vehicleSpeed;
        }
        else
        {
            horizontalSpeed = moveInput * effectiveMoveSpeed;
            // Invert/Mirror (plan.md PICKUP-011, real `Decor::SetSpeedX`:
            // "if (m_blupiInvert) speed = -speed") -- negates normal ground
            // movement only; vehicles use their own separate real speed
            // system (m_vehicleSpeed above), not SetSpeedX, so this
            // deliberately does not apply to the IsInVehicle() branch.
            if (m_invert)
            {
                horizontalSpeed = -horizontalSpeed;
            }
        }

        const bool moving = std::fabs(horizontalSpeed) > 0.001f;

        // Real Mockery/Mockeryi/Mockeryp (see TriggerMockery()'s own
        // comment) -- NOT a freeze, so it's ticked/cancelled inline here
        // instead of via Step()'s early-return freeze pattern. Moving
        // interrupts it immediately (real source has no explicit "cancel"
        // for this -- the general movement code just overwrites
        // `m_blupiAction` away from the mockery variant the instant
        // `moving` becomes true, since nothing exempts Mockery the way
        // Ecrase/Balloon/Teleporte are exempted).
        if (m_mockeryActive)
        {
            if (moving)
            {
                m_mockeryActive = false;
            }
            else
            {
                m_mockeryTimer -= dt;
                if (m_mockeryTimer <= 0.0f)
                {
                    m_mockeryActive = false;
                }
            }
        }
        if (m_mockeryCooldownTimer > 0.0f)
        {
            m_mockeryCooldownTimer -= dt;
        }

        if (moving)
        {
            const float dx = std::sin(m_yaw) * horizontalSpeed * dt;
            const float dz = -std::cos(m_yaw) * horizontalSpeed * dt;
            if (dx != 0.0f)
            {
                TryMoveAxis(world, dx, 0.0f, tempPassable);
            }
            if (dz != 0.0f)
            {
                TryMoveAxis(world, 0.0f, dz, tempPassable);
            }
        }

        // Nage (fully submerged, plan.md E3D-MIG-148): Jump swims upward
        // instead of the normal ground jump, and works regardless of
        // m_onGround (mid-water, not resting on anything) -- a natural
        // adaptation for "swimming up", see kSwimUpSpeed's own comment.
        // Takes precedence over the normal ground jump below since a
        // ground-jump impulse doesn't make sense while submerged.
        if (m_nage && jumpPressed)
        {
            m_velocityY = kSwimUpSpeed;
            m_onGround = false;
        }
        else if (m_onGround && jumpPressed && !m_ecrase && !m_balloon &&
                 (m_vehicleMode == VehicleMode::None || m_vehicleMode == VehicleMode::Skateboard))
        {
            // Real Decor.cpp:2913-2947 ground-jump gate: excludes every
            // other vehicle mode (Helico/Over/Jeep/Tank, already excluded
            // above by the VehicleMode check; Nage/Surf/Suspend don't exist
            // in this engine) -- only Skateboard gets its OWN distinct
            // velocity (kSkateboardJumpSpeed/Powered), not the headroom-
            // modulated Decor::IsNormalJump() values below, which apply to
            // ordinary (no-vehicle) Blupi only.
            const bool powered = m_secretPower == SecretPower::Power;
            if (m_vehicleMode == VehicleMode::Skateboard)
            {
                m_velocityY = powered ? kSkateboardJumpSpeedPowered : kSkateboardJumpSpeed;
            }
            else if (HasJumpHeadroom(world))
            {
                m_velocityY = powered ? kJumpSpeedPowered : kJumpSpeed;
            }
            else
            {
                m_velocityY = powered ? kJumpSpeedReducedPowered : kJumpSpeedReduced;
            }
            m_onGround = false;
        }

        if (m_ecrase)
        {
            m_ecraseTimer -= dt;
            if (m_ecraseTimer <= 0.0f)
            {
                m_ecrase = false;
                m_ecraseTimer = 0.0f;
            }
        }

        if (m_balloon)
        {
            m_balloonTimer -= dt;
            if (m_balloonTimer <= 0.0f)
            {
                m_balloon = false;
                m_balloonTimer = 0.0f;
            }
        }

        // Secret powers (plan.md E3D-MIG-170/172): the shared gauge ticks
        // down at whichever real rate matches the currently-active power
        // (see kShieldTickSeconds/kPowerTickSeconds/kCloudTickSeconds/
        // kHideTickSeconds's own comment) until it reaches 0, then clears
        // to None -- real warning-sound thresholds are exposed via
        // JustCrossedSecretPowerWarning() below for the caller to play the
        // real per-power channel.
        m_secretPowerJustWarned = false;
        if (m_secretPower != SecretPower::None)
        {
            const float tickSeconds = m_secretPower == SecretPower::Shield ? kShieldTickSeconds
                                     : m_secretPower == SecretPower::Power  ? kPowerTickSeconds
                                     : m_secretPower == SecretPower::Hide   ? kHideTickSeconds
                                                                            : kCloudTickSeconds;
            const int warnLevel = m_secretPower == SecretPower::Shield ? kShieldWarnLevel
                                 : m_secretPower == SecretPower::Power  ? kPowerWarnLevel
                                 : m_secretPower == SecretPower::Hide   ? kHideWarnLevel
                                                                        : kCloudWarnLevel;
            m_secretPowerTimer += dt;
            while (m_secretPowerTimer >= tickSeconds && m_secretPowerLevel > 0)
            {
                m_secretPowerTimer -= tickSeconds;
                --m_secretPowerLevel;
                if (m_secretPowerLevel == warnLevel)
                {
                    m_secretPowerJustWarned = true;
                }
            }
            if (m_secretPowerLevel <= 0)
            {
                m_secretPower = SecretPower::None;
                m_secretPowerTimer = 0.0f;
            }
        }

        // Invert/Mirror (plan.md PICKUP-011): independent gauge, same shape
        // as the secret-power tick above but no warning stage (see
        // kInvertMax's own comment) -- just a one-shot expiry flag.
        m_invertJustExpired = false;
        if (m_invert)
        {
            m_invertTimer += dt;
            while (m_invertTimer >= kInvertTickSeconds && m_invertLevel > 0)
            {
                m_invertTimer -= kInvertTickSeconds;
                --m_invertLevel;
            }
            if (m_invertLevel <= 0)
            {
                m_invert = false;
                m_invertTimer = 0.0f;
                m_invertJustExpired = true;
            }
        }

        // Helicopter/Overcraft (plan.md E3D-MIG-171): free vertical flight
        // instead of constant gravity -- lookUpHeld (real "Up" input)
        // ascends, crouchHeld (real "Down") descends, ramped via
        // kVehicleVerticalAccel rather than snapping instantly, matching
        // the real "accel 0.5" for both flying modes. Repurposes the same
        // two inputs that mean camera pitch on foot, the same "same input,
        // different meaning per mode" pattern already used for tempPassable
        // and Nage's own swim-up jump. Jeep/Tank/Skateboard are NOT flight
        // modes -- they fall through to the normal gravity path below,
        // matching the real source's own "uses the shared ground gravity/
        // Air path" note for Skateboard (and this session's decision not
        // to model Jeep/Tank's own real airborne-heavy-fall nuance).
        if (m_vehicleMode == VehicleMode::Helicopter || m_vehicleMode == VehicleMode::Overcraft)
        {
            const bool isHelicopter = (m_vehicleMode == VehicleMode::Helicopter);
            const float ascendSpeed = isHelicopter ? kHelicopterAscendSpeed : kOvercraftAscendSpeed;
            const float descendSpeed = isHelicopter ? kHelicopterDescendSpeed : kOvercraftDescendSpeed;
            float targetVelocityY = 0.0f;
            if (lookUpHeld)
            {
                targetVelocityY = ascendSpeed;
            }
            else if (crouchHeld)
            {
                targetVelocityY = -descendSpeed;
            }
            if (m_velocityY < targetVelocityY)
            {
                m_velocityY = std::min(m_velocityY + kVehicleVerticalAccel * dt, targetVelocityY);
            }
            else if (m_velocityY > targetVelocityY)
            {
                m_velocityY = std::max(m_velocityY - kVehicleVerticalAccel * dt, targetVelocityY);
            }
        }
        else if (m_balloon)
        {
            // Wasp "balloon" status: Blupi RISES, he does not hold still --
            // a direct port of the real dedicated balloon-movement block at
            // Decor.cpp:4039-4058 (see kBalloonDuration's own comment for
            // the full real-source citation and the px/tick -> units/s
            // conversion behind these constants). Real "Up" is `m_blupiSpeedY
            // < 0.0` and real "Down" is `> 0.0`; this engine's own
            // Helicopter/Overcraft branch just above already established
            // lookUpHeld/crouchHeld as that same up/down pair, so the same
            // mapping is reused here rather than inventing a second one.
            const float targetRise = (lookUpHeld || jumpPressed) ? kBalloonRiseSpeedFast
                                     : crouchHeld                ? 0.0f
                                                                 : kBalloonRiseSpeed;
            if (m_velocityY < targetRise)
            {
                m_velocityY = std::min(m_velocityY + kBalloonRiseAccel * dt, targetRise);
            }
            else if (m_velocityY > targetRise)
            {
                // Real Down-held case only ever decelerates a rise back toward
                // 0 (`if (m_blupiVitesseY < 0.0) m_blupiVitesseY += 1.0;`) --
                // it never accelerates downward, which a 0.0f target already
                // reproduces exactly.
                m_velocityY = std::max(m_velocityY - kBalloonRiseAccel * dt, targetRise);
            }
        }
        else
        {
            // Nage (plan.md E3D-MIG-148): a slow floaty sink instead of a
            // free-fall drop while genuinely submerged (kNageGravityMultiplier's
            // own comment).
            const float effectiveGravity = m_nage ? kGravity * kNageGravityMultiplier : kGravity;
            m_velocityY = std::max(m_velocityY - effectiveGravity * dt, kFallLimit);
        }
        float newY = m_y + m_velocityY * dt;

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        // referenceY = m_y (pre-fall position): a real ceiling above where
        // Blupi already is stays irrelevant to "what does he land on while
        // falling" (see GroundHeightAt's own comment).
        const int groundY = GroundHeightAt(world, gx, gz, tempPassable, m_y);

        // Balloon rise ceiling stop (kBalloonHorizontalSpeed's own comment,
        // point 2): real source's general `TestPath()` swept collision
        // stops a rising Blupi against solid decor the same as walking into
        // a wall. Checked every ballooned frame regardless of current
        // vertical direction (not gated on `newY > m_y`) -- a hovering/
        // descending Down-held Blupi can't run further INTO a ceiling
        // above him, but the ground-check-contamination problem below
        // depends only on static proximity to that block, not on which way
        // he's currently moving.
        //
        // `ceilingInReach` (found true whenever CeilingHeightAt() sees
        // anything at all, not just once the clamp itself fires) must
        // suppress the ground-check below for the ENTIRE time a ceiling is
        // within its scan window, not merely the one frame the clamp
        // triggers: `groundY` was computed from the same referenceY=m_y,
        // and GroundHeightAt()'s own scan window grows with that same
        // value, so a few frames BEFORE m_y actually reaches the ceiling's
        // real bottom surface, GroundHeightAt() already finds that same
        // block first and misreports it as ground to land ON TOP of --
        // confirmed live (a debug run showed Blupi teleport from y~2.0
        // straight to y~4.0 well before ever reaching the real 2.5 ceiling
        // contact height, the ceiling block's own "land on top" height,
        // silently pre-empting this clamp on an earlier frame than
        // expected). Both scans share the exact same "is this block
        // reachable from floor(m_y)+1" trigger distance, so gating on
        // CeilingHeightAt()'s own result (not just the clamp) reliably
        // covers every frame the ground-check would otherwise be
        // contaminated by the same block.
        bool ceilingInReach = false;
        if (m_balloon)
        {
            const int ceilingY = CeilingHeightAt(world, gx, gz, m_y);
            if (ceilingY != kNoCeiling)
            {
                ceilingInReach = true;
                // The solid block's own bottom surface (mirrors
                // GroundHeightAt()'s "y+1 sits at the top surface" -- here
                // the ceiling block at grid Y has its bottom at Y-0.5).
                const float ceilingBottom = static_cast<float>(ceilingY) - 0.5f;
                if (newY >= ceilingBottom)
                {
                    newY = ceilingBottom;
                    m_velocityY = 0.0f;
                }
            }
        }

        if (ceilingInReach)
        {
            // Suspended just beneath (or still rising toward) the ceiling,
            // not standing on it.
            m_onGround = false;
        }
        // kNoGround (no solid block anywhere in this column) must never
        // clamp Blupi to a fake floor -- he keeps falling under gravity
        // indefinitely, same as walking off any other ledge with a real
        // drop, letting GalaxyEggbertCnaGame::Update()'s kFallDeathY check
        // eventually catch it (plan.md E3D-MIG-067).
        else if (groundY != kNoGround && newY <= static_cast<float>(groundY))
        {
            newY = static_cast<float>(groundY);
            m_velocityY = 0.0f;
            m_onGround = true;
        }
        else
        {
            m_onGround = false;
        }
        m_y = newY;

        UpdateAnim(moving, crouchHeld, lookUpHeld, dt, pushingCrate);
    }

    void GEBlupiController::UpdateAnim(bool moving, bool crouchHeld, bool lookUpHeld, float dt, bool pushingCrate)
    {
        // Real SOUND-017/030/031 one-shot cues (plan.md, added 2026-07-20):
        // ch7/ch21 fire once, real Config::ScaleTime(4)=4 ticks=0.2s after
        // entering Down/Up (Decor.cpp:3278-3287); ch20 fires once on the
        // real Down->Stop transition specifically (Decor.cpp:3628-3633,
        // gated there on m_blupiSpeedX==0 && m_blupiSpeedY==0 -- this
        // engine's own crouchHeld ternary already IS that exact edge: Down
        // only ever transitions to Stop, never March, while crouchHeld is
        // still literally held, so the real gate needs no separate
        // modeling here). Reset every call; the caller consumes each once
        // per frame, same "*ThisFrame()" convention used throughout
        // GEInteractionSystem.
        m_downEntrySoundFiredThisFrame = false;
        m_upEntrySoundFiredThisFrame = false;
        m_downReleaseSoundFiredThisFrame = false;

        // Vehicle-mode Stop/March selection (plan.md BLUPI-037/038/047/058/
        // 069/084/088/091/094, found 2026-07-18) -- each mode has its own
        // real icon pair; falls through to the base Stop/March for
        // VehicleMode::None (never actually reached below, since the
        // ternary chain only calls this from the IsInVehicle() branch, but
        // keeps the lambda total/safe). Skateboard additionally splits into
        // its own real JumpSkate/AirSkate airborne variants (ACTION_JUMPSKATE/
        // ACTION_AIRSKATE, table_blupi actions 40/41), wired 2026-07-19 --
        // the only vehicle mode with a distinct airborne icon pair, mirroring
        // the base Jump/Air ascending-vs-falling split below.
        const auto vehicleAnimState = [this, moving]() -> AnimState
        {
            switch (m_vehicleMode)
            {
                case VehicleMode::Helicopter: return moving ? AnimState::MarchHelico : AnimState::StopHelico;
                case VehicleMode::Jeep:       return moving ? AnimState::MarchJeep   : AnimState::StopJeep;
                case VehicleMode::Tank:       return moving ? AnimState::MarchTank   : AnimState::StopTank;
                case VehicleMode::Skateboard:
                    if (!m_onGround)
                    {
                        return m_velocityY > 0.0f ? AnimState::JumpSkate : AnimState::AirSkate;
                    }
                    return moving ? AnimState::MarchSkate : AnimState::StopSkate;
                case VehicleMode::Overcraft:  return moving ? AnimState::MarchOver   : AnimState::StopOver;
                default: return moving ? AnimState::March : AnimState::Stop;
            }
        };

        // Precedence: Teleporting/Balloon/Ecrase/Hide/Nage/Surf/vehicle
        // (each a real BlupiAction status with only ONE real animation
        // regardless of grounded/airborne, see the AnimState enum's own
        // comment) beat the normal ground/air cascade entirely, which
        // otherwise matches GalaxyEggbertSimple3D::GEBlupiController::
        // UpdateState: airborne beats crouch/look-up beats moving beats
        // idle. Airborne itself splits Jump (ascending) vs Air (falling/
        // apex) by velocity sign -- see the AnimState enum's own comment
        // for why this differs from Simple3D's frame-counted trigger
        // window.
        const AnimState newState = (m_deathLocked || m_deathLossVoyageActive) ? AnimState::DeathLocked
                                  : m_bye ? AnimState::Bye
                                  : m_oneShotAnimActive ? m_oneShotAnimState
                                  : m_pickupFrozen ? AnimState::PickupBusy
                                  : m_teleporting ? AnimState::Teleporting
                                  : m_balloon     ? AnimState::Balloon
                                  : m_ecrase      ? (moving ? AnimState::MarchEcrase : AnimState::StopEcrase)
                                  : m_secretPower == SecretPower::Hide ? AnimState::Hide
                                  : m_nage        ? (moving ? AnimState::MarchNage : AnimState::StopNage)
                                  : m_surf        ? (moving ? AnimState::MarchSurf : AnimState::StopSurf)
                                  : IsInVehicle() ? vehicleAnimState()
                                  : pushingCrate  ? AnimState::Push
                                  : m_mockeryActive ? m_mockeryVariant
                                  : !m_onGround   ? (m_velocityY > 0.0f ? AnimState::Jump : AnimState::Air)
                                  : crouchHeld    ? AnimState::Down
                                  : lookUpHeld    ? AnimState::Up
                                  : moving        ? AnimState::March
                                                  : AnimState::Stop;
        if (newState != m_animState)
        {
            if (m_animState == AnimState::Down && newState == AnimState::Stop)
            {
                m_downReleaseSoundFiredThisFrame = true;
            }
            m_animState = newState;
            m_animPhase = 0;
            m_animTimer = 0.0f;
            m_animStateTimer = 0.0f;
            return;
        }

        m_animTimer += dt;
        const float frameDuration = 1.0f / kAnimFps;
        if (m_animTimer >= frameDuration)
        {
            m_animTimer -= frameDuration;
            ++m_animPhase;
        }

        const float previousStateTimer = m_animStateTimer;
        m_animStateTimer += dt;
        if (previousStateTimer < kDownUpSoundDelay && m_animStateTimer >= kDownUpSoundDelay)
        {
            if (m_animState == AnimState::Down)
            {
                m_downEntrySoundFiredThisFrame = true;
            }
            else if (m_animState == AnimState::Up)
            {
                m_upEntrySoundFiredThisFrame = true;
            }
        }
    }

    int GEBlupiController::GetAnimIcon() const noexcept
    {
        switch (m_animState)
        {
            case AnimState::March:
                return kMarchFrames[m_animPhase % (sizeof(kMarchFrames) / sizeof(kMarchFrames[0]))];
            case AnimState::Jump:
                return kJumpFrames[m_animPhase % (sizeof(kJumpFrames) / sizeof(kJumpFrames[0]))];
            case AnimState::Air:
                return kAirFrames[m_animPhase % (sizeof(kAirFrames) / sizeof(kAirFrames[0]))];
            case AnimState::Down:
                return kDownFrames[m_animPhase % (sizeof(kDownFrames) / sizeof(kDownFrames[0]))];
            case AnimState::Up:
                return kUpFrames[m_animPhase % (sizeof(kUpFrames) / sizeof(kUpFrames[0]))];
            case AnimState::StopEcrase:
                return kStopEcraseFrames[m_animPhase % (sizeof(kStopEcraseFrames) / sizeof(kStopEcraseFrames[0]))];
            case AnimState::MarchEcrase:
                return kMarchEcraseFrames[m_animPhase % (sizeof(kMarchEcraseFrames) / sizeof(kMarchEcraseFrames[0]))];
            case AnimState::Balloon:
                return kBalloonFrames[m_animPhase % (sizeof(kBalloonFrames) / sizeof(kBalloonFrames[0]))];
            case AnimState::Teleporting:
            {
                const int icon = kTeleportingFrames[m_animPhase % (sizeof(kTeleportingFrames) / sizeof(kTeleportingFrames[0]))];
                return icon >= 0 ? icon : kStopFrames[0]; // -1 = real invisible frame, see kTeleportingFrames' own comment
            }
            case AnimState::DeathLocked:
            {
                // Real per-cause hurt-sprite frame table, added 2026-07-16 (see kClear1Frames'
                // own comment for the transcription method/approval).
                int icon;
                switch (m_deathCause)
                {
                    case DeathCause::Clear1:
                        icon = kClear1Frames[m_animPhase % (sizeof(kClear1Frames) / sizeof(kClear1Frames[0]))];
                        break;
                    case DeathCause::Clear2:
                        icon = kClear2Frames[m_animPhase % (sizeof(kClear2Frames) / sizeof(kClear2Frames[0]))];
                        break;
                    case DeathCause::Clear3:
                        icon = kClear3Frames[m_animPhase % (sizeof(kClear3Frames) / sizeof(kClear3Frames[0]))];
                        break;
                    case DeathCause::Clear4:
                        icon = kClear4Frames[m_animPhase % (sizeof(kClear4Frames) / sizeof(kClear4Frames[0]))];
                        break;
                    case DeathCause::Glu:
                        icon = kGluFrames[m_animPhase % (sizeof(kGluFrames) / sizeof(kGluFrames[0]))];
                        break;
                    case DeathCause::Drown:
                    default:
                        icon = kDrownFrames[m_animPhase % (sizeof(kDrownFrames) / sizeof(kDrownFrames[0]))];
                        break;
                }
                return icon >= 0 ? icon : kStopFrames[0]; // -1 = real invisible frame (Clear2/Clear3's tail)
            }
            case AnimState::PickupBusy:
            {
                // Real per-kind pickup-freeze busy-animation frames, added 2026-07-16 (same
                // transcription as DeathLocked above).
                switch (m_pickupFreezeKind)
                {
                    case PickupFreezeKind::Sucette:
                        return kSucetteFrames[m_animPhase % (sizeof(kSucetteFrames) / sizeof(kSucetteFrames[0]))];
                    case PickupFreezeKind::Drink:
                        return kDrinkFrames[m_animPhase % (sizeof(kDrinkFrames) / sizeof(kDrinkFrames[0]))];
                    case PickupFreezeKind::Charge:
                    default:
                        return kChargeFrames[m_animPhase % (sizeof(kChargeFrames) / sizeof(kChargeFrames[0]))];
                }
            }
            case AnimState::StopHelico:
                return kStopHelicoFrames[m_animPhase % (sizeof(kStopHelicoFrames) / sizeof(kStopHelicoFrames[0]))];
            case AnimState::MarchHelico:
                return kMarchHelicoFrames[m_animPhase % (sizeof(kMarchHelicoFrames) / sizeof(kMarchHelicoFrames[0]))];
            case AnimState::StopJeep:
                return kStopJeepFrames[m_animPhase % (sizeof(kStopJeepFrames) / sizeof(kStopJeepFrames[0]))];
            case AnimState::MarchJeep:
                return kMarchJeepFrames[m_animPhase % (sizeof(kMarchJeepFrames) / sizeof(kMarchJeepFrames[0]))];
            case AnimState::StopTank:
                return kStopTankFrames[m_animPhase % (sizeof(kStopTankFrames) / sizeof(kStopTankFrames[0]))];
            case AnimState::MarchTank:
                return kMarchTankFrames[m_animPhase % (sizeof(kMarchTankFrames) / sizeof(kMarchTankFrames[0]))];
            case AnimState::StopSkate:
                return kStopSkateFrames[m_animPhase % (sizeof(kStopSkateFrames) / sizeof(kStopSkateFrames[0]))];
            case AnimState::MarchSkate:
                return kMarchSkateFrames[m_animPhase % (sizeof(kMarchSkateFrames) / sizeof(kMarchSkateFrames[0]))];
            case AnimState::JumpSkate:
                return kJumpSkateFrames[m_animPhase % (sizeof(kJumpSkateFrames) / sizeof(kJumpSkateFrames[0]))];
            case AnimState::AirSkate:
                return kAirSkateFrames[m_animPhase % (sizeof(kAirSkateFrames) / sizeof(kAirSkateFrames[0]))];
            case AnimState::StopOver:
                return kStopOverFrames[m_animPhase % (sizeof(kStopOverFrames) / sizeof(kStopOverFrames[0]))];
            case AnimState::MarchOver:
                return kMarchOverFrames[m_animPhase % (sizeof(kMarchOverFrames) / sizeof(kMarchOverFrames[0]))];
            case AnimState::StopNage:
                return kStopNageFrames[m_animPhase % (sizeof(kStopNageFrames) / sizeof(kStopNageFrames[0]))];
            case AnimState::MarchNage:
                return kMarchNageFrames[m_animPhase % (sizeof(kMarchNageFrames) / sizeof(kMarchNageFrames[0]))];
            case AnimState::StopSurf:
                return kStopSurfFrames[m_animPhase % (sizeof(kStopSurfFrames) / sizeof(kStopSurfFrames[0]))];
            case AnimState::MarchSurf:
                return kMarchSurfFrames[m_animPhase % (sizeof(kMarchSurfFrames) / sizeof(kMarchSurfFrames[0]))];
            case AnimState::Hide:
                return kHideFrames[m_animPhase % (sizeof(kHideFrames) / sizeof(kHideFrames[0]))];
            case AnimState::Push:
                return kPushFrames[m_animPhase % (sizeof(kPushFrames) / sizeof(kPushFrames[0]))];
            case AnimState::Switch:
                return kSwitchFrames[m_animPhase % (sizeof(kSwitchFrames) / sizeof(kSwitchFrames[0]))];
            case AnimState::FireTank:
                return kFireTankFrames[m_animPhase % (sizeof(kFireTankFrames) / sizeof(kFireTankFrames[0]))];
            case AnimState::TakeDynamite:
                return kTakeDynamiteFrames[m_animPhase % (sizeof(kTakeDynamiteFrames) / sizeof(kTakeDynamiteFrames[0]))];
            case AnimState::PutDynamite:
                return kPutDynamiteFrames[m_animPhase % (sizeof(kPutDynamiteFrames) / sizeof(kPutDynamiteFrames[0]))];
            case AnimState::TakeSkate:
                return kTakeSkateFrames[m_animPhase % (sizeof(kTakeSkateFrames) / sizeof(kTakeSkateFrames[0]))];
            case AnimState::DeposeSkate:
                return kDeposeSkateFrames[m_animPhase % (sizeof(kDeposeSkateFrames) / sizeof(kDeposeSkateFrames[0]))];
            case AnimState::Mockery:
                return kMockeryFrames[m_animPhase % (sizeof(kMockeryFrames) / sizeof(kMockeryFrames[0]))];
            case AnimState::Mockeryi:
                return kMockeryiFrames[m_animPhase % (sizeof(kMockeryiFrames) / sizeof(kMockeryiFrames[0]))];
            case AnimState::Mockeryp:
                return kMockerypFrames[m_animPhase % (sizeof(kMockerypFrames) / sizeof(kMockerypFrames[0]))];
            case AnimState::Stop:
            default:
                return kStopFrames[m_animPhase % (sizeof(kStopFrames) / sizeof(kStopFrames[0]))];
        }
    }

    bool GEBlupiController::AnimIconUsesElementSheet() const noexcept
    {
        // Real BlupiSearchIcon() channel rule (mobile-eggbert-reference/08-animations.md §2's own
        // "Channel selection" note, verified directly against Decor.cpp): only Clear1/Clear2/
        // Clear3/Glu/Electro use element.png -- everything else, including DeathLocked's own
        // Clear4/Drown causes, uses blupi.png. Electro has no modeled mechanic in this engine yet
        // (see AnimState's own class comment), so it never reaches m_animState here.
        if (m_animState != AnimState::DeathLocked)
        {
            return false;
        }
        return m_deathCause == DeathCause::Clear1 || m_deathCause == DeathCause::Clear2 ||
               m_deathCause == DeathCause::Clear3 || m_deathCause == DeathCause::Glu;
    }
}
