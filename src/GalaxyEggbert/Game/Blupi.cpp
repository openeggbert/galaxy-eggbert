#include "Blupi.hpp"
#include <cmath>

using namespace Urho3D;
using namespace GalaxyEggbert::Worlds;

Blupi::Blupi(Context* context, Scene* scene, const World* world, int wcx, int wcz)
    : context_(context), world_(world), wcx_(wcx), wcz_(wcz)
{
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* tech  = cache->GetResource<Technique>("Techniques/NoTexture.xml");

    // Root node (unscaled) — holds body + direction indicator as children
    // so both inherit the rotation without the scale complication.
    node_ = scene->CreateChild("Blupi");

    // --- Body: yellow ellipsoid ---
    auto* bodyNode = node_->CreateChild("BlupiBody");
    bodyNode->SetScale(Vector3(kHalfW * 2.0f, kHalfH * 2.0f, kHalfW * 2.0f));
    auto* sm = bodyNode->CreateComponent<StaticModel>();
    if (auto* m = cache->GetResource<Model>("Models/Sphere.mdl")) sm->SetModel(m);
    {
        SharedPtr<Material> mat(new Material(context_));
        if (tech) mat->SetTechnique(0, tech);
        mat->SetShaderParameter("MatDiffColor",     Color(1.0f, 0.85f, 0.08f));
        mat->SetShaderParameter("MatEmissiveColor", Color(0.28f, 0.22f, 0.02f));
        mat->SetShaderParameter("MatSpecColor",     Color(0.5f, 0.45f, 0.1f, 16.0f));
        sm->SetMaterial(mat);
    }

    // --- Direction indicator: small dark-red box sticking out at front (+Z local) ---
    // Makes the facing direction visually obvious since the sphere is symmetric.
    auto* noseNode = node_->CreateChild("BlupiFront");
    noseNode->SetPosition(Vector3(0.0f, 0.0f, kHalfW + 0.12f));
    noseNode->SetScale(Vector3(0.14f, 0.14f, 0.22f));
    auto* noseSM = noseNode->CreateComponent<StaticModel>();
    if (auto* m = cache->GetResource<Model>("Models/Box.mdl")) noseSM->SetModel(m);
    {
        SharedPtr<Material> mat(new Material(context_));
        if (tech) mat->SetTechnique(0, tech);
        mat->SetShaderParameter("MatDiffColor",     Color(0.75f, 0.15f, 0.10f));
        mat->SetShaderParameter("MatEmissiveColor", Color(0.18f, 0.03f, 0.02f));
        mat->SetShaderParameter("MatSpecColor",     Color(0.1f, 0.1f, 0.1f, 8.0f));
        noseSM->SetMaterial(mat);
    }

    SpawnAt(Vector3(0.0f, kHalfH + 0.5f, 0.0f));
}

Blupi::~Blupi() {
    if (node_) { node_->Remove(); node_ = nullptr; }
}

void Blupi::SpawnAt(const Vector3& pos) {
    if (node_) node_->SetPosition(pos);
    vel_      = Vector3::ZERO;
    onGround_ = false;
}

bool Blupi::IsSolid(int wx, int wy, int wz) const {
    if (!world_) return false;
    if (wx < 0 || wy < 0 || wz < 0) return true;
    if (wx >= 100 || wy >= 100 || wz >= 100) return false;
    return !world_->getBlock(
        static_cast<uint16_t>(wx),
        static_cast<uint16_t>(wy),
        static_cast<uint16_t>(wz)).isAir();
}

// Coordinate mapping:
//   Block (wx,wy,wz) → scene centre (wx-wcx, wy, wz-wcz)
//   Block occupies scene Y ∈ [wy-0.5, wy+0.5]
//
// Ground check uses floor(feetY) which gives the block whose Y range contains
// or is just below the feet. The feetY<top condition filters out blocks above.

void Blupi::ResolveY(Vector3& pos) {
    int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
    int wz = static_cast<int>(std::round(pos.z_)) + wcz_;

    if (vel_.y_ <= 0.0f) {
        float feetY = pos.y_ - kHalfH;
        // Check the block at/just-below feet (floor gives the containing block)
        for (int wy : { static_cast<int>(std::floor(feetY)),
                        static_cast<int>(std::floor(feetY)) - 1 }) {
            if (IsSolid(wx, wy, wz)) {
                float top = static_cast<float>(wy) + 0.5f;
                if (feetY < top + 0.01f) {
                    pos.y_   = top + kHalfH;
                    vel_.y_  = 0.0f;
                    onGround_ = true;
                    return;
                }
            }
        }
    }

    if (vel_.y_ > 0.0f) {
        float headY = pos.y_ + kHalfH;
        int wy = static_cast<int>(std::floor(headY + 0.01f));
        if (IsSolid(wx, wy, wz)) {
            float bot = static_cast<float>(wy) - 0.5f;
            if (headY > bot - 0.01f) {
                pos.y_  = bot - kHalfH;
                vel_.y_ = 0.0f;
                return;
            }
        }
    }

    onGround_ = false;
}

void Blupi::ResolveXZ(Vector3& pos) {
    // Use the block at Blupi's lower body (avoids the ground block at Blupi's feet).
    // round(pos.y_) when Blupi is on ground (pos.y_≈1.2) gives wy=1, which is
    // above the floor layer at wy=0, so floor blocks are never treated as walls.
    int bodyWY = static_cast<int>(std::round(pos.y_));

    // +X wall
    {
        int wx = static_cast<int>(std::round(pos.x_ + kHalfW + 0.01f)) + wcx_;
        int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            pos.x_ = static_cast<float>(wx - wcx_) - 0.5f - kHalfW;
            vel_.x_ = 0.0f;
        }
    }
    // -X wall
    {
        int wx = static_cast<int>(std::round(pos.x_ - kHalfW - 0.01f)) + wcx_;
        int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            pos.x_ = static_cast<float>(wx - wcx_) + 0.5f + kHalfW;
            vel_.x_ = 0.0f;
        }
    }
    // +Z wall
    {
        int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
        int wz = static_cast<int>(std::round(pos.z_ + kHalfW + 0.01f)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            pos.z_ = static_cast<float>(wz - wcz_) - 0.5f - kHalfW;
            vel_.z_ = 0.0f;
        }
    }
    // -Z wall
    {
        int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
        int wz = static_cast<int>(std::round(pos.z_ - kHalfW - 0.01f)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            pos.z_ = static_cast<float>(wz - wcz_) + 0.5f + kHalfW;
            vel_.z_ = 0.0f;
        }
    }
}

void Blupi::Update(float dt) {
    if (!node_) return;
    auto* input = context_->GetSubsystem<Input>();

    // --- Rotation: LEFT/RIGHT arrows turn Blupi ---
    if (input->GetKeyDown(KEY_LEFT))  facingYaw_ -= kTurnSpeed * dt;
    if (input->GetKeyDown(KEY_RIGHT)) facingYaw_ += kTurnSpeed * dt;
    node_->SetRotation(Quaternion(0.0f, facingYaw_, 0.0f));

    // --- Forward/back movement along Blupi's own facing direction ---
    float rad = facingYaw_ * static_cast<float>(M_PI) / 180.0f;
    Vector3 fwd(std::sin(rad), 0.0f, std::cos(rad));

    float forwardInput = 0.0f;
    if (input->GetKeyDown(KEY_UP))   forwardInput =  1.0f;
    if (input->GetKeyDown(KEY_DOWN)) forwardInput = -1.0f;

    vel_.x_ = fwd.x_ * forwardInput * kMoveSpeed;
    vel_.z_ = fwd.z_ * forwardInput * kMoveSpeed;

    // --- Gravity (derived from mobile-eggbert Decor.cpp gravity constant) ---
    vel_.y_ += kGravity * dt;
    vel_.y_  = std::max(vel_.y_, -30.0f);

    // --- Jump ---
    if (onGround_ && input->GetKeyPress(KEY_SPACE)) {
        vel_.y_   = kJumpSpeed;
        onGround_ = false;
    }

    // --- Integrate + collision, axis by axis ---
    Vector3 pos = node_->GetPosition();

    pos.y_ += vel_.y_ * dt;
    ResolveY(pos);

    pos.x_ += vel_.x_ * dt;
    pos.z_ += vel_.z_ * dt;
    ResolveXZ(pos);

    if (pos.y_ < -10.0f) SpawnAt(Vector3(0.0f, kHalfH + 0.5f, 0.0f));

    node_->SetPosition(pos);
}
