#include "Camera.hpp"
#include <cmath>
#include <algorithm>
#include <cstdint>

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
    } else {
        pitch_ += (kDefaultPitch - pitch_) * std::min(1.0f, 2.0f * dt);
    }
    dist_ -= static_cast<float>(input->GetMouseMoveWheel()) * 1.5f;
    dist_  = std::max(3.0f, std::min(40.0f, dist_));

    float yawRad   = yaw_   * static_cast<float>(M_PI) / 180.0f;
    float pitchRad = pitch_ * static_cast<float>(M_PI) / 180.0f;
    Vector3 offset(
        dist_ * std::sin(yawRad) * std::cos(pitchRad),
        dist_ * std::sin(pitchRad),
        dist_ * std::cos(yawRad) * std::cos(pitchRad));

    // Wall collision: step ray from Blupi toward the desired camera position;
    // if a solid voxel is encountered, clamp the camera distance before it.
    if (world_) {
        Vector3 rayOrigin = targetPos + Vector3(0.0f, 0.5f, 0.0f);
        float   idealDist = offset.Length();
        Vector3 dir       = offset / idealDist;
        float   hitDist   = idealDist;
        static constexpr float kStep = 0.3f;
        for (float t = 0.5f; t < idealDist; t += kStep) {
            Vector3 p  = rayOrigin + dir * t;
            int     wx = static_cast<int>(std::round(p.x_)) + wcx_;
            int     wy = static_cast<int>(std::floor(p.y_));
            int     wz = static_cast<int>(std::round(p.z_)) + wcz_;
            if (wx >= 0 && wx < 100 && wz >= 0 && wz < 100 && wy >= 0 && wy < 100) {
                if (!world_->getBlock(
                        static_cast<uint16_t>(wx),
                        static_cast<uint16_t>(wy),
                        static_cast<uint16_t>(wz)).isAir()) {
                    hitDist = std::max(1.5f, t - kStep);
                    break;
                }
            }
        }
        if (hitDist < idealDist)
            offset = dir * hitDist;
    }

    node_->SetPosition(targetPos + offset);
    node_->LookAt(targetPos + Vector3(0.0f, 0.5f, 0.0f));

    (void)dt;
}
