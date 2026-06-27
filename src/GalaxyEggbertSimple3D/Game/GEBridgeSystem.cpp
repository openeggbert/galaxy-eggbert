#include "GEBridgeSystem.hpp"
#include "GETerrainRenderer.hpp"
#include "GESound.hpp"
#include <GalaxyEggbert/def/SoundChannel.hpp>

namespace GESimple3D {

// table_bridge from mobile-eggbert Tables.cpp (157 frames, ObjectType52).
// -1 = tile invisible (only Plane entity hides; Box collider stays solid).
const int GEBridgeSystem::kTable[157] = {
    365, 366, 365, 366, 365, 366, 365, 366, 365, 366,
    365, 366, 365, 366, 365, 366, 367, 367, 368, 368,
    369, 369, 370, 370, 371, 371, 372, 372,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
     -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    372, 372, 371, 371, 370, 370, 369, 369, 368, 368,
    367, 367, 366, 366, 365, 365, 364
};

bool GEBridgeSystem::IsActive(int wx, int wz) const {
    return occupied_.count(Key(wx, wz)) > 0;
}

void GEBridgeSystem::Spawn(int wx, int wz) {
    int k = Key(wx, wz);
    if (occupied_.count(k)) return;
    occupied_.insert(k);
    active_.push_back({wx, wz, 0.0f, false});
}

void GEBridgeSystem::Update(float dt, GETerrainRenderer& terrain, GESound& sound) {
    for (int i = static_cast<int>(active_.size()) - 1; i >= 0; --i) {
        auto& b = active_[i];
        b.elapsed += dt;
        int frame = static_cast<int>(b.elapsed / kFrameTime);

        if (!b.played2 && frame >= 137) {
            b.played2 = true;
            sound.Play(GalaxyEggbert::SoundChannel::SoundChannel73);
        }

        if (frame >= kFrames) {
            terrain.SetTileIcon(b.wx, b.wz, 364);
            occupied_.erase(Key(b.wx, b.wz));
            active_.erase(active_.begin() + i);
            continue;
        }
        terrain.SetTileIcon(b.wx, b.wz, kTable[frame] >= 0 ? kTable[frame] : 364);
    }
}

void GEBridgeSystem::Clear() {
    active_.clear();
    occupied_.clear();
}

} // namespace GESimple3D
