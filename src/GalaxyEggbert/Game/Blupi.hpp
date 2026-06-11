#pragma once
#include "../GEEngine.hpp"
#include "GalaxyEggbert/Worlds/World.hpp"

// Blupi character: physics, movement, and collision against the voxel world.
//
// Physics derived from mobile-eggbert (C++ port of Speedy Blupi for Windows Phone),
// specifically Decor.cpp: m_blupiVitesseX/Y velocity integration, gravity constant,
// jump impulse, and tile-blocking collision logic.
// Scaled from 64-px tile units (mobile-eggbert) to 1-unit 3D voxel coordinates.
class Blupi {
public:
    static constexpr float kGravity   = -22.0f;
    static constexpr float kJumpSpeed =  10.0f;
    static constexpr float kMoveSpeed =   5.5f;
    static constexpr float kTurnSpeed = 180.0f; // degrees/second
    static constexpr float kHalfW     =   0.35f; // AABB half-size X and Z
    static constexpr float kHalfH     =   0.7f;  // AABB half-size Y

    explicit Blupi(Urho3D::Context* context,
                   Urho3D::Scene* scene,
                   const GalaxyEggbert::Worlds::World* world,
                   int wcx, int wcz);
    ~Blupi();

    // Call once per frame while in Play phase.
    void Update(float dt);

    Urho3D::Node* GetNode() const { return node_; }
    Urho3D::Vector3 GetPosition() const { return node_ ? node_->GetPosition() : Urho3D::Vector3::ZERO; }
    float GetFacingYaw() const { return facingYaw_; }

private:
    void SpawnAt(const Urho3D::Vector3& pos);

    bool IsSolid(int wx, int wy, int wz) const;
    void ResolveY(Urho3D::Vector3& pos);
    void ResolveXZ(Urho3D::Vector3& pos);

    Urho3D::Context*  context_;
    Urho3D::Node*     node_ = nullptr;

    const GalaxyEggbert::Worlds::World* world_;
    int wcx_, wcz_;

    Urho3D::Vector3 vel_{0.0f, 0.0f, 0.0f};
    float facingYaw_ = 0.0f; // Blupi's own facing direction, degrees
    bool onGround_ = false;
};
