#pragma once

#include <Simple3D/Simple3D.h>
#include <string>

namespace GESimple3D {

class GEHud {
public:
    static constexpr int kMaxDisplayedLives = 5;

    void Create(Simple3D::Game& game);

    void ShowPlay(int worldNum, const std::string& worldName,
                  int lives, int collected, int totalTreasures,
                  int keys49, int keys50, int keys51,
                  float shieldSecs, float levelTime, int score,
                  float gameSpeed, float hintAlpha,
                  float oxygenPct = -1.0f);

    void ShowWin(int worldNum, const std::string& worldName,
                 int collected, int totalTreasures,
                 int lives, int score, float levelTime);

    void ShowLost(int worldNum, const std::string& worldName);
    void ShowPause(int score, float levelTime);
    struct InitSlotInfo { int lives; int world; int best; };
    void ShowInit(const std::string& hint, int selectedSlot,
                  const InitSlotInfo slots[3]);
    void ShowSettings(bool soundOn, bool fromPause);
    void ShowHitFlash();
    void Update(float dt);
    void SetVisible(bool visible);

    void SetMenuBackground(const std::string& bgPath);

    int  GetClickedPauseBtn(const Simple3D::Vector2& mousePos) const;
    void SetPauseBtnHover(int hoverIdx);
    void UpdateMinimap(float worldX, float worldZ);
    void ShowWorldName(int worldNum, const std::string& name);
    void ShowExitOpen();
    void ShowScorePlus(int delta);

private:
    void HideInitLogos();

    Simple3D::UI::Image* menuBg_           = nullptr;
    Simple3D::UI::Image* speedyblupiLogo_  = nullptr;
    Simple3D::UI::Image* blupiyoupieLogo_  = nullptr;
    Simple3D::UI::Image* gamerSlotBtns_[3] = {};
    Simple3D::Label*     slotLabels_[3]    = {};
    Simple3D::UI::Image* initPlayBtn_      = nullptr;
    Simple3D::UI::Image* pauseBtns_[3]    = {};
    Simple3D::UI::Image* winWorldBg_      = nullptr;
    Simple3D::UI::Panel* minimapBg_       = nullptr;
    Simple3D::UI::Panel* minimapDot_      = nullptr;
    Simple3D::Label*     worldNameLabel_   = nullptr;
    Simple3D::UI::Image* playPauseBtn_    = nullptr;
    Simple3D::Label*     exitOpenLabel_   = nullptr;
    Simple3D::Label*     scorePlusLabel_  = nullptr;
    Simple3D::Label*     main_             = nullptr;
    Simple3D::Label*     overlay_          = nullptr;
    Simple3D::Label*     hint_             = nullptr;
    Simple3D::Label*     livesOverflow_    = nullptr;
    Simple3D::UI::Image* gauge_            = nullptr;
    Simple3D::UI::Image* lifeIcons_[kMaxDisplayedLives] = {};
    Simple3D::UI::Image* keyIcons_[3]      = {};
    Simple3D::UI::Panel* hitFlash_         = nullptr;
    float                hitFlashTimer_    = 0.0f;
    float                scoreFlashTimer_  = 0.0f;
    int                  prevScore_        = -1;
    float                worldNameTimer_   = 0.0f;
    float                exitOpenTimer_    = 0.0f;
    float                scorePlusTimer_   = 0.0f;
    float                initAnimTime_     = -1.0f;
    bool                 initAnimActive_   = false;
};

} // namespace GESimple3D
