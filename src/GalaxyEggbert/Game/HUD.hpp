#pragma once
#include "../GEEngine.hpp"

class HUD {
public:
    explicit HUD(Urho3D::Context* context);
    ~HUD() = default;

    void SetVisible(bool visible);
    void ShowPlay(int lives, int collected, int totalTreasures,
                  int keys49, int keys50, int keys51,
                  float shieldSecs = 0.0f, int world = 1, bool showHint = true,
                  float levelTime = 0.0f, int score = 0, float gameSpeed = 1.0f);
    void ShowWin();
    void ShowHitFlash();
    void ShowWorldIntro(const char* text, float alpha);
    void Update(float dt);

private:
    static constexpr int kMaxDisplayedLives = 5;

    Urho3D::Context*                     context_;
    Urho3D::WeakPtr<Urho3D::Text>        text_;
    Urho3D::WeakPtr<Urho3D::Text>        livesOverflow_;
    Urho3D::WeakPtr<Urho3D::BorderImage> gauge_;
    Urho3D::WeakPtr<Urho3D::BorderImage> lifeIcons_[kMaxDisplayedLives];
    Urho3D::WeakPtr<Urho3D::BorderImage> keyIcons_[3];
    Urho3D::WeakPtr<Urho3D::BorderImage> hitFlash_;
    Urho3D::WeakPtr<Urho3D::Text>        worldIntroText_;
    float hitFlashTimer_  = 0.0f;
};
