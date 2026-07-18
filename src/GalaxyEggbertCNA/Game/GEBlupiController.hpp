#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <array>
#include <cmath>
#include <cstdint>

namespace GalaxyEggbert::CNA
{
    // Invisible, collision-only movement placeholder for early debug testing
    // of 3D navigation (plan.md E3D-MIG-060). Grid-based collision against
    // the loaded World: stands on/climbs terrain up to a 1-block step
    // (step-up traversal — one of CLAUDE.md's allowed natural 3D
    // adaptations), falls under gravity otherwise, and blocks movement into
    // taller obstacles. Tank controls (turn + forward/back), matching
    // GalaxyEggbertSimple3D's already-shipped scheme — see Step() below.
    //
    // No 3D Blupi model exists yet (2026-07-05) — the real in-world sprite
    // (E3D-MIG-061..064) waits for it. In the meantime this tracks just
    // enough state (facing yaw, a coarse Stop/March/Jump/Down/Up animation
    // state) to drive a first-person camera and a 2D screen-corner
    // animation indicator — see GalaxyEggbertCnaGame. The frame tables
    // mirror GalaxyEggbertSimple3D's already-approved GEBlupiController.cpp
    // (galaxy-eggbert's own code, not a fresh mobile-eggbert transcription).
    //
    // Deliberately engine-agnostic (only depends on GalaxyEggbert::Worlds)
    // so Step() can be scripted/tested without a live window or keyboard —
    // see tools/VerifyBlupiMovement.cpp. The CNA target reads Keyboard
    // state itself and calls Step().
    class GEBlupiController
    {
    public:
        static constexpr float kMoveSpeed = 5.5f; // matches Simple3D::GEBlupiController
        static constexpr float kTurnSpeed = 3.14159265f; // rad/s (180 deg/s, matches Simple3D)
        static constexpr float kJumpSpeed = 12.0f; // real "-16, no Power, clear headroom" baseline -- see kJumpSpeedPowered's own comment

        // Jump-height headroom modulation (plan.md TILE-041, real
        // Decor::IsNormalJump(), verified against mobile-eggbert-reference/
        // 12-hazards-and-interactables.md's "Jump physics" section --
        // corrected 2026-07-14, the old checklist description of this as a
        // special "tile that forces a jump" was wrong). Real 4 magnitudes:
        // clear-headroom+noPower=-16 (kJumpSpeed itself, the existing
        // baseline/anchor per kSpringBounceHeld/NotHeld's own comment),
        // clear-headroom+Power=-26, blocked-headroom+noPower=-12,
        // blocked-headroom+Power=-16 (coincidentally the same magnitude as
        // clear-headroom+noPower -- a real coincidence in mobile-eggbert's
        // own numbers, not a bug here). Same proportional-anchoring
        // technique as kSpringBounceHeld/NotHeld above.
        static constexpr float kJumpSpeedPowered = kJumpSpeed * (26.0f / 16.0f);
        static constexpr float kJumpSpeedReduced = kJumpSpeed * (12.0f / 16.0f);
        static constexpr float kJumpSpeedReducedPowered = kJumpSpeed * (16.0f / 16.0f);

        // Real ground-jump gate (Decor.cpp:2913-2947, found 2026-07-16 while
        // researching E3D-MIG-065) also excludes Helico/Over/Balloon/Ecrase/
        // Jeep/Tank/Nage/Surf/Suspend entirely -- only Skateboard is allowed
        // through, with its OWN distinct velocity (-17 Power/-13 noPower),
        // not the headroom-modulated values above. This engine's jump gate
        // previously had no vehicle-mode check at all (the same "predates
        // vehicle modeling" gap already found/fixed this session for
        // Sucette/Drink/Charge/TriggerTeleport) -- Jeep/Tank could
        // incorrectly launch a normal Blupi jump; Skateboard used the wrong
        // (headroom-based) magnitude instead of its own. Same proportional-
        // anchoring technique as kJumpSpeedPowered/Reduced above.
        static constexpr float kSkateboardJumpSpeed = kJumpSpeed * (13.0f / 16.0f);
        static constexpr float kSkateboardJumpSpeedPowered = kJumpSpeed * (17.0f / 16.0f);

        static constexpr float kGravity   = 25.0f;
        static constexpr float kFallLimit = -10.0f;
        static constexpr float kStepLimit = 1.0f;
        static constexpr float kAnimFps   = 8.0f; // matches Simple3D::GEBlupiController

        // Crusher squash state (plan.md E3D-MIG-143, `m_blupiEcrase` per
        // mobile-eggbert-reference/12-hazards-and-interactables.md, verified
        // directly against Decor.cpp:5549-5597/5180-5197). Real duration:
        // m_blupiTimeShield set to 100, decremented every Config::ScaleTime(2)
        // ticks -> 200 normalized ticks at the 20Hz reference rate = 10s;
        // modeled here as a plain 10s real-time countdown rather than the
        // tick-based decrement, same total duration. kEcraseSpeedMultiplier
        // is an approximation, not a transcribed constant -- the real
        // table (10-blupi-mechanics.md's vehicle/mode movement table) gives
        // a "grounded x4 of m_blupiSpeedX" figure that doesn't cleanly
        // convert to a fraction of kMoveSpeed without further research.
        static constexpr float kEcraseDuration = 10.0f;
        static constexpr float kEcraseSpeedMultiplier = 0.5f;

        // Wasp "balloon" status (plan.md E3D-MIG-135, `m_blupiBalloon` per
        // mobile-eggbert-reference/04-enemy-behavior.md's ObjectType44
        // section, verified directly against Decor.cpp:5826-5863 (trigger)
        // and the recovery block right before Decor.cpp:5549 (same
        // m_blupiTimeShield=100/decremented-every-ScaleTime(2)-ticks
        // pattern as Crusher -- same real 10s duration, NOT the "100-tick"
        // read literally as 100 raw ticks). kBalloonGravityMultiplier is an
        // approximation of "Blupi floats rather than dying" -- the real
        // source sets this up as a status flag other code branches key off
        // of; the specific floaty-fall-speed feel isn't itself transcribed
        // from a located constant.
        static constexpr float kBalloonDuration = 10.0f;
        static constexpr float kBalloonGravityMultiplier = 0.2f;

        // Spring bounce (plan.md E3D-MIG-145, icon 211 = BlockTypes::Spring,
        // verified directly against Decor.cpp:2835-2911/7312-7320). Real
        // bounce velocity depends on whether Jump is held at the moment of
        // contact and whether the "Power" (SecretPower) state is active:
        // held+Power=-25, held+noPower=-19, not-held+Power=-16,
        // not-held+noPower=-10 (negative=upward in the real source's own
        // convention) -- Power isn't modeled yet (Phase 17), so only the
        // two noPower magnitudes apply here. kJumpSpeed itself has no
        // documented real-value derivation (tuned by feel, not
        // transcribed), so these preserve the REAL PROPORTION between the
        // spring's two magnitudes and the real baseline ground-jump
        // velocity (`IsNormalJump`'s held+noPower=-16) applied on top of
        // this engine's own already-tuned kJumpSpeed -- the same technique
        // GalaxyEggbertSimple3D's own stomp bounce already used
        // (`kJumpSpeed * 0.65f`), not an independent re-derivation.
        static constexpr float kSpringBounceHeld = kJumpSpeed * (19.0f / 16.0f);
        static constexpr float kSpringBounceNotHeld = kJumpSpeed * (10.0f / 16.0f);

        // Teleporter transit duration (plan.md E3D-MIG-147, icons 330-333,
        // verified directly against Decor.cpp:6349, `Config::ScaleTime(128)`
        // -- 128 ticks at the real 20Hz reference rate = 6.4s exactly, a
        // direct transcription, not an approximation).
        static constexpr float kTeleportDuration = 6.4f;

        // Real "Bye" farewell freeze (plan.md BLUPI-049, found 2026-07-17,
        // verified directly against Decor.cpp:6436: `m_blupiPhase ==
        // Config::ScaleTime(30)` -- 30 ticks at the real 20Hz reference
        // rate = 1.5s exactly, a direct transcription). Real trigger is
        // narrow: ONLY stepping onto a hub-screen world-select marker
        // (`Decor::IsWorld()`, `Decor.cpp:5476-5488`) -- NOT the level-
        // exit-reached path, PauseBack, or PauseRestart, all of which
        // change level instantly in real source with no Bye at all. Real
        // behavior during the freeze: `m_blupiFocus=false` (input frozen,
        // modeled below the same way Teleporte freezes everything),
        // `m_blupiFront=true` (turns to face the camera -- the caller sets
        // this via SetYaw() once at trigger time, since only it knows
        // where the camera is). No dedicated per-frame "wave" sprite table
        // exists in real source (checked Tables.cpp/Decor.cpp directly) --
        // the "farewell" is really just standing still, turned front-on,
        // for 1.5s, so GetAnimIcon() below falls through to the same
        // static Stop-pose already used for DeathLocked/PickupBusy.
        static constexpr float kByeDuration = 1.5f;

        // Real `table_blupi` frameCount/20.0f durations for the one-shot
        // action animations below (Switch 10 frames, TakeDynamite 18,
        // PutDynamite 26), passed to TriggerOneShotAnim() by the caller.
        static constexpr float kSwitchDuration = 10.0f / 20.0f;
        static constexpr float kTakeDynamiteDuration = 18.0f / 20.0f;
        static constexpr float kPutDynamiteDuration = 26.0f / 20.0f;

        // Water breath gauge (plan.md E3D-MIG-148, `m_blupiLevel`, verified
        // against mobile-eggbert-reference/12-hazards-and-interactables.md's
        // "Water depth state machine" section): starts at 100, ticks down by
        // 1 every `Config::ScaleTime(5)` ticks -- 5 ticks at the real 20Hz
        // reference rate = 0.25s/level, a direct transcription (same
        // ScaleTime-at-20Hz-baseline technique already used for
        // kTeleportDuration above), giving the real ~25s (100 x 0.25s)
        // maximum submersion time. Only ticks while genuinely submerged
        // (Nage) -- standing at the surface (Surf) does not consume it, and
        // it resets to full whenever Nage ends (matches the real "gauge
        // hidden" behavior on resurfacing, not a persisted shared resource
        // across dives).
        static constexpr int kWaterGaugeMax = 100;
        static constexpr int kWaterGaugeRedThreshold = 25;
        static constexpr float kWaterGaugeTickSeconds = 5.0f / 20.0f;

        // Nage (fully submerged) gravity -- an approximation (unlike
        // kWaterGaugeTickSeconds above, no exact "buoyancy" constant is
        // documented in the reference), giving a slow, floaty sink instead
        // of a normal free-fall drop, the same approximation shape already
        // used for kBalloonGravityMultiplier. jumpPressed while Nage applies
        // a modest constant upward kick (kSwimUpSpeed, also not a
        // transcribed value -- real mobile-eggbert's continuous 2D swim
        // input has no direct equivalent to derive an exact constant from)
        // instead of a normal ground jump, a natural adaptation for
        // "swimming up" rather than "jumping off solid ground". The real
        // "Jump near the surface launches you clear of the water" nuance
        // (-16/-12, gated on a specific sub-tile depth band) is NOT modeled
        // -- no sub-tile position exists in this engine's single-point
        // collision, same simplification already applied to spikes/fans.
        static constexpr float kNageGravityMultiplier = 0.3f;
        static constexpr float kSwimUpSpeed = kJumpSpeed * 0.4f;

        // Secret powers (plan.md E3D-MIG-170/172, real m_blupiShield/Power/
        // Cloud/Hide + shared m_blupiTimeShield, verified directly against
        // Decor.cpp ~5071-5137/~6014-6087, NOT the reference doc's own
        // speculative "Sp0-Sp7 tile icons" guess -- that guess is WRONG,
        // see GetSecretPower()'s own comment for the correction. Only one
        // is ever active at a time (matches def/SecretPower.hpp's own
        // documented invariant); all 4 start their shared 100-level gauge
        // at the same value but tick down at their OWN real rate, giving
        // 4 different real durations despite the same start value:
        // Shield every ScaleTime(5)=0.25s/level (25s total), Power every
        // ScaleTime(3)=0.15s/level (15s), Cloud/Hide every ScaleTime(4)=
        // 0.2s/level (20s each) -- all 4 are direct transcriptions, not
        // approximations. Real per-power warning sounds fire at the exact
        // real remaining-level thresholds (Shield@10, Power@20, Cloud@25,
        // Hide@20).
        static constexpr int kSecretPowerMax = 100;
        static constexpr float kShieldTickSeconds = 5.0f / 20.0f;
        static constexpr float kPowerTickSeconds = 3.0f / 20.0f;
        static constexpr float kCloudTickSeconds = 4.0f / 20.0f;
        static constexpr float kHideTickSeconds = 4.0f / 20.0f;
        static constexpr int kShieldWarnLevel = 10;
        static constexpr int kPowerWarnLevel = 20;
        static constexpr int kCloudWarnLevel = 25;
        static constexpr int kHideWarnLevel = 20;

        // Invert/Mirror (plan.md PICKUP-011, real m_blupiInvert, ObjectType40,
        // verified against mobile-eggbert-reference/13-object-pickups.md's
        // "Mirror/Invert" section, Decor.cpp ~6040-6052). Independent of the
        // 4 powers above (its own separate flag/gauge, not part of the
        // mutually-exclusive SecretPower slot) -- real gate is only
        // `!m_blupiHide`, so it coexists with Shield/Power/Cloud and even
        // active vehicle rides. Shares the same 100-level gauge shape and
        // Power's own real ScaleTime(3)=0.15s/level tick rate (a direct
        // transcription, "tying with Power for the shared-timer buffs'
        // fastest decrement" per the reference doc), but has NO warning
        // stage (unlike the 4 powers above) -- confirmed in the same doc.
        static constexpr int kInvertMax = 100;
        static constexpr float kInvertTickSeconds = 3.0f / 20.0f;

        // Suspended/hanging bar-and-rope movement mode (plan.md TILE-045,
        // real Decor::GetTypeBarre()/m_blupiSuspend, verified against
        // mobile-eggbert-reference/10-blupi-mechanics.md's "Suspended/
        // hanging mode" section + a direct Decor.cpp read, 2026-07-14,
        // Decor.cpp:4732-4790/7158-7193). Real trigger icons 138/202
        // (BlockTypes.hpp doesn't need new named constants for these --
        // GetBarreCellType() below checks the raw icon values directly,
        // same convention as isMobileTransparent()'s own raw-icon table).
        // Grab is AUTOMATIC (no button) whenever standing at a "Hanging"
        // cell (bar tile with open space below) and not already doing
        // something else exclusive -- matches the real trigger exactly.
        // Horizontal movement while hanging reuses the plain moveInput*
        // speed formula this engine's normal walk already uses (no
        // acceleration ramp, matching the real "speedX*5, no ramp"
        // behavior's overall shape) -- kSuspendMoveSpeed has no established
        // proportional anchor to derive an exact ratio from (unlike
        // kJumpSpeedPowered/etc.), so it reuses kMoveSpeed directly as a
        // reasonable value, not a transcribed constant. kSuspendReleaseSpeed
        // is proportionally anchored to kJumpSpeed the same way as the jump-
        // headroom constants (real fixed -11.0 vs the real jump baseline
        // -16.0 already anchored to kJumpSpeed). The real 10-tick
        // jump-release wind-up animation is NOT modeled (no visible Blupi
        // model exists to show it) -- pressing Jump releases instantly, a
        // documented simplification, not a transcription gap. Real 5-tick
        // no-regrab grace timer (both the free-fall-drop and jump-release
        // paths) is a direct ScaleTime(5)-at-20Hz transcription, same
        // technique as kWaterGaugeTickSeconds/kTeleportDuration.
        static constexpr float kSuspendMoveSpeed = kMoveSpeed;
        static constexpr float kSuspendReleaseSpeed = kJumpSpeed * (11.0f / 16.0f);
        static constexpr float kSuspendNoRegrabSeconds = 5.0f / 20.0f;
        static constexpr float kSuspendDropHoldSeconds = 5.0f / 20.0f;

        // Ghost mode free-flight speed (plan.md BLUPI-111, see
        // ToggleGhost()'s own comment). Real `BlupiGhostStep()`
        // (`Decor.cpp:2639-2705`, `#ifdef MODERN`) moves Blupi directly
        // along BOTH real screen axes at `m_blupiSpeedX/Y * 4.0` -- a
        // real, exact 4x multiplier on the same per-frame speed value
        // normal walking already uses, not an approximation. This engine
        // has no strafe input (tank controls only: turn + forward/back),
        // so the real X-axis free movement is adapted to the same
        // forward/back-along-facing-yaw shape every other mode here
        // already uses (a natural 3D adaptation, not a new mechanic);
        // the real Y-axis (screen-vertical) maps directly to this
        // engine's own world-Y (height) -- see the dedicated
        // jumpPressed/crouchHeld reuse in Step()'s own ghost branch for
        // how vertical flight is driven without inventing a new keybind.
        static constexpr float kGhostSpeed = kMoveSpeed * 4.0f;

        // Vehicle mounts (plan.md E3D-MIG-171, real m_blupiHelico/Jeep/Tank/
        // Skate/Over, verified against mobile-eggbert-reference/
        // 10-blupi-mechanics.md §6/13-object-pickups.md's own "Vehicle
        // mounts" section). Real per-mode horizontal max-speed/accel/decel
        // are given in mobile-eggbert's own tick-domain px/tick units with
        // no established px-to-this-engine conversion factor (unlike
        // vertical fall distance, which had a real "match by feel duration"
        // precedent) -- these constants instead preserve the REAL RELATIVE
        // PROPORTION between vehicles (Jeep fastest at 20px/tick, Tank/
        // Overcraft slowest confirmed ground/hover modes at 12, Skateboard
        // 15, Helicopter's "normal" setting 16 -- "easy move" isn't
        // modeled, always using the normal-setting figures), anchored to an
        // arbitrary-but-reasonable "vehicles feel faster than walking"
        // baseline (kJeepMaxSpeed = kMoveSpeed * 2.5), same technique
        // already used for kSpringBounceHeld/NotHeld. Real accel=1.0/tick
        // for every mode below happens to convert to the SAME engine accel
        // (2.5 * kMoveSpeed) under this scaling -- not a coincidence, a
        // consequence of anchoring proportionally; decel differs per mode
        // since real decel rates differ (Tank stops fastest at 3.0/tick,
        // Overcraft/Skateboard hold momentum longest at 1.0/tick, matching
        // the doc's own "holds momentum longer" note).
        static constexpr float kVehicleAccel = kMoveSpeed * 2.5f;
        static constexpr float kJeepMaxSpeed = kMoveSpeed * 2.5f;       // real 20px/tick
        static constexpr float kJeepDecel = kVehicleAccel * 2.0f;       // real decel 2.0/tick
        static constexpr float kTankMaxSpeed = kMoveSpeed * 1.5f;       // real 12px/tick
        static constexpr float kTankDecel = kVehicleAccel * 3.0f;       // real decel 3.0/tick
        static constexpr float kOvercraftMaxSpeed = kMoveSpeed * 1.5f;  // real 12px/tick
        static constexpr float kOvercraftDecel = kVehicleAccel * 1.0f;  // real decel 1.0/tick
        static constexpr float kSkateboardMaxSpeed = kMoveSpeed * 1.875f; // real 15px/tick
        static constexpr float kSkateboardDecel = kVehicleAccel * 1.0f; // real decel 1.0/tick
        static constexpr float kHelicopterMaxSpeed = kMoveSpeed * 2.0f; // real 16px/tick (normal setting)
        static constexpr float kHelicopterDecel = kVehicleAccel * 2.0f; // real decel 2.0/tick

        // Vertical flight (Helicopter/Overcraft only -- Jeep/Tank/Skateboard
        // use the existing ground gravity path unchanged for FALLING,
        // matching the real source's own "uses the shared ground gravity/Air
        // path" note for Skateboard, and this session's decision not to
        // model Jeep/Tank's own real airborne-heavy-fall nuance -- this is
        // about gravity/falling only, NOT about whether Jump itself
        // triggers; see kSkateboardJumpSpeed's own comment above for that
        // separate, real per-mode jump-trigger gate). Real ascend/descend
        // targets (Helicopter -10/+12 px/tick, Overcraft -5/+12) are scaled
        // by the same technique as the horizontal speeds above, anchored to
        // each vehicle's own already-scaled horizontal max speed.
        static constexpr float kHelicopterAscendSpeed = kHelicopterMaxSpeed * (10.0f / 16.0f);
        static constexpr float kHelicopterDescendSpeed = kHelicopterMaxSpeed * (12.0f / 16.0f);
        static constexpr float kOvercraftAscendSpeed = kOvercraftMaxSpeed * (5.0f / 12.0f);
        static constexpr float kOvercraftDescendSpeed = kOvercraftMaxSpeed * (12.0f / 12.0f);
        static constexpr float kVehicleVerticalAccel = kVehicleAccel * 0.5f; // real accel 0.5/tick (both flying modes)

        // Jump vs Air mirrors GalaxyEggbertSimple3D::GEBlupiController's own
        // already-shipped split (real BlupiAction IDs 4/5) -- Simple3D
        // distinguishes them by a fixed 3-frame post-trigger window (its
        // own discrete animPhase-counted state machine); this class instead
        // uses velocity sign (m_velocityY > 0 = ascending = Jump, <= 0 =
        // falling/apex = Air), a natural adaptation to this class's
        // continuous-velocity physics rather than Simple3D's frame-counted
        // one, while keeping the same real two-state distinction (2026-07-11,
        // plan.md E3D-MIG-064 -- expanding the animation indicator beyond
        // its original Stop/March/Jump/Down/Up debug-stopgap set, per
        // mobile-eggbert-reference/08-animations.md §2's confirmed Air
        // frame data, already ported once via Simple3D so this reuses that
        // same pre-approved table rather than a fresh mobile-eggbert
        // transcription).
        // StopEcrase/MarchEcrase/Balloon/Teleporting (real BlupiAction IDs
        // 72/73/66/74) added 2026-07-11 with explicit user approval to
        // transcribe their real `table_blupi` icon-frame data (plan.md
        // E3D-MIG-064) -- unlike Jump/Air above, no pre-approved in-repo
        // source existed for these, so their frame arrays (see
        // GEBlupiController.cpp) are a fresh, narrowly-scoped transcription
        // of exactly these 4 records from mobile-eggbert's
        // Tables::table_blupi, not a wholesale table copy. Real
        // `BlupiAction` only defines ONE animation per status regardless of
        // grounded/airborne (no "AirEcrase"/"AirBalloon" variant exists),
        // so this class's own m_ecrase/m_balloon/m_teleporting flags take
        // precedence over the Jump/Air/Down/Up/March/Stop cascade below
        // rather than combining with it (see UpdateAnim()). StopEcrase vs
        // MarchEcrase is chosen by the same `moving` bool that already
        // splits Stop/March, mirroring the real table's own idle/moving
        // split for the squashed state (Balloon/Teleporting have no such
        // split in the real data -- one state covers both).
        enum class AnimState : std::uint8_t
        {
            Stop, March, Jump, Air, Down, Up,
            StopEcrase, MarchEcrase, Balloon, Teleporting,
            // Real "Bye" farewell freeze (see kByeDuration above and
            // TriggerBye() below) -- same "single static pose regardless
            // of grounded/airborne" shape as Teleporting/Balloon/Ecrase,
            // no dedicated frame table (falls through to the Stop pose in
            // GetAnimIcon()).
            Bye,
            // Real hazard-death lock + life-loss Voyage window (see
            // TriggerDeathLock()/IsDeathHidden() below) -- one real BlupiAction
            // status covering both freeze sub-states (locked hurt pose,
            // then invisible while the life-loss Voyage flies), same
            // "single animation regardless of grounded/airborne" shape as
            // Teleporting/Balloon/Ecrase. The real per-cause hurt-sprite
            // frame table (`Tables::table_blupi`, Clear1/Clear2/Clear3/
            // Clear4/Glu/Drown) IS transcribed (added 2026-07-16, see
            // GetAnimIcon()'s own per-`m_deathCause` switch below).
            DeathLocked,
            // Real Sucette/Drink/Charge 2-stage pickup delay (see
            // TriggerPickupFreeze() below) -- same real per-kind frame
            // table shape as DeathLocked (added 2026-07-16, see
            // GetAnimIcon()'s own per-`m_pickupFreezeKind` switch below).
            PickupBusy,
            // Vehicle-mode Stop/March pairs (plan.md BLUPI-037/038/047/058/
            // 069/084/088/091/094, found 2026-07-18 while auditing the HUD
            // animation icon's real scope): each real BlupiAction has its
            // own distinct Stop/March icon sequence (`table_blupi`, IDs
            // 15/16 Helico, 25/26 Jeep, 50/51 Tank, 37/38 Skate, 67/68
            // Over) -- selected below purely by `m_vehicleMode` + the same
            // `moving` bool that already splits the base Stop/March. Real
            // Turn variants (TurnHelico/Jeep/Tank/Skate/Over) are NOT
            // modeled -- precise real turn-trigger detection (direction-
            // change edge, distinct from just "moving") needs its own
            // research pass, same open gap as the base humanoid `Turn`
            // action (`BLUPI-025`), never implemented either.
            StopHelico, MarchHelico,
            StopJeep, MarchJeep,
            StopTank, MarchTank,
            StopSkate, MarchSkate,
            StopOver, MarchOver,
            // Swim/Surf Stop/March pairs (plan.md BLUPI-040/041/043/044,
            // found 2026-07-18) -- same shape as the vehicle pairs above,
            // selected by `m_nage`/`m_surf`. TurnNage/TurnSurf NOT modeled,
            // same reason as the vehicle Turn variants above. `Drown`
            // (real BlupiAction 24) is a distinct death-cause animation,
            // already covered by `DeathCause::Drown`'s own real frame
            // table under `DeathLocked` above, not a separate AnimState.
            StopNage, MarchNage,
            StopSurf, MarchSurf,
            // Hide (plan.md BLUPI-051, found 2026-07-18): real single
            // static pose (`table_blupi` ID 35, 9-frame idle-fidget cycle,
            // same shape as Stop's own long idle table) while
            // `SecretPower::Hide` is active.
            Hide,
            // Push (plan.md BLUPI-036, found 2026-07-18): real single
            // looping cycle (`table_blupi` ID 14, no idle/moving split in
            // real source) while `Step()`'s own `pushingCrate` parameter is
            // true (the caller's per-frame `GEInteractionSystem::
            // CrateBeingPushedThisFrame()` result).
            Push,
            // Real one-shot action animations (plan.md BLUPI-073/076, found
            // 2026-07-18): Switch (`table_blupi` ID 82, 0.5s), TakeDynamite/
            // PutDynamite (IDs 86/87, 0.9s/1.3s) -- see TriggerOneShotAnim()
            // below for the shared freeze/timer mechanism (same "freeze
            // everything, count a timer down, auto-resume" shape as
            // TriggerBye()/TriggerPickupFreeze()).
            Switch, TakeDynamite, PutDynamite
        };

        // Real `SecretPower` (plan.md E3D-MIG-170): the underlying game enum
        // itself only has 5 values (None/Shield/Power/Cloud/Hide,
        // `def/SecretPower.hpp`) -- NOT 8. The reference doc's own "Sp0-Sp7"
        // label for tile icons 158-165 is a speculative name-based guess
        // ("likely SecretPower value 0-7") that this session's direct
        // `Decor.cpp` research disproves: `Decor::IsWorld()` (~7079-7095)
        // shows icons 158-165/166-173 are hub-screen world-select markers
        // (locked/unlocked pairs, `06-doors.md`'s own `AdaptDoors` section),
        // wholly unrelated to Blupi's own secret-power buffs. The real
        // buffs are granted by 4 `MoveObject` pickups instead (ObjectType25
        // Shield, 26 Sucette->Power, 30 Drink->Hide, 31 Charge->Cloud),
        // confirmed directly in `Decor.cpp` ~6014-6087/~3048-3235.
        enum class SecretPower : std::uint8_t { None, Shield, Power, Cloud, Hide };

        // Vehicle mounts (plan.md E3D-MIG-171). Real confirmed pickup->
        // vehicle mapping (mobile-eggbert-reference/13-object-pickups.md):
        // ObjectType13->Helicopter, 19->Jeep, 28->Tank, 24->Skateboard,
        // 46->Overcraft (NOT "Balloon" despite ObjectType.hpp's own
        // misleading doc comment -- 10-blupi-mechanics.md's own research
        // found this is a confirmed real discrepancy: touching 46 sets
        // `m_blupiOver`, the Overcraft flag, not a separate Balloon ride).
        // The real standalone "Balloon" vehicle movement model exists in
        // Decor.cpp but this session found no confirmed pickup that grants
        // it, so it isn't included here.
        enum class VehicleMode : std::uint8_t { None, Helicopter, Jeep, Tank, Skateboard, Overcraft };

        void SetPosition(float x, float y, float z) noexcept;

        // Real "continuous ride" (plan.md E3D-MIG-152, `Decor::
        // MoveObjectStepLine`'s per-tick overlap re-test): called by the
        // caller once per frame when it determines Blupi is standing on an
        // active platform lift (this class has no knowledge of
        // `MobileObjSpec`/lifts itself, same "caller determines the
        // terrain/object fact" split as tempPassable/inSurfWater). Snaps
        // his position directly onto the lift's current surface -- Y is
        // corrected every frame rather than accumulated as a delta,
        // matching the real source's own drift-correction approach -- and
        // marks him grounded with zero vertical velocity, the same state a
        // normal landing leaves him in, so Step()'s own gravity doesn't
        // immediately re-trigger a fall next frame. Unlike SetPosition(),
        // this is NOT a teleport (no animation/state reset beyond velocity)
        // since it's called every single frame while riding, not once.
        void RideLift(float x, float y, float z) noexcept;

        // Sets facing directly — useful for tests/tools that need to face a
        // specific direction without stepping turnInput to get there (see
        // tools/VerifyBlupiMovement.cpp). Not used by normal gameplay input.
        void SetYaw(float yaw) noexcept { m_yaw = yaw; }

        [[nodiscard]] float GetX() const noexcept { return m_x; }
        [[nodiscard]] float GetY() const noexcept { return m_y; }
        [[nodiscard]] float GetZ() const noexcept { return m_z; }
        [[nodiscard]] float GetVelocityY() const noexcept { return m_velocityY; }
        [[nodiscard]] bool IsOnGround() const noexcept { return m_onGround; }

        // Block type directly beneath Blupi's feet, or Air (0) if not
        // grounded or the query position is out of grid range -- lets
        // callers implement ground-contact hazards (e.g. lava, which is
        // deliberately kept solid/walkable-on in this engine, see
        // BlockTypes.hpp's isMobileTransparent() comment) without
        // duplicating the grid-conversion math, and lets tools test the
        // detection logic directly (see tools/VerifyBlupiMovement.cpp)
        // without a live Game/GraphicsDevice.
        [[nodiscard]] std::uint16_t GetGroundBlockType(const Worlds::World& world) const noexcept;

        // Block type directly one cell ABOVE Blupi's own standing position
        // (mirrors GetGroundBlockType()'s exact shape/signature, but looks
        // up instead of down) -- used for the real Teleporter trigger check
        // (plan.md E3D-MIG-147, Decor.cpp:7378-7394's `pos.Y - 60`, "one
        // tile above Blupi"), matching the real detection geometry exactly.
        // An earlier attempt found this genuinely unreachable in this
        // engine's collision model (a solid pillar one cell above open
        // floor in the same column made Blupi land ON the pillar, since
        // GroundHeightAt treats the topmost solid block in a column as
        // that column's floor) -- resolved by making teleporter icons
        // ALWAYS non-solid for collision purposes (GroundHeightAt's own
        // IsTeleporterIcon() skip, plan.md E3D-MIG-147 §3), reproducing
        // real mobile-eggbert's genuinely per-tile-independent 2D
        // collision (a tile's solidity has no bearing on the tile below
        // it) well enough for this one purpose without a general
        // per-cell-occupancy collision rewrite. Unlike GetGroundBlockType(),
        // this is NOT gated on IsOnGround() -- the real IsTeleporte() check
        // itself is unconditional; the grounded requirement comes from the
        // separate trigger-site gate in TriggerTeleport() below, not from
        // this query itself.
        [[nodiscard]] std::uint16_t GetBlockTypeAbove(const Worlds::World& world) const noexcept;

        // Block type AT Blupi's own standing cell (mirrors
        // GetGroundBlockType()'s exact shape, but queries `round(m_y)`
        // rather than `round(m_y) - 1`) -- used for the real Water Surf/Nage
        // detection (plan.md E3D-MIG-148, Decor.cpp `IsSurfWater`/
        // `IsDeepWater`/`IsOutWater` ~7462-7493): whether the tile he
        // currently occupies is water at all. Unlike GetGroundBlockType(),
        // NOT gated on IsOnGround() -- water detection must work while
        // sinking through a deep pool (mid-water, not resting on anything).
        [[nodiscard]] std::uint16_t GetBlockTypeAt(const Worlds::World& world) const noexcept;

        // Enters the crusher-squash state (real m_blupiEcrase=true): zeroes
        // velocity, starts the kEcraseDuration recovery countdown. A no-op
        // (returns false) if already squashed, matching the real
        // `!m_blupiEcrase` re-trigger guard -- lets the caller play the
        // real entry sound (channel 70) only on an actual new trigger, not
        // every frame Blupi stands on an active crusher. Step() handles the
        // countdown and auto-clears it; IsEcrased() reflects the current
        // state either way.
        bool TriggerCrush() noexcept;
        [[nodiscard]] bool IsEcrased() const noexcept { return m_ecrase; }

        // Enters the balloon status (real m_blupiBalloon=true): zeroes
        // velocity, starts the kBalloonDuration recovery countdown. A
        // no-op (returns false) while already ballooned, matching the real
        // `!m_blupiBalloon` re-trigger guard in the wasp contact check --
        // lets the caller play the real entry sound (channel 40) only on
        // an actual new trigger. Step() applies reduced gravity while
        // ballooned and auto-clears it on timeout.
        bool TriggerBalloon() noexcept;
        [[nodiscard]] bool IsBallooned() const noexcept { return m_balloon; }

        // Pops the balloon early (real behavior when a type 3/16/96/97
        // hazard is touched while ballooned, Decor.cpp:5766-5781): clears
        // the status immediately (same real recovery sound, channel 41, as
        // a natural timeout -- both are observable via the same
        // before/after IsBallooned() comparison, no separate signal
        // needed) and forces Blupi briefly airborne (real m_blupiAir=true)
        // instead of the hazard killing him. A no-op if not currently
        // ballooned.
        void PopBalloon() noexcept;

        // Secret powers (plan.md E3D-MIG-170/172/173/174, see the
        // SecretPower enum's own comment). Each Trigger*() applies the real
        // exact OTHER-BUFF gate for that pickup (checked directly against
        // Decor.cpp) and, if it passes, grants that power (resetting the
        // shared gauge to kSecretPowerMax, overwriting whatever was active
        // before -- matches the real source's own asymmetric gates, which
        // don't uniformly check every other buff). Returns false (no-op) if
        // the gate fails, so the caller only plays the real grant sound on
        // an actual new trigger, same idiom as every other Trigger*() here.
        // Real vehicle-mode clauses (Power/Cloud/Hide only -- Shield has
        // none, confirmed directly) are enforced by the CALLER instead
        // (`GalaxyEggbertCnaGame.cpp`'s `canGrantPower/Cloud/Hide`, fixed
        // 2026-07-16), not inside these methods, since this class has no
        // vehicle-mode access of its own. The real 2-stage "busy" animation
        // + delay before Power(Sucette)/Hide(Drink)/Cloud(Charge) actually
        // activate (32/36/64 ticks) IS modeled -- see TriggerPickupFreeze()
        // below -- these methods themselves still grant instantly, matching
        // real source's own Charge (no deferral) and being called from the
        // deferred completion point for Sucette/Drink.
        bool TriggerShield() noexcept; // real gate: not already Shield/Hide/Power
        bool TriggerPower() noexcept;  // real gate: not already Shield
        bool TriggerCloud() noexcept;  // real gate: not already ANY power (loosest/most defensive)
        bool TriggerHide() noexcept;   // real gate: not already Shield/Cloud

        [[nodiscard]] SecretPower GetSecretPower() const noexcept { return m_secretPower; }
        [[nodiscard]] bool IsShielded() const noexcept { return m_secretPower == SecretPower::Shield; }
        [[nodiscard]] bool IsHidden() const noexcept { return m_secretPower == SecretPower::Hide; }
        // Real hazard-immunity gate (`!m_blupiShield && !m_blupiHide`,
        // confirmed identical across essentially every hazard/enemy death
        // check in Decor.cpp -- lava/spikes/saw/blitz/crusher/dynamite/fan/
        // the shared kill-list/wasp/large-creature/projectiles all use
        // this exact same two-flag gate, not just Shield alone). Callers
        // should gate every hazard/enemy death check on `!IsInvincible()`.
        // Real Cheat2 "SuperBlupi" (2026-07-13, plan.md `CHEAT-002`) is a
        // SEPARATE persistent invincibility flag (`m_bSuperBlupi`, toggled
        // by the cheat, independent of and stacking with Shield/Hide) --
        // confirmed via research to be checked alongside
        // `!m_blupiShield && !m_blupiHide` at every one of ~30 real hazard-
        // death call sites, i.e. a third OR'd condition, not a separate
        // gate. Does NOT grant "all abilities" (an earlier draft
        // description of this cheat was wrong) -- pure invincibility only.
        [[nodiscard]] bool IsInvincible() const noexcept
        {
            return IsShielded() || IsHidden() || m_cheatSuperBlupi;
        }
        void SetCheatSuperBlupi(bool enabled) noexcept { m_cheatSuperBlupi = enabled; }
        [[nodiscard]] bool GetCheatSuperBlupi() const noexcept { return m_cheatSuperBlupi; }
        [[nodiscard]] int GetSecretPowerLevel() const noexcept { return m_secretPowerLevel; }
        // True for exactly the one Step() call where the active power's
        // gauge crosses its own real warning threshold (Shield@10/Power@20/
        // Cloud@25/Hide@20) -- the caller plays the real per-power warning
        // channel (43/45/56/63) once, same one-shot shape as JustDrowned().
        [[nodiscard]] bool JustCrossedSecretPowerWarning() const noexcept { return m_secretPowerJustWarned; }

        // Invert/Mirror (plan.md PICKUP-011, see kInvertMax's own comment).
        // Real gate: blocked only while already Invert itself, or while
        // Hide is active -- independent of Shield/Power/Cloud/vehicles.
        bool TriggerInvert() noexcept; // real gate: not already Invert, not Hide
        [[nodiscard]] bool IsInverted() const noexcept { return m_invert; }
        [[nodiscard]] int GetInvertLevel() const noexcept { return m_invertLevel; }
        // True for exactly the one Step() call where Invert's gauge reaches
        // 0 naturally (no warning stage, unlike the 4 powers above) -- the
        // caller plays the real expiry channel (67) once.
        [[nodiscard]] bool JustExpiredInvert() const noexcept { return m_invertJustExpired; }

        // Suspended/hanging bar-and-rope mode (plan.md TILE-045, see
        // kSuspendMoveSpeed's own comment).
        [[nodiscard]] bool IsSuspended() const noexcept { return m_suspended; }

        // Ghost mode (plan.md BLUPI-111), real trigger confirmed by the
        // user (2026-07-14): activated by typing "ghost" during Play, via
        // a real SECOND cheat-entry method (`InputPad.cpp:686-753`)
        // distinct from the numbered on-screen cheat menu -- see
        // `GEInputPad::UpdateTypedGhostCheat()`'s own comment. Real
        // `Decor::CheatAction()`'s Ghost branch is an unconditional
        // toggle (`Decor.cpp:2046-2070`): turning ON clears every other
        // vehicle/movement mode and zeroes vertical velocity; turning OFF
        // only succeeds if Blupi's CURRENT position isn't inside solid
        // geometry (`!DecorDetect(BlupiRect(m_blupiPos))`,
        // `Decor.cpp:2065`) -- otherwise the toggle-off is silently
        // rejected (stays ghosted) rather than stranding him inside a
        // wall. Returns the new ghost state (so the caller can play a
        // toggle sound only on an actual state change, same idiom as
        // every other Trigger*() here, even though this one isn't gated
        // on anything when turning on).
        bool ToggleGhost(const Worlds::World& world) noexcept;
        [[nodiscard]] bool IsGhost() const noexcept { return m_ghost; }

        // Vehicle mounts (plan.md E3D-MIG-171, see VehicleMode's own
        // comment). Real gate: blocked while already riding ANY other
        // vehicle, Nage/Surf, Suspended, or Balloon/Ecrase (fixed 2026-07-16
        // -- Balloon/Ecrase were missing from this gate entirely; this
        // comment previously claimed "Ecrase already blocks separately via
        // its own state elsewhere", which was never actually true) -- NOT
        // gated on Shield/Power (real note: "none of them check Shield
        // or Power"). Zeroes horizontal velocity and silently cancels
        // Cloud/Hide if active (matching the real "if Cloud or Hide was
        // active it is silently cancelled" -- Shield/Power are left
        // untouched). A no-op (returns false) if the gate fails, same
        // idiom as every other Trigger*() here. inNage/inSurf are passed in
        // by the caller (this class doesn't call itself recursively to
        // check its own water state).
        bool TriggerMount(VehicleMode mode, bool inNage, bool inSurf) noexcept;

        // Voluntary dismount (real: action-button while riding, no fixed
        // duration otherwise). A no-op if not currently in a vehicle.
        // Zeroes horizontal velocity; the caller is responsible for
        // spawning the vehicle pickup back into the world at Blupi's
        // position (matching the real "deposits vehicle pickup back into
        // the world" behavior) and playing any dismount sound -- this
        // class has no access to GEWorldRuntime/MobileObjSpec.
        void TriggerDismount() noexcept;

        [[nodiscard]] VehicleMode GetVehicleMode() const noexcept { return m_vehicleMode; }
        [[nodiscard]] bool IsInVehicle() const noexcept { return m_vehicleMode != VehicleMode::None; }

        // Vehicle motor sound (plan.md SOUND-007/008, BLUPI-084/090/148, found 2026-07-17): real
        // `Decor::AdaptMotorVehicleSound()` crossfades a start/loop/stop sound set per vehicle,
        // selecting the "high" (moving) vs "low" (idle) loop variant via `m_blupiMotorHigh`.
        // Real source gives Helicopter/Jeep/Tank/Overcraft their own motor sound set; Skateboard
        // has none (confirmed: `AdaptMotorVehicleSound()`'s own `if`/`else if` chain checks only
        // those 4 modes). `HasVehicleMotor()` is the "should a loop be playing at all" gate;
        // `IsVehicleMotorHigh()` is the pitch-select flag, translated from the real per-mode
        // `m_blupiMotorHigh` assignments (Jeep: `m_blupiAction != BlupiAction::Stop`, i.e.
        // nonzero horizontal velocity including coasting after input release, translated here as
        // nonzero `m_vehicleSpeed`; Helicopter/Overcraft: real code uses their own analogous
        // "not idle" checks, translated here as nonzero vertical `m_velocityY`, the flight-mode
        // equivalent of horizontal motion).
        [[nodiscard]] bool HasVehicleMotor() const noexcept
        {
            return IsInVehicle() && m_vehicleMode != VehicleMode::Skateboard;
        }
        [[nodiscard]] bool IsVehicleMotorHigh() const noexcept
        {
            if (m_vehicleMode == VehicleMode::Helicopter || m_vehicleMode == VehicleMode::Overcraft)
            {
                return std::fabs(m_velocityY) > 0.01f;
            }
            return std::fabs(m_vehicleSpeed) > 0.01f;
        }

        // Real per-vehicle hazard immunity (found 2026-07-16, `Decor.cpp:5504-5528`): Overcraft/
        // Jeep/Tank protect against Spike/Drip/Saw specifically (`!m_blupiOver && !m_blupiJeep &&
        // !m_blupiTank`, identical across all 3) -- NOT Helicopter or Skateboard, which get no
        // such immunity in real source. Lava/Blitz/Crusher deliberately do NOT check this (real
        // source only gates those on Shield/Hide/SuperBlupi, confirmed directly, no vehicle
        // clause at all) -- do not apply this blanket-style to every hazard.
        [[nodiscard]] bool HasVehicleHazardImmunity() const noexcept
        {
            return m_vehicleMode == VehicleMode::Overcraft || m_vehicleMode == VehicleMode::Jeep ||
                   m_vehicleMode == VehicleMode::Tank;
        }

        // Launches Blupi upward off a spring tile (real gate: grounded and
        // not already airborne -- swimming/surfing/suspended don't exist in
        // this engine, so only `m_onGround` remains relevant). A no-op
        // (returns false) while airborne, matching the real `!m_blupiAir`
        // guard -- lets the caller only play the real bounce sound (channel
        // 41) on an actual new trigger, not every frame Blupi stands on a
        // spring. jumpHeld selects which of the two real magnitudes applies
        // (see kSpringBounceHeld/kSpringBounceNotHeld above).
        bool TriggerSpringBounce(bool jumpHeld) noexcept;

        // Enters the teleport-transit state (plan.md E3D-MIG-147, real
        // `BlupiAction::Teleporte`: `m_blupiVitesseX/Y=0`,
        // `m_blupiFocus=false`) -- freezes Blupi completely (no movement/
        // turning/jump/gravity at all, see Step()) for kTeleportDuration
        // seconds. Real gate: grounded and not already in transit
        // (`!m_blupiAir`), not ballooned/squashed (`!m_blupiBalloon &&
        // !m_blupiEcrase`), and not in any vehicle mount (`!m_blupiHelico/
        // Over/Jeep/Tank/Skate`, Decor.cpp:5593-5594 -- added 2026-07-16;
        // this comment previously claimed vehicles weren't modeled, which
        // stopped being true once VehicleMode was added). A no-op (returns
        // false) if any of those aren't met, matching the same idempotent-
        // re-trigger shape
        // as TriggerCrush()/TriggerBalloon()/TriggerSpringBounce() -- lets
        // the caller play the real entry sound (channel 71) only on an
        // actual new trigger. `icon` (330-333) is remembered so the caller
        // can look up the paired destination via
        // GEWorldRuntime::FindTeleportDestination() once IsTeleporting()
        // naturally clears (a before/after comparison across Step(), same
        // pattern already used for the balloon/crusher recovery sounds).
        bool TriggerTeleport(std::uint16_t icon) noexcept;
        [[nodiscard]] bool IsTeleporting() const noexcept { return m_teleporting; }
        [[nodiscard]] std::uint16_t GetTeleportIcon() const noexcept { return m_teleportIcon; }

        // Real "Bye" farewell freeze (see kByeDuration's own comment above) --
        // a no-op (returns false) if any other freeze is already active,
        // matching the same idempotent-re-trigger shape as TriggerTeleport().
        // The caller (only site: world-select portal contact) is expected to
        // call SetYaw() right after a successful trigger to face the camera,
        // and to detect natural completion the same before/after-Step() way
        // teleport completion is detected (IsBye() was true, now false).
        bool TriggerBye() noexcept;
        [[nodiscard]] bool IsBye() const noexcept { return m_bye; }

        // Generic one-shot action-animation freeze (plan.md BLUPI-073/076,
        // found 2026-07-18) shared by Switch/TakeDynamite/PutDynamite --
        // same idempotent-re-trigger/freeze/auto-resume shape as
        // TriggerBye(), parameterised on which AnimState + real duration to
        // play since these 3 states never overlap in practice (one action
        // button per frame). The caller detects natural completion the same
        // before/after-Step() way teleport/Bye completion is detected.
        bool TriggerOneShotAnim(AnimState state, float durationSeconds) noexcept;
        [[nodiscard]] bool IsOneShotAnimPlaying() const noexcept { return m_oneShotAnimActive; }

        // Real death-lock + life-loss Voyage (plan.md death-VFX follow-up, verified directly
        // against `Decor.cpp:6374-6392`'s shared per-cause duration dispatch): every real hazard
        // death locks Blupi in a frozen hurt state for a fixed per-cause duration, THEN goes
        // `Hide` (invisible) and plays a life-loss Voyage (the caller starts this second part,
        // see ConsumeDeathLockResolved() below) before control returns. Two CHAINED freeze
        // sub-states, same "freeze everything, count a timer down, auto-resume" shape as
        // TriggerTeleport()/m_teleporting above:
        //   1. The lock itself (`IsDeathLocked()`), duration from `cause` (see kDeathLockTicks*
        //      constants below -- Clear1=70/Clear2=100/Clear3=70/Clear4=110/Glu=100/Drown=90 real
        //      ticks, `/20` to seconds, the same tick-rate conversion used throughout this
        //      session).
        //   2. Once (1) elapses, automatically transitions into the life-loss-Voyage window
        //      (`IsDeathHidden()`), a fixed real `ScaleTime(40)`=40 ticks=2.0s -- Blupi stays frozen
        //      and invisible through this too. `ConsumeDeathLockResolved()` fires exactly once,
        //      the frame (1) elapses and (2) begins, telling the caller to apply the real
        //      `m_blupiRestart`-gated respawn NOW, call `GEInteractionSystem::LoseLife()`, and
        //      start the cosmetic icon-48 life-loss Voyage itself (this class has no dependency on
        //      GEInteractionSystem/GESound, same one-way layering as the lift-riding
        //      IsRidingLift()/RideDeltaX() pattern below).
        // The already-shipped Clear2Ascend/Clear3Ascend cosmetic ascend-Voyage and Clear4's
        // particle burst (both started by the caller at CONTACT time, independent of this lock)
        // are UNCHANGED by this -- confirmed via direct source read that `m_blupiPhase` (this
        // lock's own timer) and `m_voyagePhase` (the ascend Voyage's timer) are separate counters.
        enum class DeathCause : std::uint8_t { Clear1, Clear2, Clear3, Clear4, Glu, Drown };

        // No-op (returns false) if already locked -- matches the idempotent-re-trigger shape of
        // every other Trigger*() here. shouldRespawn is the real `m_blupiRestart` flag for this
        // specific cause (false only for Fan/generic-hazard-contact -- see the plan's own
        // Decor.cpp:5458-5472/5782-5815 citation; true for every other real cause).
        bool TriggerDeathLock(DeathCause cause, bool shouldRespawn) noexcept;
        [[nodiscard]] bool IsDeathLocked() const noexcept { return m_deathLocked; }
        [[nodiscard]] bool IsDeathHidden() const noexcept { return m_deathLossVoyageActive; }
        // True exactly once, the frame the lock (part 1) elapses and the life-loss-Voyage window
        // (part 2) begins. outShouldRespawn echoes the value passed into TriggerDeathLock().
        [[nodiscard]] bool ConsumeDeathLockResolved(bool& outShouldRespawn) noexcept;

        // Real 2-stage "busy" pickup delay (Sucette/Drink/Charge, plan.md `173`, verified directly
        // against `Decor.cpp:6025-6087`/`3048-3235`): contact plays an immediate "grab" sound and
        // freezes Blupi (`m_blupiFocus=false`, the SAME mechanism as `TriggerTeleport()`/the death
        // lock -- a third application of that template) for a real fixed duration (Sucette=32
        // ticks=1.6s, Drink=36=1.8s, Charge=64=3.2s), THEN plays a second "complete" sound. Sucette/
        // Drink's real buff (Power/Hide) is granted ONLY at completion -- the ONLY reward that's
        // actually deferred; Charge's Cloud buff grants immediately at contact in real source too
        // (confirmed -- only its freeze/completion-sound was missing), so the caller does not call
        // any "grant" method for Charge at resolution, only plays the sound + respawns the item.
        // Completion also RE-SPAWNS the same pickup at its original position (real `ObjectStart`,
        // speed=0, static) -- the caller does this via GEInteractionSystem, this class has no world
        // access, same one-way layering as every other Trigger*() here.
        enum class PickupFreezeKind : std::uint8_t { Sucette, Drink, Charge };

        // No-op (returns false) if already frozen (pickup or death) -- matches every other
        // Trigger*() here. A death-lock started while pickup-frozen always wins (real BlupiDead()
        // unconditionally overwrites whatever action was active) -- TriggerDeathLock() cancels any
        // pending pickup freeze outright, see its own .cpp implementation.
        bool TriggerPickupFreeze(PickupFreezeKind kind) noexcept;
        [[nodiscard]] bool IsPickupFrozen() const noexcept { return m_pickupFrozen; }
        // True exactly once, the frame the freeze elapses. outKind echoes the value passed into
        // TriggerPickupFreeze().
        [[nodiscard]] bool ConsumePickupFreezeResolved(PickupFreezeKind& outKind) noexcept;

        // Water Surf (standing/floating at the surface, dry above) / Nage
        // (fully submerged) status (plan.md E3D-MIG-148) -- unlike every
        // other status above, these are NOT set via a Trigger*() call:
        // Step()'s own `inSurfWater`/`inDeepWater` parameters set them
        // directly every frame (the caller recomputes both from
        // GetBlockTypeAt()/GetBlockTypeAbove() + BlockTypes::isWater() each
        // frame, the same "caller determines the terrain fact, this class
        // just tracks status" split already used for tempPassable). The
        // caller detects Surf/Nage/dry TRANSITIONS (for the real splash/
        // resurface sounds -- channels 22/25) via a before/after comparison
        // of these getters around the Step() call, the same idiom already
        // used for the balloon/crusher/teleport recovery sounds.
        [[nodiscard]] bool IsSurf() const noexcept { return m_surf; }
        [[nodiscard]] bool IsNage() const noexcept { return m_nage; }

        // Real submersion breath gauge, 0-100 (plan.md E3D-MIG-148,
        // `m_blupiLevel`) -- only ticks down while Nage, resets to full
        // whenever Nage ends. See JustDrowned() for the death signal.
        [[nodiscard]] int GetWaterGaugeLevel() const noexcept { return m_waterGaugeLevel; }

        // True for exactly the one Step() call where the gauge crosses from
        // above zero to zero while still Nage -- the caller checks this
        // once per frame (same shape as every terrain-hazard death check
        // already in GalaxyEggbertCnaGame::Update()) and applies the real
        // drowning death consequence (channel 26, distinct from every other
        // death cause's sound) via the shared triggerDeath() lambda, which
        // respawns Blupi (moving him out of the water) before the next
        // Step() call recomputes Nage.
        [[nodiscard]] bool JustDrowned() const noexcept { return m_justDrowned; }

        // Real 10-slot safe-position FIFO respawn (plan.md E3D-MIG-067,
        // `Decor::BlupiAddFifo`/`m_blupiValidPos`, verified directly
        // against Decor.cpp:6467-6478/6654-6673). Call once per frame
        // (after Step()) with the caller's own "not on/under any
        // recognized hazard" determination (`externallySafe`) -- this
        // class only knows its own grounded/balloon/ecrase state, not
        // terrain hazard tile types (checked via GetGroundBlockType() by
        // the caller) or teleporter-trigger occupancy
        // (GetBlockTypeAbove()). Real gate also includes vehicle/shield/
        // hide/ledge-teeter/transport-riding/projectile-path checks --
        // none of those concepts (or an occupancy grid for the last one)
        // exist in this engine yet, same simplification as every other
        // hazard/mechanic. Updates the tracked valid respawn position to
        // the FIFO's OLDEST entry BEFORE pushing the current position
        // (matching the real source's exact order, giving roughly the
        // same "don't respawn exactly where you died" buffer), deduping
        // consecutive identical positions the same way the real FIFO does
        // (extended to all 3 axes here, since real mobile-eggbert's own
        // 2-axis dedup is a direct consequence of it having no Z axis at
        // all, not a deliberate 2-of-3 choice for a 3D engine).
        void UpdateSafePosition(bool externallySafe) noexcept;
        [[nodiscard]] float GetValidX() const noexcept { return m_validX; }
        [[nodiscard]] float GetValidY() const noexcept { return m_validY; }
        [[nodiscard]] float GetValidZ() const noexcept { return m_validZ; }

        // Facing angle in radians, 0 = looking toward -Z. Updated every Step()
        // by turnInput (see below) — unlike a strafe-style controller, yaw is
        // driven directly by turning, not derived from movement direction.
        [[nodiscard]] float GetYaw() const noexcept { return m_yaw; }

        // Current coarse animation state and the blupi.png icon index to
        // display for it right now (10 columns, 60x60 px tiles — see
        // GalaxyEggbertSimple3D::GEBlupiController for the same convention).
        [[nodiscard]] AnimState GetAnimState() const noexcept { return m_animState; }
        [[nodiscard]] int GetAnimIcon() const noexcept;

        // Tank controls, matching GalaxyEggbertSimple3D's already-shipped
        // scheme (GalaxyEggbertSimpleGame::SetupInput's "Move" axis) and
        // mobile-eggbert's own control feel: turnInput (-1/0/+1, Left/Right)
        // rotates facing; moveInput (-1/0/+1, Down/Up) moves forward/back
        // along the current facing direction — arrows are not a strafe pad.
        // crouchHeld (LShift)/lookUpHeld (RShift) mirror Simple3D's Down/Up
        // BlupiState and don't affect collision, only animation state and
        // (via the CNA game's camera) eye height/look pitch. tempPassable
        // (plan.md E3D-MIG-146, default false so existing callers/tests
        // that don't place a Temp tile are unaffected) is the caller's
        // pre-computed GEWorldRuntime::IsTempPassableAtPhase() result for
        // this frame -- kept as a plain bool rather than a GEWorldRuntime
        // dependency so this class stays engine-agnostic/scriptable (see
        // the class comment). When true, any BlockTypes::Temp cell is
        // treated as non-solid for ground-height purposes (both the main
        // landing check and TryMoveAxis's step-up gate), so Blupi
        // genuinely falls through a vanished Temp tile instead of standing
        // on it. inSurfWater/inDeepWater (plan.md E3D-MIG-148, both default
        // false so existing callers/tests are unaffected) are the caller's
        // own per-frame Surf/Nage determination (see IsSurf()/IsNage()'s own
        // comment) -- inDeepWater additionally drives reduced (floaty)
        // gravity and a swim-up jump instead of the normal ground jump.
        // pushingCrate (plan.md BLUPI-036, found 2026-07-18, default false so
        // existing callers/tests are unaffected) is the caller's own
        // per-frame `GEInteractionSystem::CrateBeingPushedThisFrame()`
        // result -- drives the real Push animation (`table_blupi` ID 14),
        // which has no separate idle variant in real source (only a single
        // looping cycle), so it's shown unconditionally while true,
        // matching how Balloon/Ecrase also have no idle/moving split.
        void Step(const Worlds::World& world, float turnInput, float moveInput,
                  bool jumpPressed, bool crouchHeld, bool lookUpHeld, float dt,
                  bool tempPassable = false, bool inSurfWater = false, bool inDeepWater = false,
                  bool pushingCrate = false);

    private:
        // Sentinel GroundHeightAt() returns when a column has no solid
        // block anywhere (plan.md E3D-MIG-067) -- distinct from every real
        // returned height (always >= 1, since it's y+1 for a solid block
        // at y>=0). Callers must treat this as "keep falling", not as
        // solid ground at Y=0 (a real bug found and fixed 2026-07-11: the
        // old `return 0` fallback silently acted as a floor, making
        // Blupi's fall-off-world death unreachable via normal walking).
        static constexpr int kNoGround = -1;

        [[nodiscard]] static bool IsSolidAt(const Worlds::World& world, int gx, int gy, int gz);
        // @p referenceY bounds the scan to grid Y <= floor(referenceY) + 1
        // (fixed 2026-07-13, NEXT.md §4/§5's "roofed-interior" limitation):
        // previously this always scanned from the TOP of the entire world
        // down, so a solid ceiling anywhere above an open interior (e.g. a
        // roofed tunnel) registered as that column's "floor", making the
        // real floor beneath it unreachable -- Blupi got resolved onto TOP
        // of the ceiling instead of standing on the real ground below it.
        // Bounding the scan by the caller's own current height (or current
        // height + a reachable step-up allowance, for TryMoveAxis) means
        // only solid blocks AT OR BELOW roughly where the caller already
        // is can ever be treated as ground -- a real ceiling above stays
        // irrelevant once the caller is already beneath it, matching real
        // mobile-eggbert's per-tile-independent 2D collision well enough
        // for this purpose without a general per-cell-occupancy rewrite.
        [[nodiscard]] static int GroundHeightAt(const Worlds::World& world, int gx, int gz, bool tempPassable,
                                                 float referenceY);
        // Real Decor::IsNormalJump() headroom probe (see kJumpSpeedPowered's
        // own comment): checks the 2 grid cells directly above Blupi's
        // current standing height for solid blocks. Real source offsets the
        // probe 15px toward Blupi's current facing direction to pick
        // between 2 adjacent columns near a tile boundary -- simplified
        // here to a single column (this engine's own current position,
        // already grid-snapped), same "single stance, not multi-candidate"
        // simplification already used for TryActivateSwitch/the bridge
        // trigger scan, not a transcription gap.
        [[nodiscard]] bool HasJumpHeadroom(const Worlds::World& world) const;

        // Real Decor::GetTypeBarre() classification (see kSuspendMoveSpeed's
        // own comment) -- checks the icon at grid (gx,gy,gz) for a real bar
        // tile (138/202); if found, None/Hanging/LandingAvailable is decided
        // by whether the cell directly below (gy-1) is solid. Simplified to
        // a single grid-snapped cell (this engine's own current position),
        // same "single stance" simplification as HasJumpHeadroom/
        // TryActivateSwitch -- the real sub-pixel bottom-of-hitbox probe and
        // separate DecorDetect landing-rect check aren't modeled.
        enum class BarreCellType { None, Hanging, LandingAvailable };
        [[nodiscard]] static BarreCellType GetBarreCellType(const Worlds::World& world, int gx, int gy, int gz);
        void TryMoveAxis(const Worlds::World& world, float ddx, float ddz, bool tempPassable);
        void UpdateAnim(bool moving, bool crouchHeld, bool lookUpHeld, float dt, bool pushingCrate = false);

        float m_x = 0.0f;
        float m_y = 1.0f;
        float m_z = 0.0f;
        float m_velocityY = 0.0f;
        bool m_onGround = false;

        float m_yaw = 0.0f;
        AnimState m_animState = AnimState::Stop;
        int m_animPhase = 0;
        float m_animTimer = 0.0f;

        bool m_ecrase = false;
        float m_ecraseTimer = 0.0f;

        bool m_balloon = false;
        float m_balloonTimer = 0.0f;

        SecretPower m_secretPower = SecretPower::None;
        int m_secretPowerLevel = 0;
        float m_secretPowerTimer = 0.0f;
        bool m_secretPowerJustWarned = false;

        bool m_invert = false;
        int m_invertLevel = 0;
        float m_invertTimer = 0.0f;
        bool m_invertJustExpired = false;

        bool m_suspended = false;
        float m_suspendGraceTimer = 0.0f; // real m_blupiNoBarre, prevents an immediate re-grab
        float m_suspendDropHoldTimer = 0.0f; // real "holding Down >5 ticks drops him" accumulator

        bool m_ghost = false;

        bool m_cheatSuperBlupi = false;

        VehicleMode m_vehicleMode = VehicleMode::None;
        float m_vehicleSpeed = 0.0f; // current ramped horizontal speed (real "vitesse"), signed by moveInput's own sign

        bool m_teleporting = false;
        float m_teleportTimer = 0.0f;
        std::uint16_t m_teleportIcon = 0;

        bool m_bye = false;
        float m_byeTimer = 0.0f;

        bool m_oneShotAnimActive = false;
        float m_oneShotAnimTimer = 0.0f;
        AnimState m_oneShotAnimState = AnimState::Stop;

        // Real ScaleTime(40)=40 ticks=2.0s, same conversion as every other duration here.
        static constexpr float kLifeLossVoyageDuration = 2.0f;
        bool m_deathLocked = false;
        float m_deathLockTimer = 0.0f;
        bool m_deathLossVoyageActive = false;
        float m_deathLossVoyageTimer = 0.0f;
        bool m_deathLockShouldRespawn = false;
        bool m_deathLockResolvedPending = false; // consumed once via ConsumeDeathLockResolved()
        // Remembered so GetAnimIcon()'s DeathLocked case can select the real per-cause hurt-sprite
        // frame array (added 2026-07-16, same table_blupi transcription as kClear1Frames etc.).
        DeathCause m_deathCause = DeathCause::Clear1;

        bool m_pickupFrozen = false;
        float m_pickupFreezeTimer = 0.0f;
        PickupFreezeKind m_pickupFreezeKind = PickupFreezeKind::Sucette;
        bool m_pickupFreezeResolvedPending = false; // consumed once via ConsumePickupFreezeResolved()

        bool m_surf = false;
        bool m_nage = false;
        int m_waterGaugeLevel = kWaterGaugeMax;
        float m_waterGaugeTimer = 0.0f;
        bool m_justDrowned = false;

        static constexpr int kSafeFifoCapacity = 10;
        std::array<std::array<float, 3>, kSafeFifoCapacity> m_safeFifo{};
        int m_safeFifoCount = 0;
        // Real default (Decor.cpp:356, m_blupiValidPos = m_blupiStartPos)
        // -- matches this class's own m_x/m_y/m_z defaults, so a respawn
        // before the FIFO has ever recorded a safe frame still lands
        // somewhere sane (the spawn point) rather than an uninitialized
        // origin.
        float m_validX = 0.0f;
        float m_validY = 1.0f;
        float m_validZ = 0.0f;
    };
}
