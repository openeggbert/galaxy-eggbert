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
    if (type == ObjectType::ObjectType5) ++totalTreasures_;
}

// Derived from mobile-eggbert Decor.cpp MoveObjectStepIcon().
// ScaleDiv(N) at 60 fps = N*3 (original game ran at 20 fps).
int Decor::GetIcon(const Object& obj) const {
    int p = obj.animPhase;
    // Key animation tables (ported verbatim from mobile-eggbert Tables.cpp).
    static const int kCle1[12] = {209,210,211,212,213,214,215,214,213,212,211,210};
    static const int kCle2[12] = {220,221,222,221,220,219,218,217,216,217,218,219};
    static const int kCle3[12] = {229,228,227,226,225,224,223,224,225,226,227,228};
    // Shield first 8 frames only (icons 144-151); frames 9-16 are outside element.png bounds.
    static const int kShield[8] = {144,145,146,147,148,149,150,151};
    // Bulldozer (table_bulldozer_left): icons 65,66,67 cycling, phase / 9 at 60 fps.
    static const int kBulldozer[8] = {66,66,67,67,66,66,65,65};
    // Bird (table_oiseau_left): icons 98-105, phase / 6 at 60 fps.
    static const int kBird[8] = {98,99,100,101,102,103,104,105};
    // Fish (table_poisson_left): icons 81-83 wobble, phase / 6 at 60 fps.
    static const int kFish[8] = {82,82,81,81,82,82,83,83};
    // Blupit tank (table_blupit_left): icons 248-250 cycling, phase / 6 at 60 fps.
    static const int kBlupit[8] = {249,249,250,250,249,249,248,248};
    switch (obj.type) {
        case ObjectType::ObjectType1:  return 29;                      // platform: static
        case ObjectType::ObjectType2:  return 12 + (p / 6) % 9;       // enemy A: icons 12-20
        case ObjectType::ObjectType3:  return 48 + (p / 6) % 9;       // enemy B: icons 48-56
        case ObjectType::ObjectType4:  return kBulldozer[(p / 9) % 8]; // bulldozer
        case ObjectType::ObjectType12: return 32;                      // crate: static
        case ObjectType::ObjectType13: return 68;                      // helicopter: static
        case ObjectType::ObjectType16: return 69 + (p / 3) % 9;        // spider: icons 69-77
        case ObjectType::ObjectType17: return kFish[(p / 6) % 8];      // fish
        case ObjectType::ObjectType20: return kBird[(p / 6) % 8];      // bird
        case ObjectType::ObjectType30: return 178;                     // drink: static
        case ObjectType::ObjectType33: return kBlupit[(p / 6) % 8];   // blupit tank
        case ObjectType::ObjectType5: {                             // treasure: 0→10→0 bounce
            int q = (p / 9) % 22;
            return (q < 11) ? q : (21 - q);
        }
        case ObjectType::ObjectType6:  return 21 + (p / 12) % 8;  // egg:     icons 21-28
        case ObjectType::ObjectType7:  return 29 + (p /  9) % 8;  // exit:    icons 29-36
        case ObjectType::ObjectType49: return kCle1[(p / 9) % 12]; // red key
        case ObjectType::ObjectType50: return kCle2[(p / 9) % 12]; // green key
        case ObjectType::ObjectType51: return kCle3[(p / 9) % 12]; // blue key
        case ObjectType::ObjectType25: return kShield[(p / 6) % 8];// shield orb
        default:                       return 0;
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
    exitReached_  = false;
    blupiHit_     = false;
    platformDelta_ = Vector3::ZERO;

    for (int i = 0; i < objCount_; ++i) {
        Object& obj = objects_[i];
        if (!obj.active) continue;

        const Vector3 oldPos = obj.pos;
        StepMovement(obj, dt);
        ++obj.animPhase;

        // Platform carry: if Blupi is horizontally within 0.85 units and
        // within 1.5 units vertically, push Blupi with the platform's XZ delta.
        if (obj.type == ObjectType::ObjectType1) {
            Vector3 delta = obj.pos - oldPos;
            float dx = obj.pos.x_ - blupiPos.x_;
            float dz = obj.pos.z_ - blupiPos.z_;
            if (std::sqrt(dx*dx + dz*dz) < 0.85f &&
                std::abs(obj.pos.y_ - blupiPos.y_) < 1.5f) {
                platformDelta_.x_ += delta.x_;
                platformDelta_.z_ += delta.z_;
            }
        }

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
            case ObjectType::ObjectType49:
            case ObjectType::ObjectType50:
            case ObjectType::ObjectType51:
                obj.active = false;
                obj.node->Remove();
                ++keysCollected_;
                break;
            case ObjectType::ObjectType25:
                obj.active = false;
                obj.node->Remove();
                shieldCollected_ = true;
                break;
            case ObjectType::ObjectType13: // helicopter → shield (vehicle boarding placeholder)
                obj.active = false;
                obj.node->Remove();
                shieldCollected_ = true;
                break;
            case ObjectType::ObjectType30: // drink → collectible
                obj.active = false;
                obj.node->Remove();
                ++collected_;
                break;
            case ObjectType::ObjectType7:
                exitReached_ = true;
                break;
            case ObjectType::ObjectType2:
            case ObjectType::ObjectType3:
            case ObjectType::ObjectType4:
            case ObjectType::ObjectType16:
            case ObjectType::ObjectType17:
            case ObjectType::ObjectType20:
            case ObjectType::ObjectType33:
                blupiHit_ = true;
                break;
            default:
                break;
        }
    }
}
