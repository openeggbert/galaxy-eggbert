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
    static constexpr float kHalfW     =   0.35f;
    static constexpr float kHalfH     =   0.7f;

    explicit Blupi(Urho3D::Context* context,
                   Urho3D::Scene*   scene,
                   const GalaxyEggbert::Worlds::World* world,
                   int wcx, int wcz);
    ~Blupi();

    void Update(float dt);
    void Respawn();

    Urho3D::Node*   GetNode()            const { return node_; }
    Urho3D::Vector3 GetPosition()        const { return node_ ? node_->GetPosition() : Urho3D::Vector3::ZERO; }
    float           GetFacingYaw()       const { return facingYaw_; }
    bool            WasJumpedThisFrame() const { return jumpedThisFrame_; }

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
    float facingYaw_ = 0.0f;
    bool  onGround_  = false;

    // Sprite animation (billboard facing camera, UV-mapped from blupi.png)
    bool                       jumpedThisFrame_ = false;
    Urho3D::BillboardSet*      sprite_          = nullptr;
    GalaxyEggbert::BlupiAction action_   = GalaxyEggbert::BlupiAction::Stop;
    int                        animTick_ = 0;

    static constexpr float kSheetW = 600.0f;
    static constexpr float kSheetH = 2040.0f;
    static constexpr float kTile   =  60.0f;
    static constexpr int   kCols   =  10;
};
