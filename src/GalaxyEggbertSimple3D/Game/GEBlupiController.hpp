#pragma once

#include <Simple3D/Simple3D.h>

namespace GESimple3D {

// Blupi character controller for the Simple3D port.
//
// S3D-1 pass: physics via Simple3D CharacterController, tank-control movement,
// jump, glide placeholder, respawn on fall. No sprite animation yet.
// TODO(S3D-3): Add billboard sprite animation from blupi.png.
// TODO: Shield timer, crouch, death freeze — stubbed below.
class GEBlupiController {
public:
    static constexpr float kMoveSpeed  = 5.5f;
    static constexpr float kTurnSpeed  = 180.0f;
    static constexpr float kJumpSpeed  = 12.0f;
    static constexpr float kGravity    = 25.0f;
    static constexpr float kFallLimit  = -10.0f; // respawn below this Y

    // Creates the player entity and CharacterController.
    void Create(Simple3D::Game& game);

    // Reads input and updates the character. Call every frame from Game::Update.
    void Update(Simple3D::Game& game, float dt);

    // Teleports Blupi to the spawn point.
    void Respawn();

    void SetSpawnPoint(const Simple3D::Vector3& pos) { spawn_ = pos; }

    Simple3D::Entity*   GetEntity()       const { return player_; }
    Simple3D::Vector3   GetPosition()     const;
    float               GetFacingYaw()    const { return yaw_; }
    bool                IsOnGround()      const;
    float               GetVelY()         const;

    void SetShieldTimer(float t) { shieldTimer_ = t; }
    float GetShieldTimer()  const { return shieldTimer_; }
    bool  IsShieldActive()  const { return shieldTimer_ > 0.0f; }

    void SetInputFrozen(bool f) { inputFrozen_ = f; }

    // True for one frame after landing
    bool WasLandedThisFrame() const { return landedThisFrame_; }
    // True for one frame after the jump key was pressed
    bool WasJumpedThisFrame() const { return jumpedThisFrame_; }

private:
    Simple3D::Entity*             player_       = nullptr;
    Simple3D::CharacterController* cc_          = nullptr;
    Simple3D::Vector3             spawn_        = {0.0f, 0.86f, 0.0f};
    float yaw_             = 0.0f;
    float shieldTimer_     = 0.0f;
    bool  inputFrozen_     = false;
    bool  wasGrounded_     = false;
    bool  landedThisFrame_ = false;
    bool  jumpedThisFrame_ = false;
};

} // namespace GESimple3D
