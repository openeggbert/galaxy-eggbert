#include "GEInteractionSystem.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
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

        // INFRA-006 pilot (plan.md §7, `REMAKE-ANALYSIS.md` P1-2): the first
        // object-type family migrated to a per-type handler table, replacing
        // the 5 near-identical `case` bodies this used to be (see
        // `TryGrantSecretPowerPickup()` below) with one lookup + one generic
        // dispatch -- "add/fix a 6th secret-power pickup" now touches only
        // this table + the small `Gate` switch below, not the god-method's
        // own case list. Deliberately narrow scope for this first migrated
        // family: 2 OTHER dispatch sites in this file reference these same
        // 5 types (the dynamite-blast destructible-type list, ~27 types; the
        // pickup-touch-radius gate, ~13 types) but are cross-cutting
        // concepts spanning many MORE types than just this family -- fully
        // replacing those would need the other families sharing them
        // migrated too, so they're left as open-coded lists for now, a
        // deliberate scope boundary for a first pilot, not an oversight.
        struct SecretPowerPickupHandler
        {
            ObjectType type;
            // Which of Update()'s own caller-supplied blupiCanGrantX gates
            // applies -- an enum, not a function pointer, since every gate
            // is a plain bool parameter, not independent logic.
            enum class Gate : std::uint8_t { Shield, Power, Cloud, Hide, Invert } gate;
            // Sucette/Drink need the real action-button edge
            // (blupiActionPressedEdge, Decor.cpp:6025/6053); Shield/Charge/
            // Invert grant automatically on contact.
            bool requiresActionButton;
            GEInteractionSystem::EventKind grantedEvent;
            // Only Power/Cloud/Hide carry the pickup-position payload their
            // real 2-stage grab/freeze/complete delay needs (see
            // GEInteractionSystem::EventKind's own comment) -- Shield/Invert
            // grant instantly, no position needed.
            bool hasPositionPayload;
        };
        constexpr SecretPowerPickupHandler kSecretPowerPickupHandlers[] = {
            {ObjectType::ObjectType25, SecretPowerPickupHandler::Gate::Shield, false,
             GEInteractionSystem::EventKind::ShieldGranted, false},
            {ObjectType::ObjectType26, SecretPowerPickupHandler::Gate::Power, true,
             GEInteractionSystem::EventKind::PowerGranted, true},
            {ObjectType::ObjectType30, SecretPowerPickupHandler::Gate::Hide, true,
             GEInteractionSystem::EventKind::HideGranted, true},
            {ObjectType::ObjectType31, SecretPowerPickupHandler::Gate::Cloud, false,
             GEInteractionSystem::EventKind::CloudGranted, true},
            {ObjectType::ObjectType40, SecretPowerPickupHandler::Gate::Invert, false,
             GEInteractionSystem::EventKind::InvertGranted, false},
        };

        // INFRA-006 2nd family (plan.md §7, `REMAKE-ANALYSIS.md` P1-2): the
        // 17 purely-cosmetic particle/effect types that just self-expire
        // after a fixed `phase` lifetime, replacing what used to be 17
        // near-identical `if (obj.type == ObjectTypeN)` blocks (~190 lines)
        // with one table + one dispatch (`TryTickExpiringParticle()` below).
        // Chosen as the pilot's follow-up for the same low-risk reason the
        // pilot itself was: no interaction with Blupi, no kill/hazard logic.
        //
        // Two real, DIFFERENT shapes existed in the original code, preserved
        // here via `alwaysContinueEvenBeforeExpiry` per entry -- collapsing
        // them into one shape would silently change behavior:
        //   - `false` (10 types): the original `if (type==N && phase>=T)`
        //     only matched (and only ever `continue`d) once actually
        //     expired -- while still alive, execution fell through to the
        //     shared `AdvancePatrolStep()` call further down. Load-bearing
        //     for ObjectType36/39/41/42 (real posStart->posEnd slides);
        //     harmless-but-preserved for ObjectType27/57/92/98/99/100
        //     (static markers where that call is already a no-op).
        //   - `true` (7 types): the original `if (type==N) { ...; continue; }`
        //     always continued regardless of expiry state, deliberately
        //     never reaching `AdvancePatrolStep()` at all.
        struct ExpiringParticleHandler
        {
            ObjectType type;
            float expiryPhase;
            bool alwaysContinueEvenBeforeExpiry;
        };
        constexpr ExpiringParticleHandler kExpiringParticleHandlers[] = {
            // Always-continue family (never reaches AdvancePatrolStep()).
            {ObjectType::ObjectType11, 9.0f, true},   // CAM-009-adjacent fan-hit shockwave flash
            {ObjectType::ObjectType14, 14.0f, true},  // PICKUP-078/080 water splash (Plouf)
            {ObjectType::ObjectType35, 6.0f, true},   // PICKUP-078/080 water splash (Tiplouf)
            {ObjectType::ObjectType8, 39.0f, true},   // VISUAL-008 dynamite-blast explosion flash
            {ObjectType::ObjectType10, 20.0f, true},  // VISUAL-008 fish/bird explosion flash
            {ObjectType::ObjectType93, 5.0f, true},   // Clear3Ascend Lava-death puff ("158" death-VFX)
            {ObjectType::ObjectType9, 20.0f, true},   // VISUAL-008 follower-blocked-path debris flash
            // Falls-through-pre-expiry family (real slide types + static markers coded the same way).
            {ObjectType::ObjectType41, 16.0f, false}, // VISUAL-014/015 Invert start burst (real slide)
            {ObjectType::ObjectType42, 16.0f, false}, // VISUAL-014/015 Invert stop burst (real slide)
            {ObjectType::ObjectType39, 11.0f, false}, // VISUAL-012 treasure/collectible sparkle burst (real slide)
            {ObjectType::ObjectType36, 16.0f, false}, // VISUAL-013 Pollution puff (real slide)
            {ObjectType::ObjectType57, 20.0f, false},  // VISUAL-011-adjacent Shield magic trail (static)
            {ObjectType::ObjectType27, 24.0f, false},  // VISUAL-011-adjacent Power magic trail (static)
            {ObjectType::ObjectType98, 10.0f, false},  // VISUAL-009 bullet-hit splat, small (static)
            {ObjectType::ObjectType99, 13.0f, false},  // VISUAL-009 bullet-hit splat, medium (static)
            {ObjectType::ObjectType100, 18.0f, false}, // VISUAL-009 bullet-hit splat, large (static)
            {ObjectType::ObjectType92, 128.0f, false}, // VISUAL-010 teleporter arc (static)
        };

        // Returns true if the caller should `continue` the per-object loop
        // (obj.type is a member of this family, either just expired-and-
        // deactivated or a pre-expiry state that never needs anything below
        // this point); false if obj.type isn't in this family at all, OR is
        // a still-alive `alwaysContinueEvenBeforeExpiry=false` entry that
        // must fall through to the shared `AdvancePatrolStep()` call.
        bool TryTickExpiringParticle(MobileObjSpec& obj)
        {
            for (const auto& handler : kExpiringParticleHandlers)
            {
                if (handler.type != obj.type)
                {
                    continue;
                }
                const bool expired = obj.phase >= handler.expiryPhase;
                if (expired)
                {
                    obj.active = false;
                }
                return handler.alwaysContinueEvenBeforeExpiry || expired;
            }
            return false;
        }

        // INFRA-006 3rd family (plan.md §7): the 5 "basic" pickups that share
        // one real shape -- self-delete on contact, defer their reward to
        // voyage completion (plan.md `158`), and either need no gate at all
        // (Treasure/Key1/2/3) or one plain caller-supplied bool gate
        // (Dynamite, same "Gate is a plain bool, not independent logic"
        // pattern as the secret-power-pickup pilot). New
        // `TryCollectBasicPickup()` below replaces these 5 near-identical
        // `case` bodies with one lookup + one dispatch.
        //
        // Deliberately excluded from this table (kept as their own case
        // bodies, not an oversight): Egg(6) computes its voyage endpoint X
        // from LIVE state (`lifeEggCount_`) at grant time -- not expressible
        // as fixed table data without a per-entry function hook, which would
        // cost more real complexity than a single type is worth collapsing.
        // BulletPack(29) has a real immediate side effect
        // (`bulletCount_ = kBulletCap`) BEFORE deactivating, unlike every
        // other pickup here (whose reward is deferred to voyage completion)
        // -- same reasoning, left open-coded. This mirrors the pilot's own
        // "table the uniform majority, leave genuinely special cases
        // open-coded" precedent rather than forcing a false-generic shape.
        struct BasicPickupHandler
        {
            ObjectType type;
            GEInteractionSystem::VoyageKind voyageKind;
            int voyageIcon;
            float endX;
            float endY;
            // Only Dynamite sets this -- Treasure/Key1/2/3 are always granted
            // on contact, matching the real source exactly.
            bool requiresDynamiteGate;
            bool spawnsSparkleBurst;
        };
        constexpr BasicPickupHandler kBasicPickupHandlers[] = {
            {ObjectType::ObjectType5, GEInteractionSystem::VoyageKind::Treasure, 6, 430.0f, 430.0f, false, true},    // treasure, Decor.cpp:5956
            {ObjectType::ObjectType49, GEInteractionSystem::VoyageKind::Key1, 215, 520.0f, 418.0f, false, true},     // key 1, Decor.cpp:5971
            {ObjectType::ObjectType50, GEInteractionSystem::VoyageKind::Key2, 222, 530.0f, 418.0f, false, true},     // key 2, Decor.cpp:5986
            {ObjectType::ObjectType51, GEInteractionSystem::VoyageKind::Key3, 229, 540.0f, 418.0f, false, true},     // key 3, Decor.cpp:6001
            {ObjectType::ObjectType55, GEInteractionSystem::VoyageKind::Dynamite, 252, 505.0f, 414.0f, true, false}, // dynamite, Decor.cpp:6123-6125
        };

        // Opens a door tile (plan.md E3D-MIG-160/162, real `Decor::OpenDoor`,
        // ~11667): removes the tile from the terrain grid (becomes passable)
        // and spawns a transient ObjectType22 at that cell (real
        // `table_bridge`-style slide, handled by this file's own
        // ObjectType22 branch in the main Update() loop, not the generic
        // patrol system) so it visually slides up and out of the way
        // instead of just vanishing. Real channel 33. Shared by both the
        // key-gated (160) and treasure-gated (162) door families --
        // opening itself is identical, only the trigger condition differs.
        // The door's own icon is NOT carried onto the slide object --
        // `GEObjectIcons::GetObjIcon()` already has no confirmed icon data
        // for type 22 (returns 0 regardless), a pre-existing gap unrelated
        // to this task.
        void OpenDoorAt(GEWorldRuntime& worldRuntime, int gx, int gy, int gz, GESound& sound)
        {
            worldRuntime.GetWorldMutable().setBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                      static_cast<std::uint16_t>(gz),
                                                      Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));

            MobileObjSpec slide;
            slide.type = ObjectType::ObjectType22;
            slide.posStartX = slide.posEndX = slide.currentX =
                static_cast<float>(gx) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
            slide.posStartY = slide.posEndY = slide.currentY = static_cast<float>(gy);
            slide.posStartZ = slide.posEndZ = slide.currentZ =
                static_cast<float>(gz) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
            slide.phase = 0.0f;
            slide.active = true;

            auto& objects = worldRuntime.GetMobileObjectsMutable();
            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = slide;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(slide);
            }
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel33);
        }

        // Real Decor::OpenDoorsTresor (~11642): scans the whole grid,
        // opening every treasure-gated door (icon 421+N) whose
        // requirement N is now met. Shared by the real per-pickup trigger
        // (called from ApplyVoyageReward() the instant a treasure voyage
        // completes, plan.md `158`) and
        // GEInteractionSystem::CheatAllTreasure() below, which needs the
        // exact same scan after crediting every treasure at once.
        void ScanAndOpenTreasureDoors(GEWorldRuntime& worldRuntime, int treasuresCollected, GESound& sound)
        {
            auto& terrain = worldRuntime.GetWorldMutable();
            const int axis = static_cast<int>(terrain.blocksPerAxis());
            for (int gx = 0; gx < axis; ++gx)
            {
                for (int gy = 0; gy < axis; ++gy)
                {
                    for (int gz = 0; gz < axis; ++gz)
                    {
                        const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                            static_cast<std::uint16_t>(gz))
                                               .type();
                        if (icon >= 421 && icon <= 420 + treasuresCollected)
                        {
                            OpenDoorAt(worldRuntime, gx, gy, gz, sound);
                        }
                    }
                }
            }
        }

        // Real shared patrol-turn mechanic (plan.md E3D-MIG-131,
        // `Decor::MoveObjectStepLine`, verified directly against
        // Decor.cpp:8005-8141): a 4-phase cycle -- 1=dwell at posStart for
        // timeStopStartTicks, 2=advance to posEnd over stepAdvanceTicks,
        // 3=dwell at posEnd for timeStopEndTicks, 4=recede back over
        // stepRecedeTicks, then loop to 1. The real position update is
        // linear interpolation on the elapsed-time fraction (not per-tick
        // accumulation), so it can't drift -- reproduced here with a
        // normalized `t` in [0,1] rather than the real integer pixel math,
        // same effect. Ticks are at the 20Hz reference rate (same
        // convention as MobileObjSpec::phase). Applies to every MoveObject
        // EXCEPT platform lifts/crates, which keep their own existing
        // speed-based ping-pong patrol (see the two branches above) --
        // callers gate that exclusion, not this function. A no-op if
        // posStart==posEnd is the caller's responsibility too (matches the
        // real guard exactly -- see Update()'s call site).
        constexpr float kTicksPerSecond = 20.0f;

        void AdvancePatrolStep(MobileObjSpec& obj, float dt)
        {
            const float dtTicks = dt * kTicksPerSecond;
            switch (obj.patrolStep)
            {
                case 1: // dwell at posStart
                    obj.patrolTime += dtTicks;
                    if (obj.patrolTime >= obj.timeStopStartTicks)
                    {
                        obj.patrolStep = 2;
                        obj.patrolTime = 0.0f;
                    }
                    break;
                case 2: // advance posStart -> posEnd
                {
                    obj.patrolTime += dtTicks;
                    const float t = (obj.stepAdvanceTicks > 0.0f)
                        ? std::min(obj.patrolTime / obj.stepAdvanceTicks, 1.0f) : 1.0f;
                    obj.currentX = obj.posStartX + (obj.posEndX - obj.posStartX) * t;
                    obj.currentY = obj.posStartY + (obj.posEndY - obj.posStartY) * t;
                    obj.currentZ = obj.posStartZ + (obj.posEndZ - obj.posStartZ) * t;
                    if (t >= 1.0f)
                    {
                        // Real per-type special case at this exact junction
                        // (Decor.cpp:8095-8098): a fired projectile
                        // (ObjectType23) does NOT dwell at posEnd like every
                        // other MoveObject -- it self-destructs the instant
                        // it reaches the end of its pre-computed clear path
                        // (real: type reset to ObjectType0; here: active=false,
                        // this class's existing "destroyed" convention). Real
                        // source treats the rising water bubble (ObjectType15,
                        // plan.md PICKUP-079) identically at this same exact
                        // junction (`|| m_moveObject[i].type ==
                        // ObjectType::ObjectType15` in the real condition) --
                        // it self-deletes on reaching the surface rather than
                        // dwelling there, found 2026-07-17.
                        if (obj.type == ObjectType::ObjectType23 || obj.type == ObjectType::ObjectType15)
                        {
                            obj.active = false;
                        }
                        else if (obj.type == ObjectType::ObjectType34)
                        {
                            // Real Decor.cpp:8099-8104 (VISUAL-016) -- the
                            // goo particle "sticks to geometry": unlike
                            // every other MoveObject, which just starts
                            // dwelling at posEnd (and later recedes back via
                            // step 4), this type also collapses its own
                            // posStart/posEnd onto its landing spot, so it
                            // never recedes -- it's stuck there permanently.
                            // Not exercised by any placed content in real
                            // mobile-eggbert (no world file places type 34)
                            // or this project's own worlds; ported for
                            // correctness in case a future custom/edited
                            // world ever does.
                            obj.posStartX = obj.posEndX = obj.currentX;
                            obj.posStartY = obj.posEndY = obj.currentY;
                            obj.posStartZ = obj.posEndZ = obj.currentZ;
                            obj.patrolStep = 3;
                            obj.patrolTime = 0.0f;
                        }
                        else
                        {
                            obj.patrolStep = 3;
                            obj.patrolTime = 0.0f;
                        }
                    }
                    break;
                }
                case 3: // dwell at posEnd
                    obj.patrolTime += dtTicks;
                    if (obj.patrolTime >= obj.timeStopEndTicks)
                    {
                        obj.patrolStep = 4;
                        obj.patrolTime = 0.0f;
                    }
                    break;
                case 4: // recede posEnd -> posStart
                {
                    obj.patrolTime += dtTicks;
                    const float t = (obj.stepRecedeTicks > 0.0f)
                        ? std::min(obj.patrolTime / obj.stepRecedeTicks, 1.0f) : 1.0f;
                    obj.currentX = obj.posEndX + (obj.posStartX - obj.posEndX) * t;
                    obj.currentY = obj.posEndY + (obj.posStartY - obj.posEndY) * t;
                    obj.currentZ = obj.posEndZ + (obj.posStartZ - obj.posEndZ) * t;
                    if (t >= 1.0f)
                    {
                        obj.patrolStep = 1;
                        obj.patrolTime = 0.0f;
                    }
                    break;
                }
                default:
                    obj.patrolStep = 1;
                    obj.patrolTime = 0.0f;
                    break;
            }
        }

        // The real shared kill list (Decor.cpp:5782-5816, verified directly
        // against source for this task): ObjectType2/3 (generic patrol
        // hazards), 4 (bulldozer), 16 (spider), 17 (fish), 20 (bird), 96/97
        // (follower, both dormant and awake) all use the exact same contact-
        // death check -- the only difference in that source block is purely
        // cosmetic (17/20 get a bigger screen-shake + a different explosion
        // ObjectType than the rest), not a behavioral difference in whether
        // or how Blupi dies, so this deliberately does NOT split them into
        // separate per-type checks. Real per-type quirks that ARE modeled:
        // type3's duck-immunity (see the blupiCrouching check below). Real
        // per-type quirks NOT modeled: type17/20's bigger explosion effect
        // (cosmetic), type2's wider "thrown object" anticipation box and
        // taunt-suppression (cosmetic/reaction polish), type16's always-
        // self-destroys/no-turn-table framing (already true here since
        // every hazard here is destroyed on contact), follower 96/97's real
        // homing-toward-Blupi movement AI (a separate, NOT-yet-implemented
        // feature from this contact-death check -- an un-homing follower
        // still correctly kills Blupi on contact if he touches it).
        bool IsGenericHazard(ObjectType t)
        {
            switch (t)
            {
                case ObjectType::ObjectType2:
                case ObjectType::ObjectType3:
                case ObjectType::ObjectType4:
                case ObjectType::ObjectType16:
                case ObjectType::ObjectType17:
                case ObjectType::ObjectType20:
                case ObjectType::ObjectType96:
                case ObjectType::ObjectType97:
                    return true;
                default:
                    return false;
            }
        }

        // Real balloon-pop subset (Decor.cpp:5766-5781): exactly types
        // 3/16/96/97, NOT the full 8-type IsGenericHazard() list -- 2/4/17/20
        // still kill Blupi even while ballooned, per the real source's
        // if/else-if chain (the pop check comes first and is mutually
        // exclusive with the kill check; only these 4 types are ever
        // eligible for the pop branch at all).
        bool IsBalloonPoppableHazard(ObjectType t)
        {
            switch (t)
            {
                case ObjectType::ObjectType3:
                case ObjectType::ObjectType16:
                case ObjectType::ObjectType96:
                case ObjectType::ObjectType97:
                    return true;
                default:
                    return false;
            }
        }

        // True once during the frame `prevTicks` crosses `threshold` --
        // real mobile-eggbert fires blupih/blupit's shots on an exact tick
        // equality (`time == Config::ScaleTime(N)`), which a continuously-
        // accumulated float can step past without ever equaling; this is
        // the same "fires exactly once" edge-detection shape used
        // elsewhere in this codebase for tick-gated events.
        bool CrossedTick(float prevTicks, float dtTicks, float threshold)
        {
            return prevTicks < threshold && (prevTicks + dtTicks) >= threshold;
        }

        // Real ObjectStart's SearchDistRight-driven travel distance
        // (Decor.cpp:7794-7869, "@note" comment): casts from
        // (gx,gy,gz) one cell at a time in direction (dx,dy,dz), counting
        // consecutive AIR cells until the first solid cell or the world
        // edge. Returns 0 if the very first stepped-to cell is already
        // solid (the real "num3==0" case) -- the caller cancels the shot
        // (no visible projectile) but, per the real ObjectStart code, still
        // plays the attack sound (its `!= -1` sound gate only checks for a
        // free object-pool slot, never whether the raycast found room to
        // travel -- `ObjectStart` returns a valid, non -1 index even on
        // this early-cancel path). One grid cell here == one real 64px
        // mobile-eggbert tile, matching this file's existing world<->grid
        // conversion convention (see GEWorldRuntime's kMobileTileSize use).
        int SearchAirDistance(const Worlds::World& world, int gx, int gy, int gz, int dx, int dy, int dz)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            int x = gx;
            int y = gy;
            int z = gz;
            int dist = 0;
            while (true)
            {
                x += dx;
                y += dy;
                z += dz;
                if (x < 0 || x >= blocksPerAxis || y < 0 || y >= blocksPerAxis || z < 0 || z >= blocksPerAxis)
                {
                    break;
                }
                if (!world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                     static_cast<std::uint16_t>(z)).isAir())
                {
                    break;
                }
                ++dist;
            }
            return dist;
        }

        // Converts a live object's world-space position to a clamped grid
        // cell, matching GEBlupiController's own kWorldCenterX/Z + lround
        // convention exactly (duplicated here rather than exposed from
        // GEBlupiController, which keeps its own version private).
        void ToGridCell(const Worlds::World& world, float wx, float wy, float wz, int& gx, int& gy, int& gz)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            gx = std::clamp(static_cast<int>(std::lround(wx)) + GEWorldRuntime::kWorldCenterX, 0, blocksPerAxis - 1);
            gy = std::clamp(static_cast<int>(std::lround(wy)), 0, blocksPerAxis - 1);
            gz = std::clamp(static_cast<int>(std::lround(wz)) + GEWorldRuntime::kWorldCenterZ, 0, blocksPerAxis - 1);
        }

        // Matches GEBlupiController::IsSolidAt's own logic exactly
        // (duplicated here rather than exposed from GEBlupiController,
        // which keeps it private -- same reason ToGridCell above is its
        // own copy rather than a shared call).
        bool IsSolidAt(const Worlds::World& world, int gx, int gy, int gz)
        {
            return !world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                    static_cast<std::uint16_t>(gz)).isAir();
        }

        // Spawns a fired-projectile MobileObjSpec (ObjectType23) travelling
        // `dist` grid cells from `obj`'s own position in direction
        // (dirX,dirY,dirZ) (a unit axis vector) -- shared by blupih (straight
        // down) and blupit (horizontal) below. stepAdvanceTicks reuses the
        // real ObjectStart formula directly (Decor.cpp:7866:
        // `ScaleTime(abs(speed*dist/64))`, speed magnitude 5 for both real
        // callers here): since one grid cell already equals one real 64px
        // tile, `dist` (in cells) IS the real formula's `dist/64` term, so
        // `5.0f * dist` ticks is a direct, non-approximated transcription,
        // not an invented pacing constant. patrolStep starts at 2 (advance),
        // skipping the dwell entirely, matching real ObjectStart's own
        // `step=2; time=0` initialization (a fresh shot never dwells at its
        // spawn point).
        MobileObjSpec MakeBullet(const MobileObjSpec& obj, float dirX, float dirY, float dirZ, int dist)
        {
            MobileObjSpec bullet;
            bullet.type = ObjectType::ObjectType23;
            bullet.posStartX = bullet.currentX = obj.currentX;
            bullet.posStartY = bullet.currentY = obj.currentY;
            bullet.posStartZ = bullet.currentZ = obj.currentZ;
            bullet.posEndX = obj.currentX + dirX * static_cast<float>(dist);
            bullet.posEndY = obj.currentY + dirY * static_cast<float>(dist);
            bullet.posEndZ = obj.currentZ + dirZ * static_cast<float>(dist);
            bullet.patrolStep = 2;
            bullet.patrolTime = 0.0f;
            bullet.stepAdvanceTicks = 5.0f * static_cast<float>(dist);
            return bullet;
        }

        // Treasure/collectible sparkle burst (plan.md VISUAL-012,
        // ObjectType39) -- confirmed via a direct source read
        // (`Decor.cpp:5948-5960`, the real ObjectType5/treasure-collect
        // site): 4 `ObjectStart(pos, ObjectType39, speed)` calls
        // (`speed ∈ {-60,60,10,-10}`, encoding up/down/+X/-X) at the
        // collected object's own position, no pre-offset -- same real
        // `SearchDistRight()` short-circuit as Invert's burst
        // (`Decor.cpp:7628-7653`, flat 500 real-px for this ObjectType
        // too), same 64px-per-tile conversion, same real screen-Y-to-
        // world-Y sign flip. Appends directly to pendingSpawns since,
        // unlike Invert's grant/expiry (fired from the game class after
        // this class's own Update() has already returned), the real
        // treasure-collect site is INSIDE this class's own per-object
        // loop, so the existing deferred-spawn flush at the end of
        // Update() already covers it.
        void AppendSparkleBurst(float x, float y, float z, std::vector<MobileObjSpec>& pendingSpawns)
        {
            // Real behavior (`Decor::ObjectStart()`, same as the Invert
            // burst's own updated comment) -- spawns exactly AT the
            // collected object's position (no pre-offset, matching
            // Invert's GRANT shape) and slides toward a 500px-out posEnd
            // over 78 ticks, self-deleting at phase>=11 (see below) long
            // before actually arriving.
            constexpr float kReach = 500.0f / 64.0f;
            const float dirs[4][3] = {
                {0.0f, 1.0f, 0.0f},  // up
                {0.0f, -1.0f, 0.0f}, // down
                {1.0f, 0.0f, 0.0f},  // +X (real "right")
                {-1.0f, 0.0f, 0.0f}, // -X (real "left")
            };
            for (const auto& dir : dirs)
            {
                MobileObjSpec spec;
                spec.type = ObjectType::ObjectType39;
                spec.active = true;
                spec.phase = 0.0f;
                spec.currentX = spec.posStartX = x;
                spec.currentY = spec.posStartY = y;
                spec.currentZ = spec.posStartZ = z;
                spec.posEndX = x + dir[0] * kReach;
                spec.posEndY = y + dir[1] * kReach;
                spec.posEndZ = z + dir[2] * kReach;
                spec.patrolStep = 2; // real step=2, matches SpawnInvertBurst()'s own spawn
                spec.patrolTime = 0.0f;
                spec.stepAdvanceTicks = 78.0f;
                pendingSpawns.push_back(spec);
            }
        }

        // Explosion flash (plan.md VISUAL-008, ObjectType8 SmallShake /
        // ObjectType10 BigShake) -- a single instance spawned exactly at
        // the given position, no offset (real `ObjectStart(pos, type, 0)`
        // -- speed=0 means no direction/offset encoding, same reasoning
        // as `SpawnFanHitFlash()`). Two real, independent trigger sites
        // share this exact spawn shape: `Decor::DynamiteStart()`
        // (`Decor.cpp:9065`, ObjectType8, once per blast in the 9-blast
        // sequence) and the generic-hazard contact-kill site
        // (`Decor.cpp:5797-5816`, already this engine's own existing
        // `IsGenericHazard()` block, plan.md CAM-008/009 -- ObjectType10
        // for fish/17 and bird/20 specifically, paired with the
        // already-wired BigShake; ObjectType8 for every other hazard
        // type, paired with SmallShake). Appends directly to pendingSpawns
        // since both real spawn sites are INSIDE this class's own
        // per-object loop.
        void AppendExplosionFlash(ObjectType type, float x, float y, float z, std::vector<MobileObjSpec>& pendingSpawns)
        {
            MobileObjSpec spec;
            spec.type = type;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = spec.posEndX = x;
            spec.currentY = spec.posStartY = spec.posEndY = y;
            spec.currentZ = spec.posStartZ = spec.posEndZ = z;
            pendingSpawns.push_back(spec);
        }

        // Bullet-hit splat effect (plan.md VISUAL-009, ObjectType98/99/100
        // -- despite ObjectType.hpp's own "water splash" doc comments,
        // the ONLY real spawn site is `Decor::StartSploutchGlu()`
        // (`Decor.cpp:7763-7789`), called exactly once, from the
        // ObjectType23-bullet-kills-Blupi block (`Decor.cpp:5914-5947`,
        // already this class's own `ObjectType23` contact-death branch) --
        // a scattered 7-instance splat (1x98, 4x99, 2x100), each at a
        // small fixed real-pixel offset from the bullet's own position,
        // all `speed=0` (static, no offset/interpolation, same shape as
        // `SpawnFanHitFlash()`). Real self-delete: phase>=10 (98), >=13
        // (99), >=18 (100). Appends directly to pendingSpawns since, like
        // the treasure sparkle above, the real spawn site is INSIDE this
        // class's own per-object loop.
        void AppendSplatEffect(float x, float y, float z, std::vector<MobileObjSpec>& pendingSpawns)
        {
            struct Splat { ObjectType type; float dx, dy; };
            static constexpr float k = 64.0f; // real px-per-tile scale
            static const Splat splats[7] = {
                {ObjectType::ObjectType98, 0.0f / k, 0.0f / k},
                {ObjectType::ObjectType99, 15.0f / k, -20.0f / k},
                {ObjectType::ObjectType99, -20.0f / k, -18.0f / k},
                {ObjectType::ObjectType99, 23.0f / k, 18.0f / k},
                {ObjectType::ObjectType99, -15.0f / k, 18.0f / k},
                {ObjectType::ObjectType100, 32.0f / k, -10.0f / k},
                {ObjectType::ObjectType100, -28.0f / k, -15.0f / k},
            };
            for (const auto& splat : splats)
            {
                MobileObjSpec spec;
                spec.type = splat.type;
                spec.active = true;
                spec.phase = 0.0f;
                spec.currentX = spec.posStartX = spec.posEndX = x + splat.dx;
                spec.currentY = spec.posStartY = spec.posEndY = y + splat.dy;
                spec.currentZ = spec.posStartZ = spec.posEndZ = z;
                pendingSpawns.push_back(spec);
            }
        }

        // Blupih (ObjectType32, plan.md E3D-MIG-134) attack: verified
        // directly against Decor.cpp:8878-8886 -- during a turn-dwell
        // (step 1 or 3), at dwell-frame 21 exactly, drops one ObjectType23
        // straight down via `ObjectStart(pos, ObjectType23, 55)` (the real
        // speed encoding's >50 branch always means "straight down",
        // magnitude 55-50=5 -- see SearchAirDistance's own comment for why
        // aiming at Blupi is never modeled: it isn't real behavior).
        void FireBlupihShot(const MobileObjSpec& obj, const Worlds::World& world, GESound& sound,
                             std::vector<MobileObjSpec>& pendingSpawns)
        {
            int gx, gy, gz;
            ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, gx, gy, gz);
            const int dist = SearchAirDistance(world, gx, gy, gz, 0, -1, 0);
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel52);
            if (dist > 0)
            {
                pendingSpawns.push_back(MakeBullet(obj, 0.0f, -1.0f, 0.0f, dist));
            }
        }

        // Blupit (ObjectType33, plan.md E3D-MIG-134) attack: verified
        // directly against Decor.cpp:8928-8969 -- fires two horizontal
        // shots per turn-dwell bracketing the turn, one at dwell-frame 3,
        // one at dwell-frame 21 (see the two CrossedTick() call sites in
        // Update() below, which pick `dirXSign` for each per the real
        // if/else condition and pass it here). Just the raycast+spawn+sound
        // half of the real ObjectStart call, mirroring FireBlupihShot's
        // shape for the horizontal case.
        void FireBlupitShot(const MobileObjSpec& obj, float dirXSign, const Worlds::World& world,
                             GESound& sound, std::vector<MobileObjSpec>& pendingSpawns)
        {
            int gx, gy, gz;
            ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, gx, gy, gz);
            const int dirSign = (dirXSign > 0.0f) ? 1 : -1;
            const int dist = SearchAirDistance(world, gx, gy, gz, dirSign, 0, 0);
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel52);
            if (dist > 0)
            {
                pendingSpawns.push_back(MakeBullet(obj, dirXSign, 0.0f, 0.0f, dist));
            }
        }

        // Matches GalaxyEggbertSimple3D's GEDecorSystem (AddTriggerSphere(0.7f)).
        constexpr float kPickupRadius = 0.7f;
        constexpr float kMaxEggCount = 10; // real mobile-eggbert MAX_EGG_COUNT (Decor.cpp:96)
        // The real contact hitbox is a tile-based rectangle overlap
        // (MoveObjectDetect), not a radius -- kPickupRadius is reused here
        // as the closest existing documented approximation, same
        // simplification already applied to every pickup type above.
        constexpr float kHazardContactRadius = kPickupRadius;

        // Real `BlupiElectro` aura (plan.md `068`, Decor.cpp:9610-9638,
        // mobile-eggbert-reference/10-blupi-mechanics.md §9): while
        // `m_blupiCloud` (this engine's SecretPower::Cloud) is active,
        // instantly destroys small enemies within 40px of Blupi's own
        // box -- an offensive aura Blupi carries, unrelated to the
        // `Blitz` lightning HAZARD despite the similarly-named real
        // function. Real check expands Blupi's own box by 40px; modeled
        // as a circular radius (same simplification as every other
        // proximity test in this file) combining that 40px real
        // expansion with a 32px half-tile (Blupi's own box is measured
        // from its center here, not its edge) -- same "half-tile + real
        // px offset, /64" combination already used for
        // kFollowerWakeRadius below.
        constexpr float kCloudAuraRadius = (40.0f + 32.0f) / 64.0f;

        // Follower wake box (ObjectType96, plan.md E3D-MIG-137, real
        // Decor.cpp:9646-9678 MoveObjectFollow): the real check is an
        // axis-aligned rect test (the follower's own tile padded +-100px
        // on all sides vs. a narrow ~28px-wide strip through Blupi's
        // default hitbox) -- approximated here as a circular distance
        // check, same simplification as every other proximity test in
        // this file. 100px real padding + a 32px half-tile (the real rect
        // is measured from the follower's TILE edges, not its center)
        // converts to grid units via the same 64px/cell convention used
        // throughout this file (SearchAirDistance etc.).
        constexpr float kFollowerWakeRadius = (100.0f + 32.0f) / 64.0f;

        // Real follower homing speed (ObjectType97, Decor.cpp:8025-8044):
        // exactly 1 real px/tick, independently per axis (a Chebyshev-
        // style step-toward, NOT a normalized diagonal), at the same 20Hz
        // reference tick rate MoveObjectStepLine itself runs at (matching
        // AdvancePatrolStep's own convention) -- a direct, non-
        // approximated transcription via the same 64px/cell grid
        // conversion used throughout this file.
        constexpr float kFollowerHomingSpeed = 20.0f / 64.0f;
    }

    void GEInteractionSystem::Update(float dt, GEWorldRuntime& worldRuntime,
                                      float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                                      GESound& sound, bool blupiCrouching, bool blupiBallooned,
                                      int blupiFacingDX, int blupiFacingDZ, bool blupiInvincible,
                                      bool blupiCanGrantShield, bool blupiCanGrantPower,
                                      bool blupiCanGrantCloud, bool blupiCanGrantHide,
                                      bool blupiFirePressed, bool blupiCanFire, bool blupiCloudActive,
                                      bool blupiCanGrantInvert, bool blupiActionPressedEdge,
                                      bool blupiCanPushCrate)
    {
        events_.clear();
        crateBeingPushedThisFrame_ = false;
        ridingLift_ = false;
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        const Worlds::World& world = worldRuntime.GetWorld();

        // Real Voyage (plan.md `158`) -- ticks any in-flight "fly to HUD
        // icon" animation forward, applying its real reward (see
        // ApplyVoyageReward()) the instant it completes. Runs before the
        // per-object pickup loop below so a treasure voyage completing
        // THIS frame is scanned for newly-satisfied doors THIS frame too
        // (ApplyVoyageReward() calls ScanAndOpenTreasureDoors() directly).
        TickVoyage(dt, worldRuntime, sound);

        // Platform lift riding (plan.md E3D-MIG-152) -- detect which lift
        // (if any) Blupi is standing on BEFORE it takes this frame's patrol
        // step below, using its position as of the end of last frame.
        // Remembered by pointer (this loop never resizes `objects`, only
        // mutates elements in place, so the pointer stays valid for the
        // rest of this call) so the platform-lift branch below can compute
        // the exact same lift's own displacement once it moves.
        MobileObjSpec* riddenLift = nullptr;
        for (auto& candidate : objects)
        {
            if (!candidate.active || !IsPlatformLift(candidate.type))
            {
                continue;
            }
            const float liftStandY = candidate.currentY + 2.0f;
            if (std::fabs(blupiX - candidate.currentX) < 0.5f && std::fabs(blupiZ - candidate.currentZ) < 0.5f &&
                std::fabs(blupiY - liftStandY) < 0.2f)
            {
                riddenLift = &candidate;
                break;
            }
        }
        const float riddenLiftOldX = riddenLift ? riddenLift->currentX : 0.0f;
        const float riddenLiftOldZ = riddenLift ? riddenLift->currentZ : 0.0f;

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

        // Blitz-emitter zap sound (plan.md SOUND-079/VISUAL-024, ch69) --
        // see blitzEmitterPresent_'s own comment for why a one-time lazy
        // world scan is behaviorally equivalent to the real per-tile check.
        if (blitzEmitterPresent_ < 0)
        {
            blitzEmitterPresent_ = 0;
            const auto axis = world.blocksPerAxis();
            for (std::uint16_t gx = 0; gx < axis && blitzEmitterPresent_ == 0; ++gx)
            {
                for (std::uint16_t gz = 0; gz < axis && blitzEmitterPresent_ == 0; ++gz)
                {
                    for (std::uint16_t gy = 0; gy + 1 < axis; ++gy)
                    {
                        if (world.getBlock(gx, gy, gz).type() == GalaxyEggbert::BlockTypes::Blitz &&
                            world.getBlock(gx, static_cast<std::uint16_t>(gy + 1), gz).type() ==
                                GalaxyEggbert::BlockTypes::BlitzEmitter)
                        {
                            blitzEmitterPresent_ = 1;
                            break;
                        }
                    }
                }
            }
        }
        if (blitzEmitterPresent_ == 1)
        {
            // Real fixed tick set within the 100-tick cycle (Decor.cpp:624),
            // reproduced directly via this engine's own 1:1-tick
            // `GetAnimPhase()`.
            const int animTick = worldRuntime.GetAnimPhase() % 100;
            if (animTick == 0 || animTick == 7 || animTick == 18 || animTick == 25 || animTick == 33 ||
                animTick == 44)
            {
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel69);
            }
        }

        bool touchingExitThisFrame = false;
        // Fired projectiles (ObjectType23) spawned by blupih/blupit this
        // frame -- collected here rather than appended to `objects`
        // directly, since this loop holds references into that same
        // vector and a mid-loop push_back could reallocate and invalidate
        // them. Flushed into `objects` (reusing an inactive slot first,
        // matching the real fixed-pool MoveObjectFree()'s slot-reuse
        // semantics rather than growing unboundedly) after the loop below.
        std::vector<MobileObjSpec> pendingSpawns;

        for (auto& obj : objects)
        {
            if (!obj.active)
            {
                continue;
            }

            // Real `BlupiElectro` aura (plan.md `068`) -- checked before
            // every other per-object branch so a Type4 enemy (also in
            // IsGenericHazard's own list) is destroyed by the aura rather
            // than also killing Blupi via hazard contact the same frame.
            if (blupiCloudActive &&
                (obj.type == ObjectType::ObjectType4 || obj.type == ObjectType::ObjectType32 ||
                 obj.type == ObjectType::ObjectType33))
            {
                const float adx = obj.currentX - blupiX;
                const float ady = obj.currentY - blupiY;
                const float adz = obj.currentZ - blupiZ;
                if (adx * adx + ady * ady + adz * adz < kCloudAuraRadius * kCloudAuraRadius)
                {
                    obj.active = false;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel59);
                    continue;
                }
            }

            // Perso-decoy/lethal-decor trap (found 2026-07-16, real Decor.cpp:7957-7975 +
            // `Decor::MovePersoDetect()` ~9835-9865) -- resolves the "what does placing a Perso
            // decoy actually DO gameplay-wise" mystery HUD-017's own writeup left open: small
            // enemies (4/32/33) that patrol into contact with ANY real 200-203 object -- the
            // placed Perso decoy (200) itself, OR one of the 201-203 lethal-looking decorations
            // (`170`) -- mutually destroy each other. Real effects: an ObjectType8 explosion +
            // channel 10 + SmallShake at the ENEMY's position, then an ObjectType37 dissolve
            // effect at a small real offset from it; the detected 200-203 object is also deleted.
            // Checked before the Cloud aura above in real source's own per-object loop (this
            // engine's own ordering here is a natural adaptation, not a faithfulness question --
            // the two can't both apply to the same enemy the same frame regardless of order).
            if (obj.type == ObjectType::ObjectType4 || obj.type == ObjectType::ObjectType32 ||
                obj.type == ObjectType::ObjectType33)
            {
                for (auto& other : objects)
                {
                    if (!other.active)
                    {
                        continue;
                    }
                    if (other.type != ObjectType::ObjectType200 && other.type != ObjectType::ObjectType201 &&
                        other.type != ObjectType::ObjectType202 && other.type != ObjectType::ObjectType203)
                    {
                        continue;
                    }
                    const float pdx = other.currentX - obj.currentX;
                    const float pdy = other.currentY - obj.currentY;
                    const float pdz = other.currentZ - obj.currentZ;
                    if (pdx * pdx + pdy * pdy + pdz * pdz < kHazardContactRadius * kHazardContactRadius)
                    {
                        AppendExplosionFlash(ObjectType::ObjectType8, obj.currentX, obj.currentY, obj.currentZ,
                                              pendingSpawns);
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
                        events_.push_back(Event{EventKind::SmallShakeTriggered});
                        AppendExplosionFlash(ObjectType::ObjectType37, obj.currentX, obj.currentY, obj.currentZ,
                                              pendingSpawns);
                        other.active = false;
                        obj.active = false;
                        break;
                    }
                }
                if (!obj.active)
                {
                    continue;
                }
            }

            // INFRA-006 2nd family (plan.md §7): the 17 purely-cosmetic
            // particle/effect types (VISUAL-008/009/010/012/013/014/015,
            // CAM-009-adjacent, PICKUP-078/080, Clear3Ascend/"158") that just
            // self-expire after a fixed `phase` lifetime -- see
            // kExpiringParticleHandlers/TryTickExpiringParticle() above for
            // the full per-type table and the two real shapes it preserves.
            if (TryTickExpiringParticle(obj))
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

                // Platform lift riding (plan.md E3D-MIG-152): this is the
                // exact lift Blupi was standing on before the patrol step
                // just above -- report its own displacement this tick (a
                // delta, not an absolute position) plus the real constant
                // conveyor nudge for types 47/48 (kConveyorNudgeSpeed is an
                // approximation, plan.md E3D-MIG-154: the real 2px/tick has
                // no exact unit-conversion established for this engine's
                // grid scale), and the new absolute stand height (a snap,
                // matching the real source's own "correct Y drift every
                // frame" approach). The caller applies these via
                // GEBlupiController::RideLift().
                if (&obj == riddenLift)
                {
                    constexpr float kConveyorNudgeSpeed = 0.3f;
                    float nudgeX = 0.0f;
                    if (obj.type == ObjectType::ObjectType47) nudgeX = kConveyorNudgeSpeed * dt;
                    else if (obj.type == ObjectType::ObjectType48) nudgeX = -kConveyorNudgeSpeed * dt;

                    ridingLift_ = true;
                    rideDeltaX_ = (obj.currentX - riddenLiftOldX) + nudgeX;
                    rideDeltaZ_ = obj.currentZ - riddenLiftOldZ;
                    rideStandY_ = obj.currentY + 2.0f;
                }
                continue;
            }

            // Crate push (ObjectType12) -- faithful to mobile-eggbert
            // TestPushCaisse (see GalaxyEggbertSimple3D's GEDecorSystem.cpp,
            // whose own comment cites the real function name). X-axis only,
            // matching that reference exactly (mobile-eggbert is a 2D side-
            // scroller; X is its only horizontal movement axis, so a
            // Z-axis push was never a real mechanic to begin with). Real
            // gate also excludes every vehicle mode + Balloon/Ecrase
            // (Decor.cpp:6130-6132, found 2026-07-16) -- blupiCanPushCrate
            // (vehicle+Ecrase) and blupiBallooned (already an existing
            // parameter) together cover it.
            if (IsCrate(obj.type) && blupiCanPushCrate && !blupiBallooned)
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
                        // Linked crates (plan.md E3D-MIG-150, real
                        // Decor::SearchLinkCaisse ~9397): flood-fill outward
                        // from the seed (pushed) crate, adding any other
                        // crate whose box touches an already-linked one (1
                        // grid unit in X or Y, same Z column -- this
                        // engine's crates only ever occupy a single Z per
                        // real mobile-eggbert's own X/Y-only geometry),
                        // restricted to crates AT OR ABOVE the seed's own
                        // row (Y) -- "you push the stack, not the floor it
                        // rests on", matching the real restriction exactly.
                        std::vector<MobileObjSpec*> linked;
                        linked.push_back(&obj);
                        bool addedAny = true;
                        while (addedAny)
                        {
                            addedAny = false;
                            for (auto& other : objects)
                            {
                                if (!other.active || !IsCrate(other.type)) continue;
                                if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                if (other.currentY < obj.currentY - 0.5f) continue;
                                if (std::fabs(other.currentZ - obj.currentZ) > 0.5f) continue;
                                bool touches = false;
                                for (auto* member : linked)
                                {
                                    if (std::fabs(other.currentX - member->currentX) < 1.5f &&
                                        std::fabs(other.currentY - member->currentY) < 1.5f)
                                    {
                                        touches = true;
                                        break;
                                    }
                                }
                                if (touches)
                                {
                                    linked.push_back(&other);
                                    addedAny = true;
                                }
                            }
                        }

                        // TestPushCaisse (~9321): every linked member must
                        // clear at its own proposed destination -- floor
                        // support (real: two lower-corner edge strips) is
                        // only re-checked for members at the seed's own row
                        // (the row being actively pushed); members stacked
                        // ABOVE that row skip the floor test entirely and
                        // only need the general non-collision check, same
                        // as the real source. A stack moves atomically:
                        // any one member blocked cancels the whole push.
                        bool allClear = true;
                        for (auto* member : linked)
                        {
                            const float destX = member->currentX + pushDir;
                            const int wx = static_cast<int>(std::round(destX)) + GEWorldRuntime::kWorldCenterX;
                            const int wz = static_cast<int>(std::round(member->currentZ)) + GEWorldRuntime::kWorldCenterZ;
                            const int axis = static_cast<int>(world.blocksPerAxis());
                            if (wx < 0 || wx >= axis || wz < 0 || wz >= axis)
                            {
                                allClear = false;
                                break;
                            }

                            const bool isSeedRow = std::fabs(member->currentY - obj.currentY) < 0.5f;
                            if (isSeedRow)
                            {
                                const bool hasFloor = !world.getBlock(static_cast<std::uint16_t>(wx), 0,
                                                                       static_cast<std::uint16_t>(wz))
                                                            .isAir();
                                if (!hasFloor)
                                {
                                    allClear = false;
                                    break;
                                }
                            }

                            bool occupied = false;
                            for (const auto& other : objects)
                            {
                                if (!other.active || !IsCrate(other.type)) continue;
                                if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                if (std::fabs(other.currentX - destX) < 0.5f &&
                                    std::fabs(other.currentY - member->currentY) < 0.5f &&
                                    std::fabs(other.currentZ - member->currentZ) < 0.5f)
                                {
                                    occupied = true;
                                    break;
                                }
                            }
                            if (occupied)
                            {
                                allClear = false;
                                break;
                            }
                        }

                        if (allClear)
                        {
                            for (auto* member : linked)
                            {
                                member->currentX += pushDir;
                            }
                            // Real crate-push loop sound (found 2026-07-16, Decor.cpp:6138/6147
                            // start, `:3637` stop on leaving `BlupiAction::Push`) -- ch38, not
                            // "electric arc (long)" as plan.md's own SOUND-048 entry claimed.
                            // This class has no direct sound-instance-lifetime access (its own
                            // `sound` parameter is fire-and-forget `Play()` only), so the actual
                            // start/stop loop control lives in the caller
                            // (`GalaxyEggbertCnaGame::wasPushingCrate_`) -- this flag is the
                            // signal it acts on. Deliberately still a plain bool, NOT part of
                            // `events_` (INFRA-007, plan.md §7) -- this is continuous per-frame
                            // state the caller edge-detects itself, not a one-shot event.
                            crateBeingPushedThisFrame_ = true;
                        }
                    }
                }
                continue;
            }

            // Door-opening slide effect (ObjectType22, plan.md E3D-MIG-160,
            // real `Decor::OpenDoor` ~11667): slides up by exactly one grid
            // unit over the real `Config::ScaleTime(50)` = 50 ticks (2.5s
            // at the 20Hz reference rate obj.phase already advances at),
            // then self-destructs -- a one-shot animation, not the generic
            // dwell/advance/dwell/recede patrol (which loops and doesn't
            // fit a "play once and vanish" effect), so it's handled
            // directly here instead.
            if (obj.type == ObjectType::ObjectType22)
            {
                constexpr float kSlideTicks = 50.0f;
                const float t = std::min(obj.phase / kSlideTicks, 1.0f);
                obj.currentY = obj.posStartY + t;
                if (obj.phase >= kSlideTicks)
                {
                    obj.active = false;
                }
                continue;
            }

            // Dynamite fuse (ObjectType56, plan.md E3D-MIG-155) -- real
            // 9-blast sequence at fixed ticks (Decor.cpp ~8252-8296),
            // verified directly against the source, not just the reference
            // doc's rounded "~200px/~100px" summary: tick 50 is the center
            // blast (dx,dy)=(0,0); 53=(-100,8); 55=(80,10); 56=(-15,-100);
            // 59=(20,70); 62=(30,-50); 64=(-40,30); 67=(-180,10); 69=
            // (200,-10) -- real pixel offsets, /64 to this engine's grid
            // units (real dx -> this engine's X, real dy -> this engine's Y,
            // same X/Y-only 2D-source convention already used for blupih/
            // blupit/follower; Z is always 0, no real source axis maps to
            // it). obj.phase already advances at the real 20Hz reference
            // rate (GEWorldRuntime::Update()); reconstructing the phase as
            // of the START of this frame (`prevPhase`) lets each blast fire
            // on the exact frame its tick is crossed, not every frame after.
            if (obj.type == ObjectType::ObjectType56)
            {
                struct Blast { float tick, dx, dy; };
                static constexpr Blast kBlasts[] = {
                    {50.0f, 0.0f / 64.0f, 0.0f / 64.0f},
                    {53.0f, -100.0f / 64.0f, 8.0f / 64.0f},
                    {55.0f, 80.0f / 64.0f, 10.0f / 64.0f},
                    {56.0f, -15.0f / 64.0f, -100.0f / 64.0f},
                    {59.0f, 20.0f / 64.0f, 70.0f / 64.0f},
                    {62.0f, 30.0f / 64.0f, -50.0f / 64.0f},
                    {64.0f, -40.0f / 64.0f, 30.0f / 64.0f},
                    {67.0f, -180.0f / 64.0f, 10.0f / 64.0f},
                    {69.0f, 200.0f / 64.0f, -10.0f / 64.0f},
                };
                const float prevPhase = obj.phase - dt * 20.0f;
                for (const auto& blast : kBlasts)
                {
                    if (prevPhase < blast.tick && obj.phase >= blast.tick)
                    {
                        const float centerX = obj.currentX + blast.dx;
                        const float centerY = obj.currentY + blast.dy;
                        const float centerZ = obj.currentZ;

                        // Real dynamite-blast explosion flash (plan.md
                        // VISUAL-008, ObjectType8, Decor.cpp:9065) -- one
                        // instance per blast, at its own center, not just
                        // the central (dx=0,dy=0) one.
                        AppendExplosionFlash(ObjectType::ObjectType8, centerX, centerY, centerZ, pendingSpawns);

                        if (blast.dx == 0.0f && blast.dy == 0.0f)
                        {
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
                            // Real camera shake (plan.md CAM-008,
                            // Decor::DynamiteStart(), Decor.cpp:9068-9070,
                            // confirmed via direct source read): SmallShake
                            // fires ONLY for the center blast (dx=0,dy=0),
                            // not for every peripheral tick in the 9-blast
                            // sequence.
                            events_.push_back(Event{EventKind::SmallShakeTriggered});
                        }

                        // Real 128x128px (2x2 tile) area -- +-1 grid unit
                        // in X/Y around the blast center, same Z.
                        constexpr float kBlastHalfExtent = 1.0f;

                        // Clear destructible hazard tiles by icon (real:
                        // saws 378/379, drip hazards 404/410 -- the latter
                        // has no placeable BlockTypes constant in this
                        // engine yet, so only Saw/SawStopped are handled).
                        const int wx0 = static_cast<int>(std::round(centerX - kBlastHalfExtent)) +
                                        GEWorldRuntime::kWorldCenterX;
                        const int wx1 = static_cast<int>(std::round(centerX + kBlastHalfExtent)) +
                                        GEWorldRuntime::kWorldCenterX;
                        const int wz = static_cast<int>(std::round(centerZ)) + GEWorldRuntime::kWorldCenterZ;
                        const int wy = static_cast<int>(std::round(centerY)) < 0
                                           ? 0
                                           : static_cast<int>(std::round(centerY));
                        const int axis = static_cast<int>(world.blocksPerAxis());
                        if (wy >= 0 && wy < axis && wz >= 0 && wz < axis)
                        {
                            for (int wx = std::max(0, wx0); wx <= std::min(axis - 1, wx1); ++wx)
                            {
                                const auto blockType = world.getBlock(static_cast<std::uint16_t>(wx),
                                                                       static_cast<std::uint16_t>(wy),
                                                                       static_cast<std::uint16_t>(wz))
                                                            .type();
                                if (blockType == GalaxyEggbert::BlockTypes::Saw ||
                                    blockType == GalaxyEggbert::BlockTypes::SawStopped)
                                {
                                    worldRuntime.GetWorldMutable().setBlock(
                                        static_cast<std::uint16_t>(wx), static_cast<std::uint16_t>(wy),
                                        static_cast<std::uint16_t>(wz),
                                        Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));
                                }
                            }
                        }

                        // Destroy every enemy/crate/object overlapping the
                        // blast rect, from the real explicit type list
                        // (Decor.cpp ~9102-9132, ported exactly, not
                        // approximated) -- crates are destroyed as a full
                        // linked group (real SearchLinkCaisse), everything
                        // else is a plain deactivate. The blast flash itself
                        // is spawned above; no further per-victim debris
                        // visual is modeled (real source has none either --
                        // ObjectType8 above is the only cosmetic effect at
                        // this site).
                        for (auto& victim : objects)
                        {
                            if (!victim.active || &victim == &obj)
                            {
                                continue;
                            }
                            const bool isDestructibleType =
                                victim.type == ObjectType::ObjectType2 || victim.type == ObjectType::ObjectType3 ||
                                victim.type == ObjectType::ObjectType4 || victim.type == ObjectType::ObjectType6 ||
                                victim.type == ObjectType::ObjectType12 || victim.type == ObjectType::ObjectType13 ||
                                victim.type == ObjectType::ObjectType16 || victim.type == ObjectType::ObjectType17 ||
                                victim.type == ObjectType::ObjectType18 || victim.type == ObjectType::ObjectType19 ||
                                victim.type == ObjectType::ObjectType20 || victim.type == ObjectType::ObjectType24 ||
                                victim.type == ObjectType::ObjectType25 || victim.type == ObjectType::ObjectType26 ||
                                victim.type == ObjectType::ObjectType28 || victim.type == ObjectType::ObjectType30 ||
                                victim.type == ObjectType::ObjectType32 || victim.type == ObjectType::ObjectType33 ||
                                victim.type == ObjectType::ObjectType34 || victim.type == ObjectType::ObjectType40 ||
                                victim.type == ObjectType::ObjectType44 || victim.type == ObjectType::ObjectType46 ||
                                victim.type == ObjectType::ObjectType52 || victim.type == ObjectType::ObjectType54 ||
                                victim.type == ObjectType::ObjectType96 || victim.type == ObjectType::ObjectType97 ||
                                victim.type == ObjectType::ObjectType200 || victim.type == ObjectType::ObjectType201 ||
                                victim.type == ObjectType::ObjectType202 || victim.type == ObjectType::ObjectType203;
                            if (!isDestructibleType)
                            {
                                continue;
                            }
                            if (std::fabs(victim.currentX - centerX) > kBlastHalfExtent + 0.5f ||
                                std::fabs(victim.currentY - centerY) > kBlastHalfExtent + 0.5f ||
                                std::fabs(victim.currentZ - centerZ) > 0.5f)
                            {
                                continue;
                            }
                            if (IsCrate(victim.type))
                            {
                                std::vector<MobileObjSpec*> linked;
                                linked.push_back(&victim);
                                bool addedAny = true;
                                while (addedAny)
                                {
                                    addedAny = false;
                                    for (auto& other : objects)
                                    {
                                        if (!other.active || !IsCrate(other.type)) continue;
                                        if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                        bool touches = false;
                                        for (auto* member : linked)
                                        {
                                            if (std::fabs(other.currentX - member->currentX) < 1.5f &&
                                                std::fabs(other.currentY - member->currentY) < 1.5f &&
                                                std::fabs(other.currentZ - member->currentZ) < 0.5f)
                                            {
                                                touches = true;
                                                break;
                                            }
                                        }
                                        if (touches)
                                        {
                                            linked.push_back(&other);
                                            addedAny = true;
                                        }
                                    }
                                }
                                for (auto* member : linked)
                                {
                                    member->active = false;
                                }
                            }
                            else
                            {
                                victim.active = false;
                            }
                        }

                        // Blupi death check (real: `m_blupiFocus && !shield
                        // && !hide && !superblupi`) -- Shield/Hide (plan.md
                        // E3D-MIG-170, blupiInvincible) modeled now; focus/
                        // superBlupi don't exist in this engine yet.
                        if (!blupiInvincible && std::fabs(blupiX - centerX) <= kBlastHalfExtent + 0.5f &&
                            std::fabs(blupiY - centerY) <= kBlastHalfExtent + 0.5f &&
                            std::fabs(blupiZ - centerZ) <= 0.5f)
                        {
                            events_.push_back(Event{EventKind::Died});
                            // Real LoseLife()/respawn deferred to the death-lock resolution point
                            // (death-VFX follow-up) -- deterministic Clear1 (no VFX, matches
                            // `159`), shouldRespawn=false, confirmed no `m_blupiRestart=true` near
                            // this real site (Decor.cpp:9168-9173).
                            Event deathLockEvent{EventKind::DeathLockRequested};
                            deathLockEvent.deathLockKind = PendingDeathKind::Clear1;
                            deathLockEvent.deathLockShouldRespawn = false;
                            ReplaceEvent(EventKind::DeathLockRequested, deathLockEvent);
                        }
                    }
                }
                if (obj.phase >= 70.0f)
                {
                    obj.active = false;
                }
                continue;
            }

            // Bridge construction (ObjectType52, plan.md PICKUP-064, real
            // Decor.cpp ~8540-8562): each tick writes the real
            // `table_bridge` icon at that phase BOTH to this object's own
            // sprite AND directly into the live terrain grid cell at its
            // spawn position -- ground collision genuinely toggles across
            // the sequence (icons 367-372 and the -1 "no tile" sentinel are
            // non-solid per real `table_decor_quart`/this engine's own
            // `BlockTypes::isMobileTransparent()`, already confirmed to
            // agree). Real per-mobile-eggbert's exact frame-by-frame
            // `table_bridge` array is not transcribed here (no pre-approved
            // in-repo source for it, unlike blupi's own tables) -- this
            // instead reproduces the DOCUMENTED aggregate shape from
            // mobile-eggbert-reference/14-crates-lifts-bridges-effects.md:
            // 28 ticks ascending through icons 365-372, 112 ticks holding
            // the -1 sentinel, 17 ticks descending back through 372..365
            // ending at the original 364 -- with this engine's own
            // reasonable uniform pacing within each of those 3 documented
            // windows, not a copied per-tick table.
            if (obj.type == ObjectType::ObjectType52)
            {
                constexpr float kAscendTicks = 28.0f;
                constexpr float kHoldTicks = 112.0f;
                constexpr float kDescendTicks = 17.0f;
                constexpr float kTotalTicks = kAscendTicks + kHoldTicks + kDescendTicks; // 157
                constexpr float kProgressSoundTick = 137.0f;

                const float prevPhase = obj.phase - dt * 20.0f;
                if (prevPhase < kProgressSoundTick && obj.phase >= kProgressSoundTick)
                {
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel73);
                }

                int icon;
                if (obj.phase < kAscendTicks)
                {
                    const int step = static_cast<int>(obj.phase * 8.0f / kAscendTicks);
                    icon = 365 + std::clamp(step, 0, 7);
                }
                else if (obj.phase < kAscendTicks + kHoldTicks)
                {
                    icon = -1; // real "no tile" sentinel -- fromMobileIconId() maps this to Air
                }
                else
                {
                    const float descendPhase = obj.phase - (kAscendTicks + kHoldTicks);
                    const int step = static_cast<int>(descendPhase * 9.0f / kDescendTicks);
                    icon = 372 - std::clamp(step, 0, 8); // 372 down to 364
                }

                const int gx = static_cast<int>(std::lround(obj.currentX)) + GEWorldRuntime::kWorldCenterX;
                const int gz = static_cast<int>(std::lround(obj.currentZ)) + GEWorldRuntime::kWorldCenterZ;
                const int gy = static_cast<int>(std::lround(obj.currentY)) - 1;
                const int axis = static_cast<int>(world.blocksPerAxis());
                if (gx >= 0 && gx < axis && gy >= 0 && gy < axis && gz >= 0 && gz < axis)
                {
                    worldRuntime.GetWorldMutable().setBlock(
                        static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                        static_cast<std::uint16_t>(gz),
                        Worlds::Block::make(GalaxyEggbert::BlockTypes::fromMobileIconId(icon)));
                }

                if (obj.phase >= kTotalTicks)
                {
                    obj.active = false;
                }
                continue;
            }

            // Follower wake-up (ObjectType96 -> 97, plan.md E3D-MIG-137,
            // real Decor.cpp:9646-9678 `MoveObjectFollow`) -- a dormant
            // follower promotes to the homing type the instant Blupi comes
            // within its padded detection box, playing its wake sound
            // (real channel 92) exactly once at the transition. Does NOT
            // `continue` -- a follower that wakes this exact frame also
            // takes its first homing step this same frame, below.
            if (obj.type == ObjectType::ObjectType96)
            {
                const float fwdx = obj.currentX - blupiX;
                const float fwdy = obj.currentY - blupiY;
                const float fwdz = obj.currentZ - blupiZ;
                if (fwdx * fwdx + fwdy * fwdy + fwdz * fwdz < kFollowerWakeRadius * kFollowerWakeRadius)
                {
                    obj.type = ObjectType::ObjectType97;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel92);
                }
            }

            // Follower homing (ObjectType97, plan.md E3D-MIG-137, real
            // Decor.cpp:8025-8064) -- once awake, steps X and Y
            // independently (Chebyshev-style, NOT a normalized diagonal)
            // toward Blupi's live position every frame at
            // kFollowerHomingSpeed. currentZ is deliberately left
            // untouched -- real mobile-eggbert has no Z axis at all, only
            // X (horizontal) and Y (vertical) exist in the source this is
            // ported from, matching blupih/blupit's own shots, which never
            // touch Z either. Collapses posStart/posEnd to the new
            // position every successful step (matching the real source's
            // own `posStart = posEnd = end`), which naturally makes the
            // generic patrol-turn block below a no-op for this object (its
            // own `posStart != posEnd` guard), exactly like the real
            // source's generic dwell/advance/dwell/recede block becomes a
            // no-op immediately after this same real function's homing
            // branch runs -- no separate type exclusion needed.
            //
            // Blocked-path self-destruct (real: `TestPath` fails ->
            // `ObjectDelete` + a real `ObjectType9` explosion + channel 10,
            // `Decor.cpp:8025-8064`) is modeled as a single-point solid
            // check at the destination cell via `GEBlupiController::
            // IsSolidAt` (this file's own existing tile-grid convention,
            // e.g. `SearchAirDistance`) -- real `TestPath` sweeps a
            // rectangle, not a point, same simplification as every other
            // collision check in this file. The cosmetic `ObjectType9`
            // debris flash (plan.md VISUAL-008) and real `SmallShake`
            // camera shake (`m_decorAction = DecorAction::SmallShake`,
            // `Decor.cpp:8062-8063`, plan.md CAM-008 -- this exact site was
            // missed by that earlier audit) are both now modeled too,
            // found and fixed 2026-07-14 (this class's own real `-1`
            // blank-frame renderer support, added for the bullet-splat
            // effect, already covers `table_explo2`'s own sentinels). Real
            // `end.X/Y -= 34` pre-offset before the spawn is the same 2D
            // sprite-corner-anchoring artifact already dismissed for every
            // other real `ObjectStart(..., 0)` site this session -- spawns
            // at the follower's own position instead. `continue`s on self-destruct so an already-destroyed
            // follower can't also register a contact-kill against Blupi
            // this same frame via the generic hazard check below; falls
            // through (no `continue`) on a successful step, since that
            // check is exactly how a follower that catches up to Blupi is
            // supposed to kill him.
            if (obj.type == ObjectType::ObjectType97)
            {
                float endX = obj.currentX;
                float endY = obj.currentY;
                const float stepAmount = kFollowerHomingSpeed * dt;
                if (endX < blupiX) endX = std::min(endX + stepAmount, blupiX);
                else if (endX > blupiX) endX = std::max(endX - stepAmount, blupiX);
                if (endY < blupiY) endY = std::min(endY + stepAmount, blupiY);
                else if (endY > blupiY) endY = std::max(endY - stepAmount, blupiY);

                int fgx, fgy, fgz;
                ToGridCell(world, endX, endY, obj.currentZ, fgx, fgy, fgz);
                if (IsSolidAt(world, fgx, fgy, fgz))
                {
                    obj.active = false;
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
                    AppendExplosionFlash(ObjectType::ObjectType9, obj.currentX, obj.currentY, obj.currentZ,
                                          pendingSpawns);
                    events_.push_back(Event{EventKind::SmallShakeTriggered});
                    continue;
                }
                obj.currentX = obj.posStartX = obj.posEndX = endX;
                obj.currentY = obj.posStartY = obj.posEndY = endY;
                obj.posStartZ = obj.posEndZ = obj.currentZ;
            }

            // Real shared patrol-turn mechanic (plan.md E3D-MIG-131) --
            // every remaining MoveObject type (enemies, pickups, etc., NOT
            // lifts/crates handled above). Does NOT `continue`: the wasp/
            // hazard/pickup contact checks below must see this frame's
            // freshly-updated currentX/Y/Z, not last frame's position. A
            // no-op if posStart==posEnd, matching the real guard exactly
            // (stationary objects never advance patrolStep/patrolTime).
            if (obj.posStartX != obj.posEndX || obj.posStartY != obj.posEndY || obj.posStartZ != obj.posEndZ)
            {
                const int prevPatrolStep = obj.patrolStep;
                const float prevPatrolTime = obj.patrolTime;
                AdvancePatrolStep(obj, dt);

                // Stationary shooters (ObjectType32 blupih / 33 blupit,
                // plan.md E3D-MIG-134) -- attack timing is gated on the
                // dwell step (1 or 3) and exact dwell-frame BEFORE this
                // frame's AdvancePatrolStep() call, using CrossedTick() on
                // the pre-call patrolTime -- not the post-call value, which
                // may already belong to a different patrolStep if the
                // dwell happened to end this same frame.
                if (prevPatrolStep == 1 || prevPatrolStep == 3)
                {
                    const float dtTicks = dt * kTicksPerSecond;
                    if (obj.type == ObjectType::ObjectType32 && CrossedTick(prevPatrolTime, dtTicks, 21.0f))
                    {
                        FireBlupihShot(obj, world, sound, pendingSpawns);
                    }
                    else if (obj.type == ObjectType::ObjectType33)
                    {
                        const bool aboutToWalkRight =
                            (obj.posStartX < obj.posEndX && prevPatrolStep == 1) ||
                            (obj.posStartX > obj.posEndX && prevPatrolStep == 3);
                        if (CrossedTick(prevPatrolTime, dtTicks, 3.0f))
                        {
                            FireBlupitShot(obj, aboutToWalkRight ? -1.0f : 1.0f, world, sound, pendingSpawns);
                        }
                        if (CrossedTick(prevPatrolTime, dtTicks, 21.0f))
                        {
                            FireBlupitShot(obj, aboutToWalkRight ? 1.0f : -1.0f, world, sound, pendingSpawns);
                        }
                    }
                }
            }

            // Blupih/blupit's own body (ObjectType32/33) is deliberately
            // NOT in IsGenericHazard() -- real mobile-eggbert never treats
            // walking into their body as a damage path, only their fired
            // projectile is harmful (see FireBlupihShot/FireBlupitShot
            // above). Falls through to the final pickup-type filter below,
            // which already excludes them (no separate `continue` needed).

            // Fired projectile contact (ObjectType23, plan.md E3D-MIG-134):
            // verified directly against Decor.cpp:5914-5947 -- always fatal
            // (real Glu/glue-style death) and destroys the bullet itself.
            // Real Shield/Hide immunity IS modeled now (plan.md E3D-MIG-170,
            // blupiInvincible); win-and-death-action/superBlupi gates are
            // NOT modeled -- neither concept exists in this engine yet.
            // Real death has no distinct sound call of
            // its own (StartSploutchGlu only spawns silent splash-effect
            // debris, now modeled -- plan.md VISUAL-009, see
            // AppendSplatEffect()'s own comment) -- channel 74 reused here
            // for consistency with this class's existing hazard-death
            // sound approximation.
            if (obj.type == ObjectType::ObjectType23)
            {
                const float bdx = obj.currentX - blupiX;
                const float bdy = obj.currentY - blupiY;
                const float bdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && bdx * bdx + bdy * bdy + bdz * bdz < kHazardContactRadius * kHazardContactRadius)
                {
                    AppendSplatEffect(obj.currentX, obj.currentY, obj.currentZ, pendingSpawns);
                    obj.active = false;
                    events_.push_back(Event{EventKind::Died});
                    // Real LoseLife()/respawn deferred to the death-lock resolution point
                    // (death-VFX follow-up) -- this is one of the real Glu trigger sites (direct
                    // `m_blupiAction=Glu` assignment, Decor.cpp:5914-5946, NOT via BlupiDead),
                    // shouldRespawn=true (confirmed `m_blupiRestart=true` at Decor.cpp:5927).
                    {
                        Event deathLockEvent{EventKind::DeathLockRequested};
                        deathLockEvent.deathLockKind = PendingDeathKind::Glu;
                        deathLockEvent.deathLockShouldRespawn = true;
                        ReplaceEvent(EventKind::DeathLockRequested, deathLockEvent);
                    }
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                }
                continue;
            }

            // Wasp (ObjectType44, plan.md E3D-MIG-135) -- does NOT kill or
            // destroy itself; contact signals a BalloonTouched event so
            // the caller can attempt GEBlupiController::TriggerBalloon()
            // (idempotent there, not here -- see the class comment). Real
            // gate also includes `!m_blupiShield && !m_blupiHide`
            // (Decor.cpp:5826, plan.md E3D-MIG-170) -- a shielded/hidden
            // Blupi doesn't get ballooned at all.
            if (obj.type == ObjectType::ObjectType44)
            {
                const float wdx = obj.currentX - blupiX;
                const float wdy = obj.currentY - blupiY;
                const float wdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && wdx * wdx + wdy * wdy + wdz * wdz < kHazardContactRadius * kHazardContactRadius)
                {
                    events_.push_back(Event{EventKind::BalloonTouched});
                }
                continue;
            }

            // Large creature (ObjectType54, plan.md E3D-MIG-136) -- verified
            // directly against Decor.cpp:5867-5913: contact is lethal ONLY
            // while the creature is paused mid-turn (patrolStep 1 or 3, the
            // real `step != 2 && step != 4` gate) -- unlike every hazard
            // above, walking into it while it is actually mid-walk is
            // completely safe. Real immunity is
            // `!m_blupiBalloon && !m_blupiShield && !m_blupiHide &&
            // !m_bSuperBlupi` (plus a `m_blupiFocus` gate); balloon AND
            // Shield/Hide (plan.md E3D-MIG-170, blupiInvincible) are both
            // modeled now -- only superBlupi/focus don't exist in this
            // engine yet -- unlike the 4 balloon-
            // POPPABLE types, contact while ballooned does nothing at all
            // here (matches the real source: `!m_blupiBalloon` gates the
            // whole branch, there's no separate pop path for type 54). The
            // creature itself is never destroyed by this contact (no
            // `ObjectDelete` in the real branch, unlike the shared kill-list
            // types below) -- it always survives to keep guarding.
            // **Corrected 2026-07-16** (this comment previously mis-stated the
            // real behavior): contact does NOT spare Blupi if he's riding a
            // vehicle -- real source sets `BlupiAction::Glu` unconditionally
            // either way (still a real death via the shared per-action
            // life-loss dispatch, `Decor.cpp:6374-6392`), it just ALSO plays a
            // different sound/shake (channel 10 + SmallShake + a pop effect,
            // vs. plain channel 51) and clears vehicle/Balloon/Ecrase state
            // when contact happens while riding/ballooned/squashed
            // (Decor.cpp:5884-5905) -- mirroring what `BlupiDead()` already
            // does unconditionally for every other real death cause
            // (`GEBlupiController::TriggerDeathLock()` now does this too,
            // fixed alongside this same research). The one remaining gap is
            // purely cosmetic: this engine always plays the plain channel-51
            // sound here regardless of vehicle/Balloon/Ecrase state at
            // contact, since `GEInteractionSystem` has no `GEBlupiController`
            // access to know which applies -- not worth new plumbing for an
            // audio-only distinction (the actual game-state outcome, death +
            // full state clear, is now faithful either way). The real
            // unconditional taunt icon (mockery `83` regardless of
            // facing, Decor.cpp:9575-9578) is also NOT modeled -- no idle-
            // taunt animation system exists in this engine at all yet
            // (cosmetic, same as type2's taunt-suppression above).
            if (obj.type == ObjectType::ObjectType54)
            {
                if ((obj.patrolStep == 1 || obj.patrolStep == 3) && !blupiBallooned && !blupiInvincible)
                {
                    const float gdx = obj.currentX - blupiX;
                    const float gdy = obj.currentY - blupiY;
                    const float gdz = obj.currentZ - blupiZ;
                    if (gdx * gdx + gdy * gdy + gdz * gdz < kHazardContactRadius * kHazardContactRadius)
                    {
                        events_.push_back(Event{EventKind::Died});
                        // Real LoseLife()/respawn deferred to the death-lock resolution point
                        // (death-VFX follow-up) -- this is one of the real Glu trigger sites
                        // (direct `m_blupiAction=Glu` assignment, Decor.cpp:5867-5910, NOT via
                        // BlupiDead), shouldRespawn=true (confirmed `m_blupiRestart=true` at
                        // Decor.cpp:5879).
                        Event deathLockEvent{EventKind::DeathLockRequested};
                        deathLockEvent.deathLockKind = PendingDeathKind::Glu;
                        deathLockEvent.deathLockShouldRespawn = true;
                        ReplaceEvent(EventKind::DeathLockRequested, deathLockEvent);
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel51);
                    }
                }
                continue;
            }

            // Generic hazard contact (ObjectType2/3/4/16/17/20/96/97, plan.md
            // E3D-MIG-132) -- real shared kill list: BlupiDead(Clear1,
            // Clear2) + the hazard itself is destroyed (converted to an
            // explosion). Type3's real duck-immunity (MoveObjectDetect
            // skips it entirely while Blupi's action is Down) is modeled
            // via blupiCrouching; the others have no such immunity. Real
            // death is a 50/50 `BlupiDead(Clear1, Clear2)` coinflip
            // (Decor.cpp:6547-6614, `RollClear2Coinflip()`) -- Clear2 plays
            // channel 74 + the real ascend Voyage (icon 230, plan.md `158`
            // death-VFX follow-up, 2026-07-14), Clear1 plays/spawns
            // nothing further. While ballooned, exactly
            // 4 of these 8 types (3/16/96/97, IsBalloonPoppableHazard())
            // pop the balloon instead of killing (real channel 41 is
            // played by GEBlupiController's own IsBallooned() before/after
            // comparison in the caller, not here -- see PopBalloon()'s
            // comment) -- 2/4/17/20 still kill even while ballooned. Real
            // gate also includes `!m_blupiShield && !m_blupiHide`
            // (Decor.cpp:5784, plan.md E3D-MIG-170) -- modeled via
            // blupiInvincible, skipping the whole contact (no kill, no
            // balloon-pop either).
            if (IsGenericHazard(obj.type))
            {
                if (obj.type == ObjectType::ObjectType3 && blupiCrouching)
                {
                    continue;
                }
                const float hdx = obj.currentX - blupiX;
                const float hdy = obj.currentY - blupiY;
                const float hdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && hdx * hdx + hdy * hdy + hdz * hdz < kHazardContactRadius * kHazardContactRadius)
                {
                    if (blupiBallooned && IsBalloonPoppableHazard(obj.type))
                    {
                        events_.push_back(Event{EventKind::BalloonPopped});
                    }
                    else
                    {
                        obj.active = false;
                        events_.push_back(Event{EventKind::Died});
                        // Real LoseLife()/respawn now deferred to the death-lock/life-loss-Voyage
                        // resolution point (death-VFX follow-up) -- shouldRespawn=false, confirmed
                        // no `m_blupiRestart=true` near this real site (Decor.cpp:5782-5815).
                        const bool isClear2 = RollClear2Coinflip();
                        Event deathLockEvent{EventKind::DeathLockRequested};
                        deathLockEvent.deathLockKind = isClear2 ? PendingDeathKind::Clear2 : PendingDeathKind::Clear1;
                        deathLockEvent.deathLockShouldRespawn = false;
                        ReplaceEvent(EventKind::DeathLockRequested, deathLockEvent);
                        if (isClear2)
                        {
                            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                            RequestClear2Ascend(obj.currentX, obj.currentY, obj.currentZ);
                        }
                        // Real camera shake (plan.md CAM-008/009, Decor.cpp
                        // ~5782-5814, confirmed via direct source read) --
                        // this exact real site plays SmallShake for every
                        // one of these 8 hazard types EXCEPT the fish (17)
                        // and bird (20) variants, which play BigShake
                        // instead (real behavior, not an approximation).
                        // Real explosion flash (plan.md VISUAL-008, see
                        // AppendExplosionFlash()'s own comment) is spawned
                        // at the SAME site, same fish/bird split:
                        // ObjectType10 for fish/bird, ObjectType8 for every
                        // other hazard type.
                        if (obj.type == ObjectType::ObjectType17 || obj.type == ObjectType::ObjectType20)
                        {
                            events_.push_back(Event{EventKind::BigShakeTriggered});
                            AppendExplosionFlash(ObjectType::ObjectType10, obj.currentX, obj.currentY, obj.currentZ,
                                                  pendingSpawns);
                        }
                        else
                        {
                            events_.push_back(Event{EventKind::SmallShakeTriggered});
                            AppendExplosionFlash(ObjectType::ObjectType8, obj.currentX, obj.currentY, obj.currentZ,
                                                  pendingSpawns);
                        }
                    }
                }
                continue;
            }

            // Types 201-203 (plan.md PICKUP-069, found 2026-07-16, real Decor.cpp:6088-6115) --
            // lethal decorative objects sharing ObjectType200's real "MoveObject 200-203" range,
            // but NOT Perso itself (200, the placeable decoy, already implemented separately,
            // handled entirely by TryPerso()/GalaxyEggbertCnaGame.cpp -- not here). Real contact:
            // same Clear1/Clear2 coinflip death as the generic-hazard list above (real
            // BlupiDead(Clear1, Clear2)), Shield/Hide immunity (blupiInvincible), no
            // m_blupiRestart=true anywhere in this real block (shouldRespawn=false, same as the
            // generic-hazard list) -- but ALWAYS channel 10 + SmallShake + an ObjectType10 pop
            // effect (no fish/bird BigShake variant here, unlike the generic-hazard list).
            if (obj.type == ObjectType::ObjectType201 || obj.type == ObjectType::ObjectType202 ||
                obj.type == ObjectType::ObjectType203)
            {
                const float sdx = obj.currentX - blupiX;
                const float sdy = obj.currentY - blupiY;
                const float sdz = obj.currentZ - blupiZ;
                if (!blupiInvincible &&
                    sdx * sdx + sdy * sdy + sdz * sdz < kHazardContactRadius * kHazardContactRadius)
                {
                    obj.active = false;
                    events_.push_back(Event{EventKind::Died});
                    const bool isClear2 = RollClear2Coinflip();
                    Event deathLockEvent{EventKind::DeathLockRequested};
                    deathLockEvent.deathLockKind = isClear2 ? PendingDeathKind::Clear2 : PendingDeathKind::Clear1;
                    deathLockEvent.deathLockShouldRespawn = false;
                    ReplaceEvent(EventKind::DeathLockRequested, deathLockEvent);
                    if (isClear2)
                    {
                        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                        RequestClear2Ascend(obj.currentX, obj.currentY, obj.currentZ);
                    }
                    events_.push_back(Event{EventKind::SmallShakeTriggered});
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel10);
                    AppendExplosionFlash(ObjectType::ObjectType10, obj.currentX, obj.currentY, obj.currentZ,
                                          pendingSpawns);
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
                obj.type != ObjectType::ObjectType7 && obj.type != ObjectType::ObjectType21 &&
                obj.type != ObjectType::ObjectType49 &&
                obj.type != ObjectType::ObjectType50 && obj.type != ObjectType::ObjectType51 &&
                obj.type != ObjectType::ObjectType55 && obj.type != ObjectType::ObjectType25 &&
                obj.type != ObjectType::ObjectType26 && obj.type != ObjectType::ObjectType30 &&
                obj.type != ObjectType::ObjectType31 && obj.type != ObjectType::ObjectType29 &&
                obj.type != ObjectType::ObjectType40)
            {
                continue;
            }

            const float dx = obj.currentX - blupiX;
            const float dy = obj.currentY - blupiY;
            const float dz = obj.currentZ - blupiZ;
            const bool touching = (dx * dx + dy * dy + dz * dz) < kPickupRadius * kPickupRadius;

            // Secret exit (ObjectType21, plan.md PICKUP-009/083, found 2026-07-16) shares the
            // EXACT same real exit-gate logic as the regular exit (ObjectType7) below
            // (Decor.cpp:6158-6184, one `if` covering both types) -- the only real difference is
            // setting `m_bFoundCle=true` on this variant, a "found the secret" flag with no
            // known consumer in this engine's own save/ranking scope (not modeled, matching this
            // engine's deliberately-independent GESaveData format). Previously this engine only
            // recognized ObjectType7, so a secret exit did nothing at all on contact.
            if (obj.type == ObjectType::ObjectType7 || obj.type == ObjectType::ObjectType21)
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
            // contact, matching real source exactly -- but the REWARD
            // (counter increment) is now deferred to voyage completion
            // (plan.md `158`, fixed 2026-07-14; previously applied
            // immediately as a documented simplification). Each case
            // records a this-frame voyage request (`RequestVoyage()`) with
            // the pickup's own world position as the voyage's START and a
            // fixed HUD-space point (matching `GEHud.cpp`'s own real
            // constants exactly) as its END -- the caller
            // (`GalaxyEggbertCnaGame.cpp`) projects the world position and
            // calls `BeginVoyage()` right after this `Update()` call
            // returns.
            switch (obj.type)
            {
                // INFRA-006 3rd family (plan.md §7): Treasure/Key1/2/3/
                // Dynamite share one handler-table-driven dispatch,
                // TryCollectBasicPickup() -- see its own comment and
                // kBasicPickupHandlers above. Real sparkle burst
                // (plan.md VISUAL-012) fires for Treasure AND the 3 key
                // pickups (`Decor.cpp:5956-6006`, confirmed via
                // ObjectType.hpp's own "Key 1/2/3 collectible" doc comments),
                // spawned immediately at touch time, NOT deferred to voyage
                // completion.
                case ObjectType::ObjectType5:  // treasure
                case ObjectType::ObjectType49: // key 1
                case ObjectType::ObjectType50: // key 2
                case ObjectType::ObjectType51: // key 3
                case ObjectType::ObjectType55: // dynamite stick
                    TryCollectBasicPickup(obj, dynamiteCount_ == 0, pendingSpawns);
                    break;
                case ObjectType::ObjectType6: // extra-life egg
                    // Real MAX_EGG_COUNT=10 gate: at the cap, touching an
                    // egg does nothing at all -- not even removed (real
                    // gate is on the WHOLE touch-time block, Decor.cpp:6007,
                    // not just the reward).
                    if (lifeEggCount_ < kMaxEggCount)
                    {
                        obj.active = false;
                        // Real end point VoyageGetPosVie(m_nbVies+1) =
                        // (210+16*(lifeEggCount_+1), 417), Decor.cpp:10141-
                        // 10147/6012.
                        RequestVoyage(VoyageKind::Egg, 21, false, obj.currentX, obj.currentY, obj.currentZ,
                                      210.0f + 16.0f * static_cast<float>(lifeEggCount_ + 1), 417.0f, true);
                    }
                    break;
                case ObjectType::ObjectType29: // bullet pack
                    // Real gate: only picked up below the cap (real
                    // m_blupiBullet < 10) -- touching a pack while already
                    // at max ammo does nothing at all, not even removed.
                    // Real reward is granted immediately (NOT deferred --
                    // Decor.cpp:5739, unlike every other reward-bearing
                    // pickup above), but it still triggers the SAME single
                    // voyage slot (force-completing whatever else is in
                    // flight) and the deferred completion sound -- real
                    // source plays no immediate sound here either
                    // (Decor.cpp:5731-5744, no PlaySound call, no
                    // VoyageInit icon==177 case) -- the previous immediate
                    // `SoundChannel54` was a mismatch, removed 2026-07-14.
                    if (bulletCount_ < kBulletCap)
                    {
                        bulletCount_ = kBulletCap;
                        obj.active = false;
                        // Real end point (570,430), Decor.cpp:5736-5738.
                        RequestVoyage(VoyageKind::BulletPack, 177, false, obj.currentX, obj.currentY, obj.currentZ,
                                      570.0f, 430.0f, true);
                    }
                    break;
                // Secret powers (plan.md E3D-MIG-170, real Decor.cpp
                // ~6014-6087) -- Shield/Charge/Invert really are automatic
                // on contact; Sucette/Drink additionally require the real
                // action button held at contact (`blupiActionPressedEdge`,
                // Decor.cpp:6025/6053, plan.md `173`, found 2026-07-14).
                // Power/Cloud/Hide's own real 2-stage grab/freeze/complete
                // delay is handled by the caller (GEBlupiController::
                // TriggerPickupFreeze(), since this class has no access to
                // it) -- see EventKind's own comment on the position
                // payload. Invert/Mirror (ObjectType40, plan.md PICKUP-011)
                // is a separate real pickup family sharing the same
                // instant-grant-on-contact shape as Shield/Charge. Real
                // per-pickup gates, including Power/Cloud/Hide's real
                // vehicle-mode + Balloon/Ecrase exclusions (Decor.cpp:
                // 6025-6087; Shield/Invert have none, verified 2026-07-16),
                // are passed in from the caller's own GEBlupiController
                // state (blupiCanGrantShield/Power/Cloud/Hide/Invert) since
                // this class has no access to GEBlupiController itself --
                // consistent with the blupiBallooned/blupiCrouching
                // precedent. INFRA-006 pilot (plan.md §7): these 5 types
                // share one handler-table-driven dispatch,
                // TryGrantSecretPowerPickup() -- see its own comment and
                // kSecretPowerPickupHandlers above.
                case ObjectType::ObjectType25: // shield stick
                case ObjectType::ObjectType26: // suction-cup ("Sucette" -> Power)
                case ObjectType::ObjectType30: // drink ("Drink" -> Hide)
                case ObjectType::ObjectType31: // charge ("Charge" -> Cloud)
                case ObjectType::ObjectType40: // mirror/invert
                    TryGrantSecretPowerPickup(obj, blupiCanGrantShield, blupiCanGrantPower, blupiCanGrantCloud,
                                              blupiCanGrantHide, blupiCanGrantInvert, blupiActionPressedEdge);
                    break;
                default:
                    break;
            }
        }

        if (!touchingExitThisFrame)
        {
            exitContactActive_ = false;
        }

        // Key-gated doors (plan.md E3D-MIG-160/161, real `Decor::IsDoor`
        // ~7360): probes Blupi's own cell AND one cell further in his
        // facing direction (blupiFacingDX/DZ) for a door icon (334-336),
        // matching the real "trigger a step before actually reaching it"
        // detection exactly. Opens automatically if the matching key is
        // currently held -- no action-button gate in the real source,
        // unlike switches/dynamite. Real key flags are NOT consumed on
        // pickup, only on use (`Decor.cpp` ~5619) -- modeled here as
        // clearing the whole count to 0 (this engine's keys1_/keys2_/
        // keys3_ are plain pickup counters, not a persisted bitmask, but
        // real levels only ever grant one of each key before it must be
        // re-collected, so "count > 0" / "clear to 0" behaves identically
        // to the real boolean flag for the realistic case). Real voyage-
        // deferred key-flag-setting (pickup consumed from the world
        // immediately, but not "held" until a HUD-fly animation completes)
        // is NOT modeled -- same simplification as every other pickup this
        // session, applied immediately instead.
        {
            auto& terrain = worldRuntime.GetWorldMutable();
            const int axis = static_cast<int>(terrain.blocksPerAxis());
            const int probeGX[2] = {
                static_cast<int>(std::lround(blupiX)) + GEWorldRuntime::kWorldCenterX,
                static_cast<int>(std::lround(blupiX + static_cast<float>(blupiFacingDX))) +
                    GEWorldRuntime::kWorldCenterX,
            };
            const int probeGZ[2] = {
                static_cast<int>(std::lround(blupiZ)) + GEWorldRuntime::kWorldCenterZ,
                static_cast<int>(std::lround(blupiZ + static_cast<float>(blupiFacingDZ))) +
                    GEWorldRuntime::kWorldCenterZ,
            };
            const int probeGY = static_cast<int>(std::lround(blupiY));
            for (int p = 0; p < 2; ++p)
            {
                const int gx = probeGX[p];
                const int gz = probeGZ[p];
                if (gx < 0 || gx >= axis || gz < 0 || gz >= axis || probeGY < 0 || probeGY >= axis)
                {
                    continue;
                }
                const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(probeGY),
                                                    static_cast<std::uint16_t>(gz))
                                       .type();
                if (!GalaxyEggbert::BlockTypes::isDoor(icon))
                {
                    continue;
                }
                const int keyType = GalaxyEggbert::BlockTypes::doorKeyType(icon);
                const bool hasKey = (keyType == 49 && keys1_ > 0) || (keyType == 50 && keys2_ > 0) ||
                                     (keyType == 51 && keys3_ > 0);
                if (hasKey)
                {
                    OpenDoorAt(worldRuntime, gx, probeGY, gz, sound);
                    if (keyType == 49) keys1_ = 0;
                    else if (keyType == 50) keys2_ = 0;
                    else if (keyType == 51) keys3_ = 0;
                    // Real door-unlock key-consumption flourish (plan.md
                    // `158`, Decor.cpp:5620-5624) -- purely cosmetic, the
                    // key bit is already cleared above, before the voyage
                    // even starts (no reward at completion). Reversed
                    // direction from every pickup above: start = the
                    // fixed HUD position of whichever key was consumed
                    // (matching Key1/2/3's own real end points), end = the
                    // door's own world position (needs projecting, unlike
                    // every pickup's fixed end). Real icon is dynamic per
                    // door: `214 + (doorIcon-334)*7` = 214/221/228 for
                    // door1/2/3.
                    const float startX = (keyType == 49) ? 520.0f : (keyType == 50) ? 530.0f : 540.0f;
                    const int doorIconId = 214 + (static_cast<int>(icon) - 334) * 7;
                    const float doorWorldX = static_cast<float>(gx) - static_cast<float>(GEWorldRuntime::kWorldCenterX);
                    const float doorWorldY = static_cast<float>(probeGY);
                    const float doorWorldZ = static_cast<float>(gz) - static_cast<float>(GEWorldRuntime::kWorldCenterZ);
                    RequestVoyage(VoyageKind::DoorUnlock, doorIconId, false, doorWorldX, doorWorldY, doorWorldZ,
                                  startX, 418.0f, false);
                }
            }
        }

        // Treasure-gated doors (plan.md E3D-MIG-162, real
        // Decor::OpenDoorsTresor ~11642): a door needing N treasures uses
        // icon 420+N -- scans the whole grid and opens every one whose
        // requirement is now met, all at once. Real reward timing (plan.md
        // `158`) means this now happens inside ApplyVoyageReward() itself
        // (called from TickVoyage() above, or from BeginVoyage()'s own
        // force-complete path), the instant a treasure voyage completes --
        // not unconditionally here every frame. Shared with
        // CheatAllTreasure() below, which still scans immediately (the
        // cheat is a separate, real immediate-credit mechanic).

        // Bridge construction trigger (ObjectType52, plan.md PICKUP-064, real
        // Decor::IsBridge ~7334, called every tick from Decor.cpp ~5608-5612
        // while Blupi has focus -- not action-button gated, unlike switches/
        // dynamite). Real source probes 2 candidate foot-height offsets;
        // this engine uses a single stance height (round(blupiY)-1, same
        // convention as TryActivateSwitch's own standing-cell check) --
        // a documented simplification, not a transcribed real detail.
        // Spawns exactly one ObjectType52 at the Bridge cell, guarded so a
        // second never stacks on top of an already-in-progress one.
        {
            int bridgeGX, bridgeGY, bridgeGZ;
            ToGridCell(world, blupiX, blupiY - 1.0f, blupiZ, bridgeGX, bridgeGY, bridgeGZ);
            if (world.getBlock(static_cast<std::uint16_t>(bridgeGX), static_cast<std::uint16_t>(bridgeGY),
                                static_cast<std::uint16_t>(bridgeGZ))
                    .type() == GalaxyEggbert::BlockTypes::Bridge)
            {
                bool alreadyBuilding = false;
                for (const auto& obj : objects)
                {
                    if (obj.active && obj.type == ObjectType::ObjectType52 &&
                        std::lround(obj.currentX) == std::lround(blupiX) &&
                        std::lround(obj.currentZ) == std::lround(blupiZ))
                    {
                        alreadyBuilding = true;
                        break;
                    }
                }
                if (!alreadyBuilding)
                {
                    // currentY stays in world-space "standing height" (grid
                    // Y + 1), the same convention every other MobileObjSpec
                    // uses -- the per-tick terrain write below converts back
                    // to a grid index the same way TryActivateSwitch's own
                    // standing-cell check does.
                    MobileObjSpec bridge{};
                    bridge.type = ObjectType::ObjectType52;
                    bridge.posStartX = bridge.posEndX = bridge.currentX = static_cast<float>(std::lround(blupiX));
                    bridge.posStartY = bridge.posEndY = bridge.currentY = static_cast<float>(bridgeGY) + 1.0f;
                    bridge.posStartZ = bridge.posEndZ = bridge.currentZ = static_cast<float>(std::lround(blupiZ));
                    bridge.phase = 0.0f;
                    bridge.active = true;
                    pendingSpawns.push_back(bridge);
                    sound.Play(GalaxyEggbert::SoundChannel::SoundChannel72);
                }
            }
        }

        // Player-fired Tank bullet (2026-07-13, plan.md BULLET-001, real
        // Decor.cpp:4308-4344) -- see Update()'s own class comment for the
        // full real-behavior citation. Cooldown ticks down every frame
        // regardless of input; only reset on an actual shot.
        fireCooldownTimer_ = std::max(0.0f, fireCooldownTimer_ - dt);
        if (blupiFirePressed && blupiCanFire && fireCooldownTimer_ <= 0.0f)
        {
            if (bulletCount_ > 0)
            {
                int gx, gy, gz;
                ToGridCell(world, blupiX, blupiY, blupiZ, gx, gy, gz);
                const int dist = SearchAirDistance(world, gx, gy, gz, blupiFacingDX, 0, blupiFacingDZ);
                --bulletCount_;
                fireCooldownTimer_ = kFireCooldownSeconds;
                events_.push_back(Event{EventKind::TankFired});
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel52);
                if (dist > 0)
                {
                    MobileObjSpec blupiPos{};
                    blupiPos.currentX = blupiX;
                    blupiPos.currentY = blupiY;
                    blupiPos.currentZ = blupiZ;
                    pendingSpawns.push_back(MakeBullet(blupiPos, static_cast<float>(blupiFacingDX), 0.0f,
                                                        static_cast<float>(blupiFacingDZ), dist));
                }
            }
            else
            {
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel53);
            }
        }

        // Flush this frame's blupih/blupit shots into the live object list
        // (deferred from the loop above, see pendingSpawns' own comment) --
        // reuse an inactive slot first, matching the real fixed-pool
        // MoveObjectFree()'s slot-reuse semantics rather than growing
        // `objects` unboundedly every time a shooter fires.
        for (auto& spec : pendingSpawns)
        {
            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = spec;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(spec);
            }
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

    bool GEInteractionSystem::PlaceDynamite(GEWorldRuntime& worldRuntime, float x, float y, float z, bool grounded,
                                             bool blupiCanUseHands)
    {
        // Real gate (Decor.cpp ~4792-4812): carrying at least one, and
        // solid ground under both feet -- approximated here as `grounded`
        // (this engine's single-point collision has no separate left/right
        // foot check). Real "not in any vehicle mode" clause fixed
        // 2026-07-16 -- see blupiCanUseHands's own header comment
        // ("lift-transported" still isn't modeled, no such concept exists).
        if (dynamiteCount_ <= 0 || !grounded || !blupiCanUseHands)
        {
            return false;
        }
        --dynamiteCount_;

        MobileObjSpec fuse;
        fuse.type = ObjectType::ObjectType56;
        fuse.posStartX = fuse.posEndX = fuse.currentX = x;
        fuse.posStartY = fuse.posEndY = fuse.currentY = y;
        fuse.posStartZ = fuse.posEndZ = fuse.currentZ = z;
        fuse.phase = 0.0f;
        fuse.active = true;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = fuse;
                return true;
            }
        }
        objects.push_back(fuse);
        return true;
    }

    bool GEInteractionSystem::TryPerso(GEWorldRuntime& worldRuntime, float x, float y, float z, bool grounded,
                                        bool blupiCanUseHands)
    {
        auto& objects = worldRuntime.GetMobileObjectsMutable();

        // Real pickup (Decor.cpp ~6088-6101): standing near an already-
        // placed decoy takes priority over placing a new one (matching the
        // real source's own MoveObjectDetect check before the placement
        // branch). Proximity radius matches every other pickup/interaction
        // check in this file. Real reward is deferred to voyage completion
        // (plan.md `158`, fixed 2026-07-14) -- see RequestVoyage()'s own
        // callers for the full citation.
        constexpr float kPersoRadius = 1.1f;
        for (auto& obj : objects)
        {
            if (!obj.active || obj.type != ObjectType::ObjectType200)
            {
                continue;
            }
            const float dx = obj.currentX - x;
            const float dy = obj.currentY - y;
            const float dz = obj.currentZ - z;
            if (dx * dx + dy * dy + dz * dz < kPersoRadius * kPersoRadius)
            {
                if (persoCount_ >= kPersoCap)
                {
                    return false;
                }
                obj.active = false;
                // Real end point (0,438), Decor.cpp:6098-6100.
                RequestVoyage(VoyageKind::Perso, 108, true, obj.currentX, obj.currentY, obj.currentZ, 0.0f, 438.0f,
                              true);
                return true;
            }
        }

        // Real placement (Decor.cpp ~4818-4841): carrying at least one,
        // and solid ground under both feet -- approximated as `grounded`,
        // same simplification as PlaceDynamite() above. Real vehicle-mode
        // gate fixed 2026-07-16 -- see blupiCanUseHands's own header
        // comment; deliberately does NOT apply to the pickup branch above.
        if (persoCount_ <= 0 || !grounded || !blupiCanUseHands)
        {
            return false;
        }
        --persoCount_;

        MobileObjSpec decoy;
        decoy.type = ObjectType::ObjectType200;
        decoy.posStartX = decoy.posEndX = decoy.currentX = x;
        decoy.posStartY = decoy.posEndY = decoy.currentY = y;
        decoy.posStartZ = decoy.posEndZ = decoy.currentZ = z;
        decoy.phase = 0.0f;
        decoy.active = true;

        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = decoy;
                return true;
            }
        }
        objects.push_back(decoy);
        return true;
    }

    void GEInteractionSystem::CheatOpenDoors(GEWorldRuntime& worldRuntime, GESound& sound)
    {
        auto& terrain = worldRuntime.GetWorldMutable();
        const int axis = static_cast<int>(terrain.blocksPerAxis());
        for (int gx = 0; gx < axis; ++gx)
        {
            for (int gy = 0; gy < axis; ++gy)
            {
                for (int gz = 0; gz < axis; ++gz)
                {
                    const auto icon = terrain.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                        static_cast<std::uint16_t>(gz))
                                           .type();
                    // Both real door families, opened regardless of
                    // whether Blupi actually holds the matching key/
                    // treasure count -- see this method's own header
                    // comment for the real-vs-adapted reasoning (440 is
                    // this session's own confirmed real total icon
                    // ceiling, a safe upper bound for the treasure-door
                    // family's icon range).
                    if (GalaxyEggbert::BlockTypes::isDoor(icon) || (icon >= 421 && icon <= 440))
                    {
                        OpenDoorAt(worldRuntime, gx, gy, gz, sound);
                    }
                }
            }
        }
    }

    bool GEInteractionSystem::CheatCleanAll(GEWorldRuntime& worldRuntime)
    {
        bool anyDestroyed = false;
        for (auto& obj : worldRuntime.GetMobileObjectsMutable())
        {
            if (!obj.active)
            {
                continue;
            }
            if (IsGenericHazard(obj.type) || obj.type == ObjectType::ObjectType32 ||
                obj.type == ObjectType::ObjectType33 || obj.type == ObjectType::ObjectType44 ||
                obj.type == ObjectType::ObjectType54)
            {
                obj.active = false;
                anyDestroyed = true;
            }
        }
        return anyDestroyed;
    }

    void GEInteractionSystem::SpawnInvertBurst(GEWorldRuntime& worldRuntime, float blupiX, float blupiY,
                                                float blupiZ, bool isGrant)
    {
        // Real behavior confirmed via direct Decor.cpp read (the general
        // `Decor::ObjectStart()` sets posStart=the spawn point itself,
        // posEnd=spawn point + the bucketed direction's real 500px reach,
        // step=2, stepAdvance=ScaleTime(|speedMagnitude*500/64|)=78 at this
        // build's 20Hz reference rate -- NOT an instant static burst; the
        // object is meant to visibly slide from posStart to posEnd over 78
        // ticks, though it always self-deletes (phase>=16, see below) long
        // before actually arriving). Grant spawns each instance exactly AT
        // Blupi (no pre-offset), reaching a full 500px-out posEnd. Expiry
        // additionally pre-offsets each spawn point 100px in the OPPOSITE
        // direction before the same 500px push (`Decor.cpp:5145-5156`,
        // e.g. the "up" instance spawns 100px below Blupi then pushes up
        // 500px, netting a 400px-above final posEnd but sweeping back
        // through Blupi's own position along the way) -- both use the same
        // 64px-per-tile scale used throughout this engine's own atlas/
        // movement math.
        constexpr float kReach = 500.0f / 64.0f;
        constexpr float kExpiryPreOffset = 100.0f / 64.0f;
        const ObjectType type = isGrant ? ObjectType::ObjectType41 : ObjectType::ObjectType42;

        // Real screen-Y (up/down) maps to this engine's world-Y (height),
        // same convention already established for camera shake and Ghost
        // mode's vertical flight -- real screen-up (the real `speed=-60`
        // case) is world +Y here, real screen-down (`speed=60`) is world
        // -Y. Real screen-X (left/right) maps directly to world X with no
        // sign flip, same convention used everywhere else this session.
        const float dirs[4][3] = {
            {0.0f, 1.0f, 0.0f},  // up
            {0.0f, -1.0f, 0.0f}, // down
            {1.0f, 0.0f, 0.0f},  // +X (real "right")
            {-1.0f, 0.0f, 0.0f}, // -X (real "left")
        };

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (const auto& dir : dirs)
        {
            const float startOffset = isGrant ? 0.0f : -kExpiryPreOffset;
            const float endOffset = isGrant ? kReach : (kReach - kExpiryPreOffset);

            MobileObjSpec spec;
            spec.type = type;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = blupiX + dir[0] * startOffset;
            spec.currentY = spec.posStartY = blupiY + dir[1] * startOffset;
            spec.currentZ = spec.posStartZ = blupiZ + dir[2] * startOffset;
            spec.posEndX = blupiX + dir[0] * endOffset;
            spec.posEndY = blupiY + dir[1] * endOffset;
            spec.posEndZ = blupiZ + dir[2] * endOffset;
            spec.patrolStep = 2; // real step=2 -- skips the dwell-at-start phase, matches Pollution puff's own spawn
            spec.patrolTime = 0.0f;
            spec.stepAdvanceTicks = 78.0f; // real |magnitude-10 * 500/64| -- same for grant and expiry, see comment above

            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = spec;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(spec);
            }
        }
    }

    void GEInteractionSystem::SpawnSawDeathBurst(GEWorldRuntime& worldRuntime, float blupiX, float blupiY,
                                                  float blupiZ, GESound& sound)
    {
        // Real `Decor::BlupiDead`'s own Clear4 branch (Decor.cpp:
        // 6608-6613): 3 `ObjectStart(pos, ObjectType41, speed)` calls with
        // speed -70/20/-20, decoded via `Decor::ObjectStart`'s own real
        // direction/magnitude logic (Decor.cpp:7805-7869, same function
        // SpawnInvertBurst() above already ports): speed<-50 -> up
        // (magnitude = |speed+50| = 20), speed>0 -> right (magnitude 20),
        // speed<0 -> left (magnitude 20) -- no "down" direction, unlike
        // Invert's 4-direction burst. Same real 500px short-circuit reach
        // (`SearchDistRight()`'s own flat-500 case for ObjectType41,
        // confirmed by SpawnInvertBurst()'s own comment) but a LARGER
        // stepAdvance than Invert's (magnitude 20 vs Invert's 10, so real
        // `ScaleTime(|20*500/64|)` = 156 ticks, not 78).
        constexpr float kReach = 500.0f / 64.0f;
        const float dirs[3][3] = {
            {0.0f, 1.0f, 0.0f},  // up
            {1.0f, 0.0f, 0.0f},  // +X (real "right")
            {-1.0f, 0.0f, 0.0f}, // -X (real "left")
        };

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (const auto& dir : dirs)
        {
            MobileObjSpec spec;
            spec.type = ObjectType::ObjectType41;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = blupiX;
            spec.currentY = spec.posStartY = blupiY;
            spec.currentZ = spec.posStartZ = blupiZ;
            spec.posEndX = blupiX + dir[0] * kReach;
            spec.posEndY = blupiY + dir[1] * kReach;
            spec.posEndZ = blupiZ + dir[2] * kReach;
            spec.patrolStep = 2;
            spec.patrolTime = 0.0f;
            spec.stepAdvanceTicks = 156.0f; // real |magnitude 20 * 500/64|

            bool placed = false;
            for (auto& slot : objects)
            {
                if (!slot.active)
                {
                    slot = spec;
                    placed = true;
                    break;
                }
            }
            if (!placed)
            {
                objects.push_back(spec);
            }
        }
        sound.Play(GalaxyEggbert::SoundChannel::SoundChannel75);
    }

    void GEInteractionSystem::SpawnFanHitFlash(GEWorldRuntime& worldRuntime, float x, float y, float z)
    {
        MobileObjSpec spec;
        spec.type = ObjectType::ObjectType11;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = x;
        spec.currentY = spec.posStartY = spec.posEndY = y;
        spec.currentZ = spec.posStartZ = spec.posEndZ = z;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    void GEInteractionSystem::RespawnPickupItem(GEWorldRuntime& worldRuntime, float x, float y, float z,
                                                 GalaxyEggbert::ObjectType type)
    {
        // Real `ObjectStart(pos, type, 0)` at the real 2-stage pickup's own completion
        // (Decor.cpp:3212-3235/3048-3057, plan.md `173`) -- a static respawn (speed=0, no
        // posEnd/patrol movement) of the SAME pickup at its original position.
        MobileObjSpec spec;
        spec.type = type;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = x;
        spec.currentY = spec.posStartY = spec.posEndY = y;
        spec.currentZ = spec.posStartZ = spec.posEndZ = z;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    void GEInteractionSystem::SpawnTeleportArc(GEWorldRuntime& worldRuntime, float x, float y, float z)
    {
        MobileObjSpec spec;
        spec.type = ObjectType::ObjectType92;
        spec.active = true;
        spec.phase = 0.0f;
        constexpr float kOffsetY = 5.0f / 64.0f; // real celSwitch.Y = blupiPos.Y - 5 (screen), world Y+ (sign flip)
        spec.currentX = spec.posStartX = spec.posEndX = x;
        spec.currentY = spec.posStartY = spec.posEndY = y + kOffsetY;
        spec.currentZ = spec.posStartZ = spec.posEndZ = z;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    bool GEInteractionSystem::HasActiveObjectOfType(const GEWorldRuntime& worldRuntime, ObjectType type) const
    {
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (obj.active && obj.type == type)
            {
                return true;
            }
        }
        return false;
    }

    void GEInteractionSystem::SpawnWaterSplash(GEWorldRuntime& worldRuntime, ObjectType type, float x, float y,
                                                float z)
    {
        MobileObjSpec spec;
        spec.type = type;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = x;
        spec.currentY = spec.posStartY = spec.posEndY = y;
        spec.currentZ = spec.posStartZ = spec.posEndZ = z;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    void GEInteractionSystem::SpawnWaterBubble(GEWorldRuntime& worldRuntime, const Worlds::World& world, float x,
                                                float y, float z)
    {
        // Real `pos.Y -= 20` pre-offset (Decor.cpp:7037) before the column
        // scan -- a small upward nudge, less than one full 64px tile.
        constexpr float kPreOffset = 20.0f / 64.0f;
        const float startY = y + kPreOffset;

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = static_cast<int>(std::lround(x)) + GEWorldRuntime::kWorldCenterX;
        const int gz = static_cast<int>(std::lround(z)) + GEWorldRuntime::kWorldCenterZ;
        int count = 0;
        if (gx >= 0 && gz >= 0 && gx < blocksPerAxis && gz < blocksPerAxis)
        {
            for (int gy = static_cast<int>(std::lround(startY)); gy < blocksPerAxis; ++gy)
            {
                if (gy < 0)
                {
                    continue;
                }
                const auto block = world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                    static_cast<std::uint16_t>(gz))
                                        .type();
                if (!GalaxyEggbert::BlockTypes::isWater(block))
                {
                    break;
                }
                ++count;
            }
        }
        --count; // real `num--` (Decor.cpp:7050) -- don't count the tile Blupi is standing in

        if (count <= 0)
        {
            return;
        }

        MobileObjSpec spec;
        spec.type = ObjectType::ObjectType15;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = x;
        spec.currentY = spec.posStartY = startY;
        spec.posEndY = startY + static_cast<float>(count);
        spec.currentZ = spec.posStartZ = spec.posEndZ = z;
        spec.patrolStep = 2; // real step=2 -- skips the dwell-at-start phase, matches SpawnInvertBurst()'s own spawn
        spec.patrolTime = 0.0f;
        spec.stepAdvanceTicks = static_cast<float>(count) * 10.0f; // real ScaleTime(count*10), 1:1 tick convention

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    void GEInteractionSystem::TickPollutionPuff(GEWorldRuntime& worldRuntime, float blupiX, float blupiY,
                                                 float blupiZ, bool isHelicopter, bool isOvercraft, bool isJeep,
                                                 bool isTank, bool isMoving, bool ascending, int facingDX)
    {
        ++pollutionTick_;
        const int realTime = pollutionTick_;       // real m_time -- exact for Helicopter/Overcraft below
        const int blupiPhase = pollutionTick_;     // stands in for real m_blupiPhase -- see this method's own header comment

        bool flag = false;
        float pointX = 0.0f, pointY = 0.0f;
        int num = 20;

        if (isHelicopter)
        {
            if (ascending)
            {
                if (realTime % 20 != 0 && realTime % 20 != 2 && realTime % 20 != 5 && realTime % 20 != 8 &&
                    realTime % 20 != 10 && realTime % 20 != 11 && realTime % 20 != 16 && realTime % 20 != 18)
                {
                    return;
                }
            }
            else if (!isMoving)
            {
                if (realTime % 50 != 0 && realTime % 50 != 12 && realTime % 50 != 30)
                {
                    return;
                }
            }
            else if (realTime % 20 != 0 && realTime % 20 != 3 && realTime % 20 != 5 && realTime % 20 != 11 &&
                     realTime % 20 != 15)
            {
                return;
            }
            pointX = 22.0f;
            flag = true;
        }
        if (isOvercraft)
        {
            if (ascending)
            {
                if (realTime % 20 != 0 && realTime % 20 != 2 && realTime % 20 != 5 && realTime % 20 != 8 &&
                    realTime % 20 != 11 && realTime % 20 != 13 && realTime % 20 != 14 && realTime % 20 != 18)
                {
                    return;
                }
                num = 58;
                // Real `m_random.get()->Next(-10, 10)` -- no RNG exists
                // anywhere else in this engine; a small deterministic
                // stand-in seeded by the tick counter is used instead,
                // since only cosmetic jitter (not gameplay) depends on it.
                pointX = static_cast<float>(((pollutionTick_ * 1103515245 + 12345) / 65536) % 21 - 10);
                pointY = 22.0f;
            }
            else
            {
                if (realTime % 50 != 0 && realTime % 50 != 12 && realTime % 50 != 30)
                {
                    return;
                }
                num = 20;
                pointX = 30.0f;
            }
            flag = true;
        }
        if (isJeep)
        {
            if (!isMoving)
            {
                if (blupiPhase % 50 != 0 && blupiPhase % 50 != 12 && blupiPhase % 50 != 20 && blupiPhase % 50 != 35)
                {
                    return;
                }
            }
            else if (blupiPhase % 20 != 0 && blupiPhase % 20 != 3 && blupiPhase % 20 != 5 && blupiPhase % 20 != 11 &&
                     blupiPhase % 20 != 15)
            {
                return;
            }
            pointX = 32.0f;
            flag = true;
        }
        if (isTank)
        {
            if (!isMoving)
            {
                if (blupiPhase % 50 != 0 && blupiPhase % 50 != 15 && blupiPhase % 50 != 28)
                {
                    return;
                }
            }
            else if (blupiPhase % 20 != 0 && blupiPhase % 20 != 4 && blupiPhase % 20 != 12)
            {
                return;
            }
            pointX = 35.0f;
            flag = true;
        }
        if (!flag)
        {
            return;
        }

        float spawnX = blupiX;
        if (facingDX >= 0) // real Direction::Right
        {
            spawnX -= (pointX - 5.0f) / 64.0f;
            if (num < 50)
            {
                num = -num;
            }
        }
        else
        {
            spawnX += pointX / 64.0f;
        }
        const float spawnY = blupiY - pointY / 64.0f; // real screen-Y+ -> this engine's world Y- (established sign flip)
        const float spawnZ = blupiZ;

        // Real `ObjectStart()`'s speed-bucket encoding (same as
        // SpawnInvertBurst()/AppendSparkleBurst() -- >50 down, <-50 up
        // [never reached here, |num|<=58], >0 right, <0 left) and its real
        // flat 500px reach for this ObjectType (`Decor::SearchDistRight()`,
        // `Decor.cpp:7628-7653`).
        constexpr float kReach = 500.0f / 64.0f;
        float endX = spawnX, endY = spawnY;
        int magnitude;
        if (num > 50)
        {
            endY -= kReach;
            magnitude = num - 50;
        }
        else if (num > 0)
        {
            endX += kReach;
            magnitude = num;
        }
        else
        {
            endX -= kReach;
            magnitude = -num;
        }

        MobileObjSpec spec;
        spec.type = ObjectType::ObjectType36;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spawnX;
        spec.currentY = spec.posStartY = spawnY;
        spec.currentZ = spec.posStartZ = spawnZ;
        spec.posEndX = endX;
        spec.posEndY = endY;
        spec.posEndZ = spawnZ;
        spec.patrolStep = 2; // real step=2 -- skips the dwell-at-start phase entirely
        spec.patrolTime = 0.0f;
        spec.stepAdvanceTicks = static_cast<float>(std::abs(magnitude * 500 / 64));

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    void GEInteractionSystem::ResetMagicTrail(float x, float y, float z)
    {
        magicTrailLastX_ = x;
        magicTrailLastY_ = y;
        magicTrailLastZ_ = z;
    }

    void GEInteractionSystem::TickMagicTrail(GEWorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                                              bool isShielded, bool isPowered)
    {
        if (!isShielded && !isPowered)
        {
            return;
        }

        // Real Manhattan distance check ignores Z (Decor.cpp:5204-5237,
        // same 2D-source-only convention as every other real X/Y-only
        // check this session).
        constexpr float kThreshold = 40.0f / 64.0f;
        const float dist = std::fabs(blupiX - magicTrailLastX_) + std::fabs(blupiY - magicTrailLastY_);
        if (dist < kThreshold)
        {
            return;
        }

        MobileObjSpec spec;
        spec.type = isShielded ? ObjectType::ObjectType57 : ObjectType::ObjectType27;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = blupiX;
        spec.currentY = spec.posStartY = spec.posEndY = blupiY;
        spec.currentZ = spec.posStartZ = spec.posEndZ = blupiZ;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        bool placed = false;
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            objects.push_back(spec);
        }

        ResetMagicTrail(blupiX, blupiY, blupiZ);
    }

    void GEInteractionSystem::ReplaceEvent(EventKind kind, Event newEvent)
    {
        events_.erase(std::remove_if(events_.begin(), events_.end(),
                                      [kind](const Event& event) { return event.kind == kind; }),
                      events_.end());
        events_.push_back(newEvent);
    }

    void GEInteractionSystem::RequestVoyage(VoyageKind kind, int iconId, bool isButtonChannel, float worldX,
                                             float worldY, float worldZ, float fixedX, float fixedY,
                                             bool worldIsStart)
    {
        // Overwrites any UN-CONSUMED same-frame request (see the public
        // API comment's documented rare-edge-case simplification).
        Event event{EventKind::VoyageRequested};
        event.voyageKind = kind;
        event.voyageIconId = iconId;
        event.voyageIsButtonChannel = isButtonChannel;
        event.voyageWorldX = worldX;
        event.voyageWorldY = worldY;
        event.voyageWorldZ = worldZ;
        event.voyageFixedX = fixedX;
        event.voyageFixedY = fixedY;
        event.voyageWorldIsStart = worldIsStart;
        event.voyageIsAscend = false;
        ReplaceEvent(EventKind::VoyageRequested, event);
    }

    void GEInteractionSystem::RequestClear2Ascend(float worldX, float worldY, float worldZ)
    {
        Event event{EventKind::VoyageRequested};
        event.voyageKind = VoyageKind::Clear2Ascend;
        event.voyageIconId = 230;
        event.voyageIsButtonChannel = false;
        event.voyageWorldX = worldX;
        event.voyageWorldY = worldY;
        event.voyageWorldZ = worldZ;
        event.voyageWorldIsStart = true;
        event.voyageIsAscend = true;
        event.voyageAscendOffsetY = 300.0f; // real Decor.cpp:6595 (Clear2's own pos2.Y offset)
        ReplaceEvent(EventKind::VoyageRequested, event);
    }

    void GEInteractionSystem::TryGrantSecretPowerPickup(MobileObjSpec& obj, bool blupiCanGrantShield,
                                                         bool blupiCanGrantPower, bool blupiCanGrantCloud,
                                                         bool blupiCanGrantHide, bool blupiCanGrantInvert,
                                                         bool blupiActionPressedEdge)
    {
        for (const auto& handler : kSecretPowerPickupHandlers)
        {
            if (handler.type != obj.type)
            {
                continue;
            }
            const bool gateOpen = [&]
            {
                switch (handler.gate)
                {
                    case SecretPowerPickupHandler::Gate::Shield: return blupiCanGrantShield;
                    case SecretPowerPickupHandler::Gate::Power:  return blupiCanGrantPower;
                    case SecretPowerPickupHandler::Gate::Cloud:  return blupiCanGrantCloud;
                    case SecretPowerPickupHandler::Gate::Hide:   return blupiCanGrantHide;
                    case SecretPowerPickupHandler::Gate::Invert: return blupiCanGrantInvert;
                }
                return false;
            }();
            if (gateOpen && (!handler.requiresActionButton || blupiActionPressedEdge))
            {
                obj.active = false;
                Event event{handler.grantedEvent};
                if (handler.hasPositionPayload)
                {
                    event.pickupX = obj.currentX;
                    event.pickupY = obj.currentY;
                    event.pickupZ = obj.currentZ;
                }
                events_.push_back(event);
            }
            return;
        }
    }

    void GEInteractionSystem::TryCollectBasicPickup(MobileObjSpec& obj, bool dynamiteGateOpen,
                                                     std::vector<MobileObjSpec>& pendingSpawns)
    {
        for (const auto& handler : kBasicPickupHandlers)
        {
            if (handler.type != obj.type)
            {
                continue;
            }
            if (handler.requiresDynamiteGate && !dynamiteGateOpen)
            {
                return;
            }
            obj.active = false;
            RequestVoyage(handler.voyageKind, handler.voyageIcon, false, obj.currentX, obj.currentY, obj.currentZ,
                          handler.endX, handler.endY, true);
            if (handler.spawnsSparkleBurst)
            {
                AppendSparkleBurst(obj.currentX, obj.currentY, obj.currentZ, pendingSpawns);
            }
            return;
        }
    }

    void GEInteractionSystem::BeginVoyage(GEWorldRuntime& worldRuntime, VoyageKind kind, int iconId,
                                           bool isButtonChannel, float startX, float startY, float endX, float endY,
                                           GESound& sound, float worldAnchorX, float worldAnchorY,
                                           float worldAnchorZ)
    {
        // Real `VoyageInit()`'s own force-complete-the-previous-one
        // behavior (`Decor.cpp:10160-10164`).
        if (voyageKind_ != VoyageKind::None)
        {
            ApplyVoyageReward(worldRuntime, sound);
        }

        voyageKind_ = kind;
        voyageIconId_ = iconId;
        voyageIsButton_ = isButtonChannel;
        voyageStartX_ = startX;
        voyageStartY_ = startY;
        voyageEndX_ = endX;
        voyageEndY_ = endY;
        voyagePhase_ = 0.0f;
        voyageWorldAnchorX_ = worldAnchorX;
        voyageWorldAnchorY_ = worldAnchorY;
        voyageWorldAnchorZ_ = worldAnchorZ;
        // Real `(|dx|+|dy|)/10`, integer-truncated (`Decor.cpp:10169-10172`).
        const int dx = static_cast<int>(std::fabs(endX - startX));
        const int dy = static_cast<int>(std::fabs(endY - startY));
        voyageTotal_ = static_cast<float>((dx + dy) / 10);
        // Real fixed-duration overrides (`Decor.cpp:10222-10226`) --
        // NOT distance-proportional despite the 300px/2000px real ascend
        // distance, matching real VoyageInit's own icon==230/40 overrides
        // exactly.
        if (kind == VoyageKind::Clear2Ascend)
        {
            voyageTotal_ = 100.0f;
        }
        else if (kind == VoyageKind::Clear3Ascend)
        {
            voyageTotal_ = 50.0f;
        }
        else if (kind == VoyageKind::LifeLoss)
        {
            voyageTotal_ = 40.0f; // real ScaleTime(40), Decor.cpp:10173
        }

        // Real touch-time sounds (`Decor::VoyageInit`'s own per-icon
        // cases) -- independent of the deferred reward-applied sound
        // played later in ApplyVoyageReward(). Treasure's completes-the-
        // set distinction is decided by the caller (RequestVoyage()'s own
        // iconId/kind are fixed per pickup, but the SOUND choice for
        // Treasure needs treasuresCollected_/totalTreasures_ read here
        // since Treasure hasn't incremented yet at this point).
        switch (kind)
        {
            case GEInteractionSystem::VoyageKind::Treasure:
            {
                const bool completesSet = (treasuresCollected_ + 1 >= totalTreasures_);
                sound.Play(completesSet ? GalaxyEggbert::SoundChannel::SoundChannel19
                                        : GalaxyEggbert::SoundChannel::SoundChannel11);
                break;
            }
            case GEInteractionSystem::VoyageKind::Key1:
            case GEInteractionSystem::VoyageKind::Key2:
            case GEInteractionSystem::VoyageKind::Key3:
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
                break;
            case VoyageKind::Egg:
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel12);
                break;
            case VoyageKind::Perso:
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel60);
                break;
            case GEInteractionSystem::VoyageKind::Dynamite:
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel60);
                break;
            case VoyageKind::BulletPack:
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel54);
                break;
            case VoyageKind::Clear2Ascend:
            case VoyageKind::Clear3Ascend:
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel74);
                break;
            // LifeLoss: the ONLY Voyage kind whose effect fires at START, not
            // completion -- matches real VoyageInit's own inline `m_nbVies--`
            // (Decor.cpp:10172-10176). The caller has already captured
            // Lives() BEFORE this call for the start-position argument (real
            // `VoyageGetPosVie(m_nbVies)` uses the PRE-decrement value, since
            // it's evaluated as a call argument before VoyageInit's body
            // runs) -- LoseLife() here is what actually applies the
            // decrement (and the real reset-to-3 + game-over detection, via
            // its own existing logic, for the case the caller predicted
            // would NOT be game-over).
            case VoyageKind::LifeLoss:
                LoseLife();
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel9);
                break;
            // DoorUnlock: real source plays no immediate sound at all --
            // its dynamic icon never matches any of VoyageInit's
            // fixed-icon checks (confirmed via direct source read,
            // Decor.cpp:10141-10226).
            default:
                break;
        }
    }

    void GEInteractionSystem::TickVoyage(float dt, GEWorldRuntime& worldRuntime, GESound& sound)
    {
        if (voyageKind_ == VoyageKind::None)
        {
            return;
        }
        voyagePhase_ += dt * 20.0f; // same dt*20 real-20Hz-tick convention as AdvancePatrolStep
        // Real icon-cycle animation (`Decor::VoyageStep`'s own `if
        // (m_voyagePhase < m_voyageTotal) { if (m_time % ScaleTime(2) == 0
        // && icon in [230,241]) icon++ }`, Decor.cpp:10241-10252) -- uses
        // this voyage's own phase as the tick source (ScaleTime(2)==2 at
        // this build's reference rate), since this engine has no single
        // global `m_time` frame counter to match exactly.
        if (voyageKind_ == VoyageKind::Clear2Ascend && voyagePhase_ < voyageTotal_ &&
            static_cast<int>(voyagePhase_) % 2 == 0)
        {
            ++voyageIconId_;
            if (voyageIconId_ > 241)
            {
                voyageIconId_ = 230;
            }
        }
        if (voyageKind_ == VoyageKind::Clear3Ascend)
        {
            SpawnLavaAscendPuff(worldRuntime);
        }
        if (voyagePhase_ >= voyageTotal_)
        {
            ApplyVoyageReward(worldRuntime, sound);
        }
    }

    void GEInteractionSystem::SpawnLavaAscendPuff(GEWorldRuntime& worldRuntime)
    {
        // Real `Decor::VoyageDraw`'s icon==40 special case (Decor.cpp:
        // 10331-10348): every tick while the ascend is active, spawns a
        // tiny ObjectType93 puff scattered by a fixed 7-value horizontal
        // table plus a wider vertical random range -- both halved/
        // quadrupled respectively during the real 30-tick pre-move delay
        // (`num==0`, i.e. phase<=30 here), giving a denser puff right at
        // the death spot before the icon itself appears (VoyageIconVisible()
        // is false for that same window). Real X/Y offsets are added in
        // 2D screen space; ported here as a small world-space X/Y jitter
        // around Blupi's own death position (voyageWorldAnchor{X,Y,Z}_,
        // set by BeginVoyage()) -- this engine has no 2D decor-pixel space
        // to reproduce the real `pos.X -= 34; pos.X/Y += m_posDecor.X/Y`
        // conversion against.
        static constexpr float kHorizTable[7] = {-8.0f, -6.0f, -4.0f, 0.0f, 4.0f, 6.0f, 8.0f};
        std::uniform_int_distribution<int> horizIndexDist(0, 6);
        std::uniform_int_distribution<int> vertNoiseDist(-10, 10);
        float horizOffset = kHorizTable[horizIndexDist(rng_)];
        float vertOffset = static_cast<float>(vertNoiseDist(rng_));
        const bool preMove = (voyagePhase_ - 30.0f) <= 0.0f;
        if (preMove)
        {
            horizOffset *= 0.5f;
            vertOffset *= 4.0f;
        }

        MobileObjSpec spec;
        spec.type = ObjectType::ObjectType93;
        spec.active = true;
        spec.phase = 0.0f;
        spec.currentX = spec.posStartX = spec.posEndX = voyageWorldAnchorX_ + horizOffset / 64.0f;
        spec.currentY = spec.posStartY = spec.posEndY = voyageWorldAnchorY_ + vertOffset / 64.0f;
        spec.currentZ = spec.posStartZ = spec.posEndZ = voyageWorldAnchorZ_;

        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (auto& slot : objects)
        {
            if (!slot.active)
            {
                slot = spec;
                return;
            }
        }
        objects.push_back(spec);
    }

    bool GEInteractionSystem::RollClear2Coinflip()
    {
        std::uniform_int_distribution<int> coinDist(0, 1);
        return coinDist(rng_) == 1;
    }

    void GEInteractionSystem::ApplyVoyageReward(GEWorldRuntime& worldRuntime, GESound& sound)
    {
        // Real `VoyageStep()`'s completion branch (`Decor.cpp:10254-10306`)
        // -- applies the real reward, then plays the shared "reward
        // applied" sound (channel 3) for every kind EXCEPT DoorUnlock
        // (whose dynamic icon never matches any of VoyageStep's own
        // fixed-icon checks, so real completion is silent for it).
        constexpr int kMaxEggCount = 10; // real MAX_EGG_COUNT, same value as Update()'s own local constant
        switch (voyageKind_)
        {
            case GEInteractionSystem::VoyageKind::Treasure:
                ++treasuresCollected_;
                ScanAndOpenTreasureDoors(worldRuntime, treasuresCollected_, sound);
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case GEInteractionSystem::VoyageKind::Key1:
                ++keys1_;
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case GEInteractionSystem::VoyageKind::Key2:
                ++keys2_;
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case GEInteractionSystem::VoyageKind::Key3:
                ++keys3_;
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::Egg:
                if (lifeEggCount_ < kMaxEggCount)
                {
                    ++lifeEggCount_;
                    ++lives_;
                }
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case GEInteractionSystem::VoyageKind::Dynamite:
                ++dynamiteCount_;
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::Perso:
                ++persoCount_;
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::BulletPack:
                // Real reward already applied immediately at touch time
                // (matches this engine's own bulletCount_ handling) --
                // only the deferred completion sound fires here.
                sound.Play(GalaxyEggbert::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::DoorUnlock:
            // Clear2Ascend/Clear3Ascend: real completion is silent with no
            // reward -- neither icon 230 nor 40 appears in VoyageStep's
            // own completion if-chain (confirmed via direct source read).
            case VoyageKind::Clear2Ascend:
            case VoyageKind::Clear3Ascend:
            // LifeLoss: real completion (`Stop`/`m_blupiFocus=true`, unfreezing Blupi) is handled
            // by GEBlupiController's own independent life-loss-Voyage timer (see
            // TriggerDeathLock()'s own comment) -- the effect (LoseLife()) already fired at START
            // (BeginVoyage()'s own LifeLoss case), so completion here is a pure no-op.
            case VoyageKind::LifeLoss:
            case VoyageKind::None:
            default:
                break;
        }
        voyageKind_ = VoyageKind::None;
    }

    float GEInteractionSystem::VoyageDrawX() const noexcept
    {
        if (voyageKind_ == VoyageKind::None || voyageTotal_ <= 0.0f)
        {
            return voyageStartX_;
        }
        // Real `Decor::VoyageDraw`'s own icon==40 delay (Decor.cpp:
        // 10310-10316): position stays clamped to the start point for the
        // first 30 ticks -- only Clear3Ascend has this delay.
        const float adjustedPhase =
            (voyageKind_ == VoyageKind::Clear3Ascend) ? std::max(voyagePhase_ - 30.0f, 0.0f) : voyagePhase_;
        const float t = std::min(adjustedPhase / voyageTotal_, 1.0f);
        return voyageStartX_ + (voyageEndX_ - voyageStartX_) * t;
    }

    float GEInteractionSystem::VoyageDrawY() const noexcept
    {
        if (voyageKind_ == VoyageKind::None || voyageTotal_ <= 0.0f)
        {
            return voyageStartY_;
        }
        const float adjustedPhase =
            (voyageKind_ == VoyageKind::Clear3Ascend) ? std::max(voyagePhase_ - 30.0f, 0.0f) : voyagePhase_;
        const float t = std::min(adjustedPhase / voyageTotal_, 1.0f);
        return voyageStartY_ + (voyageEndY_ - voyageStartY_) * t;
    }

    bool GEInteractionSystem::VoyageIconVisible() const noexcept
    {
        if (voyageKind_ == VoyageKind::None)
        {
            return false;
        }
        if (voyageKind_ == VoyageKind::Clear3Ascend && (voyagePhase_ - 30.0f) <= 0.0f)
        {
            return false;
        }
        return true;
    }

    void GEInteractionSystem::CheatAllTreasure(GEWorldRuntime& worldRuntime, GESound& sound)
    {
        bool anyCollected = false;
        for (auto& obj : worldRuntime.GetMobileObjectsMutable())
        {
            if (obj.active && obj.type == ObjectType::ObjectType5)
            {
                obj.active = false;
                ++treasuresCollected_;
                anyCollected = true;
            }
        }
        if (anyCollected)
        {
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel11);
            ScanAndOpenTreasureDoors(worldRuntime, treasuresCollected_, sound);
        }
    }

    bool GEInteractionSystem::CheatFindExit(const GEWorldRuntime& worldRuntime, float& outX, float& outY,
                                             float& outZ) const
    {
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (obj.active && obj.type == ObjectType::ObjectType7)
            {
                outX = obj.currentX;
                outY = obj.currentY;
                outZ = obj.currentZ;
                return true;
            }
        }
        return false;
    }
}
