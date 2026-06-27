#pragma once

#include <Simple3D/Simple3D.h>

namespace GESimple3D {

// Camera rig for the Simple3D port.
//
// Uses Simple3D Camera orbit mode: RMB drag rotates, scroll zooms.
// Collision avoidance via SetCollisionEnabled(true).
// S3D-1 pass implements the basic orbit setup.
// TODO: Camera shake — Simple3D has no camera-shake API yet.
//       Workaround: offset the camera target slightly via position jitter (S3D-later).
class GECameraRig {
public:
    // Creates the Simple3D camera in orbit mode following the given entity.
    void Create(Simple3D::Game& game, Simple3D::Entity* target);

    // Rotate camera to face the same direction as Blupi (yaw in degrees).
    void SetYaw(float yaw);

    Simple3D::Camera* GetCamera() const { return cam_; }

    void StartShake(float intensity = 0.4f, float duration = 0.3f) {
        if (cam_) cam_->Shake(intensity, duration);
    }

private:
    Simple3D::Camera* cam_   = nullptr;
    float             pitch_ = 20.0f;
};

} // namespace GESimple3D
