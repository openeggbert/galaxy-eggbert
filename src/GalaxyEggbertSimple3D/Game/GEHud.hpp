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
                  float gameSpeed, bool showHint);

    void ShowWin(int worldNum, const std::string& worldName,
                 int collected, int totalTreasures,
                 int lives, int score, float levelTime);

    void ShowLost(int worldNum, const std::string& worldName);
    void ShowPause(int score, float levelTime);
    void ShowInit(const std::string& text, int selectedSlot = 0);
    void ShowSettings(bool soundOn, bool fromPause);
    void ShowHitFlash();
    void Update(float dt);
    void SetVisible(bool visible);

    void SetMenuBackground(const std::string& bgPath);

private:
    void HideInitLogos();

    Simple3D::UI::Image* menuBg_           = nullptr;  // full-screen menu background; created first
    Simple3D::UI::Image* speedyblupiLogo_  = nullptr;
    Simple3D::UI::Image* blupiyoupieLogo_  = nullptr;
    Simple3D::UI::Image* gamerSlotBtns_[3] = {};
    Simple3D::Label*     main_             = nullptr;
    Simple3D::Label*     overlay_          = nullptr;
    Simple3D::Label*     hint_             = nullptr;
    Simple3D::Label*     livesOverflow_    = nullptr;
    Simple3D::UI::Image* gauge_            = nullptr;
    Simple3D::UI::Image* lifeIcons_[kMaxDisplayedLives] = {};
    Simple3D::UI::Image* keyIcons_[3]      = {};
    Simple3D::UI::Panel* hitFlash_         = nullptr;
    float                hitFlashTimer_    = 0.0f;
    float                initAnimTime_     = -1.0f;  // -1 = not started
    bool                 initAnimActive_   = false;
};

} // namespace GESimple3D
