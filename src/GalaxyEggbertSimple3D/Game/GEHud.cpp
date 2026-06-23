#include "GEHud.hpp"
#include <cstdio>

using namespace Simple3D;

namespace GESimple3D {

void GEHud::Create(Game& game) {
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
}

void GEHud::ShowPlay(int worldNum, const std::string& worldName,
                     int lives, int collected, int totalTreasures,
                     int keys49, int keys50, int keys51,
                     float shieldSecs, float levelTime, int score,
                     float gameSpeed, bool showHint) {
    if (!main_) return;
    overlay_->SetVisible(false);
    main_->SetVisible(true);
    hint_->SetVisible(showHint);

    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;
    const char* speedTag = (gameSpeed > 1.2f) ? " FAST" : (gameSpeed < 0.8f) ? " SLOW" : "";

    char buf[256];
    if (shieldSecs > 0.0f) {
        std::snprintf(buf, sizeof(buf),
            "World %d: %s | Lives: %d | Treasures: %d/%d | Keys: %d%d%d | SHIELD %.1fs | %d:%02d | Score: %d%s",
            worldNum, worldName.c_str(), lives, collected, totalTreasures,
            keys49, keys50, keys51, shieldSecs, mins, secs, score, speedTag);
        main_->SetColor(shieldSecs < 1.5f ? Color(1.0f, 0.55f, 0.1f) : Color(0.85f, 0.90f, 1.0f));
    } else {
        std::snprintf(buf, sizeof(buf),
            "World %d: %s | Lives: %d | Treasures: %d/%d | Keys: %d%d%d | %d:%02d | Score: %d%s",
            worldNum, worldName.c_str(), lives, collected, totalTreasures,
            keys49, keys50, keys51, mins, secs, score, speedTag);
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

    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;

    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "LEVEL COMPLETE!\n\nWorld %d: %s\nTreasures: %d/%d  Lives: %d  Time: %d:%02d\nScore: %d\n\nPress any key...",
        worldNum, worldName.c_str(), collected, totalTreasures, lives, mins, secs, score);
    overlay_->SetText(buf);
}

void GEHud::ShowLost(int worldNum, const std::string& worldName) {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);

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

    int mins = static_cast<int>(levelTime) / 60;
    int secs = static_cast<int>(levelTime) % 60;

    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "PAUSED\n\nScore: %d  Time: %d:%02d\n\nESC to resume",
        score, mins, secs);
    overlay_->SetText(buf);
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

void GEHud::ShowInit() {
    if (!overlay_) return;
    main_->SetVisible(false);
    hint_->SetVisible(false);
    overlay_->SetVisible(true);
    overlay_->SetText("Galaxy Eggbert\n\nPress 1, 2 or 3 to select gamer slot\nPress Esc to quit");
    overlay_->SetColor(Color(1.0f, 0.92f, 0.4f));
}

void GEHud::SetVisible(bool visible) {
    if (main_)    main_->SetVisible(visible);
    if (overlay_) overlay_->SetVisible(false);
    if (hint_)    hint_->SetVisible(visible);
}

} // namespace GESimple3D
