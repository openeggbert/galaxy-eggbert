#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/def/ObjectType.hpp"
#include "ObjectNode.hpp"
#include <array>
#include <memory>

// Partial port of mobile-eggbert Decor.cpp — object pool, patrol movement,
// and Blupi–object collision for Phase 8 basic gameplay.
//
// Covers: MoveObjectStepLine (linear patrol), MoveObjectStepIcon (icon/phase)
// for ObjectType2 (enemy), ObjectType5 (treasure), ObjectType6 (egg), ObjectType7 (exit).
// Full Decor port (tile events, all enemy AI, vehicles) is future work.
class Decor {
public:
    static constexpr int kMaxObjects = 50;

    struct Object {
        GalaxyEggbert::ObjectType  type      = GalaxyEggbert::ObjectType::ObjectType0;
        Urho3D::Vector3            posStart;
        Urho3D::Vector3            posEnd;
        Urho3D::Vector3            pos;
        float                      speed     = 1.5f;
        int                        direction = 1;   // +1 toward posEnd, -1 toward posStart
        int                        animPhase = 0;
        bool                       active    = false;
        std::unique_ptr<ObjectNode> node;
    };

    Decor(Urho3D::Context* context, Urho3D::Scene* scene);
    ~Decor() = default;

    // posEnd left at default → stationary object.
    void PlaceObject(GalaxyEggbert::ObjectType type,
                     Urho3D::Vector3 pos,
                     Urho3D::Vector3 posEnd = Urho3D::Vector3(0.0f, -999.0f, 0.0f),
                     float speed = 1.5f);

    void Update(float dt, Urho3D::Vector3 blupiPos);

    bool WasExitReached()     const { return exitReached_;     }
    bool WasBlupiHit()        const { return blupiHit_;        }
    bool WasShieldCollected() const { return shieldCollected_; }
    int  GetCollected()       const { return collected_;       }
    int  GetTotalTreasures()  const { return totalTreasures_;  }
    int  GetKeysCollected()   const { return keysCollected_;   }
    void ClearEvents()              { exitReached_ = blupiHit_ = shieldCollected_ = false; }

private:
    int  GetIcon(const Object& obj) const;
    void StepMovement(Object& obj, float dt);
    bool TouchesBlupi(const Object& obj, Urho3D::Vector3 blupiPos) const;

    Urho3D::Context* context_;
    Urho3D::Scene*   scene_;
    std::array<Object, kMaxObjects> objects_{};
    int  objCount_    = 0;
    bool exitReached_     = false;
    bool blupiHit_        = false;
    bool shieldCollected_ = false;
    int  collected_       = 0;
    int  totalTreasures_  = 0;
    int  keysCollected_   = 0;
};
