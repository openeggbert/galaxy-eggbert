#include "GalaxyEggbertGame.hpp"
#include "GalaxyEggbert/BlockTypes.hpp"
#include "GalaxyEggbert/def/SoundChannel.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>

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

    terrainRoot_ = scene_->CreateChild("Terrain");
}

void GalaxyEggbertGame::BuildDemoWorld() {
    const int R = 15;
    for (int dz = -R; dz <= R; ++dz) {
        for (int dx = -R; dx <= R; ++dx) {
            int wx = kWCX + dx, wz = kWCZ + dz;
            if (wx < 0 || wz < 0 || wx >= 100 || wz >= 100) continue;
            world_->setBlock(static_cast<uint16_t>(wx), 0, static_cast<uint16_t>(wz),
                             Block::make(BlockTypes::Ground));
        }
    }
    for (int dx = -R; dx <= R; ++dx) {
        auto wall = [&](int wx, int wz) {
            if (wx >= 0 && wz >= 0 && wx < 100 && wz < 100) {
                world_->setBlock(static_cast<uint16_t>(wx), 1, static_cast<uint16_t>(wz), Block::make(BlockTypes::Wall));
                world_->setBlock(static_cast<uint16_t>(wx), 2, static_cast<uint16_t>(wz), Block::make(BlockTypes::Wall));
            }
        };
        wall(kWCX + dx, kWCZ - R); wall(kWCX + dx, kWCZ + R);
        wall(kWCX - R, kWCZ + dx); wall(kWCX + R, kWCZ + dx);
    }
    struct Plat { int dx, dz, h; uint16_t type; };
    const Plat platforms[] = {
        { 10,  0, 1, BlockTypes::StoneA },
        {-10,  0, 1, BlockTypes::StoneB },
        {  0, 10, 1, BlockTypes::Platform },
        {  0,-10, 1, BlockTypes::Sp0 },
        { 10, 10, 1, BlockTypes::StoneA }, { 10, 10, 2, BlockTypes::StoneA },
        {-10, 10, 1, BlockTypes::StoneB }, {-10, 10, 2, BlockTypes::StoneB },
        { 10,-10, 1, BlockTypes::Wall   }, { 10,-10, 2, BlockTypes::Wall   },
        {-10,-10, 1, BlockTypes::StoneA }, {-10,-10, 2, BlockTypes::StoneA },
    };
    for (const auto& p : platforms) {
        int wx = kWCX + p.dx, wz = kWCZ + p.dz;
        if (wx >= 0 && wz >= 0 && wx < 100 && wz < 100)
            world_->setBlock(static_cast<uint16_t>(wx),
                             static_cast<uint16_t>(p.h),
                             static_cast<uint16_t>(wz),
                             Block::make(p.type));
    }

    // Lava strip at dz=-5 (hazard zone near south area)
    for (int dx = -3; dx <= 3; ++dx) {
        int wx = kWCX + dx, wz = kWCZ - 5;
        if (wx >= 0 && wz >= 0 && wx < 100 && wz < 100)
            world_->setBlock(static_cast<uint16_t>(wx), 0,
                             static_cast<uint16_t>(wz),
                             Block::make(BlockTypes::Lava));
    }
    // Spike tile at (6, 4)
    world_->setBlock(static_cast<uint16_t>(kWCX + 6), 0,
                     static_cast<uint16_t>(kWCZ + 4),
                     Block::make(BlockTypes::Spike));

    // Extra hazards scaled by world number
    for (int i = 1; i < currentWorld_; ++i) {
        int wx = kWCX - 6 + (i - 1) * 2;
        int wz = kWCZ + 6;
        if (wx >= 0 && wx < 100 && wz >= 0 && wz < 100)
            world_->setBlock(static_cast<uint16_t>(wx), 0,
                             static_cast<uint16_t>(wz),
                             Block::make(BlockTypes::Crusher));
    }
}

void GalaxyEggbertGame::SpawnTerrainNodes() {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* boxModel = cache->GetResource<Model>("Models/Box.mdl");
    if (!boxModel || !world_ || !terrainRoot_) return;

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
                            auto* node = terrainRoot_->CreateChild("Block");
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

void GalaxyEggbertGame::LoadWorld(int worldNum) {
    if (terrainRoot_) terrainRoot_->RemoveAllChildren();
    tileMatCache_.clear();

    objectSheet_ = context_->GetSubsystem<ResourceCache>()
        ->GetResource<Texture2D>("icons/object-m.png");

    char fname[64];
    std::snprintf(fname, sizeof(fname), "worlds/world%03d.vwr", worldNum);
    auto* fs = context_->GetSubsystem<FileSystem>();
    const String worldFile = fs->GetProgramDir() + fname;

    world_.reset();
    if (fs->FileExists(worldFile)) {
        try {
            world_ = std::make_unique<World>(World::loadFromFile(worldFile.CString()));
        } catch (...) {
            world_.reset();
        }
    }
    if (!world_) {
        world_ = std::make_unique<World>();
        BuildDemoWorld();
        std::filesystem::create_directories(
            std::filesystem::path(worldFile.CString()).parent_path());
        try { world_->saveToFile(worldFile.CString()); } catch (...) {}
    }

    SpawnTerrainNodes();
}

// ─── phase transitions ───────────────────────────────────────────────────────

void GalaxyEggbertGame::CreateDemoObjects() {
    using OT = GalaxyEggbert::ObjectType;
    decor_ = std::make_unique<Decor>(context_, scene_.Get());
    // Treasures (stationary)
    decor_->PlaceObject(OT::ObjectType5, Vector3(-5.0f, 1.0f,  0.0f));
    decor_->PlaceObject(OT::ObjectType5, Vector3( 0.0f, 1.0f,  5.0f));
    decor_->PlaceObject(OT::ObjectType5, Vector3( 5.0f, 1.0f, -5.0f));
    // Extra-life egg
    decor_->PlaceObject(OT::ObjectType6, Vector3(-3.0f, 1.0f, -6.0f));
    // Red key
    decor_->PlaceObject(OT::ObjectType49, Vector3( 3.0f, 1.0f, -3.0f));
    // Shield orb
    decor_->PlaceObject(OT::ObjectType25, Vector3(-2.0f, 1.0f,  4.0f));
    // Patrolling enemy A
    decor_->PlaceObject(OT::ObjectType2,
                        Vector3(-8.0f, 1.0f, 3.0f),
                        Vector3(-2.0f, 1.0f, 3.0f), 2.0f);
    // Patrolling enemy B (variant)
    decor_->PlaceObject(OT::ObjectType3,
                        Vector3( 4.0f, 1.0f, 8.0f),
                        Vector3( 9.0f, 1.0f, 8.0f), 2.5f);
    // Spider
    decor_->PlaceObject(OT::ObjectType16,
                        Vector3(-7.0f, 1.0f, -2.0f),
                        Vector3(-2.0f, 1.0f, -2.0f), 1.5f);
    // Level exit
    decor_->PlaceObject(OT::ObjectType7, Vector3(0.0f, 1.0f, -8.0f));

    // Extra patrol enemies added per world beyond 1
    for (int i = 1; i < currentWorld_ && i <= kMaxWorld; ++i) {
        float off = static_cast<float>(i) * 3.0f;
        decor_->PlaceObject(OT::ObjectType2,
            Vector3(-off, 1.0f, off - 10.0f),
            Vector3(-off + 4.0f, 1.0f, off - 10.0f),
            1.5f + static_cast<float>(i) * 0.4f);
    }
}

void GalaxyEggbertGame::EnterPhase(GamePhase next) {
    if (phases_->Current() == next) return;
    phases_->Enter(next);
    switch (next) {
        case GamePhase::Play:
            if (!blupi_)
                blupi_ = std::make_unique<Blupi>(context_, scene_.Get(), world_.get(), kWCX, kWCZ);
            if (!decor_) CreateDemoObjects();
            hud_->ShowPlay(Vector3::ZERO, 0.0f, lives_, 0, 0, 0.0f, currentWorld_);
            hud_->SetVisible(true);
            break;
        default:
            hud_->SetVisible(false);
            break;
    }
}

// ─── per-frame ───────────────────────────────────────────────────────────────

void GalaxyEggbertGame::UpdateInit(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();

    // Rebuild slot display text every frame so stats stay current
    char buf[512];
    int n = 0;
    n += std::snprintf(buf+n, sizeof(buf)-n, "Galaxy Eggbert\n\nSelect a gamer:\n\n");
    for (int i = 0; i < GameData::kMaxGamer; ++i) {
        int nbVies, mainDoors, secDoors;
        gameData_.GetGamerInfo(i, nbVies, mainDoors, secDoors);
        int lastWorld = gameData_.GetLastWorldForGamer(i);
        n += std::snprintf(buf+n, sizeof(buf)-n,
            "  [%d]  Lives: %d   World: %d   Doors: %d\n",
            i+1, nbVies, lastWorld, mainDoors);
    }
    std::snprintf(buf+n, sizeof(buf)-n,
        "\n1/2/3: choose gamer   S: Settings");
    phases_->SetOverlayText(buf);

    if (input->GetKeyPress(KEY_1)) { SelectGamer(0); return; }
    if (input->GetKeyPress(KEY_2)) { SelectGamer(1); return; }
    if (input->GetKeyPress(KEY_3)) { SelectGamer(2); return; }
    if (input->GetKeyPress(KEY_S)) {
        settingsReturnPhase_ = GamePhase::Init;
        EnterPhase(GamePhase::MainSetup);
    }
}

void GalaxyEggbertGame::SelectGamer(int slot) {
    gameData_.SetSelectedGamer(slot);
    lives_        = gameData_.GetNbVies();
    currentWorld_ = gameData_.GetLastWorld();
    gameData_.Write(savePath_);
    decor_.reset();
    blupi_.reset();
    LoadWorld(currentWorld_);
    EnterPhase(GamePhase::Play);
}

void GalaxyEggbertGame::UpdateSettings(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();

    bool soundOn = gameData_.GetSounds();
    char buf[128];
    std::snprintf(buf, sizeof(buf),
        "Settings\n\n"
        "Sound: %s\n\n"
        "S: toggle sound   ESC: back",
        soundOn ? "ON" : "OFF");
    phases_->SetOverlayText(buf);

    if (input->GetKeyPress(KEY_S)) {
        gameData_.SetSounds(!soundOn);
        gameData_.Write(savePath_);
        if (sound_) sound_->SetEnabled(gameData_.GetSounds());
    }
    if (input->GetKeyPress(KEY_ESCAPE)) {
        EnterPhase(settingsReturnPhase_);
    }
}

void GalaxyEggbertGame::UpdatePlay(float dt) {
    auto* input = context_->GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_ESCAPE)) { EnterPhase(GamePhase::Pause); return; }

    if (blupi_) blupi_->Update(dt);
    if (blupi_ && blupi_->WasJumpedThisFrame() && sound_)
        sound_->Play(SoundChannel::SoundChannel1);

    Vector3 pos = blupi_ ? blupi_->GetPosition() : Vector3::ZERO;
    float   yaw = blupi_ ? blupi_->GetFacingYaw() : 0.0f;
    camera_->Update(dt, pos, yaw);

    // Shield countdown
    if (shieldTimer_ > 0.0f) shieldTimer_ -= dt;

    // Tile hazard check — only when standing on ground
    if (blupi_ && blupi_->IsOnGround() && world_ && shieldTimer_ <= 0.0f) {
        int wx = static_cast<int>(std::round(pos.x_)) + kWCX;
        int wz = static_cast<int>(std::round(pos.z_)) + kWCZ;
        int wy = static_cast<int>(std::floor(pos.y_ - Blupi::kHalfH));
        if (wx >= 0 && wx < 100 && wy >= 0 && wy < 100 && wz >= 0 && wz < 100) {
            uint16_t bt = world_->getBlock(
                static_cast<uint16_t>(wx),
                static_cast<uint16_t>(wy),
                static_cast<uint16_t>(wz)).type();
            if (bt == BlockTypes::Lava || bt == BlockTypes::Spike || bt == BlockTypes::Crusher) {
                if (sound_) sound_->Play(SoundChannel::SoundChannel8);
                --lives_;
                if (lives_ <= 0) {
                    gameData_.SetNbVies(3);
                    gameData_.Write(savePath_);
                    EnterPhase(GamePhase::Lost);
                    return;
                }
                gameData_.SetNbVies(lives_);
                gameData_.Write(savePath_);
                blupi_->Respawn();
                pos = blupi_->GetPosition();
            }
        }
    }

    if (decor_) {
        decor_->Update(dt, pos);

        // Shield pickup
        if (decor_->WasShieldCollected()) {
            shieldTimer_ = 5.0f;
            if (sound_) sound_->Play(SoundChannel::SoundChannel42);
        }

        // Key pickup
        int keys = decor_->GetKeysCollected();
        if (keys > keysCollected_) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel11);
            keysCollected_ = keys;
        }

        // Treasure pickup
        int collected = decor_->GetCollected();
        if (collected > prevCollected_) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel10);
            prevCollected_ = collected;
        }

        if (decor_->WasExitReached()) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel57);
            ++currentWorld_;
            gameData_.SetNbVies(lives_);
            gameData_.SetLastWorld(currentWorld_);
            gameData_.Write(savePath_);
            EnterPhase(GamePhase::Win);
            return;
        }
        if (decor_->WasBlupiHit() && shieldTimer_ <= 0.0f) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel8);
            --lives_;
            if (lives_ <= 0) {
                gameData_.SetNbVies(3);
                gameData_.Write(savePath_);
                EnterPhase(GamePhase::Lost);
                return;
            }
            gameData_.SetNbVies(lives_);
            gameData_.Write(savePath_);
            if (blupi_) blupi_->Respawn();
        }
        decor_->ClearEvents();
    }

    hud_->ShowPlay(pos, yaw, lives_,
                   decor_ ? decor_->GetCollected() : 0,
                   keysCollected_, shieldTimer_, currentWorld_);
}

void GalaxyEggbertGame::AdvanceToNextWorld() {
    if (currentWorld_ > kMaxWorld) {
        currentWorld_ = 1;
        gameData_.SetLastWorld(1);
        gameData_.Write(savePath_);
    }
    prevCollected_ = 0;
    keysCollected_ = 0;
    shieldTimer_   = 0.0f;
    decor_.reset();
    blupi_.reset();
    LoadWorld(currentWorld_);
    EnterPhase(GamePhase::Play);
}

void GalaxyEggbertGame::UpdateWin(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();
    const Key keys[] = { KEY_SPACE, KEY_RETURN, KEY_ESCAPE,
                         KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT };
    for (Key k : keys) {
        if (input->GetKeyPress(k)) { AdvanceToNextWorld(); return; }
    }
    if (input->GetMouseButtonPress(MOUSEB_LEFT)) AdvanceToNextWorld();
}

void GalaxyEggbertGame::UpdateLost(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();
    const Key keys[] = { KEY_SPACE, KEY_RETURN, KEY_ESCAPE,
                         KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT };
    for (Key k : keys) {
        if (input->GetKeyPress(k)) { ResetLevel(); return; }
    }
    if (input->GetMouseButtonPress(MOUSEB_LEFT)) ResetLevel();
}

void GalaxyEggbertGame::ResetLevel() {
    lives_         = 3;
    currentWorld_  = 1;
    prevCollected_ = 0;
    keysCollected_ = 0;
    shieldTimer_   = 0.0f;
    gameData_.SetNbVies(3);
    gameData_.SetLastWorld(1);
    gameData_.Write(savePath_);
    decor_.reset();
    blupi_.reset();
    EnterPhase(GamePhase::Init);
}

void GalaxyEggbertGame::UpdatePause(float dt) {
    (void)dt;
    auto* input = context_->GetSubsystem<Input>();
    if (input->GetKeyPress(KEY_ESCAPE)) { EnterPhase(GamePhase::Play); return; }
    if (input->GetKeyPress(KEY_S)) {
        settingsReturnPhase_ = GamePhase::Pause;
        EnterPhase(GamePhase::PlaySetup);
    }
}

// ─── lifecycle ───────────────────────────────────────────────────────────────

void GalaxyEggbertGame::Start() {
    auto* fs = context_->GetSubsystem<FileSystem>();
    savePath_ = fs->GetUserDocumentsDir().CString();
    savePath_ += "GalaxyEggbert/save.dat";

    if (gameData_.Read(savePath_)) {
        lives_        = gameData_.GetNbVies();
        currentWorld_ = gameData_.GetLastWorld();
    }

    CreateScene();
    LoadWorld(currentWorld_);
    phases_ = std::make_unique<PhaseManager>(context_);
    hud_    = std::make_unique<HUD>(context_);
    camera_ = std::make_unique<CameraController>(context_, scene_.Get());
    sound_  = std::make_unique<SoundManager>(context_, scene_.Get());
    if (!gameData_.GetSounds()) sound_->SetEnabled(false);
    EnterPhase(GamePhase::Init);
}

void GalaxyEggbertGame::Stop() {
    if (!savePath_.empty()) {
        gameData_.SetNbVies(lives_);
        gameData_.SetLastWorld(currentWorld_);
        gameData_.Write(savePath_);
    }
    if (sound_) sound_->StopAll();
    decor_.reset();
    blupi_.reset();
    phases_.reset();
    hud_.reset();
    camera_.reset();
    sound_.reset();
    terrainRoot_.Reset();
    scene_.Reset();
    world_.reset();
    tileMatCache_.clear();
    objectSheet_.Reset();
}

void GalaxyEggbertGame::Update(float dt) {
    if (!phases_) return;
    auto* engine = context_->GetSubsystem<Engine>();
    auto* input  = context_->GetSubsystem<Input>();
    const GamePhase phase = phases_->Current();

    if (phase == GamePhase::Init && input->GetKeyPress(KEY_ESCAPE)) {
        engine->Exit(); return;
    }

    if (input->GetKeyPress(KEY_F1)) drawDebug_ = !drawDebug_;

    switch (phase) {
        case GamePhase::Init:      UpdateInit(dt);     break;
        case GamePhase::Play:      UpdatePlay(dt);     break;
        case GamePhase::Pause:     UpdatePause(dt);    break;
        case GamePhase::Win:       UpdateWin(dt);      break;
        case GamePhase::Lost:      UpdateLost(dt);     break;
        case GamePhase::MainSetup:
        case GamePhase::PlaySetup: UpdateSettings(dt); break;
        default: break;
    }

    if (drawDebug_ && scene_) {
        auto* dbg = scene_->GetComponent<DebugRenderer>();
        auto* oct = scene_->GetComponent<Octree>();
        if (dbg && oct) oct->DrawDebugGeometry(true);
    }
}
