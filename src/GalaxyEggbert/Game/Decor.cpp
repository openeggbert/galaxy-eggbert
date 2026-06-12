#include "Decor.hpp"
#include <cmath>

using namespace Urho3D;
using namespace GalaxyEggbert;

Decor::Decor(Context* context, Scene* scene)
    : context_(context), scene_(scene) {}

void Decor::PlaceObject(ObjectType type, Vector3 pos, Vector3 posEnd, float speed) {
    if (objCount_ >= kMaxObjects) return;
    Object& obj   = objects_[objCount_++];
    obj.type      = type;
    obj.pos       = pos;
    obj.posStart  = pos;
    obj.posEnd    = (posEnd.y_ == -999.0f) ? pos : posEnd;
    obj.speed     = speed;
    obj.direction = 1;
    obj.animPhase = 0;
    obj.active    = true;
    obj.node      = std::make_unique<ObjectNode>(context_, scene_);
    obj.node->SetPosition(pos);
}

// Derived from mobile-eggbert Decor.cpp MoveObjectStepIcon().
// ScaleDiv(N) at 60 fps = N*3 (original game ran at 20 fps).
int Decor::GetIcon(const Object& obj) const {
    int p = obj.animPhase;
    switch (obj.type) {
        case ObjectType::ObjectType2: return 12 + (p / 6) % 9;    // enemy: icons 12-20
        case ObjectType::ObjectType5: {                             // treasure: 0→10→0 bounce
            int q = (p / 9) % 22;
            return (q < 11) ? q : (21 - q);
        }
        case ObjectType::ObjectType6: return 21 + (p / 12) % 8;   // egg: icons 21-28
        case ObjectType::ObjectType7: return 29 + (p / 9) % 8;    // exit: icons 29-36
        default:                      return 0;
    }
}

// Linear patrol between posStart↔posEnd.
// Derived from mobile-eggbert Decor.cpp MoveObjectStepLine().
void Decor::StepMovement(Object& obj, float dt) {
    if (obj.posStart.x_ == obj.posEnd.x_ &&
        obj.posStart.z_ == obj.posEnd.z_) return; // stationary

    Vector3 target = (obj.direction > 0) ? obj.posEnd : obj.posStart;
    Vector3 delta  = target - obj.pos;
    delta.y_       = 0.0f;
    float dist     = delta.Length();
    float step     = obj.speed * dt;

    if (dist <= step) {
        obj.pos.x_    = target.x_;
        obj.pos.z_    = target.z_;
        obj.direction = -obj.direction;
    } else {
        obj.pos += delta.Normalized() * step;
    }
}

bool Decor::TouchesBlupi(const Object& obj, Vector3 blupiPos) const {
    float dx = obj.pos.x_ - blupiPos.x_;
    float dz = obj.pos.z_ - blupiPos.z_;
    return std::sqrt(dx * dx + dz * dz) < 0.85f;
}

void Decor::Update(float dt, Vector3 blupiPos) {
    exitReached_ = false;
    blupiHit_    = false;

    for (int i = 0; i < objCount_; ++i) {
        Object& obj = objects_[i];
        if (!obj.active) continue;

        StepMovement(obj, dt);
        ++obj.animPhase;

        obj.node->UpdateIcon(GetIcon(obj));
        obj.node->SetPosition(obj.pos);

        if (!TouchesBlupi(obj, blupiPos)) continue;

        switch (obj.type) {
            case ObjectType::ObjectType5:
            case ObjectType::ObjectType6:
                obj.active = false;
                obj.node->Remove();
                ++collected_;
                break;
            case ObjectType::ObjectType7:
                exitReached_ = true;
                break;
            case ObjectType::ObjectType2:
            case ObjectType::ObjectType3:
                blupiHit_ = true;
                break;
            default:
                break;
        }
    }
}
