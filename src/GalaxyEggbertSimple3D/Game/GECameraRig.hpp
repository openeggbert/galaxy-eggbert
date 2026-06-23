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

    Simple3D::Camera* GetCamera() const { return cam_; }

    // Camera shake — no-op in S3D-1 (Simple3D lacks shake API).
    // TODO: Implement via position jitter when Simple3D supports it.
    void StartShake(float /*intensity*/ = 0.4f, float /*duration*/ = 0.3f) {}

private:
    Simple3D::Camera* cam_ = nullptr;
};

} // namespace GESimple3D
