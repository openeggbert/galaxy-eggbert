#include "GEBlupiController.hpp"
#include <cmath>

using namespace Simple3D;

namespace GESimple3D {

// Animation frame tables from mobile-eggbert table_blupi (blupi.png icon indices).
// Each icon: col = icon % 10, row = icon / 10, each tile 60×60 px.
static const int kStopFrames[]  = {0};
static const int kMarchFrames[] = {5, 6, 7, 8, 9, 10};
static const int kJumpFrames[]  = {17, 18, 19};
static const int kAirFrames[]   = {169, 26, 170, 170, 27};
static const int kDownFrames[]  = {33};
static const int kUpFrames[]    = {44};

// StopNage (BlupiAction 18): 10 frames cycling icons 76-77 from blupi.png.
static const int kSwimIdleFrames[] = {76,76,76,76,76,76,77,77,77,77};
// MarchNage (BlupiAction 19): 14 frames cycling icons 76-81,39 from blupi.png.
static const int kSwimMoveFrames[] = {76,76,77,77,78,78,79,79,80,80,81,81,39,39};

static constexpr float kAnimFps = 8.0f; // animation ticks per second

void GEBlupiController::Create(Game& game) {
    player_ = game.CreateEntity("Blupi");
    player_->SetPosition(spawn_);
    cc_ = player_->AddCharacterController(0.35f, 1.6f);
    cc_->SetGravity(kGravity);
    player_->SetCollisionLayer(CollisionLayer::Actor);
    player_->SetCollisionMask(MakeCollisionMask({
        CollisionLayer::StaticGeometry,
        CollisionLayer::Trigger
    }));

    sprite_ = player_->CreateChild("BlupiSprite");
    sprite_->SetLocalPosition(Vector3(0.0f, kVisHalf - kHalfH, 0.0f));
    float worldSize = kTilePx / 64.0f;
    sprite_->AddBillboard("icons/blupi.png", worldSize);

    state_     = BlupiState::Stop;
    animPhase_ = 0;
    animTimer_ = 0.0f;
    ApplySprite();
}

void GEBlupiController::Update(Game& game, float dt) {
    if (!player_ || !cc_) return;

    landedThisFrame_ = false;
    jumpedThisFrame_ = false;

    bool grounded = cc_->IsOnGround();
    if (!wasGrounded_ && grounded) landedThisFrame_ = true;
    wasGrounded_ = grounded;

    if (shieldTimer_ > 0.0f) shieldTimer_ -= dt;

    // Shield visual: pulse blue tint on sprite while shielded (HUD-010).
    if (sprite_) {
        if (IsShieldActive()) {
            float p = 0.55f + 0.45f * std::sinf(shieldTimer_ * 3.14159f * 6.0f);
            sprite_->SetMaterialColor(Color(0.25f * p, 0.5f * p + 0.25f, p, 1.0f));
        } else {
            sprite_->SetMaterialColor(Color(1.0f, 1.0f, 1.0f, 1.0f));
        }
    }

    if (inputFrozen_) {
        cc_->Move(Vector3::ZERO, dt);
        AdvanceAnim(dt);
        ApplySprite();
        return;
    }

    Vector2 axis = game.GetAxis2D("Move");
    float stickX = axis.x_;
    float stickY = axis.y_;

    float mag = std::sqrt(stickX*stickX + stickY*stickY);
    if (mag > 1.0f) { stickX /= mag; stickY /= mag; }

    // Facing direction: track last horizontal rotation input
    if (stickX < -0.1f) facingRight_ = false;
    if (stickX >  0.1f) facingRight_ = true;

    yaw_ += stickX * kTurnSpeed * dt;
    if (stickX != 0.0f)
        player_->SetRotation(Quaternion(0.0f, yaw_, 0.0f));

    float rad = yaw_ * (3.14159265f / 180.0f);
    float speed = swimming_ ? kMoveSpeed * 0.4f : kMoveSpeed;
    Vector3 moveVel(
        std::sin(rad) * stickY * speed,
        0.0f,
        std::cos(rad) * stickY * speed);
    cc_->Move(moveVel, dt);

    bool jumpPressed = game.IsActionPressed("Jump");
    if (jumpPressed && cc_->TryJump(kJumpSpeed))
        jumpedThisFrame_ = true;

    bool crouchHeld = game.IsKeyDown(Key::LShift);
    bool lookUpHeld = game.IsKeyDown(Key::RShift);

    // Glide: RShift in air caps fall speed (inspired by BlupiStep m_blupiSuspend)
    if (lookUpHeld && !grounded) {
        Vector3 v = player_->GetLinearVelocity();
        if (v.y_ < -3.0f) {
            v.y_ = -3.0f;
            player_->SetLinearVelocity(v);
        }
    }

    // Fall out of world
    if (player_->GetPosition().y_ < kFallLimit) Respawn();

    bool moving = (std::fabs(stickX) > 0.1f || std::fabs(stickY) > 0.1f);
    UpdateState(grounded, moving, jumpedThisFrame_, crouchHeld, lookUpHeld, stickX);
    AdvanceAnim(dt);
    ApplySprite();
}

void GEBlupiController::UpdateState(bool grounded, bool moving, bool jumpTriggered,
                                     bool crouchHeld, bool lookUpHeld, float stickX) {
    (void)stickX;

    // Jump triggered this frame: enter Jump state
    if (jumpTriggered) {
        state_     = BlupiState::Jump;
        animPhase_ = 0;
        animTimer_ = 0.0f;
        return;
    }

    switch (state_) {
    case BlupiState::Jump:
        // After all 3 jump frames play, move to Air
        if (animPhase_ >= 3) {
            state_     = BlupiState::Air;
            animPhase_ = 0;
        }
        break;

    case BlupiState::Air:
        if (grounded) {
            state_     = moving ? BlupiState::March : BlupiState::Stop;
            animPhase_ = 0;
            break;
        }
        // Glide in air
        if (lookUpHeld) { state_ = BlupiState::Up; animPhase_ = 0; }
        break;

    case BlupiState::Up:
        if (grounded) {
            state_     = moving ? BlupiState::March : BlupiState::Stop;
            animPhase_ = 0;
            break;
        }
        if (!lookUpHeld) { state_ = BlupiState::Air; animPhase_ = 0; }
        break;

    default:
        // On-ground states: Stop, March, Down
        if (!grounded) {
            state_     = BlupiState::Air;
            animPhase_ = 0;
            break;
        }
        if (crouchHeld) {
            if (state_ != BlupiState::Down) { state_ = BlupiState::Down; animPhase_ = 0; }
        } else if (lookUpHeld) {
            if (state_ != BlupiState::Up)   { state_ = BlupiState::Up;   animPhase_ = 0; }
        } else if (swimming_) {
            BlupiState tgt = moving ? BlupiState::SwimMove : BlupiState::SwimIdle;
            if (state_ != tgt) { state_ = tgt; animPhase_ = 0; }
        } else if (moving) {
            if (state_ != BlupiState::March){ state_ = BlupiState::March; animPhase_ = 0; }
        } else {
            if (state_ != BlupiState::Stop) { state_ = BlupiState::Stop;  animPhase_ = 0; }
        }
        break;
    }
}

void GEBlupiController::AdvanceAnim(float dt) {
    steppedThisFrame_ = false;
    animTimer_ += dt;
    if (animTimer_ >= 1.0f / kAnimFps) {
        animTimer_ -= 1.0f / kAnimFps;
        animPhase_++;
        // Step sound on march frames 0 and 3 of 6-frame cycle (matches mobile-eggbert footstep cadence).
        if (state_ == BlupiState::March) {
            int f = animPhase_ % 6;
            steppedThisFrame_ = (f == 0 || f == 3);
        }
    }
}

void GEBlupiController::ApplySprite() {
    if (!sprite_) return;

    const int* frames;
    int nFrames;
    switch (state_) {
    case BlupiState::March: frames = kMarchFrames; nFrames = 6; break;
    case BlupiState::Jump:  frames = kJumpFrames;  nFrames = 3; break;
    case BlupiState::Air:      frames = kAirFrames;      nFrames = 5;  break;
    case BlupiState::Down:     frames = kDownFrames;      nFrames = 1;  break;
    case BlupiState::SwimIdle: frames = kSwimIdleFrames;  nFrames = 10; break;
    case BlupiState::SwimMove: frames = kSwimMoveFrames;  nFrames = 14; break;
    case BlupiState::Up:    frames = kUpFrames;    nFrames = 1; break;
    default:                frames = kStopFrames;  nFrames = 1; break;
    }

    int icon = frames[animPhase_ % nFrames];
    int col  = icon % kCols;
    int row  = icon / kCols;
    sprite_->SetBillboardUVRect(col * kTilePx, row * kTilePx, kTilePx, kTilePx);
    sprite_->SetFlipX2D(facingRight_);
}

void GEBlupiController::Respawn() {
    if (!player_) return;
    yaw_       = 0.0f;
    swimming_  = false;
    state_     = BlupiState::Stop;
    animPhase_ = 0;
    animTimer_ = 0.0f;
    player_->SetPosition(spawn_);
    player_->SetLinearVelocity(Vector3::ZERO);
    player_->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
    wasGrounded_ = false;
}

void GEBlupiController::Launch(float ySpeed) {
    if (!player_) return;
    Vector3 v = player_->GetLinearVelocity();
    v.y_ = ySpeed;
    player_->SetLinearVelocity(v);
    state_     = BlupiState::Jump;
    animPhase_ = 0;
    animTimer_ = 0.0f;
}

void GEBlupiController::BounceUp() {
    Launch(kJumpSpeed * 0.65f);
}

void GEBlupiController::SetSpriteVisible(bool v) {
    if (sprite_) sprite_->SetActive(v);
}

Vector3 GEBlupiController::GetPosition() const {
    return player_ ? player_->GetPosition() : Vector3::ZERO;
}

bool GEBlupiController::IsOnGround() const {
    return cc_ ? cc_->IsOnGround() : false;
}

float GEBlupiController::GetVelY() const {
    return player_ ? player_->GetLinearVelocity().y_ : 0.0f;
}

float GEBlupiController::GetVelX() const {
    return player_ ? player_->GetLinearVelocity().x_ : 0.0f;
}

} // namespace GESimple3D
