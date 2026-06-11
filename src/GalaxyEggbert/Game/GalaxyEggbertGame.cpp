#include "GalaxyEggbertGame.hpp"
#include "GalaxyEggbert/BlockTypes.hpp"

#include <cmath>
#include <cstdio>

using namespace Urho3D;
using namespace GalaxyEggbert;
using namespace GalaxyEggbert::Worlds;

GalaxyEggbertGame::GalaxyEggbertGame(Context* context)
    : context_(context) {}

GalaxyEggbertGame::~GalaxyEggbertGame() { Stop(); }

// ─── material helpers ────────────────────────────────────────────────────────

SharedPtr<Material> GalaxyEggbertGame::MakeFlatMaterial(const Color& color, float emissive) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    SharedPtr<Material> mat(new Material(context_));
    auto* tech = cache->GetResource<Technique>("Techniques/NoTexture.xml");
    if (!tech) tech = cache->GetResource<Technique>("Techniques/Diff.xml");
    if (tech) mat->SetTechnique(0, tech);
    mat->SetShaderParameter("MatDiffColor",
        Color(color.r_, color.g_, color.b_, color.a_));
    mat->SetShaderParameter("MatEmissiveColor",
        Color(color.r_ * emissive, color.g_ * emissive, color.b_ * emissive));
    mat->SetShaderParameter("MatSpecColor", Color(0.04f, 0.04f, 0.04f, 1.0f));
    return mat;
}

SharedPtr<Material> GalaxyEggbertGame::GetTileMaterial(uint16_t blockType) {
    auto it = tileMatCache_.find(blockType);
    if (it != tileMatCache_.end()) return it->second;

    SharedPtr<Material> mat;
    const int icon = BlockTypes::toIconIndex(blockType);

    if (icon >= 0 && objectSheet_) {
        mat = SharedPtr<Material>(new Material(context_));
        auto* cache = context_->GetSubsystem<ResourceCache>();
        auto* tech = cache->GetResource<Technique>("Techniques/Diff.xml");
        if (!tech) tech = cache->GetResource<Technique>("Techniques/DiffUnlit.xml");
        if (tech) mat->SetTechnique(0, tech);

        mat->SetTexture(TU_DIFFUSE, objectSheet_);
        mat->SetShaderParameter("MatDiffColor", Color(1.0f, 1.0f, 1.0f, 1.0f));
        mat->SetShaderParameter("MatSpecColor",  Color(0.02f, 0.02f, 0.02f, 1.0f));

        float uOff, vOff, uS, vS;
        BlockTypes::tileUV(icon, uOff, vOff, uS, vS);
        mat->SetShaderParameter("UOffset", Vector4(uS, 0.0f, 0.0f, uOff));
        mat->SetShaderParameter("VOffset", Vector4(0.0f, vS, 0.0f, vOff));
    } else {
        mat = MakeFlatMaterial(Color(0.45f, 0.45f, 0.50f));
    }

    tileMatCache_[blockType] = mat;
    return mat;
}

// ─── scene setup ─────────────────────────────────────────────────────────────

void GalaxyEggbertGame::CreateScene() {
    scene_ = new Scene(context_);
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();

    auto* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-2000.0f, 2000.0f));
    zone->SetAmbientColor(Color(0.28f, 0.24f, 0.42f));
    zone->SetFogColor(Color(0.14f, 0.12f, 0.30f));
    zone->SetFogStart(80.0f);
    zone->SetFogEnd(200.0f);

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
    fill->SetColor(Color(0.35f, 0.45f, 0.85f));
    fill->SetBrightness(0.55f);
    fill->SetCastShadows(false);
}

void GalaxyEggbertGame::CreateTerrain() {
    // Load object-m.png sprite sheet (shared by all tile materials).
    auto* cache = context_->GetSubsystem<ResourceCache>();
    objectSheet_ = cache->GetResource<Texture2D>("icons/object-m.png");

    auto* boxModel = cache->GetResource<Model>("Models/Box.mdl");
    if (!boxModel) return;

    world_ = std::make_unique<World>();

    // Build a demo Speedy-Blupi-style level centred on (kWCX, 0, kWCZ).
    // Flat grass floor with a few stone platforms.
    const int R = 15; // half-radius of the play area
    for (int dz = -R; dz <= R; ++dz) {
        for (int dx = -R; dx <= R; ++dx) {
            int wx = kWCX + dx;
            int wz = kWCZ + dz;
            if (wx < 0 || wz < 0 || wx >= 100 || wz >= 100) continue;
            world_->setBlock(static_cast<uint16_t>(wx), 0, static_cast<uint16_t>(wz),
                             Block::make(BlockTypes::Ground));
        }
    }

    // Stone border walls
    for (int dx = -R; dx <= R; ++dx) {
        auto f = [&](int wx, int wz) {
            if (wx >= 0 && wz >= 0 && wx < 100 && wz < 100) {
                world_->setBlock(static_cast<uint16_t>(wx), 1, static_cast<uint16_t>(wz), Block::make(BlockTypes::Wall));
                world_->setBlock(static_cast<uint16_t>(wx), 2, static_cast<uint16_t>(wz), Block::make(BlockTypes::Wall));
            }
        };
        f(kWCX + dx, kWCZ - R);
        f(kWCX + dx, kWCZ + R);
        f(kWCX - R, kWCZ + dx);
        f(kWCX + R, kWCZ + dx);
    }

    // Some stone-A platforms at various heights
    struct Plat { int dx, dz, h; uint16_t type; };
    const Plat platforms[] = {
        { 5, 3, 1, BlockTypes::StoneA }, { 5, 3, 2, BlockTypes::StoneA },
        {-5, 3, 1, BlockTypes::StoneA }, {-5, 3, 2, BlockTypes::StoneA },
        { 5,-5, 1, BlockTypes::StoneB }, { 5,-5, 2, BlockTypes::StoneB },
        { 0, 6, 1, BlockTypes::Platform},
        { 0,-6, 1, BlockTypes::Sp0    },
        { 8, 0, 1, BlockTypes::StoneA }, { 8, 0, 2, BlockTypes::StoneA }, { 8, 0, 3, BlockTypes::StoneA },
        {-8, 0, 1, BlockTypes::StoneB }, {-8, 0, 2, BlockTypes::StoneB },
    };
    for (const auto& p : platforms) {
        int wx = kWCX + p.dx;
        int wz = kWCZ + p.dz;
        if (wx >= 0 && wz >= 0 && wx < 100 && wz < 100)
            world_->setBlock(static_cast<uint16_t>(wx),
                             static_cast<uint16_t>(p.h),
                             static_cast<uint16_t>(wz),
                             Block::make(p.type));
    }

    // Spawn scene nodes for all non-air blocks.
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
                                static_cast<float>(wx) - kWCX,
                                static_cast<float>(wy),
                                static_cast<float>(wz) - kWCZ));
                            auto* sm = node->CreateComponent<StaticModel>();
                            sm->SetModel(boxModel);
                            sm->SetMaterial(GetTileMaterial(b.type()));
                        }
                    }
                }
            }
        }
    }
}

void GalaxyEggbertGame::CreateCamera() {
    cameraNode_ = scene_->CreateChild("Camera");

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
    auto* ui = context_->GetSubsystem<UI>();
    if (!ui) return;

    auto* root = ui->GetRoot();
    auto* text = root->CreateChild<Text>("HUD");
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    if (!font) font = cache->GetResource<Font>("Fonts/DejaVuSansMono.ttf");
    if (font) text->SetFont(font, 14);
    text->SetText("Galaxy Eggbert\nUP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause  RMB: rotate camera");
    text->SetColor(Color(0.85f, 0.90f, 1.0f));
    text->SetPosition(12, 12);
    hudText_ = text;
}

// ─── overlay (menu screens) ──────────────────────────────────────────────────

void GalaxyEggbertGame::ShowOverlay(const char* texPath) {
    HideOverlay();
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* ui = context_->GetSubsystem<UI>();
    if (!ui) return;

    auto* tex = cache->GetResource<Texture2D>(texPath);
    if (!tex) return;

    auto* root = ui->GetRoot();
    auto* img = root->CreateChild<BorderImage>("Overlay");
    img->SetTexture(tex);
    img->SetFullImageRect();
    img->SetSize(root->GetWidth(), root->GetHeight());
    img->SetAlignment(HA_LEFT, VA_TOP);
    img->SetOpacity(1.0f);
    overlayEl_ = img;
}

void GalaxyEggbertGame::HideOverlay() {
    if (UIElement* el = overlayEl_) {
        el->Remove();
        overlayEl_.Reset();
    }
}

// ─── phase transitions ───────────────────────────────────────────────────────

void GalaxyEggbertGame::EnterPhase(GamePhase next) {
    if (phase_ == next) return;
    phase_ = next;

    switch (next) {
        case GamePhase::Init:
            ShowOverlay("backgrounds/init.png");
            if (Text* t = hudText_) t->SetVisible(false);
            break;

        case GamePhase::Play:
            HideOverlay();
            if (!blupi_)
                blupi_ = std::make_unique<Blupi>(context_, scene_, world_.get(), kWCX, kWCZ);
            if (Text* t = hudText_) {
                t->SetText("UP/DOWN: move  LEFT/RIGHT: turn  SPACE: jump  ESC: pause  RMB: rotate camera");
                t->SetVisible(true);
            }
            break;

        case GamePhase::Pause:
            ShowOverlay("backgrounds/pause.png");
            if (Text* t = hudText_) t->SetVisible(false);
            break;

        default:
            break;
    }
}

// ─── per-frame ───────────────────────────────────────────────────────────────

void GalaxyEggbertGame::UpdateInit(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();
    // Any key or mouse click starts the game
    // Any common key or mouse click starts the game
    const Key startKeys[] = {
        KEY_SPACE, KEY_RETURN, KEY_W, KEY_A, KEY_S, KEY_D,
        KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT, KEY_Z, KEY_X
    };
    bool anyKey = input->GetMouseButtonPress(MOUSEB_LEFT);
    for (Key k : startKeys) { if (input->GetKeyPress(k)) { anyKey = true; break; } }
    if (anyKey) EnterPhase(GamePhase::Play);
}

void GalaxyEggbertGame::UpdatePlay(float dt) {
    auto* input = context_->GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_ESCAPE)) { EnterPhase(GamePhase::Pause); return; }

    if (blupi_) blupi_->Update(dt);
    UpdateCamera(dt);
}

void GalaxyEggbertGame::UpdatePause(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_ESCAPE)) EnterPhase(GamePhase::Play);
}

void GalaxyEggbertGame::UpdateCamera(float dt) {
    if (!cameraNode_) return;
    auto* input = context_->GetSubsystem<Input>();

    // Mouse-look with right button
    if (input->GetMouseButtonDown(MOUSEB_RIGHT)) {
        const float sens = 0.12f;
        camYaw_   += sens * static_cast<float>(input->GetMouseMoveX());
        camPitch_ += sens * static_cast<float>(input->GetMouseMoveY());
        camPitch_  = std::max(-60.0f, std::min(60.0f, camPitch_));
    }

    // Zoom with scroll
    camDist_ -= static_cast<float>(input->GetMouseMoveWheel()) * 1.5f;
    camDist_  = std::max(3.0f, std::min(40.0f, camDist_));

    // 3rd-person orbit around Blupi (or origin when no Blupi yet)
    Vector3 target = blupi_ ? blupi_->GetPosition() : Vector3::ZERO;

    float yawRad   = camYaw_   * static_cast<float>(M_PI) / 180.0f;
    float pitchRad = camPitch_ * static_cast<float>(M_PI) / 180.0f;

    Vector3 offset(
        camDist_ * std::sin(yawRad) * std::cos(pitchRad),
        camDist_ * std::sin(pitchRad),
        camDist_ * std::cos(yawRad) * std::cos(pitchRad));

    cameraNode_->SetPosition(target + offset);
    cameraNode_->LookAt(target + Vector3(0.0f, kWCZ == 50 ? 0.5f : 0.0f, 0.0f));

    (void)dt;
}

// ─── lifecycle ───────────────────────────────────────────────────────────────

void GalaxyEggbertGame::Start() {
    CreateScene();
    CreateTerrain();
    CreateCamera();
    CreateHUD();
    EnterPhase(GamePhase::Init);
}

void GalaxyEggbertGame::Stop() {
    blupi_.reset();
    HideOverlay();
    scene_.Reset();
    world_.reset();
    tileMatCache_.clear();
    objectSheet_.Reset();
}

void GalaxyEggbertGame::Update(float dt) {
    auto* engine = context_->GetSubsystem<Engine>();
    auto* input  = context_->GetSubsystem<Input>();

    if (phase_ == GamePhase::Init && input->GetKeyPress(KEY_ESCAPE)) {
        engine->Exit(); return;
    }

    if (input->GetKeyPress(KEY_F1)) drawDebug_ = !drawDebug_;

    switch (phase_) {
        case GamePhase::Init:  UpdateInit(dt);  break;
        case GamePhase::Play:  UpdatePlay(dt);  break;
        case GamePhase::Pause: UpdatePause(dt); break;
        default: break;
    }

    if (drawDebug_ && scene_) {
        auto* dbg = scene_->GetComponent<DebugRenderer>();
        auto* oct = scene_->GetComponent<Octree>();
        if (dbg && oct) oct->DrawDebugGeometry(true);
    }
}
