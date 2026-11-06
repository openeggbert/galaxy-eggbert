#pragma once

#include "Sound.hpp"
#include "WorldRuntime.hpp"

#include <GalaxyEggbert/Def/BlupiAction.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <random>
#include <unordered_set>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Real mobile-eggbert interactive-object behavior (2026-07-10): platform
    // lift patrol movement, crate push, and pickup collection (treasure/egg/
    // keys/level-exit) -- the first slice of interactive-object support.
    // Behavior is grounded in the researched `mobile-eggbert-reference/13-object-
    // pickups.md`/`07-sounds.md` behavioral spec for exact pickup semantics
    // (which sound channel, whether the object is actually removed, egg's
    // MAX_EGG_COUNT=10 cap, exit's treasure-gate) -- the spec is the more
    // authoritative source; the real source deletes every pickup
    // immediately on contact per `13-object-pickups.md`'s "Voyage" section.
    // This is a minimal implementation operating directly on
    // WorldRuntime's MobileObjSpec list and Blupi's live position.
    //
    // A basic lives foundation now exists (2026-07-11, LoseLife()/Lives()
    // below, plan.md E3D-MIG-130) -- just the counter and its real
    // reset-to-3-on-zero behavior, wired so far only to the one
    // always-active hazard that needed no per-tile-type work at all
    // (falling off the world, see GalaxyEggbertGame::Update()). This
    // does NOT yet unblock full hazard/enemy contact below -- those still
    // need their own per-type behavior (Phase 14/13), not just a lives
    // counter to decrement.
    //
    // Shared kill-list contact (2026-07-11, plan.md E3D-MIG-132, widened
    // 2026-07-11) now works for GalaxyEggbert::Def::ObjectType 2/3/4/16/17/20/96/97 -- touching
    // any of them kills Blupi and destroys it. Verified directly against
    // the real Decor.cpp:5782-5816 source: that one code block IS the real
    // shared contact check for exactly this set of 8 types (2=patrol
    // hazard, 3=patrol hazard, 4=bulldozer, 16=spider, 17=fish, 20=bird,
    // 96/97=follower dormant/awake) -- the only difference the real source
    // makes between them is purely cosmetic (17/20 get a bigger screen-
    // shake + a different explosion GalaxyEggbert::Def::ObjectType), not a behavioral
    // difference in whether/how Blupi dies, so the registry's
    // genericContactHazard capability covers all
    // 8 with one check rather than splitting them into separate per-type
    // branches that would all do the same thing. Real death sound is a
    // 50/50 coinflip between channel 74 and silence -- BlupiDead(Clear1,
    // Clear2)'s own Clear2 branch plays channel 74, Clear1 plays nothing --
    // simplified here to always channel 74, a documented approximation of
    // the coinflip rather than an invented value. Type3's real duck-
    // immunity (skipped while Blupi's action is Down) IS modeled via the
    // blupiCrouching parameter below. The fish/bird's (17/20) bigger burst
    // (ObjectType10) and the follower's homing (96/97, below) are modeled
    // too; the taunts the enemies call for are MockeryDetect()'s (BD-0044).
    //
    // Wasp (ObjectType44) "balloon" status now works (2026-07-11, plan.md
    // E3D-MIG-135), verified directly against Decor.cpp:5826-5863 (trigger)
    // and 5766-5781 (the balloon-pop interaction with hazards). Contact
    // does NOT kill Blupi or destroy the wasp -- it signals a
    // BalloonTouched event (EventsThisFrame()) every frame Blupi overlaps
    // it; the real `!m_blupiBalloon` re-trigger guard lives in
    // BlupiController::TriggerBalloon() itself (idempotent, same pattern
    // as TriggerCrush()), not here, since InteractionSystem has no access
    // to Blupi's current balloon state. While ballooned (the caller passes
    // this in via blupiBallooned), touching one of exactly 4 of the 8
    // shared-kill-list types (3/16/96/97 -- NOT 2/4/17/20, confirmed by the
    // real source's if/else-if chain: the pop check for 3/16/96/97 comes
    // FIRST and is mutually exclusive with the kill check right after it,
    // but 2/4/17/20 only ever reach the kill check) pops the balloon
    // instead of killing -- signaled via a BalloonPopped event, same
    // caller-applies-the-actual-state-change split as Died.
    //
    // Blupih/blupit (ObjectType32/33) stationary shooters now work
    // (2026-07-10, plan.md E3D-MIG-134), verified directly against
    // Decor.cpp:8878-8969 (attack timing) and 7794-7869 (the real
    // ObjectStart raycast/travel-distance encoding). Their own body is
    // NOT a damage path (they lack the registry's genericContactHazard
    // capability) -- only their fired
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
    // types, there is no separate pop path for it. Shield/Hide/superBlupi
    // (blupiInvincible) and focus (blupiFocused, BD-0060) spare him; in a
    // vehicle or squashed he still dies and the vehicle bursts at the
    // creature (BD-0061). Its taunt is MockeryDetect()'s Mockeryp (BD-0044).
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
    // RideStandY()'s own comment. BlupiController itself still has no
    // knowledge of MobileObjSpec objects (same engine-agnostic split as
    // every other terrain/object fact it doesn't compute itself) -- the
    // caller (GalaxyEggbertGame::Update()) reads these back and calls
    // BlupiController::RideLift() with them. Real fast-fall tunnelling
    // prevention (the 30px swept multi-step probe) is NOT modeled -- this
    // engine's own gravity/dt doesn't cross a lift's height in one frame
    // under normal conditions. Types 47/48's real conveyor nudge is folded
    // into RideDeltaX() directly (kConveyorNudgeSpeed: the real 2 px a tick, 0.625 blocks a
    // second).
    //
    // NOT yet implemented (deliberately, not an oversight):
    //  - Every other pickup type in IsPickup() (helicopter, shield, drink,
    //    mirror/invert, vehicles, dynamite, etc.) -- deferred to a follow-up;
    //    only the types placed in today's sample world (treasure/egg/keys/
    //    exit) are wired up.
    class InteractionSystem
    {
    public:
        // Advances patrol movement + crate push, and detects/collects
        // pickups Blupi is standing on. Call once per frame, after
        // blupi.Step() (so blupiX/Y/Z are this frame's final position) --
        // blupiMoveDX is this frame's X position delta (blupiX this frame
        // minus blupiX last frame), used as the crate-push velocity signal
        // (BlupiController exposes no velocity accessor; the
        // caller computes this itself from before/after Step() positions).
        // blupiCrouching gates type3's real duck-immunity, default false so
        // existing callers/tests that don't care about it are unaffected.
        // blupiBallooned (default false, same reason) gates whether
        // touching a 3/16/96/97 hazard pops the balloon instead of
        // killing, AND whether the large creature (54)'s turn-dwell
        // contact is lethal at all (real `!m_blupiBalloon` gates that
        // whole branch, no pop path for it) -- the caller reads this back
        // from BlupiController::IsBallooned() before calling Update(),
        // same as blupiCrouching's own GetAnimState()-derived source. If this
        // call kills Blupi via enemy contact, a Died event (EventsThisFrame())
        // fires for the rest of this frame only -- InteractionSystem has no
        // access to BlupiController, so the caller is the one that must
        // actually respawn Blupi (see GalaxyEggbertGame::Update()).
        // blupiFacingDX/DZ (plan.md E3D-MIG-160, both default 0 so existing
        // callers/tests are unaffected) are the rounded cardinal-direction
        // grid offset one cell in front of Blupi's current facing (the
        // caller derives this from BlupiController::GetYaw()) -- doors
        // (real `Decor::IsDoor`) probe BOTH Blupi's own cell and this one,
        // so he can trigger a door a step before actually reaching it,
        // matching the real source exactly.
        // blupiInvincible (plan.md E3D-MIG-170, both default false so
        // existing callers/tests are unaffected) is the caller's own
        // `BlupiController::IsInvincible()` (Shield or Hide active) --
        // gates every hazard/enemy death check inside this class (real
        // `!m_blupiShield && !m_blupiHide`, confirmed identical across
        // essentially every hazard in Decor.cpp). blupiCanGrantShield/
        // Power/Cloud/Hide are the caller's own per-power gate state
        // (`BlupiController::GetSecretPower()`, translated to 4 bools
        // here rather than exposing the enum, since this class doesn't
        // otherwise depend on BlupiController's types) used to decide
        // whether touching a secret-power pickup (ObjectType25/26/30/31)
        // actually grants it -- see the *GrantedThisFrame() signals below.
        // blupiFirePressed/blupiCanFire (2026-07-13, plan.md BULLET-001,
        // both default false so existing callers/tests are unaffected):
        // real Tank-mounted "Fire" (`GalaxyEggbert::Def::KeyPressFlags::Fire`, a dedicated key
        // -- NOT the Action button used for dynamite/Perso/switches/
        // vehicle mount above) verified directly against Decor.cpp:
        // 4308-4344. blupiCanFire is the caller's own vehicle check (the tank,
        // or the helicopter off the ground: its glue drop, HelicoGlu,
        // Decor.cpp:3694-3708 and 3058-3075 -- a glue ball dropped under it,
        // the helicopter kicked up 10 px a tick; blupiDropsGlue).
        // blupiFirePressed is a LEVEL state (held-down),
        // not edge-triggered -- the real source has no separate "must
        // release between shots" debounce, just the cooldown gate below.
        // Real gate: `m_blupiTimeFire==0` (a 0.5s/Config::ScaleTime(10)
        // cooldown at the real 20fps base rate, ticked down every frame
        // regardless of input) -- ammo is checked only once the cooldown
        // has elapsed; `m_blupiBullet==0` plays the real out-of-ammo click
        // (channel 53) WITHOUT starting the cooldown (the real
        // `m_blupiTimeFire=ScaleTime(10)` assignment only happens on an
        // actual shot, Decor.cpp:4343) -- so holding Fire with no ammo
        // replays the click every frame (Sound's own "don't restart an
        // already-playing channel" policy naturally throttles this, see
        // Sound.hpp). A real shot consumes exactly 1 bullet
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
        void Update(float dt, WorldRuntime& worldRuntime,
                    float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                    Sound& sound, bool blupiCrouching = false, bool blupiBallooned = false,
                    int blupiFacingDX = 0, int blupiFacingDZ = 0, bool blupiInvincible = false,
                    bool blupiCanGrantShield = true, bool blupiCanGrantPower = true,
                    bool blupiCanGrantCloud = true, bool blupiCanGrantHide = true,
                    bool blupiFirePressed = false, bool blupiCanFire = false,
                    bool blupiCloudActive = false, bool blupiCanGrantInvert = true,
                    // Real Sucette(26)/Drink(30) gate (plan.md `173`, found while implementing the
                    // 2-stage pickup delay): `getButtonPressedProperty()==PlayAction`,
                    // Decor.cpp:6025/6053 -- unlike Shield/Charge/Invert, which really are
                    // automatic on contact. Edge-detected (true only the frame the button is FIRST
                    // pressed, same idiom as this engine's own switch-activation check), passed in
                    // since this class has no input-device access of its own.
                    bool blupiActionPressedEdge = false,
                    // Real crate-push gate (found 2026-07-16, Decor.cpp:6130-6132) ALSO excludes
                    // every vehicle mode + Ecrase (blupiBallooned above already covers Balloon) --
                    // this class has no BlupiController access, so the caller pre-computes
                    // !IsInVehicle() && !IsEcrased() and passes it in, same convention as
                    // blupiCanGrantShield/Power/etc. Default true so every existing call site
                    // (synthetic tests included) keeps pushing crates unless it opts in.
                    bool blupiCanPushCrate = true,
                    // Blupi's Z displacement this frame (crates are pushed along X or Z, whichever
                    // he walks into them along), and whether he holds the crate in front of him to
                    // pull it (crouching, facing it: BlupiController's StopPop/Pop). A held crate
                    // follows him a cell at a time as he backs away (Decor.cpp's Pop,
                    // TestPushCaisse(bPop)).
                    float blupiMoveDZ = 0.0f, bool blupiHoldingCrate = false,
                    // Riding the jeep, the tank or the skateboard: a mine or a small enemy he runs
                    // into is destroyed and he lives (Decor.cpp:5782-5824: BlupiDead only when
                    // !m_blupiJeep && !m_blupiTank && !m_blupiSkate).
                    bool blupiArmoredVehicle = false,
                    // Flying the helicopter (in the air: the caller's blupiCanFire): Fire drops a
                    // glue ball straight down instead of the tank's shot ahead, 8 ticks after the
                    // press, Blupi held meanwhile (Decor.cpp:3058-3080 HelicoGlu, 3694-3708).
                    bool blupiDropsGlue = false,
                    // Under the drink's Hide no follower sees him: a dormant one does not wake
                    // (Decor::MoveObjectFollow returns while m_blupiHide) and an awake one stops
                    // homing (Decor.cpp:8026's `!m_blupiHide`).
                    bool blupiHidden = false,
                    // Whether he may be killed by what he touches (the original's m_blupiFocus, or
                    // he is in the air: Decor.cpp:5788-5790): false while he is busy taking a
                    // pickup, teleporting or waving goodbye. A mine or small enemy touched then
                    // still explodes, but he lives; a lethal decoration (201-203) is not met at
                    // all (Decor.cpp:6088, the whole block gated on m_blupiFocus).
                    bool blupiFocused = true,
                    // Cheering at the exit, waving goodbye, dying or gone after it: a shot (23) does
                    // not meet him then (Decor.cpp:5914-5917; BD-0062).
                    bool blupiOutOfPlay = false);

        // The crate (ObjectType12) right in front of Blupi along his facing axis and at his
        // height, close enough to hold and pull (the original's CaisseInFront()), or nullptr.
        // facingX/Z: his facing direction (sin yaw, -cos yaw). maxAhead: the farthest centre-to-
        // centre distance along the axis that counts.
        [[nodiscard]] static const MobileObjSpec* CrateInFront(const WorldRuntime& worldRuntime, float blupiX,
                                                               float blupiY, float blupiZ, float facingX,
                                                               float facingZ, float maxAhead = 2.2f) noexcept;

        // Pushing and pulling crates (CratePush.hpp has the original's rules and the 3D
        // adaptation). Called once a tick after Blupi's own step, before Update().
        struct CratePushInput
        {
            float blupiX = 0.0f, blupiY = 0.0f, blupiZ = 0.0f;
            float facingX = 0.0f, facingZ = -1.0f; // sin(yaw), -cos(yaw)
            bool onGround = true;
            // Decor.cpp:6130-6132: not in a vehicle, the balloon, squashed or swimming.
            bool canPush = true;
            bool walkingForward = false; // Up held: a march into a crate becomes Push
            bool holding = false;        // crouching, facing a crate: StopPop
            bool pullingBack = false;    // holding and Down held: Pop
            bool power = false;          // m_blupiPower: twice the speed
            // Whether Blupi's collision point (feet and head) is clear of blocks at (x, y, z): a
            // pull backs him away with the crate, which needs room behind him (other crates are
            // tested here). Empty: no blocks in the way.
            std::function<bool(float x, float y, float z)> blupiClearAt;
        };
        struct CratePushOutput
        {
            bool pushing = false;   // Push: walking into a crate (moving it or not)
            bool pulling = false;   // Pop: a held crate following him
            // Where Blupi stands against the crate he moves (centre-to-centre kContactGap
            // along the axis, on its line across it).
            bool holdBlupi = false;
            float blupiX = 0.0f, blupiZ = 0.0f;
            // His facing along the axis (unit X or Z, toward the crate).
            float faceX = 0.0f, faceZ = 0.0f;
        };
        CratePushOutput UpdateCratePush(float dt, WorldRuntime& worldRuntime, const CratePushInput& input);
        [[nodiscard]] const CratePushOutput& LastCratePush() const noexcept { return cratePush_; }

        // INFRA-007 (plan.md §7): all 14 of this class's former *ThisFrame() signals (Died
        // through DeathLockRequested below) are carried by one typed per-frame event queue
        // instead of 14 parallel booleans (+ 9 more float members for Power/Cloud/Hide's pickup
        // position, + a dozen more for Voyage/DeathLock's own payload). Migrated in 3 steps (see
        // plan.md's own writeup for the full history): step 1 was BlupiController's separate 3
        // sound-cue flags (its own EventKind/Event, unrelated to this class's); step 2 was the 11
        // simple/position-payload Kinds here (Died through InvertGranted); step 3 (this) is
        // VoyageRequested/DeathLockRequested, saved for last since they carry the largest
        // payloads and need ReplaceEvent()'s extra "only one survives per frame" care (see its
        // own comment). CrateBeingPushedThisFrame() (above) stays explicitly OUT of this queue --
        // it isn't a one-shot event at all (see its own comment).
        // Forward-declared so Event (below) can carry VoyageRequested/DeathLockRequested payload
        // fields typed by these -- full definitions live further down (VoyageKind next to
        // RequestVoyage()/BeginVoyage(), PendingDeathKind next to the death-lock trigger sites),
        // where they've always been documented, closer to their own real behavioral context.
        enum class VoyageKind : std::uint8_t;
        enum class PendingDeathKind : std::uint8_t;

        enum class EventKind : std::uint8_t
        {
            // True the frame Blupi is killed via enemy/hazard contact (see
            // the class comment above) -- InteractionSystem has no
            // access to BlupiController, so the caller is the one that
            // must actually respawn Blupi. As of 2026-07-14 the caller
            // defers this to the death-lock/life-loss-Voyage system
            // instead (see EventKind::DeathLockRequested below) and does
            // NOT consume this Kind directly any more -- kept for tests/
            // other consumers, not dead, just not read by the live game
            // loop today (a deliberate choice, not an oversight -- don't
            // "clean it up" as a side effect of an unrelated change).
            Died,
            // Wasp contact -- fires every frame Blupi overlaps a wasp, NOT
            // debounced to once per contact (the real `!m_blupiBalloon`
            // re-trigger guard lives in BlupiController::TriggerBalloon()
            // instead, which is why the caller's own TriggerBalloon() call
            // is safely idempotent against repeated events here).
            BalloonTouched,
            // True the one frame a 3/16/96/97 hazard was popped instead of
            // killing (see the class comment above) -- the caller must
            // call BlupiController::PopBalloon() itself.
            BalloonPopped,
            // Real camera shake (plan.md CAM-008/009) -- generic-hazard
            // contact-kill fires SmallShake (most types) or BigShake
            // (fish/bird, ObjectType17/20) instead; the caller triggers
            // its own CameraShake with the matching type.
            SmallShakeTriggered,
            BigShakeTriggered,
            // Real FireTank anim trigger (plan.md BLUPI-091 gap, table_blupi ID 53) -- fires the
            // one frame a Tank bullet actually launched, NOT the empty-clip "click" case (ch53) --
            // the caller fires TriggerOneShotAnim(FireTank, ...) itself.
            TankFired,
            // The helicopter's glue drop (blupiDropsGlue): Fire started it (the caller plays
            // HelicoGlu), and, kGlueDropDelaySeconds later, the ball left (the caller kicks the
            // helicopter up).
            GlueDropStarted,
            GlueDropped,
            // A patrolling enemy (ObjectType2) passed close by without touching him: the caller
            // arms his Ouf4 (BlupiController::ArmOuf()).
            EnemyNearMiss,
            // Secret power pickups (plan.md E3D-MIG-170, ObjectType25/26/30/31) -- fires the one
            // frame that pickup's real gate passed and the world object was actually removed; the
            // caller then calls the matching BlupiController::TriggerX() (its own internal gate
            // should agree, since both check the same state) and plays the real grant sound only
            // if that returns true. Shield grants instantly on contact (real, single-stage).
            ShieldGranted,
            // Power/Cloud/Hide (Sucette/Charge/Drink, plan.md `173`, 2026-07-14) are real 2-stage
            // pickups -- the world object is still destroyed HERE at contact (matching real
            // immediate ObjectDelete), and the caller plays the real immediate "grab" sound +
            // starts BlupiController::TriggerPickupFreeze() here, but the actual buff grant
            // (Power/Hide only -- Cloud's own buff already grants at contact in real source too)
            // and the "complete" sound are deferred to
            // BlupiController::ConsumePickupFreezeResolved(), which also needs the pickup's own
            // original position to respawn it (real ObjectStart(pos, type, 0) at completion) --
            // these are the ONLY 3 of these 11 kinds whose Event carries a real payload (below),
            // captured at the same moment as the event itself.
            PowerGranted,
            CloudGranted,
            HideGranted,
            // Invert/Mirror pickup (plan.md PICKUP-011, ObjectType40) -- same one-shot shape as
            // Shield above; the caller calls BlupiController::TriggerInvert() and plays channel
            // 66 only if that returns true.
            InvertGranted,
            // A pickup/death-VFX site recorded a pending Voyage this frame -- see VoyageKind's own
            // comment (further down) and RequestVoyage()/RequestClear2Ascend() for the full real
            // behavior. Unlike every Kind above, at most ONE VoyageRequested event can exist in
            // EventsThisFrame() at a time -- RequestVoyage()/RequestClear2Ascend() replace any
            // existing one rather than appending (matching the real "last request this frame wins,
            // the rest are silently dropped" behavior already documented on VoyageKind, which
            // predates this queue and was a flat single-struct overwrite before it).
            VoyageRequested,
            // One of the 4 real death-lock trigger sites living inside Update() itself fired this
            // frame -- see PendingDeathKind's own comment (further down). Same "at most one, last
            // wins" rule as VoyageRequested above, and the two ARE independent (a Clear2 outcome
            // sets both in the same frame, see PendingDeathKind's comment) -- this rule only
            // dedupes within a single Kind, never across different Kinds.
            DeathLockRequested,
        };
        struct Event
        {
            EventKind kind;
            // Only meaningful for PowerGranted/CloudGranted/HideGranted --
            // see EventKind's own comment above.
            float pickupX = 0.0f;
            float pickupY = 0.0f;
            float pickupZ = 0.0f;
            // Only meaningful for VoyageRequested -- see RequestVoyage()/RequestClear2Ascend()
            // (further down) for what each field means; mirrors the pre-queue flat pendingX_
            // members exactly, just grouped onto the event that carries them.
            VoyageKind voyageKind = VoyageKind::None;
            int voyageIconId = 0;
            bool voyageIsButtonChannel = false;
            float voyageWorldX = 0.0f, voyageWorldY = 0.0f, voyageWorldZ = 0.0f;
            float voyageFixedX = 0.0f, voyageFixedY = 0.0f;
            bool voyageWorldIsStart = false;
            bool voyageIsAscend = false;
            float voyageAscendOffsetY = 0.0f;
            // Only meaningful for DeathLockRequested -- see the death-lock trigger sites (further
            // down) for what each field means.
            PendingDeathKind deathLockKind = PendingDeathKind::Clear1;
            bool deathLockShouldRespawn = false;
        };
        [[nodiscard]] const std::vector<Event>& EventsThisFrame() const noexcept { return events_; }

        // Real crate-push loop sound (found 2026-07-16, plan.md SOUND-048's real meaning --
        // ch38, not "electric arc") -- true while Blupi is in Push (UpdateCratePush).
        // The caller compares this against its own previous-frame value to start/stop the real
        // looped ch38 sound (this class has no sound-instance-lifetime access of its own).
        // NOT part of EventsThisFrame() above -- unlike every Kind there, this is a plain
        // per-frame FACT with no memory of its own, not a discrete one-shot event (INFRA-007
        // explicitly excluded it, see plan.md §7).
        [[nodiscard]] bool CrateBeingPushedThisFrame() const noexcept { return crateBeingPushedThisFrame_; }

        // The original's m_blupiPosHelico (Decor.cpp:3910/4031/4303/4446/4507 leaving a vehicle,
        // 5780 a balloon pop, 6360 a teleport's arrival): until Blupi is more than 30 px from where
        // that happened (6341-6345) the hazards 2/3/4/16/17/20/96/97 neither kill him nor are
        // destroyed, and none pops his balloon (5767, 5786). The caller starts it for a vehicle
        // left and a teleport; a balloon pop starts it here.
        void StartContactGrace(float x, float y, float z) noexcept
        {
            contactGrace_ = true;
            graceX_ = x;
            graceY_ = y;
            graceZ_ = z;
        }
        [[nodiscard]] bool ContactGraceActive() const noexcept { return contactGrace_; }
        // Blupi touches an object of this type (object minus Blupi, cells): the original's boxes
        // (Decor::MoveObjectDetect, BD-0021), for the caller's own contacts (vehicles, BD-0024).
        [[nodiscard]] static bool BlupiTouches(GalaxyEggbert::Def::ObjectType type, float dx, float dy, float dz);
        // Decor::MockeryDetect (Decor.cpp:9518-9601; BD-0044): the taunt an enemy beside Blupi, or
        // lava or a crusher under his jump, calls for -- Mockery (63) at one ahead of him, Mockeryi
        // (64) behind him or under his jump, Mockeryp (83) at the big creature (54), None for the
        // tracking enemy (2) ahead or nothing near. The caller gates it (no cooldown, no vehicle,
        // bar, balloon or squash, in control) and arms the answer as an Ouf.
        [[nodiscard]] static GalaxyEggbert::Def::BlupiAction MockeryDetect(const WorldRuntime& worldRuntime,
                                                                          float blupiX, float blupiY, float blupiZ,
                                                                          float yaw, bool airborne);
        // The last Update() spent the action press (a dynamite stick taken): the caller's "nothing
        // to do" head shake (Non) must not follow it.
        [[nodiscard]] bool ActionPressSpent() const noexcept { return actionSpent_; }

        // Re-spawns a pickup at its original position as a static, active
        // object (real `ObjectStart(pos, type, 0)`, speed=0) -- called by
        // the game class once the real 2-stage freeze resolves for
        // Sucette/Drink/Charge (plan.md `173`).
        void RespawnPickupItem(WorldRuntime& worldRuntime, float x, float y, float z,
                                GalaxyEggbert::Def::ObjectType type);

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
        void SpawnInvertBurst(WorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                               bool isGrant);

        // Fan-hit shockwave flash (plan.md CAM-009-adjacent, ObjectType11)
        // -- a single instance spawned exactly at (x,y,z), no offset (real
        // `ObjectStart(celSwitch, ObjectType11, 0)`, `Decor.cpp:5467` --
        // the real `celSwitch` -34/-34 pixel pre-offset is a 2D
        // sprite-corner-anchoring artifact, not meaningful for this
        // engine's center-anchored billboards, same reasoning already
        // applied to every other real `ObjectStart(..., 0)` site this
        // session). Called directly by the game class at its own existing
        // Fan-hazard site (`InteractionSystem::Update()` isn't involved
        // in that check), same reasoning as `SpawnInvertBurst()` above.
        void SpawnFanHitFlash(WorldRuntime& worldRuntime, float x, float y, float z);

        // A wrecked vehicle's explosion (ObjectType9, Decor.cpp:5427: a vehicle that meets
        // water), called by the game class, which wrecks it.
        void SpawnWreck(WorldRuntime& worldRuntime, float x, float y, float z);
        // A still effect where it is (an expiring particle: the crusher's flash 90, the balloon's
        // end 91, the tentacle 53; BD-0054), called by the game class.
        void SpawnEffect(WorldRuntime& worldRuntime, GalaxyEggbert::Def::ObjectType type, float x, float y, float z);

        // The pieces that fly away (Decor::ByeByeAdd / ByeByeStep / ByeByeDraw, Decor.cpp:10058-10128;
        // BD-0049). A destroyed enemy, crate or invert potion (ObjectDelete's list: 4, 12, 16, 17,
        // 20, 40, 96, 97), a destroyed fan (ModifDecor of 126-137) and the lost helicopter
        // (ByeByeHelico, element 68) leave a copy of their picture that rises on a slowing curve,
        // (10 - phase)^1.5 px a tick for 10 ticks, then falls the same way, drifting sideways at
        // 10-19 px a tick (slowing by 1 a tick) and spinning, for 30 ticks of its phase. Galaxy
        // drifts them along X (the original's screen across) and draws them with their model.
        struct ByeByePiece
        {
            MobileObjSpec look;        // the object as it was destroyed (its picture), active
            int fanIcon = -1;          // or a fan's object-m icon
            bool helicopter = false;   // or the helicopter
            float x = 0.0f, y = 0.0f, z = 0.0f; // cells, y up
            float speedX = 0.0f;       // px a tick
            float rotationDegrees = 0.0f;
            float rotationSpeed = 0.0f; // degrees a tick
            float animationSpeed = 1.0f;
            float phase = 0.0f;
        };
        [[nodiscard]] const std::vector<ByeByePiece>& ByeByePieces() const noexcept { return byeBye_; }
        // The game class's own pieces: the helicopter lost (ByeByeHelico: rotation 7, speed 0.5),
        // a fan destroyed by touching it (ModifDecor: rotation 17).
        void ByeByeHelicopter(float x, float y, float z);
        void ByeByeFan(std::uint16_t icon, float x, float y, float z);

        // Teleporter arc (plan.md VISUAL-010, ObjectType92) -- despite
        // Def/ObjectType.hpp's own "spawned when Blupi uses a charged attack"
        // doc comment, the ONLY real spawn site is the teleporter-trigger
        // block (`Decor.cpp:5593-5606`, already this engine's own existing
        // `TriggerTeleport()` call site) -- a single instance at
        // (blupiX, blupiY+5/64, blupiZ) (real `celSwitch = (blupiPos.X,
        // blupiPos.Y-5)`, screen Y- -> world Y+, the usual sign flip), no
        // further offset (`speed=0`). Real self-delete at phase>=128, a
        // long 6.4s lifetime matching this engine's own
        // `BlupiController::kTeleportDuration` exactly -- the arc plays
        // for the whole teleport transit. Called directly by the game
        // class at its own existing `TriggerTeleport()` success site, same
        // reasoning as `SpawnFanHitFlash()` above.
        void SpawnTeleportArc(WorldRuntime& worldRuntime, float x, float y, float z);
        // A teleport's arrival (Decor.cpp:6355-6356): two ObjectType27 sparkle clusters sliding
        // +X and -X from where he lands, toward the nearest wall each way (BD-0017).
        void SpawnTeleportArrival(WorldRuntime& worldRuntime, float x, float y, float z);

        // Water splash (plan.md PICKUP-078/080, ObjectType14/35) -- real
        // `Decor::MoveObjectPlouf()`/`MoveObjectTiplouf()` (`Decor.cpp:
        // 6991-7025`), confirmed via direct source read during 2026-07-17
        // water-splash research. Both are static (no posEnd slide, real
        // `ObjectStart(pos, type, 0)`), single-instance-at-a-time (a real
        // pre-check scans for an already-active instance of the SAME type
        // and no-ops if found -- reproduced by the caller checking
        // `HasActiveObjectOfType()` before calling this, same idiom as
        // every other "only one at a time" pickup gate this session).
        // Real self-delete: Plouf phase>=14, Tiplouf phase>=6 (`Decor.cpp:
        // 8599-8622`) -- handled generically in `Update()`'s per-object
        // loop, same as every other cosmetic burst. Called directly by the
        // game class at its own water Surf/Nage transition site, same
        // reasoning as `SpawnFanHitFlash()` above.
        void SpawnWaterSplash(WorldRuntime& worldRuntime, GalaxyEggbert::Def::ObjectType type, float x, float y, float z);

        // Real pre-check for SpawnWaterSplash()'s "only one Plouf/Tiplouf
        // active at once" gate (`Decor.cpp:6993-6998`/`7007-7012`).
        [[nodiscard]] bool HasActiveObjectOfType(const WorldRuntime& worldRuntime, GalaxyEggbert::Def::ObjectType type) const;

        // Ambient rising water bubble (plan.md PICKUP-079, ObjectType15) --
        // real `Decor::MoveObjectBlup()` (`Decor.cpp:7027-7070`): scans
        // upward from (x,y,z) counting consecutive water-tile columns
        // (this engine's `BlockTypes::isWater()`, matching the real
        // "icon==91 or icon==92" check's intent), spawns a bubble that
        // rises exactly that many tiles (posEnd = start + count, real
        // `stepAdvance = ScaleTime(count*10)`, this build's already-
        // established 1:1 tick convention) and self-deletes on arrival
        // (folded into the shared `AdvancePatrolStep()`'s existing
        // ObjectType23-arrival branch, since real source treats both
        // identically at that exact junction, `Decor.cpp:8095-8098`). A
        // no-op if there's no clear water column above (count<=0), matching
        // the real guard exactly. Called periodically from the game class
        // while `BlupiController::IsNage()`, keyed off
        // `WorldRuntime::GetAnimPhase()` the same way every other real
        // `m_time %`-gated periodic effect in this engine already is.
        void SpawnWaterBubble(WorldRuntime& worldRuntime, const Worlds::World& world, float x, float y, float z);

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
        // real `GalaxyEggbert::Def::Direction::Right`. Real `m_blupiPhase` (Jeep/Tank's own
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
        void TickPollutionPuff(WorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
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
        // per this engine's own mutually-exclusive `GalaxyEggbert::Def::SecretPower` enum), so
        // one tracker suffices here too. `ResetMagicTrail()` mirrors every
        // real grant site's own `m_blupiPosMagic = m_blupiPos` (confirmed
        // at multiple real grant sites, e.g. `Decor.cpp:6022`) -- call it
        // at Shield/Power's own grant sites, same idiom as
        // `SpawnInvertBurst()`'s grant-site call. The distance toward the
        // 40px threshold is the original's Manhattan distance, over X, Y
        // and Z in 3D. Hide's own afterimage trail (`ObjectType58`, a copy of
        // his own current picture) is the host's (BD-0056), not here. Real
        // level-load/respawn resets of `m_blupiPosMagic` (several
        // additional real call sites) are also NOT modeled -- a minor,
        // accepted simplification (a stale tracker only shifts the first
        // post-respawn marker's exact trigger point, cosmetic only).
        void ResetMagicTrail(float x, float y, float z);
        void TickMagicTrail(WorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ, bool isShielded,
                             bool isPowered);

        [[nodiscard]] int TreasuresCollected() const noexcept { return treasuresCollected_; }
        [[nodiscard]] int TotalTreasures() const noexcept { return totalTreasures_ < 0 ? 0 : totalTreasures_; }
        // Testability accessor for the Blitz-emitter zap sound's lazy world
        // scan (plan.md SOUND-079/VISUAL-024) -- false before the first
        // Update() call, same "-1 = not yet scanned" convention as
        // TotalTreasures() above.
        [[nodiscard]] bool HasBlitzEmitterPair() const noexcept { return blitzEmitterPresent_ == 1; }
        [[nodiscard]] bool ExitReached() const noexcept { return exitReached_; }
        // The exit reached was the golden key (ObjectType21, the original's `m_bFoundCle`):
        // winning it completes the world (Decor::OpenGoldsWin()) and returns to the main hub.
        [[nodiscard]] bool ExitWasGoldenKey() const noexcept { return exitWasGoldenKey_; }

        // Opens the door block at raw cell (gx, gy, gz): the block becomes air and a slide
        // (ObjectType22, sound 33) carries its icon up one cell, as Decor::OpenDoor() does. Used
        // for the hubs' doors when a hub loads (Decor::AdaptDoors()).
        static void OpenDoor(WorldRuntime& worldRuntime, int gx, int gy, int gz, Sound& sound);
        [[nodiscard]] int Key1Count() const noexcept { return keys1_; }
        [[nodiscard]] int Key2Count() const noexcept { return keys2_; }
        [[nodiscard]] int Key3Count() const noexcept { return keys3_; }
        [[nodiscard]] int LifeEggCount() const noexcept { return lifeEggCount_; }

        // Real mobile-eggbert m_nbVies (lives) tracking (2026-07-11, plan.md
        // E3D-MIG-130): starts at 3 (GameData default,
        // mobile-eggbert-reference/11-save-and-progression.md), incremented
        // by egg pickups while under MAX_EGG_COUNT=10 lives (see the ObjectType6
        // case in Update(): the gate is this count, as the original's m_nbVies;
        // lifeEggCount_ above only tallies the eggs taken). Call LoseLife() on any death; a caller decides which sound
        // channel fits the specific death cause (channel 8 generic/fall/
        // lava/electric, 26 drowning, 51 glue per
        // mobile-eggbert-reference/07-sounds.md) and plays it itself --
        // LoseLife() only manages the counter.
        void LoseLife();
        [[nodiscard]] int Lives() const noexcept { return lives_; }
        // Restores a saved lives count (2026-07-13, plan.md MENU-040..045,
        // real Resume phase) -- ResumeContinue's own real equivalent, via
        // SaveData, since this engine has no `GameData`-style per-gamer
        // slot to read lives from directly.
        void SetLives(int lives) noexcept { lives_ = lives; }
        // The typed cheats' inventory (Decor::CheatAction, OR-0007): tenbombs m_blupiPerso = 10,
        // birdlime m_blupiBullet = 10, udynamite m_blupiDynamite = 1, weelkeys (and opendoors)
        // every key.
        void CheatSetPerso(int count) noexcept { persoCount_ = count; }
        void CheatSetBullets(int count) noexcept { bulletCount_ = count; }
        void CheatSetDynamite(int count) noexcept { dynamiteCount_ = count; }
        void CheatAllKeys() noexcept
        {
            keys1_ = std::max(keys1_, 1);
            keys2_ = std::max(keys2_, 1);
            keys3_ = std::max(keys3_, 1);
        }
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
        // approval for) -- it renders via whatever ObjectIcons::GetObjIcon()
        // already returns for type 56, a documented simplification. The 9
        // real blast ticks/offsets (Decor.cpp ~8258-8290, NOT just the
        // reference doc's rounded summary) and the real destructible-type
        // list (~9102-9132) are handled inside Update() itself, not exposed
        // here.
        [[nodiscard]] int DynamiteCount() const noexcept { return dynamiteCount_; }
        // blupiCanUseHands (found 2026-07-16, Decor.cpp:4792-4794): real placement gate ALSO
        // excludes every vehicle mode (Helico/Over/Jeep/Tank/Skate) plus Balloon/Ecrase --
        // unlike switch activation, Helicopter is NOT exempt here. This class has no
        // BlupiController access, so the caller computes and passes this in, same convention as
        // blupiCanGrantShield/Power/etc.
        bool PlaceDynamite(WorldRuntime& worldRuntime, float x, float y, float z, bool grounded,
                            bool blupiCanUseHands = true);

        // Perso (decoy statue) placement/pickup (plan.md HUD-017, real
        // Decor.cpp ~4818-4841 place, ~6088-6101 pickup start, ~10291-10294
        // pickup completion). A placeable `ObjectType200` decoy (same
        // `blupi.png` look as Blupi himself) -- once placed, the bulldozer
        // and the shooters (4, 32, 33) that meet it are destroyed with it
        // (MovePersoDetect, BD-0028); here the place/retrieve/count loop. Real
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
        // blupiCanUseHands: same real vehicle/Balloon/Ecrase gate as PlaceDynamite() above
        // (Decor.cpp:4792-4794 covers BOTH dynamite and Perso placement under the one condition,
        // an `if`/`else if`) -- ONLY affects the placement branch below, NOT the pickup branch
        // above it, which real source gates solely on `m_blupiFocus` (Decor.cpp:6088-6101, no
        // vehicle clause at all) -- picking up an already-placed decoy must still work while
        // riding/ballooned/squashed.
        [[nodiscard]] int PersoCount() const noexcept { return persoCount_; }
        bool TryPerso(WorldRuntime& worldRuntime, float x, float y, float z, bool grounded,
                      bool blupiCanUseHands = true);

        // Voyage (plan.md `158`, real `Decor::VoyageInit`/`VoyageStep`/
        // `VoyageDraw`, `Decor.cpp:10141-10348`) -- real pickups do NOT
        // apply their reward immediately: the world object is deleted on
        // contact, but the actual counter increment (plus e.g. re-scanning
        // treasure-gated doors) only happens once a short 2D "fly to HUD
        // icon" animation completes. Previously modeled as immediate
        // application (a documented simplification); this class now
        // matches the real deferred timing. Real state: which reward, a
        // start/end point in mobile-eggbert's 640x480 reference screen
        // space (the same space `Hud` already uses), a phase counter,
        // and `total = (|dx|+|dy|)/10` ticks (real integer-truncated
        // Manhattan screen distance). Only one voyage is active at a time;
        // starting a new one force-completes the previous one's reward
        // immediately (real `VoyageInit`'s own
        // `if (m_voyageIcon != -1) { phase = total; Step(); }`).
        //
        // This class has no camera access, so it can't project a pickup's
        // 3D world position into 2D screen space itself. Instead, pickup
        // sites record a "pending" voyage request (a VoyageRequested event +
        // whichever ONE endpoint still needs projecting, plus the other,
        // already-resolved endpoint) via `RequestVoyage()`; the caller
        // (`GalaxyEggbertGame.cpp`, which has `camera_`) looks for a
        // VoyageRequested event right after `Update()` returns,
        // projects the world point via `Hud::ProjectWorldToHudSpace()`,
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
        // animation types (`GalaxyEggbert::Def::BlupiAction::Clear2`/`Clear3` -- Clear1's own
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

        // A VoyageRequested event's payload fields (Event, above) mean:
        //   voyageWorldX/Y/Z, voyageFixedX/Y -- the world-position endpoint (still needing
        //     projection) and the already-resolved fixed HUD-space endpoint.
        //   voyageWorldIsStart -- true: the projected world point is the voyage's START (every
        //     pickup -- flies FROM the pickup's own position TO a fixed HUD point). False: the
        //     world point is the voyage's END (real door-unlock key-consumption flourish only --
        //     flies FROM the fixed HUD key position TO the door's own world position).
        //   voyageIsAscend -- true only for the generic-hazard-contact Clear2 coinflip site
        //     (inside Update(), no camera access -- unlike Clear2/Clear3's OTHER 2 real trigger
        //     sites, fall-off-world and Lava, which live directly in `GalaxyEggbertGame.cpp`
        //     and so call `BeginVoyage()` straight away, no pending round-trip needed). When
        //     true, the caller computes end = (projectedX, projectedY - voyageAscendOffsetY)
        //     instead of reading voyageFixedX/Y (which are unused/stale in this mode).

        // Death-lock request (death-lock/life-loss-Voyage follow-up) from the 4 real trigger sites
        // living inside Update() itself, which has no `BlupiController&`/camera access (every
        // OTHER real cause -- fall/Lava/Saw/Blitz/Drown/Fan -- is checked directly in
        // `GalaxyEggbertGame.cpp`, which already has `blupi_` in scope and calls
        // `blupi_.TriggerDeathLock()` straight away): generic-hazard-contact (Clear1/Clear2 real
        // 50/50 coinflip, `shouldRespawn=false` -- confirmed no `m_blupiRestart=true` near
        // Decor.cpp:5782-5815), dynamite blast (deterministic Clear1, `shouldRespawn=false` --
        // confirmed no `m_blupiRestart=true` near Decor.cpp:9168-9173 either, a correction to an
        // earlier assumption that only Fan/generic-hazard lacked it), and fired-projectile-contact
        // (ObjectType23)/large-creature-grab (ObjectType54), both real Glu, `shouldRespawn=true`
        // (confirmed `m_blupiRestart=true` at Decor.cpp:5879/5927). A LOCAL enum, deliberately NOT
        // shared with `BlupiController::DeathCause` (this class stays fully decoupled from it,
        // same reasoning as every other pending signal here) -- the caller maps this to the real
        // `DeathCause` when calling `TriggerDeathLock()`. This is a SEPARATE Kind from
        // VoyageRequested above -- a Clear2 outcome sets BOTH in the same frame (the cosmetic
        // Clear2Ascend request via RequestClear2Ascend() below, AND this one), consumed
        // independently by the caller.
        enum class PendingDeathKind : std::uint8_t { Clear1, Clear2, Glu };

        // Called by the game class once it has resolved both endpoints
        // (projecting whichever one was still a world position). Force-
        // completes any voyage already in flight first (see class comment).
        // worldAnchorX/Y/Z is Blupi's own 3D world position at the moment
        // the voyage started -- unused by every pickup kind (defaulted to
        // 0), but needed by Clear3Ascend to anchor its own real continuous
        // world-space puff-particle spawn (see TickVoyage()'s own comment).
        void BeginVoyage(WorldRuntime& worldRuntime, VoyageKind kind, int iconId, bool isButtonChannel,
                          float startX, float startY, float endX, float endY, Sound& sound,
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
        void SpawnSawDeathBurst(WorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                                 Sound& sound);

        // Draw-side state for `Hud::Draw()` -- current interpolated
        // position in the same 640x480 reference space.
        [[nodiscard]] bool VoyageActive() const noexcept { return voyageKind_ != VoyageKind::None; }
        [[nodiscard]] int VoyageIconId() const noexcept { return voyageIconId_; }
        // The Clear2 death's soul (the angel, element 230-241) on its way up: where it set off
        // (the death's world point) and how far it is, 0..1 of its 300 px (OR-0047: drawn in 3D).
        [[nodiscard]] bool VoyageIsSoul() const noexcept { return voyageKind_ == VoyageKind::Clear2Ascend; }
        [[nodiscard]] float VoyageProgress() const noexcept
        {
            return voyageTotal_ > 0.0f ? std::clamp(voyagePhase_ / voyageTotal_, 0.0f, 1.0f) : 1.0f;
        }
        [[nodiscard]] float VoyageWorldAnchorX() const noexcept { return voyageWorldAnchorX_; }
        [[nodiscard]] float VoyageWorldAnchorY() const noexcept { return voyageWorldAnchorY_; }
        [[nodiscard]] float VoyageWorldAnchorZ() const noexcept { return voyageWorldAnchorZ_; }
        [[nodiscard]] bool VoyageIsButtonChannel() const noexcept { return voyageIsButton_; }
        // The life-loss voyage flies the lives row's small Blupi (the blupi.png sheet).
        [[nodiscard]] bool VoyageIsBlupiChannel() const noexcept { return voyageKind_ == VoyageKind::LifeLoss; }
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
        // per-frame test -- see InteractionSystem.cpp's own comment).
        // True this frame if Blupi (at the blupiX/Y/Z passed into Update())
        // was standing on an active platform lift BEFORE it took this
        // frame's patrol step. RideDeltaX/Z is the lift's own horizontal
        // displacement this tick (+ the real constant conveyor nudge for
        // types 47/48, plan.md E3D-MIG-154) -- a delta, not an absolute
        // position, so the caller applies it on top of Blupi's own
        // already-Step()'d position rather than overriding his own
        // movement input. RideStandY is an absolute snap (matching the
        // real source's own "correct Y drift every frame" approach): the
        // caller should pass both into BlupiController::RideLift().
        [[nodiscard]] bool IsRidingLift() const noexcept { return ridingLift_; }
        [[nodiscard]] float RideDeltaX() const noexcept { return rideDeltaX_; }
        [[nodiscard]] float RideDeltaZ() const noexcept { return rideDeltaZ_; }
        [[nodiscard]] float RideStandY() const noexcept { return rideStandY_; }
        // The ridden lift's middle after its step, and whether it is a conveyor (47/48): his
        // vertigo at its edge (Decor::AscenseurVertigo, BlupiController::SetLiftUnder; BD-0047).
        // Decor::SetMoveTraj / SetBalleTraj (Decor.cpp:8150-8156, 7638-7642; BD-0048): every cell an
        // object has been in since the mission began (a lift: the cell over it, where one rides
        // it) and every shot's whole path. The safe position he respawns at is never taken in one
        // (IsMoveTraj / IsBalleTraj in the FIFO's gate, Decor.cpp:6471).
        [[nodiscard]] bool IsTrajectoryCell(const WorldRuntime& worldRuntime, float x, float y, float z) const;
        // Decor::MoveAscenseurDetect(pos, OVERHEIGHT) (Decor.cpp:3941, 9815-9842; BD-0052): a lift's
        // top from 18 px over his feet to 79 under them, within 50 px across: the overcraft may
        // climb over it.
        [[nodiscard]] static bool LiftWithinOverHeight(const WorldRuntime& worldRuntime, float x, float y, float z);
        [[nodiscard]] float RideLiftX() const noexcept { return rideLiftX_; }
        [[nodiscard]] float RideLiftZ() const noexcept { return rideLiftZ_; }
        [[nodiscard]] bool RideLiftIsConveyor() const noexcept { return rideLiftConveyor_; }

        // Hidden cheat menu (plan.md CHEAT-001..009, 2026-07-13), verified
        // directly against `Decor::CheatAction(Tables::CheatCodes)` --
        // overturned several draft assumptions in this repo's own earlier
        // (unverified) plan.md checklist, see each method's own comment
        // for the correction. Real dispatch is one-shot (pressing a cheat
        // button immediately closes the overlay and applies the effect,
        // no confirmation) -- these methods mirror that: called once, not
        // every frame, from the caller's own button-press handler.
        //
        // Cheat1 "OpenDoors" is the host's (GalaxyEggbertGame::AdaptHubDoors(), Decor::
        // AdaptDoors() with m_bCheatDoors): it opens the hubs' doors only.

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
        // the caller only triggers its own CameraShake on a real
        // effect, same idiom as every other *ThisFrame() signal here.
        bool CheatCleanAll(WorldRuntime& worldRuntime);

        // Cheat8 "AllTreasure": every active ObjectType5 is collected at
        // once (deactivated, `treasuresCollected_` incremented per
        // treasure found), then the existing treasure-gated door scan
        // (the same one a real pickup already triggers) opens every door
        // now satisfied -- reuses `OpenDoorAt()`/the real Channel11
        // fanfare, not a separate mechanism.
        void CheatAllTreasure(WorldRuntime& worldRuntime, Sound& sound);

        // Cheat9 "EndGoal": real effect ALWAYS teleports Blupi to the
        // exit (`ObjectType7`), but only actually triggers a real win if
        // `treasuresCollected_ >= totalTreasures_` at that moment --
        // otherwise Blupi just arrives at the exit without winning (an
        // earlier draft description of this cheat as an unconditional
        // "win immediately" was wrong). Returns the exit's real position
        // via out-params (false if no active exit exists in the world);
        // the caller teleports Blupi there itself (this class has no
        // access to BlupiController) -- the EXISTING exit-contact
        // detection in Update() then naturally re-evaluates the real
        // win/no-win gate on the very next frame, so no separate
        // win-forcing logic is needed here at all.
        [[nodiscard]] bool CheatFindExit(const WorldRuntime& worldRuntime,
                                         float& outX, float& outY, float& outZ) const;

    private:
        bool ridingLift_ = false;
        float rideDeltaX_ = 0.0f;
        float rideDeltaZ_ = 0.0f;
        float rideStandY_ = 0.0f;
        float rideLiftX_ = 0.0f;
        std::unordered_set<std::uint32_t> trajectoryCells_; // IsTrajectoryCell()
        std::vector<ByeByePiece> byeBye_;                   // ByeByePieces()
        float byeByeTicks_ = 0.0f;                          // the 20 Hz ticks not yet stepped
        // ObjectDelete: the object goes; one of the listed types leaves a piece (ByeByePiece).
        void Retire(MobileObjSpec& obj);
        void ByeByeAdd(ByeByePiece piece, float rotationSpeed, float animationSpeed);
        void TickByeBye(float dt);
        float rideLiftZ_ = 0.0f;
        bool rideLiftConveyor_ = false;

        int treasuresCollected_ = 0;
        int totalTreasures_ = -1; // computed lazily on first Update() call
        // -1 = not yet scanned, 0 = scanned/none found, 1 = scanned/found.
        // Real `Decor::BlitzActif()` (plan.md SOUND-079/VISUAL-024, found
        // 2026-07-17) plays the zap sound (ch69) per visible Blitz(305) tile
        // that has a BlitzEmitter(304) tile directly above it, on a fixed set
        // of ticks within a 100-tick cycle. Only a visible column is heard
        // (BitzActif runs from the drawing of the screen's cells), so the
        // zap is played from each column's place through Sound::PlayAt, heard
        // within the original's reach of Blupi (BD-0076); the columns are
        // found once (the world's emitters do not move).
        int blitzEmitterPresent_ = -1;
        std::vector<std::array<float, 3>> blitzColumns_; // render cells of each bolt under an emitter
        bool exitReached_ = false;
        bool exitWasGoldenKey_ = false;
        // The original's m_goalPhase (Decor.cpp:6158-6189): ticks until an exit he stands on is checked
        // again; 0 when he is off every exit.
        float exitGoalTicks_ = 0.0f;
        int keys1_ = 0;
        int keys2_ = 0;
        int keys3_ = 0;
        // Eggs taken so far (the autoplay's tally). The MAX_EGG_COUNT=10 gate is on lives_, the
        // original's m_nbVies, which an egg raises (Decor.cpp:6007, 10264).
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
        // This-frame pending request (consumed by the game class) -- now carried as a
        // VoyageRequested/DeathLockRequested entry in events_ (INFRA-007, plan.md §7) instead of
        // this flat member block; ReplaceEvent() (declared below) preserves the exact "only one
        // pending request survives per frame" behavior this flat representation used to give for
        // free via plain assignment.
        // First randomness needed anywhere in this engine's gameplay code
        // (real `Decor::BlupiDead`'s own `m_random`, Decor.cpp:6551) --
        // seeded from real entropy since the real coinflip this ports is
        // genuinely non-deterministic (cosmetic-only: it never affects
        // `lives_`/reward state, only whether the ascend VFX/sound plays).
        std::mt19937 rng_{std::random_device{}()};

        void TickVoyage(float dt, WorldRuntime& worldRuntime, Sound& sound);
        void ApplyVoyageReward(WorldRuntime& worldRuntime, Sound& sound);
        // Removes any existing EventsThisFrame() entry of the same Kind before appending newEvent
        // -- used by RequestVoyage()/RequestClear2Ascend() and the 4 death-lock trigger sites
        // below (INFRA-007, plan.md §7) to preserve the exact "only one such request can be
        // pending at a time, a later one silently replaces an earlier same-frame one" behavior the
        // pre-queue flat member representation gave for free via plain assignment (see
        // EventKind::VoyageRequested/DeathLockRequested's own comments), and for EnemyNearMiss
        // (one a frame, however many enemies pass). NOT used for the other Kinds -- those never
        // had a single-flat-member representation to begin with.
        void ReplaceEvent(EventKind kind, Event newEvent);
        // Records a this-frame voyage request (see the public API comment
        // above for the world/fixed-point split). Called from the pickup
        // switch, TryPerso(), and the key-gated-door block.
        void RequestVoyage(VoyageKind kind, int iconId, bool isButtonChannel, float worldX, float worldY,
                            float worldZ, float fixedX, float fixedY, bool worldIsStart);
        // Records a this-frame Clear2Ascend request from the generic-
        // hazard-contact site (the only Clear2 trigger site inside
        // Update() itself, with no camera access -- see
        // EventKind::VoyageRequested's own comment).
        void RequestClear2Ascend(float worldX, float worldY, float worldZ);
        // INFRA-006 pilot (plan.md §7): looks up `obj.type` in
        // kSecretPowerPickupHandlers (InteractionSystem.cpp's own
        // anonymous namespace) and applies its rule -- replaces what used
        // to be 5 near-identical `case` bodies in Update()'s own switch
        // with one shared dispatch. No-op if `obj.type` isn't one of the 5
        // handled types (only ever called from those 5 case labels, see
        // the call site's own comment).
        void TryGrantSecretPowerPickup(MobileObjSpec& obj, bool blupiCanGrantShield, bool blupiCanGrantPower,
                                        bool blupiCanGrantCloud, bool blupiCanGrantHide, bool blupiCanGrantInvert,
                                        bool blupiActionPressedEdge);
        // INFRA-006 3rd family (plan.md §7): looks up `obj.type` in
        // kBasicPickupHandlers (InteractionSystem.cpp's own anonymous
        // namespace) and applies its rule -- replaces what used to be 5
        // near-identical `case` bodies in Update()'s own switch with one
        // shared dispatch. No-op if `obj.type` isn't one of the 5 handled
        // types (only ever called from those 5 case labels, see the call
        // site's own comment). Dynamite's gate is checked at its call site.
        void TryCollectBasicPickup(MobileObjSpec& obj, std::vector<MobileObjSpec>& pendingSpawns);
        // Real `Decor::VoyageDraw`'s icon==40 puff spawn (Decor.cpp:
        // 10331-10348), called once per TickVoyage() while Clear3Ascend is
        // active (see TickVoyage()'s own comment).
        void SpawnLavaAscendPuff(WorldRuntime& worldRuntime);
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
        static constexpr float kGlueDropDelaySeconds = 8.0f / 20.0f;
        float glueDropTimer_ = 0.0f; // > 0: a helicopter glue ball about to leave
        static constexpr int kPersoCap = 5; // real m_blupiPerso cap
        int persoCount_ = 0;
        bool crateBeingPushedThisFrame_ = false; // set by UpdateCratePush(); NOT part of events_ (see its own comment)
        bool contactGrace_ = false;
        bool actionSpent_ = false;
        float graceX_ = 0.0f, graceY_ = 0.0f, graceZ_ = 0.0f;
        int crateAction_ = 0;            // 0 none, 1 Push, 2 Pop: the action the speed ramps in
        float crateActionSeconds_ = 0.0f; // time in that action (the original's m_blupiPhase)
        CratePushOutput cratePush_;
        std::vector<Event> events_; // cleared and refilled every Update() call (INFRA-007)
    };
}
