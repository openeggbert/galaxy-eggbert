#include "Blupi.hpp"
#include "Tables.hpp"
#include <cmath>

using namespace Urho3D;
using namespace GalaxyEggbert;
using namespace GalaxyEggbert::Worlds;

Blupi::Blupi(Context* context, Scene* scene, const World* world, int wcx, int wcz)
    : context_(context), world_(world), wcx_(wcx), wcz_(wcz)
{
    auto* cache = context_->GetSubsystem<ResourceCache>();

    node_ = scene->CreateChild("Blupi");

    // Billboard sprite: always faces the camera, UV-mapped from blupi.png.
    sprite_ = node_->CreateComponent<BillboardSet>();
    sprite_->SetNumBillboards(1);
    sprite_->SetFaceCameraMode(FC_ROTATE_XYZ);

    {
        auto* tex  = cache->GetResource<Texture2D>("icons/blupi.png");
        auto* tech = cache->GetResource<Technique>("Techniques/DiffAlpha.xml");
        SharedPtr<Material> mat(new Material(context_));
        if (tech) mat->SetTechnique(0, tech);
        if (tex)  mat->SetTexture(TU_DIFFUSE, tex);
        mat->SetShaderParameter("MatDiffColor", Color(1.0f, 1.0f, 1.0f, 1.0f));
        mat->SetShaderParameter("MatSpecColor",  Color(0.0f, 0.0f, 0.0f, 0.0f));
        sprite_->SetMaterial(mat);
    }

    Billboard* bb = sprite_->GetBillboard(0);
    // Offset the billboard center upward so the sprite's bottom aligns with
    // the physics feet (center - kHalfH). Without this offset the lower part
    // of the sprite clips into the block surface.
    static constexpr float kVisHalf = 60.0f / 64.0f / 2.0f; // half visual height
    bb->position_ = Vector3(0.0f, kVisHalf - kHalfH, 0.0f);
    bb->size_     = Vector2(60.0f / 64.0f, 60.0f / 64.0f);
    bb->enabled_  = true;
    sprite_->Commit();

    SpawnAt(spawn_);
    UpdateSprite();
}

void Blupi::Respawn() {
    SpawnAt(spawn_);
}

Blupi::~Blupi() {
    if (node_) { node_->Remove(); node_ = nullptr; }
}

void Blupi::SpawnAt(const Vector3& pos) {
    if (node_) node_->SetPosition(pos);
    vel_           = Vector3::ZERO;
    onGround_      = false;
    animTick_      = 0;
    flashTimer_    = 0.0f;
    flashTickTimer_= 0.0f;
    landedThisFrame_ = false;
    if (sprite_) {
        sprite_->GetBillboard(0)->enabled_ = true;
        sprite_->Commit();
    }
}

void Blupi::UpdateSprite() {
    if (!sprite_) return;
    // Divide tick by 3 to match original 20 fps animation speed at 60 fps.
    // Mirrors Decor.cpp: scaledPhase = m_blupiPhase / Config::ScaleDiv(1).
    int icon = Tables::GetBlupiIcon(action_, animTick_ / 3);
    int col  = icon % kCols;
    int row  = icon / kCols;
    float u0 = col * kTile / kSheetW;
    float v0 = row * kTile / kSheetH;
    Billboard* bb = sprite_->GetBillboard(0);
    bb->uv_   = Rect(u0, v0, u0 + kTile / kSheetW, v0 + kTile / kSheetH);
    bb->color_ = shieldActive_ ? Color(0.5f, 0.85f, 1.0f) : Color::WHITE;
    sprite_->Commit();
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

void Blupi::ResolveY(Vector3& pos) {
    int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
    int wz = static_cast<int>(std::round(pos.z_)) + wcz_;

    if (vel_.y_ <= 0.0f) {
        float feetY = pos.y_ - kHalfH;
        for (int wy : { static_cast<int>(std::floor(feetY)),
                        static_cast<int>(std::floor(feetY)) - 1 }) {
            if (IsSolid(wx, wy, wz)) {
                float top = static_cast<float>(wy) + 0.5f;
                if (feetY < top + 0.01f) {
                    pos.y_    = top + kHalfH;
                    vel_.y_   = 0.0f;
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
    // round(pos.y_) when on ground (pos.y_≈0.86) gives wy=1, above the floor
    // layer at wy=0, so floor blocks are never treated as walls.
    int bodyWY = static_cast<int>(std::round(pos.y_));

    auto pushX = [&](float xEdge, int sign) {
        int wx = static_cast<int>(std::round(xEdge)) + wcx_;
        int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            pos.x_  = static_cast<float>(wx - wcx_) - sign * (0.5f + kHalfW);
            vel_.x_ = 0.0f;
        }
    };
    auto pushZ = [&](float zEdge, int sign) {
        int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
        int wz = static_cast<int>(std::round(zEdge)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            pos.z_  = static_cast<float>(wz - wcz_) - sign * (0.5f + kHalfW);
            vel_.z_ = 0.0f;
        }
    };

    pushX(pos.x_ + kHalfW + 0.01f, -1);
    pushX(pos.x_ - kHalfW - 0.01f, +1);
    pushZ(pos.z_ + kHalfW + 0.01f, -1);
    pushZ(pos.z_ - kHalfW - 0.01f, +1);
}

void Blupi::Update(float dt) {
    if (!node_) return;
    auto* input = context_->GetSubsystem<Input>();
    jumpedThisFrame_  = false;
    stepThisFrame_    = false;
    landedThisFrame_  = false;
    bool wasOnGround  = onGround_;

    // --- Rotation ---
    bool turning = false;
    if (input->GetKeyDown(KEY_LEFT))  { facingYaw_ -= kTurnSpeed * dt; turning = true; }
    if (input->GetKeyDown(KEY_RIGHT)) { facingYaw_ += kTurnSpeed * dt; turning = true; }
    node_->SetRotation(Quaternion(0.0f, facingYaw_, 0.0f));

    // --- Crouch / look-up: lock movement while held ---
    bool crouching = input->GetKeyDown(KEY_LSHIFT);
    bool lookingUp = input->GetKeyDown(KEY_RSHIFT);

    // --- Forward/back movement along facing direction ---
    float rad = facingYaw_ * static_cast<float>(M_PI) / 180.0f;
    Vector3 fwd(std::sin(rad), 0.0f, std::cos(rad));

    float forwardInput = 0.0f;
    if (!crouching && !lookingUp) {
        if (input->GetKeyDown(KEY_UP))   forwardInput =  1.0f;
        if (input->GetKeyDown(KEY_DOWN)) forwardInput = -1.0f;
    }

    vel_.x_ = fwd.x_ * forwardInput * kMoveSpeed;
    vel_.z_ = fwd.z_ * forwardInput * kMoveSpeed;

    // --- Gravity ---
    vel_.y_ += kGravity * dt;
    vel_.y_  = std::max(vel_.y_, -30.0f);

    // --- Jump (Left Ctrl) ---
    if (onGround_ && input->GetKeyPress(KEY_LCTRL)) {
        vel_.y_          = kJumpSpeed;
        onGround_        = false;
        jumpedThisFrame_ = true;
    }

    // --- Integrate + collision ---
    Vector3 pos = node_->GetPosition();
    pos.y_ += vel_.y_ * dt;
    ResolveY(pos);
    landedThisFrame_ = !wasOnGround && onGround_;
    pos.x_ += vel_.x_ * dt;
    pos.z_ += vel_.z_ * dt;
    ResolveXZ(pos);

    if (pos.y_ < -10.0f) {
        fallDeath_ = true;
        SpawnAt(spawn_);
        return;
    }

    node_->SetPosition(pos);

    // --- Animation state ---
    float hSpeed = std::sqrt(vel_.x_ * vel_.x_ + vel_.z_ * vel_.z_);
    BlupiAction newAction;
    if (crouching && onGround_) {
        newAction = BlupiAction::Down;
    } else if (lookingUp && onGround_) {
        newAction = BlupiAction::Up;
    } else if (!onGround_) {
        newAction = (vel_.y_ > 0.0f) ? BlupiAction::Jump : BlupiAction::Air;
    } else if (hSpeed > 0.1f) {
        newAction = BlupiAction::March;
    } else if (turning) {
        newAction = BlupiAction::Turn;
    } else {
        newAction = BlupiAction::Stop;
    }

    if (newAction != action_) {
        action_   = newAction;
        animTick_ = 0;
    }
    ++animTick_;

    // One footstep sound per stride (6-frame march cycle × 3 ticks/frame = 18 ticks).
    if (action_ == BlupiAction::March && onGround_ && animTick_ % 18 == 1)
        stepThisFrame_ = true;

    // Invincibility flash: toggle billboard visibility every 0.1 s.
    if (flashTimer_ > 0.0f) {
        flashTimer_     -= dt;
        flashTickTimer_ -= dt;
        if (flashTickTimer_ <= 0.0f) {
            flashTickTimer_ = 0.1f;
            bool vis = sprite_->GetBillboard(0)->enabled_;
            sprite_->GetBillboard(0)->enabled_ = !vis;
            sprite_->Commit();
        }
        if (flashTimer_ <= 0.0f && sprite_) {
            sprite_->GetBillboard(0)->enabled_ = true;
            sprite_->Commit();
        }
    }

    UpdateSprite();
}
