#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/Worlds/World.hpp"

// Named CameraController to avoid conflict with Urho3D::Camera component.
class CameraController {
public:
    CameraController(Urho3D::Context* context, Urho3D::Scene* scene);
    ~CameraController() = default;

    void Update(float dt, Urho3D::Vector3 targetPos, float targetYaw);
    void SetCollisionWorld(const GalaxyEggbert::Worlds::World* w, int wcx, int wcz) {
        world_ = w; wcx_ = wcx; wcz_ = wcz;
    }
    void StartShake(float intensity = 0.4f, float duration = 0.3f) {
        shakeIntensity_ = intensity; shakeTimer_ = duration; shakeDuration_ = duration;
    }
    Urho3D::Node* GetNode() const { return node_; }

private:
    Urho3D::Context* context_;
    Urho3D::Node*    node_  = nullptr;
    float yaw_        = 180.0f;
    float pitch_      =  20.0f; // = kDefaultPitch
    float dist_       =  12.0f;
    float targetDist_ =  12.0f;
    static constexpr float kDefaultPitch = 20.0f;

    const GalaxyEggbert::Worlds::World* world_ = nullptr;
    int wcx_ = 50;
    int wcz_ = 50;

    float shakeTimer_     = 0.0f;
    float shakeIntensity_ = 0.0f;
    float shakeDuration_  = 0.3f;
};
