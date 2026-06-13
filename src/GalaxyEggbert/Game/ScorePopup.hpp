#pragma once
#include "../GEEngine.hpp"

// Floating score text: rises 1.5 units and fades out over kDuration seconds.
// Call Update(dt); it returns false when the animation is done.
class ScorePopup {
public:
    ScorePopup(Urho3D::Context* ctx, Urho3D::Scene* scene,
               Urho3D::Vector3 pos, const char* text);
    ~ScorePopup();
    bool Update(float dt);

private:
    static constexpr float kDuration = 1.0f;
    static constexpr float kRiseSpeed = 1.5f;

    Urho3D::Node* node_  = nullptr;
    float         timer_ = 0.0f;
    Urho3D::Vector3 origin_;
};
