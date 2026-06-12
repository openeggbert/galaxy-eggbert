#include "PhaseManager.hpp"

using namespace Urho3D;
using namespace GalaxyEggbert;

PhaseManager::PhaseManager(Context* context) : context_(context) {}

void PhaseManager::Enter(GamePhase phase) {
    phase_ = phase;
    switch (phase) {
        case GamePhase::Init:  ShowOverlay("backgrounds/init.png");  break;
        case GamePhase::Pause: ShowOverlay("backgrounds/pause.png"); break;
        default:               HideOverlay();                        break;
    }
}

void PhaseManager::ShowOverlay(const char* texPath) {
    HideOverlay();
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* ui    = context_->GetSubsystem<UI>();
    if (!ui) return;
    auto* tex = cache->GetResource<Texture2D>(texPath);
    if (!tex) return;
    auto* root = ui->GetRoot();
    auto* img  = root->CreateChild<BorderImage>("Overlay");
    img->SetTexture(tex);
    img->SetFullImageRect();
    img->SetSize(root->GetWidth(), root->GetHeight());
    img->SetAlignment(HA_LEFT, VA_TOP);
    img->SetOpacity(1.0f);
    overlayEl_ = img;
}

void PhaseManager::HideOverlay() {
    if (UIElement* el = overlayEl_) {
        el->Remove();
        overlayEl_.Reset();
    }
}
