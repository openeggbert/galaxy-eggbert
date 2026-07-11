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

    bool GEBlupiController::IsSolidAt(const Worlds::World& world, int gx, int gy, int gz)
    {
        return !world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                               static_cast<std::uint16_t>(gz)).isAir();
    }

    int GEBlupiController::GroundHeightAt(const Worlds::World& world, int gx, int gz, bool tempPassable)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int y = blocksPerAxis - 1; y >= 0; --y)
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
                return y + 1;
            }
        }
        // No solid block anywhere in this column -- see kNoGround's own
        // comment for why this must not be treated as solid ground at
        // Y=0.
        return kNoGround;
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

    void GEBlupiController::TryMoveAxis(const Worlds::World& world, float ddx, float ddz, bool tempPassable)
    {
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const float candidateX = m_x + ddx;
        const float candidateZ = m_z + ddz;

        const int gx = ClampGrid(static_cast<int>(std::lround(candidateX + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(candidateZ + kWorldCenterZ)), blocksPerAxis);

        const int targetGroundY = GroundHeightAt(world, gx, gz, tempPassable);

        // Allow the move if the destination column's ground is at most
        // kStepLimit above the current feet Y (step-up); any amount lower
        // is always fine (falling is handled by the vertical pass in Step()).
        if (static_cast<float>(targetGroundY) <= m_y + kStepLimit)
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
                                  bool tempPassable)
    {
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

        const bool moving = (moveInput != 0.0f);
        if (moving)
        {
            const float dx = std::sin(m_yaw) * moveInput * effectiveMoveSpeed * dt;
            const float dz = -std::cos(m_yaw) * moveInput * effectiveMoveSpeed * dt;
            if (dx != 0.0f)
            {
                TryMoveAxis(world, dx, 0.0f, tempPassable);
            }
            if (dz != 0.0f)
            {
                TryMoveAxis(world, 0.0f, dz, tempPassable);
            }
        }

        if (m_onGround && jumpPressed && !m_ecrase)
        {
            m_velocityY = kJumpSpeed;
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

        // Wasp "balloon" status: reduced gravity while active (kBalloonGravityMultiplier's own
        // comment explains this is an approximation of "floats rather than dying").
        const float effectiveGravity = m_balloon ? kGravity * kBalloonGravityMultiplier : kGravity;
        m_velocityY = std::max(m_velocityY - effectiveGravity * dt, kFallLimit);
        float newY = m_y + m_velocityY * dt;

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = ClampGrid(static_cast<int>(std::lround(m_x + kWorldCenterX)), blocksPerAxis);
        const int gz = ClampGrid(static_cast<int>(std::lround(m_z + kWorldCenterZ)), blocksPerAxis);
        const int groundY = GroundHeightAt(world, gx, gz, tempPassable);

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
