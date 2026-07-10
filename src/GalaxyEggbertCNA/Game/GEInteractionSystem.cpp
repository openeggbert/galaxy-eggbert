#include "GEInteractionSystem.hpp"

#include <GalaxyEggbert/Worlds/Block.hpp>

#include <cmath>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        bool IsPlatformLift(ObjectType t)
        {
            return t == ObjectType::ObjectType1 || t == ObjectType::ObjectType47 ||
                   t == ObjectType::ObjectType48;
        }

        bool IsCrate(ObjectType t)
        {
            return t == ObjectType::ObjectType12;
        }

        // ObjectType2/ObjectType3 -- generic patrol hazards, real shared kill
        // list per mobile-eggbert-reference/04-enemy-behavior.md (verified
        // directly against Decor.cpp:5782-5816 for this task).
        bool IsGenericHazard(ObjectType t)
        {
            return t == ObjectType::ObjectType2 || t == ObjectType::ObjectType3;
        }

        // Matches GalaxyEggbertSimple3D's GEDecorSystem (AddTriggerSphere(0.7f)).
        constexpr float kPickupRadius = 0.7f;
        constexpr float kMaxEggCount = 10; // real mobile-eggbert MAX_EGG_COUNT (Decor.cpp:96)
        // The real contact hitbox is a tile-based rectangle overlap
        // (MoveObjectDetect), not a radius -- kPickupRadius is reused here
        // as the closest existing documented approximation, same
        // simplification already applied to every pickup type above.
        constexpr float kHazardContactRadius = kPickupRadius;
    }

    void GEInteractionSystem::Update(float dt, GEWorldRuntime& worldRuntime,
                                      float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                                      GESound& sound, bool blupiCrouching)
    {
        diedThisFrame_ = false;
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        const Worlds::World& world = worldRuntime.GetWorld();

        if (totalTreasures_ < 0)
        {
            totalTreasures_ = 0;
            for (const auto& obj : objects)
            {
                if (obj.type == ObjectType::ObjectType5)
                {
                    ++totalTreasures_;
                }
            }
        }

        bool touchingExitThisFrame = false;

        for (auto& obj : objects)
        {
            if (!obj.active)
            {
                continue;
            }

            // Platform lift patrol (ObjectType1/47/48): ping-pong between
            // posStart and posEnd at `speed` units/sec -- matches
            // GalaxyEggbertSimple3D's GEDecorSystem::Update() exactly
            // (same target-select/distance-flip/move-toward-target shape).
            if (IsPlatformLift(obj.type))
            {
                const float targetX = (obj.direction > 0.0f) ? obj.posEndX : obj.posStartX;
                const float targetY = (obj.direction > 0.0f) ? obj.posEndY : obj.posStartY;
                const float targetZ = (obj.direction > 0.0f) ? obj.posEndZ : obj.posStartZ;
                const float dx = targetX - obj.currentX;
                const float dy = targetY - obj.currentY;
                const float dz = targetZ - obj.currentZ;
                const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                if (dist < 0.05f)
                {
                    obj.direction = -obj.direction;
                }
                else
                {
                    const float step = obj.speed * dt / std::max(dist, 0.001f);
                    obj.currentX += dx * step;
                    obj.currentY += dy * step;
                    obj.currentZ += dz * step;
                }
                continue;
            }

            // Crate push (ObjectType12) -- faithful to mobile-eggbert
            // TestPushCaisse (see GalaxyEggbertSimple3D's GEDecorSystem.cpp,
            // whose own comment cites the real function name). X-axis only,
            // matching that reference exactly (mobile-eggbert is a 2D side-
            // scroller; X is its only horizontal movement axis, so a
            // Z-axis push was never a real mechanic to begin with).
            if (IsCrate(obj.type))
            {
                const float relX = obj.currentX - blupiX;
                const float relZ = obj.currentZ - blupiZ;
                const float distX = std::fabs(relX);
                const float distZ = std::fabs(relZ);
                if (distZ < 0.6f && distX > 0.3f && distX < 1.1f)
                {
                    const float pushDir = (relX > 0.0f) ? 1.0f : -1.0f;
                    if (blupiMoveDX * pushDir > 0.01f)
                    {
                        const float destX = obj.currentX + pushDir;
                        const int wx = static_cast<int>(std::round(destX)) + GEWorldRuntime::kWorldCenterX;
                        const int wz = static_cast<int>(std::round(obj.currentZ)) + GEWorldRuntime::kWorldCenterZ;
                        const int axis = static_cast<int>(world.blocksPerAxis());
                        if (wx >= 0 && wx < axis && wz >= 0 && wz < axis)
                        {
                            const bool hasFloor = !world.getBlock(static_cast<std::uint16_t>(wx), 0,
                                                                   static_cast<std::uint16_t>(wz))
                                                        .isAir();
                            if (hasFloor)
                            {
                                bool occupied = false;
                                for (const auto& other : objects)
                                {
                                    if (&other == &obj || !IsCrate(other.type))
                                    {
                                        continue;
                                    }
                                    if (std::fabs(other.currentX - destX) < 0.5f &&
                                        std::fabs(other.currentZ - obj.currentZ) < 0.5f)
                                    {
                                        occupied = true;
                                        break;
                                    }
                                }
                                if (!occupied)
                                {
                                    obj.currentX = destX;
                                }
                            }
                        }
                    }
                }
                continue;
            }

            // Generic hazard contact (ObjectType2/3, plan.md E3D-MIG-132) --
            // real shared kill list: BlupiDead(Clear1, Clear2) + the hazard
            // itself is destroyed (converted to an explosion). Type3's real
            // duck-immunity (MoveObjectDetect skips it entirely while
            // Blupi's action is Down) is modeled via blupiCrouching; type2
            // has no such immunity. Real death sound is a 50/50 coinflip
            // (BlupiDead's own Clear2 branch plays channel 74, Clear1 plays
            // nothing, per Decor.cpp:6547-6614) -- simplified to always
            // channel 74 rather than modeling the coinflip.
            if (IsGenericHazard(obj.type))
            {
                if (obj.type == ObjectType::ObjectType3 && blupiCrouching)
                {
                    continue;
                }
                const float hdx = obj.currentX - blupiX;
                const float hdy = obj.currentY - blupiY;
                const float hdz = obj.currentZ - blupiZ;
                if (hdx * hdx + hdy * hdy + hdz * hdz < kHazardContactRadius * kHazardContactRadius)
                {
                    obj.active = false;
                    LoseLife();
                    diedThisFrame_ = true;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                }
                continue;
            }

            // Pickup collection -- treasure/egg/level-exit/keys, the only
            // IsPickup() types placed in today's sample world. Sound
            // channels and removal behavior come from `mobile-eggbert-
            // reference/13-object-pickups.md`/`07-sounds.md`, not from
            // GalaxyEggbertSimple3D's GESound shortcuts (PlayCollect/
            // PlayKey/PlayLife) -- that spec is the more carefully-verified
            // source and corrects 2 real mistakes the shortcuts would have
            // propagated: treasure/key pickup is channel 11 (or 19 for the
            // set-completing treasure), not channel 10, and egg pickup is
            // channel 3, not channel 42 (42 is Shield activation, unrelated).
            if (obj.type != ObjectType::ObjectType5 && obj.type != ObjectType::ObjectType6 &&
                obj.type != ObjectType::ObjectType7 && obj.type != ObjectType::ObjectType49 &&
                obj.type != ObjectType::ObjectType50 && obj.type != ObjectType::ObjectType51)
            {
                continue;
            }

            const float dx = obj.currentX - blupiX;
            const float dy = obj.currentY - blupiY;
            const float dz = obj.currentZ - blupiZ;
            const bool touching = (dx * dx + dy * dy + dz * dz) < kPickupRadius * kPickupRadius;

            if (obj.type == ObjectType::ObjectType7)
            {
                // Level-exit goal: debounced to fire once per contact
                // "session" (real mobile-eggbert re-checks every 50 ticks
                // of continued contact via m_goalPhase; approximated here
                // as "once per entry" rather than exactly replicating that
                // tick-based re-fire, which needs no equivalent without a
                // HUD/rejection-sound spam concern this simpler debounce
                // already avoids). Gated on having all treasure, per
                // `13-object-pickups.md`'s real m_nbTresor >= m_totalTresor
                // check -- win fanfare (channel 14) if so, rejection sound
                // (channel 13) if not. Never removed (active stays true) --
                // it's a goal marker you touch, not a consumable pickup.
                if (touching)
                {
                    touchingExitThisFrame = true;
                    if (!exitContactActive_)
                    {
                        exitContactActive_ = true;
                        if (treasuresCollected_ >= totalTreasures_)
                        {
                            exitReached_ = true;
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel14);
                        }
                        else
                        {
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel13);
                        }
                    }
                }
                continue;
            }

            if (!touching)
            {
                continue;
            }

            // Every other pickup type here is deleted immediately on
            // contact per `13-object-pickups.md`'s "Voyage" section ("the
            // world object is deleted immediately, and a voyage animation
            // is [flown to the HUD]") -- no HUD-fly animation exists here,
            // just the immediate removal + reward + sound.
            switch (obj.type)
            {
                case ObjectType::ObjectType5: // treasure
                {
                    const bool completesSet = (treasuresCollected_ + 1 >= totalTreasures_);
                    ++treasuresCollected_;
                    sound.Play(completesSet ? GalaxyEggbert::SoundChannel::SoundChannel19
                                             : GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                }
                case ObjectType::ObjectType6: // extra-life egg
                    // Real MAX_EGG_COUNT=10 gate: at the cap, touching an
                    // egg does nothing at all -- not even removed.
                    if (lifeEggCount_ < kMaxEggCount)
                    {
                        ++lifeEggCount_;
                        ++lives_;
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                        obj.active = false;
                    }
                    break;
                case ObjectType::ObjectType49: // key 1
                    ++keys1_;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                case ObjectType::ObjectType50: // key 2
                    ++keys2_;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                case ObjectType::ObjectType51: // key 3
                    ++keys3_;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                    obj.active = false;
                    break;
                default:
                    break;
            }
        }

        if (!touchingExitThisFrame)
        {
            exitContactActive_ = false;
        }
    }

    void GEInteractionSystem::LoseLife()
    {
        --lives_;
        if (lives_ <= 0)
        {
            // Real DoorsLost() (Decor.cpp:11716) resets m_nbVies back to 3
            // on game-over rather than a permanent depletion.
            lives_ = 3;
            ++gameOverCount_;
        }
    }
}
