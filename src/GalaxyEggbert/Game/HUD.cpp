#include "HUD.hpp"
#include <algorithm>
#include <cstdio>

using namespace Urho3D;

// blupi.png: 600×2040, 10 cols × 34 rows of 60×60 px. Icon 48 = col 8, row 4.
static const IntRect kBlupiHeadRect{480, 240, 540, 300};
// element.png: 600×1740, 10 cols × 29 rows of 60×60 px. Icon 215 = col 5, row 21.
static const IntRect kKeyIconRect{300, 1260, 360, 1320};

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

    // Life icons: Blupi head sprite (icon 48) inside the gauge area.
    auto* blupiTex = cache->GetResource<Texture2D>("icons/blupi.png");
    for (int i = 0; i < kMaxDisplayedLives; i++) {
        auto* icon = root->CreateChild<BorderImage>("LifeIcon" + String(i));
        if (blupiTex) {
            icon->SetTexture(blupiTex);
            icon->SetImageRect(kBlupiHeadRect);
        }
        icon->SetSize(20, 20);
        icon->SetAlignment(HA_LEFT, VA_BOTTOM);
        icon->SetPosition(18 + i * 22, -82);
        icon->SetVisible(false);
        lifeIcons_[i] = icon;
    }

    // Key icons: element.png icon 215 (red key), one per collected key (max 3).
    auto* elemTex = cache->GetResource<Texture2D>("icons/element.png");
    for (int i = 0; i < 3; i++) {
        auto* icon = root->CreateChild<BorderImage>("KeyIcon" + String(i));
        if (elemTex) {
            icon->SetTexture(elemTex);
            icon->SetImageRect(kKeyIconRect);
        }
        icon->SetSize(20, 20);
        icon->SetAlignment(HA_LEFT, VA_BOTTOM);
        icon->SetPosition(18 + i * 24, -60);
        icon->SetVisible(false);
        keyIcons_[i] = icon;
    }

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
    if (Text* t = text_) t->SetVisible(visible);
    if (!visible) {
        if (BorderImage* g = gauge_) g->SetVisible(false);
        for (int i = 0; i < kMaxDisplayedLives; i++)
            if (BorderImage* icon = lifeIcons_[i]) icon->SetVisible(false);
        for (int i = 0; i < 3; i++)
            if (BorderImage* icon = keyIcons_[i]) icon->SetVisible(false);
    }
}

void HUD::ShowWin() {
    if (Text* t = text_)
        t->SetText("LEVEL COMPLETE!\nPress any key to continue...");
    if (BorderImage* g = gauge_) g->SetVisible(false);
    for (int i = 0; i < kMaxDisplayedLives; i++)
        if (BorderImage* icon = lifeIcons_[i]) icon->SetVisible(false);
    for (int i = 0; i < 3; i++)
        if (BorderImage* icon = keyIcons_[i]) icon->SetVisible(false);
}

static const char* WorldName(int world) {
    static const char* kNames[] = {
        "Grassland", "Forest", "Ice Caves", "Lava Fields", "Space Station"
    };
    int idx = world - 1;
    if (idx >= 0 && idx < 5) return kNames[idx];
    return "Unknown";
}

void HUD::ShowPlay(int lives, int collected, int totalTreasures, int keys,
                   float shieldSecs, int world) {
    int showLives = std::min(lives, kMaxDisplayedLives);
    for (int i = 0; i < kMaxDisplayedLives; i++)
        if (BorderImage* icon = lifeIcons_[i]) icon->SetVisible(i < showLives);

    int showKeys = std::min(keys, 3);
    for (int i = 0; i < 3; i++)
        if (BorderImage* icon = keyIcons_[i]) icon->SetVisible(i < showKeys);

    if (BorderImage* g = gauge_) g->SetVisible(true);

    if (Text* t = text_) {
        char buf[256];
        if (shieldSecs > 0.0f) {
            std::snprintf(buf, sizeof(buf),
                "UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause\n"
                "World %d: %s | Treasures: %d/%d  SHIELD %.1fs",
                world, WorldName(world), collected, totalTreasures, shieldSecs);
        } else {
            std::snprintf(buf, sizeof(buf),
                "UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause\n"
                "World %d: %s | Treasures: %d/%d",
                world, WorldName(world), collected, totalTreasures);
        }
        t->SetText(buf);
    }
}
