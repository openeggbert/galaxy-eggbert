#include "HUD.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace Urho3D;

// blupi.png: 600×2040, 10 cols × 34 rows of 60×60 px. Icon 48 = col 8, row 4.
static const IntRect kBlupiHeadRect{480, 240, 540, 300};
// element.png: 600×1740, 10 cols × 29 rows of 60×60 px.
// Icon 209 (red key) = col 9, row 20. Icon 220 (green) = col 0, row 22. Icon 229 (blue) = col 9, row 22.
static const IntRect kKeyRects[3] = {
    {540, 1200, 600, 1260}, // type49 red key
    {  0, 1320,  60, 1380}, // type50 green key
    {540, 1320, 600, 1380}, // type51 blue key
};

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

    // Key icons: distinct red/green/blue icons from element.png, one slot per key type.
    auto* elemTex = cache->GetResource<Texture2D>("icons/element.png");
    for (int i = 0; i < 3; i++) {
        auto* icon = root->CreateChild<BorderImage>("KeyIcon" + String(i));
        if (elemTex) {
            icon->SetTexture(elemTex);
            icon->SetImageRect(kKeyRects[i]);
        }
        icon->SetSize(20, 20);
        icon->SetAlignment(HA_LEFT, VA_BOTTOM);
        icon->SetPosition(18 + i * 24, -60);
        icon->SetVisible(false);
        keyIcons_[i] = icon;
    }

    auto* flash = root->CreateChild<BorderImage>("HitFlash");
    flash->SetColor(Color(1.0f, 0.0f, 0.0f, 0.0f));
    flash->SetSize(10000, 10000);
    flash->SetVisible(false);
    hitFlash_ = flash;

    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");

    auto* text = root->CreateChild<Text>("HUD");
    if (font) text->SetFont(font, 14);
    text->SetColor(Color(0.85f, 0.90f, 1.0f));
    text->SetPosition(12, 12);
    text->SetVisible(false);
    text_ = text;

    auto* overflow = root->CreateChild<Text>("LivesOverflow");
    if (font) overflow->SetFont(font, 13);
    overflow->SetColor(Color(1.0f, 0.92f, 0.4f));
    overflow->SetAlignment(HA_LEFT, VA_BOTTOM);
    overflow->SetPosition(18 + kMaxDisplayedLives * 22, -84);
    overflow->SetVisible(false);
    livesOverflow_ = overflow;

    auto* intro = root->CreateChild<Text>("WorldIntro");
    if (font) intro->SetFont(font, 32);
    intro->SetTextAlignment(HA_CENTER);
    intro->SetAlignment(HA_CENTER, VA_CENTER);
    intro->SetPosition(0, -50);
    intro->SetVisible(false);
    worldIntroText_ = intro;
}

void HUD::SetDanger(bool danger) { dangerMode_ = danger; if (!danger) dangerPhase_ = 0.0f; }

static constexpr float kFlashDuration = 0.4f;

void HUD::ShowHitFlash() {
    hitFlashTimer_ = kFlashDuration;
    if (BorderImage* f = hitFlash_) {
        f->SetColor(Color(1.0f, 0.0f, 0.0f, 0.5f));
        f->SetVisible(true);
    }
}

void HUD::Update(float dt) {
    if (dangerMode_) {
        dangerPhase_ += dt * 2.0f; // 2 Hz pulse
        float a = 0.18f * (0.5f + 0.5f * std::sin(dangerPhase_ * 3.14159f));
        if (BorderImage* f = hitFlash_) {
            f->SetColor(Color(1.0f, 0.0f, 0.0f, a));
            f->SetVisible(a > 0.005f);
        }
    }
    if (hitFlashTimer_ <= 0.0f) return;
    hitFlashTimer_ -= dt;
    if (hitFlashTimer_ <= 0.0f) {
        hitFlashTimer_ = 0.0f;
        if (BorderImage* f = hitFlash_) f->SetVisible(false);
        return;
    }
    float alpha = 0.5f * (hitFlashTimer_ / kFlashDuration);
    if (BorderImage* f = hitFlash_) f->SetColor(Color(1.0f, 0.0f, 0.0f, alpha));
}

void HUD::SetVisible(bool visible) {
    if (Text* t = text_) t->SetVisible(visible);
    if (!visible) {
        dangerMode_  = false;
        dangerPhase_ = 0.0f;
        if (Text* ot = livesOverflow_) ot->SetVisible(false);
        if (Text* it = worldIntroText_) it->SetVisible(false);
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

void HUD::ShowWorldIntro(const char* text, float alpha) {
    if (Text* t = worldIntroText_) {
        bool show = alpha > 0.01f;
        t->SetVisible(show);
        if (show) {
            t->SetText(text);
            t->SetColor(Color(1.0f, 0.92f, 0.4f, alpha));
        }
    }
}

static const char* WorldName(int world) {
    static const char* kNames[] = {
        "Grassland", "Forest", "Ice Caves", "Lava Fields", "Space Station"
    };
    int idx = world - 1;
    if (idx >= 0 && idx < 5) return kNames[idx];
    return "Unknown";
}

void HUD::ShowPlay(int lives, int collected, int totalTreasures,
                   int keys49, int keys50, int keys51,
                   float shieldSecs, int world, bool showHint, float levelTime, int score,
                   float gameSpeed) {
    int showLives = std::min(lives, kMaxDisplayedLives);
    for (int i = 0; i < kMaxDisplayedLives; i++)
        if (BorderImage* icon = lifeIcons_[i]) icon->SetVisible(i < showLives);

    if (Text* ot = livesOverflow_) {
        if (lives > kMaxDisplayedLives) {
            char cnt[8];
            std::snprintf(cnt, sizeof(cnt), "x%d", lives);
            ot->SetText(cnt);
            ot->SetVisible(true);
        } else {
            ot->SetVisible(false);
        }
    }

    // Key slots: 0=red(type49), 1=green(type50), 2=blue(type51).
    const int keyHave[3] = { keys49, keys50, keys51 };
    for (int i = 0; i < 3; i++)
        if (BorderImage* icon = keyIcons_[i]) icon->SetVisible(keyHave[i] > 0);

    if (BorderImage* g = gauge_) g->SetVisible(true);

    if (Text* t = text_) {
        char buf[256];
        const char* hint = showHint
            ? "ARROWS: move/turn  LCTRL: jump  LSHIFT: crouch  RSHIFT: glide(air)/look-up  G: speed  ESC: pause\n"
            : "";
        int mins = static_cast<int>(levelTime) / 60;
        int secs = static_cast<int>(levelTime) % 60;
        const char* speedTag = (gameSpeed > 1.2f) ? "  FAST" : (gameSpeed < 0.8f) ? "  SLOW" : "";
        if (shieldSecs > 0.0f) {
            std::snprintf(buf, sizeof(buf),
                "%sWorld %d: %s | Treasures: %d/%d  SHIELD %.1fs  %d:%02d  Score: %d%s",
                hint, world, WorldName(world), collected, totalTreasures, shieldSecs, mins, secs, score, speedTag);
            t->SetColor(shieldSecs < 1.5f ? Color(1.0f, 0.55f, 0.1f) : Color(0.85f, 0.90f, 1.0f));
        } else {
            std::snprintf(buf, sizeof(buf),
                "%sWorld %d: %s | Treasures: %d/%d  %d:%02d  Score: %d%s",
                hint, world, WorldName(world), collected, totalTreasures, mins, secs, score, speedTag);
            t->SetColor(Color(0.85f, 0.90f, 1.0f));
        }
        t->SetText(buf);
    }
}
