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
    // Wasp (ObjectType44) "balloon" status now works (2026-07-11, plan.md
    // E3D-MIG-135), verified directly against Decor.cpp:5826-5863 (trigger)
    // and 5766-5781 (the balloon-pop interaction with hazards). Contact
    // does NOT kill Blupi or destroy the wasp -- it signals
    // BalloonTouchedThisFrame() every frame Blupi overlaps it; the real
    // `!m_blupiBalloon` re-trigger guard lives in
    // GEBlupiController::TriggerBalloon() itself (idempotent, same pattern
    // as TriggerCrush()), not here, since GEInteractionSystem has no access
    // to Blupi's current balloon state. While ballooned (the caller passes
    // this in via blupiBallooned), touching one of exactly 4 of the 8
    // shared-kill-list types (3/16/96/97 -- NOT 2/4/17/20, confirmed by the
    // real source's if/else-if chain: the pop check for 3/16/96/97 comes
    // FIRST and is mutually exclusive with the kill check right after it,
    // but 2/4/17/20 only ever reach the kill check) pops the balloon
    // instead of killing -- signaled via BalloonPoppedThisFrame(), same
    // caller-applies-the-actual-state-change split as DiedThisFrame().
    //
    // Blupih/blupit (ObjectType32/33) stationary shooters now work
    // (2026-07-10, plan.md E3D-MIG-134), verified directly against
    // Decor.cpp:8878-8969 (attack timing) and 7794-7869 (the real
    // ObjectStart raycast/travel-distance encoding). Their own body is
    // NOT a damage path (not in IsGenericHazard()) -- only their fired
    // ObjectType23 projectile is, spawned during a turn-dwell (patrolStep
    // 1 or 3) at the real dwell-frame(s): blupih drops one straight down
    // at frame 21, blupit fires two horizontal shots bracketing the turn
    // (frame 3 away from the upcoming walk direction, frame 21 toward it
    // -- see FireBlupitShot's comment for a correction against this
    // class's own reference-doc summary, which had the two frames
    // backwards). The projectile's travel distance is a real grid
    // raycast (SearchAirDistance in the .cpp) to the next solid cell,
    // NOT aimed at Blupi (the real source never aims either). Contact
    // with the projectile is always fatal, same simplification (no
    // shield/hide/superblupi gating, none of those exist yet) as every
    // other hazard here.
    //
    // Large creature (ObjectType54) now works too (2026-07-11, plan.md
    // E3D-MIG-136), verified directly against Decor.cpp:5867-5913. Contact
    // is lethal ONLY while it is paused mid-turn (patrolStep 1 or 3, the
    // real `step != 2 && step != 4` gate) -- safe to touch while it's
    // actually walking. It is never destroyed by the contact (no real
    // ObjectDelete in that branch). Real balloon immunity IS modeled
    // (`blupiBallooned` blocks the whole branch, matching the real
    // `!m_blupiBalloon` gate) -- unlike the 4 balloon-poppable hazard
    // types, there is no separate pop path for it. Real shield/hide/
    // superBlupi/focus immunity and the real "destroys Blupi's current
    // vehicle instead of killing him" branch are NOT modeled (no such
    // concepts exist in this engine yet), so contact always takes the
    // real no-vehicle death branch. The real unconditional taunt icon is
    // also NOT modeled -- no idle-taunt animation system exists at all.
    //
    // Follower (ObjectType96/97) real homing now works too (2026-07-11,
    // plan.md E3D-MIG-137, the last open item in Phase 13), verified
    // directly against Decor.cpp:9646-9678 (the wake box) and 8025-8064
    // (the homing step). A dormant 96 wakes into the homing 97 once Blupi
    // is within its padded detection box (approximated as a circular
    // distance check, real channel 92 wake sound); once awake it steps X
    // and Y independently toward Blupi's live position at a real 1px/tick
    // (Z is left untouched -- no such axis exists in the real 2D source),
    // and self-destructs (real channel 10, no debris object spawned -- no
    // decorative-effect system exists yet) if its next step would land in
    // a solid cell, rather than continuing to home. Contact-kill/pop was
    // already covered by the shared kill list before this.
    //
    // Riding a moving platform lift (ObjectType1/47/48) now works too
    // (2026-07-12, plan.md E3D-MIG-152) -- see IsRidingLift()/RideDeltaX/Z()/
    // RideStandY()'s own comment. GEBlupiController itself still has no
    // knowledge of MobileObjSpec objects (same engine-agnostic split as
    // every other terrain/object fact it doesn't compute itself) -- the
    // caller (GalaxyEggbertCnaGame::Update()) reads these back and calls
    // GEBlupiController::RideLift() with them. Real fast-fall tunnelling
    // prevention (the 30px swept multi-step probe) is NOT modeled -- this
    // engine's own gravity/dt doesn't cross a lift's height in one frame
    // under normal conditions. Types 47/48's real conveyor nudge is folded
    // into RideDeltaX() directly (kConveyorNudgeSpeed is an approximation,
    // the real 2px/tick has no exact unit-conversion established here).
    //
    // NOT yet implemented (deliberately, not an oversight):
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
        // blupiBallooned (default false, same reason) gates whether
        // touching a 3/16/96/97 hazard pops the balloon instead of
        // killing, AND whether the large creature (54)'s turn-dwell
        // contact is lethal at all (real `!m_blupiBalloon` gates that
        // whole branch, no pop path for it) -- the caller reads this back
        // from GEBlupiController::IsBallooned() before calling Update(),
        // same as blupiCrouching's own GetAnimState()-derived source. If this
        // call kills Blupi via enemy contact, DiedThisFrame() returns true
        // for the rest of this frame only -- GEInteractionSystem has no
        // access to GEBlupiController, so the caller is the one that must
        // actually respawn Blupi (see GalaxyEggbertCnaGame::Update()).
        // blupiFacingDX/DZ (plan.md E3D-MIG-160, both default 0 so existing
        // callers/tests are unaffected) are the rounded cardinal-direction
        // grid offset one cell in front of Blupi's current facing (the
        // caller derives this from GEBlupiController::GetYaw()) -- doors
        // (real `Decor::IsDoor`) probe BOTH Blupi's own cell and this one,
        // so he can trigger a door a step before actually reaching it,
        // matching the real source exactly.
        // blupiInvincible (plan.md E3D-MIG-170, both default false so
        // existing callers/tests are unaffected) is the caller's own
        // `GEBlupiController::IsInvincible()` (Shield or Hide active) --
        // gates every hazard/enemy death check inside this class (real
        // `!m_blupiShield && !m_blupiHide`, confirmed identical across
        // essentially every hazard in Decor.cpp). blupiCanGrantShield/
        // Power/Cloud/Hide are the caller's own per-power gate state
        // (`GEBlupiController::GetSecretPower()`, translated to 4 bools
        // here rather than exposing the enum, since this class doesn't
        // otherwise depend on GEBlupiController's types) used to decide
        // whether touching a secret-power pickup (ObjectType25/26/30/31)
        // actually grants it -- see the *GrantedThisFrame() signals below.
        void Update(float dt, GEWorldRuntime& worldRuntime,
                    float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                    GESound& sound, bool blupiCrouching = false, bool blupiBallooned = false,
                    int blupiFacingDX = 0, int blupiFacingDZ = 0, bool blupiInvincible = false,
                    bool blupiCanGrantShield = true, bool blupiCanGrantPower = true,
                    bool blupiCanGrantCloud = true, bool blupiCanGrantHide = true);

        [[nodiscard]] bool DiedThisFrame() const noexcept { return diedThisFrame_; }
        // Wasp contact (see the class comment above) -- true every frame
        // Blupi overlaps a wasp, NOT debounced to once per contact (the
        // real `!m_blupiBalloon` re-trigger guard lives in
        // GEBlupiController::TriggerBalloon() instead).
        [[nodiscard]] bool BalloonTouchedThisFrame() const noexcept { return balloonTouchedThisFrame_; }
        // True the one frame a 3/16/96/97 hazard was popped instead of
        // killing (see the class comment above) -- the caller must call
        // GEBlupiController::PopBalloon() itself.
        [[nodiscard]] bool BalloonPoppedThisFrame() const noexcept { return balloonPoppedThisFrame_; }

        // Secret power pickups (plan.md E3D-MIG-170, ObjectType25/26/30/31)
        // -- true the one frame that pickup's real gate passed and the
        // world object was actually removed; the caller then calls the
        // matching `GEBlupiController::TriggerX()` (its own internal gate
        // should agree, since both check the same state) and plays the
        // real grant sound (channels 42/44/55/62) only if that returns
        // true. Real 2-stage delay/animation before Power/Cloud/Hide
        // actually activate is NOT modeled (see GEBlupiController::
        // TriggerPower()'s own comment) -- all 4 grant on contact here.
        [[nodiscard]] bool ShieldGrantedThisFrame() const noexcept { return shieldGrantedThisFrame_; }
        [[nodiscard]] bool PowerGrantedThisFrame() const noexcept { return powerGrantedThisFrame_; }
        [[nodiscard]] bool CloudGrantedThisFrame() const noexcept { return cloudGrantedThisFrame_; }
        [[nodiscard]] bool HideGrantedThisFrame() const noexcept { return hideGrantedThisFrame_; }

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
        // Restores a saved lives count (2026-07-13, plan.md MENU-040..045,
        // real Resume phase) -- ResumeContinue's own real equivalent, via
        // GESaveData, since this engine has no `GameData`-style per-gamer
        // slot to read lives from directly.
        void SetLives(int lives) noexcept { lives_ = lives; }
        // Diagnostic only: real DoorsLost() (Decor.cpp:11716) resets
        // m_nbVies back to 3 on game-over rather than a permanent
        // depletion -- this counter is how a caller/verification tool can
        // observe it happened (also now drives the real Lost screen,
        // plan.md MENU-053..057, and the Win/Lost save checkpoint,
        // MENU-040..045).
        [[nodiscard]] int GameOverCount() const noexcept { return gameOverCount_; }

        // Dynamite (plan.md E3D-MIG-155, ObjectType55 pickup / ObjectType56
        // fuse, verified directly against Decor.cpp ~6116-6129 (pickup),
        // ~4792-4812 (placement gate), ~8252-8296 (fuse timing), ~9058
        // (per-blast effect)). Real `m_blupiDynamite` caps at exactly 1 --
        // picking up a second does nothing until the first is placed (see
        // the ObjectType55 case in Update()). PlaceDynamite() is the action-
        // button placement: a no-op (returns false) unless carrying one and
        // `grounded` (this engine's stand-in for the real "solid ground
        // under both feet" check) -- spawns a real ObjectType56 fuse object
        // at the given position and decrements the count. The fuse's own
        // 100-tick animation (`table_dynamitef`) is NOT ported (needs a
        // fresh data transcription this session didn't get explicit
        // approval for) -- it renders via whatever GEObjectIcons::GetObjIcon()
        // already returns for type 56, a documented simplification. The 9
        // real blast ticks/offsets (Decor.cpp ~8258-8290, NOT just the
        // reference doc's rounded summary) and the real destructible-type
        // list (~9102-9132) are handled inside Update() itself, not exposed
        // here.
        [[nodiscard]] int DynamiteCount() const noexcept { return dynamiteCount_; }
        bool PlaceDynamite(GEWorldRuntime& worldRuntime, float x, float y, float z, bool grounded);

        // Perso (decoy statue) placement/pickup (plan.md HUD-017, real
        // Decor.cpp ~4818-4841 place, ~6088-6101 pickup start, ~10291-10294
        // pickup completion). A placeable `ObjectType200` decoy (same
        // `blupi.png` look as Blupi himself) -- what it actually DOES once
        // placed (if anything beyond existing as a marker) wasn't found in
        // the real source by this port's own research; not modeled as a
        // gameplay effect, just the place/retrieve/count loop itself. Real
        // cap 5 (`m_blupiPerso < 5` gates pickup), starts at 0. Real gate:
        // mutually exclusive with dynamite (an `else if` in the real
        // source -- the caller is expected to only call this when
        // `PlaceDynamite()` above didn't act, matching that precedence).
        // A single call handles BOTH real branches (pickup takes priority
        // when already standing near a placed decoy, matching the real
        // source's own `MoveObjectDetect` check before attempting a new
        // placement): returns true if either a pickup or a placement
        // happened. The real voyage-flight-animation before the pickup
        // counter actually increments is NOT modeled -- increments
        // immediately, same simplification as every other pickup this
        // session.
        [[nodiscard]] int PersoCount() const noexcept { return persoCount_; }
        bool TryPerso(GEWorldRuntime& worldRuntime, float x, float y, float z, bool grounded);

        // Bullet pack (ObjectType29, plan.md E3D-MIG-175, real Decor.cpp
        // ~5731-5744). Automatic on contact, no button, gated on
        // `bulletCount_ < kBulletCap` (real m_blupiBullet < 10) -- picking
        // one up at the cap does nothing (object stays in the world,
        // matching the real source exactly). Tops up to exactly
        // kBulletCap, not a running total (`+= 10` then clamped in the real
        // source has the same net effect here since the gate already
        // guarantees the prior count was below the cap). The actual
        // firing/ammo-consumption mechanic (Helicopter/Tank vehicle fire
        // button, real ObjectType23 projectile spawn) is NOT modeled --
        // deferred as its own follow-up, out of this pickup's scope.
        [[nodiscard]] int BulletCount() const noexcept { return bulletCount_; }

        // Platform lift riding (plan.md E3D-MIG-152, real `Decor::
        // MoveObjectStepLine`'s per-tick overlap re-test, unified with the
        // real separate `AscenseurDetect` initial-catch check into one
        // per-frame test -- see GEInteractionSystem.cpp's own comment).
        // True this frame if Blupi (at the blupiX/Y/Z passed into Update())
        // was standing on an active platform lift BEFORE it took this
        // frame's patrol step. RideDeltaX/Z is the lift's own horizontal
        // displacement this tick (+ the real constant conveyor nudge for
        // types 47/48, plan.md E3D-MIG-154) -- a delta, not an absolute
        // position, so the caller applies it on top of Blupi's own
        // already-Step()'d position rather than overriding his own
        // movement input. RideStandY is an absolute snap (matching the
        // real source's own "correct Y drift every frame" approach): the
        // caller should pass both into GEBlupiController::RideLift().
        [[nodiscard]] bool IsRidingLift() const noexcept { return ridingLift_; }
        [[nodiscard]] float RideDeltaX() const noexcept { return rideDeltaX_; }
        [[nodiscard]] float RideDeltaZ() const noexcept { return rideDeltaZ_; }
        [[nodiscard]] float RideStandY() const noexcept { return rideStandY_; }

    private:
        bool ridingLift_ = false;
        float rideDeltaX_ = 0.0f;
        float rideDeltaZ_ = 0.0f;
        float rideStandY_ = 0.0f;

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
        int dynamiteCount_ = 0; // real m_blupiDynamite, caps at 1
        static constexpr int kBulletCap = 10; // real m_blupiBullet cap
        int bulletCount_ = 0;
        static constexpr int kPersoCap = 5; // real m_blupiPerso cap
        int persoCount_ = 0;
        bool diedThisFrame_ = false; // reset at the top of every Update() call
        bool balloonTouchedThisFrame_ = false; // reset at the top of every Update() call
        bool balloonPoppedThisFrame_ = false; // reset at the top of every Update() call
        bool shieldGrantedThisFrame_ = false; // reset at the top of every Update() call
        bool powerGrantedThisFrame_ = false;  // reset at the top of every Update() call
        bool cloudGrantedThisFrame_ = false;  // reset at the top of every Update() call
        bool hideGrantedThisFrame_ = false;   // reset at the top of every Update() call
    };
}
