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
    // FC_ROTATE_Y: billboard rotates around world-Y only so size.y is always a
    // vertical world-unit offset. FC_ROTATE_XYZ tilts with camera pitch, causing
    // the sprite bottom to swing forward in Z and visually clip into terrain blocks.
    sprite_->SetFaceCameraMode(FC_ROTATE_Y);

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

    // Blob shadow: very flat Box.mdl (Y scale 0.01) with solid dark NoTexture material.
    // Positioned at terrain surface below Blupi each frame via UpdateShadow().
    {
        auto* boxModel = cache->GetResource<Model>("Models/Box.mdl");
        auto* tech     = cache->GetResource<Technique>("Techniques/NoTexture.xml");
        shadowNode_ = scene->CreateChild("BlupiShadow");
        auto* sm = shadowNode_->CreateComponent<StaticModel>();
        if (boxModel) sm->SetModel(boxModel);
        SharedPtr<Material> shadowMat(new Material(context_));
        if (tech) shadowMat->SetTechnique(0, tech);
        shadowMat->SetShaderParameter("MatDiffColor",    Color(0.05f, 0.05f, 0.05f, 1.0f));
        shadowMat->SetShaderParameter("MatEmissiveColor", Color(0.0f,  0.0f,  0.0f,  0.0f));
        sm->SetMaterial(shadowMat);
    }

    SpawnAt(spawn_);
    UpdateSprite();
    UpdateShadow();
}

void Blupi::Respawn() {
    SpawnAt(spawn_);
}

void Blupi::SnapToSurface(float surfaceY) {
    if (!node_ || vel_.y_ > 0.0f) return;
    Vector3 pos = node_->GetPosition();
    float feetY = pos.y_ - kHalfH;
    // Only snap when Blupi's feet are within [surfaceY-0.4, surfaceY+0.3].
    if (feetY > surfaceY + 0.3f || feetY < surfaceY - 0.4f) return;
    pos.y_    = surfaceY + kHalfH;
    vel_.y_   = 0.0f;
    onGround_ = true;
    node_->SetPosition(pos);
}

Blupi::~Blupi() {
    if (shadowNode_) { shadowNode_->Remove(); shadowNode_ = nullptr; }
    if (node_)       { node_->Remove();       node_       = nullptr; }
}

void Blupi::SpawnAt(const Vector3& pos) {
    if (node_) node_->SetPosition(pos);
    vel_           = Vector3::ZERO;
    onGround_      = false;
    animTick_      = 0;
    flashTimer_    = 0.0f;
    flashTickTimer_= 0.0f;
    landedThisFrame_ = false;
    coyoteTimer_   = 0.0f;
    jumpBuffer_    = 0.0f;
    jumpHeld_      = false;
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
    if (!shieldActive_) {
        bb->color_ = Color::WHITE;
    } else if (shieldWarning_ && std::fmod(shieldBlinkPhase_, 0.3f) >= 0.15f) {
        bb->color_ = Color::WHITE; // blink to white rapidly when shield is about to expire
    } else {
        bb->color_ = Color(0.5f, 0.85f, 1.0f);
    }
    sprite_->Commit();
}

void Blupi::UpdateShadow() {
    if (!shadowNode_ || !node_) return;
    Vector3 pos = node_->GetPosition();
    int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
    int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
    int startWY = static_cast<int>(std::floor(pos.y_ - kHalfH - 0.05f));
    float groundTop = -999.0f;
    for (int wy = startWY; wy >= 0 && startWY - wy < 20; --wy) {
        if (IsSolid(wx, wy, wz)) {
            groundTop = static_cast<float>(wy) + 0.5f; // block top surface Y
            break;
        }
    }
    if (groundTop > -900.0f) {
        float height = std::max(0.0f, (pos.y_ - kHalfH) - groundTop);
        float scale  = std::max(0.25f, 0.55f - height * 0.025f);
        shadowNode_->SetEnabled(true);
        shadowNode_->SetPosition(Vector3(pos.x_, groundTop + 0.02f, pos.z_));
        shadowNode_->SetScale(Vector3(scale, 0.01f, scale));
    } else {
        shadowNode_->SetEnabled(false);
    }
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

    // Auto step-up: when on ground and a 1-tile step is ahead (body-level blocked,
    // one above clear), snap Blupi onto the step top instead of stopping.
    // diff ≤ 1.0 prevents stepping up into blocks that are too high.
    auto tryStepUp = [&](int wx, int wz) -> bool {
        if (!onGround_ || vel_.y_ > 0.0f) return false;
        if (!IsSolid(wx, bodyWY, wz) || IsSolid(wx, bodyWY + 1, wz)) return false;
        float stepTop = static_cast<float>(bodyWY) + 0.5f;
        float feet    = pos.y_ - kHalfH;
        float diff    = stepTop - feet;
        if (diff <= 0.0f || diff > 1.0f) return false;
        pos.y_    = stepTop + kHalfH;
        onGround_ = true;
        vel_.y_   = 0.0f;
        return true;
    };

    auto pushX = [&](float xEdge, int sign) {
        int wx = static_cast<int>(std::round(xEdge)) + wcx_;
        int wz = static_cast<int>(std::round(pos.z_)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            if (!tryStepUp(wx, wz)) {
                pos.x_  = static_cast<float>(wx - wcx_) - sign * (0.5f + kHalfW);
                vel_.x_ = 0.0f;
            }
        }
    };
    auto pushZ = [&](float zEdge, int sign) {
        int wx = static_cast<int>(std::round(pos.x_)) + wcx_;
        int wz = static_cast<int>(std::round(zEdge)) + wcz_;
        if (IsSolid(wx, bodyWY, wz)) {
            if (!tryStepUp(wx, wz)) {
                pos.z_  = static_cast<float>(wz - wcz_) - sign * (0.5f + kHalfW);
                vel_.z_ = 0.0f;
            }
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

    // Input freeze: suppress all movement after death while the game's respawn timer counts
    // down. Animation and shield blink still tick so the character doesn't look frozen solid.
    if (inputFrozen_) {
        action_ = BlupiAction::Stop;
        ++animTick_;
        if (shieldActive_) shieldBlinkPhase_ += dt;
        UpdateSprite();
        UpdateShadow();
        return;
    }

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

    // Coyote time: refreshed every frame while grounded; counts down in air.
    // Allows jump for kCoyoteTime seconds after walking off an edge.
    if (onGround_) {
        coyoteTimer_ = kCoyoteTime;
    } else {
        coyoteTimer_ = std::max(0.0f, coyoteTimer_ - dt);
    }

    // Jump buffer: store intent for kJumpBuffer seconds so a jump press just
    // before landing still fires on the landing frame.
    if (input->GetKeyPress(KEY_LCTRL)) {
        jumpBuffer_ = kJumpBuffer;
    } else if (jumpBuffer_ > 0.0f) {
        jumpBuffer_ = std::max(0.0f, jumpBuffer_ - dt);
    }

    // --- Gravity (glide: Right Shift in air reduces gravity and caps fall speed) ---
    if (!onGround_ && lookingUp) {
        vel_.y_ += kGravity * 0.12f * dt;
        vel_.y_  = std::max(vel_.y_, -1.5f);
    } else {
        vel_.y_ += kGravity * dt;
        vel_.y_  = std::max(vel_.y_, -30.0f);
    }

    // --- Jump: fires when buffer is active and coyote window is open ---
    if (jumpBuffer_ > 0.0f && coyoteTimer_ > 0.0f) {
        vel_.y_          = kJumpSpeed;
        onGround_        = false;
        jumpedThisFrame_ = true;
        jumpBuffer_      = 0.0f;
        coyoteTimer_     = 0.0f;
        jumpHeld_        = true;
    }

    // Variable jump height: releasing jump early cuts the arc to kMinJumpSpeed.
    if (jumpHeld_) {
        if (onGround_) {
            jumpHeld_ = false;
        } else if (!input->GetKeyDown(KEY_LCTRL) && vel_.y_ > kMinJumpSpeed) {
            vel_.y_  = kMinJumpSpeed;
            jumpHeld_ = false;
        }
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
        if (lookingUp && vel_.y_ <= 0.0f) newAction = BlupiAction::Up; // gliding
        else newAction = (vel_.y_ > 0.0f) ? BlupiAction::Jump : BlupiAction::Air;
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

    if (shieldActive_) shieldBlinkPhase_ += dt;

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
    UpdateShadow();
}
