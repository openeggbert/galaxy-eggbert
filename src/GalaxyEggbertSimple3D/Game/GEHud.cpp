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
static constexpr int   kSlotY[3]     = { 259, 364, 469 };

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
    // Created first so they render behind all other HUD elements.
    menuBg_ = game.CreateImage("");
    menuBg_->SetSize(1280, 720);
    menuBg_->SetAnchor(Anchor::TopLeft);
    menuBg_->SetPosition(0, 0);
    menuBg_->SetVisible(false);

    // speedyblupi.png: 640×160 → drawn at 1280×240, slides in from above.
    speedyblupiLogo_ = game.CreateImage("backgrounds/speedyblupi.png");
    speedyblupiLogo_->SetAnchor(Anchor::TopLeft);
    speedyblupiLogo_->SetSize(1280, 240);
    speedyblupiLogo_->SetPosition(0, -240);
    speedyblupiLogo_->SetVisible(false);

    // blupiyoupie.png: 410×380 → drawn scaled ×2/×1.5, centered at (936,420).
    // Screen centre is (640,360); offset = (296,60).
    blupiyoupieLogo_ = game.CreateImage("backgrounds/blupiyoupie.png");
    blupiyoupieLogo_->SetAnchor(Anchor::Center);
    blupiyoupieLogo_->SetPosition(296, 60);
    blupiyoupieLogo_->SetSize(820, 570);
    blupiyoupieLogo_->SetColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
    blupiyoupieLogo_->SetVisible(false);

    // Gamer slot buttons: pad.png, cell 140×140.
    // Positions scaled from 640×480 → 1280×720 (factor 2×/1.5×).
    // buttonSizeFactor2 = 720*140/480 = 210; size = 105×105.
    for (int i = 0; i < 3; ++i) {
        gamerSlotBtns_[i] = game.CreateImage("icons/pad.png");
        gamerSlotBtns_[i]->SetImageRect((4 + i) * 140, 0, 140, 140);
        gamerSlotBtns_[i]->SetSize(105, 105);
        gamerSlotBtns_[i]->SetAnchor(Anchor::TopLeft);
        gamerSlotBtns_[i]->SetPosition(20, kSlotY[i]);
        gamerSlotBtns_[i]->SetVisible(false);
    }

    // Slot info labels: right of each slot button, show lives/world/best per slot.
    for (int i = 0; i < 3; ++i) {
        slotLabels_[i] = game.CreateLabel("");
        slotLabels_[i]->SetPosition(135, kSlotY[i] + 15);
        slotLabels_[i]->SetFontSize(14);
        slotLabels_[i]->SetColor(Color(1.0f, 0.92f, 0.4f));
        slotLabels_[i]->SetVisible(false);
    }

    // InitPlay button: pad.png icon 7 (col7*140=980, row0).
    initPlayBtn_ = game.CreateImage("icons/pad.png");
    initPlayBtn_->SetImageRect(980, 0, 140, 140);
    initPlayBtn_->SetSize(210, 210);
    initPlayBtn_->SetAnchor(Anchor::TopLeft);
    initPlayBtn_->SetPosition(1050, 310);
    initPlayBtn_->SetVisible(false);

    // Pause icon buttons: Resume (icon 10), Settings (icon 19), Quit (icon 11).
    // pad.png: 8 cols×3 rows, 140×140; icon N → col=N%8, row=N/8.
    static const int kPausePadIcon[3] = { 10, 19, 11 };
    for (int i = 0; i < 3; ++i) {
        pauseBtns_[i] = game.CreateImage("icons/pad.png");
        int col = kPausePadIcon[i] % 8;
        int row = kPausePadIcon[i] / 8;
        pauseBtns_[i]->SetImageRect(col * 140, row * 140, 140, 140);
        pauseBtns_[i]->SetSize(105, 105);
        pauseBtns_[i]->SetAnchor(Anchor::TopLeft);
        pauseBtns_[i]->SetPosition(462 + i * 125, 470);
        pauseBtns_[i]->SetVisible(false);
    }

    // Minimap: 100×100 panel at bottom-right; Blupi dot inside it.
    minimapBg_ = game.CreatePanel();
    minimapBg_->SetColor(Color(0.0f, 0.0f, 0.0f, 0.6f));
    minimapBg_->SetSize(100, 100);
    minimapBg_->SetAnchor(Anchor::TopLeft);
    minimapBg_->SetPosition(1170, 610);
    minimapBg_->SetVisible(false);

    minimapDot_ = game.CreatePanel();
    minimapDot_->SetColor(Color(1.0f, 0.9f, 0.2f, 1.0f));
    minimapDot_->SetSize(4, 4);
    minimapDot_->SetAnchor(Anchor::TopLeft);
    minimapDot_->SetPosition(1218, 658);  // default: world centre
    minimapDot_->SetVisible(false);

    // World thumbnail shown on win screen (top-right corner).
    winWorldBg_ = game.CreateImage("");
    winWorldBg_->SetSize(340, 191);
    winWorldBg_->SetAnchor(Anchor::TopLeft);
    winWorldBg_->SetPosition(900, 250);
    winWorldBg_->SetVisible(false);

    // World name banner: large centred text, fades out after 2.5 s (HUD-040).
    worldNameLabel_ = game.CreateLabel("");
    worldNameLabel_->SetPosition(640, 300);
    worldNameLabel_->SetFontSize(28);
    worldNameLabel_->SetColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
    worldNameLabel_->SetVisible(false);

    // "EXIT OPEN!" banner (HUD-020/026): green, 3 s fade.
    exitOpenLabel_ = game.CreateLabel("EXIT OPEN!");
    exitOpenLabel_->SetPosition(640, 200);
    exitOpenLabel_->SetFontSize(40);
    exitOpenLabel_->SetColor(Color(0.3f, 1.0f, 0.3f, 0.0f));
    exitOpenLabel_->SetVisible(false);

    // Score "+N" popup (HUD-025): yellow, 0.8 s fade.
    scorePlusLabel_ = game.CreateLabel("");
    scorePlusLabel_->SetPosition(640, 340);
    scorePlusLabel_->SetFontSize(24);
    scorePlusLabel_->SetColor(Color(1.0f, 1.0f, 0.3f, 0.0f));
    scorePlusLabel_->SetVisible(false);

    // Pause icon during Play (HUD-022): pad.png icon 10, top-right corner.
    playPauseBtn_ = game.CreateImage("icons/pad.png");
    playPauseBtn_->SetImageRect(2 * 140, 1 * 140, 140, 140);  // icon 10: col2, row1
    playPauseBtn_->SetSize(36, 36);
    playPauseBtn_->SetAnchor(Anchor::TopLeft);
    playPauseBtn_->SetPosition(1234, 8);
    playPauseBtn_->SetVisible(false);

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

void GEHud::HideInitLogos() {
    initAnimTime_   = -1.0f;
    initAnimActive_ = false;
    if (speedyblupiLogo_)  speedyblupiLogo_->SetVisible(false);
    if (blupiyoupieLogo_)  blupiyoupieLogo_->SetVisible(false);
    for (int i = 0; i < 3; ++i) {
        if (gamerSlotBtns_[i]) gamerSlotBtns_[i]->SetVisible(false);
        if (slotLabels_[i])    slotLabels_[i]->SetVisible(false);
    }
    if (initPlayBtn_) initPlayBtn_->SetVisible(false);
    for (int i = 0; i < 3; ++i) if (pauseBtns_[i]) pauseBtns_[i]->SetVisible(false);
    if (winWorldBg_)     winWorldBg_->SetVisible(false);
    if (minimapBg_)      minimapBg_->SetVisible(false);
    if (minimapDot_)     minimapDot_->SetVisible(false);
    if (worldNameLabel_) worldNameLabel_->SetVisible(false);
    worldNameTimer_ = 0.0f;
    if (exitOpenLabel_)  { exitOpenLabel_->SetVisible(false);  exitOpenTimer_  = 0.0f; }
    if (scorePlusLabel_) { scorePlusLabel_->SetVisible(false); scorePlusTimer_ = 0.0f; }
    if (playPauseBtn_)   playPauseBtn_->SetVisible(false);
    // Reset overlay position for non-init screens.
    if (overlay_) overlay_->SetPosition(12, 12);
}

void GEHud::Update(float dt) {
    // Init logo animation: speedyblupi slides from top, blupiyoupie scales in.
    if (initAnimActive_) {
        initAnimTime_ += dt;
        float t = std::min(initAnimTime_ / 1.0f, 1.0f);

        // speedyblupi: ease-out quadratic slide down from y=-240
        float eased = 1.0f - (1.0f - t) * (1.0f - t);
        int yPos = static_cast<int>(-240.0f + eased * 240.0f);
        if (speedyblupiLogo_) speedyblupiLogo_->SetPosition(0, yPos);

        // blupiyoupie: scale from 0.5→1.0, opacity from 0.25→1.0
        float scale   = 0.5f + t * 0.5f;
        float opacity = std::min(scale * scale, 1.0f);
        int w = static_cast<int>(820.0f * scale);
        int h = static_cast<int>(570.0f * scale);
        if (blupiyoupieLogo_) {
            blupiyoupieLogo_->SetSize(w, h);
            blupiyoupieLogo_->SetColor(Color(1.0f, 1.0f, 1.0f, opacity));
        }

        if (t >= 1.0f) initAnimActive_ = false;
    }

    if (scoreFlashTimer_ > 0.0f) scoreFlashTimer_ -= dt;

    // World name banner fade (HUD-040): 0.4s fade-in, hold, 0.6s fade-out.
    if (worldNameTimer_ > 0.0f) {
        worldNameTimer_ -= dt;
        static constexpr float kTotal = 2.5f, kFadeIn = 0.4f, kFadeOut = 0.6f;
        float alpha;
        if (worldNameTimer_ > kTotal - kFadeIn)
            alpha = 1.0f - (worldNameTimer_ - (kTotal - kFadeIn)) / kFadeIn;
        else if (worldNameTimer_ < kFadeOut)
            alpha = worldNameTimer_ / kFadeOut;
        else
            alpha = 1.0f;
        if (worldNameLabel_) worldNameLabel_->SetColor(Color(1.0f, 1.0f, 0.9f, alpha));
        if (worldNameTimer_ <= 0.0f && worldNameLabel_) worldNameLabel_->SetVisible(false);
    }

    // EXIT OPEN! banner fade (HUD-020/026): 0.3s in, hold, 0.5s out.
    if (exitOpenTimer_ > 0.0f) {
        exitOpenTimer_ -= dt;
        static constexpr float kETotal = 3.0f, kEIn = 0.3f, kEOut = 0.5f;
        float alpha;
        if (exitOpenTimer_ > kETotal - kEIn)
            alpha = 1.0f - (exitOpenTimer_ - (kETotal - kEIn)) / kEIn;
        else if (exitOpenTimer_ < kEOut)
            alpha = exitOpenTimer_ / kEOut;
        else
            alpha = 1.0f;
        if (exitOpenLabel_) exitOpenLabel_->SetColor(Color(0.3f, 1.0f, 0.3f, alpha));
        if (exitOpenTimer_ <= 0.0f && exitOpenLabel_) exitOpenLabel_->SetVisible(false);
    }

    // Score "+N" popup fade (HUD-025): 0.8 s linear fade-out.
    if (scorePlusTimer_ > 0.0f) {
        scorePlusTimer_ -= dt;
        float alpha = std::max(0.0f, scorePlusTimer_ / 0.8f);
        if (scorePlusLabel_) scorePlusLabel_->SetColor(Color(1.0f, 1.0f, 0.3f, alpha));
        if (scorePlusTimer_ <= 0.0f && scorePlusLabel_) scorePlusLabel_->SetVisible(false);
    }

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
                     float gameSpeed, float hintAlpha,
                     float oxygenPct) {
    if (!main_) return;
    SetMenuBackground("");
    HideInitLogos();
    overlay_->SetVisible(false);
    main_->SetVisible(true);
    if (playPauseBtn_) playPauseBtn_->SetVisible(true);
    if (hintAlpha <= 0.0f) {
        if (hint_) hint_->SetVisible(false);
    } else {
        if (hint_) {
            hint_->SetVisible(true);
            hint_->SetColor(Color(0.75f, 0.85f, 0.75f, hintAlpha));
        }
    }

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
    if (oxygenPct >= 0.0f) {
        char o2[32];
        std::snprintf(o2, sizeof(o2), "  O2:%d%%", static_cast<int>(oxygenPct * 100.0f));
        std::strncat(buf, o2, sizeof(buf) - std::strlen(buf) - 1);
    }
    main_->SetText(buf);
    if (score != prevScore_) { prevScore_ = score; scoreFlashTimer_ = 0.5f; }
    if (scoreFlashTimer_ > 0.0f)
        main_->SetColor(Color(1.0f, 1.0f, 0.15f));  // bright yellow flash on score change
    if (minimapBg_)  minimapBg_->SetVisible(true);
    if (minimapDot_) minimapDot_->SetVisible(true);
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
    HideInitLogos();
    overlay_->SetPosition(12, 260);
    if (winWorldBg_) {
        char decorPath[64];
        std::snprintf(decorPath, sizeof(decorPath), "backgrounds/decor%03d.png", worldNum - 1);
        winWorldBg_->SetTexture(decorPath);
        winWorldBg_->SetVisible(true);
    }
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
    HideInitLogos();
    overlay_->SetPosition(12, 260);
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
    HideInitLogos();
    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;

    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "PAUSED\n\nScore: %d  Time: %d:%02d\n\nESC: resume   S: settings   Q: quit",
        score, mins, secs);
    overlay_->SetText(buf);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
    for (int i = 0; i < 3; ++i) if (pauseBtns_[i]) pauseBtns_[i]->SetVisible(true);
}

static void HidePlayHud(Simple3D::UI::Image* gauge,
                        Simple3D::UI::Image* lifeIcons[], int nLives,
                        Simple3D::UI::Image* keyIcons[], Simple3D::Label* overflow) {
    if (gauge) gauge->SetVisible(false);
    for (int i = 0; i < nLives; ++i) if (lifeIcons[i]) lifeIcons[i]->SetVisible(false);
    for (int i = 0; i < 3; ++i)     if (keyIcons[i])  keyIcons[i]->SetVisible(false);
    if (overflow) overflow->SetVisible(false);
}

void GEHud::ShowInit(const std::string& hint, int selectedSlot,
                     const InitSlotInfo slots[3]) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    HidePlayHud(gauge_, lifeIcons_, kMaxDisplayedLives, keyIcons_, livesOverflow_);
    for (int i = 0; i < 3; ++i) if (pauseBtns_[i]) pauseBtns_[i]->SetVisible(false);
    if (winWorldBg_)     winWorldBg_->SetVisible(false);
    if (minimapBg_)      minimapBg_->SetVisible(false);
    if (minimapDot_)     minimapDot_->SetVisible(false);
    if (worldNameLabel_) worldNameLabel_->SetVisible(false);
    worldNameTimer_ = 0.0f;
    if (playPauseBtn_)   playPauseBtn_->SetVisible(false);
    if (exitOpenLabel_)  { exitOpenLabel_->SetVisible(false);  exitOpenTimer_  = 0.0f; }
    if (scorePlusLabel_) { scorePlusLabel_->SetVisible(false); scorePlusTimer_ = 0.0f; }
    SetMenuBackground("backgrounds/init.png");
    // Start logo animation only on first entry; called every frame from UpdateInit.
    if (initAnimTime_ < 0.0f) {
        initAnimTime_   = 0.0f;
        initAnimActive_ = true;
        if (speedyblupiLogo_) { speedyblupiLogo_->SetPosition(0, -240); speedyblupiLogo_->SetVisible(true); }
        if (blupiyoupieLogo_) { blupiyoupieLogo_->SetSize(410, 285);    blupiyoupieLogo_->SetColor(Color(1,1,1,0)); blupiyoupieLogo_->SetVisible(true); }
        for (int i = 0; i < 3; ++i) {
            if (gamerSlotBtns_[i]) gamerSlotBtns_[i]->SetVisible(true);
            if (slotLabels_[i])    slotLabels_[i]->SetVisible(true);
        }
        if (initPlayBtn_) initPlayBtn_->SetVisible(true);
    }
    // Update selected gamer slot highlight (normal icon 4/5/6, selected 16/17/18).
    for (int i = 0; i < 3; ++i) {
        if (!gamerSlotBtns_[i]) continue;
        bool sel = (i == selectedSlot);
        int ix = sel ? i * 140 : (4 + i) * 140;
        int iy = sel ? 280 : 0;
        gamerSlotBtns_[i]->SetImageRect(ix, iy, 140, 140);
    }
    // Update per-slot info labels (MENU-010).
    static const char* kName[] = { "Gamer A", "Gamer B", "Gamer C" };
    for (int i = 0; i < 3; ++i) {
        if (!slotLabels_[i]) continue;
        char buf[128];
        std::snprintf(buf, sizeof(buf),
            "%s\nLives: %d  World: %d  Best: %d",
            kName[i], slots[i].lives, slots[i].world, slots[i].best);
        slotLabels_[i]->SetText(buf);
    }
    overlay_->SetPosition(12, 640);  // bottom strip — clear of logos and slot buttons
    overlay_->SetText(hint);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

void GEHud::ShowSettings(bool soundOn, bool fromPause) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    HidePlayHud(gauge_, lifeIcons_, kMaxDisplayedLives, keyIcons_, livesOverflow_);
    SetMenuBackground("backgrounds/setup.png");
    HideInitLogos();
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

void GEHud::SetPauseBtnHover(int hoverIdx) {
    for (int i = 0; i < 3; ++i) {
        if (!pauseBtns_[i]) continue;
        float alpha = (hoverIdx < 0 || i == hoverIdx) ? 1.0f : 0.5f;
        pauseBtns_[i]->SetColor(Color(1.0f, 1.0f, 1.0f, alpha));
    }
}

void GEHud::UpdateMinimap(float worldX, float worldZ) {
    if (!minimapDot_) return;
    // World coords ∈ [-50,50]; map to minimap pixel [0,100]. Clamp to border.
    int px = static_cast<int>(worldX + 50.0f);
    int pz = static_cast<int>(worldZ + 50.0f);
    if (px < 0) px = 0; else if (px > 96) px = 96;
    if (pz < 0) pz = 0; else if (pz > 96) pz = 96;
    minimapDot_->SetPosition(1170 + px, 610 + pz);
}

int GEHud::GetClickedPauseBtn(const Simple3D::Vector2& mousePos) const {
    for (int i = 0; i < 3; ++i) {
        if (!pauseBtns_[i] || !pauseBtns_[i]->IsVisible()) continue;
        auto pos  = pauseBtns_[i]->GetPosition();
        auto size = pauseBtns_[i]->GetSize();
        if (mousePos.x_ >= pos.x_ && mousePos.x_ < pos.x_ + size.x_ &&
            mousePos.y_ >= pos.y_ && mousePos.y_ < pos.y_ + size.y_)
            return i;
    }
    return -1;
}

void GEHud::ShowExitOpen() {
    if (!exitOpenLabel_) return;
    exitOpenLabel_->SetColor(Color(0.3f, 1.0f, 0.3f, 0.0f));
    exitOpenLabel_->SetVisible(true);
    exitOpenTimer_ = 3.0f;
}

void GEHud::ShowScorePlus(int delta) {
    if (!scorePlusLabel_) return;
    char buf[16];
    std::snprintf(buf, sizeof(buf), "+%d", delta);
    scorePlusLabel_->SetText(buf);
    scorePlusLabel_->SetColor(Color(1.0f, 1.0f, 0.3f, 1.0f));
    scorePlusLabel_->SetVisible(true);
    scorePlusTimer_ = 0.8f;
}

void GEHud::ShowWorldName(int worldNum, const std::string& name) {
    if (!worldNameLabel_) return;
    char buf[128];
    std::snprintf(buf, sizeof(buf), "World %d: %s", worldNum, name.c_str());
    worldNameLabel_->SetText(buf);
    worldNameLabel_->SetColor(Color(1.0f, 1.0f, 0.9f, 0.0f));
    worldNameLabel_->SetVisible(true);
    worldNameTimer_ = 2.5f;
}

} // namespace GESimple3D
