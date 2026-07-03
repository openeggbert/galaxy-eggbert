#pragma once

#include <Simple3D/Simple3D.h>
#include <GalaxyEggbert/Worlds/World.hpp>
#include <GalaxyEggbert/def/ObjectType.hpp>
#include <memory>
#include <string>
#include <vector>

namespace GESimple3D {

// One moving/interactive object loaded from a mobile-eggbert MoveObject line.
struct MobileObjSpec {
    GalaxyEggbert::ObjectType type;
    Simple3D::Vector3         posStart;
    Simple3D::Vector3         posEnd;
    float                     speed = 1.5f;
};

// World runtime state for one loaded level.
// Holds the voxel world, parsed spawn point, object list, and per-level timer.
// Engine-independent — no Urho3D or Simple3D rendering here; only data.
class GEWorldRuntime {
public:
    static constexpr int kWCX = 50; // world centre offset X
    static constexpr int kWCZ = 50; // world centre offset Z

    static const char* WorldName(int world);

    GEWorldRuntime();

    // Load a mobile-eggbert .txt world file.
    // Returns true on success, false if the file cannot be opened or parsed.
    // On failure the world remains empty (all air blocks).
    bool LoadFromMobileEggbertFile(const std::string& path);

    // Build a minimal hardcoded demo world used when no .txt file is found.
    void BuildDemoWorld();

    // Reset per-level timer and event flags.
    void ResetLevel();

    // Advance the level timer.
    void Update(float dt);

    // Accessors
    GalaxyEggbert::Worlds::World*       GetWorld()          { return world_.get(); }
    const GalaxyEggbert::Worlds::World* GetWorld()    const { return world_.get(); }
    const Simple3D::Vector3&  GetBlupiSpawn()         const { return blupiSpawn_; }
    int                       GetSkyRegion()          const { return skyRegion_; }
    int                       GetWorldNum()           const { return worldNum_; }
    float                     GetLevelTime()          const { return levelTime_; }
    int                       GetTotalTreasures()     const { return totalTreasures_; }

    const std::vector<MobileObjSpec>& GetMobileObjects() const { return mobileObjects_; }

    // BigDecor: is a second 100x100 background tile layer in mobile-eggbert
    // level files (see mobile-eggbert-2d-reference.md §2.3) — parsed and
    // stored here (same icon-id-to-block-type conversion as the main grid),
    // but not yet rendered anywhere; how to represent it in 3D is an open
    // question, not decided by this parser. Row-major, [row*100 + col].
    const std::vector<uint16_t>& GetBigDecor() const { return bigDecor_; }

    void SetWorldNum(int n) { worldNum_ = n; }
    void SetTotalTreasures(int n) { totalTreasures_ = n; }

    // Global animation tick (increments at 6 fps). Used for crusher kill-phase check.
    int GetAnimPhase() const { return animPhase_; }

private:
    std::unique_ptr<GalaxyEggbert::Worlds::World> world_;
    std::vector<MobileObjSpec> mobileObjects_;
    std::vector<uint16_t> bigDecor_;
    Simple3D::Vector3 blupiSpawn_{0.0f, 0.86f, 0.0f};
    int   skyRegion_      = 0;
    int   worldNum_       = 1;
    int   totalTreasures_ = 0;
    float levelTime_      = 0.0f;
    float animTimer_      = 0.0f;
    int   animPhase_      = 0;
};

} // namespace GESimple3D
