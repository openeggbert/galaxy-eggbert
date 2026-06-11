#include "GalaxyEggbertGame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>

using namespace Urho3D;
using namespace GalaxyEggbert::Worlds;

GalaxyEggbertGame::GalaxyEggbertGame(Context* context)
    : context_(context) {}

GalaxyEggbertGame::~GalaxyEggbertGame() { Stop(); }

// --------------- material helpers ---------------

SharedPtr<Material> GalaxyEggbertGame::MakeFlatMaterial(const Color& color, float emissive) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    SharedPtr<Material> mat(new Material(context_));

    auto* tech = cache->GetResource<Technique>("Techniques/NoTexture.xml");
    if (!tech) tech = cache->GetResource<Technique>("Techniques/Diff.xml");
    if (tech) mat->SetTechnique(0, tech);

    mat->SetShaderParameter("MatDiffColor",    color);
    mat->SetShaderParameter("MatEmissiveColor",
        Color(color.r_ * emissive, color.g_ * emissive, color.b_ * emissive));
    mat->SetShaderParameter("MatSpecColor",    Color(0.04f, 0.04f, 0.04f, 1.0f));
    return mat;
}

SharedPtr<Material> GalaxyEggbertGame::GetTileMaterial(uint16_t blockType) {
    auto it = tileMatCache_.find(blockType);
    if (it != tileMatCache_.end()) return it->second;

    auto* cache = context_->GetSubsystem<ResourceCache>();

    char texPath[64];
    snprintf(texPath, sizeof(texPath), "backgrounds/decor%03d.png", blockType);
    auto* tex = cache->GetResource<Texture2D>(texPath);

    SharedPtr<Material> mat;
    if (tex) {
        mat = SharedPtr<Material>(new Material(context_));
        auto* tech = cache->GetResource<Technique>("Techniques/Diff.xml");
        if (!tech) tech = cache->GetResource<Technique>("Techniques/DiffUnlit.xml");
        if (tech) mat->SetTechnique(0, tech);
        mat->SetTexture(TU_DIFFUSE, tex);
        mat->SetShaderParameter("MatDiffColor",    Color(1.0f, 1.0f, 1.0f, 1.0f));
        mat->SetShaderParameter("MatSpecColor",    Color(0.02f, 0.02f, 0.02f, 1.0f));
    } else {
        // fallback: flat grey for tile types without a texture file
        mat = MakeFlatMaterial(Color(0.45f, 0.45f, 0.50f));
    }

    tileMatCache_[blockType] = mat;
    return mat;
}

// --------------- scene creation ---------------

void GalaxyEggbertGame::CreateScene() {
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    auto* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));
    zone->SetAmbientColor(Color(0.22f, 0.18f, 0.38f));
    zone->SetFogColor(Color(0.12f, 0.10f, 0.28f));
    zone->SetFogStart(80.0f);
    zone->SetFogEnd(220.0f);

    auto* sunNode = scene_->CreateChild("Sun");
    sunNode->SetDirection(Vector3(-0.6f, -1.0f, -0.4f));
    auto* sun = sunNode->CreateComponent<Light>();
    sun->SetLightType(LIGHT_DIRECTIONAL);
    sun->SetColor(Color(1.0f, 0.94f, 0.80f));
    sun->SetBrightness(2.0f);
    sun->SetCastShadows(false);

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

    // Build world data model — terrain stored as blocks in the World.
    // World is 100×100×100; we use the centre (50, 0, 50) as scene origin.
    world_ = std::make_unique<World>();

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> gap(0, 99);

    // Surface tile types: decor001–decor013 (files that exist in Content/backgrounds/)
    // 0 = air; use types 1-13 for surface variety, type 2 for subsurface stone.
    const std::vector<uint16_t> surfaceTypes = {1, 2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13};
    std::uniform_int_distribution<int> typePick(0, static_cast<int>(surfaceTypes.size()) - 1);

    const int radius = 18;
    const int wcx = 50, wcz = 50; // world-space centre

    for (int wz = wcz - radius; wz <= wcz + radius; ++wz) {
        for (int wx = wcx - radius; wx <= wcx + radius; ++wx) {
            const int rx = wx - wcx, rz = wz - wcz;
            const bool spawn = (std::abs(rx) <= 3 && std::abs(rz) <= 3);
            if (!spawn && gap(rng) < 8) continue;

            const float hf = std::sin(rx * 0.28f) * 1.7f
                           + std::cos(rz * 0.24f) * 1.5f
                           + std::sin((rx + rz) * 0.13f) * 1.1f;
            std::uniform_int_distribution<int> jit(-1, 2);
            int height = spawn ? 1
                               : std::max(1, std::min(2 + static_cast<int>(std::round(hf)) + jit(rng), 7));

            for (int wy = 0; wy < height; ++wy) {
                uint16_t blockType = (wy == height - 1)
                    ? surfaceTypes[typePick(rng)]
                    : uint16_t(2); // subsurface stone (decor002)
                world_->setBlock(
                    static_cast<uint16_t>(wx),
                    static_cast<uint16_t>(wy),
                    static_cast<uint16_t>(wz),
                    Block::make(blockType));
            }
        }
    }

    // Spawn one StaticModel node per non-air block, iterating chunk by chunk
    // so empty chunks (the vast majority of the 100×100×100 world) are skipped.
    const uint8_t cpa = world_->chunksPerAxis();
    for (uint8_t ccy = 0; ccy < cpa; ++ccy) {
        for (uint8_t ccz = 0; ccz < cpa; ++ccz) {
            for (uint8_t ccx = 0; ccx < cpa; ++ccx) {
                if (world_->chunk(ccx, ccy, ccz).isEmpty()) continue;
                for (uint8_t ly = 0; ly < 10; ++ly) {
                    for (uint8_t lz = 0; lz < 10; ++lz) {
                        for (uint8_t lx = 0; lx < 10; ++lx) {
                            const uint16_t wx = ccx * 10 + lx;
                            const uint16_t wy = ccy * 10 + ly;
                            const uint16_t wz = ccz * 10 + lz;
                            const Block b = world_->getBlock(wx, wy, wz);
                            if (b.isAir()) continue;

                            auto* node = scene_->CreateChild("Block");
                            node->SetPosition(Vector3(
                                static_cast<float>(wx) - wcx,
                                static_cast<float>(wy) - 0.5f,
                                static_cast<float>(wz) - wcz));
                            auto* sm = node->CreateComponent<StaticModel>();
                            sm->SetModel(boxModel);
                            sm->SetMaterial(GetTileMaterial(b.type()));
                        }
                    }
                }
            }
        }
    }

    // Orbiting Blupi placeholder
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
        "Galaxy Eggbert  [Phase 3 / U3D — World data model]\n"
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
    world_.reset();
    tileMatCache_.clear();
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
