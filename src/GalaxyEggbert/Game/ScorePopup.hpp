#pragma once
#include "../GEEngine.hpp"

// Floating score text: rises 1.5 units and fades out over kDuration seconds.
// Call Update(dt); it returns false when the animation is done.
class ScorePopup {
public:
    ScorePopup(Urho3D::Context* ctx, Urho3D::Scene* scene,
               Urho3D::Vector3 pos, const char* text,
               Urho3D::Color color = Urho3D::Color(1.0f, 0.95f, 0.2f, 1.0f));
    ~ScorePopup();
    bool Update(float dt);

private:
    static constexpr float kDuration = 1.0f;
    static constexpr float kRiseSpeed = 1.5f;

    Urho3D::Node*   node_   = nullptr;
    Urho3D::Color   color_;
    float           timer_  = 0.0f;
    Urho3D::Vector3 origin_;
};
