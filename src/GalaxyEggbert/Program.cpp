/**
 * @file Main.cpp
 * @brief Nova3D Cube3D Demo — Urho3D-like walkable colored-box scene.
 *
 * This example uses the Nova3D public API (Urho3D-like style) to recreate
 * the CNA cube3d_demo scene: a two-storey house with garden, fences, trees,
 * bushes, balcony, basement, stairs, windows, doors, and first-person camera.
 *
 * Controls:
 *  - W / S            : move forward / backward
 *  - A / D            : strafe left / right
 *  - Q / E            : move down / up
 *  - Left/Right arrow : yaw   (look left / right)
 *  - Up/Down arrow    : pitch (look up   / down)
 *  - Esc              : quit
 */

#include <Nova3D/Nova3D.h>
#include <cmath>
#include <vector>

using namespace Nova3D;

namespace {
constexpr float kPi       = 3.14159265358979323846f;
constexpr float kPiOver4  = kPi * 0.25f;
constexpr float kDeg2Rad  = kPi / 180.0f;
} // namespace

class Cube3DDemo : public Application {
public:
    explicit Cube3DDemo(Context* context)
        : Application(context) {}

    void Setup() override {
    }

    void Start() override {
        auto* renderer = GetContext()->GetRenderer();
        renderer->SetClearColor(Color::FromBytes(135, 196, 230, 255));

        BuildScene();

        cameraPosition_ = Vector3(0.0f, 1.7f, 9.0f);
        yaw_   = kPi;
        pitch_ = -5.0f * kDeg2Rad;
        UpdateCameraTarget();
    }

    void Stop() override {
    }

    void Update(float timeStep) override {
        auto* input = GetContext()->GetInput();
        auto* engine = GetContext()->GetEngine();

        if (input->GetKeyDown(KEY_ESCAPE))
            engine->Exit();

        // Rotation
        const float rotSpeed = 1.6f * timeStep;
        if (input->GetKeyDown(KEY_LEFT))  yaw_   += rotSpeed;
        if (input->GetKeyDown(KEY_RIGHT)) yaw_   -= rotSpeed;
        if (input->GetKeyDown(KEY_UP))    pitch_ += rotSpeed;
        if (input->GetKeyDown(KEY_DOWN))  pitch_ -= rotSpeed;

        const float pitchLimit = 80.0f * kDeg2Rad;
        if (pitch_ >  pitchLimit) pitch_ =  pitchLimit;
        if (pitch_ < -pitchLimit) pitch_ = -pitchLimit;

        // Forward / right derived from yaw+pitch
        const float cp = std::cos(pitch_);
        const float sp = std::sin(pitch_);
        const float cy = std::cos(yaw_);
        const float sy = std::sin(yaw_);
        const Vector3 forward(cp * sy, sp, cp * cy * -1.0f);
        Vector3 right = forward.CrossProduct(Vector3::UP);
        float rlen = right.Length();
        if (rlen > 1e-6f) right = right * (1.0f / rlen);

        // Movement
        const float speed = 5.0f * timeStep;
        Vector3 move(0, 0, 0);
        if (input->GetKeyDown(KEY_W)) move += forward * speed;
        if (input->GetKeyDown(KEY_S)) move -= forward * speed;
        if (input->GetKeyDown(KEY_D)) move += right   * speed;
        if (input->GetKeyDown(KEY_A)) move -= right   * speed;
        if (input->GetKeyDown(KEY_E)) move += Vector3::UP * speed;
        if (input->GetKeyDown(KEY_Q)) move -= Vector3::UP * speed;

        // AABB collision
        Vector3 candidate = cameraPosition_;
        candidate.x_ += move.x_;
        if (CollidesWithSolid(candidate)) candidate.x_ = cameraPosition_.x_;
        candidate.z_ += move.z_;
        if (CollidesWithSolid(candidate)) candidate.z_ = cameraPosition_.z_;
        candidate.y_ += move.y_;
        cameraPosition_ = candidate;
        UpdateCameraTarget();

        // Update camera matrices each frame.
        auto* renderer = GetContext()->GetRenderer();
        float aspect = static_cast<float>(renderer->GetViewportWidth()) /
                       static_cast<float>(std::max(1, renderer->GetViewportHeight()));
        renderer->SetCameraMatrices(cameraPosition_, cameraTarget_, Vector3::UP,
                                     kPiOver4, aspect, 0.1f, 300.0f);
    }

private:
    // --- Collider ---
    struct Collider {
        Vector3 center;
        Vector3 halfSize;
    };

    static constexpr float kPlayerHalfX = 0.30f;
    static constexpr float kPlayerHalfZ = 0.30f;
    static constexpr float kPlayerEyeToTop    = 0.10f;
    static constexpr float kPlayerEyeToBottom = 1.60f;

    bool CollidesWithSolid(const Vector3& eye) const {
        const float pxMin = eye.x_ - kPlayerHalfX;
        const float pxMax = eye.x_ + kPlayerHalfX;
        const float pyMin = eye.y_ - kPlayerEyeToBottom;
        const float pyMax = eye.y_ + kPlayerEyeToTop;
        const float pzMin = eye.z_ - kPlayerHalfZ;
        const float pzMax = eye.z_ + kPlayerHalfZ;
        for (const auto& c : colliders_) {
            const float cxMin = c.center.x_ - c.halfSize.x_;
            const float cxMax = c.center.x_ + c.halfSize.x_;
            const float cyMin = c.center.y_ - c.halfSize.y_;
            const float cyMax = c.center.y_ + c.halfSize.y_;
            const float czMin = c.center.z_ - c.halfSize.z_;
            const float czMax = c.center.z_ + c.halfSize.z_;
            if (pxMax > cxMin && pxMin < cxMax &&
                pyMax > cyMin && pyMin < cyMax &&
                pzMax > czMin && pzMin < czMax) {
                return true;
            }
        }
        return false;
    }

    void UpdateCameraTarget() {
        const float cp = std::cos(pitch_);
        const float sp = std::sin(pitch_);
        const float cy = std::cos(yaw_);
        const float sy = std::sin(yaw_);
        const Vector3 forward(cp * sy, sp, -cp * cy);
        cameraTarget_ = cameraPosition_ + forward;
    }

    // --- Scene building helpers ---
    void AddBox(const Vector3& position, const Vector3& size, const Color& color, bool solid = true) {
        if (solid) {
            colliders_.push_back(Collider{
                position,
                Vector3(size.x_ * 0.5f, size.y_ * 0.5f, size.z_ * 0.5f)
            });
        }
        GetContext()->GetRenderer()->AddBox(position, size, color);
    }

    void AddGround(float size, const Color& color) {
        GetContext()->GetRenderer()->AddGround(size, color);
    }

    void AddFence(const Vector3& from, const Vector3& to,
                  float postSpacing, float postHeight,
                  const Color& postColor, const Color& railColor) {
        const Vector3 d = to - from;
        const float len = std::sqrt(d.x_ * d.x_ + d.z_ * d.z_);
        if (len < 1e-3f) return;
        const Vector3 dir(d.x_ / len, 0.0f, d.z_ / len);
        const int posts = static_cast<int>(len / postSpacing) + 1;
        for (int i = 0; i < posts; ++i) {
            const float t = static_cast<float>(i) * postSpacing;
            const Vector3 p = from + dir * t;
            AddBox(Vector3(p.x_, postHeight * 0.5f, p.z_),
                   Vector3(0.1f, postHeight, 0.1f), postColor);
        }
        const Vector3 mid = (from + to) * 0.5f;
        const bool axisX = std::abs(d.z_) < 1e-3f;
        const bool axisZ = std::abs(d.x_) < 1e-3f;
        const Vector3 railSize = axisX
            ? Vector3(len, 0.05f, 0.05f)
            : axisZ ? Vector3(0.05f, 0.05f, len)
                    : Vector3(0.05f, 0.05f, 0.05f);
        if (axisX || axisZ) {
            AddBox(Vector3(mid.x_, postHeight * 0.75f, mid.z_), railSize, railColor);
            AddBox(Vector3(mid.x_, postHeight * 0.35f, mid.z_), railSize, railColor);
        }
    }

    void AddTree(const Vector3& base, float trunkH, float crownSize) {
        const Color trunkColor = Color::FromBytes(110, 70, 35, 255);
        const Color crownColor = Color::FromBytes(50, 140, 55, 255);
        AddBox(base + Vector3(0.0f, trunkH * 0.5f, 0.0f),
               Vector3(0.4f, trunkH, 0.4f), trunkColor, true);
        AddBox(base + Vector3(0.0f, trunkH + crownSize * 0.5f, 0.0f),
               Vector3(crownSize, crownSize, crownSize), crownColor, false);
    }

    void AddBush(const Vector3& base, float size = 0.7f) {
        AddBox(base + Vector3(0.0f, size * 0.5f, 0.0f),
               Vector3(size, size, size),
               Color::FromBytes(60, 130, 60, 255), false);
    }

    void AddPath(const Vector3& center, const Vector3& size, const Color& color) {
        AddBox(Vector3(center.x_, 0.025f, center.z_),
               Vector3(size.x_, 0.05f, size.z_), color, false);
    }

    void AddWindow(char wallAxis, float wallCoord,
                   const Vector3& center, float w, float h) {
        const Color glass = Color::FromBytes(70, 110, 160, 255);
        const Color frame = Color::FromBytes(80, 65, 50, 255);
        const float frameT = 0.06f;
        const float frameW = 0.08f;
        const float offset = 0.05f;

        const bool axisZ = (wallAxis == 'Z' || wallAxis == 'z');
        const float sign = (wallAxis == 'Z' || wallAxis == 'X') ? +1.0f : -1.0f;

        Vector3 c = center;
        if (axisZ) c.z_ = wallCoord + sign * offset;
        else       c.x_ = wallCoord + sign * offset;

        Vector3 glassSize = axisZ
            ? Vector3(w, h, frameT)
            : Vector3(frameT, h, w);
        AddBox(c, glassSize, glass, false);

        Vector3 fc = c;
        if (axisZ) fc.z_ += sign * 0.005f; else fc.x_ += sign * 0.005f;

        if (axisZ) {
            AddBox(Vector3(fc.x_, fc.y_ + h * 0.5f, fc.z_),
                   Vector3(w + frameW, frameW, frameT), frame, false);
            AddBox(Vector3(fc.x_, fc.y_ - h * 0.5f, fc.z_),
                   Vector3(w + frameW, frameW, frameT), frame, false);
            AddBox(Vector3(fc.x_ - w * 0.5f, fc.y_, fc.z_),
                   Vector3(frameW, h, frameT), frame, false);
            AddBox(Vector3(fc.x_ + w * 0.5f, fc.y_, fc.z_),
                   Vector3(frameW, h, frameT), frame, false);
            AddBox(Vector3(fc.x_, fc.y_, fc.z_),
                   Vector3(frameW * 0.6f, h, frameT), frame, false);
        } else {
            AddBox(Vector3(fc.x_, fc.y_ + h * 0.5f, fc.z_),
                   Vector3(frameT, frameW, w + frameW), frame, false);
            AddBox(Vector3(fc.x_, fc.y_ - h * 0.5f, fc.z_),
                   Vector3(frameT, frameW, w + frameW), frame, false);
            AddBox(Vector3(fc.x_, fc.y_, fc.z_ - w * 0.5f),
                   Vector3(frameT, h, frameW), frame, false);
            AddBox(Vector3(fc.x_, fc.y_, fc.z_ + w * 0.5f),
                   Vector3(frameT, h, frameW), frame, false);
            AddBox(Vector3(fc.x_, fc.y_, fc.z_),
                   Vector3(frameT, h, frameW * 0.6f), frame, false);
        }
    }

    void AddDoor(float wallZ, float sign,
                 float cx, float cyBase, float w, float h) {
        const Color frame  = Color::FromBytes(70, 50, 30, 255);
        const Color door   = Color::FromBytes(120, 75, 40, 255);
        const Color handle = Color::FromBytes(220, 200, 60, 255);
        const float frameT = 0.06f;
        const float frameW_ = 0.10f;
        const float panelT = 0.05f;
        const float off    = 0.05f;

        const float zOuter = wallZ + sign * off;
        const float cy     = cyBase + h * 0.5f;

        AddBox(Vector3(cx, cyBase + h, zOuter),
               Vector3(w + 2 * frameW_, frameW_, frameT), frame, false);
        AddBox(Vector3(cx - w * 0.5f - frameW_ * 0.5f, cy, zOuter),
               Vector3(frameW_, h, frameT), frame, false);
        AddBox(Vector3(cx + w * 0.5f + frameW_ * 0.5f, cy, zOuter),
               Vector3(frameW_, h, frameT), frame, false);
        AddBox(Vector3(cx, cy, wallZ - sign * 0.02f),
               Vector3(w * 0.95f, h * 0.97f, panelT), door, false);
        AddBox(Vector3(cx + w * 0.30f, cyBase + h * 0.45f, zOuter + sign * 0.01f),
               Vector3(0.08f, 0.08f, 0.08f), handle, false);
    }

    void AddEntranceSteps(float cx, float frontZ, float baseY) {
        const Color stoneColor = Color::FromBytes(150, 145, 135, 255);
        const int steps = 3;
        const float stepRise = baseY / steps;
        const float stepRun  = 0.30f;
        for (int i = 0; i < steps; ++i) {
            const float y = stepRise * (static_cast<float>(i) + 0.5f);
            const float z = frontZ + 0.10f + stepRun * (static_cast<float>(steps - i) - 0.5f);
            AddBox(Vector3(cx, y, z),
                   Vector3(1.6f, stepRise, stepRun), stoneColor, false);
        }
    }

    void BuildScene() {
        // Garden ground
        AddGround(60.0f, Color::FromBytes(70, 140, 70, 255));

        AddBox(Vector3(-10.0f, 0.05f, 6.0f), Vector3(4.0f, 0.10f, 3.0f), Color::FromBytes(80,150,75,255), false);
        AddBox(Vector3(12.0f, 0.05f, -8.0f), Vector3(5.0f, 0.10f, 4.0f), Color::FromBytes(85,155,80,255), false);

        const Color wallColor  = Color::FromBytes(210, 190, 150, 255);
        const Color innerWall  = Color::FromBytes(200, 180, 140, 255);
        const Color floorColor = Color::FromBytes(170, 140, 100, 255);
        const Color roofColor  = Color::FromBytes(160, 60, 40, 255);
        const Color baseColor  = Color::FromBytes(130, 110, 90, 255);
        const Color stoneColor = Color::FromBytes(120, 120, 130, 255);

        const float W = 8.0f, D = 6.0f;
        const float floorH = 2.5f;
        const float wallT  = 0.2f;
        const float baseH  = 0.3f;
        const float baseY  = baseH * 0.5f;
        const float f1Y    = baseH + floorH * 0.5f;
        const float f2Y    = baseH + floorH * 1.5f;
        const float ceilY  = baseH + floorH;
        const float roofY  = baseH + floorH * 2.0f;

        // Foundation
        AddBox(Vector3(0.0f, baseY, 0.0f), Vector3(W + 0.4f, baseH, D + 0.4f), baseColor, false);

        // Basement
        AddBox(Vector3(W * 0.5f - 1.0f, -0.6f, -D * 0.5f + 1.0f),
               Vector3(1.6f, 1.2f, 1.6f), Color::FromBytes(40, 35, 30, 255), false);
        for (int i = 0; i < 4; ++i) {
            const float y = -0.15f - i * 0.20f;
            const float zOff = -D * 0.5f + 0.4f + i * 0.2f;
            AddBox(Vector3(W * 0.5f - 1.0f, y, zOff),
                   Vector3(1.4f, 0.10f, 0.2f), stoneColor, false);
        }

        // 1st-floor outer walls
        AddBox(Vector3(0.0f, f1Y, -D * 0.5f + wallT * 0.5f), Vector3(W, floorH, wallT), wallColor);
        AddBox(Vector3(-W * 0.5f + wallT * 0.5f, f1Y, 0.0f), Vector3(wallT, floorH, D), wallColor);
        AddBox(Vector3(W * 0.5f - wallT * 0.5f, f1Y, 0.0f), Vector3(wallT, floorH, D), wallColor);

        // Front wall with door opening
        const float frontZ = D * 0.5f - wallT * 0.5f;
        const float doorW = 1.0f, doorH = 2.0f;
        const float doorCX = -1.5f;
        const float lintelH = floorH - doorH;
        const float lintelY = baseH + doorH + lintelH * 0.5f;
        const float leftSegW  = (doorCX - doorW * 0.5f) - (-W * 0.5f);
        const float rightSegW = (W * 0.5f) - (doorCX + doorW * 0.5f);

        AddBox(Vector3(-W * 0.5f + leftSegW * 0.5f, f1Y, frontZ), Vector3(leftSegW, floorH, wallT), wallColor);
        AddBox(Vector3(W * 0.5f - rightSegW * 0.5f, f1Y, frontZ), Vector3(rightSegW, floorH, wallT), wallColor);
        AddBox(Vector3(doorCX, lintelY, frontZ), Vector3(doorW, lintelH, wallT), wallColor);
        AddDoor(frontZ + wallT * 0.5f, +1.0f, doorCX, baseH, doorW, doorH);
        AddEntranceSteps(doorCX, frontZ + wallT * 0.5f, baseH);

        // Windows
        AddWindow('Z', frontZ + wallT * 0.5f, Vector3(-3.5f, baseH + 1.4f, 0.0f), 0.9f, 1.0f);
        AddWindow('Z', frontZ + wallT * 0.5f, Vector3(1.8f, baseH + 1.4f, 0.0f), 0.9f, 1.0f);
        AddWindow('X', W * 0.5f - wallT * 0.5f, Vector3(0.0f, baseH + 1.4f, 1.5f), 0.9f, 1.0f);
        AddWindow('x', -W * 0.5f + wallT * 0.5f, Vector3(0.0f, baseH + 1.4f, -1.5f), 0.9f, 1.0f);

        // Interior partition
        const float partX = 1.0f;
        AddBox(Vector3(partX, f1Y, (-D * 0.5f + (-0.5f)) * 0.5f),
               Vector3(wallT, floorH, (-0.5f) - (-D * 0.5f)), innerWall, true);
        AddBox(Vector3(partX, f1Y, (1.0f + D * 0.5f) * 0.5f),
               Vector3(wallT, floorH, D * 0.5f - 1.0f), innerWall, true);

        // Floor between stories
        const float fT = 0.10f;
        const float ceilCY = ceilY + fT * 0.5f;
        AddBox(Vector3(0.0f, ceilCY, (-0.5f + D * 0.5f) * 0.5f),
               Vector3(W, fT, D * 0.5f - (-0.5f)), floorColor, false);
        AddBox(Vector3(0.0f, ceilCY, (-2.0f + (-D * 0.5f)) * 0.5f),
               Vector3(W, fT, (-D * 0.5f) - (-2.0f) * -1.0f * 0.0f + (-2.0f + D * 0.5f)),
               floorColor, false);
        AddBox(Vector3((2.0f + (-W * 0.5f)) * 0.5f, ceilCY, -1.25f),
               Vector3(2.0f - (-W * 0.5f), fT, 1.5f), floorColor, false);
        AddBox(Vector3((3.5f + W * 0.5f) * 0.5f, ceilCY, -1.25f),
               Vector3(W * 0.5f - 3.5f, fT, 1.5f), floorColor, false);

        // Stairs
        const int stepCount = 8;
        const float stepRise = (ceilY - baseH) / stepCount;
        const float stepRun  = 0.30f;
        for (int i = 0; i < stepCount; ++i) {
            const float sy = baseH + stepRise * (static_cast<float>(i) + 0.5f);
            const float sz = -0.5f - stepRun * (static_cast<float>(i) + 0.5f);
            AddBox(Vector3(2.75f, sy, sz), Vector3(1.5f, stepRise, stepRun), stoneColor, false);
        }

        // 2nd-floor walls
        AddBox(Vector3(0.0f, f2Y, -D * 0.5f + wallT * 0.5f), Vector3(W, floorH, wallT), wallColor);
        AddBox(Vector3(-W * 0.5f + wallT * 0.5f, f2Y, 0.0f), Vector3(wallT, floorH, D), wallColor);
        AddBox(Vector3(W * 0.5f - wallT * 0.5f, f2Y, 0.0f), Vector3(wallT, floorH, D), wallColor);

        // Front wall with balcony doorway
        const float balDoorW = 1.2f, balDoorH = 2.0f;
        const float balCX = 2.0f;
        const float balLintelH = floorH - balDoorH;
        const float balLintelY = baseH + floorH + balDoorH + balLintelH * 0.5f;
        const float balLeftW  = (balCX - balDoorW * 0.5f) - (-W * 0.5f);
        const float balRightW = (W * 0.5f) - (balCX + balDoorW * 0.5f);
        AddBox(Vector3(-W * 0.5f + balLeftW * 0.5f, f2Y, frontZ), Vector3(balLeftW, floorH, wallT), wallColor);
        AddBox(Vector3(W * 0.5f - balRightW * 0.5f, f2Y, frontZ), Vector3(balRightW, floorH, wallT), wallColor);
        AddBox(Vector3(balCX, balLintelY, frontZ), Vector3(balDoorW, balLintelH, wallT), wallColor);

        // 2nd-floor windows
        AddWindow('X', W * 0.5f - wallT * 0.5f, Vector3(0.0f, baseH + floorH + 1.4f, 1.5f), 0.9f, 1.0f);
        AddWindow('x', -W * 0.5f + wallT * 0.5f, Vector3(0.0f, baseH + floorH + 1.4f, -1.5f), 0.9f, 1.0f);
        AddWindow('Z', frontZ + wallT * 0.5f, Vector3(-2.5f, baseH + floorH + 1.4f, 0.0f), 1.0f, 1.0f);

        // Roof
        const int roofLayers = 5;
        const float roofTotalH = 1.6f;
        const float layerH = roofTotalH / roofLayers;
        for (int i = 0; i < roofLayers; ++i) {
            const float t = static_cast<float>(i + 1) / roofLayers;
            const float w = (W + 0.6f) * (1.0f - 0.85f * t * t) + 0.2f;
            const float d = (D + 0.6f) * (1.0f - 0.85f * t * t) + 0.2f;
            const float y = roofY + layerH * (static_cast<float>(i) + 0.5f);
            const Color rc = Color::FromBytes(
                static_cast<unsigned char>(160 - i * 6),
                static_cast<unsigned char>(60 - i * 4), 40, 255);
            AddBox(Vector3(0.0f, y, 0.0f), Vector3(w, layerH, d), rc, false);
        }
        AddBox(Vector3(0.0f, roofY + roofTotalH + 0.10f, 0.0f),
               Vector3(0.6f, 0.20f, 0.6f), Color::FromBytes(110, 40, 30, 255), false);

        // Balcony
        const float balPlatY = baseH + floorH;
        AddBox(Vector3(balCX, balPlatY + 0.05f, frontZ + 0.6f),
               Vector3(2.5f, 0.10f, 1.2f), stoneColor, false);
        AddBox(Vector3(balCX, balPlatY + 0.85f, frontZ + 1.2f),
               Vector3(2.6f, 0.10f, 0.10f), wallColor);
        AddBox(Vector3(balCX, balPlatY + 0.30f, frontZ + 1.2f),
               Vector3(2.6f, 0.05f, 0.05f), wallColor);
        AddBox(Vector3(balCX - 1.25f, balPlatY + 0.45f, frontZ + 0.6f),
               Vector3(0.10f, 0.85f, 1.2f), wallColor);
        AddBox(Vector3(balCX + 1.25f, balPlatY + 0.45f, frontZ + 0.6f),
               Vector3(0.10f, 0.85f, 1.2f), wallColor);
        for (int i = -2; i <= 2; ++i) {
            const float bx = balCX + i * 0.5f;
            AddBox(Vector3(bx, balPlatY + 0.45f, frontZ + 1.2f),
                   Vector3(0.05f, 0.7f, 0.05f), wallColor, false);
        }

        // Path
        const Color pathColor = Color::FromBytes(190, 185, 170, 255);
        AddPath(Vector3(doorCX, 0.0f, frontZ + 1.5f), Vector3(1.4f, 0.0f, 1.6f), pathColor);
        AddPath(Vector3(doorCX, 0.0f, frontZ + 3.5f), Vector3(1.4f, 0.0f, 2.4f), pathColor);
        AddPath(Vector3(doorCX, 0.0f, frontZ + 6.0f), Vector3(1.4f, 0.0f, 2.6f), pathColor);

        // Fence
        const Color postCol = Color::FromBytes(140, 110, 70, 255);
        const Color railCol = Color::FromBytes(160, 130, 90, 255);
        const float postSpacing = 0.6f;
        AddFence(Vector3(-12.0f, 0.0f, 10.0f), Vector3(-3.0f, 0.0f, 10.0f), postSpacing, 1.1f, postCol, railCol);
        AddFence(Vector3(-12.0f, 0.0f, 10.0f), Vector3(-12.0f, 0.0f, -2.0f), postSpacing, 1.1f, postCol, railCol);
        AddFence(Vector3(3.0f, 0.0f, 10.0f), Vector3(12.0f, 0.0f, 10.0f), postSpacing, 1.1f, postCol, railCol);
        AddFence(Vector3(12.0f, 0.0f, 10.0f), Vector3(12.0f, 0.0f, -2.0f), postSpacing, 1.1f, postCol, railCol);

        // Trees
        AddTree(Vector3(6.0f, 0.0f, 4.0f), 1.8f, 1.8f);
        AddTree(Vector3(-6.0f, 0.0f, 6.5f), 1.6f, 1.6f);
        AddTree(Vector3(-9.0f, 0.0f, -3.0f), 2.0f, 2.0f);
        AddTree(Vector3(9.0f, 0.0f, -6.0f), 1.5f, 1.5f);
        AddTree(Vector3(8.0f, 0.0f, 7.5f), 1.4f, 1.4f);
        AddTree(Vector3(-7.5f, 0.0f, -7.0f), 1.7f, 1.7f);

        // Bushes
        AddBush(Vector3(-3.0f, 0.0f, 7.0f), 0.7f);
        AddBush(Vector3(0.5f, 0.0f, 7.5f), 0.6f);
        AddBush(Vector3(4.0f, 0.0f, 7.0f), 0.7f);
        AddBush(Vector3(-5.5f, 0.0f, 5.0f), 0.5f);
        AddBush(Vector3(5.5f, 0.0f, 5.5f), 0.6f);
    }

    std::vector<Collider> colliders_;
    Vector3 cameraPosition_{ 0.0f, 1.7f, 9.0f };
    Vector3 cameraTarget_  { 0.0f, 1.0f, 0.0f };
    float yaw_   = kPi;
    float pitch_ = 0.0f;
};

int main() {
    Context context;
    Cube3DDemo app(&context);
    return app.Run();
}
