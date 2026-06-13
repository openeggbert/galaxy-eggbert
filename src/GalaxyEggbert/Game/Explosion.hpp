#pragma once
#include "../GEEngine.hpp"

// One-shot death explosion: plays table_explo1 (39 frames, icons 0-11) from explo.png.
// Call Update(dt) each frame; it returns false when the animation finishes.
class Explosion {
public:
    Explosion(Urho3D::Context* ctx, Urho3D::Scene* scene, Urho3D::Vector3 pos);
    ~Explosion();
    bool Update(float dt);

private:
    static constexpr int   kCols   = 24;
    static constexpr float kSheetW = 1440.0f;
    static constexpr float kSheetH = 1440.0f;
    static constexpr float kTile   = 60.0f;
    static constexpr float kFps    = 12.0f;
    static constexpr int   kTotal  = 39;
    static const int kSeq[kTotal];

    Urho3D::Node*         node_   = nullptr;
    Urho3D::BillboardSet* sprite_ = nullptr;
    float timer_ = 0.0f;
    int   frame_ = -1;

    void SetFrame(int icon);
};
