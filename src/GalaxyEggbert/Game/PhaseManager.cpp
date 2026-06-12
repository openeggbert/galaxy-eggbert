#include "PhaseManager.hpp"

using namespace Urho3D;
using namespace GalaxyEggbert;

PhaseManager::PhaseManager(Context* context) : context_(context) {}

void PhaseManager::Enter(GamePhase phase) {
    phase_ = phase;
    switch (phase) {
        case GamePhase::Init:      ShowOverlay("backgrounds/init.png");  break;
        case GamePhase::Pause:     ShowOverlay("backgrounds/pause.png"); break;
        case GamePhase::Lost:      ShowOverlay("backgrounds/lost.png");  break;
        case GamePhase::Win:       ShowOverlay("backgrounds/win.png");   break;
        case GamePhase::MainSetup:
        case GamePhase::PlaySetup: ShowOverlay("backgrounds/setup.png"); break;
        default:                   HideOverlay();                        break;
    }
}

void PhaseManager::SetOverlayText(const String& text) {
    if (Text* t = overlayText_) t->SetText(text);
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

    auto* txt = img->CreateChild<Text>("OverlayText");
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");
    if (font) txt->SetFont(font, 16);
    txt->SetColor(Color(0.95f, 0.98f, 1.0f));
    txt->SetPosition(40, 40);
    overlayText_ = txt;
}

void PhaseManager::HideOverlay() {
    if (UIElement* el = overlayEl_) {
        el->Remove();
        overlayEl_.Reset();
    }
    overlayText_.Reset();
}
