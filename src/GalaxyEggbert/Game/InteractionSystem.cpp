#include "InteractionSystem.hpp"

#include "CratePush.hpp"
#include "ObjectDefinitionRegistry.hpp"
#include "ObjectVerticalPlacement.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace GalaxyEggbert::Game
{
    namespace
    {
        // Real MAX_EGG_COUNT (Decor.cpp:96) -- was two separate local
        // constexpr constants (one `int`, one `float`, both correctly 10
        // today) at the egg-touch and voyage-completion gate sites, found
        // via a fresh code audit (2026-07-23): harmless while they agree,
        // but a future rebalance touching only one site would silently
        // desync the two independently-enforced gates. Hoisted to one
        // shared constant both sites now use.
        constexpr int kMaxEggCount = 10;

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
            GalaxyEggbert::Def::ObjectType type;
            // Which of Update()'s own caller-supplied blupiCanGrantX gates
            // applies -- an enum, not a function pointer, since every gate
            // is a plain bool parameter, not independent logic.
            enum class Gate : std::uint8_t { Shield, Power, Cloud, Hide, Invert } gate;
            // Sucette/Drink need the real action-button edge
            // (blupiActionPressedEdge, Decor.cpp:6025/6053); Shield/Charge/
            // Invert grant automatically on contact.
            bool requiresActionButton;
            InteractionSystem::EventKind grantedEvent;
            // Only Power/Cloud/Hide carry the pickup-position payload their
            // real 2-stage grab/freeze/complete delay needs (see
            // InteractionSystem::EventKind's own comment) -- Shield/Invert
            // grant instantly, no position needed.
            bool hasPositionPayload;
        };
        constexpr SecretPowerPickupHandler kSecretPowerPickupHandlers[] = {
            {GalaxyEggbert::Def::ObjectType::ObjectType25, SecretPowerPickupHandler::Gate::Shield, false,
             InteractionSystem::EventKind::ShieldGranted, false},
            {GalaxyEggbert::Def::ObjectType::ObjectType26, SecretPowerPickupHandler::Gate::Power, true,
             InteractionSystem::EventKind::PowerGranted, true},
            {GalaxyEggbert::Def::ObjectType::ObjectType30, SecretPowerPickupHandler::Gate::Hide, true,
             InteractionSystem::EventKind::HideGranted, true},
            {GalaxyEggbert::Def::ObjectType::ObjectType31, SecretPowerPickupHandler::Gate::Cloud, false,
             InteractionSystem::EventKind::CloudGranted, true},
            {GalaxyEggbert::Def::ObjectType::ObjectType40, SecretPowerPickupHandler::Gate::Invert, false,
             InteractionSystem::EventKind::InvertGranted, false},
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
            GalaxyEggbert::Def::ObjectType type;
            float expiryPhase;
            bool alwaysContinueEvenBeforeExpiry;
        };
        constexpr ExpiringParticleHandler kExpiringParticleHandlers[] = {
            // Always-continue family (never reaches AdvancePatrolStep()).
            {GalaxyEggbert::Def::ObjectType::ObjectType11, 9.0f, true},   // CAM-009-adjacent fan-hit shockwave flash
            {GalaxyEggbert::Def::ObjectType::ObjectType90, 12.0f, true},  // the crusher's flash (Decor.cpp:8443, BD-0054)
            {GalaxyEggbert::Def::ObjectType::ObjectType91, 6.0f, true},   // the balloon's end (Decor.cpp:8455, BD-0054)
            {GalaxyEggbert::Def::ObjectType::ObjectType53, 90.0f, true},  // the tentacle (Decor.cpp:8527, BD-0054)
            {GalaxyEggbert::Def::ObjectType::ObjectType37, 70.0f, true},  // the decoy trap's dissolve (Decor.cpp:8976, BD-0055)
            {GalaxyEggbert::Def::ObjectType::ObjectType38, 90.0f, false}, // the Cloud's kill, sliding down (Decor.cpp:8988, BD-0055)
            {GalaxyEggbert::Def::ObjectType::ObjectType14, 14.0f, true},  // PICKUP-078/080 water splash (Plouf)
            {GalaxyEggbert::Def::ObjectType::ObjectType35, 6.0f, true},   // PICKUP-078/080 water splash (Tiplouf)
            {GalaxyEggbert::Def::ObjectType::ObjectType8, 39.0f, true},   // VISUAL-008 dynamite-blast explosion flash
            {GalaxyEggbert::Def::ObjectType::ObjectType10, 20.0f, true},  // VISUAL-008 fish/bird explosion flash
            {GalaxyEggbert::Def::ObjectType::ObjectType93, 5.0f, true},   // Clear3Ascend Lava-death puff ("158" death-VFX)
            {GalaxyEggbert::Def::ObjectType::ObjectType9, 20.0f, true},   // VISUAL-008 follower-blocked-path debris flash
            // Falls-through-pre-expiry family (real slide types + static markers coded the same way).
            {GalaxyEggbert::Def::ObjectType::ObjectType41, 16.0f, false}, // VISUAL-014/015 Invert start burst (real slide)
            {GalaxyEggbert::Def::ObjectType::ObjectType42, 16.0f, false}, // VISUAL-014/015 Invert stop burst (real slide)
            {GalaxyEggbert::Def::ObjectType::ObjectType39, 11.0f, false}, // VISUAL-012 treasure/collectible sparkle burst (real slide)
            {GalaxyEggbert::Def::ObjectType::ObjectType36, 16.0f, false}, // VISUAL-013 Pollution puff (real slide)
            {GalaxyEggbert::Def::ObjectType::ObjectType57, 20.0f, false},  // VISUAL-011-adjacent Shield magic trail (static)
            {GalaxyEggbert::Def::ObjectType::ObjectType27, 24.0f, false},  // VISUAL-011-adjacent Power magic trail (static)
            {GalaxyEggbert::Def::ObjectType::ObjectType98, 10.0f, false},  // VISUAL-009 bullet-hit splat, small (static)
            {GalaxyEggbert::Def::ObjectType::ObjectType99, 13.0f, false},  // VISUAL-009 bullet-hit splat, medium (static)
            {GalaxyEggbert::Def::ObjectType::ObjectType100, 18.0f, false}, // VISUAL-009 bullet-hit splat, large (static)
            {GalaxyEggbert::Def::ObjectType::ObjectType92, 128.0f, false}, // VISUAL-010 teleporter arc (static)
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
        // voyage completion (plan.md `158`); Dynamite's gate (the action
        // button, none carried, none flying) is checked at its case before
        // the call. `TryCollectBasicPickup()` below replaces these 5
        // near-identical `case` bodies with one lookup + one dispatch.
        //
        // Deliberately excluded from this table (kept as their own case
        // bodies, not an oversight): Egg(6) computes its voyage endpoint X
        // from LIVE state (`lives_`) at grant time -- not expressible
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
            GalaxyEggbert::Def::ObjectType type;
            InteractionSystem::VoyageKind voyageKind;
            int voyageIcon;
            float endX;
            float endY;
            bool spawnsSparkleBurst;
        };
        constexpr BasicPickupHandler kBasicPickupHandlers[] = {
            {GalaxyEggbert::Def::ObjectType::ObjectType5, InteractionSystem::VoyageKind::Treasure, 6, 430.0f, 430.0f, true},    // treasure, Decor.cpp:5956
            {GalaxyEggbert::Def::ObjectType::ObjectType49, InteractionSystem::VoyageKind::Key1, 215, 520.0f, 418.0f, true},     // key 1, Decor.cpp:5971
            {GalaxyEggbert::Def::ObjectType::ObjectType50, InteractionSystem::VoyageKind::Key2, 222, 530.0f, 418.0f, true},     // key 2, Decor.cpp:5986
            {GalaxyEggbert::Def::ObjectType::ObjectType51, InteractionSystem::VoyageKind::Key3, 229, 540.0f, 418.0f, true},     // key 3, Decor.cpp:6001
            {GalaxyEggbert::Def::ObjectType::ObjectType55, InteractionSystem::VoyageKind::Dynamite, 252, 505.0f, 414.0f, false}, // dynamite, Decor.cpp:6123-6125
        };

        bool IsCrate(const MobileObjSpec& obj)
        {
            return obj.active && GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::StaticObstacle;
        }

        // Crates are boxes where they are (a unit cube centred on currentX/Z, from currentY up, at any
        // position along X and Z, as the original's crates rest at any pixel: OR-0022/0023).
        constexpr float kCrateEps = 1e-3f;

        // A crate's box covers part of raw cell (gx, gy, gz)?
        bool CrateAtCell(const std::vector<MobileObjSpec>& objects, int gx, int gy, int gz,
                         const std::vector<MobileObjSpec*>& except)
        {
            for (const auto& other : objects)
            {
                if (!IsCrate(other) || std::find(except.begin(), except.end(), &other) != except.end())
                {
                    continue;
                }
                const float rawX = other.currentX + static_cast<float>(WorldRuntime::kWorldCenterX);
                const float rawZ = other.currentZ + static_cast<float>(WorldRuntime::kWorldCenterZ);
                if (static_cast<int>(std::lround(other.currentY)) == gy &&
                    std::fabs(rawX - static_cast<float>(gx)) < 1.0f - kCrateEps &&
                    std::fabs(rawZ - static_cast<float>(gz)) < 1.0f - kCrateEps)
                {
                    return true;
                }
            }
            return false;
        }

        // Whether a crate's box (not one of `except`) holds Blupi's collision point at relative (x, z)
        // between his feet and his head standing at y: where he cannot stand (BlupiController's
        // IsCrateAt, the same test).
        bool CrateBoxHoldsBlupi(const std::vector<MobileObjSpec>& objects, float x, float y, float z,
                                const std::vector<MobileObjSpec*>& except)
        {
            constexpr float kFeet = 0.05f;
            constexpr float kHead = 49.0f / 64.0f; // BlupiController::kHeadHeight
            for (const auto& other : objects)
            {
                if (IsCrate(other) && std::find(except.begin(), except.end(), &other) == except.end() &&
                    std::fabs(other.currentX - x) < 0.5f && std::fabs(other.currentZ - z) < 0.5f &&
                    other.currentY < y + kHead && other.currentY + 1.0f > y + kFeet)
                {
                    return true;
                }
            }
            return false;
        }

        // `seed` and every crate linked to it (Decor::SearchLinkCaisse): crates whose boxes touch (1 px)
        // an already linked one, at or above the seed's row; for a pull only those within 32 px of
        // the seed along the axis (the stack on it). In 3D they must also share the push's lane
        // (overlap across it): a crate beside the lane is not dragged along.
        std::vector<MobileObjSpec*> CrateGroup(std::vector<MobileObjSpec>& objects, MobileObjSpec& seed, int axis,
                                               bool pulled)
        {
            const auto along = [axis](const MobileObjSpec& o) { return axis == 0 ? o.currentX : o.currentZ; };
            const auto across = [axis](const MobileObjSpec& o) { return axis == 0 ? o.currentZ : o.currentX; };
            constexpr float kTouch = 1.0f + 1.0f / 64.0f;
            std::vector<MobileObjSpec*> linked{&seed};
            for (bool added = true; added;)
            {
                added = false;
                for (auto& other : objects)
                {
                    if (!IsCrate(other) || std::find(linked.begin(), linked.end(), &other) != linked.end() ||
                        other.currentY < seed.currentY - 0.5f ||
                        std::fabs(across(other) - across(seed)) >= 1.0f - kCrateEps ||
                        (pulled && std::fabs(along(other) - along(seed)) > 0.5f))
                    {
                        continue;
                    }
                    for (auto* member : linked)
                    {
                        if (std::fabs(along(other) - along(*member)) <= kTouch &&
                            std::fabs(across(other) - across(*member)) <= kTouch &&
                            std::fabs(other.currentY - member->currentY) <= kTouch)
                        {
                            linked.push_back(&other);
                            added = true;
                            break;
                        }
                    }
                }
            }
            return linked;
        }

        // Solid for a crate (DecorDetect): a block with a collision mask, or outside the world.
        bool CrateBlockSolid(const Worlds::World& world, int gx, int gy, int gz)
        {
            const int n = static_cast<int>(world.blocksPerAxis());
            if (gx < 0 || gy < 0 || gz < 0 || gx >= n || gy >= n || gz >= n)
            {
                return true;
            }
            const auto block =
                world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy), static_cast<std::uint16_t>(gz));
            return !block.isAir() && GetBlockDefinition(block.type()).collisionMask != 0;
        }

        // The raw cells a span [lo, hi] (raw, centre-anchored cells) overlaps.
        void CellSpan(float lo, float hi, int& first, int& last)
        {
            first = static_cast<int>(std::floor(lo + 0.5f + kCrateEps));
            last = static_cast<int>(std::floor(hi + 0.5f - kCrateEps));
        }

        // Whether a crate box centred at raw (rx, rz), row y, overlaps a solid block.
        bool CrateBoxHitsBlock(const Worlds::World& world, float rx, int y, float rz)
        {
            int x0, x1, z0, z1;
            CellSpan(rx - 0.5f, rx + 0.5f, x0, x1);
            CellSpan(rz - 0.5f, rz + 0.5f, z0, z1);
            for (int gx = x0; gx <= x1; ++gx)
                for (int gz = z0; gz <= z1; ++gz)
                    if (CrateBlockSolid(world, gx, y, gz))
                        return true;
            return false;
        }

        // Support under a crate box at raw (rx, rz), row y (Decor::TestPushOneCaisse): each of its
        // bottom's corner patches, 20 px square, stands on a solid block or on a crate below. A crate
        // may so overhang an edge by up to 20 px, as the original's.
        bool CrateSupported(const Worlds::World& world, const std::vector<MobileObjSpec>& objects, float rx, int y,
                            float rz, const std::vector<MobileObjSpec*>& group)
        {
            constexpr float kPatch = 20.0f / 64.0f;
            for (const float sx : {-1.0f, 1.0f})
            {
                for (const float sz : {-1.0f, 1.0f})
                {
                    const float px0 = sx < 0.0f ? rx - 0.5f : rx + 0.5f - kPatch;
                    const float pz0 = sz < 0.0f ? rz - 0.5f : rz + 0.5f - kPatch;
                    bool held = false;
                    int x0, x1, z0, z1;
                    CellSpan(px0, px0 + kPatch, x0, x1);
                    CellSpan(pz0, pz0 + kPatch, z0, z1);
                    for (int gx = x0; gx <= x1 && !held; ++gx)
                        for (int gz = z0; gz <= z1 && !held; ++gz)
                            held = CrateBlockSolid(world, gx, y - 1, gz);
                    for (const auto& other : objects)
                    {
                        if (held)
                            break;
                        if (!IsCrate(other) || std::find(group.begin(), group.end(), &other) != group.end() ||
                            static_cast<int>(std::lround(other.currentY)) != y - 1)
                            continue;
                        const float ox = other.currentX + static_cast<float>(WorldRuntime::kWorldCenterX);
                        const float oz = other.currentZ + static_cast<float>(WorldRuntime::kWorldCenterZ);
                        held = px0 + kPatch > ox - 0.5f + kCrateEps && px0 < ox + 0.5f - kCrateEps &&
                               pz0 + kPatch > oz - 0.5f + kCrateEps && pz0 < oz + 0.5f - kCrateEps;
                    }
                    if (!held)
                        return false;
                }
            }
            return true;
        }

        // Whether every crate of `group` can move by `delta` along `axis` (0: X, 2: Z): its box free
        // of blocks and of the other crates, and the seed row's crates supported where they arrive
        // (Decor::TestPushCaisse: all or nothing).
        bool CrateGroupCanMove(const WorldRuntime& worldRuntime, const std::vector<MobileObjSpec>& objects,
                               const std::vector<MobileObjSpec*>& group, int axis, float delta)
        {
            const auto& world = worldRuntime.GetWorld();
            const float seedY = group.front()->currentY;
            for (const auto* member : group)
            {
                const float rx = member->currentX + static_cast<float>(WorldRuntime::kWorldCenterX) + (axis == 0 ? delta : 0.0f);
                const float rz = member->currentZ + static_cast<float>(WorldRuntime::kWorldCenterZ) + (axis == 2 ? delta : 0.0f);
                const int y = static_cast<int>(std::lround(member->currentY));
                if (y < 1 || CrateBoxHitsBlock(world, rx, y, rz))
                {
                    return false;
                }
                // The solid BigDecor stop a crate as blocks do (Q-014): its cube against their boxes.
                for (const auto& box : worldRuntime.SolidBigDecorBoxes())
                {
                    if (rx + 0.5f - kCrateEps > box.minX && rx - 0.5f + kCrateEps < box.maxX &&
                        rz + 0.5f - kCrateEps > box.minZ && rz - 0.5f + kCrateEps < box.maxZ &&
                        static_cast<float>(y) + 1.0f > box.minY && static_cast<float>(y) < box.maxY)
                    {
                        return false;
                    }
                }
                for (const auto& other : objects)
                {
                    if (!IsCrate(other) || std::find(group.begin(), group.end(), &other) != group.end() ||
                        static_cast<int>(std::lround(other.currentY)) != y)
                    {
                        continue;
                    }
                    const float ox = other.currentX + static_cast<float>(WorldRuntime::kWorldCenterX);
                    const float oz = other.currentZ + static_cast<float>(WorldRuntime::kWorldCenterZ);
                    if (std::fabs(ox - rx) < 1.0f - kCrateEps && std::fabs(oz - rz) < 1.0f - kCrateEps)
                    {
                        return false;
                    }
                }
                if (std::fabs(member->currentY - seedY) < 0.5f && !CrateSupported(world, objects, rx, y, rz, group))
                {
                    return false;
                }
            }
            return true;
        }

        // Opens a door tile (plan.md E3D-MIG-160/162, real `Decor::OpenDoor`,
        // ~11667): removes the tile from the terrain grid (becomes passable)
        // and spawns a transient ObjectType22 at that cell (real
        // `table_bridge`-style slide, handled by this file's own
        // ObjectType22 branch in the main Update() loop, not the generic
        // patrol system) so it visually slides up and out of the way
        // instead of just vanishing. Real channel 33. Shared by both the
        // key-gated (160) and treasure-gated (162) door families --
        // opening itself is identical, only the trigger condition differs.
        // The slide shows the door's own icon on the object channel, as
        // the real OpenDoor sets `channel = Object; icon = m_decor[..].icon`
        // (a block type is its object-m.png icon, so it goes in visualIcon).
        void OpenDoorAt(WorldRuntime& worldRuntime, int gx, int gy, int gz, Sound& sound)
        {
            const std::uint16_t doorIcon = worldRuntime.GetWorld()
                                               .getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                         static_cast<std::uint16_t>(gz))
                                               .type();
            worldRuntime.GetWorldMutable().setBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                      static_cast<std::uint16_t>(gz),
                                                      Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));

            MobileObjSpec slide;
            slide.type = GalaxyEggbert::Def::ObjectType::ObjectType22;
            slide.posStartX = slide.posEndX = slide.currentX =
                static_cast<float>(gx) - static_cast<float>(WorldRuntime::kWorldCenterX);
            slide.posStartY = slide.posEndY = slide.currentY = static_cast<float>(gy);
            slide.posStartZ = slide.posEndZ = slide.currentZ =
                static_cast<float>(gz) - static_cast<float>(WorldRuntime::kWorldCenterZ);
            slide.phase = 0.0f;
            slide.visualIcon = doorIcon;
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
            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel33);
        }

        // Real Decor::OpenDoorsTresor (~11642): scans the whole grid,
        // opening every treasure-gated door (icon 421+N) whose
        // requirement N is now met. Shared by the real per-pickup trigger
        // (called from ApplyVoyageReward() the instant a treasure voyage
        // completes, plan.md `158`) and
        // InteractionSystem::CheatAllTreasure() below, which needs the
        // exact same scan after crediting every treasure at once.
        void ScanAndOpenTreasureDoors(WorldRuntime& worldRuntime, int treasuresCollected, Sound& sound)
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

        // INFRA-006 7th family (plan.md §7): per-type terminal behavior at
        // the shared AdvancePatrolStep() arrival junction. These are the
        // only three ObjectTypes that override the normal "start dwelling at
        // posEnd" behavior after an advance. Keeping the action as table
        // data makes adding or auditing such an exception local to this
        // list, rather than extending the generic patrol state machine with
        // another open-coded type comparison.
        enum class PatrolArrivalAction : std::uint8_t
        {
            None,
            Deactivate,
            StickAtDestination,
        };

        struct PatrolArrivalHandler
        {
            GalaxyEggbert::Def::ObjectType type;
            PatrolArrivalAction action;
        };

        constexpr PatrolArrivalHandler kPatrolArrivalHandlers[] = {
            // Fired projectile / rising water bubble: ObjectDelete as soon
            // as the precomputed clear path ends (Decor.cpp:8095-8098).
            {GalaxyEggbert::Def::ObjectType::ObjectType23, PatrolArrivalAction::Deactivate},
            {GalaxyEggbert::Def::ObjectType::ObjectType15, PatrolArrivalAction::Deactivate},
            // Goo: stop at the landing point instead of ever receding
            // (Decor.cpp:8099-8104).
            {GalaxyEggbert::Def::ObjectType::ObjectType34, PatrolArrivalAction::StickAtDestination},
        };

        PatrolArrivalAction GetPatrolArrivalAction(GalaxyEggbert::Def::ObjectType type)
        {
            for (const auto& handler : kPatrolArrivalHandlers)
            {
                if (handler.type == type)
                {
                    return handler.action;
                }
            }
            return PatrolArrivalAction::None;
        }

        // Each phase lasts one tick longer than its count (BD-0010 follow-up): the original advances
        // a phase's time while it is short of its end (time < timeStopStart; posCurrent != posEnd)
        // and only on the next tick sees it done and switches (Decor.cpp:8065-8140), so a dwell of N
        // ticks holds N + 1, a move of N ticks moves for N and rests a tick at its end, and the
        // cycle is the four counts plus four.
        constexpr float kPatrolSwitchTick = 1.0f;

        void AdvancePatrolStep(MobileObjSpec& obj, float dt)
        {
            const float dtTicks = dt * kTicksPerSecond;
            switch (obj.patrolStep)
            {
                case 1: // dwell at posStart
                    obj.patrolTime += dtTicks;
                    if (obj.patrolTime >= obj.timeStopStartTicks + kPatrolSwitchTick)
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
                    if (t >= 1.0f && obj.patrolTime >= std::max(obj.stepAdvanceTicks, 0.0f) + kPatrolSwitchTick)
                    {
                        switch (GetPatrolArrivalAction(obj.type))
                        {
                            case PatrolArrivalAction::Deactivate:
                                obj.active = false;
                                break;
                            case PatrolArrivalAction::StickAtDestination:
                                obj.posStartX = obj.posEndX = obj.currentX;
                                obj.posStartY = obj.posEndY = obj.currentY;
                                obj.posStartZ = obj.posEndZ = obj.currentZ;
                                [[fallthrough]];
                            case PatrolArrivalAction::None:
                                obj.patrolStep = 3;
                                obj.patrolTime = 0.0f;
                                break;
                        }
                    }
                    break;
                }
                case 3: // dwell at posEnd
                    obj.patrolTime += dtTicks;
                    if (obj.patrolTime >= obj.timeStopEndTicks + kPatrolSwitchTick)
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
                    if (t >= 1.0f && obj.patrolTime >= std::max(obj.stepRecedeTicks, 0.0f) + kPatrolSwitchTick)
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
        // conversion convention (see WorldRuntime's kMobileTileSize use).
        //
        // A cell stops it only as `Decor::IsBlocIcon` does (Decor.cpp:7522, used by SearchDistRight
        // :7628): every quarter-cell of its collision mask solid, or the vanishing saucer (324)
        // while it is there (`tempSolid`); empty-mask decor and part-blocks let a shot through
        // (BD-0004).
        int SearchAirDistance(const Worlds::World& world, int gx, int gy, int gz, int dx, int dy, int dz,
                              bool tempSolid = true)
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
                const auto block = world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                  static_cast<std::uint16_t>(z));
                if (!block.isAir() && (GetBlockDefinition(block.type()).collisionMask == 0xFFFF ||
                                       (block.type() == GalaxyEggbert::BlockTypes::Temp && tempSolid)))
                {
                    break;
                }
                ++dist;
            }
            return dist;
        }

        // Converts a live object's world-space position to a clamped grid
        // cell, matching BlupiController's own kWorldCenterX/Z + lround
        // convention exactly (duplicated here rather than exposed from
        // BlupiController, which keeps its own version private).
        void ToGridCell(const Worlds::World& world, float wx, float wy, float wz, int& gx, int& gy, int& gz)
        {
            const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
            gx = std::clamp(static_cast<int>(std::lround(wx)) + WorldRuntime::kWorldCenterX, 0, blocksPerAxis - 1);
            gy = std::clamp(static_cast<int>(std::lround(wy)), 0, blocksPerAxis - 1);
            gz = std::clamp(static_cast<int>(std::lround(wz)) + WorldRuntime::kWorldCenterZ, 0, blocksPerAxis - 1);
        }

        // What stops a moving object's step (`Decor::TestPath` -> `DecorDetect`): a block with any
        // solid quarter-cell in its collision mask, or a crate (DecorDetect counts crates). Decor
        // with an empty mask (cobwebs, signs, pillars as decor) and water do not (BD-0004; it used
        // to be any non-air cell).
        bool IsSolidAt(const WorldRuntime& worldRuntime, int gx, int gy, int gz)
        {
            const auto block = worldRuntime.GetWorld().getBlock(
                static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy), static_cast<std::uint16_t>(gz));
            if (!block.isAir() && GetBlockDefinition(block.type()).collisionMask != 0)
            {
                return true;
            }
            return CrateAtCell(worldRuntime.GetMobileObjects(), gx, gy, gz, {});
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
            bullet.type = GalaxyEggbert::Def::ObjectType::ObjectType23;
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
        // (`Decor.cpp:7628-7653`, flat 500 real-px for this GalaxyEggbert::Def::ObjectType
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
                spec.type = GalaxyEggbert::Def::ObjectType::ObjectType39;
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
        // (`Decor.cpp:5797-5816`, already this engine's own registry-backed
        // genericContactHazard block, plan.md CAM-008/009 -- ObjectType10
        // for fish/17 and bird/20 specifically, paired with the
        // already-wired BigShake; ObjectType8 for every other hazard
        // type, paired with SmallShake). Appends directly to pendingSpawns
        // since both real spawn sites are INSIDE this class's own
        // per-object loop.
        void AppendExplosionFlash(GalaxyEggbert::Def::ObjectType type, float x, float y, float z, std::vector<MobileObjSpec>& pendingSpawns)
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
        // -- despite Def/ObjectType.hpp's own "water splash" doc comments,
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
            struct Splat { GalaxyEggbert::Def::ObjectType type; float dx, dy; };
            static constexpr float k = 64.0f; // real px-per-tile scale
            static const Splat splats[7] = {
                {GalaxyEggbert::Def::ObjectType::ObjectType98, 0.0f / k, 0.0f / k},
                {GalaxyEggbert::Def::ObjectType::ObjectType99, 15.0f / k, -20.0f / k},
                {GalaxyEggbert::Def::ObjectType::ObjectType99, -20.0f / k, -18.0f / k},
                {GalaxyEggbert::Def::ObjectType::ObjectType99, 23.0f / k, 18.0f / k},
                {GalaxyEggbert::Def::ObjectType::ObjectType99, -15.0f / k, 18.0f / k},
                {GalaxyEggbert::Def::ObjectType::ObjectType100, 32.0f / k, -10.0f / k},
                {GalaxyEggbert::Def::ObjectType::ObjectType100, -28.0f / k, -15.0f / k},
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
        void FireBlupihShot(const MobileObjSpec& obj, const Worlds::World& world, Sound& sound,
                             std::vector<MobileObjSpec>& pendingSpawns)
        {
            int gx, gy, gz;
            ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, gx, gy, gz);
            const int dist = SearchAirDistance(world, gx, gy, gz, 0, -1, 0);
            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel52);
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
        // A creature's shot, along its patrol's axis (X or Z, whichever it walks along: in 3D a
        // creature can patrol along Z) toward `dirSign`.
        void FireBlupitShot(const MobileObjSpec& obj, float dirSign, bool alongZ, const Worlds::World& world,
                             Sound& sound, std::vector<MobileObjSpec>& pendingSpawns)
        {
            int gx, gy, gz;
            ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, gx, gy, gz);
            const int sign = (dirSign > 0.0f) ? 1 : -1;
            const int dist = SearchAirDistance(world, gx, gy, gz, alongZ ? 0 : sign, 0, alongZ ? sign : 0);
            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel52);
            if (dist > 0)
            {
                pendingSpawns.push_back(MakeBullet(obj, alongZ ? 0.0f : dirSign, 0.0f, alongZ ? dirSign : 0.0f, dist));
            }
        }

        // Contact as Decor::MoveObjectDetect has it (Decor.cpp, "src"/"src3" boxes; it replaced a
        // 0.7-cell sphere, BD-0021): Blupi 28 px across (pos+16..+44) and from 11 to 58 px below
        // the top of his cell; an object 28 px across and, by type, the bottom 24 px of its cell
        // (most), the top 24 (the hanging mine, 3), the middle 28 (fish 17, bird 20, wasp 44, big
        // creature 54), or a bullet (23) 16 px across from 10 to 28. Across is X and Z alike in
        // 3D. dx/dy/dz: object minus Blupi, in cells, y up; both stand on their cells' floors.
        // `widen`/`above`/`below` grow Blupi's box (the still mine's near-miss box: 20 px each
        // side, 40 above, 30 below).
        bool MoveObjectTouches(GalaxyEggbert::Def::ObjectType type, float dx, float dy, float dz, float widen = 0.0f,
                               float above = 0.0f, float below = 0.0f)
        {
            using T = GalaxyEggbert::Def::ObjectType;
            float objAcross = 28.0f, objTop = 36.0f, objBottom = 60.0f;
            if (type == T::ObjectType3)
            {
                objTop = 0.0f;
                objBottom = 24.0f;
            }
            else if (type == T::ObjectType17 || type == T::ObjectType20 || type == T::ObjectType44 ||
                     type == T::ObjectType54)
            {
                objTop = 16.0f;
                objBottom = 44.0f;
            }
            else if (type == T::ObjectType23)
            {
                objAcross = 16.0f;
                objTop = 10.0f;
                objBottom = 28.0f;
            }
            const float reach = (28.0f + 2.0f * widen + objAcross) * 0.5f / 64.0f;
            if (std::fabs(dx) >= reach || std::fabs(dz) >= reach)
            {
                return false;
            }
            const float blupiTop = 11.0f - above, blupiBottom = 58.0f + below;
            const float dyScreen = -dy * 64.0f; // screen y grows downward
            return objTop + dyScreen < blupiBottom && objBottom + dyScreen > blupiTop;
        }

        // Real `BlupiElectro` aura (plan.md `068`, Decor.cpp:9610-9638,
        // mobile-eggbert-reference/10-blupi-mechanics.md §9): while
        // `m_blupiCloud` (this engine's GalaxyEggbert::Def::SecretPower::Cloud) is active,
        // instantly destroys small enemies within 40px of Blupi's own
        // box -- an offensive aura Blupi carries, unrelated to the
        // `Blitz` lightning HAZARD despite the similarly-named real
        // function. Real check expands Blupi's own box by 40px; modeled
        // as a circular radius (same simplification as every other
        // proximity test in this file) combining that 40px real
        // expansion with a 32px half-tile (Blupi's own box is measured
        // from its center here, not its edge) -- the "half-tile + real
        // px offset, /64" combination.
        // The Cloud's arc (Decor::BlupiElectro, Decor.cpp:9614-9634): the enemy's box (28 px wide)
        // meets Blupi's inflated by 16 + 40 px: centres within 100 px across, 87 px up or down. In
        // 3D the reach across holds on X and on Z.
        constexpr float kCloudAuraAcross = 100.0f / 64.0f;
        constexpr float kCloudAuraUpDown = 87.0f / 64.0f;

        // Follower wake (ObjectType96, plan.md E3D-MIG-137, Decor.cpp:9646-9678 MoveObjectFollow):
        // the original's box -- the follower's cell padded by 100 px on every side against
        // Blupi's 16-44 px strip, 11-58 px under his top: 144 px either way across (on X and on Z
        // alike in 3D), from 149 px with the star over him to 158 under (BD-0053; it replaced a
        // sphere of 132 px, which left stars two cells off on a diagonal or two over him asleep).
        constexpr float kFollowerWakeAcross = 144.0f / 64.0f;
        constexpr float kFollowerWakeOver = 149.0f / 64.0f;
        constexpr float kFollowerWakeUnder = 158.0f / 64.0f;

        // Real follower homing speed (ObjectType97, Decor.cpp:8025-8044):
        // exactly 1 real px/tick, independently per axis (a Chebyshev-
        // style step-toward, NOT a normalized diagonal), at the same 20Hz
        // reference tick rate MoveObjectStepLine itself runs at (matching
        // AdvancePatrolStep's own convention) -- a direct, non-
        // approximated transcription via the same 64px/cell grid
        // conversion used throughout this file.
        constexpr float kFollowerHomingSpeed = 20.0f / 64.0f;
    }

    const MobileObjSpec* InteractionSystem::CrateInFront(const WorldRuntime& worldRuntime, float blupiX, float blupiY,
                                                         float blupiZ, float facingX, float facingZ,
                                                         float maxAhead) noexcept
    {
        const bool alongX = std::fabs(facingX) > std::fabs(facingZ);
        if (std::fabs(std::fabs(facingX) - std::fabs(facingZ)) < 0.2f)
        {
            return nullptr; // facing diagonally: no crate is in front
        }
        const float sign = alongX ? (facingX > 0.0f ? 1.0f : -1.0f) : (facingZ > 0.0f ? 1.0f : -1.0f);
        // The nearest one: of a row of crates he holds the one touching him (D50).
        const MobileObjSpec* nearest = nullptr;
        float nearestAhead = 0.0f;
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (!IsCrate(obj) || std::fabs(obj.currentY - blupiY) >= 0.5f)
            {
                continue;
            }
            const float ahead = (alongX ? obj.currentX - blupiX : obj.currentZ - blupiZ) * sign;
            const float aside = alongX ? obj.currentZ - blupiZ : obj.currentX - blupiX;
            if (ahead > 0.3f && ahead < maxAhead && std::fabs(aside) < 0.6f && (!nearest || ahead < nearestAhead))
            {
                nearest = &obj;
                nearestAhead = ahead;
            }
        }
        return nearest;
    }

    InteractionSystem::CratePushOutput InteractionSystem::UpdateCratePush(float dt, WorldRuntime& worldRuntime,
                                                                          const CratePushInput& in)
    {
        // As the original (Decor.cpp:6130-6248, 3386-3470, 9321-9507): every tick of Push or Pop Blupi
        // moves CaisseGetMove pixels and the crate group is put against him if all of it fits there
        // (blocks, other crates, the corner support); if not, nothing moves. Crates rest wherever
        // they stop; letting go stops them at once (OR-0021/0022/0023 overturned the whole-cell moves
        // of M3D-0001).
        using namespace CratePush;
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        const auto alongOf = [](int axis, const MobileObjSpec& o) { return axis == 0 ? o.currentX : o.currentZ; };
        const auto acrossOf = [](int axis, const MobileObjSpec& o) { return axis == 0 ? o.currentZ : o.currentX; };
        const bool faceAlongX = std::fabs(in.facingX) > std::fabs(in.facingZ);
        const int axis = faceAlongX ? 0 : 2;
        const float side = faceAlongX ? (in.facingX > 0.0f ? 1.0f : -1.0f) : (in.facingZ > 0.0f ? 1.0f : -1.0f);
        const bool wantPush = in.canPush && in.onGround && in.walkingForward && !in.holding;
        const bool wantPull = in.canPush && in.onGround && in.holding && in.pullingBack;

        MobileObjSpec* seed = nullptr;
        if (wantPush || in.holding)
        {
            seed = const_cast<MobileObjSpec*>(CrateInFront(worldRuntime, in.blupiX, in.blupiY, in.blupiZ, in.facingX,
                                                           in.facingZ, in.holding ? 1.1f : kEngageGap));
            // A push needs him walking into the crate's face, not grazing its corner.
            if (seed && wantPush && std::fabs(acrossOf(axis, *seed) - (axis == 0 ? in.blupiZ : in.blupiX)) >= 0.5f)
            {
                seed = nullptr;
            }
        }
        const int action = seed == nullptr ? 0 : (wantPush ? 1 : (wantPull ? 2 : 0));
        if (action != crateAction_)
        {
            crateActionSeconds_ = 0.0f;
            crateAction_ = action;
        }

        bool moved = false;
        if (action != 0)
        {
            const bool pulled = action == 2;
            std::vector<MobileObjSpec*> group = CrateGroup(objects, *seed, axis, pulled);
            const float speed = CellsPerSecond(pulled ? kPullMaxPixels : kPushMaxPixels, static_cast<int>(group.size()),
                                               in.power, crateActionSeconds_);
            const float delta = (pulled ? -side : side) * speed * dt;
            bool blupiFree = true;
            if (pulled)
            {
                // He backs away with it: room for him where it puts him (blocks, other crates).
                const float blupiAlong = alongOf(axis, *seed) + delta - side * kContactGap;
                const float toX = axis == 0 ? blupiAlong : in.blupiX;
                const float toZ = axis == 2 ? blupiAlong : in.blupiZ;
                blupiFree = (!in.blupiClearAt || in.blupiClearAt(toX, in.blupiY, toZ)) &&
                            !CrateBoxHoldsBlupi(objects, toX, in.blupiY, toZ, group);
            }
            if (blupiFree && CrateGroupCanMove(worldRuntime, objects, group, axis, delta))
            {
                for (auto* member : group)
                {
                    (axis == 0 ? member->currentX : member->currentZ) += delta;
                }
                moved = true;
            }
            else if (blupiFree)
            {
                // Blocked: off its lane by a sliver, it slides into the lane first (M3D-0002).
                const float across = acrossOf(axis, *seed);
                const float off = across - std::round(across);
                const float slide = -std::copysign(std::min(std::fabs(off), std::fabs(delta)), off);
                const int acrossAxis = axis == 0 ? 2 : 0;
                if (std::fabs(off) > 1e-5f && std::fabs(off) <= kLaneSlide &&
                    CrateGroupCanMove(worldRuntime, objects, group, acrossAxis, slide))
                {
                    for (auto* member : group)
                    {
                        (acrossAxis == 0 ? member->currentX : member->currentZ) += slide;
                    }
                    moved = true;
                }
            }
        }
        crateActionSeconds_ += dt;
        if (action == 2 && !moved)
        {
            // A pull that cannot go is the original's Stop (Decor.cpp:6239-6243): the next pull
            // starts over from rest, its CaisseGetMove ramp again from phase 0.
            crateAction_ = 0;
            crateActionSeconds_ = 0.0f;
        }

        CratePushOutput out;
        if (seed && (action != 0 || in.holding))
        {
            const float targetAlong = alongOf(axis, *seed) - side * kContactGap;
            const float targetAcross = acrossOf(axis, *seed);
            const float blupiAlong = axis == 0 ? in.blupiX : in.blupiZ;
            const float blupiAcross = axis == 0 ? in.blupiZ : in.blupiX;
            // Held only while he stands at the crate (not thrown off it, killed, jumping).
            if (in.onGround && std::fabs(blupiAlong - targetAlong) < 0.6f &&
                std::fabs(blupiAcross - targetAcross) < 0.6f && std::fabs(in.blupiY - seed->currentY) < 0.5f)
            {
                out.holdBlupi = true;
                out.blupiX = axis == 0 ? targetAlong : targetAcross;
                out.blupiZ = axis == 2 ? targetAlong : targetAcross;
                out.faceX = axis == 0 ? side : 0.0f;
                out.faceZ = axis == 2 ? side : 0.0f;
            }
        }
        // Push stays on against a crate that cannot go (the original's Push while he marches into
        // it); Pop only while the crate follows him (a blocked pull is the original's Stop).
        out.pushing = action == 1 && out.holdBlupi;
        out.pulling = action == 2 && moved && out.holdBlupi;
        crateBeingPushedThisFrame_ = out.pushing;
        cratePush_ = out;
        return out;
    }

    void InteractionSystem::OpenDoor(WorldRuntime& worldRuntime, int gx, int gy, int gz, Sound& sound)
    {
        OpenDoorAt(worldRuntime, gx, gy, gz, sound);
    }

    void InteractionSystem::Update(float dt, WorldRuntime& worldRuntime,
                                      float blupiX, float blupiY, float blupiZ, float blupiMoveDX,
                                      Sound& sound, bool blupiCrouching, bool blupiBallooned,
                                      int blupiFacingDX, int blupiFacingDZ, bool blupiInvincible,
                                      bool blupiCanGrantShield, bool blupiCanGrantPower,
                                      bool blupiCanGrantCloud, bool blupiCanGrantHide,
                                      bool blupiFirePressed, bool blupiCanFire, bool blupiCloudActive,
                                      bool blupiCanGrantInvert, bool blupiActionPressedEdge,
                                      bool blupiCanPushCrate, float blupiMoveDZ, bool blupiHoldingCrate,
                                      bool blupiArmoredVehicle, bool blupiDropsGlue, bool blupiHidden,
                                      bool blupiFocused, bool blupiOutOfPlay)
    {
        events_.clear();
        ridingLift_ = false;
        TickByeBye(dt);
        // The contact grace ends 30 px from where it began (Decor.cpp:6341-6345), out of the
        // contact box (MoveObjectTouches) by then.
        if (contactGrace_)
        {
            constexpr float kGraceReach = 30.0f / 64.0f;
            if (std::fabs(blupiX - graceX_) > kGraceReach || std::fabs(blupiY - graceY_) > kGraceReach ||
                std::fabs(blupiZ - graceZ_) > kGraceReach)
            {
                contactGrace_ = false;
            }
        }
        // The action press a dynamite stick's pickup spends (the original clears the button,
        // Decor.cpp:6119), so one press takes one stick.
        bool actionUnspent = blupiActionPressedEdge;
        actionSpent_ = false;
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
            if (!candidate.active ||
                GetObjectDefinition(candidate.type).semanticKind != ObjectSemanticKind::Lift)
            {
                continue;
            }
            // Over its cube or with a foot still on its edge (kFootReach, the vertigo test's foot):
            // walking off at the cell's boundary he would otherwise be neither on the lift nor over
            // the ledge beside it for a frame, sink below the ledge's top and drop (D54).
            constexpr float kFootReach = 0.5f + 0.15f;
            const float liftStandY = LiftRiderCenterY(candidate.currentY);
            if (std::fabs(blupiX - candidate.currentX) < kFootReach && std::fabs(blupiZ - candidate.currentZ) < kFootReach &&
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
                if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType5)
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
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel69);
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
            // every other per-object branch so a small enemy such as Type4
            // (also in the generic-contact-hazard set) is destroyed by the aura rather
            // than also killing Blupi via hazard contact the same frame.
            if (blupiCloudActive && GetObjectDefinition(obj.type).smallEnemy)
            {
                const float adx = obj.currentX - blupiX;
                const float ady = obj.currentY - blupiY;
                const float adz = obj.currentZ - blupiZ;
                if (std::fabs(adx) < kCloudAuraAcross && std::fabs(adz) < kCloudAuraAcross &&
                    std::fabs(ady) < kCloudAuraUpDown)
                {
                    Retire(obj); // its piece flies away (BD-0049)
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel59);
                    // Its electric flash slides down to the floor under it (ObjectType38 at speed
                    // 55: down at 5 px a tick, over the fall SearchDistRight measures less a cell;
                    // Decor.cpp:7976-7983, 7647; BD-0055).
                    int egx = 0, egy = 0, egz = 0;
                    ToGridCell(world, obj.currentX, obj.currentY, obj.currentZ, egx, egy, egz);
                    const int fall = SearchAirDistance(world, egx, egy, egz, 0, -1, 0);
                    MobileObjSpec arc;
                    arc.type = GalaxyEggbert::Def::ObjectType::ObjectType38;
                    arc.active = true;
                    arc.phase = 0.0f;
                    arc.currentX = arc.posStartX = arc.posEndX = obj.currentX;
                    arc.currentY = arc.posStartY = obj.currentY;
                    arc.posEndY = obj.currentY - static_cast<float>(fall);
                    arc.currentZ = arc.posStartZ = arc.posEndZ = obj.currentZ;
                    arc.patrolStep = 2;
                    arc.patrolTime = 0.0f;
                    arc.stepAdvanceTicks = 5.0f * static_cast<float>(fall);
                    pendingSpawns.push_back(arc);
                    continue;
                }
            }

            // Perso-decoy/lethal-decor trap (found 2026-07-16, real Decor.cpp:7957-7975 +
            // `Decor::MovePersoDetect()` ~9835-9865) -- resolves the "what does placing a Perso
            // decoy actually DO gameplay-wise" mystery HUD-017's own writeup left open: small
            // small enemies that patrol into contact with ANY avatar-type object -- the
            // placed Perso decoy (200) itself, OR one of the 201-203 lethal-looking decorations
            // (`170`) -- mutually destroy each other. Real effects: an ObjectType8 explosion +
            // channel 10 + SmallShake at the ENEMY's position, then an ObjectType37 dissolve
            // effect at a small real offset from it; the detected 200-203 object is also deleted.
            // Checked before the Cloud aura above in real source's own per-object loop (this
            // engine's own ordering here is a natural adaptation, not a faithfulness question --
            // the two can't both apply to the same enemy the same frame regardless of order).
            if (GetObjectDefinition(obj.type).smallEnemy)
            {
                for (auto& other : objects)
                {
                    if (!other.active)
                    {
                        continue;
                    }
                    if (GetObjectDefinition(other.type).semanticKind !=
                        ObjectSemanticKind::Avatar)
                    {
                        continue;
                    }
                    // MovePersoDetect's boxes (Decor.cpp:9835-9865): the enemy's is Blupi's (28 px
                    // across, 11 to 58 below its cell's top), the decoy's an object's (28 px, its
                    // bottom 24) -- MoveObjectTouches with the enemy in Blupi's place (BD-0028; it
                    // was a 0.7-cell sphere).
                    const float pdx = other.currentX - obj.currentX;
                    const float pdy = other.currentY - obj.currentY;
                    const float pdz = other.currentZ - obj.currentZ;
                    if (MoveObjectTouches(other.type, pdx, pdy, pdz))
                    {
                        AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType8, obj.currentX, obj.currentY, obj.currentZ,
                                              pendingSpawns);
                        sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
                        events_.push_back(Event{EventKind::SmallShakeTriggered});
                        AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType37, obj.currentX, obj.currentY, obj.currentZ,
                                              pendingSpawns);
                        other.active = false;
                        Retire(obj);
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

            // Platform lift patrol (ObjectType1/47/48): the original's step machine for every
            // MoveObject (Decor::MoveObjectStepLine, lifts included) -- dwell timeStopStart,
            // advance over stepAdvance ticks, dwell timeStopEnd, recede over stepRecede -- so
            // lifts keep their authored timing and dwell, and lifts with the same ticks move in
            // step (D43; they used to ping-pong at `speed` with no dwell).
            if (GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::Lift)
            {
                AdvancePatrolStep(obj, dt);

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
                // BlupiController::RideLift().
                if (&obj == riddenLift)
                {
                    // The original's 2 px a tick (Decor.cpp:8161-8168, m_blupiVector.X += 2 / -= 2):
                    // 2 * 20 / 64 blocks a second. It was 0.3 ("no unit conversion established").
                    constexpr float kConveyorNudgeSpeed = 2.0f * 20.0f / 64.0f;
                    float nudgeX = 0.0f;
                    switch (GetObjectDefinition(obj.type).liftConveyorDirection)
                    {
                        case LiftConveyorDirection::PositiveX:
                            nudgeX = kConveyorNudgeSpeed * dt;
                            break;
                        case LiftConveyorDirection::NegativeX:
                            nudgeX = -kConveyorNudgeSpeed * dt;
                            break;
                        case LiftConveyorDirection::None:
                        default:
                            break;
                    }

                    ridingLift_ = true;
                    rideDeltaX_ = (obj.currentX - riddenLiftOldX) + nudgeX;
                    rideDeltaZ_ = obj.currentZ - riddenLiftOldZ;
                    rideStandY_ = LiftRiderCenterY(obj.currentY);
                    rideLiftX_ = obj.currentX;
                    rideLiftZ_ = obj.currentZ;
                    rideLiftConveyor_ = GetObjectDefinition(obj.type).liftConveyorDirection != LiftConveyorDirection::None;
                }
                continue;
            }

            // Crates (ObjectType12) are pushed and pulled by UpdateCratePush(), called before
            // this; the gate (Decor.cpp:6130-6132: every vehicle mode, Balloon, Ecrase) is kept
            // here only so a crate's way through this loop is what it was.
            if (GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::StaticObstacle &&
                blupiCanPushCrate && !blupiBallooned)
            {
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
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType22)
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
            // rate (WorldRuntime::Update()); reconstructing the phase as
            // of the START of this frame (`prevPhase`) lets each blast fire
            // on the exact frame its tick is crossed, not every frame after.
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType56)
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
                        AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType8, centerX, centerY, centerZ, pendingSpawns);

                        if (blast.dx == 0.0f && blast.dy == 0.0f)
                        {
                            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
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

                        // The tiles it clears (Decor.cpp:9078-9096): saws (378/379) and drips
                        // (404/410) in its own row and the one above. Galaxy's saws lie in the
                        // floor, a row under the original's (the floor convention, GP-0450), so
                        // for saws the rows are the floor and his own; its drips hang as the
                        // original's, from the ceiling (BD-0033). Until 2026-11-03 only his own row
                        // was cleared and drips not at all, so a blast never cut a floor saw
                        // (BD-0046). Across X and Z +-1 cell (in 3D each blast reaches as far along
                        // Z as along X; 2026-10-25).
                        const int wx0 = static_cast<int>(std::round(centerX - kBlastHalfExtent)) +
                                        WorldRuntime::kWorldCenterX;
                        const int wx1 = static_cast<int>(std::round(centerX + kBlastHalfExtent)) +
                                        WorldRuntime::kWorldCenterX;
                        const int wz0 = static_cast<int>(std::round(centerZ - kBlastHalfExtent)) +
                                        WorldRuntime::kWorldCenterZ;
                        const int wz1 = static_cast<int>(std::round(centerZ + kBlastHalfExtent)) +
                                        WorldRuntime::kWorldCenterZ;
                        const int wy = static_cast<int>(std::round(centerY));
                        const int axis = static_cast<int>(world.blocksPerAxis());
                        for (int y = wy - 1; y <= wy + 1; ++y)
                        {
                            for (int wz = std::max(0, wz0); y >= 0 && y < axis && wz <= std::min(axis - 1, wz1); ++wz)
                            {
                                for (int wx = std::max(0, wx0); wx <= std::min(axis - 1, wx1); ++wx)
                                {
                                    const auto blockType = world.getBlock(static_cast<std::uint16_t>(wx),
                                                                           static_cast<std::uint16_t>(y),
                                                                           static_cast<std::uint16_t>(wz))
                                                                .type();
                                    const bool saw = blockType == GalaxyEggbert::BlockTypes::Saw ||
                                                     blockType == GalaxyEggbert::BlockTypes::SawStopped;
                                    const bool drip = blockType == GalaxyEggbert::BlockTypes::Drip || blockType == 410;
                                    if ((saw && y <= wy) || (drip && y >= wy))
                                    {
                                        worldRuntime.GetWorldMutable().setBlock(
                                            static_cast<std::uint16_t>(wx), static_cast<std::uint16_t>(y),
                                            static_cast<std::uint16_t>(wz),
                                            Worlds::Block::make(GalaxyEggbert::BlockTypes::Air));
                                    }
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
                            if (!GetObjectDefinition(victim.type).dynamiteDestructible)
                            {
                                continue;
                            }
                            if (std::fabs(victim.currentX - centerX) > kBlastHalfExtent + 0.5f ||
                                std::fabs(victim.currentY - centerY) > kBlastHalfExtent + 0.5f ||
                                std::fabs(victim.currentZ - centerZ) > kBlastHalfExtent + 0.5f)
                            {
                                continue;
                            }
                            if (GetObjectDefinition(victim.type).semanticKind ==
                                ObjectSemanticKind::StaticObstacle)
                            {
                                std::vector<MobileObjSpec*> linked;
                                linked.push_back(&victim);
                                bool addedAny = true;
                                while (addedAny)
                                {
                                    addedAny = false;
                                    for (auto& other : objects)
                                    {
                                        if (!other.active ||
                                            GetObjectDefinition(other.type).semanticKind !=
                                                ObjectSemanticKind::StaticObstacle) continue;
                                        if (std::find(linked.begin(), linked.end(), &other) != linked.end()) continue;
                                        bool touches = false;
                                        for (auto* member : linked)
                                        {
                                            if (std::fabs(other.currentX - member->currentX) < 1.5f &&
                                                std::fabs(other.currentY - member->currentY) < 1.5f &&
                                                std::fabs(other.currentZ - member->currentZ) < 1.5f)
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
                                    // Each linked crate flies away, spinning 7-22 a tick either way
                                    // (Decor.cpp:9142-9156; BD-0049).
                                    ByeByePiece piece;
                                    piece.look = *member;
                                    piece.x = member->currentX;
                                    piece.y = member->currentY;
                                    piece.z = member->currentZ;
                                    const int spin = std::uniform_int_distribution<int>(7, 22)(rng_);
                                    const bool reverse = std::uniform_int_distribution<int>(0, 99)(rng_) % 2 == 0;
                                    ByeByeAdd(piece, static_cast<float>(reverse ? -spin : spin), 1.0f);
                                    member->active = false;
                                }
                            }
                            else
                            {
                                Retire(victim);
                            }
                        }

                        // Blupi dies within a cell of the blast (Decor.cpp:9160-9166: his position
                        // from 64 px before to 60 px past the blast's, X and Y; BD-0046: it was a
                        // cell and a half), unless shielded, hidden or SuperBlupi
                        // (blupiInvincible). Screen y grows downward: up in Galaxy is before.
                        // Along Z, which the original's scattered blasts do not reach, the
                        // neighbouring cell as well (2026-10-25: in front of or behind the stick).
                        const float blupiDX = blupiX - centerX;
                        const float blupiDY = centerY - blupiY; // screen y
                        if (!blupiInvincible && blupiDX > -1.0f && blupiDX < 60.0f / 64.0f && blupiDY > -1.0f &&
                            blupiDY < 60.0f / 64.0f && std::fabs(blupiZ - centerZ) <= kBlastHalfExtent + 0.5f)
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
            // agree). The original's own table (Tables.cpp:1473, D53): the
            // bridge shakes 365/366 and holds him for 16 ticks, crumbles
            // through 367-372, is gone until tick 140 and comes back.
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType52)
            {
                static constexpr std::array<std::int16_t, 157> kBridgeTable = [] {
                    std::array<std::int16_t, 157> table{};
                    for (int t = 0; t < 157; ++t) table[t] = -1;
                    for (int t = 0; t < 16; ++t) table[t] = static_cast<std::int16_t>(t % 2 == 0 ? 365 : 366);
                    for (int t = 16; t < 28; ++t) table[t] = static_cast<std::int16_t>(367 + (t - 16) / 2);
                    for (int t = 140; t < 156; ++t) table[t] = static_cast<std::int16_t>(372 - (t - 140) / 2);
                    table[156] = 364;
                    return table;
                }();
                constexpr float kProgressSoundTick = 137.0f;

                const float prevPhase = obj.phase - dt * 20.0f;
                if (prevPhase < kProgressSoundTick && obj.phase >= kProgressSoundTick)
                {
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel73);
                }

                const int tick = std::clamp(static_cast<int>(obj.phase), 0, 156);
                const int icon = kBridgeTable[static_cast<std::size_t>(tick)];

                const int gx = static_cast<int>(std::lround(obj.currentX)) + WorldRuntime::kWorldCenterX;
                const int gz = static_cast<int>(std::lround(obj.currentZ)) + WorldRuntime::kWorldCenterZ;
                const int gy = static_cast<int>(std::lround(obj.currentY));
                const int axis = static_cast<int>(world.blocksPerAxis());
                if (gx >= 0 && gx < axis && gy >= 0 && gy < axis && gz >= 0 && gz < axis)
                {
                    worldRuntime.GetWorldMutable().setBlock(
                        static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                        static_cast<std::uint16_t>(gz),
                        Worlds::Block::make(GalaxyEggbert::BlockTypes::fromMobileIconId(icon)));
                }

                if (obj.phase >= 157.0f)
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
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType96 && !blupiHidden)
            {
                const float fwdx = obj.currentX - blupiX;
                const float fwdy = obj.currentY - blupiY;
                const float fwdz = obj.currentZ - blupiZ;
                if (std::fabs(fwdx) < kFollowerWakeAcross && std::fabs(fwdz) < kFollowerWakeAcross &&
                    fwdy < kFollowerWakeOver && fwdy > -kFollowerWakeUnder)
                {
                    obj.type = GalaxyEggbert::Def::ObjectType::ObjectType97;
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel92);
                }
            }

            // Follower homing (ObjectType97, plan.md E3D-MIG-137, real
            // Decor.cpp:8025-8064) -- once awake, steps X and Y
            // independently (Chebyshev-style, NOT a normalized diagonal)
            // toward Blupi's live position every frame at
            // kFollowerHomingSpeed -- and, in 3D (2026-10-25), Z the same way,
            // or a follower at another depth could never reach him. Collapses posStart/posEnd to the new
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
            // check at the destination cell via `BlupiController::
            // IsSolidAt` (this file's own existing tile-grid convention,
            // e.g. `SearchAirDistance`) -- real `TestPath` sweeps a
            // rectangle, not a point, same simplification as every other
            // collision check in this file. The cosmetic `ObjectType9`
            // debris flash (plan.md VISUAL-008) and real `SmallShake`
            // camera shake (`m_decorAction = GalaxyEggbert::Def::DecorAction::SmallShake`,
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
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType97 && !blupiHidden)
            {
                float endX = obj.currentX;
                float endY = obj.currentY;
                const float stepAmount = kFollowerHomingSpeed * dt;
                if (endX < blupiX) endX = std::min(endX + stepAmount, blupiX);
                else if (endX > blupiX) endX = std::max(endX - stepAmount, blupiX);
                if (endY < blupiY) endY = std::min(endY + stepAmount, blupiY);
                else if (endY > blupiY) endY = std::max(endY - stepAmount, blupiY);
                float endZ = obj.currentZ;
                if (endZ < blupiZ) endZ = std::min(endZ + stepAmount, blupiZ);
                else if (endZ > blupiZ) endZ = std::max(endZ - stepAmount, blupiZ);

                int fgx, fgy, fgz;
                ToGridCell(world, endX, endY, endZ, fgx, fgy, fgz);
                if (IsSolidAt(worldRuntime, fgx, fgy, fgz))
                {
                    Retire(obj);
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
                    AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType9, obj.currentX, obj.currentY, obj.currentZ,
                                          pendingSpawns);
                    events_.push_back(Event{EventKind::SmallShakeTriggered});
                    continue;
                }
                obj.currentX = obj.posStartX = obj.posEndX = endX;
                obj.currentY = obj.posStartY = obj.posEndY = endY;
                obj.currentZ = obj.posStartZ = obj.posEndZ = endZ;
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
                    if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType32 && CrossedTick(prevPatrolTime, dtTicks, 21.0f))
                    {
                        FireBlupihShot(obj, world, sound, pendingSpawns);
                    }
                    else if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType33)
                    {
                        const bool alongZ = std::fabs(obj.posEndZ - obj.posStartZ) > std::fabs(obj.posEndX - obj.posStartX);
                        const float start = alongZ ? obj.posStartZ : obj.posStartX;
                        const float end = alongZ ? obj.posEndZ : obj.posEndX;
                        const bool aboutToWalkPositive =
                            (start < end && prevPatrolStep == 1) || (start > end && prevPatrolStep == 3);
                        if (CrossedTick(prevPatrolTime, dtTicks, 3.0f))
                        {
                            FireBlupitShot(obj, aboutToWalkPositive ? -1.0f : 1.0f, alongZ, world, sound, pendingSpawns);
                        }
                        if (CrossedTick(prevPatrolTime, dtTicks, 21.0f))
                        {
                            FireBlupitShot(obj, aboutToWalkPositive ? 1.0f : -1.0f, alongZ, world, sound, pendingSpawns);
                        }
                    }
                }
            }

            // Blupih/blupit's own body (ObjectType32/33) is deliberately
            // NOT marked genericContactHazard in the registry -- real
            // mobile-eggbert never treats
            // walking into their body as a damage path, only their fired
            // projectile is harmful (see FireBlupihShot/FireBlupitShot
            // above). Falls through to the final pickup-type filter below,
            // which already excludes them (no separate `continue` needed).

            // Fired projectile contact (ObjectType23, plan.md E3D-MIG-134):
            // verified directly against Decor.cpp:5914-5947 -- always fatal
            // (real Glu/glue-style death) and destroys the bullet itself.
            // Real Shield/Hide/superBlupi immunity (blupiInvincible) and the
            // Win/Bye/dying/gone gate (blupiOutOfPlay, BD-0062): the shot passes.
            // Real death has no distinct sound call of
            // its own (StartSploutchGlu only spawns silent splash-effect
            // debris, now modeled -- plan.md VISUAL-009, see
            // AppendSplatEffect()'s own comment) -- channel 74 reused here
            // for consistency with this class's existing hazard-death
            // sound approximation.
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType23)
            {
                if (obj.firedByBlupi)
                {
                    continue;
                }
                const float bdx = obj.currentX - blupiX;
                const float bdy = obj.currentY - blupiY;
                const float bdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && !blupiOutOfPlay && MoveObjectTouches(obj.type, bdx, bdy, bdz))
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
                    // StartSploutchGlu's sound (Decor.cpp:7763-7792): channel 51, as the other glue
                    // death; 74 is the soul's ascent (BD-0036).
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel51);
                }
                continue;
            }

            // Wasp (ObjectType44, plan.md E3D-MIG-135) -- does NOT kill or
            // destroy itself; contact signals a BalloonTouched event so
            // the caller can attempt BlupiController::TriggerBalloon()
            // (idempotent there, not here -- see the class comment). Real
            // gate also includes `!m_blupiShield && !m_blupiHide`
            // (Decor.cpp:5826, plan.md E3D-MIG-170) -- a shielded/hidden
            // Blupi doesn't get ballooned at all.
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType44)
            {
                const float wdx = obj.currentX - blupiX;
                const float wdy = obj.currentY - blupiY;
                const float wdz = obj.currentZ - blupiZ;
                if (!blupiInvincible && MoveObjectTouches(obj.type, wdx, wdy, wdz))
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
            // vehicle -- real source sets `GalaxyEggbert::Def::BlupiAction::Glu` unconditionally
            // either way (still a real death via the shared per-action
            // life-loss dispatch, `Decor.cpp:6374-6392`), it just ALSO plays a
            // different sound/shake (channel 10 + SmallShake + a pop effect,
            // vs. plain channel 51) and clears vehicle/Balloon/Ecrase state
            // when contact happens while riding/ballooned/squashed
            // (Decor.cpp:5884-5905) -- mirroring what `BlupiDead()` already
            // does unconditionally for every other real death cause
            // (`BlupiController::TriggerDeathLock()` now does this too,
            // fixed alongside this same research). In a vehicle or squashed the
            // vehicle bursts where the creature is (ObjectType10, channel 10, a
            // small shake) instead of channel 51 (BD-0061, below). The taunt that
            // the creature calls for is MockeryDetect()'s (Mockeryp, BD-0044).
            if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType54)
            {
                // m_blupiFocus too (Decor.cpp:5867-5868): busy, he is not caught.
                if ((obj.patrolStep == 1 || obj.patrolStep == 3) && !blupiBallooned && !blupiInvincible && blupiFocused)
                {
                    const float gdx = obj.currentX - blupiX;
                    const float gdy = obj.currentY - blupiY;
                    const float gdz = obj.currentZ - blupiZ;
                    if (MoveObjectTouches(obj.type, gdx, gdy, gdz))
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
                        // In a vehicle or squashed (Decor.cpp:5884-5905; blupiCanPushCrate is exactly
                        // "neither", and the balloon cannot be on here): the vehicle bursts where the
                        // creature is (ObjectType10, channel 10, a small shake); else channel 51 (BD-0061).
                        if (!blupiCanPushCrate)
                        {
                            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
                            AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType10, obj.currentX, obj.currentY,
                                                 obj.currentZ, pendingSpawns);
                            events_.push_back(Event{EventKind::SmallShakeTriggered});
                        }
                        else
                        {
                            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel51);
                        }
                        // Its tentacle rises where he stands (ObjectType53, Decor.cpp:5871-5873; BD-0054).
                        AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType53, blupiX, blupiY, blupiZ,
                                             pendingSpawns);
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
            // 4 of these 8 types (3/16/96/97, the registry's
            // balloonPoppableHazard capability)
            // pop the balloon instead of killing (real channel 41 is
            // played by BlupiController's own IsBallooned() before/after
            // comparison in the caller, not here -- see PopBalloon()'s
            // comment) -- 2/4/17/20 still kill even while ballooned. Real
            // gate also includes `!m_blupiShield && !m_blupiHide`
            // (Decor.cpp:5784, plan.md E3D-MIG-170) -- modeled via
            // blupiInvincible, skipping the whole contact (no kill, no
            // balloon-pop either).
            if (GetObjectDefinition(obj.type).genericContactHazard)
            {
                if (blupiCrouching &&
                    GetObjectDefinition(obj.type).genericContactHazardCrouchImmune)
                {
                    continue;
                }
                const float hdx = obj.currentX - blupiX;
                const float hdy = obj.currentY - blupiY;
                const float hdz = obj.currentZ - blupiZ;
                const bool touches = MoveObjectTouches(obj.type, hdx, hdy, hdz);
                // A still mine close without touching (Decor.cpp:9763-9767, MoveObjectDetect's
                // near box: 20 px wider each side, 40 above, 30 below): his "phew" (Ouf4, armed by
                // the caller).
                if (obj.type == GalaxyEggbert::Def::ObjectType::ObjectType2 && !touches &&
                    MoveObjectTouches(obj.type, hdx, hdy, hdz, 20.0f, 40.0f, 30.0f))
                {
                    ReplaceEvent(EventKind::EnemyNearMiss, Event{EventKind::EnemyNearMiss});
                }
                if (!blupiInvincible && !contactGrace_ && touches)
                {
                    if (blupiBallooned &&
                        GetObjectDefinition(obj.type).balloonPoppableHazard)
                    {
                        events_.push_back(Event{EventKind::BalloonPopped});
                        StartContactGrace(blupiX, blupiY, blupiZ); // Decor.cpp:5780
                    }
                    else
                    {
                        Retire(obj);
                        // The hazard's explosion, whether he dies or rides over it (Decor.cpp:5822).
                        sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
                        if (blupiArmoredVehicle || !blupiFocused)
                        {
                            // Run over in the jeep, the tank or on the skateboard, or met while he
                            // is busy (no m_blupiFocus, on the ground: Decor.cpp:5788-5790): only
                            // the hazard goes (the real site's shake and flash below).
                        }
                        else
                        {
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
                                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel74);
                                RequestClear2Ascend(obj.currentX, obj.currentY, obj.currentZ);
                            }
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
                        if (GetObjectDefinition(obj.type).genericContactHazardFeedback ==
                            GenericContactHazardFeedback::Big)
                        {
                            events_.push_back(Event{EventKind::BigShakeTriggered});
                            AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType10, obj.currentX, obj.currentY, obj.currentZ,
                                                  pendingSpawns);
                        }
                        else
                        {
                            events_.push_back(Event{EventKind::SmallShakeTriggered});
                            AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType8, obj.currentX, obj.currentY, obj.currentZ,
                                                  pendingSpawns);
                        }
                    }
                }
                continue;
            }

            // Types 201-203 (plan.md PICKUP-069, found 2026-07-16, real Decor.cpp:6088-6115) --
            // lethal decorative objects sharing ObjectType200's real "MoveObject 200-203" range,
            // but NOT Perso itself (200, the placeable decoy, already implemented separately,
            // handled entirely by TryPerso()/GalaxyEggbertGame.cpp -- not here). Real contact:
            // same Clear1/Clear2 coinflip death as the generic-hazard list above (real
            // BlupiDead(Clear1, Clear2)), Shield/Hide immunity (blupiInvincible), no
            // m_blupiRestart=true anywhere in this real block (shouldRespawn=false, same as the
            // generic-hazard list) -- but ALWAYS channel 10 + SmallShake + an ObjectType10 pop
            // effect (no fish/bird BigShake variant here, unlike the generic-hazard list).
            if (GetObjectDefinition(obj.type).lethalDecorContactHazard)
            {
                const float sdx = obj.currentX - blupiX;
                const float sdy = obj.currentY - blupiY;
                const float sdz = obj.currentZ - blupiZ;
                // Not met at all while he is busy (Decor.cpp:6088 gates the block on m_blupiFocus).
                if (blupiFocused && !blupiInvincible && MoveObjectTouches(obj.type, sdx, sdy, sdz))
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
                        sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel74);
                        RequestClear2Ascend(obj.currentX, obj.currentY, obj.currentZ);
                    }
                    events_.push_back(Event{EventKind::SmallShakeTriggered});
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel10);
                    AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType10, obj.currentX, obj.currentY, obj.currentZ,
                                          pendingSpawns);
                }
                continue;
            }

            // Pickup collection -- treasure/egg/level-exit/keys, the only
            // IsPickup() types placed in today's sample world. Sound
            // channels and removal behavior come from `mobile-eggbert-
            // reference/13-object-pickups.md`/`07-sounds.md`. The verified
            // mapping avoids two earlier mistakes: treasure/key pickup is
            // channel 11 (or 19 for the
            // set-completing treasure), not channel 10, and egg pickup is
            // channel 3, not channel 42 (42 is Shield activation, unrelated).
            if (!GetObjectDefinition(obj.type).standardPickupTouch)
            {
                continue;
            }

            const float dx = obj.currentX - blupiX;
            const float dy = obj.currentY - blupiY;
            const float dz = obj.currentZ - blupiZ;
            // Touched as the original's boxes say, as the hazards (BD-0021).
            const bool touching = MoveObjectTouches(obj.type, dx, dy, dz);

            // Secret exit (ObjectType21, plan.md PICKUP-009/083, found 2026-07-16) shares the
            // EXACT same real exit-gate logic as the regular exit (ObjectType7) below
            // (Decor.cpp:6158-6184, one `if` covering both types) -- the only real difference is
            // setting `m_bFoundCle=true` on this variant, a "found the secret" flag with no
            // known consumer in this engine's own save/ranking scope (not modeled, matching this
            // engine's deliberately-independent SaveData format). Previously this engine only
            // recognized ObjectType7, so a secret exit did nothing at all on contact.
            if (GetObjectDefinition(obj.type).levelExit)
            {
                // Level-exit goal (Decor.cpp:6158-6189): checked when m_goalPhase is 0, then not for
                // 50 ticks while he stays on it (so every 51), reset when he leaves. Every treasure
                // held (m_nbTresor >= m_totalTresor): the fanfare (channel 14) and the win; else the
                // refusal (13), again each 51 ticks -- and the last treasure's reward landing while
                // he stands there wins without stepping off (BD-0026; this was once per contact).
                // Never removed: a goal marker, not a pickup.
                if (touching && !exitReached_)
                {
                    touchingExitThisFrame = true;
                    if (exitGoalTicks_ > 0.0f)
                    {
                        exitGoalTicks_ -= dt * 20.0f;
                    }
                    else
                    {
                        exitGoalTicks_ = 50.0f;
                        if (treasuresCollected_ >= totalTreasures_)
                        {
                            exitReached_ = true;
                            exitWasGoldenKey_ = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType21;
                            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel14);
                        }
                        else
                        {
                            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel13);
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
            // fixed HUD-space point (matching `Hud.cpp`'s own real
            // constants exactly) as its END -- the caller
            // (`GalaxyEggbertGame.cpp`) projects the world position and
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
                // Def/ObjectType.hpp's own "Key 1/2/3 collectible" doc comments),
                // spawned immediately at touch time, NOT deferred to voyage
                // completion.
                case GalaxyEggbert::Def::ObjectType::ObjectType5:  // treasure
                    TryCollectBasicPickup(obj, pendingSpawns);
                    break;
                case GalaxyEggbert::Def::ObjectType::ObjectType49: // key 1
                case GalaxyEggbert::Def::ObjectType::ObjectType50: // key 2
                case GalaxyEggbert::Def::ObjectType::ObjectType51: // key 3
                {
                    // A key is held or not (the original's m_blupiCle flags): one of a colour he
                    // holds, or one still flying to the panel, stays where it is until a door has
                    // used his (Decor.cpp:5962-6001; a door clears it, 5619). Taken anyway, it was
                    // lost when the first door cleared the colour (BD-0023).
                    const bool k1 = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType49;
                    const bool k2 = obj.type == GalaxyEggbert::Def::ObjectType::ObjectType50;
                    const int held = k1 ? keys1_ : k2 ? keys2_ : keys3_;
                    const VoyageKind flying = k1 ? VoyageKind::Key1 : k2 ? VoyageKind::Key2 : VoyageKind::Key3;
                    if (held == 0 && voyageKind_ != flying)
                    {
                        TryCollectBasicPickup(obj, pendingSpawns);
                    }
                    break;
                }
                case GalaxyEggbert::Def::ObjectType::ObjectType55: // dynamite stick
                    // Real gate (Decor.cpp:6116-6119): taken only with the action button, none
                    // carried and no stick still flying to the panel (the count rises when it
                    // lands; without this a second stick was taken during the flight, 065).
                    if (actionUnspent && dynamiteCount_ == 0 && voyageKind_ != VoyageKind::Dynamite)
                    {
                        actionUnspent = false;
                        actionSpent_ = true;
                        TryCollectBasicPickup(obj, pendingSpawns);
                    }
                    break;
                case GalaxyEggbert::Def::ObjectType::ObjectType6: // extra-life egg
                    // The egg is a life: taken only while he has fewer than MAX_EGG_COUNT=10
                    // lives (m_nbVies < 10, Decor.cpp:6007); at ten, touching one does nothing
                    // at all, it is not even removed. (The gate was the eggs taken so far until
                    // 2026-11-05: with his 3 lives ten eggs made 13, and after ten he could never
                    // take another however many he had lost.)
                    if (lives_ < kMaxEggCount)
                    {
                        obj.active = false;
                        // Real end point VoyageGetPosVie(m_nbVies+1) =
                        // (210+16*(lives+1), 417), Decor.cpp:10141-10147/6012.
                        RequestVoyage(VoyageKind::Egg, 21, false, obj.currentX, obj.currentY, obj.currentZ,
                                      210.0f + 16.0f * static_cast<float>(lives_ + 1), 417.0f, true);
                    }
                    break;
                case GalaxyEggbert::Def::ObjectType::ObjectType29: // bullet pack
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
                // delay is handled by the caller (BlupiController::
                // TriggerPickupFreeze(), since this class has no access to
                // it) -- see EventKind's own comment on the position
                // payload. Invert/Mirror (ObjectType40, plan.md PICKUP-011)
                // is a separate real pickup family sharing the same
                // instant-grant-on-contact shape as Shield/Charge. Real
                // per-pickup gates, including Power/Cloud/Hide's real
                // vehicle-mode + Balloon/Ecrase exclusions (Decor.cpp:
                // 6025-6087; Shield/Invert have none, verified 2026-07-16),
                // are passed in from the caller's own BlupiController
                // state (blupiCanGrantShield/Power/Cloud/Hide/Invert) since
                // this class has no access to BlupiController itself --
                // consistent with the blupiBallooned/blupiCrouching
                // precedent. INFRA-006 pilot (plan.md §7): these 5 types
                // share one handler-table-driven dispatch,
                // TryGrantSecretPowerPickup() -- see its own comment and
                // kSecretPowerPickupHandlers above.
                case GalaxyEggbert::Def::ObjectType::ObjectType25: // shield stick
                case GalaxyEggbert::Def::ObjectType::ObjectType26: // suction-cup ("Sucette" -> Power)
                case GalaxyEggbert::Def::ObjectType::ObjectType30: // drink ("Drink" -> Hide)
                case GalaxyEggbert::Def::ObjectType::ObjectType31: // charge ("Charge" -> Cloud)
                case GalaxyEggbert::Def::ObjectType::ObjectType40: // mirror/invert
                    TryGrantSecretPowerPickup(obj, blupiCanGrantShield, blupiCanGrantPower, blupiCanGrantCloud,
                                              blupiCanGrantHide, blupiCanGrantInvert, blupiActionPressedEdge);
                    break;
                default:
                    break;
            }
        }

        if (!touchingExitThisFrame)
        {
            exitGoalTicks_ = 0.0f;
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
                static_cast<int>(std::lround(blupiX)) + WorldRuntime::kWorldCenterX,
                static_cast<int>(std::lround(blupiX + static_cast<float>(blupiFacingDX))) +
                    WorldRuntime::kWorldCenterX,
            };
            const int probeGZ[2] = {
                static_cast<int>(std::lround(blupiZ)) + WorldRuntime::kWorldCenterZ,
                static_cast<int>(std::lround(blupiZ + static_cast<float>(blupiFacingDZ))) +
                    WorldRuntime::kWorldCenterZ,
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
                    // The whole door: a 3D door is a block of cells of its icon (a doorway's width
                    // and height), the original's one tile. Opening only the probed cell, which a
                    // diagonal facing can make one off his way, spent the key and left the doorway
                    // shut (D58, 107's red door).
                    std::vector<std::array<int, 3>> door{{gx, probeGY, gz}};
                    for (std::size_t k = 0; k < door.size() && door.size() < 64; ++k)
                    {
                        const auto [cx, cy, cz] = door[k];
                        constexpr int kSides[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
                        for (const auto& side : kSides)
                        {
                            const int nx = cx + side[0], ny = cy + side[1], nz = cz + side[2];
                            if (nx < 0 || ny < 0 || nz < 0 || nx >= axis || ny >= axis || nz >= axis ||
                                std::find(door.begin(), door.end(), std::array<int, 3>{nx, ny, nz}) != door.end())
                            {
                                continue;
                            }
                            if (terrain.getBlock(static_cast<std::uint16_t>(nx), static_cast<std::uint16_t>(ny),
                                                 static_cast<std::uint16_t>(nz))
                                    .type() == icon)
                            {
                                door.push_back({nx, ny, nz});
                            }
                        }
                    }
                    for (const auto& [cx, cy, cz] : door)
                    {
                        OpenDoorAt(worldRuntime, cx, cy, cz, sound);
                    }
                    if (keyType == 49) keys1_ = 0;
                    else if (keyType == 50) keys2_ = 0;
                    else if (keyType == 51) keys3_ = 0;
                    // Real door-unlock key-consumption flourish (plan.md
                    // `158`, Decor.cpp:5620-5624) -- purely cosmetic, the
                    // key bit is already cleared above, before the voyage
                    // even starts (no reward at completion). Reversed
                    // direction from every pickup above: start = a fixed
                    // HUD position (see startX), end = the
                    // door's own world position (needs projecting, unlike
                    // every pickup's fixed end). Real icon is dynamic per
                    // door: `214 + (doorIcon-334)*7` = 214/221/228 for
                    // door1/2/3.
                    // It starts at x 520 for every door (Decor.cpp:5620-5621), where the first
                    // key's own icon arrived, also for the second and third keys (530, 540; BD-0043).
                    const float startX = 520.0f;
                    const int doorIconId = 214 + (static_cast<int>(icon) - 334) * 7;
                    const float doorWorldX = static_cast<float>(gx) - static_cast<float>(WorldRuntime::kWorldCenterX);
                    const float doorWorldY = static_cast<float>(probeGY);
                    const float doorWorldZ = static_cast<float>(gz) - static_cast<float>(WorldRuntime::kWorldCenterZ);
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
            if (blupiFocused && // m_blupiFocus (Decor.cpp:5608; BD-0066)
                world.getBlock(static_cast<std::uint16_t>(bridgeGX), static_cast<std::uint16_t>(bridgeGY),
                               static_cast<std::uint16_t>(bridgeGZ))
                        .type() == GalaxyEggbert::BlockTypes::Bridge)
            {
                bool alreadyBuilding = false;
                for (const auto& obj : objects)
                {
                    if (obj.active && obj.type == GalaxyEggbert::Def::ObjectType::ObjectType52 &&
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
                    bridge.type = GalaxyEggbert::Def::ObjectType::ObjectType52;
                    bridge.posStartX = bridge.posEndX = bridge.currentX = static_cast<float>(std::lround(blupiX));
                    // In the bridge's own cell (its falling pieces are drawn there, OR-0031).
                    bridge.posStartY = bridge.posEndY = bridge.currentY = static_cast<float>(bridgeGY);
                    bridge.posStartZ = bridge.posEndZ = bridge.currentZ = static_cast<float>(std::lround(blupiZ));
                    bridge.phase = 0.0f;
                    bridge.active = true;
                    pendingSpawns.push_back(bridge);
                    sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel72);
                }
            }
        }

        // Player-fired Tank bullet (2026-07-13, plan.md BULLET-001, real
        // Decor.cpp:4308-4344) -- see Update()'s own class comment for the
        // full real-behavior citation. Cooldown ticks down every frame
        // regardless of input; only reset on an actual shot.
        fireCooldownTimer_ = std::max(0.0f, fireCooldownTimer_ - dt);
        if (glueDropTimer_ > 0.0f)
        {
            // The helicopter's glue ball leaves (Decor.cpp:3060-3075): straight down from under
            // him, one bullet, the cooldown, and the kick up (the caller's).
            glueDropTimer_ -= dt;
            if (glueDropTimer_ <= 0.0f && bulletCount_ > 0)
            {
                int gx, gy, gz;
                ToGridCell(world, blupiX, blupiY, blupiZ, gx, gy, gz);
                const int dist = SearchAirDistance(world, gx, gy, gz, 0, -1, 0);
                --bulletCount_;
                fireCooldownTimer_ = kFireCooldownSeconds;
                events_.push_back(Event{EventKind::GlueDropped});
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel52);
                if (dist > 0)
                {
                    MobileObjSpec blupiPos{};
                    blupiPos.currentX = blupiX;
                    blupiPos.currentY = blupiY;
                    blupiPos.currentZ = blupiZ;
                    MobileObjSpec ball = MakeBullet(blupiPos, 0.0f, -1.0f, 0.0f, dist);
                    ball.firedByBlupi = true;
                    pendingSpawns.push_back(ball);
                }
            }
        }
        else if (blupiFirePressed && blupiCanFire && fireCooldownTimer_ <= 0.0f)
        {
            if (bulletCount_ > 0 && blupiDropsGlue)
            {
                glueDropTimer_ = kGlueDropDelaySeconds;
                events_.push_back(Event{EventKind::GlueDropStarted});
            }
            else if (bulletCount_ > 0)
            {
                int gx, gy, gz;
                ToGridCell(world, blupiX, blupiY, blupiZ, gx, gy, gz);
                const int dist = SearchAirDistance(world, gx, gy, gz, blupiFacingDX, 0, blupiFacingDZ);
                --bulletCount_;
                fireCooldownTimer_ = kFireCooldownSeconds;
                events_.push_back(Event{EventKind::TankFired});
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel52);
                if (dist > 0)
                {
                    MobileObjSpec blupiPos{};
                    blupiPos.currentX = blupiX;
                    blupiPos.currentY = blupiY;
                    blupiPos.currentZ = blupiZ;
                    MobileObjSpec shot = MakeBullet(blupiPos, static_cast<float>(blupiFacingDX), 0.0f,
                                                    static_cast<float>(blupiFacingDZ), dist);
                    shot.firedByBlupi = true;
                    pendingSpawns.push_back(shot);
                }
            }
            else
            {
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel53);
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

        // The trajectories (IsTrajectoryCell's comment): each object's cell this tick, the cell over
        // a lift, and a new shot's whole path (SearchDistRight marks it as the shot starts).
        {
            const auto& world = worldRuntime.GetWorld();
            const int n = static_cast<int>(world.blocksPerAxis());
            const auto mark = [&](float x, float y, float z)
            {
                const int gx = static_cast<int>(std::lround(x)) + WorldRuntime::kWorldCenterX;
                const int gy = static_cast<int>(std::lround(y));
                const int gz = static_cast<int>(std::lround(z)) + WorldRuntime::kWorldCenterZ;
                if (gx >= 0 && gy >= 0 && gz >= 0 && gx < n && gy < n && gz < n)
                {
                    trajectoryCells_.insert(static_cast<std::uint32_t>((gx * n + gy) * n + gz));
                }
            };
            for (const auto& spec : pendingSpawns)
            {
                if (spec.type != GalaxyEggbert::Def::ObjectType::ObjectType23)
                {
                    continue;
                }
                const float length = std::max({std::fabs(spec.posEndX - spec.posStartX),
                                               std::fabs(spec.posEndY - spec.posStartY),
                                               std::fabs(spec.posEndZ - spec.posStartZ)});
                const int cells = static_cast<int>(std::ceil(length));
                for (int k = 0; k <= cells; ++k)
                {
                    const float t = cells > 0 ? static_cast<float>(k) / static_cast<float>(cells) : 0.0f;
                    mark(spec.posStartX + (spec.posEndX - spec.posStartX) * t,
                         spec.posStartY + (spec.posEndY - spec.posStartY) * t,
                         spec.posStartZ + (spec.posEndZ - spec.posStartZ) * t);
                }
            }
            for (const auto& obj : objects)
            {
                if (!obj.active)
                {
                    continue;
                }
                const bool lift = GetObjectDefinition(obj.type).semanticKind == ObjectSemanticKind::Lift;
                mark(obj.currentX, lift ? LiftRiderCenterY(obj.currentY) : obj.currentY, obj.currentZ);
            }
        }
    }

    bool InteractionSystem::LiftWithinOverHeight(const WorldRuntime& worldRuntime, float x, float y, float z)
    {
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (!obj.active || GetObjectDefinition(obj.type).semanticKind != ObjectSemanticKind::Lift)
            {
                continue;
            }
            const float under = y - LiftRiderCenterY(obj.currentY); // its top under his feet, cells
            if (std::fabs(obj.currentX - x) < 50.0f / 64.0f && std::fabs(obj.currentZ - z) < 50.0f / 64.0f &&
                under > -18.0f / 64.0f && under < 79.0f / 64.0f)
            {
                return true;
            }
        }
        return false;
    }

    bool InteractionSystem::IsTrajectoryCell(const WorldRuntime& worldRuntime, float x, float y, float z) const
    {
        const int n = static_cast<int>(worldRuntime.GetWorld().blocksPerAxis());
        const int gx = static_cast<int>(std::lround(x)) + WorldRuntime::kWorldCenterX;
        const int gy = static_cast<int>(std::lround(y));
        const int gz = static_cast<int>(std::lround(z)) + WorldRuntime::kWorldCenterZ;
        if (gx < 0 || gy < 0 || gz < 0 || gx >= n || gy >= n || gz >= n)
        {
            return false;
        }
        return trajectoryCells_.count(static_cast<std::uint32_t>((gx * n + gy) * n + gz)) != 0;
    }

    void InteractionSystem::LoseLife()
    {
        // A death: with lives in reserve one of them is spent (the life-loss voyage's m_nbVies--,
        // Decor.cpp:10176) and he plays on, even with none left after it; a death with none left is
        // the game over (6393-6396), and DoorsLost() (11716) gives the new game its 3 again.
        if (lives_ > 0)
        {
            --lives_;
            return;
        }
        lives_ = 3;
        ++gameOverCount_;
    }

    bool InteractionSystem::PlaceDynamite(WorldRuntime& worldRuntime, float x, float y, float z, bool grounded,
                                             bool blupiCanUseHands)
    {
        // Real gate (Decor.cpp ~4792-4812): carrying at least one, and
        // solid ground under both feet -- approximated here as `grounded`
        // (this engine's single-point collision has no separate left/right
        // foot check). Real "not in any vehicle mode" clause fixed
        // 2026-07-16, and not on a lift (m_blupiTransport, BD-0027) -- both in
        // the caller's blupiCanUseHands.
        if (dynamiteCount_ <= 0 || !grounded || !blupiCanUseHands)
        {
            return false;
        }
        --dynamiteCount_;

        MobileObjSpec fuse;
        fuse.type = GalaxyEggbert::Def::ObjectType::ObjectType56;
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

    bool InteractionSystem::TryPerso(WorldRuntime& worldRuntime, float x, float y, float z, bool grounded,
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
            if (!obj.active || obj.type != GalaxyEggbert::Def::ObjectType::ObjectType200)
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
        // The decoy only when he carries no dynamite (the original's `else if (m_blupiPerso > 0)`
        // after the dynamite, Decor.cpp:4795-4818).
        if (persoCount_ <= 0 || dynamiteCount_ > 0 || !grounded || !blupiCanUseHands)
        {
            return false;
        }
        // Not beside a charger (Decor::MoveChargeDetect, Decor.cpp:4834, 9784-9812; BD-0051): its
        // box 10 px wider each side, the bottom 24 px of its cell, against his (16-44 across,
        // 11-58 down): 54 px across, from 22 px under him to 49 over him.
        for (const auto& obj : objects)
        {
            if (!obj.active || obj.type != GalaxyEggbert::Def::ObjectType::ObjectType31)
            {
                continue;
            }
            const float dy = obj.currentY - y;
            if (std::fabs(obj.currentX - x) < 54.0f / 64.0f && std::fabs(obj.currentZ - z) < 54.0f / 64.0f &&
                dy > -22.0f / 64.0f && dy < 49.0f / 64.0f)
            {
                return false;
            }
        }
        --persoCount_;

        MobileObjSpec decoy;
        decoy.type = GalaxyEggbert::Def::ObjectType::ObjectType200;
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

    bool InteractionSystem::CheatCleanAll(WorldRuntime& worldRuntime)
    {
        // Decor::CheatAction CleanAll (Decor.cpp:1793-1822): each enemy becomes an explosion
        // (ObjectType8) where it was, with channel 10 (the caller's) and the small shake; no
        // piece flies away (the type is changed, not ObjectDelete'd).
        bool anyDestroyed = false;
        std::vector<MobileObjSpec> explosions;
        for (auto& obj : worldRuntime.GetMobileObjectsMutable())
        {
            if (!obj.active)
            {
                continue;
            }
            if (GetObjectDefinition(obj.type).genericContactHazard || obj.type == GalaxyEggbert::Def::ObjectType::ObjectType32 ||
                obj.type == GalaxyEggbert::Def::ObjectType::ObjectType33 || obj.type == GalaxyEggbert::Def::ObjectType::ObjectType44 ||
                obj.type == GalaxyEggbert::Def::ObjectType::ObjectType54)
            {
                obj.active = false;
                anyDestroyed = true;
                AppendExplosionFlash(GalaxyEggbert::Def::ObjectType::ObjectType8, obj.currentX, obj.currentY, obj.currentZ,
                                     explosions);
            }
        }
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (const auto& explosion : explosions)
        {
            objects.push_back(explosion);
        }
        return anyDestroyed;
    }

    void InteractionSystem::SpawnInvertBurst(WorldRuntime& worldRuntime, float blupiX, float blupiY,
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
        const GalaxyEggbert::Def::ObjectType type = isGrant ? GalaxyEggbert::Def::ObjectType::ObjectType41 : GalaxyEggbert::Def::ObjectType::ObjectType42;

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

    void InteractionSystem::SpawnSawDeathBurst(WorldRuntime& worldRuntime, float blupiX, float blupiY,
                                                  float blupiZ, Sound& sound)
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
            spec.type = GalaxyEggbert::Def::ObjectType::ObjectType41;
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
        sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel75);
    }

    void InteractionSystem::SpawnWreck(WorldRuntime& worldRuntime, float x, float y, float z)
    {
        MobileObjSpec spec;
        spec.type = GalaxyEggbert::Def::ObjectType::ObjectType9;
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

    void InteractionSystem::ByeByeAdd(ByeByePiece piece, float rotationSpeed, float animationSpeed)
    {
        // Decor::ByeByeAdd (Decor.cpp:10058-10080): rotation from 0, sideways 10-19 px a tick
        // either way.
        piece.rotationDegrees = 0.0f;
        piece.phase = 0.0f;
        piece.rotationSpeed = rotationSpeed;
        piece.animationSpeed = animationSpeed;
        const int across = std::uniform_int_distribution<int>(0, 9)(rng_);
        const bool right = std::uniform_int_distribution<int>(0, 999)(rng_) % 2 == 0;
        piece.speedX = static_cast<float>(right ? across + 10 : -(across + 10));
        byeBye_.push_back(piece);
    }

    void InteractionSystem::Retire(MobileObjSpec& obj)
    {
        // Decor::ObjectDelete (Decor.cpp:7881-7916): the listed types fly away, rotating 17 a
        // tick (the bulldozer 7, the fish and the bird 33), the invert potion at half speed.
        using T = GalaxyEggbert::Def::ObjectType;
        obj.active = false;
        switch (obj.type)
        {
            case T::ObjectType4:
            case T::ObjectType12:
            case T::ObjectType16:
            case T::ObjectType17:
            case T::ObjectType20:
            case T::ObjectType40:
            case T::ObjectType96:
            case T::ObjectType97:
                break;
            default:
                return;
        }
        ByeByePiece piece;
        piece.look = obj;
        piece.look.active = true;
        piece.x = obj.currentX;
        piece.y = obj.currentY;
        piece.z = obj.currentZ;
        const float rotation = obj.type == T::ObjectType4                                  ? 7.0f
                               : (obj.type == T::ObjectType17 || obj.type == T::ObjectType20) ? 33.0f
                                                                                           : 17.0f;
        ByeByeAdd(piece, rotation, obj.type == T::ObjectType40 ? 0.5f : 1.0f);
    }

    void InteractionSystem::ByeByeHelicopter(float x, float y, float z)
    {
        ByeByePiece piece;
        piece.helicopter = true;
        piece.x = x;
        piece.y = y;
        piece.z = z;
        ByeByeAdd(piece, 7.0f, 0.5f);
    }

    void InteractionSystem::ByeByeFan(std::uint16_t icon, float x, float y, float z)
    {
        ByeByePiece piece;
        piece.fanIcon = icon;
        piece.x = x;
        piece.y = y;
        piece.z = z;
        ByeByeAdd(piece, 17.0f, 1.0f);
    }

    void InteractionSystem::TickByeBye(float dt)
    {
        // Decor::ByeByeStep (Decor.cpp:10082-10117), once a 20 Hz tick; pixels to cells, screen y
        // down to Galaxy's y up.
        byeByeTicks_ += dt * 20.0f;
        while (byeByeTicks_ >= 1.0f)
        {
            byeByeTicks_ -= 1.0f;
            for (std::size_t k = 0; k < byeBye_.size();)
            {
                auto& piece = byeBye_[k];
                const float speed = piece.animationSpeed;
                const float rest = 10.0f - piece.phase;
                if (rest > 0.0f)
                {
                    piece.y += std::pow(rest, 1.5f) * speed / 64.0f;
                }
                else if (rest < 0.0f)
                {
                    piece.y -= std::pow(-rest, 1.5f) * speed / 64.0f;
                }
                piece.x += piece.speedX * speed / 64.0f;
                if (piece.speedX > 0.0f)
                {
                    piece.speedX -= speed;
                }
                else if (piece.speedX < 0.0f)
                {
                    piece.speedX += speed;
                }
                piece.rotationDegrees += piece.rotationSpeed;
                piece.phase += speed;
                if (piece.phase > 30.0f)
                {
                    byeBye_.erase(byeBye_.begin() + static_cast<std::ptrdiff_t>(k));
                }
                else
                {
                    ++k;
                }
            }
        }
    }

    void InteractionSystem::SpawnEffect(WorldRuntime& worldRuntime, GalaxyEggbert::Def::ObjectType type, float x,
                                        float y, float z)
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

    void InteractionSystem::SpawnFanHitFlash(WorldRuntime& worldRuntime, float x, float y, float z)
    {
        MobileObjSpec spec;
        spec.type = GalaxyEggbert::Def::ObjectType::ObjectType11;
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

    void InteractionSystem::RespawnPickupItem(WorldRuntime& worldRuntime, float x, float y, float z,
                                                 GalaxyEggbert::Def::ObjectType type)
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

    void InteractionSystem::SpawnTeleportArc(WorldRuntime& worldRuntime, float x, float y, float z)
    {
        MobileObjSpec spec;
        spec.type = GalaxyEggbert::Def::ObjectType::ObjectType92;
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

    bool InteractionSystem::BlupiTouches(GalaxyEggbert::Def::ObjectType type, float dx, float dy, float dz)
    {
        return MoveObjectTouches(type, dx, dy, dz);
    }

    GalaxyEggbert::Def::BlupiAction InteractionSystem::MockeryDetect(const WorldRuntime& worldRuntime, float blupiX,
                                                                     float blupiY, float blupiZ, float yaw,
                                                                     bool airborne)
    {
        using A = GalaxyEggbert::Def::BlupiAction;
        using T = GalaxyEggbert::Def::ObjectType;
        const auto& world = worldRuntime.GetWorld();
        if (airborne)
        {
            // In the air, lava or a crusher in the column under his middle, 30 or 94 px under his
            // feet (his middle + 64 and + 128): Mockeryi. Galaxy's crushers stand in the walking
            // row as the original's do; its lava lies a row lower, as the floor (IsLave's floor
            // convention, GP-0447), so lava is looked for a cell further down.
            const int n = static_cast<int>(world.blocksPerAxis());
            const int gx = static_cast<int>(std::lround(blupiX)) + WorldRuntime::kWorldCenterX;
            const int gz = static_cast<int>(std::lround(blupiZ)) + WorldRuntime::kWorldCenterZ;
            const auto blockBelow = [&](float pixels) -> std::uint16_t
            {
                const int gy = static_cast<int>(std::floor(blupiY - pixels / 64.0f));
                if (gx < 0 || gz < 0 || gy < 0 || gx >= n || gy >= n || gz >= n)
                {
                    return 0;
                }
                return world
                    .getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                              static_cast<std::uint16_t>(gz))
                    .type();
            };
            for (const float below : {30.0f, 94.0f})
            {
                if (GalaxyEggbert::BlockTypes::isCrusher(blockBelow(below)) ||
                    GalaxyEggbert::GetBlockDefinition(blockBelow(below + 64.0f)).hazard ==
                        GalaxyEggbert::BlockHazardKind::Lava)
                {
                    return A::Mockeryi;
                }
            }
        }
        // The boxes: Blupi 60 px across, from 11 to 60 px under the top of his cell (to 150 in the
        // air); an enemy 60 across, the bottom 24 px of its cell (36-60). Across is X and Z alike.
        const float reach = 60.0f / 64.0f;
        const float blupiBottom = airborne ? 150.0f : 60.0f;
        const float fwdX = std::sin(yaw);
        const float fwdZ = -std::cos(yaw);
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (!obj.active)
            {
                continue;
            }
            switch (obj.type)
            {
                case T::ObjectType2:
                case T::ObjectType4:
                case T::ObjectType16:
                case T::ObjectType20:
                case T::ObjectType23:
                case T::ObjectType32:
                case T::ObjectType33:
                case T::ObjectType44:
                case T::ObjectType54:
                case T::ObjectType96:
                case T::ObjectType97:
                    break;
                default:
                    continue;
            }
            const float dx = obj.currentX - blupiX;
            const float dz = obj.currentZ - blupiZ;
            const float dyScreen = -(obj.currentY - blupiY) * 64.0f; // screen y grows downward
            if (std::fabs(dx) >= reach || std::fabs(dz) >= reach || !(36.0f + dyScreen < blupiBottom) ||
                !(60.0f + dyScreen > 11.0f))
            {
                continue;
            }
            if (obj.type == T::ObjectType54)
            {
                return A::Mockeryp;
            }
            // Ahead of him (the original: to the side he faces), the first such enemy decides: the
            // tracking one (2) calls for nothing, the search stops there too.
            if (fwdX * dx + fwdZ * dz > 0.0f)
            {
                return obj.type == T::ObjectType2 ? A::None : A::Mockery;
            }
            return A::Mockeryi;
        }
        return A::None;
    }

    void InteractionSystem::SpawnTeleportArrival(WorldRuntime& worldRuntime, float x, float y, float z)
    {
        // Decor.cpp:6355-6356: `ObjectStart(m_blupiPos, ObjectType27, 20)` and `(..., -20)` -- two
        // sparkle clusters sliding right and left from where he lands. ObjectStart sends each as far
        // as SearchDistRight reaches (Decor.cpp:7628: from his own cell to the first solid one) at
        // ScaleTime(|20 * dist / 64|) ticks for the distance, 20 ticks a cell; ObjectType27 lives
        // 24 ticks, so a cluster drifts about a cell. Screen right is world +X, as for the Invert
        // burst (SpawnInvertBurst).
        int gx = 0, gy = 0, gz = 0;
        ToGridCell(worldRuntime.GetWorld(), x, y, z, gx, gy, gz);
        auto& objects = worldRuntime.GetMobileObjectsMutable();
        for (const int dir : {1, -1})
        {
            const float cells = 1.0f + static_cast<float>(SearchAirDistance(worldRuntime.GetWorld(), gx, gy, gz, dir, 0, 0));
            MobileObjSpec spec;
            spec.type = GalaxyEggbert::Def::ObjectType::ObjectType27;
            spec.active = true;
            spec.phase = 0.0f;
            spec.currentX = spec.posStartX = x;
            spec.currentY = spec.posStartY = y;
            spec.currentZ = spec.posStartZ = z;
            spec.posEndX = x + static_cast<float>(dir) * cells;
            spec.posEndY = y;
            spec.posEndZ = z;
            spec.patrolStep = 2; // ObjectStart's step=2: straight out, no dwell at the start
            spec.patrolTime = 0.0f;
            spec.stepAdvanceTicks = 20.0f * cells;
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

    bool InteractionSystem::HasActiveObjectOfType(const WorldRuntime& worldRuntime, GalaxyEggbert::Def::ObjectType type) const
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

    void InteractionSystem::SpawnWaterSplash(WorldRuntime& worldRuntime, GalaxyEggbert::Def::ObjectType type, float x, float y,
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

    void InteractionSystem::SpawnWaterBubble(WorldRuntime& worldRuntime, const Worlds::World& world, float x,
                                                float y, float z)
    {
        // Real `pos.Y -= 20` pre-offset (Decor.cpp:7037) before the column
        // scan -- a small upward nudge, less than one full 64px tile.
        constexpr float kPreOffset = 20.0f / 64.0f;
        const float startY = y + kPreOffset;

        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int gx = static_cast<int>(std::lround(x)) + WorldRuntime::kWorldCenterX;
        const int gz = static_cast<int>(std::lround(z)) + WorldRuntime::kWorldCenterZ;
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
        spec.type = GalaxyEggbert::Def::ObjectType::ObjectType15;
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

    void InteractionSystem::TickPollutionPuff(WorldRuntime& worldRuntime, float blupiX, float blupiY,
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
        if (facingDX >= 0) // real GalaxyEggbert::Def::Direction::Right
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
        // flat 500px reach for this GalaxyEggbert::Def::ObjectType (`Decor::SearchDistRight()`,
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
        spec.type = GalaxyEggbert::Def::ObjectType::ObjectType36;
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

    void InteractionSystem::ResetMagicTrail(float x, float y, float z)
    {
        magicTrailLastX_ = x;
        magicTrailLastY_ = y;
        magicTrailLastZ_ = z;
    }

    void InteractionSystem::TickMagicTrail(WorldRuntime& worldRuntime, float blupiX, float blupiY, float blupiZ,
                                              bool isShielded, bool isPowered)
    {
        if (!isShielded && !isPowered)
        {
            return;
        }

        // The original's Manhattan distance (Decor.cpp:5204-5237), over all three axes in 3D, so
        // walking along Z leaves a trail too.
        constexpr float kThreshold = 40.0f / 64.0f;
        const float dist = std::fabs(blupiX - magicTrailLastX_) + std::fabs(blupiY - magicTrailLastY_) +
                           std::fabs(blupiZ - magicTrailLastZ_);
        if (dist < kThreshold)
        {
            return;
        }

        MobileObjSpec spec;
        spec.type = isShielded ? GalaxyEggbert::Def::ObjectType::ObjectType57 : GalaxyEggbert::Def::ObjectType::ObjectType27;
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

    void InteractionSystem::ReplaceEvent(EventKind kind, Event newEvent)
    {
        events_.erase(std::remove_if(events_.begin(), events_.end(),
                                      [kind](const Event& event) { return event.kind == kind; }),
                      events_.end());
        events_.push_back(newEvent);
    }

    void InteractionSystem::RequestVoyage(VoyageKind kind, int iconId, bool isButtonChannel, float worldX,
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

    void InteractionSystem::RequestClear2Ascend(float worldX, float worldY, float worldZ)
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

    void InteractionSystem::TryGrantSecretPowerPickup(MobileObjSpec& obj, bool blupiCanGrantShield,
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
                Retire(obj); // the invert potion flies away (ObjectDelete, BD-0049)
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

    void InteractionSystem::TryCollectBasicPickup(MobileObjSpec& obj, std::vector<MobileObjSpec>& pendingSpawns)
    {
        for (const auto& handler : kBasicPickupHandlers)
        {
            if (handler.type != obj.type)
            {
                continue;
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

    void InteractionSystem::BeginVoyage(WorldRuntime& worldRuntime, VoyageKind kind, int iconId,
                                           bool isButtonChannel, float startX, float startY, float endX, float endY,
                                           Sound& sound, float worldAnchorX, float worldAnchorY,
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
            case InteractionSystem::VoyageKind::Treasure:
            {
                const bool completesSet = (treasuresCollected_ + 1 >= totalTreasures_);
                sound.Play(completesSet ? GalaxyEggbert::Def::SoundChannel::SoundChannel19
                                        : GalaxyEggbert::Def::SoundChannel::SoundChannel11);
                break;
            }
            case InteractionSystem::VoyageKind::Key1:
            case InteractionSystem::VoyageKind::Key2:
            case InteractionSystem::VoyageKind::Key3:
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel11);
                break;
            case VoyageKind::Egg:
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel12);
                break;
            case VoyageKind::Perso:
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel60);
                break;
            case InteractionSystem::VoyageKind::Dynamite:
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel60);
                break;
            case VoyageKind::BulletPack:
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel54);
                break;
            case VoyageKind::Clear2Ascend:
            case VoyageKind::Clear3Ascend:
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel74);
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
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel9);
                break;
            // DoorUnlock: real source plays no immediate sound at all --
            // its dynamic icon never matches any of VoyageInit's
            // fixed-icon checks (confirmed via direct source read,
            // Decor.cpp:10141-10226).
            default:
                break;
        }
    }

    void InteractionSystem::TickVoyage(float dt, WorldRuntime& worldRuntime, Sound& sound)
    {
        if (voyageKind_ == VoyageKind::None)
        {
            return;
        }
        const int tickBefore = static_cast<int>(voyagePhase_);
        voyagePhase_ += dt * 20.0f; // same dt*20 real-20Hz-tick convention as AdvancePatrolStep
        // Real icon-cycle animation (`Decor::VoyageStep`'s own `if
        // (m_voyagePhase < m_voyageTotal) { if (m_time % ScaleTime(2) == 0
        // && icon in [230,241]) icon++ }`, Decor.cpp:10241-10252) -- uses
        // this voyage's own phase as the tick source (ScaleTime(2)==2 at
        // this build's reference rate), since this engine has no single
        // global `m_time` frame counter to match exactly. Once per even tick reached, not every
        // frame while the tick is even (that turned the angel about three times too fast at 60
        // frames a second; OR-0047).
        if (voyageKind_ == VoyageKind::Clear2Ascend && voyagePhase_ < voyageTotal_)
        {
            for (int tick = tickBefore + 1; tick <= static_cast<int>(voyagePhase_); ++tick)
            {
                if (tick % 2 == 0 && ++voyageIconId_ > 241)
                {
                    voyageIconId_ = 230;
                }
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

    void InteractionSystem::SpawnLavaAscendPuff(WorldRuntime& worldRuntime)
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
        spec.type = GalaxyEggbert::Def::ObjectType::ObjectType93;
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

    bool InteractionSystem::RollClear2Coinflip()
    {
        std::uniform_int_distribution<int> coinDist(0, 1);
        return coinDist(rng_) == 1;
    }

    void InteractionSystem::ApplyVoyageReward(WorldRuntime& worldRuntime, Sound& sound)
    {
        // Real `VoyageStep()`'s completion branch (`Decor.cpp:10254-10306`)
        // -- applies the real reward, then plays the shared "reward
        // applied" sound (channel 3) for every kind EXCEPT DoorUnlock
        // (whose dynamic icon never matches any of VoyageStep's own
        // fixed-icon checks, so real completion is silent for it).
        switch (voyageKind_)
        {
            case InteractionSystem::VoyageKind::Treasure:
                ++treasuresCollected_;
                ScanAndOpenTreasureDoors(worldRuntime, treasuresCollected_, sound);
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case InteractionSystem::VoyageKind::Key1:
                keys1_ = 1; // held, not counted (m_blupiCle |= Key1, Decor.cpp:10278)
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case InteractionSystem::VoyageKind::Key2:
                keys2_ = 1; // held, not counted (m_blupiCle |= Key2, Decor.cpp:10283)
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case InteractionSystem::VoyageKind::Key3:
                keys3_ = 1; // held, not counted (m_blupiCle |= Key3, Decor.cpp:10288)
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::Egg:
                // m_nbVies++ while under MAX_EGG_COUNT (Decor.cpp:10264-10267).
                if (lives_ < kMaxEggCount)
                {
                    ++lives_;
                }
                ++lifeEggCount_;
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case InteractionSystem::VoyageKind::Dynamite:
                ++dynamiteCount_;
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::Perso:
                ++persoCount_;
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::BulletPack:
                // Real reward already applied immediately at touch time
                // (matches this engine's own bulletCount_ handling) --
                // only the deferred completion sound fires here.
                sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel3);
                break;
            case VoyageKind::DoorUnlock:
            // Clear2Ascend/Clear3Ascend: real completion is silent with no
            // reward -- neither icon 230 nor 40 appears in VoyageStep's
            // own completion if-chain (confirmed via direct source read).
            case VoyageKind::Clear2Ascend:
            case VoyageKind::Clear3Ascend:
            // LifeLoss: real completion (`Stop`/`m_blupiFocus=true`, unfreezing Blupi) is handled
            // by BlupiController's own independent life-loss-Voyage timer (see
            // TriggerDeathLock()'s own comment) -- the effect (LoseLife()) already fired at START
            // (BeginVoyage()'s own LifeLoss case), so completion here is a pure no-op.
            case VoyageKind::LifeLoss:
            case VoyageKind::None:
            default:
                break;
        }
        voyageKind_ = VoyageKind::None;
    }

    float InteractionSystem::VoyageDrawX() const noexcept
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

    float InteractionSystem::VoyageDrawY() const noexcept
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

    bool InteractionSystem::VoyageIconVisible() const noexcept
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

    void InteractionSystem::CheatAllTreasure(WorldRuntime& worldRuntime, Sound& sound)
    {
        bool anyCollected = false;
        for (auto& obj : worldRuntime.GetMobileObjectsMutable())
        {
            if (obj.active && obj.type == GalaxyEggbert::Def::ObjectType::ObjectType5)
            {
                obj.active = false;
                ++treasuresCollected_;
                anyCollected = true;
            }
        }
        if (anyCollected)
        {
            sound.Play(GalaxyEggbert::Def::SoundChannel::SoundChannel11);
            ScanAndOpenTreasureDoors(worldRuntime, treasuresCollected_, sound);
        }
    }

    bool InteractionSystem::CheatFindExit(const WorldRuntime& worldRuntime, float& outX, float& outY,
                                             float& outZ) const
    {
        for (const auto& obj : worldRuntime.GetMobileObjects())
        {
            if (obj.active && obj.type == GalaxyEggbert::Def::ObjectType::ObjectType7)
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
