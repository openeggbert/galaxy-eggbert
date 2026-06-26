#pragma once

#include <Simple3D/Simple3D.h>
#include <GalaxyEggbert/def/ObjectType.hpp>
#include "GEWorldRuntime.hpp"
#include <array>
#include <string>
#include <unordered_set>
#include <vector>

namespace GESimple3D {

// Object pool and interaction system for Simple3D port.
// Sprites from element.png (600×1740 px, 60×60 tiles, 10 cols × 29 rows).
class GEDecorSystem {
public:
    // Spawn all objects from the world's MobileObjSpec list.
    // player must be the Blupi entity so trigger callbacks can identify it.
    void Build(Simple3D::Game& game, const GEWorldRuntime& world,
               Simple3D::Entity* player);

    // Destroy all spawned entities.
    void Clear(Simple3D::Game& game);

    // Update patrol movement (S3D-1: only basic platform movement).
    void Update(float dt, const Simple3D::Vector3& blupiPos, float blupiVelY);

    // Events — cleared each frame by the game loop
    bool WasExitReached()     const { return exitReached_; }
    bool WasBlupiHit()        const { return blupiHit_; }
    bool WasShieldCollected() const { return shieldCollected_; }
    bool WasEggCollected()    const { return eggCollected_; }
    bool WasDrinkCollected()  const { return drinkCollected_; }
    bool WasStompKill()       const { return stompKill_; }
    int  GetCollected()       const { return collected_; }
    int  GetTotalTreasures()  const { return totalTreasures_; }
    int  GetKeys49()          const { return keys49_; }
    int  GetKeys50()          const { return keys50_; }
    int  GetKeys51()          const { return keys51_; }

    void ClearEvents() {
        exitReached_ = blupiHit_ = shieldCollected_ = eggCollected_ =
            drinkCollected_ = stompKill_ = false;
    }

    static constexpr int kElemTilePx = 60;
    static constexpr int kElemCols   = 10;
    static int GetObjIcon(GalaxyEggbert::ObjectType type, int phase);

private:

    struct ObjState {
        GalaxyEggbert::ObjectType type;
        Simple3D::Entity*  entity    = nullptr;
        Simple3D::Entity*  sprite    = nullptr; // child entity with billboard
        Simple3D::Vector3  posStart;
        Simple3D::Vector3  posEnd;
        float              speed     = 1.5f;
        float              direction = 1.0f;
        int                animPhase = 0;
        bool               active    = true;
        std::string        name;
    };

    std::vector<ObjState>        objects_;
    std::unordered_set<std::string> collected_set_;

    bool exitReached_     = false;
    bool blupiHit_        = false;
    bool shieldCollected_ = false;
    bool eggCollected_    = false;
    bool drinkCollected_  = false;
    bool stompKill_       = false;

    int  collected_      = 0;
    int  totalTreasures_ = 0;
    int  keys49_         = 0;
    int  keys50_         = 0;
    int  keys51_         = 0;
};

} // namespace GESimple3D
