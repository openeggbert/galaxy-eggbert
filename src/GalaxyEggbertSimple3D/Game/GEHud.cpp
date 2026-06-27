#include "GEHud.hpp"
#include <algorithm>
#include <cstdio>

using namespace Simple3D;
using namespace Simple3D::UI;

namespace GESimple3D {

// blupi.png: 600×2040, 10 cols × 34 rows of 60×60. Icon 48 = col 8, row 4.
static constexpr int kBlupiHeadX = 480, kBlupiHeadY = 240, kBlupiHeadSz = 60;

// element.png: 600×1740, 10 cols × 29 rows of 60×60.
// Icon 209 = red key (col 9, row 20). Icon 220 = green (col 0, row 22). Icon 229 = blue (col 9, row 22).
static const int kKeyRects[3][4] = {
    {540, 1200, 60, 60},  // type49 red
    {  0, 1320, 60, 60},  // type50 green
    {540, 1320, 60, 60},  // type51 blue
};

static constexpr float kFlashDuration = 0.4f;

void GEHud::SetMenuBackground(const std::string& bgPath) {
    if (!menuBg_) return;
    if (bgPath.empty()) {
        menuBg_->SetVisible(false);
        return;
    }
    menuBg_->SetTexture(bgPath);
    menuBg_->SetSize(1280, 720);
    menuBg_->SetVisible(true);
}

void GEHud::Create(Game& game) {
    // Created first so it renders behind all other HUD elements.
    menuBg_ = game.CreateImage("");
    menuBg_->SetSize(1280, 720);
    menuBg_->SetAnchor(Anchor::TopLeft);
    menuBg_->SetPosition(0, 0);
    menuBg_->SetVisible(false);

    main_ = game.CreateLabel("");
    main_->SetPosition(12, 12);
    main_->SetFontSize(14);
    main_->SetColor(Color(0.85f, 0.90f, 1.0f));

    overlay_ = game.CreateLabel("");
    overlay_->SetPosition(12, 12);
    overlay_->SetFontSize(22);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
    overlay_->SetVisible(false);

    hint_ = game.CreateLabel(
        "ARROWS move/turn  LCTRL jump  LSHIFT crouch  RSHIFT glide  G speed  ESC pause");
    hint_->SetPosition(12, 32);
    hint_->SetFontSize(13);
    hint_->SetColor(Color(0.75f, 0.85f, 0.75f));

    // Gauge sprite — jauge.png, anchored bottom-left
    gauge_ = game.CreateImage("icons/jauge.png");
    gauge_->SetSize(124, 88);
    gauge_->SetAnchor(Anchor::BottomLeft);
    gauge_->SetPosition(8, -96);
    gauge_->SetVisible(false);

    // Life icons: blupi.png icon 48 (Blupi head)
    for (int i = 0; i < kMaxDisplayedLives; ++i) {
        lifeIcons_[i] = game.CreateImage("icons/blupi.png");
        lifeIcons_[i]->SetImageRect(kBlupiHeadX, kBlupiHeadY, kBlupiHeadSz, kBlupiHeadSz);
        lifeIcons_[i]->SetSize(20, 20);
        lifeIcons_[i]->SetAnchor(Anchor::BottomLeft);
        lifeIcons_[i]->SetPosition(18 + i * 22, -82);
        lifeIcons_[i]->SetVisible(false);
    }

    // Lives overflow label (same anchor area, bottom-left)
    livesOverflow_ = game.CreateLabel("");
    livesOverflow_->SetPosition(18 + kMaxDisplayedLives * 22, 12); // approximate top-left fallback
    livesOverflow_->SetFontSize(13);
    livesOverflow_->SetColor(Color(1.0f, 0.92f, 0.4f));
    livesOverflow_->SetVisible(false);

    // Key icons: element.png red/green/blue keys
    for (int i = 0; i < 3; ++i) {
        keyIcons_[i] = game.CreateImage("icons/element.png");
        keyIcons_[i]->SetImageRect(kKeyRects[i][0], kKeyRects[i][1],
                                   kKeyRects[i][2], kKeyRects[i][3]);
        keyIcons_[i]->SetSize(20, 20);
        keyIcons_[i]->SetAnchor(Anchor::BottomLeft);
        keyIcons_[i]->SetPosition(18 + i * 24, -60);
        keyIcons_[i]->SetVisible(false);
    }

    // Hit flash: full-screen red panel
    hitFlash_ = game.CreatePanel();
    hitFlash_->SetColor(Color(1.0f, 0.0f, 0.0f, 0.0f));
    hitFlash_->SetSize(10000, 10000);
    hitFlash_->SetVisible(false);
}

void GEHud::ShowHitFlash() {
    hitFlashTimer_ = kFlashDuration;
    if (hitFlash_) {
        hitFlash_->SetColor(Color(1.0f, 0.0f, 0.0f, 0.5f));
        hitFlash_->SetVisible(true);
    }
}

void GEHud::Update(float dt) {
    if (hitFlashTimer_ <= 0.0f) return;
    hitFlashTimer_ -= dt;
    if (hitFlashTimer_ <= 0.0f) {
        hitFlashTimer_ = 0.0f;
        if (hitFlash_) hitFlash_->SetVisible(false);
        return;
    }
    float alpha = 0.5f * (hitFlashTimer_ / kFlashDuration);
    if (hitFlash_) hitFlash_->SetColor(Color(1.0f, 0.0f, 0.0f, alpha));
}

void GEHud::ShowPlay(int worldNum, const std::string& worldName,
                     int lives, int collected, int totalTreasures,
                     int keys49, int keys50, int keys51,
                     float shieldSecs, float levelTime, int score,
                     float gameSpeed, bool showHint) {
    if (!main_) return;
    SetMenuBackground("");
    overlay_->SetVisible(false);
    main_->SetVisible(true);
    hint_->SetVisible(showHint);

    // Life icons
    int showLives = std::min(lives, kMaxDisplayedLives);
    for (int i = 0; i < kMaxDisplayedLives; ++i)
        if (lifeIcons_[i]) lifeIcons_[i]->SetVisible(i < showLives);
    if (gauge_) gauge_->SetVisible(true);

    if (livesOverflow_) {
        if (lives > kMaxDisplayedLives) {
            char cnt[8];
            std::snprintf(cnt, sizeof(cnt), "x%d", lives);
            livesOverflow_->SetText(cnt);
            livesOverflow_->SetVisible(true);
        } else {
            livesOverflow_->SetVisible(false);
        }
    }

    // Key icons
    const int keyHave[3] = { keys49, keys50, keys51 };
    for (int i = 0; i < 3; ++i)
        if (keyIcons_[i]) keyIcons_[i]->SetVisible(keyHave[i] > 0);

    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;
    const char* speedTag = (gameSpeed > 1.2f) ? "  FAST" : (gameSpeed < 0.8f) ? "  SLOW" : "";

    char buf[256];
    if (shieldSecs > 0.0f) {
        std::snprintf(buf, sizeof(buf),
            "World %d: %s | Treasures: %d/%d  SHIELD %.1fs  %d:%02d  Score: %d%s",
            worldNum, worldName.c_str(), collected, totalTreasures,
            shieldSecs, mins, secs, score, speedTag);
        main_->SetColor(shieldSecs < 1.5f ? Color(1.0f, 0.55f, 0.1f) : Color(0.85f, 0.90f, 1.0f));
    } else {
        std::snprintf(buf, sizeof(buf),
            "World %d: %s | Treasures: %d/%d  %d:%02d  Score: %d%s",
            worldNum, worldName.c_str(), collected, totalTreasures,
            mins, secs, score, speedTag);
        main_->SetColor(Color(0.85f, 0.90f, 1.0f));
    }
    main_->SetText(buf);
}

void GEHud::ShowWin(int worldNum, const std::string& worldName,
                    int collected, int totalTreasures,
                    int lives, int score, float levelTime) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    if (gauge_) gauge_->SetVisible(false);
    for (int i = 0; i < kMaxDisplayedLives; ++i)
        if (lifeIcons_[i]) lifeIcons_[i]->SetVisible(false);
    for (int i = 0; i < 3; ++i)
        if (keyIcons_[i]) keyIcons_[i]->SetVisible(false);
    if (livesOverflow_) livesOverflow_->SetVisible(false);

    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;

    SetMenuBackground("backgrounds/win.png");
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "LEVEL COMPLETE!\n\nWorld %d: %s\nTreasures: %d/%d  Lives: %d  Time: %d:%02d\nScore: %d\n\nPress any key...",
        worldNum, worldName.c_str(), collected, totalTreasures, lives, mins, secs, score);
    overlay_->SetText(buf);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

void GEHud::ShowLost(int worldNum, const std::string& worldName) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    if (gauge_) gauge_->SetVisible(false);
    for (int i = 0; i < kMaxDisplayedLives; ++i)
        if (lifeIcons_[i]) lifeIcons_[i]->SetVisible(false);
    for (int i = 0; i < 3; ++i)
        if (keyIcons_[i]) keyIcons_[i]->SetVisible(false);
    if (livesOverflow_) livesOverflow_->SetVisible(false);

    SetMenuBackground("backgrounds/lost.png");
    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "GAME OVER\n\nWorld %d: %s\n\nPress any key...",
        worldNum, worldName.c_str());
    overlay_->SetText(buf);
    overlay_->SetColor(Color(1.0f, 0.3f, 0.2f));
}

void GEHud::ShowPause(int score, float levelTime) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    if (gauge_) gauge_->SetVisible(false);
    for (int i = 0; i < kMaxDisplayedLives; ++i)
        if (lifeIcons_[i]) lifeIcons_[i]->SetVisible(false);
    for (int i = 0; i < 3; ++i)
        if (keyIcons_[i]) keyIcons_[i]->SetVisible(false);
    if (livesOverflow_) livesOverflow_->SetVisible(false);

    SetMenuBackground("backgrounds/pause.png");
    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;

    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "PAUSED\n\nScore: %d  Time: %d:%02d\n\nESC: resume   S: settings",
        score, mins, secs);
    overlay_->SetText(buf);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

static void HidePlayHud(Simple3D::UI::Image* gauge,
                        Simple3D::UI::Image* lifeIcons[], int nLives,
                        Simple3D::UI::Image* keyIcons[], Simple3D::Label* overflow) {
    if (gauge) gauge->SetVisible(false);
    for (int i = 0; i < nLives; ++i) if (lifeIcons[i]) lifeIcons[i]->SetVisible(false);
    for (int i = 0; i < 3; ++i)     if (keyIcons[i])  keyIcons[i]->SetVisible(false);
    if (overflow) overflow->SetVisible(false);
}

void GEHud::ShowInit(const std::string& text) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    HidePlayHud(gauge_, lifeIcons_, kMaxDisplayedLives, keyIcons_, livesOverflow_);
    SetMenuBackground("backgrounds/init.png");
    overlay_->SetText(text);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

void GEHud::ShowSettings(bool soundOn, bool fromPause) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    HidePlayHud(gauge_, lifeIcons_, kMaxDisplayedLives, keyIcons_, livesOverflow_);
    SetMenuBackground("backgrounds/setup.png");
    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "Settings\n\nSound: %s\n\nS: toggle sound   ESC: %s",
        soundOn ? "ON" : "OFF",
        fromPause ? "back to pause" : "back to menu");
    overlay_->SetText(buf);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

void GEHud::SetVisible(bool visible) {
    if (main_)    main_->SetVisible(visible);
    if (overlay_) overlay_->SetVisible(false);
    if (hint_)    hint_->SetVisible(visible);
    if (!visible) {
        if (gauge_) gauge_->SetVisible(false);
        for (int i = 0; i < kMaxDisplayedLives; ++i)
            if (lifeIcons_[i]) lifeIcons_[i]->SetVisible(false);
        for (int i = 0; i < 3; ++i)
            if (keyIcons_[i]) keyIcons_[i]->SetVisible(false);
        if (livesOverflow_) livesOverflow_->SetVisible(false);
    }
}

} // namespace GESimple3D
