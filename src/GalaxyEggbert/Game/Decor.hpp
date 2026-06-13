#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/def/ObjectType.hpp"
#include "ObjectNode.hpp"
#include <array>
#include <memory>

// Object pool and patrol/collision engine, ported from mobile-eggbert Decor.cpp.
//
// Supported types: 1 (platform), 2/3 (enemies A/B), 4 (bulldozer), 5 (treasure),
//   6 (egg), 7 (exit), 12 (crate), 13 (helicopter), 16 (spider), 17 (fish),
//   20 (bird), 25 (shield), 30 (drink), 33 (blupit tank), 49/50/51 (keys R/G/B).
// Movement: linear XYZ patrol (StepMovement), vertical oscillation for spiders.
// Not yet ported: tile events, advanced vehicle AI (jeep, skateboard).
class Decor {
public:
    static constexpr int kMaxObjects = 100;

    struct Object {
        GalaxyEggbert::ObjectType  type          = GalaxyEggbert::ObjectType::ObjectType0;
        Urho3D::Vector3            posStart;
        Urho3D::Vector3            posEnd;
        Urho3D::Vector3            pos;
        float                      speed         = 1.5f;
        float                      respawnTimer  = 0.0f; // >0 while waiting to respawn
        int                        direction     = 1;    // +1 toward posEnd, -1 toward posStart
        int                        animPhase     = 0;
        bool                       active        = false;
        bool                       facingLeft    = true; // sprites in element.png face left
        std::unique_ptr<ObjectNode> node;
    };

    Decor(Urho3D::Context* context, Urho3D::Scene* scene);
    ~Decor() = default;

    // posEnd left at default → stationary object.
    void PlaceObject(GalaxyEggbert::ObjectType type,
                     Urho3D::Vector3 pos,
                     Urho3D::Vector3 posEnd = Urho3D::Vector3(0.0f, -999.0f, 0.0f),
                     float speed = 1.5f);

    // blupiVelY: Blupi's current Y velocity; negative = falling. Used for stomp detection.
    // totalTime: continuous game timer used for pickup Y-bobbing animation.
    void Update(float dt, Urho3D::Vector3 blupiPos, float blupiVelY = 0.0f, float totalTime = 0.0f);

    bool    WasExitReached()     const { return exitReached_;     }
    bool    WasBlupiHit()        const { return blupiHit_;        }
    bool    WasShieldCollected() const { return shieldCollected_; }
    bool    WasEggCollected()    const { return eggCollected_;    }
    bool    WasDrinkCollected()  const { return drinkCollected_;  }
    bool    WasStompKill()       const { return stompKill_;       }
    Urho3D::Vector3 GetLastStompPos()   const { return lastStompPos_;   }
    bool    WasRespawned()       const { return respawnedThis_;    }
    Urho3D::Vector3 GetLastRespawnPos() const { return lastRespawnPos_; }
    int     GetCollected()       const { return collected_;       }
    int     GetTotalTreasures()  const { return totalTreasures_;  }
    int     GetKeys49()          const { return keysType49_;      }
    int     GetKeys50()          const { return keysType50_;      }
    int     GetKeys51()          const { return keysType51_;      }
    int     GetKeysCollected()   const { return keysType49_ + keysType50_ + keysType51_; }
    // XZ delta accumulated by all platforms that Blupi is riding this frame.
    Urho3D::Vector3 GetPlatformDelta() const { return platformDelta_; }
    // Position of the exit object (ObjectType7), or Vector3(-999,0,0) if none.
    Urho3D::Vector3 GetExitPos() const { return exitPos_; }
    // Y surface height to snap Blupi onto; -999 means no platform under Blupi.
    float           GetPlatformLandY() const { return platformLandY_; }
    void ClearEvents() { exitReached_ = blupiHit_ = shieldCollected_ = eggCollected_ = drinkCollected_ = stompKill_ = respawnedThis_ = false; }

private:
    int  GetIcon(const Object& obj) const;
    void StepMovement(Object& obj, float dt);
    bool TouchesBlupi(const Object& obj, Urho3D::Vector3 blupiPos) const;

    Urho3D::Context* context_;
    Urho3D::Scene*   scene_;
    std::array<Object, kMaxObjects> objects_{};
    int  objCount_    = 0;
    bool            exitReached_     = false;
    bool            blupiHit_        = false;
    bool            shieldCollected_ = false;
    bool            eggCollected_    = false;
    bool            drinkCollected_  = false;
    bool            stompKill_       = false;
    bool            respawnedThis_   = false;
    int             collected_       = 0;
    int             totalTreasures_  = 0;
    int             keysType49_      = 0;
    int             keysType50_      = 0;
    int             keysType51_      = 0;
    Urho3D::Vector3 platformDelta_{0.0f, 0.0f, 0.0f};
    float           platformLandY_ = -999.0f;
    Urho3D::Vector3 lastStompPos_{0.0f, 0.0f, 0.0f};
    Urho3D::Vector3 lastRespawnPos_{0.0f, 0.0f, 0.0f};
    Urho3D::Vector3 exitPos_{-999.0f, 0.0f, 0.0f};
};
