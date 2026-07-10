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
    // Shared kill-list contact (2026-07-11, plan.md E3D-MIG-132, widened
    // 2026-07-11) now works for ObjectType 2/3/4/16/17/20/96/97 -- touching
    // any of them kills Blupi and destroys it. Verified directly against
    // the real Decor.cpp:5782-5816 source: that one code block IS the real
    // shared contact check for exactly this set of 8 types (2=patrol
    // hazard, 3=patrol hazard, 4=bulldozer, 16=spider, 17=fish, 20=bird,
    // 96/97=follower dormant/awake) -- the only difference the real source
    // makes between them is purely cosmetic (17/20 get a bigger screen-
    // shake + a different explosion ObjectType), not a behavioral
    // difference in whether/how Blupi dies, so IsGenericHazard() covers all
    // 8 with one check rather than splitting them into separate per-type
    // branches that would all do the same thing. Real death sound is a
    // 50/50 coinflip between channel 74 and silence -- BlupiDead(Clear1,
    // Clear2)'s own Clear2 branch plays channel 74, Clear1 plays nothing --
    // simplified here to always channel 74, a documented approximation of
    // the coinflip rather than an invented value. Type3's real duck-
    // immunity (skipped while Blupi's action is Down) IS modeled via the
    // blupiCrouching parameter below; every other per-type quirk (type2's
    // wider "thrown object" anticipation box, type17/20's bigger
    // explosion, follower 96/97's real homing-toward-Blupi movement AI) is
    // NOT modeled -- cosmetic/reaction polish or a genuinely separate
    // feature (homing), not required for the kill itself.
    //
    // NOT yet implemented (deliberately, not an oversight):
    //  - Every enemy type OUTSIDE the shared kill list above (32/33/44/54)
    //    -- each has real per-type attack/contact rules of its own (Phase
    //    13: blupih/blupit fire projectiles, wasp inflates a status instead
    //    of killing, the large creature has its own lethality window), not
    //    a plain kill-on-touch, so none of them belong in IsGenericHazard().
    //  - Follower 96/97's real homing-toward-Blupi movement (Phase 13) --
    //    they're currently just static/patrol MoveObjects like any other;
    //    only their contact-death is covered here.
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
        // blupiCrouching gates type3's real duck-immunity, default false so
        // existing callers/tests that don't care about it are unaffected.
        // If this call kills Blupi via enemy contact, DiedThisFrame()
        // returns true for the rest of this frame only -- GEInteractionSystem
        // has no access to GEBlupiController, so the caller is the one that
        // must actually respawn Blupi (see GalaxyEggbertCnaGame::Update()).
        void Update(float dt, GEWorldRuntime& worldRuntime,
                    float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                    GESound& sound, bool blupiCrouching = false);

        [[nodiscard]] bool DiedThisFrame() const noexcept { return diedThisFrame_; }

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
        bool diedThisFrame_ = false; // reset at the top of every Update() call
    };
}
