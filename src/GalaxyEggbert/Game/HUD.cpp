#include "HUD.hpp"
#include <cstdio>

using namespace Urho3D;

HUD::HUD(Context* context) : context_(context) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* ui    = context_->GetSubsystem<UI>();
    if (!ui) return;
    auto* root = ui->GetRoot();
    auto* text = root->CreateChild<Text>("HUD");
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");
    if (font) text->SetFont(font, 14);
    text->SetText("UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause  RMB: camera");
    text->SetColor(Color(0.85f, 0.90f, 1.0f));
    text->SetPosition(12, 12);
    text->SetVisible(false);
    text_ = text;
}

void HUD::SetVisible(bool visible) {
    if (Text* t = text_) t->SetVisible(visible);
}

void HUD::ShowPlay(Vector3 pos, float facingYaw) {
    if (Text* t = text_) {
        char buf[128];
        std::snprintf(buf, sizeof(buf),
            "UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause\n"
            "pos (%.1f, %.1f, %.1f)  facing %.0f deg",
            pos.x_, pos.y_, pos.z_, facingYaw);
        t->SetText(buf);
    }
}
