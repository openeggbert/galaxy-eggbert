#include "GalaxyEggbertGame.hpp"

#include <algorithm>
#include <cmath>
#include <random>

using namespace Urho3D;

GalaxyEggbertGame::GalaxyEggbertGame(Context* context)
    : context_(context) {}

GalaxyEggbertGame::~GalaxyEggbertGame() { Stop(); }

// ============================================================
// Shared stubs (Nova3D path — scene graph not yet implemented)
// ============================================================
#ifndef GE_ENGINE_U3D

void GalaxyEggbertGame::Start()  {}
void GalaxyEggbertGame::Update(float /*dt*/) {}
void GalaxyEggbertGame::Stop()   {}

#else // GE_ENGINE_U3D
// ============================================================
// U3D implementation — full Urho3D scene graph
// ============================================================

// --------------- helpers ---------------

SharedPtr<Material> GalaxyEggbertGame::MakeFlatMaterial(const Color& color, float emissive) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    SharedPtr<Material> mat(new Material(context_));

    auto* tech = cache->GetResource<Technique>("Techniques/NoTexture.xml");
    if (!tech) tech = cache->GetResource<Technique>("Techniques/Diff.xml");
    if (tech) mat->SetTechnique(0, tech);

    mat->SetShaderParameter("MatDiffColor",   color);
    mat->SetShaderParameter("MatEmissiveColor",
        Color(color.r_ * emissive, color.g_ * emissive, color.b_ * emissive));
    mat->SetShaderParameter("MatSpecColor",   Color(0.04f, 0.04f, 0.04f, 1.0f));
    return mat;
}

// --------------- scene creation ---------------

void GalaxyEggbertGame::CreateScene() {
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    // Atmosphere — galaxy alien world: deep blue-purple sky, distant fog
    auto* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));
    zone->SetAmbientColor(Color(0.22f, 0.18f, 0.38f));  // purple-blue ambient
    zone->SetFogColor(Color(0.12f, 0.10f, 0.28f));       // deep space fog
    zone->SetFogStart(80.0f);
    zone->SetFogEnd(220.0f);

    // Sun (slightly warm directional)
    auto* sunNode = scene_->CreateChild("Sun");
    sunNode->SetDirection(Vector3(-0.6f, -1.0f, -0.4f));
    auto* sun = sunNode->CreateComponent<Light>();
    sun->SetLightType(LIGHT_DIRECTIONAL);
    sun->SetColor(Color(1.0f, 0.94f, 0.80f));
    sun->SetBrightness(2.0f);
    sun->SetCastShadows(false);

    // Soft secondary fill
    auto* fillNode = scene_->CreateChild("Fill");
    fillNode->SetDirection(Vector3(0.65f, -0.4f, 0.5f));
    auto* fill = fillNode->CreateComponent<Light>();
    fill->SetLightType(LIGHT_DIRECTIONAL);
    fill->SetColor(Color(0.40f, 0.50f, 0.90f));
    fill->SetBrightness(0.60f);
    fill->SetCastShadows(false);
}

void GalaxyEggbertGame::CreateTerrain() {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* boxModel = cache->GetResource<Model>("Models/Box.mdl");
    if (!boxModel) return;

    // Galaxy-alien colour palette
    SharedPtr<Material> mats[5];
    mats[0] = MakeFlatMaterial(Color(0.18f, 0.72f, 0.52f), 0.18f); // teal alien grass
    mats[1] = MakeFlatMaterial(Color(0.28f, 0.22f, 0.45f), 0.14f); // purple rock
    mats[2] = MakeFlatMaterial(Color(0.50f, 0.50f, 0.62f), 0.12f); // blue-grey stone
    mats[3] = MakeFlatMaterial(Color(0.70f, 0.78f, 0.92f), 0.16f); // silver-blue sand
    mats[4] = MakeFlatMaterial(Color(0.10f, 0.40f, 0.90f), 0.24f); // crystal-blue water

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> gap(0, 99);
    std::uniform_int_distribution<int> matPick(0, 99);

    const int radius = 18;
    for (int z = -radius; z <= radius; ++z) {
        for (int x = -radius; x <= radius; ++x) {
            const bool spawn = (std::abs(x) <= 3 && std::abs(z) <= 3);
            if (!spawn && gap(rng) < 8) continue;

            const float hf = std::sin(x * 0.28f) * 1.7f
                           + std::cos(z * 0.24f) * 1.5f
                           + std::sin((x + z) * 0.13f) * 1.1f;
            std::uniform_int_distribution<int> jit(-1, 2);
            int height = spawn ? 1 : std::max(1, std::min(2 + static_cast<int>(std::round(hf)) + jit(rng), 7));

            for (int y = 0; y < height; ++y) {
                auto* node = scene_->CreateChild("Block");
                node->SetPosition(Vector3(static_cast<float>(x),
                                         static_cast<float>(y) - 0.5f,
                                         static_cast<float>(z)));
                auto* sm = node->CreateComponent<StaticModel>();
                sm->SetModel(boxModel);

                SharedPtr<Material> mat;
                if (y == height - 1) {
                    const int r = matPick(rng);
                    mat = mats[r < 65 ? 0 : r < 78 ? 2 : r < 90 ? 3 : 4];
                } else if (y > height - 4) {
                    mat = mats[1];
                } else {
                    mat = mats[2];
                }
                sm->SetMaterial(mat);
            }
        }
    }

    // Orbiting marker (glowing yellow — "Blupi" placeholder)
    auto* lightNode = scene_->CreateChild("OrbitLight");
    auto* orbitLight = lightNode->CreateComponent<Light>();
    orbitLight->SetLightType(LIGHT_POINT);
    orbitLight->SetColor(Color(1.0f, 0.85f, 0.20f));
    orbitLight->SetRange(20.0f);
    orbitLight->SetBrightness(1.6f);

    orbitNode_ = scene_->CreateChild("OrbitObject");
    orbitNode_->SetScale(Vector3(1.5f, 1.5f, 1.5f));
    lightNode->SetParent(orbitNode_.Get());
    lightNode->SetPosition(Vector3::ZERO);

    auto* orbitSM = orbitNode_->CreateComponent<StaticModel>();
    orbitSM->SetModel(boxModel);
    orbitSM->SetMaterial(MakeFlatMaterial(Color(1.0f, 0.85f, 0.12f), 0.30f));
}

void GalaxyEggbertGame::CreateCamera() {
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->SetPosition(Vector3(0.0f, 12.0f, -28.0f));
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    auto* cam = cameraNode_->CreateComponent<Camera>();
    cam->SetNearClip(0.1f);
    cam->SetFarClip(300.0f);

    auto* renderer = context_->GetSubsystem<Renderer>();
    SharedPtr<Viewport> vp(new Viewport(context_, scene_, cam));
    renderer->SetViewport(0, vp);
    renderer->SetDrawShadows(false);
}

void GalaxyEggbertGame::CreateHUD() {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* ui    = context_->GetSubsystem<UI>();
    if (!ui) return;

    auto* root = ui->GetRoot();
    auto* text = root->CreateChild<Text>();

    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");
    if (font) text->SetFont(font, 15);

    text->SetText(
        "Galaxy Eggbert  [Phase 1 / U3D]\n"
        "WASD: move  |  RMB: look  |  Shift: fast  |  Q/E: down/up\n"
        "F1: debug geometry  |  ESC: quit");
    text->SetColor(Color(0.85f, 0.90f, 1.0f));
    text->SetPosition(12, 12);
}

// --------------- lifecycle ---------------

void GalaxyEggbertGame::Start() {
    CreateScene();
    CreateTerrain();
    CreateCamera();
    CreateHUD();
}

void GalaxyEggbertGame::Stop() {
    scene_.Reset();
}

// --------------- per-frame ---------------

void GalaxyEggbertGame::UpdateCamera(float dt) {
    if (!cameraNode_) return;

    auto* input = context_->GetSubsystem<Input>();
    const float sens  = 0.12f;
    const bool  fast  = input->GetKeyDown(KEY_LSHIFT) || input->GetKeyDown(KEY_RSHIFT);
    const float speed = fast ? 28.0f : 12.0f;

    if (input->GetMouseButtonDown(MOUSEB_RIGHT)) {
        yaw_   += sens * static_cast<float>(input->GetMouseMoveX());
        pitch_ += sens * static_cast<float>(input->GetMouseMoveY());
        pitch_  = std::max(-80.0f, std::min(80.0f, pitch_));
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }

    Vector3 move = Vector3::ZERO;
    if (input->GetKeyDown(KEY_W)) move += Vector3::FORWARD;
    if (input->GetKeyDown(KEY_S)) move += Vector3::BACK;
    if (input->GetKeyDown(KEY_A)) move += Vector3::LEFT;
    if (input->GetKeyDown(KEY_D)) move += Vector3::RIGHT;
    if (move.LengthSquared() > 0.0f)
        cameraNode_->Translate(move.Normalized() * speed * dt, TS_LOCAL);

    if (input->GetKeyDown(KEY_Q))
        cameraNode_->Translate(Vector3::DOWN * speed * dt, TS_WORLD);
    if (input->GetKeyDown(KEY_E))
        cameraNode_->Translate(Vector3::UP   * speed * dt, TS_WORLD);
}

void GalaxyEggbertGame::UpdateOrbit(float dt) {
    if (!orbitNode_) return;
    elapsed_ += dt;
    const float r = 10.0f;
    orbitNode_->SetPosition(Vector3(
        std::cos(elapsed_ * 0.8f) * r,
        8.0f + std::sin(elapsed_ * 1.6f) * 1.2f,
        std::sin(elapsed_ * 0.8f) * r));
    orbitNode_->SetRotation(Quaternion(elapsed_ * 50.0f, elapsed_ * 75.0f, elapsed_ * 30.0f));
}

void GalaxyEggbertGame::Update(float dt) {
    auto* input  = context_->GetSubsystem<Input>();
    auto* engine = context_->GetSubsystem<Engine>();

    if (input->GetKeyPress(KEY_ESCAPE)) { engine->Exit(); return; }
    if (input->GetKeyPress(KEY_F1))     drawDebug_ = !drawDebug_;

    UpdateCamera(dt);
    UpdateOrbit(dt);

    if (drawDebug_ && scene_) {
        auto* dbg = scene_->GetComponent<DebugRenderer>();
        auto* oct = scene_->GetComponent<Octree>();
        if (dbg && oct) oct->DrawDebugGeometry(true);
    }
}

#endif // GE_ENGINE_U3D
