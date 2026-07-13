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
        // already-approved GEBlupiController.cpp (kStopFrames/kMarchFrames/
        // kJumpFrames/kDownFrames/kUpFrames/kAirFrames) — not a fresh
        // mobile-eggbert transcription.
        constexpr int kStopFrames[]  = {0};
        constexpr int kMarchFrames[] = {5, 6, 7, 8, 9, 10};
        constexpr int kJumpFrames[]  = {17, 18, 19};
        constexpr int kAirFrames[]   = {169, 26, 170, 170, 27};
        constexpr int kDownFrames[]  = {33};
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
        if (IsInVehicle() || inNage || inSurf)
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
        if (m_teleporting || !m_onGround || m_balloon || m_ecrase)
        {
            return false;
        }
        m_teleporting = true;
        m_teleportTimer = kTeleportDuration;
        m_teleportIcon = icon;
        m_velocityY = 0.0f;
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
                                  bool tempPassable, bool inSurfWater, bool inDeepWater)
    {
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
            const bool wasAboveZero = m_waterGaugeLevel > 0;
            m_waterGaugeTimer += dt;
            while (m_waterGaugeTimer >= kWaterGaugeTickSeconds && m_waterGaugeLevel > 0)
            {
                m_waterGaugeTimer -= kWaterGaugeTickSeconds;
                --m_waterGaugeLevel;
            }
            m_justDrowned = wasAboveZero && m_waterGaugeLevel <= 0;
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
        if (IsInVehicle())
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
        else if (m_onGround && jumpPressed && !m_ecrase)
        {
            // Real Decor::IsNormalJump() headroom modulation (plan.md
            // TILE-041, see kJumpSpeedPowered's own comment) -- a clipped
            // ceiling within 2 tiles overhead reduces jump strength instead
            // of letting Blupi clip through it.
            const bool powered = m_secretPower == SecretPower::Power;
            if (HasJumpHeadroom(world))
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
        else
        {
            // Wasp "balloon" status: reduced gravity while active (kBalloonGravityMultiplier's own
            // comment explains this is an approximation of "floats rather than dying"). Nage
            // (plan.md E3D-MIG-148): same shape, a slow floaty sink instead of a free-fall drop
            // while genuinely submerged (kNageGravityMultiplier's own comment).
            const float effectiveGravity = m_balloon ? kGravity * kBalloonGravityMultiplier
                                          : m_nage    ? kGravity * kNageGravityMultiplier
                                                      : kGravity;
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

        // kNoGround (no solid block anywhere in this column) must never
        // clamp Blupi to a fake floor -- he keeps falling under gravity
        // indefinitely, same as walking off any other ledge with a real
        // drop, letting GalaxyEggbertCnaGame::Update()'s kFallDeathY check
        // eventually catch it (plan.md E3D-MIG-067).
        if (groundY != kNoGround && newY <= static_cast<float>(groundY))
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

        UpdateAnim(moving, crouchHeld, lookUpHeld, dt);
    }

    void GEBlupiController::UpdateAnim(bool moving, bool crouchHeld, bool lookUpHeld, float dt)
    {
        // Precedence: Teleporting/Balloon/Ecrase (each a real BlupiAction
        // status with only ONE real animation regardless of grounded/
        // airborne, see the AnimState enum's own comment) beat the normal
        // ground/air cascade entirely, which otherwise matches
        // GalaxyEggbertSimple3D::GEBlupiController::UpdateState: airborne
        // beats crouch/look-up beats moving beats idle. Airborne itself
        // splits Jump (ascending) vs Air (falling/apex) by velocity sign --
        // see the AnimState enum's own comment for why this differs from
        // Simple3D's frame-counted trigger window.
        const AnimState newState = m_teleporting ? AnimState::Teleporting
                                  : m_balloon     ? AnimState::Balloon
                                  : m_ecrase      ? (moving ? AnimState::MarchEcrase : AnimState::StopEcrase)
                                  : !m_onGround   ? (m_velocityY > 0.0f ? AnimState::Jump : AnimState::Air)
                                  : crouchHeld    ? AnimState::Down
                                  : lookUpHeld    ? AnimState::Up
                                  : moving        ? AnimState::March
                                                  : AnimState::Stop;
        if (newState != m_animState)
        {
            m_animState = newState;
            m_animPhase = 0;
            m_animTimer = 0.0f;
            return;
        }

        m_animTimer += dt;
        const float frameDuration = 1.0f / kAnimFps;
        if (m_animTimer >= frameDuration)
        {
            m_animTimer -= frameDuration;
            ++m_animPhase;
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
            case AnimState::Stop:
            default:
                return kStopFrames[m_animPhase % (sizeof(kStopFrames) / sizeof(kStopFrames[0]))];
        }
    }
}
