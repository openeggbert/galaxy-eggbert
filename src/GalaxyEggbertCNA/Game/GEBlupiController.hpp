#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <array>
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
        static constexpr float kJumpSpeed = 12.0f;
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
        enum class AnimState : std::uint8_t { Stop, March, Jump, Air, Down, Up };

        void SetPosition(float x, float y, float z) noexcept;

        // Sets facing directly — useful for tests/tools that need to face a
        // specific direction without stepping turnInput to get there (see
        // tools/VerifyBlupiMovement.cpp). Not used by normal gameplay input.
        void SetYaw(float yaw) noexcept { m_yaw = yaw; }

        [[nodiscard]] float GetX() const noexcept { return m_x; }
        [[nodiscard]] float GetY() const noexcept { return m_y; }
        [[nodiscard]] float GetZ() const noexcept { return m_z; }
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
        // (`!m_blupiAir`), and not ballooned/squashed (`!m_blupiBalloon &&
        // !m_blupiEcrase` -- vehicles aren't modeled, so those clauses of
        // the real gate don't apply). A no-op (returns false) if any of
        // those aren't met, matching the same idempotent-re-trigger shape
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
        // on it.
        void Step(const Worlds::World& world, float turnInput, float moveInput,
                  bool jumpPressed, bool crouchHeld, bool lookUpHeld, float dt,
                  bool tempPassable = false);

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
        [[nodiscard]] static int GroundHeightAt(const Worlds::World& world, int gx, int gz, bool tempPassable);
        void TryMoveAxis(const Worlds::World& world, float ddx, float ddz, bool tempPassable);
        void UpdateAnim(bool moving, bool crouchHeld, bool lookUpHeld, float dt);

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

        bool m_teleporting = false;
        float m_teleportTimer = 0.0f;
        std::uint16_t m_teleportIcon = 0;

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
