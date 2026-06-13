#include "ScorePopup.hpp"

using namespace Urho3D;

ScorePopup::ScorePopup(Context* ctx, Scene* scene, Vector3 pos, const char* text)
    : origin_(pos)
{
    auto* cache = ctx->GetSubsystem<ResourceCache>();
    node_ = scene->CreateChild("ScorePopup");
    node_->SetPosition(pos);

    auto* t3d = node_->CreateComponent<Text3D>();
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");
    if (font) t3d->SetFont(font, 24.0f);
    t3d->SetText(text);
    t3d->SetColor(Color(1.0f, 0.95f, 0.2f, 1.0f));
    t3d->SetAlignment(HA_CENTER, VA_CENTER);
    t3d->SetFaceCameraMode(FC_ROTATE_XYZ);
}

ScorePopup::~ScorePopup() {
    if (node_) { node_->Remove(); node_ = nullptr; }
}

bool ScorePopup::Update(float dt) {
    timer_ += dt;
    if (timer_ >= kDuration) return false;
    float t = timer_ / kDuration;
    node_->SetPosition(origin_ + Vector3(0.0f, t * kRiseSpeed, 0.0f));
    float alpha = 1.0f - t;
    if (auto* t3d = node_->GetComponent<Text3D>())
        t3d->SetColor(Color(1.0f, 0.95f, 0.2f, alpha));
    return true;
}
