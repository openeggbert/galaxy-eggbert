#include "HUD.hpp"
#include <cstdio>

using namespace Urho3D;

HUD::HUD(Context* context) : context_(context) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* ui    = context_->GetSubsystem<UI>();
    if (!ui) return;
    auto* root = ui->GetRoot();

    auto* gauge = root->CreateChild<BorderImage>("Gauge");
    auto* gaugeTex = cache->GetResource<Texture2D>("icons/jauge.png");
    if (gaugeTex) {
        gauge->SetTexture(gaugeTex);
        gauge->SetFullImageRect();
        gauge->SetSize(124, 88);
        gauge->SetAlignment(HA_LEFT, VA_BOTTOM);
        gauge->SetPosition(8, -96);
    }
    gauge->SetVisible(false);
    gauge_ = gauge;

    auto* text = root->CreateChild<Text>("HUD");
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");
    if (font) text->SetFont(font, 14);
    text->SetColor(Color(0.85f, 0.90f, 1.0f));
    text->SetPosition(12, 12);
    text->SetVisible(false);
    text_ = text;
}

void HUD::SetVisible(bool visible) {
    if (Text* t = text_)        t->SetVisible(visible);
    if (BorderImage* g = gauge_) g->SetVisible(visible);
}

void HUD::ShowWin() {
    if (Text* t = text_)
        t->SetText("LEVEL COMPLETE!\nPress any key to continue...");
    if (BorderImage* g = gauge_) g->SetVisible(false);
}

void HUD::ShowPlay(Vector3 pos, float facingYaw, int lives, int collected,
                   int keys, float shieldSecs) {
    if (Text* t = text_) {
        char buf[256];
        if (shieldSecs > 0.0f) {
            std::snprintf(buf, sizeof(buf),
                "UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause\n"
                "pos (%.1f, %.1f, %.1f)  facing %.0f deg\n"
                "Lives: %d  Treasures: %d  Keys: %d  SHIELD %.1fs",
                pos.x_, pos.y_, pos.z_, facingYaw, lives, collected, keys, shieldSecs);
        } else {
            std::snprintf(buf, sizeof(buf),
                "UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause\n"
                "pos (%.1f, %.1f, %.1f)  facing %.0f deg\n"
                "Lives: %d  Treasures: %d  Keys: %d",
                pos.x_, pos.y_, pos.z_, facingYaw, lives, collected, keys);
        }
        t->SetText(buf);
    }
    if (BorderImage* g = gauge_) g->SetVisible(true);
}
