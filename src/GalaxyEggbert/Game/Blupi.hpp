#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/Worlds/World.hpp"
#include "GalaxyEggbert/def/BlupiAction.hpp"

// Blupi character: physics, movement, collision, and sprite animation.
//
// Physics derived from mobile-eggbert Decor.cpp: m_blupiVitesseX/Y velocity
// integration, gravity constant, jump impulse, and tile-blocking collision logic.
// Scaled from 64-px tile units to 1-unit 3D voxel coordinates.
//
// Sprite animation mirrors Decor.cpp BlupiAnimate() via Tables::GetBlupiIcon().
// Icons index into icons/blupi.png (600x2040, 60x60 tiles, 10 cols x 34 rows).
class Blupi {
public:
    static constexpr float kGravity   = -22.0f;
    static constexpr float kJumpSpeed =  10.0f;
    static constexpr float kMoveSpeed =   5.5f;
    static constexpr float kTurnSpeed = 180.0f; // degrees/second
    static constexpr float kHalfW     =  0.35f;
    // 2D tile = 64 px, Blupi height = 46 px → 3D height = 46/64 units, half = 23/64.
    static constexpr float kHalfH     =  23.0f / 64.0f; // ≈ 0.359
    // Grace period after walking off an edge where jump still fires (coyote time).
    static constexpr float kCoyoteTime  = 0.12f;
    // Queued jump window: jump pressed this many seconds before landing still fires.
    static constexpr float kJumpBuffer  = 0.12f;
    // Releasing jump early cuts the arc to this velocity (short hop).
    static constexpr float kMinJumpSpeed = kJumpSpeed * 0.30f;

    explicit Blupi(Urho3D::Context* context,
                   Urho3D::Scene*   scene,
                   const GalaxyEggbert::Worlds::World* world,
                   int wcx, int wcz);
    ~Blupi();

    void Update(float dt);
    void Respawn();
    void SetSpawnPoint(Urho3D::Vector3 pos) { spawn_ = pos; }
    void ApplyExternalDelta(Urho3D::Vector3 delta) {
        if (node_) node_->SetPosition(node_->GetPosition() + delta);
    }
    bool WasFallDeath()        const { return fallDeath_; }
    void ClearFallDeath()            { fallDeath_ = false; }
    bool  WasLandedThisFrame() const { return landedThisFrame_; }
    float GetVelY()            const { return vel_.y_; }
    void  StartFlash(float duration) { flashTimer_ = duration; flashTickTimer_ = 0.0f; }
    void  Bounce() { vel_.y_ = kJumpSpeed * 0.6f; onGround_ = false; }
    void  SnapToSurface(float surfaceY);
    void  SetShieldActive(bool active) { shieldActive_ = active; }
    void  SetShieldWarning(bool w)    { shieldWarning_ = w; }
    void  SetInputFrozen(bool f)      { inputFrozen_ = f; }

    Urho3D::Node*   GetNode()            const { return node_; }
    Urho3D::Vector3 GetPosition()        const { return node_ ? node_->GetPosition() : Urho3D::Vector3::ZERO; }
    float           GetFacingYaw()       const { return facingYaw_; }
    bool            WasJumpedThisFrame() const { return jumpedThisFrame_; }
    bool            WasStepThisFrame()   const { return stepThisFrame_;   }
    bool            IsOnGround()         const { return onGround_; }

private:
    void SpawnAt(const Urho3D::Vector3& pos);
    void UpdateSprite();

    bool IsSolid(int wx, int wy, int wz) const;
    void ResolveY(Urho3D::Vector3& pos);
    void ResolveXZ(Urho3D::Vector3& pos);

    Urho3D::Context* context_;
    Urho3D::Node*    node_ = nullptr;

    const GalaxyEggbert::Worlds::World* world_;
    int wcx_, wcz_;

    Urho3D::Vector3 vel_{0.0f, 0.0f, 0.0f};
    Urho3D::Vector3 spawn_{0.0f, kHalfH + 0.5f, 0.0f};
    float facingYaw_ = 0.0f;
    bool  onGround_       = false;
    bool  fallDeath_      = false;
    bool  landedThisFrame_= false;
    float flashTimer_     = 0.0f;
    float flashTickTimer_ = 0.0f;
    bool  shieldActive_   = false;
    bool  shieldWarning_  = false;
    float shieldBlinkPhase_ = 0.0f;
    bool  inputFrozen_    = false;
    float coyoteTimer_    = 0.0f;
    float jumpBuffer_     = 0.0f;
    bool  jumpHeld_       = false;

    // Sprite animation (billboard facing camera, UV-mapped from blupi.png)
    bool                       jumpedThisFrame_ = false;
    bool                       stepThisFrame_   = false;
    Urho3D::BillboardSet*      sprite_          = nullptr;
    GalaxyEggbert::BlupiAction action_   = GalaxyEggbert::BlupiAction::Stop;
    int                        animTick_ = 0;

    static constexpr float kSheetW = 600.0f;
    static constexpr float kSheetH = 2040.0f;
    static constexpr float kTile   =  60.0f;
    static constexpr int   kCols   =  10;
};
