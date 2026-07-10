#pragma once

#include "GESound.hpp"
#include "GEWorldRuntime.hpp"

namespace GalaxyEggbert::CNA
{
    // Real mobile-eggbert interactive-object behavior (2026-07-10): platform
    // lift patrol movement, crate push, and pickup collection (treasure/egg/
    // keys/level-exit) -- the first slice of interactive-object support.
    // Behavior (not code) ported from two sources: GalaxyEggbertSimple3D's
    // already-shipped GEDecorSystem.cpp (patrol ping-pong movement, and crate
    // push specifically -- its own comment cites the real mobile-eggbert
    // function name TestPushCaisse) for the parts it gets right, and the
    // more carefully-researched `mobile-eggbert-reference/13-object-
    // pickups.md`/`07-sounds.md` behavioral spec for exact pickup semantics
    // (which sound channel, whether the object is actually removed, egg's
    // MAX_EGG_COUNT=10 cap, exit's treasure-gate) -- the spec is the more
    // authoritative source where the two disagree (see GEInteractionSystem.cpp
    // for the one confirmed case: Simple3D never removes a collected
    // treasure/exit object, but the real source deletes every pickup
    // immediately on contact per `13-object-pickups.md`'s "Voyage" section).
    //
    // CNA has no entity/scene-graph framework to port Simple3D's code INTO,
    // so this is a fresh, minimal implementation operating directly on
    // GEWorldRuntime's MobileObjSpec list and Blupi's live position.
    //
    // NOT yet implemented (deliberately, not an oversight):
    //  - Enemy hit/stomp/hazard (ObjectType2/3/16/17/20/32/33/44/54/96) --
    //    mobile-eggbert's real hazard/stomp behavior only means something
    //    once a lives/gauge/respawn system exists, and none does yet in
    //    GalaxyEggbertCNA; a stomp animation with no lives to lose would be
    //    inventing a hollow, partial version of the mechanic.
    //  - Riding a moving platform lift (ObjectType1/47/48) -- platforms now
    //    genuinely patrol (see Update()), but GEBlupiController's collision
    //    only tests the static terrain grid, not MobileObjSpec objects, so
    //    Blupi cannot yet stand on one. Needs its own dedicated collision-
    //    system work, not attempted here.
    //  - Every other pickup type in IsPickup() (helicopter, shield, drink,
    //    mirror/invert, vehicles, dynamite, etc.) -- deferred to a follow-up;
    //    only the types placed in today's sample world (treasure/egg/keys/
    //    exit) are wired up.
    class GEInteractionSystem
    {
    public:
        // Advances patrol movement + crate push, and detects/collects
        // pickups Blupi is standing on. Call once per frame, after
        // blupi.Step() (so blupiX/Y/Z are this frame's final position) --
        // blupiMoveDX is this frame's X position delta (blupiX this frame
        // minus blupiX last frame), used as a stand-in for Simple3D's
        // blupiVelX (GEBlupiController exposes no velocity accessor; the
        // caller computes this itself from before/after Step() positions).
        void Update(float dt, GEWorldRuntime& worldRuntime,
                    float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                    GESound& sound);

        [[nodiscard]] int TreasuresCollected() const noexcept { return treasuresCollected_; }
        [[nodiscard]] int TotalTreasures() const noexcept { return totalTreasures_ < 0 ? 0 : totalTreasures_; }
        [[nodiscard]] bool ExitReached() const noexcept { return exitReached_; }
        [[nodiscard]] int Key1Count() const noexcept { return keys1_; }
        [[nodiscard]] int Key2Count() const noexcept { return keys2_; }
        [[nodiscard]] int Key3Count() const noexcept { return keys3_; }
        [[nodiscard]] int LifeEggCount() const noexcept { return lifeEggCount_; }

    private:
        int treasuresCollected_ = 0;
        int totalTreasures_ = -1; // computed lazily on first Update() call
        bool exitReached_ = false;
        bool exitContactActive_ = false; // debounces the exit-touch sound/check to once per contact
        int keys1_ = 0;
        int keys2_ = 0;
        int keys3_ = 0;
        // Mirrors mobile-eggbert's m_nbVies for the one thing that matters
        // here (egg pickup's MAX_EGG_COUNT=10 gate) -- not a real lives/
        // gauge system (no game-over, no HUD), just the counter that gate
        // needs to be faithful.
        int lifeEggCount_ = 0;
    };
}
