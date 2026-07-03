#include "GEDecorSystem.hpp"
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <cmath>

using namespace Simple3D;
using namespace GalaxyEggbert;

namespace GESimple3D {

// Ported from Decor.cpp GetIcon(). element.png icon index per ObjectType + animation phase.
int GEDecorSystem::GetObjIcon(ObjectType type, int p) {
    static const int kCle1[12]    = {209,210,211,212,213,214,215,214,213,212,211,210};
    static const int kCle2[12]    = {220,221,222,221,220,219,218,217,216,217,218,219};
    static const int kCle3[12]    = {229,228,227,226,225,224,223,224,225,226,227,228};
    static const int kShield[8]   = {144,145,146,147,148,149,150,151};
    static const int kBulldozer[8]= {66,66,67,67,66,66,65,65};
    static const int kBird[8]     = {98,99,100,101,102,103,104,105};
    static const int kFish[8]     = {82,82,81,81,82,82,83,83};
    static const int kBlupit[8]   = {249,249,250,250,249,249,248,248};
    // Tables below are transcribed from mobile-eggbert's Tables.cpp
    // (table_guepe_left, table_creature_left, table_blupih_left,
    // table_follow1, table_chenille, table_cle, table_skate, table_power,
    // table_invert) for the 12 previously-unspawned ObjectType values
    // (mobile-eggbert-2d-reference.md §2.4). Simplified from mobile-eggbert's
    // real 4-state turn/walk step machine (MoveObjectStepIcon) to a single
    // continuous phase-indexed cycle, matching how this function already
    // simplifies the other patrol enemies above (kBird/kFish/kBlupit) —
    // only the "left" frame table is used, direction comes from SetFlipX2D
    // in Update() like the existing enemies, not a separate mirrored table.
    static const int kGuepeLeft[6]    = {195,196,197,198,197,196};
    static const int kCreature[8]     = {247,248,249,250,251,250,249,248}; // same for both directions in source
    static const int kBlupihLeft[8]   = {66,67,68,67,66,69,70,69};
    static const int kFollow1[26]     = {256,256,256,257,257,258,259,260,261,262,
                                          263,264,264,265,265,265,264,264,263,262,
                                          261,260,259,258,257,257};
    static const int kChenille[6]     = {311,312,313,314,315,316};
    static const int kCleGeneric[12]  = {122,123,124,125,126,127,128,127,126,125,124,123};
    static const int kSkate[34]       = {129,129,129,129,130,130,130,131,131,132,
                                          132,133,133,134,134,134,135,135,135,135,
                                          134,134,134,133,133,132,132,131,131,131,
                                          130,130,130,130};
    static const int kPower[8]        = {136,137,138,139,140,141,142,143};
    static const int kInvert[20]      = {187,187,187,188,189,190,191,192,193,194,
                                          187,187,187,194,193,192,191,190,189,188};
    switch (type) {
        case ObjectType::ObjectType1:  return 29;
        case ObjectType::ObjectType2:  return 12 + (p / 6) % 9;
        case ObjectType::ObjectType3:  return 48 + (p / 6) % 9;
        case ObjectType::ObjectType4:  return kBulldozer[(p / 9) % 8];
        case ObjectType::ObjectType12: return 32;
        case ObjectType::ObjectType13: return 68;
        case ObjectType::ObjectType16: return 69 + (p / 3) % 9;
        case ObjectType::ObjectType17: return kFish[(p / 6) % 8];
        case ObjectType::ObjectType20: return kBird[(p / 6) % 8];
        case ObjectType::ObjectType30: return 178;
        case ObjectType::ObjectType33: return kBlupit[(p / 6) % 8];
        case ObjectType::ObjectType5: { int q = (p / 9) % 22; return (q < 11) ? q : (21 - q); }
        case ObjectType::ObjectType6:  return 21 + (p / 12) % 8;
        case ObjectType::ObjectType7:  return 29 + (p /  9) % 8;
        case ObjectType::ObjectType49: return kCle1[(p / 9) % 12];
        case ObjectType::ObjectType50: return kCle2[(p / 9) % 12];
        case ObjectType::ObjectType51: return kCle3[(p / 9) % 12];
        case ObjectType::ObjectType25: return kShield[(p / 6) % 8];
        // Static single-icon pickups (Decor.cpp MoveObjectStepIcon sets a
        // constant icon for these, no animation).
        case ObjectType::ObjectType19: return 89;  // jeep
        case ObjectType::ObjectType46: return 208; // balloon
        case ObjectType::ObjectType55: return 252; // dynamite (idle icon; fuse/explosion not implemented)
        // Animated pickups/power-ups.
        case ObjectType::ObjectType21: return kCleGeneric[(p / 9) % 12];   // secret-level exit
        case ObjectType::ObjectType24: return kSkate[(p / 3) % 34];        // skateboard
        case ObjectType::ObjectType26: return kPower[(p / 6) % 8];         // suction-cup
        case ObjectType::ObjectType40: return kInvert[(p / 4) % 20];       // mirror/invert
        // Platform lift with a moving-track texture (table_chenille), same
        // motion/collision category as ObjectType1 (see IsPlatform()).
        case ObjectType::ObjectType47: return kChenille[(p / 6) % 6];
        // Patrol enemies. Direction is conveyed by SetFlipX2D (see Update()),
        // matching how kBird/kFish/kBlupit already work — so only the "left"
        // frame table is needed here.
        case ObjectType::ObjectType32: return kBlupihLeft[(p / 6) % 8];  // blupih
        case ObjectType::ObjectType44: return kGuepeLeft[(p / 6) % 6];   // wasp/bee
        case ObjectType::ObjectType54: return kCreature[(p / 6) % 8];    // large creature
        // Follower: idle table while dormant, alert table once awake
        // (GEDecorSystem::Update() drives the phase differently per state —
        // see ObjState::followerAwake). GetObjIcon can't see that flag (it's
        // per-instance state, not derivable from type+phase alone), so this
        // default covers the common dormant case only; Update() special-cases
        // the awake animation via a distinct helper.
        case ObjectType::ObjectType96: return kFollow1[(p / 3) % 26];
        default:                       return 0;
    }
}

// Follower (96) icon while awake/homing — separate from GetObjIcon() because
// the awake/dormant animation choice depends on ObjState::followerAwake, not
// just (type, phase).
static int GetFollowerAwakeIcon(int p) {
    static const int kFollow2[5] = {256,258,260,262,264};
    return kFollow2[(p / 6) % 5];
}

static bool IsPickup(ObjectType t) {
    return t == ObjectType::ObjectType5  ||  // treasure
           t == ObjectType::ObjectType6  ||  // egg
           t == ObjectType::ObjectType7  ||  // exit
           t == ObjectType::ObjectType13 ||  // helicopter
           t == ObjectType::ObjectType25 ||  // shield
           t == ObjectType::ObjectType30 ||  // drink
           t == ObjectType::ObjectType49 ||  // key red
           t == ObjectType::ObjectType50 ||  // key green
           t == ObjectType::ObjectType51 ||  // key blue
           t == ObjectType::ObjectType19 ||  // jeep
           t == ObjectType::ObjectType21 ||  // secret-level exit
           t == ObjectType::ObjectType24 ||  // skateboard
           t == ObjectType::ObjectType26 ||  // suction-cup
           t == ObjectType::ObjectType40 ||  // mirror/invert
           t == ObjectType::ObjectType46 ||  // balloon
           t == ObjectType::ObjectType55;    // dynamite
}

static bool IsEnemy(ObjectType t) {
    return t == ObjectType::ObjectType2  ||
           t == ObjectType::ObjectType3  ||
           t == ObjectType::ObjectType4  ||
           t == ObjectType::ObjectType16 ||
           t == ObjectType::ObjectType17 ||
           t == ObjectType::ObjectType20 ||
           t == ObjectType::ObjectType33 ||
           t == ObjectType::ObjectType32 ||  // blupih
           t == ObjectType::ObjectType44 ||  // wasp/bee
           t == ObjectType::ObjectType54 ||  // large creature
           t == ObjectType::ObjectType96;    // follower (dormant until awake)
}

static bool IsPlatform(ObjectType t) {
    return t == ObjectType::ObjectType1 ||
           t == ObjectType::ObjectType47; // platform lift with track animation
}

// Billboard world size matches element.png tile: 60 px at 64 px/world-unit.
static constexpr float kElemWorldSize = GEDecorSystem::kElemTilePx / 64.0f;
// Sprite child vertical offset: same calculation as ObjectNode (kVisHalf - 0.5)
static constexpr float kSpriteOffY = kElemWorldSize * 0.5f - 0.5f;

static Entity* MakeSprite(Game& game, Entity* parent, const std::string& name, ObjectType type) {
    auto* s = parent->CreateChild(name + "_spr");
    s->SetLocalPosition(Simple3D::Vector3(0.0f, kSpriteOffY, 0.0f));
    s->AddBillboard("icons/element.png", kElemWorldSize);
    int icon = GEDecorSystem::GetObjIcon(type, 0);
    int col  = icon % GEDecorSystem::kElemCols;
    int row  = icon / GEDecorSystem::kElemCols;
    s->SetBillboardUVRect(col * GEDecorSystem::kElemTilePx,
                          row * GEDecorSystem::kElemTilePx,
                          GEDecorSystem::kElemTilePx,
                          GEDecorSystem::kElemTilePx);
    return s;
}

static constexpr int kWCX = GEWorldRuntime::kWCX;
static constexpr int kWCZ = GEWorldRuntime::kWCZ;

void GEDecorSystem::Build(Game& game, const GEWorldRuntime& world, Entity* player) {
    Clear(game);
    world_ = world.GetWorld();

    totalTreasures_ = 0;
    int idx = 0;

    for (const auto& spec : world.GetMobileObjects()) {
        ObjState st;
        st.type     = spec.type;
        st.posStart = spec.posStart;
        st.posEnd   = spec.posEnd;
        st.speed    = spec.speed;
        st.name     = "Obj" + std::to_string(idx++);

        auto* e = game.CreateEntity(st.name);
        e->SetPosition(spec.posStart);

        if (IsPickup(spec.type)) {
            st.sprite = MakeSprite(game, e, st.name, spec.type);
            e->AddTriggerSphere(0.7f);
            e->SetCollisionMask(CollisionLayer::Actor);

            if (spec.type == ObjectType::ObjectType5) {
                ++totalTreasures_;
                std::string name = st.name;
                e->SetOnTriggerEnter([this, name, player](Entity* other) {
                    if (!other || other != player) return;
                    if (collected_set_.count(name)) return;
                    collected_set_.insert(name);
                    ++collected_;
                });
            } else if (spec.type == ObjectType::ObjectType7) {
                std::string name = st.name;
                e->SetOnTriggerEnter([this, name, player](Entity* other) {
                    if (!other || other != player) return;
                    if (collected_set_.count(name)) return;
                    collected_set_.insert(name);
                    exitReached_ = true;
                });
            } else if (spec.type == ObjectType::ObjectType6) {
                std::string name = st.name;
                e->SetOnTriggerEnter([this, name, player, &game, e](Entity* other) {
                    if (!other || other != player) return;
                    if (collected_set_.count(name)) return;
                    collected_set_.insert(name);
                    eggCollected_ = true;
                    game.DestroyEntity(e);
                });
            } else if (spec.type == ObjectType::ObjectType30) {
                std::string name = st.name;
                e->SetOnTriggerEnter([this, name, player, &game, e](Entity* other) {
                    if (!other || other != player) return;
                    if (collected_set_.count(name)) return;
                    collected_set_.insert(name);
                    drinkCollected_ = true;
                    game.DestroyEntity(e);
                });
            } else if (spec.type == ObjectType::ObjectType25) {
                std::string name = st.name;
                e->SetOnTriggerEnter([this, name, player, &game, e](Entity* other) {
                    if (!other || other != player) return;
                    if (collected_set_.count(name)) return;
                    collected_set_.insert(name);
                    shieldCollected_ = true;
                    game.DestroyEntity(e);
                });
            } else if (spec.type == ObjectType::ObjectType49 ||
                       spec.type == ObjectType::ObjectType50 ||
                       spec.type == ObjectType::ObjectType51) {
                ObjectType keyType = spec.type;
                std::string name = st.name;
                e->SetOnTriggerEnter([this, keyType, name, player, &game, e](Entity* other) {
                    if (!other || other != player) return;
                    if (collected_set_.count(name)) return;
                    collected_set_.insert(name);
                    if (keyType == ObjectType::ObjectType49) ++keys49_;
                    if (keyType == ObjectType::ObjectType50) ++keys50_;
                    if (keyType == ObjectType::ObjectType51) ++keys51_;
                    game.DestroyEntity(e);
                });
            }
        } else if (IsPlatform(spec.type) || IsEnemy(spec.type)) {
            st.sprite = MakeSprite(game, e, st.name, spec.type);
        } else if (spec.type == ObjectType::ObjectType12) {
            st.sprite = MakeSprite(game, e, st.name, spec.type);
        }

        st.entity = e;
        objects_.push_back(std::move(st));
    }
}

void GEDecorSystem::Clear(Game& game) {
    for (auto& st : objects_) {
        if (st.entity) game.DestroyEntity(st.entity);
    }
    objects_.clear();
    collected_set_.clear();
    collected_ = 0; totalTreasures_ = 0;
    keys49_ = keys50_ = keys51_ = 0;
    world_ = nullptr;
    ClearEvents();
}

void GEDecorSystem::Update(float dt, const Vector3& blupiPos,
                            float blupiVelY, float blupiVelX) {
    // ClearEvents() is called by the game loop AFTER it has processed events,
    // not here, so the caller can read them after Update() returns.

    for (auto& st : objects_) {
        if (!st.entity || !st.active) continue;

        // Advance animation phase (capped at large value to avoid overflow)
        st.animPhase = (st.animPhase + 1) % 10000;

        // Update billboard UV
        if (st.sprite) {
            int icon = (st.type == ObjectType::ObjectType96 && st.followerAwake)
                           ? GetFollowerAwakeIcon(st.animPhase)
                           : GetObjIcon(st.type, st.animPhase);
            int col  = icon % kElemCols;
            int row  = icon / kElemCols;
            st.sprite->SetBillboardUVRect(col * kElemTilePx, row * kElemTilePx,
                                          kElemTilePx, kElemTilePx);

            if (IsEnemy(st.type)) {
                bool facingRight = (st.direction > 0.0f) == (st.posEnd.x_ >= st.posStart.x_);
                st.sprite->SetFlipX2D(facingRight);
            }
        }

        if (IsPlatform(st.type) || IsEnemy(st.type)) {
            if (st.type == ObjectType::ObjectType96) {
                // Follower (mobile-eggbert-2d-reference.md §5, §2.4): dormant
                // and stationary until Blupi comes within range (mobile-eggbert's
                // MoveObjectFollow uses a ~100px/64 ≈ 1.56-unit padded box;
                // approximated here as a simple radius check), then moves
                // directly toward Blupi every frame instead of patrolling
                // posStart<->posEnd. Decor.cpp's real ObjectType97 homing
                // logic advances 1 raw pixel per tick toward Blupi — approximated
                // here as continuous speed-based motion, consistent with how
                // this function already simplifies the other enemies' movement.
                Vector3 pos = st.entity->GetPosition();
                float dx0 = pos.x_ - blupiPos.x_;
                float dz0 = pos.z_ - blupiPos.z_;
                if (!st.followerAwake && (dx0 * dx0 + dz0 * dz0) < 2.5f * 2.5f) {
                    st.followerAwake = true;
                }
                if (st.followerAwake) {
                    Vector3 diff = blupiPos - pos;
                    diff.y_ = 0.0f;
                    float dist = std::sqrt(diff.x_ * diff.x_ + diff.z_ * diff.z_);
                    if (dist > 0.05f) {
                        Vector3 move = diff * (st.speed * dt / std::max(dist, 0.001f));
                        st.entity->SetPosition(pos + move);
                    }
                }
            } else {
                Vector3 pos = st.entity->GetPosition();
                Vector3 target = (st.direction > 0.0f) ? st.posEnd : st.posStart;
                Vector3 diff = target - pos;
                float dist = std::sqrt(diff.x_ * diff.x_ + diff.y_ * diff.y_ + diff.z_ * diff.z_);
                if (dist < 0.05f) {
                    st.direction = -st.direction;
                } else {
                    Vector3 move = diff * (st.speed * dt / std::max(dist, 0.001f));
                    st.entity->SetPosition(pos + move);
                }
            }

            if (IsEnemy(st.type)) {
                Vector3 ep = st.entity->GetPosition();
                float dx = ep.x_ - blupiPos.x_;
                float dy = ep.y_ - blupiPos.y_;
                float dz = ep.z_ - blupiPos.z_;
                float d2 = dx*dx + dy*dy + dz*dz;
                if (d2 < 0.9f * 0.9f) {
                    if (blupiVelY < -1.0f && blupiPos.y_ > ep.y_ + 0.3f) {
                        stompKill_ = true;
                        stompPos_  = ep;
                        st.active = false;
                        st.entity->SetPosition(Vector3(0.0f, -999.0f, 0.0f));
                    } else {
                        blupiHit_ = true;
                    }
                }
            }
        }

        // Crate push (ObjectType12) — faithful to mobile-eggbert TestPushCaisse.
        // Blupi must be adjacent in X (same Z lane) and moving toward the crate.
        // Floor support check: destination world tile at y=0 must be non-Air so the
        // crate doesn't get pushed off a cliff into empty space.
        if (st.type == ObjectType::ObjectType12) {
            st.pushCooldown = std::max(0.0f, st.pushCooldown - dt);

            if (st.pushCooldown <= 0.0f && world_) {
                Vector3 pos = st.entity->GetPosition();
                float relX = pos.x_ - blupiPos.x_;
                float relZ = pos.z_ - blupiPos.z_;
                float distX = std::abs(relX);
                float distZ = std::abs(relZ);

                // Blupi in same Z lane, adjacent tile in X, and moving toward crate.
                if (distZ < 0.6f && distX > 0.3f && distX < 1.1f) {
                    float pushDir = (relX > 0.0f) ? 1.0f : -1.0f;
                    if (blupiVelX * pushDir > 0.1f) {
                        float destX = pos.x_ + pushDir;
                        int wx = static_cast<int>(std::round(destX)) + kWCX;
                        int wz = static_cast<int>(std::round(pos.z_)) + kWCZ;

                        if (wx >= 0 && wx < 100 && wz >= 0 && wz < 100) {
                            using namespace GalaxyEggbert::Worlds;
                            bool hasFloor = !world_->getBlock(
                                static_cast<uint16_t>(wx), 0,
                                static_cast<uint16_t>(wz)).isAir();
                            if (hasFloor) {
                                bool occupied = false;
                                for (auto& other : objects_) {
                                    if (&other == &st || other.type != ObjectType::ObjectType12
                                        || !other.entity) continue;
                                    Vector3 op = other.entity->GetPosition();
                                    if (std::abs(op.x_ - destX) < 0.5f &&
                                        std::abs(op.z_ - pos.z_) < 0.5f) {
                                        occupied = true; break;
                                    }
                                }
                                if (!occupied) {
                                    st.entity->SetPosition(Vector3(destX, pos.y_, pos.z_));
                                    st.posStart.x_ += pushDir;
                                    st.posEnd.x_   += pushDir;
                                    st.pushCooldown = 0.4f;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace GESimple3D
