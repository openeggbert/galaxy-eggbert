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
    // A basic lives foundation now exists (2026-07-11, LoseLife()/Lives()
    // below, plan.md E3D-MIG-130) -- just the counter and its real
    // reset-to-3-on-zero behavior, wired so far only to the one
    // always-active hazard that needed no per-tile-type work at all
    // (falling off the world, see GalaxyEggbertCnaGame::Update()). This
    // does NOT yet unblock full hazard/enemy contact below -- those still
    // need their own per-type behavior (Phase 14/13), not just a lives
    // counter to decrement.
    //
    // NOT yet implemented (deliberately, not an oversight):
    //  - Enemy hit/stomp/hazard (ObjectType2/3/16/17/20/32/33/44/54/96) --
    //    real per-type behavior (patrol/attack/contact rules) doesn't exist
    //    yet, not just the lives system it would have needed (that part is
    //    now done, see above).
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

        // Real mobile-eggbert m_nbVies (lives) tracking (2026-07-11, plan.md
        // E3D-MIG-130): starts at 3 (GameData default,
        // mobile-eggbert-reference/11-save-and-progression.md), incremented
        // by egg pickups up to MAX_EGG_COUNT=10 (see the ObjectType6 case in
        // Update() -- lifeEggCount_ above is both "eggs collected so far"
        // and the gate counter, same single value mobile-eggbert itself
        // uses). Call LoseLife() on any death; a caller decides which sound
        // channel fits the specific death cause (channel 8 generic/fall/
        // lava/electric, 26 drowning, 51 glue per
        // mobile-eggbert-reference/07-sounds.md) and plays it itself --
        // LoseLife() only manages the counter.
        void LoseLife();
        [[nodiscard]] int Lives() const noexcept { return lives_; }
        // Diagnostic only: real DoorsLost() (Decor.cpp:11716) resets
        // m_nbVies back to 3 on game-over rather than a permanent
        // depletion -- no real Lost-screen UI exists yet (plan.md
        // MENU-053..057) to make a game-over user-visible, so this counter
        // is how a caller/verification tool can observe it happened.
        [[nodiscard]] int GameOverCount() const noexcept { return gameOverCount_; }

    private:
        int treasuresCollected_ = 0;
        int totalTreasures_ = -1; // computed lazily on first Update() call
        bool exitReached_ = false;
        bool exitContactActive_ = false; // debounces the exit-touch sound/check to once per contact
        int keys1_ = 0;
        int keys2_ = 0;
        int keys3_ = 0;
        // Eggs collected so far -- also the MAX_EGG_COUNT=10 gate counter
        // (mobile-eggbert uses a single m_nbVies-driven value for both;
        // kept separate here only so lives_ can start at its own real
        // default of 3 while this stays a pure 0..10 egg tally).
        int lifeEggCount_ = 0;
        int lives_ = 3; // real GameData default (11-save-and-progression.md)
        int gameOverCount_ = 0;
    };
}
