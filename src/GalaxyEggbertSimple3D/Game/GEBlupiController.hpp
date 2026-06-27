#pragma once

#include <Simple3D/Simple3D.h>
#include <cstdint>

namespace GESimple3D {

// Blupi animation/movement state — values match mobile-eggbert BlupiAction IDs.
enum class BlupiState : uint8_t {
    Stop     = 1,
    March    = 2,
    Jump     = 4,
    Air      = 5,
    Down     = 6,  // crouch (LShift)
    Up       = 7,  // look-up / glide (RShift)
    SwimIdle = 18, // StopNage — treading water
    SwimMove = 19, // MarchNage — swimming forward
};

class GEBlupiController {
public:
    static constexpr float kMoveSpeed  = 5.5f;
    static constexpr float kTurnSpeed  = 180.0f;
    static constexpr float kJumpSpeed  = 12.0f;
    static constexpr float kGravity    = 25.0f;
    static constexpr float kFallLimit  = -10.0f;

    // blupi.png: 60×60 px tiles, 10 columns per row
    static constexpr int   kTilePx  = 60;
    static constexpr int   kCols    = 10;
    static constexpr float kHalfH   = 23.0f / 64.0f;
    static constexpr float kVisHalf = 60.0f / 64.0f / 2.0f;

    void Create(Simple3D::Game& game);
    void Update(Simple3D::Game& game, float dt);
    void Respawn();
    // Upward bounce after stomping an enemy (inspired by mobile-eggbert BlupiStep bounce).
    void BounceUp();
    // Launch Blupi upward at arbitrary speed (spring, catapult, etc.).
    void Launch(float ySpeed);

    void SetSpawnPoint(const Simple3D::Vector3& pos) { spawn_ = pos; }

    Simple3D::Entity* GetEntity()    const { return player_; }
    Simple3D::Vector3 GetPosition()  const;
    float             GetFacingYaw() const { return yaw_; }
    bool              IsOnGround()   const;
    float             GetVelY()      const;

    void  SetShieldTimer(float t) { shieldTimer_ = t; }
    float GetShieldTimer()  const { return shieldTimer_; }
    bool  IsShieldActive()  const { return shieldTimer_ > 0.0f; }

    void SetSwimming(bool sw) { swimming_ = sw; }
    bool IsSwimming()   const { return swimming_; }

    void SetInputFrozen(bool f) { inputFrozen_ = f; }

    bool WasLandedThisFrame()  const { return landedThisFrame_;  }
    bool WasJumpedThisFrame()  const { return jumpedThisFrame_;  }
    bool WasSteppedThisFrame() const { return steppedThisFrame_; }

    BlupiState GetState() const { return state_; }

    void SetSpriteVisible(bool v);

private:
    void UpdateState(bool grounded, bool moving, bool jumpTriggered,
                     bool crouchHeld, bool lookUpHeld, float stickX);
    void AdvanceAnim(float dt);
    void ApplySprite();

    Simple3D::Entity*              player_    = nullptr;
    Simple3D::Entity*              sprite_    = nullptr;
    Simple3D::CharacterController* cc_        = nullptr;
    Simple3D::Vector3              spawn_     = {0.0f, 0.86f, 0.0f};

    float      yaw_            = 0.0f;
    float      shieldTimer_    = 0.0f;
    bool       inputFrozen_    = false;
    bool       wasGrounded_     = false;
    bool       landedThisFrame_ = false;
    bool       jumpedThisFrame_ = false;
    bool       steppedThisFrame_= false;
    bool       facingRight_    = false;
    bool       swimming_       = false;

    BlupiState state_          = BlupiState::Stop;
    int        animPhase_      = 0;   // index into current state's frame table
    float      animTimer_      = 0.0f;
};

} // namespace GESimple3D
