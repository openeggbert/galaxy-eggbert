#include "Camera.hpp"
#include <cmath>
#include <algorithm>

using namespace Urho3D;

CameraController::CameraController(Context* context, Scene* scene)
    : context_(context)
{
    node_ = scene->CreateChild("Camera");
    auto* cam = node_->CreateComponent<Camera>();
    cam->SetNearClip(0.1f);
    cam->SetFarClip(300.0f);
    auto* renderer = context_->GetSubsystem<Renderer>();
    SharedPtr<Viewport> vp(new Viewport(context_, scene, cam));
    renderer->SetViewport(0, vp);
    renderer->SetDrawShadows(false);
}

void CameraController::Update(float dt, Vector3 targetPos, float targetYaw) {
    auto* input = context_->GetSubsystem<Input>();

    float target = targetYaw + 180.0f;
    float diff = target - yaw_;
    while (diff >  180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    yaw_ += diff * std::min(1.0f, 8.0f * dt);

    if (input->GetMouseButtonDown(MOUSEB_RIGHT)) {
        pitch_ += 0.12f * static_cast<float>(input->GetMouseMoveY());
        pitch_  = std::max(-60.0f, std::min(60.0f, pitch_));
    }
    dist_ -= static_cast<float>(input->GetMouseMoveWheel()) * 1.5f;
    dist_  = std::max(3.0f, std::min(40.0f, dist_));

    float yawRad   = yaw_   * static_cast<float>(M_PI) / 180.0f;
    float pitchRad = pitch_ * static_cast<float>(M_PI) / 180.0f;
    Vector3 offset(
        dist_ * std::sin(yawRad) * std::cos(pitchRad),
        dist_ * std::sin(pitchRad),
        dist_ * std::cos(yawRad) * std::cos(pitchRad));
    node_->SetPosition(targetPos + offset);
    node_->LookAt(targetPos + Vector3(0.0f, 0.5f, 0.0f));

    (void)dt;
}
