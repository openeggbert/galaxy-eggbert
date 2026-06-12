#pragma once
#include "../GEEngine.hpp"

class HUD {
public:
    explicit HUD(Urho3D::Context* context);
    ~HUD() = default;

    void SetVisible(bool visible);
    void ShowPlay(Urho3D::Vector3 pos, float facingYaw, int lives, int collected);
    void ShowWin();

private:
    Urho3D::Context*                    context_;
    Urho3D::WeakPtr<Urho3D::Text>       text_;
    Urho3D::WeakPtr<Urho3D::BorderImage> gauge_;
};
