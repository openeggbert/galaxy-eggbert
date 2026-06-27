#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>
#include <vector>
#include <set>

namespace GESimple3D {

class GETerrainRenderer;
class GESound;

// Bridge construction animation (ObjectType52 from mobile-eggbert).
// Drives the 157-frame table_bridge sequence on a bridge tile's top-face Plane entity.
// Physics (Box collider) remains solid throughout — only the visual Plane changes.
class GEBridgeSystem {
public:
    // Returns true if a bridge at (wx,wz) is already animating.
    bool IsActive(int wx, int wz) const;

    // Start a bridge animation at tile (wx, wz). No-op if already active.
    void Spawn(int wx, int wz);

    // Advance all active bridge animations. Updates terrain tile visuals and plays sounds.
    void Update(float dt, GETerrainRenderer& terrain, GESound& sound);

    // Cancel all active animations (call on level load/reset).
    void Clear();

private:
    static constexpr float kFrameTime = 1.0f / 30.0f;  // 30 fps — matches mobile-eggbert timing
    static constexpr int   kFrames    = 157;
    static const int       kTable[157];

    struct Bridge { int wx; int wz; float elapsed; bool played2; };
    std::vector<Bridge>    active_;
    std::set<int>          occupied_;   // wx*1000+wz keys for fast dedup

    static int Key(int wx, int wz) { return wx * 1000 + wz; }
};

} // namespace GESimple3D
