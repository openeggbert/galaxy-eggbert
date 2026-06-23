#include "GEDecorSystem.hpp"
#include <cmath>

using namespace Simple3D;
using namespace GalaxyEggbert;

namespace GESimple3D {

static bool IsPickup(ObjectType t) {
    return t == ObjectType::ObjectType5  ||  // treasure
           t == ObjectType::ObjectType6  ||  // egg
           t == ObjectType::ObjectType7  ||  // exit
           t == ObjectType::ObjectType13 ||  // helicopter
           t == ObjectType::ObjectType25 ||  // shield
           t == ObjectType::ObjectType30 ||  // drink
           t == ObjectType::ObjectType49 ||  // key red
           t == ObjectType::ObjectType50 ||  // key green
           t == ObjectType::ObjectType51;    // key blue
}

static bool IsEnemy(ObjectType t) {
    return t == ObjectType::ObjectType2  ||
           t == ObjectType::ObjectType3  ||
           t == ObjectType::ObjectType4  ||
           t == ObjectType::ObjectType16 ||
           t == ObjectType::ObjectType17 ||
           t == ObjectType::ObjectType20 ||
           t == ObjectType::ObjectType33;
}

static bool IsPlatform(ObjectType t) {
    return t == ObjectType::ObjectType1;
}

void GEDecorSystem::Build(Game& game, const GEWorldRuntime& world, Entity* player) {
    Clear(game);

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
            // Small trigger sphere for collection; visual is a tiny box.
            e->SetScale(0.5f);
            e->AddModel("Models/Box.mdl");
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
            // Placeholder box — no texture or sprite yet (S3D-4).
            // TODO(S3D-4): Add billboard sprite from element.png for enemies.
            e->SetScale(0.8f);
            e->AddModel("Models/Box.mdl");
            if (IsEnemy(spec.type)) {
                // Proximity check handled in Update(); no physics body for enemy boxes.
                // TODO: Add trigger box and stomp detection via velY.
            }
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
    ClearEvents();
}

void GEDecorSystem::Update(float dt, const Vector3& blupiPos, float blupiVelY) {
    // ClearEvents() is called by the game loop AFTER it has processed events,
    // not here, so the caller can read them after Update() returns.

    for (auto& st : objects_) {
        if (!st.entity || !st.active) continue;

        // Basic patrol movement for platforms and enemies.
        // TODO(S3D-4): Port full patrol + spider oscillation logic.
        if (IsPlatform(st.type) || IsEnemy(st.type)) {
            Vector3 pos = st.entity->GetPosition();
            Vector3 target = (st.direction > 0.0f) ? st.posEnd : st.posStart;
            Vector3 diff = target - pos;
            float dist = std::sqrt(diff.x_ * diff.x_ + diff.z_ * diff.z_);
            if (dist < 0.05f) {
                st.direction = -st.direction;
            } else {
                Vector3 move = diff * (st.speed * dt / std::max(dist, 0.001f));
                st.entity->SetPosition(pos + move);
            }

            // Simple proximity enemy-hit check (no stomp detection yet).
            if (IsEnemy(st.type)) {
                Vector3 ep = st.entity->GetPosition();
                float dx = ep.x_ - blupiPos.x_;
                float dy = ep.y_ - blupiPos.y_;
                float dz = ep.z_ - blupiPos.z_;
                float d2 = dx*dx + dy*dy + dz*dz;
                if (d2 < 0.9f * 0.9f) {
                    // Stomp: Blupi falling onto enemy
                    if (blupiVelY < -1.0f && blupiPos.y_ > ep.y_ + 0.3f) {
                        stompKill_ = true;
                        st.active = false;
                        st.entity->SetPosition(Vector3(0.0f, -999.0f, 0.0f)); // hide
                    } else {
                        blupiHit_ = true;
                    }
                }
            }
        }
    }
}

} // namespace GESimple3D
