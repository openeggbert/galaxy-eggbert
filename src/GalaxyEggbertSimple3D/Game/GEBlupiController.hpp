#pragma once

#include <Simple3D/Simple3D.h>

namespace GESimple3D {

// Blupi character controller for the Simple3D port.
// TODO: crouch, death freeze — stubbed below.
class GEBlupiController {
public:
    static constexpr float kMoveSpeed  = 5.5f;
    static constexpr float kTurnSpeed  = 180.0f;
    static constexpr float kJumpSpeed  = 12.0f;
    static constexpr float kGravity    = 25.0f;
    static constexpr float kFallLimit  = -10.0f;

    // blupi.png sprite sheet constants (600×2040 px, 60×60 tiles, 10 cols)
    static constexpr float kSheetW  = 600.0f;
    static constexpr float kSheetH  = 2040.0f;
    static constexpr int   kTilePx  = 60;
    static constexpr int   kCols    = 10;
    static constexpr float kHalfH   = 23.0f / 64.0f;   // physics half-height
    static constexpr float kVisHalf = 60.0f / 64.0f / 2.0f; // billboard visual half

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

    bool WasLandedThisFrame() const { return landedThisFrame_; }
    bool WasJumpedThisFrame() const { return jumpedThisFrame_; }

    void SetSpriteVisible(bool v);

private:
    void UpdateSprite(bool moving);

    Simple3D::Entity*             player_       = nullptr;
    Simple3D::Entity*             sprite_       = nullptr;
    Simple3D::CharacterController* cc_          = nullptr;
    Simple3D::Vector3             spawn_        = {0.0f, 0.86f, 0.0f};
    float yaw_             = 0.0f;
    float shieldTimer_     = 0.0f;
    int   animTick_        = 0;
    bool  inputFrozen_     = false;
    bool  wasGrounded_     = false;
    bool  landedThisFrame_ = false;
    bool  jumpedThisFrame_ = false;
};

} // namespace GESimple3D
