#include "Blupi.hpp"
#include <cmath>

using namespace Urho3D;
using namespace GalaxyEggbert::Worlds;

Blupi::Blupi(Context* context, Scene* scene, const World* world, int wcx, int wcz)
    : context_(context), world_(world), wcx_(wcx), wcz_(wcz)
{
    auto* cache = context_->GetSubsystem<ResourceCache>();

    node_ = scene->CreateChild("Blupi");

    // Yellow ellipsoid: scale Sphere.mdl to 0.7×1.4×0.7 (≈ kHalfW*2 × kHalfH*2 × kHalfW*2)
    auto* sm = node_->CreateComponent<StaticModel>();
    auto* model = cache->GetResource<Model>("Models/Sphere.mdl");
    if (model) sm->SetModel(model);

    SharedPtr<Material> mat(new Material(context_));
    auto* tech = cache->GetResource<Technique>("Techniques/NoTexture.xml");
    if (!tech) tech = cache->GetResource<Technique>("Techniques/Diff.xml");
    if (tech) mat->SetTechnique(0, tech);
    mat->SetShaderParameter("MatDiffColor",     Color(1.0f, 0.85f, 0.08f));
    mat->SetShaderParameter("MatEmissiveColor", Color(0.30f, 0.25f, 0.02f));
    mat->SetShaderParameter("MatSpecColor",     Color(0.6f, 0.55f, 0.1f, 32.0f));
    sm->SetMaterial(mat);

    node_->SetScale(Vector3(kHalfW * 2.0f, kHalfH * 2.0f, kHalfW * 2.0f));

    // Spawn on top of the centre block (world 50,0,50 → scene origin + a little above)
    SpawnAt(Vector3(0.0f, kHalfH + 0.5f, 0.0f));
}

Blupi::~Blupi() {
    if (node_) { node_->Remove(); node_ = nullptr; }
}

void Blupi::SpawnAt(const Vector3& pos) {
    if (node_) node_->SetPosition(pos);
    vel_ = Vector3::ZERO;
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

// scene → world: wx = round(px) + wcx,  wy = round(py),  wz = round(pz) + wcz
// Block centre in scene: (wx-wcx, wy, wz-wcz); occupies y in [wy-0.5, wy+0.5]

void Blupi::ResolveY(Vector3& pos) {
    int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
    int wz = static_cast<int>(std::round(pos.z_)) + wcz_;

    if (vel_.y_ <= 0.0f) {
        // Check ground below feet
        float feetY = pos.y_ - kHalfH;
        int wy = static_cast<int>(std::floor(feetY + 0.5f)); // block whose centre is just below feet
        if (IsSolid(wx, wy, wz)) {
            float top = static_cast<float>(wy) + 0.5f;
            if (feetY < top) {
                pos.y_ = top + kHalfH;
                vel_.y_ = 0.0f;
                onGround_ = true;
                return;
            }
        }
    }

    // Check ceiling above head
    if (vel_.y_ > 0.0f) {
        float headY = pos.y_ + kHalfH;
        int wy = static_cast<int>(std::floor(headY + 0.5f));
        if (IsSolid(wx, wy, wz)) {
            float bot = static_cast<float>(wy) - 0.5f;
            if (headY > bot) {
                pos.y_ = bot - kHalfH;
                vel_.y_ = 0.0f;
                return;
            }
        }
    }

    onGround_ = false;
}

void Blupi::ResolveXZ(Vector3& pos) {
    // Check block Y levels that overlap with Blupi's body height.
    // Use feetY + epsilon so we never check the ground block Blupi is
    // standing on — otherwise every adjacent floor tile would look like a wall.
    float feetY = pos.y_ - kHalfH;
    float headY = pos.y_ + kHalfH;
    int wyMin = static_cast<int>(std::floor(feetY + 0.51f));
    int wyMax = static_cast<int>(std::floor(headY + 0.49f));

    for (int checkWY = wyMin; checkWY <= wyMax; ++checkWY) {
        {
            int wx = static_cast<int>(std::floor(pos.x_ + kHalfW + 0.5f)) + wcx_;
            int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
            if (IsSolid(wx, checkWY, wz)) {
                pos.x_ = static_cast<float>(wx - wcx_) - 0.5f - kHalfW;
                if (vel_.x_ > 0.0f) vel_.x_ = 0.0f;
            }
        }
        {
            int wx = static_cast<int>(std::floor(pos.x_ - kHalfW + 0.5f)) + wcx_;
            int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
            if (IsSolid(wx, checkWY, wz)) {
                pos.x_ = static_cast<float>(wx - wcx_) + 0.5f + kHalfW;
                if (vel_.x_ < 0.0f) vel_.x_ = 0.0f;
            }
        }
        {
            int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
            int wz = static_cast<int>(std::floor(pos.z_ + kHalfW + 0.5f)) + wcz_;
            if (IsSolid(wx, checkWY, wz)) {
                pos.z_ = static_cast<float>(wz - wcz_) - 0.5f - kHalfW;
                if (vel_.z_ > 0.0f) vel_.z_ = 0.0f;
            }
        }
        {
            int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
            int wz = static_cast<int>(std::floor(pos.z_ - kHalfW + 0.5f)) + wcz_;
            if (IsSolid(wx, checkWY, wz)) {
                pos.z_ = static_cast<float>(wz - wcz_) + 0.5f + kHalfW;
                if (vel_.z_ < 0.0f) vel_.z_ = 0.0f;
            }
        }
    }
}

void Blupi::Update(float dt, float cameraYaw) {
    if (!node_) return;
    auto* input = context_->GetSubsystem<Input>();

    // --- Horizontal input (camera-relative, arrow keys) ---
    // fwd/right point in the direction the player should move when pressing
    // UP/RIGHT. The camera sits at offset (sin(yaw), ..., cos(yaw)) from the
    // target, so the "forward into the scene" direction is the negative of that.
    float rad = cameraYaw * static_cast<float>(M_PI) / 180.0f;
    Vector3 fwd (-std::sin(rad), 0.0f, -std::cos(rad));
    Vector3 right(-std::cos(rad), 0.0f,  std::sin(rad));

    Vector3 move = Vector3::ZERO;
    if (input->GetKeyDown(KEY_UP))    move += fwd;
    if (input->GetKeyDown(KEY_DOWN))  move -= fwd;
    if (input->GetKeyDown(KEY_LEFT))  move -= right;
    if (input->GetKeyDown(KEY_RIGHT)) move += right;

    if (move.LengthSquared() > 0.0f) move.Normalize();
    vel_.x_ = move.x_ * kMoveSpeed;
    vel_.z_ = move.z_ * kMoveSpeed;

    // Face direction of travel
    if (move.LengthSquared() > 0.0f)
        node_->SetRotation(Quaternion(0.0f, std::atan2(move.x_, move.z_) * 180.0f / static_cast<float>(M_PI), 0.0f));

    // --- Gravity ---
    vel_.y_ += kGravity * dt;
    vel_.y_ = std::max(vel_.y_, -30.0f); // terminal velocity

    // --- Jump ---
    if (onGround_ && input->GetKeyPress(KEY_SPACE)) {
        vel_.y_ = kJumpSpeed;
        onGround_ = false;
    }

    // --- Integrate + collision, axis by axis ---
    Vector3 pos = node_->GetPosition();

    // Y first so ground check is stable
    pos.y_ += vel_.y_ * dt;
    ResolveY(pos);

    pos.x_ += vel_.x_ * dt;
    pos.z_ += vel_.z_ * dt;
    ResolveXZ(pos);

    // Safety net: don't fall out of the world
    if (pos.y_ < -10.0f) SpawnAt(Vector3(0.0f, kHalfH + 0.5f, 0.0f));

    node_->SetPosition(pos);
}
