#pragma once

#include <Simple3D/Simple3D.h>
#include <string>

namespace GESimple3D {

// Text-only HUD for the Simple3D port (S3D-1 pass).
//
// Only Simple3D::Label is used — no images, icons, or gauge sprites.
// TODO(S3D-5): Add gauge sprite, life icons, key icons, hit flash image.
// See docs/SIMPLE3D_GAPS.md: "UI images / gauges / icons / panels".
class GEHud {
public:
    // Creates all label elements. Call once in Game::Start().
    void Create(Simple3D::Game& game);

    // Show the play HUD with current game state.
    void ShowPlay(int worldNum, const std::string& worldName,
                  int lives, int collected, int totalTreasures,
                  int keys49, int keys50, int keys51,
                  float shieldSecs, float levelTime, int score,
                  float gameSpeed, bool showHint);

    // Show the "LEVEL COMPLETE!" screen.
    void ShowWin(int worldNum, const std::string& worldName,
                 int collected, int totalTreasures,
                 int lives, int score, float levelTime);

    // Show the "GAME OVER" screen.
    void ShowLost(int worldNum, const std::string& worldName);

    // Show the "PAUSED" overlay.
    void ShowPause(int score, float levelTime);

    // Show the gamer select prompt (Init phase).
    void ShowInit();

    // Hide all HUD elements.
    void SetVisible(bool visible);

private:
    Simple3D::Label* main_    = nullptr; // primary multi-line HUD text
    Simple3D::Label* overlay_ = nullptr; // phase overlay (Win / Lost / Pause)
    Simple3D::Label* hint_    = nullptr; // controls hint (auto-fades)
};

} // namespace GESimple3D
