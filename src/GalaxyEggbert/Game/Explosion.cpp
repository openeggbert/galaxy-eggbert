#include "Explosion.hpp"

using namespace Urho3D;

// Ported verbatim from mobile-eggbert Tables.cpp table_explo1[39].
const int Explosion::kSeq[kTotal] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 3,
    4, 4, 3, 4, 3, 3, 4, 4, 5, 5,
    4, 5, 6, 5, 6, 6, 5, 5, 6, 7,
    7, 8, 8, 9, 9, 10, 10, 11, 11
};

Explosion::Explosion(Context* ctx, Scene* scene, Vector3 pos, float scale) {
    auto* cache = ctx->GetSubsystem<ResourceCache>();
    node_ = scene->CreateChild("Explosion");
    node_->SetPosition(pos);

    sprite_ = node_->CreateComponent<BillboardSet>();
    sprite_->SetNumBillboards(1);
    sprite_->SetFaceCameraMode(FC_ROTATE_XYZ);

    auto* tex  = cache->GetResource<Texture2D>("icons/explo.png");
    auto* tech = cache->GetResource<Technique>("Techniques/DiffAlpha.xml");
    SharedPtr<Material> mat(new Material(ctx));
    if (tech) mat->SetTechnique(0, tech);
    if (tex)  mat->SetTexture(TU_DIFFUSE, tex);
    mat->SetShaderParameter("MatDiffColor", Color(1.0f, 1.0f, 1.0f, 1.0f));
    mat->SetShaderParameter("MatSpecColor",  Color(0.0f, 0.0f, 0.0f, 0.0f));
    sprite_->SetMaterial(mat);

    Billboard* bb = sprite_->GetBillboard(0);
    bb->position_ = Vector3::ZERO;
    bb->size_     = Vector2(1.5f * scale, 1.5f * scale);
    bb->enabled_  = true;
    SetFrame(kSeq[0]);
}

Explosion::~Explosion() {
    if (node_) { node_->Remove(); node_ = nullptr; sprite_ = nullptr; }
}

void Explosion::SetFrame(int icon) {
    if (!sprite_) return;
    int   col = icon % kCols;
    int   row = icon / kCols;
    float u0  = col * kTile / kSheetW;
    float v0  = row * kTile / kSheetH;
    float us  = kTile / kSheetW;
    float vs  = kTile / kSheetH;
    Billboard* bb = sprite_->GetBillboard(0);
    bb->uv_ = Rect(u0, v0, u0 + us, v0 + vs);
    sprite_->Commit();
}

bool Explosion::Update(float dt) {
    timer_ += dt;
    int idx = static_cast<int>(timer_ * kFps);
    if (idx >= kTotal) return false;
    if (idx != frame_) {
        frame_ = idx;
        SetFrame(kSeq[idx]);
    }
    return true;
}
