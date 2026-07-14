#pragma once

#include "GESound.hpp"
#include "GEWorldRuntime.hpp"

#include <random>

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
        // blupiFirePressed/blupiCanFire (2026-07-13, plan.md BULLET-001,
        // both default false so existing callers/tests are unaffected):
        // real Tank-mounted "Fire" (`KeyPressFlags::Fire`, a dedicated key
        // -- NOT the Action button used for dynamite/Perso/switches/
        // vehicle mount above) verified directly against Decor.cpp:
        // 4308-4344. blupiCanFire is the caller's own `GetVehicleMode()==
        // Tank` check (translated to a bool here, same
        // don't-expose-GEBlupiController's-enum pattern as
        // blupiCanGrantX above) -- Helicopter's own real firing branch
        // (`HelicoGlu`, Decor.cpp:3691-3706) was NOT independently
        // confirmed to actually spawn a projectile during this session's
        // research (only that channel 52 covers both vehicle paths) --
        // deliberately NOT modeled here until that's verified, rather
        // than guessed. blupiFirePressed is a LEVEL state (held-down),
        // not edge-triggered -- the real source has no separate "must
        // release between shots" debounce, just the cooldown gate below.
        // Real gate: `m_blupiTimeFire==0` (a 0.5s/Config::ScaleTime(10)
        // cooldown at the real 20fps base rate, ticked down every frame
        // regardless of input) -- ammo is checked only once the cooldown
        // has elapsed; `m_blupiBullet==0` plays the real out-of-ammo click
        // (channel 53) WITHOUT starting the cooldown (the real
        // `m_blupiTimeFire=ScaleTime(10)` assignment only happens on an
        // actual shot, Decor.cpp:4343) -- so holding Fire with no ammo
        // replays the click every frame (GESound's own "don't restart an
        // already-playing channel" policy naturally throttles this, see
        // GESound.hpp). A real shot consumes exactly 1 bullet
        // (`m_blupiBullet--`) and raycasts along Blupi's own facing
        // (blupiFacingDX/DZ above -- horizontal only, matching the real
        // Tank's own speed=+-5 encoding) to the next solid cell via the
        // same SearchAirDistance/MakeBullet helpers blupih/blupit's own
        // fired shots already use (ObjectType23, contact with Blupi
        // already fatal -- see this class's own existing kill-list
        // handling, unchanged by this feature).
        //
        // blupiCloudActive (plan.md `068`, `Decor::BlupiElectro`,
        // mobile-eggbert-reference/10-blupi-mechanics.md §9): while true
        // (caller's own `GetSecretPower()==Cloud` check), instantly
        // destroys small enemies (ObjectType4/32/33) within a real 40px
        // aura around Blupi -- an offensive aura Blupi carries, unrelated
        // to the `Blitz` lightning HAZARD despite the similarly-named
        // real function. Real sound channel 59 on each kill.
        void Update(float dt, GEWorldRuntime& worldRuntime,
                    float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                    GESound& sound, bool blupiCrouching = false, bool blupiBallooned = false,
                    int blupiFacingDX = 0, int blupiFacingDZ = 0, bool blupiInvincible = false,
                    bool blupiCanGrantShield = true, bool blupiCanGrantPower = true,
                    bool blupiCanGrantCloud = true, bool blupiCanGrantHide = true,
                    bool blupiFirePressed = false, bool blupiCanFire = false,
                    bool blupiCloudActive = false, bool blupiCanGrantInvert = true);

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
        // Real camera shake (plan.md CAM-008/009) -- true the one frame a
        // generic-hazard contact-kill fired SmallShake (most types) or
        // BigShake (fish/bird, ObjectType17/20) instead; the caller
        // triggers its own GECameraShake with the matching type.
        [[nodiscard]] bool SmallShakeTriggeredThisFrame() const noexcept { return smallShakeTriggeredThisFrame_; }
        [[nodiscard]] bool BigShakeTriggeredThisFrame() const noexcept { return bigShakeTriggeredThisFrame_; }

        // Secret power pickups (plan.md E3D-MIG-170, ObjectType25/26/30/31)
        // -- true the one frame that pickup's real gate passed and the
        // world object was actually removed; the caller then calls the
        // matching `GEBlupiController::TriggerX()` (its own internal gate
        // should agree, since both check the same state) and plays the
        // real grant sound only if that returns true. Shield grants
        // instantly on contact (real, single-stage). Power/Cloud/Hide
        // (plan.md `173`, 2026-07-14) are real 2-stage pickups -- the world
        // object is still destroyed HERE at contact (matching real
        // immediate `ObjectDelete`), and the caller plays the real
        // immediate "grab" sound + starts `GEBlupiController::
        // TriggerPickupFreeze()` here, but the actual buff grant (Power/
        // Hide only -- Cloud's own buff already grants at contact in real
        // source too) and the "complete" sound are deferred to
        // `GEBlupiController::ConsumePickupFreezeResolved()`, which also
        // needs the pickup's own original position to respawn it (real
        // `ObjectStart(pos, type, 0)` at completion) -- the Pickup*X/Y/Z()
        // getters below capture that position at the same moment as the
        // *ThisFrame() flag.
        [[nodiscard]] bool ShieldGrantedThisFrame() const noexcept { return shieldGrantedThisFrame_; }
        [[nodiscard]] bool PowerGrantedThisFrame() const noexcept { return powerGrantedThisFrame_; }
        [[nodiscard]] bool CloudGrantedThisFrame() const noexcept { return cloudGrantedThisFrame_; }
        [[nodiscard]] bool HideGrantedThisFrame() const noexcept { return hideGrantedThisFrame_; }
        [[nodiscard]] float PowerPickupX() const noexcept { return powerPickupX_; }
        [[nodiscard]] float PowerPickupY() const noexcept { return powerPickupY_; }
        [[nodiscard]] float PowerPickupZ() const noexcept { return powerPickupZ_; }
        [[nodiscard]] float CloudPickupX() const noexcept { return cloudPickupX_; }
        [[nodiscard]] float CloudPickupY() const noexcept { return cloudPickupY_; }
        [[nodiscard]] float CloudPickupZ() const noexcept { return cloudPickupZ_; }
        [[nodiscard]] float HidePickupX() const noexcept { return hidePickupX_; }
        [[nodiscard]] float HidePickupY() const noexcept { return hidePickupY_; }
        [[nodiscard]] float HidePickupZ() const noexcept { return hidePickupZ_; }
        // Re-spawns a pickup at its original position as a static, active
        // object (real `ObjectStart(pos, type, 0)`, speed=0) -- called by
        // the game class once the real 2-stage freeze resolves for
        // Sucette/Drink/Charge (plan.md `173`).
        void RespawnPickupItem(GEWorldRuntime& worldRuntime, float x, float y, float z,
                                GalaxyEggbert::ObjectType type);
        // Invert/Mirror pickup (plan.md PICKUP-011, ObjectType40) -- same
        // one-shot shape as the 4 signals above; the caller calls
        // GEBlupiController::TriggerInvert() and plays channel 66 only if
        // that returns true.
        [[nodiscard]] bool InvertGrantedThisFrame() const noexcept { return invertGrantedThisFrame_; }

        // Invert start/stop particle burst (plan.md VISUAL-014/015,
        // ObjectType41 on grant / ObjectType42 on expiry) -- spawns 4
        // instances around (blupiX,blupiY,blupiZ), one in each of the 4
        // real directions (up/down/+X/-X -- see .cpp for the real
        // screen-Y-to-world-Y sign flip, same convention as this engine's
        // camera shake). Real `Decor.cpp` confirms grant places them at an
        // exact 500 real-px radius (`ObjectStart(m_blupiPos, ...)`, no
        // pre-offset, `Decor.cpp:6048-6051`) while expiry places them
        // slightly closer at 400 real-px (`Decor.cpp:5137-5158`'s own
        // ±100px pre-offset partially cancelling the same 500px final
        // offset) -- both converted to this engine's world units via the
        // same 64px-per-tile scale used throughout. Called directly by the
        // game class at its own existing Invert grant/expiry sites (this
        // class's own Update() has already returned by the time those
        // fire, so it can't use the pendingSpawns deferred-spawn pattern --
        // reuses an inactive slot first, matching that same real
        // MoveObjectFree() slot-reuse semantics via a direct push instead).
        void SpawnInvertBurst(GEWorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                               bool isGrant);

        // Fan-hit shockwave flash (plan.md CAM-009-adjacent, ObjectType11)
        // -- a single instance spawned exactly at (x,y,z), no offset (real
        // `ObjectStart(celSwitch, ObjectType11, 0)`, `Decor.cpp:5467` --
        // the real `celSwitch` -34/-34 pixel pre-offset is a 2D
        // sprite-corner-anchoring artifact, not meaningful for this
        // engine's center-anchored billboards, same reasoning already
        // applied to every other real `ObjectStart(..., 0)` site this
        // session). Called directly by the game class at its own existing
        // Fan-hazard site (`GEInteractionSystem::Update()` isn't involved
        // in that check), same reasoning as `SpawnInvertBurst()` above.
        void SpawnFanHitFlash(GEWorldRuntime& worldRuntime, float x, float y, float z);

        // Teleporter arc (plan.md VISUAL-010, ObjectType92) -- despite
        // ObjectType.hpp's own "spawned when Blupi uses a charged attack"
        // doc comment, the ONLY real spawn site is the teleporter-trigger
        // block (`Decor.cpp:5593-5606`, already this engine's own existing
        // `TriggerTeleport()` call site) -- a single instance at
        // (blupiX, blupiY+5/64, blupiZ) (real `celSwitch = (blupiPos.X,
        // blupiPos.Y-5)`, screen Y- -> world Y+, the usual sign flip), no
        // further offset (`speed=0`). Real self-delete at phase>=128, a
        // long 6.4s lifetime matching this engine's own
        // `GEBlupiController::kTeleportDuration` exactly -- the arc plays
        // for the whole teleport transit. Called directly by the game
        // class at its own existing `TriggerTeleport()` success site, same
        // reasoning as `SpawnFanHitFlash()` above.
        void SpawnTeleportArc(GEWorldRuntime& worldRuntime, float x, float y, float z);

        // Pollution puff (plan.md VISUAL-013, ObjectType36) -- real
        // vehicle-exhaust smoke, confirmed via direct source read
        // (`Decor::MoveObjectPollution()`, `Decor.cpp:6877-6989`). Called
        // once per frame unconditionally (the real function is called from
        // 4 different movement-mode branches, but is itself the gate --
        // `if (!flag) return;` -- so a single unconditional call is
        // behaviorally equivalent). isHelicopter/isOvercraft/isJeep/isTank
        // are NOT mutually exclusive in the real source's own if-chain, but
        // only one can realistically be true at a time (Blupi drives one
        // vehicle at a time). isMoving is the real `m_blupiVitesseX==0.0`
        // check (false = stationary); ascending is real `m_blupiSpeedY<0.0`
        // (Overcraft) / `m_blupiVitesseY<-5.0` (Helicopter, threshold
        // dropped as a simplification -- just "moving up"); facingDX>=0 is
        // real `Direction::Right`. Real `m_blupiPhase` (Jeep/Tank's own
        // emission schedule) is Blupi's own animation-state-phase counter,
        // reset on every real action-state change -- this engine has no
        // equivalent action-state machine yet (no visible Blupi model/
        // animation exists at all, NEXT.md's single largest gap), so this
        // reuses the same monotonic, never-resetting tick counter as
        // Helicopter/Overcraft's real `m_time` -- a deliberate, documented
        // simplification for this purely-cosmetic effect (the puff cadence
        // drifts out of exact sync with real per-action resets, but still
        // produces the same irregular-feeling emission pattern). Spawns via
        // the existing generic `AdvancePatrolStep()` machinery (sets
        // `posStart`/`posEnd`/`stepAdvanceTicks`/`patrolStep=2` directly,
        // matching real `ObjectStart()`'s own `step=2` skip-the-dwell-phase
        // behavior) rather than a bespoke interpolation block --
        // `SpawnInvertBurst()`/treasure sparkle were refactored the same
        // day (2026-07-14) to also use this shared machinery.
        void TickPollutionPuff(GEWorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                               bool isHelicopter, bool isOvercraft, bool isJeep, bool isTank, bool isMoving,
                               bool ascending, int facingDX);

        // Shield/Power magic trail (plan.md VISUAL-011/E3D-MIG-??,
        // ObjectType57/27) -- real `Decor.cpp:5204-5237`: while Shield or
        // Power is active, drops a single static sparkle marker exactly at
        // Blupi's current position every time he has moved a real
        // Manhattan (X+Y, screen-space) distance of >=40px since the last
        // drop (`m_blupiPosMagic`), then resets the tracker to the new
        // drop point -- a breadcrumb trail, not a burst (posStart=posEnd=
        // spawn point, no offset/interpolation, same `speed=0` shape as
        // `SpawnFanHitFlash()`). Real `m_blupiPosMagic` is a SINGLE shared
        // tracker (Shield/Power/Hide can never be active simultaneously,
        // per this engine's own mutually-exclusive `SecretPower` enum), so
        // one tracker suffices here too. `ResetMagicTrail()` mirrors every
        // real grant site's own `m_blupiPosMagic = m_blupiPos` (confirmed
        // at multiple real grant sites, e.g. `Decor.cpp:6022`) -- call it
        // at Shield/Power's own grant sites, same idiom as
        // `SpawnInvertBurst()`'s grant-site call. Real distance check
        // ignores Z (matching every other 2D-source-derived check this
        // session, e.g. follower homing) -- only X/Y accumulate toward the
        // 40px threshold, though the spawned marker still uses Blupi's
        // full 3D position. Hide's own real afterimage trail
        // (`ObjectType58`, a snapshot of Blupi's OWN current sprite, not a
        // fixed icon) is NOT modeled here -- blocked on the same "no
        // visible Blupi model/animation" gap as Pollution puff's
        // Jeep/Tank `m_blupiPhase` simplification, not a quick add. Real
        // level-load/respawn resets of `m_blupiPosMagic` (several
        // additional real call sites) are also NOT modeled -- a minor,
        // accepted simplification (a stale tracker only shifts the first
        // post-respawn marker's exact trigger point, cosmetic only).
        void ResetMagicTrail(float x, float y, float z);
        void TickMagicTrail(GEWorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ, bool isShielded,
                             bool isPowered);

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

        // Voyage (plan.md `158`, real `Decor::VoyageInit`/`VoyageStep`/
        // `VoyageDraw`, `Decor.cpp:10141-10348`) -- real pickups do NOT
        // apply their reward immediately: the world object is deleted on
        // contact, but the actual counter increment (plus e.g. re-scanning
        // treasure-gated doors) only happens once a short 2D "fly to HUD
        // icon" animation completes. Previously modeled as immediate
        // application (a documented simplification); this class now
        // matches the real deferred timing. Real state: which reward, a
        // start/end point in mobile-eggbert's 640x480 reference screen
        // space (the same space `GEHud` already uses), a phase counter,
        // and `total = (|dx|+|dy|)/10` ticks (real integer-truncated
        // Manhattan screen distance). Only one voyage is active at a time;
        // starting a new one force-completes the previous one's reward
        // immediately (real `VoyageInit`'s own
        // `if (m_voyageIcon != -1) { phase = total; Step(); }`).
        //
        // This class has no camera access, so it can't project a pickup's
        // 3D world position into 2D screen space itself. Instead, pickup
        // sites record a "pending" voyage request (this-frame flag +
        // whichever ONE endpoint still needs projecting, plus the other,
        // already-resolved endpoint) via `RequestVoyage()`; the caller
        // (`GalaxyEggbertCnaGame.cpp`, which has `camera_`) checks
        // `VoyagePendingThisFrame()` right after `Update()` returns,
        // projects the world point via `GEHud::ProjectWorldToHudSpace()`,
        // and calls `BeginVoyage()` with both fully-resolved endpoints --
        // the same "called by the game class right after Update()
        // returns" shape already used for `SpawnInvertBurst()`/
        // `SpawnTeleportArc()` above. A pickup touched while ANOTHER
        // pickup's request from the SAME frame is still pending
        // (un-consumed) overwrites it, silently dropping the first one's
        // reward -- an accepted, documented simplification for a
        // vanishingly rare edge case (two different pickups touched in
        // the exact same tick), not worth the extra bookkeeping.
        // Clear2Ascend/Clear3Ascend (plan.md `158` death-VFX follow-up,
        // verified directly against `Decor::BlupiDead`/`VoyageInit`/
        // `VoyageStep`/`VoyageDraw`, Decor.cpp:6547-6614/10141-10350): the
        // real "soul ascends" Voyage fired by 2 of Blupi's 8 death-
        // animation types (`BlupiAction::Clear2`/`Clear3` -- Clear1's own
        // death has no VFX at all, Clear4/Saw is a particle burst not a
        // Voyage, Clear5-8 are unreachable dead code, and Glu -- spikes/
        // drip/projectile/large-creature-grab deaths -- is a wholly
        // separate, un-researched "stuck in goo" mechanic, out of scope
        // here). Unlike every pickup kind above, BOTH endpoints derive
        // from Blupi's OWN position (start = Blupi projected to HUD
        // space, end = straight up from there by a fixed HUD-space
        // offset -- 300 for Clear2, 2000 for Clear3) rather than one
        // pickup-fixed HUD point -- a natural technical adaptation of the
        // real `pos`/`pos2` (both `m_blupiPos - m_posDecor`, differing
        // only by a real screen-Y offset before the shared
        // `HotSpotToHud()` transform) for an engine with no 2D scrolling
        // "decor" pixel space. No reward at either kind's completion
        // (confirmed: neither icon 230 nor 40 appears in `VoyageStep`'s
        // completion if-chain, Decor.cpp:10256-10304).
        enum class VoyageKind : std::uint8_t
        {
            None, Treasure, Key1, Key2, Key3, Egg, Dynamite, Perso, BulletPack, DoorUnlock,
            Clear2Ascend, Clear3Ascend,
            // Real life-loss Voyage (icon 48/Blupi channel, `Decor.cpp:6383-6389`/`10172-10176`,
            // death-lock follow-up) -- the ONLY Voyage kind whose effect (LoseLife()) fires at
            // START, not completion (`BeginVoyage()`'s own switch calls it directly, matching
            // real `VoyageInit`'s own inline `m_nbVies--`). Fixed total=40 (real `ScaleTime(40)`),
            // start = the lives-HUD icon position (`210+16*lives_`, using the PRE-decrement count
            // -- real `VoyageGetPosVie(m_nbVies)` is evaluated as a call argument before
            // `VoyageInit`'s own body runs), end = Blupi's own (possibly just-respawned) position
            // projected to HUD space by the caller. No reward at completion (`VoyageStep`'s
            // icon==48 branch only does `Stop`/`m_blupiFocus=true`).
            LifeLoss
        };

        [[nodiscard]] bool VoyagePendingThisFrame() const noexcept { return voyagePendingThisFrame_; }
        [[nodiscard]] VoyageKind VoyagePendingKind() const noexcept { return pendingKind_; }
        [[nodiscard]] int VoyagePendingIconId() const noexcept { return pendingIconId_; }
        [[nodiscard]] bool VoyagePendingIsButtonChannel() const noexcept { return pendingIsButton_; }
        [[nodiscard]] float VoyagePendingWorldX() const noexcept { return pendingWorldX_; }
        [[nodiscard]] float VoyagePendingWorldY() const noexcept { return pendingWorldY_; }
        [[nodiscard]] float VoyagePendingWorldZ() const noexcept { return pendingWorldZ_; }
        [[nodiscard]] float VoyagePendingFixedX() const noexcept { return pendingFixedX_; }
        [[nodiscard]] float VoyagePendingFixedY() const noexcept { return pendingFixedY_; }
        // True: the projected world point is the voyage's START (every
        // pickup -- flies FROM the pickup's own position TO a fixed HUD
        // point). False: the world point is the voyage's END (real
        // door-unlock key-consumption flourish only -- flies FROM the
        // fixed HUD key position TO the door's own world position).
        [[nodiscard]] bool VoyagePendingWorldIsStart() const noexcept { return pendingWorldIsStart_; }
        // True only for the generic-hazard-contact Clear2 coinflip site
        // (inside Update(), no camera access -- unlike Clear2/Clear3's
        // OTHER 2 real trigger sites, fall-off-world and Lava, which live
        // directly in `GalaxyEggbertCnaGame.cpp` and so call `BeginVoyage()`
        // straight away, no pending round-trip needed). When true, the
        // caller computes end = (projectedX, projectedY -
        // VoyagePendingAscendOffsetY()) instead of reading
        // VoyagePendingFixedX/Y() (which are unused/stale in this mode).
        [[nodiscard]] bool VoyagePendingIsAscend() const noexcept { return pendingIsAscend_; }
        [[nodiscard]] float VoyagePendingAscendOffsetY() const noexcept { return pendingAscendOffsetY_; }

        // Death-lock request (death-lock/life-loss-Voyage follow-up) from the 4 real trigger sites
        // living inside Update() itself, which has no `GEBlupiController&`/camera access (every
        // OTHER real cause -- fall/Lava/Saw/Blitz/Drown/Fan -- is checked directly in
        // `GalaxyEggbertCnaGame.cpp`, which already has `blupi_` in scope and calls
        // `blupi_.TriggerDeathLock()` straight away): generic-hazard-contact (Clear1/Clear2 real
        // 50/50 coinflip, `shouldRespawn=false` -- confirmed no `m_blupiRestart=true` near
        // Decor.cpp:5782-5815), dynamite blast (deterministic Clear1, `shouldRespawn=false` --
        // confirmed no `m_blupiRestart=true` near Decor.cpp:9168-9173 either, a correction to an
        // earlier assumption that only Fan/generic-hazard lacked it), and fired-projectile-contact
        // (ObjectType23)/large-creature-grab (ObjectType54), both real Glu, `shouldRespawn=true`
        // (confirmed `m_blupiRestart=true` at Decor.cpp:5879/5927). A LOCAL enum, deliberately NOT
        // shared with `GEBlupiController::DeathCause` (this class stays fully decoupled from it,
        // same reasoning as every other pending signal here) -- the caller maps this to the real
        // `DeathCause` when calling `TriggerDeathLock()`. This is a SEPARATE signal from the
        // VoyagePending* fields above -- a Clear2 outcome sets BOTH in the same frame (the
        // cosmetic Clear2Ascend request via RequestClear2Ascend() below, AND this one), consumed
        // independently by the caller.
        enum class PendingDeathKind : std::uint8_t { Clear1, Clear2, Glu };
        [[nodiscard]] bool DeathLockRequestedThisFrame() const noexcept { return deathLockRequestedThisFrame_; }
        [[nodiscard]] PendingDeathKind DeathLockPendingKind() const noexcept { return deathLockPendingKind_; }
        [[nodiscard]] bool DeathLockShouldRespawn() const noexcept { return deathLockShouldRespawn_; }

        // Called by the game class once it has resolved both endpoints
        // (projecting whichever one was still a world position). Force-
        // completes any voyage already in flight first (see class comment).
        // worldAnchorX/Y/Z is Blupi's own 3D world position at the moment
        // the voyage started -- unused by every pickup kind (defaulted to
        // 0), but needed by Clear3Ascend to anchor its own real continuous
        // world-space puff-particle spawn (see TickVoyage()'s own comment).
        void BeginVoyage(GEWorldRuntime& worldRuntime, VoyageKind kind, int iconId, bool isButtonChannel,
                          float startX, float startY, float endX, float endY, GESound& sound,
                          float worldAnchorX = 0.0f, float worldAnchorY = 0.0f, float worldAnchorZ = 0.0f);

        // Real 50/50 Clear1(nothing)/Clear2(ascend) coinflip
        // (`Decor::BlupiDead(action1, action2)`'s own `m_random.get()->
        // Next() % 2 == 0` choice, Decor.cpp:6551-6554), shared by the Fan
        // and generic-hazard-contact real trigger sites (the only 2 of
        // Clear2's 3 real trigger sites that aren't deterministic --
        // fall-off-world and Lava always pick Clear2/Clear3 outright).
        // Returns true if Clear2 was picked (caller then begins/requests
        // the ascend voyage + plays channel 74); false means Clear1 (do
        // nothing further -- Clear1 itself has no real VFX or sound).
        [[nodiscard]] bool RollClear2Coinflip();

        // Real Clear4/Saw death VFX (`Decor::BlupiDead`'s own Clear4
        // branch, Decor.cpp:6608-6613): 3 ObjectType41 particles (up/
        // right/left, no "down" -- decoded directly from the real
        // `ObjectStart(pos, ObjectType41, speed)` calls with speed
        // -70/20/-20 via `Decor::ObjectStart`'s own direction/magnitude
        // logic, Decor.cpp:7805-7869) + channel 75. Deterministic (Saw
        // death is always Clear4, no coinflip) -- called directly by the
        // game class in place of its existing `triggerDeath(ch75)` call
        // (which must instead pass playChannel=false to avoid double-
        // playing channel 75, since this method plays it itself, matching
        // the real source's own single call site for that sound).
        void SpawnSawDeathBurst(GEWorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                                 GESound& sound);

        // Draw-side state for `GEHud::Draw()` -- current interpolated
        // position in the same 640x480 reference space.
        [[nodiscard]] bool VoyageActive() const noexcept { return voyageKind_ != VoyageKind::None; }
        [[nodiscard]] int VoyageIconId() const noexcept { return voyageIconId_; }
        [[nodiscard]] bool VoyageIsButtonChannel() const noexcept { return voyageIsButton_; }
        [[nodiscard]] float VoyageDrawX() const noexcept;
        [[nodiscard]] float VoyageDrawY() const noexcept;
        // False only for Clear3Ascend during its real 30-tick pre-move
        // delay (`Decor::VoyageDraw`'s own `if (icon != 40 || channel !=
        // Element || num != 0) HudIcon(...)` guard, Decor.cpp:10318) --
        // every other kind (including Clear2Ascend, which has no delay)
        // is always visible while active.
        [[nodiscard]] bool VoyageIconVisible() const noexcept;

        // Bullet pack (ObjectType29, plan.md E3D-MIG-175, real Decor.cpp
        // ~5731-5744). Automatic on contact, no button, gated on
        // `bulletCount_ < kBulletCap` (real m_blupiBullet < 10) -- picking
        // one up at the cap does nothing (object stays in the world,
        // matching the real source exactly). Tops up to exactly
        // kBulletCap, not a running total (`+= 10` then clamped in the real
        // source has the same net effect here since the gate already
        // guarantees the prior count was below the cap). The actual
        // firing/ammo-consumption mechanic (Tank fire button, real
        // ObjectType23 projectile spawn) is done -- see Update()'s own
        // blupiFirePressed/blupiCanFire parameters (plan.md BULLET-001).
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

        // Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13), verified
        // directly against `Decor::CheatAction(Tables::CheatCodes)` --
        // overturned several draft assumptions in this repo's own earlier
        // (unverified) plan.md checklist, see each method's own comment
        // for the correction. Real dispatch is one-shot (pressing a cheat
        // button immediately closes the overlay and applies the effect,
        // no confirmation) -- these methods mirror that: called once, not
        // every frame, from the caller's own button-press handler.
        //
        // Cheat1 "OpenDoors": real `m_bCheatDoors` is a persistent toggle
        // re-applied via `AdaptDoors()`; ported here as a one-shot "open
        // every door in the world right now" instead (this engine has no
        // reversible "close it back up" bookkeeping, and the toggle's
        // OBSERVABLE effect -- every door open -- is what matters).
        // Scans the whole grid for both real door families: key-gated
        // (`BlockTypes::isDoor()`, icons 334-336) and treasure-gated
        // (icon 421+N, scanned up to this session's own confirmed real
        // total icon ceiling of 440) -- opens both regardless of whether
        // Blupi actually holds the matching key/treasure count.
        void CheatOpenDoors(GEWorldRuntime& worldRuntime, GESound& sound);

        // Cheat7 "CleanAll": real effect converts a SPECIFIC hazard/enemy
        // type list (`2,3,4,16,17,20,32,33,44,54,96,97` -- exactly this
        // class's own existing shared-kill-list set plus wasp/large-
        // creature/blupih/blupit) into an explosion decoration with a
        // screen-shake (real SmallShake, `Decor.cpp:1811`, plan.md
        // CAM-008, confirmed and wired 2026-07-14 -- see the returned
        // bool below), NOT "remove all mobile objects" (an earlier draft
        // description of this cheat was wrong -- treasures/pickups/
        // vehicles are untouched). Ported here as deactivating every
        // active instance of those 12 types; the explosion-decoration
        // visual is a documented simplification (no particle system
        // exists). Returns true if anything was actually destroyed, so
        // the caller only triggers its own GECameraShake on a real
        // effect, same idiom as every other *ThisFrame() signal here.
        bool CheatCleanAll(GEWorldRuntime& worldRuntime);

        // Cheat8 "AllTreasure": every active ObjectType5 is collected at
        // once (deactivated, `treasuresCollected_` incremented per
        // treasure found), then the existing treasure-gated door scan
        // (the same one a real pickup already triggers) opens every door
        // now satisfied -- reuses `OpenDoorAt()`/the real Channel11
        // fanfare, not a separate mechanism.
        void CheatAllTreasure(GEWorldRuntime& worldRuntime, GESound& sound);

        // Cheat9 "EndGoal": real effect ALWAYS teleports Blupi to the
        // exit (`ObjectType7`), but only actually triggers a real win if
        // `treasuresCollected_ >= totalTreasures_` at that moment --
        // otherwise Blupi just arrives at the exit without winning (an
        // earlier draft description of this cheat as an unconditional
        // "win immediately" was wrong). Returns the exit's real position
        // via out-params (false if no active exit exists in the world);
        // the caller teleports Blupi there itself (this class has no
        // access to GEBlupiController) -- the EXISTING exit-contact
        // detection in Update() then naturally re-evaluates the real
        // win/no-win gate on the very next frame, so no separate
        // win-forcing logic is needed here at all.
        [[nodiscard]] bool CheatFindExit(const GEWorldRuntime& worldRuntime,
                                         float& outX, float& outY, float& outZ) const;

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
        int pollutionTick_ = 0; // stands in for real m_time/m_blupiPhase, see TickPollutionPuff()'s own comment

        // Voyage (plan.md `158`), see its own public-API comment above.
        // Active/in-flight state:
        VoyageKind voyageKind_ = VoyageKind::None;
        int voyageIconId_ = 0;
        bool voyageIsButton_ = false;
        float voyageStartX_ = 0.0f, voyageStartY_ = 0.0f, voyageEndX_ = 0.0f, voyageEndY_ = 0.0f;
        float voyagePhase_ = 0.0f, voyageTotal_ = 0.0f;
        // Blupi's own world position at the moment the active voyage
        // started (see BeginVoyage()'s own comment) -- only meaningful for
        // Clear3Ascend's continuous puff-particle spawn.
        float voyageWorldAnchorX_ = 0.0f, voyageWorldAnchorY_ = 0.0f, voyageWorldAnchorZ_ = 0.0f;
        // This-frame pending request (consumed by the game class):
        bool voyagePendingThisFrame_ = false;
        VoyageKind pendingKind_ = VoyageKind::None;
        int pendingIconId_ = 0;
        bool pendingIsButton_ = false;
        float pendingWorldX_ = 0.0f, pendingWorldY_ = 0.0f, pendingWorldZ_ = 0.0f;
        float pendingFixedX_ = 0.0f, pendingFixedY_ = 0.0f;
        bool pendingWorldIsStart_ = true;
        bool pendingIsAscend_ = false;
        float pendingAscendOffsetY_ = 0.0f;
        bool deathLockRequestedThisFrame_ = false;
        PendingDeathKind deathLockPendingKind_ = PendingDeathKind::Clear1;
        bool deathLockShouldRespawn_ = false;
        // First randomness needed anywhere in this engine's gameplay code
        // (real `Decor::BlupiDead`'s own `m_random`, Decor.cpp:6551) --
        // seeded from real entropy since the real coinflip this ports is
        // genuinely non-deterministic (cosmetic-only: it never affects
        // `lives_`/reward state, only whether the ascend VFX/sound plays).
        std::mt19937 rng_{std::random_device{}()};

        void TickVoyage(float dt, GEWorldRuntime& worldRuntime, GESound& sound);
        void ApplyVoyageReward(GEWorldRuntime& worldRuntime, GESound& sound);
        // Records a this-frame voyage request (see the public API comment
        // above for the world/fixed-point split). Called from the pickup
        // switch, TryPerso(), and the key-gated-door block.
        void RequestVoyage(VoyageKind kind, int iconId, bool isButtonChannel, float worldX, float worldY,
                            float worldZ, float fixedX, float fixedY, bool worldIsStart);
        // Records a this-frame Clear2Ascend request from the generic-
        // hazard-contact site (the only Clear2 trigger site inside
        // Update() itself, with no camera access -- see
        // VoyagePendingIsAscend()'s own comment).
        void RequestClear2Ascend(float worldX, float worldY, float worldZ);
        // Real `Decor::VoyageDraw`'s icon==40 puff spawn (Decor.cpp:
        // 10331-10348), called once per TickVoyage() while Clear3Ascend is
        // active (see TickVoyage()'s own comment).
        void SpawnLavaAscendPuff(GEWorldRuntime& worldRuntime);
        // Real m_blupiPosMagic, see TickMagicTrail()'s own comment. Only
        // consulted once Shield/Power has actually granted (which always
        // calls ResetMagicTrail() first), so this default is never read
        // uninitialized.
        float magicTrailLastX_ = 0.0f, magicTrailLastY_ = 0.0f, magicTrailLastZ_ = 0.0f;
        static constexpr int kBulletCap = 10; // real m_blupiBullet cap
        int bulletCount_ = 0;
        // Real Tank "Fire" cooldown (2026-07-13, plan.md BULLET-001):
        // Config::ScaleTime(10) at the real 20fps base rate = 0.5s,
        // ticked down every Update() regardless of input; only reset on
        // an actual shot (see Update()'s own class comment).
        static constexpr float kFireCooldownSeconds = 0.5f;
        float fireCooldownTimer_ = 0.0f;
        static constexpr int kPersoCap = 5; // real m_blupiPerso cap
        int persoCount_ = 0;
        bool diedThisFrame_ = false; // reset at the top of every Update() call
        bool balloonTouchedThisFrame_ = false; // reset at the top of every Update() call
        bool balloonPoppedThisFrame_ = false; // reset at the top of every Update() call
        bool smallShakeTriggeredThisFrame_ = false; // reset at the top of every Update() call
        bool bigShakeTriggeredThisFrame_ = false; // reset at the top of every Update() call
        bool shieldGrantedThisFrame_ = false; // reset at the top of every Update() call
        bool powerGrantedThisFrame_ = false;  // reset at the top of every Update() call
        bool cloudGrantedThisFrame_ = false;  // reset at the top of every Update() call
        bool hideGrantedThisFrame_ = false;   // reset at the top of every Update() call
        bool invertGrantedThisFrame_ = false; // reset at the top of every Update() call
        float powerPickupX_ = 0.0f, powerPickupY_ = 0.0f, powerPickupZ_ = 0.0f;
        float cloudPickupX_ = 0.0f, cloudPickupY_ = 0.0f, cloudPickupZ_ = 0.0f;
        float hidePickupX_ = 0.0f, hidePickupY_ = 0.0f, hidePickupZ_ = 0.0f;
    };
}
