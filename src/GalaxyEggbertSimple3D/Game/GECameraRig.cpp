#include "GECameraRig.hpp"

using namespace Simple3D;

namespace GESimple3D {

void GECameraRig::Create(Game& game, Entity* target) {
    cam_ = game.CreateCamera();
    cam_->SetOrbitMode(target, 12.0f);
    cam_->SetOrbitAngles(180.0f, pitch_);
    cam_->SetOrbitPitchLimits(-5.0f, 70.0f);
    cam_->SetOrbitSensitivity(0.3f);
    cam_->SetCollisionEnabled(true);
    cam_->SetFOV(65.0f);
    cam_->SetFarClip(500.0f);
}

void GECameraRig::SetYaw(float yaw) {
    if (cam_) cam_->SetOrbitAngles(yaw + 180.0f, pitch_);
}

} // namespace GESimple3D
