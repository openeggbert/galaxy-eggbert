#include "GEBlupiController.hpp"
#include <cmath>

using namespace Simple3D;

namespace GESimple3D {

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

    // Billboard sprite child: offset upward so the sprite bottom aligns with physics feet.
    sprite_ = player_->CreateChild("BlupiSprite");
    sprite_->SetLocalPosition(Vector3(0.0f, kVisHalf - kHalfH, 0.0f));
    float worldSize = kTilePx / 64.0f;
    sprite_->AddBillboard("icons/blupi.png", worldSize);
    UpdateSprite(false);
}

void GEBlupiController::Update(Game& game, float dt) {
    if (!player_ || !cc_) return;

    landedThisFrame_ = false;
    jumpedThisFrame_ = false;

    bool grounded = cc_->IsOnGround();
    if (!wasGrounded_ && grounded) landedThisFrame_ = true;
    wasGrounded_ = grounded;

    if (shieldTimer_ > 0.0f) shieldTimer_ -= dt;

    if (inputFrozen_) {
        cc_->Move(Vector3::ZERO, dt);
        return;
    }

    // TODO(#24B): Move input bindings to BindAction/BindAxis2D when task is defined.
    // For now using direct key + gamepad calls.
    Vector2 axis = game.GetAxis2D("Move");
    float stickX = axis.x_;
    float stickY = axis.y_;

    // Keyboard fallback (direct keys)
    if (game.IsKeyDown(Key::Left))  stickX -= 1.0f;
    if (game.IsKeyDown(Key::Right)) stickX += 1.0f;
    if (game.IsKeyDown(Key::Up))    stickY += 1.0f;
    if (game.IsKeyDown(Key::Down))  stickY -= 1.0f;

    // Clamp to unit circle
    float mag = std::sqrt(stickX * stickX + stickY * stickY);
    if (mag > 1.0f) { stickX /= mag; stickY /= mag; }

    yaw_ += stickX * kTurnSpeed * dt;
    if (stickX != 0.0f)
        player_->SetRotation(Quaternion(0.0f, yaw_, 0.0f));

    float rad = yaw_ * (3.14159265f / 180.0f);
    Vector3 moveVel(
        std::sin(rad) * stickY * kMoveSpeed,
        0.0f,
        std::cos(rad) * stickY * kMoveSpeed);
    cc_->Move(moveVel, dt);

    bool jumpPressed = game.IsActionPressed("Jump");
    if (jumpPressed && cc_->TryJump(kJumpSpeed))
        jumpedThisFrame_ = true;

    // RShift: glide — reduce gravity while falling (placeholder, no full glide logic yet)
    // TODO: Implement glide (reduced gravity, capped fall speed) matching mobile-eggbert.
    if (game.IsKeyDown(Key::RShift) && !grounded) {
        Vector3 v = player_->GetLinearVelocity();
        if (v.y_ < -3.0f) {
            v.y_ = -3.0f;
            player_->SetLinearVelocity(v);
        }
    }

    // Respawn on fall
    Vector3 pos = player_->GetPosition();
    if (pos.y_ < kFallLimit) Respawn();

    bool moving = (std::fabs(stickX) > 0.1f || std::fabs(stickY) > 0.1f);
    UpdateSprite(moving);
}

void GEBlupiController::Respawn() {
    if (!player_) return;
    yaw_ = 0.0f;
    player_->SetPosition(spawn_);
    player_->SetLinearVelocity(Vector3::ZERO);
    player_->SetRotation(Quaternion(0.0f, 0.0f, 0.0f));
    wasGrounded_ = false;
}

void GEBlupiController::UpdateSprite(bool moving) {
    if (!sprite_) return;
    // Walk animation: frames 0-9 in row 0 when moving, frame 0 when idle.
    // Each frame is kTilePx × kTilePx pixels; 10 columns per row.
    animTick_ = moving ? (animTick_ + 1) % (kCols * 3) : 0;
    int icon = animTick_ / 3; // 3 engine ticks per animation frame
    int col  = icon % kCols;
    int row  = icon / kCols;
    sprite_->SetBillboardUVRect(col * kTilePx, row * kTilePx, kTilePx, kTilePx);
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

} // namespace GESimple3D
