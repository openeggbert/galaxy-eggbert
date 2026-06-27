#pragma once

#include <Simple3D/Simple3D.h>
#include <vector>

namespace GESimple3D {

// Billboard explosion effects from explo.png.
// Stomp kill → Spawn() plays table_explo1 (39 frames at ~30fps = 1.3 s).
// explo.png: 1440×1440 px, 144×144 px per cell, 10 columns.
class GEExploSystem {
public:
    static constexpr int   kCellPx   = 144;
    static constexpr float kWorldSize = 1.5f;

    void Spawn(Simple3D::Game& game, Simple3D::Vector3 pos);
    void Update(Simple3D::Game& game, float dt);
    void Clear(Simple3D::Game& game);

private:
    struct Explo {
        Simple3D::Entity* entity = nullptr;
        float             elapsed = 0.0f;
    };

    std::vector<Explo> active_;
    int nextId_ = 0;

    static constexpr float kFrameTime = 1.0f / 30.0f;
    static constexpr int kFrames = 39;
    static const int kTable[39];

    static void ApplyFrame(Simple3D::Entity* e, int icon);
};

} // namespace GESimple3D
