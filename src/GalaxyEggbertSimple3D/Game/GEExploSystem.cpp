#include "GEExploSystem.hpp"

using namespace Simple3D;

namespace GESimple3D {

// table_explo1 from mobile-eggbert Tables.cpp: 39-frame stomp-kill explosion.
const int GEExploSystem::kTable[39] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 3,
    4, 4, 3, 4, 3, 3, 4, 4, 5, 5,
    4, 5, 6, 5, 6, 6, 5, 5, 6, 7,
    7, 8, 8, 9, 9, 10, 10, 11, 11
};

void GEExploSystem::ApplyFrame(Entity* e, int icon) {
    int col = icon % 10;
    int row = icon / 10;
    e->SetBillboardUVRect(col * kCellPx, row * kCellPx, kCellPx, kCellPx);
}

void GEExploSystem::Spawn(Game& game, Vector3 pos) {
    auto* e = game.CreateEntity("Explo" + std::to_string(nextId_++));
    pos.y_ += 0.5f;
    e->SetPosition(pos);
    e->AddBillboard("icons/explo.png", kWorldSize);
    ApplyFrame(e, kTable[0]);
    active_.push_back({e, 0.0f});
}

void GEExploSystem::Update(Game& game, float dt) {
    for (int i = static_cast<int>(active_.size()) - 1; i >= 0; --i) {
        auto& ex = active_[i];
        ex.elapsed += dt;
        int frame = static_cast<int>(ex.elapsed / kFrameTime);
        if (frame >= kFrames) {
            game.DestroyEntity(ex.entity);
            active_.erase(active_.begin() + i);
            continue;
        }
        ApplyFrame(ex.entity, kTable[frame]);
    }
}

void GEExploSystem::Clear(Game& game) {
    for (auto& ex : active_)
        game.DestroyEntity(ex.entity);
    active_.clear();
}

} // namespace GESimple3D
