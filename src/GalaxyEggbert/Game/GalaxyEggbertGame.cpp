#include "GalaxyEggbertGame.hpp"
#include "GalaxyEggbert/BlockTypes.hpp"
#include "GalaxyEggbert/def/SoundChannel.hpp"

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace Urho3D;
using namespace GalaxyEggbert;
using namespace GalaxyEggbert::Worlds;

static const char* WorldName(int world) {
    static const char* kNames[] = {
        "Grassland", "Forest", "Ice Caves", "Lava Fields", "Space Station"
    };
    int idx = world - 1;
    return (idx >= 0 && idx < 5) ? kNames[idx] : "Unknown";
}

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

// World-specific sky and fog palette.
// Derived from the visual theme of each Speedy Blupi world.
static void ApplyWorldSky(Urho3D::Scene* scene, int world) {
    using namespace Urho3D;
    struct SkyPalette { Color ambient; Color fog; };
    static const SkyPalette kPalette[] = {
        { Color(0.28f, 0.38f, 0.22f), Color(0.55f, 0.72f, 0.42f) }, // 1 Grassland
        { Color(0.18f, 0.28f, 0.16f), Color(0.12f, 0.20f, 0.10f) }, // 2 Forest
        { Color(0.30f, 0.35f, 0.48f), Color(0.62f, 0.72f, 0.88f) }, // 3 Ice Caves
        { Color(0.38f, 0.16f, 0.08f), Color(0.22f, 0.08f, 0.04f) }, // 4 Lava Fields
        { Color(0.05f, 0.06f, 0.18f), Color(0.02f, 0.03f, 0.10f) }, // 5 Space Station
    };
    int idx = std::max(0, std::min(world - 1, 4));
    auto* zoneNode = scene->GetChild("Zone");
    if (!zoneNode) return;
    auto* zone = zoneNode->GetComponent<Zone>();
    if (!zone) return;
    zone->SetAmbientColor(kPalette[idx].ambient);
    zone->SetFogColor(kPalette[idx].fog);
}

void GalaxyEggbertGame::UpdateSkyDome(int region) {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* skyNode = scene_->GetChild("SkyDome");
    if (!skyNode) {
        skyNode = scene_->CreateChild("SkyDome");
        skyNode->SetScale(500.0f);
        auto* sky = skyNode->CreateComponent<StaticModel>();
        sky->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
    }
    auto* sky = skyNode->GetComponent<StaticModel>();
    if (!sky) return;
    char path[64];
    std::snprintf(path, sizeof(path), "backgrounds/decor%03d.png", region);
    auto* tex = cache->GetResource<Texture2D>(path);
    if (!tex) return;
    SharedPtr<Material> mat(new Material(context_));
    auto* tech = cache->GetResource<Technique>("Techniques/DiffSkydome.xml");
    if (tech) mat->SetTechnique(0, tech);
    mat->SetTexture(TU_DIFFUSE, tex);
    sky->SetMaterial(mat);
}

void GalaxyEggbertGame::SpawnTerrainNodes() {
    auto* cache = context_->GetSubsystem<ResourceCache>();
    auto* boxModel = cache->GetResource<Model>("Models/Box.mdl");
    if (!boxModel || !world_ || !terrainRoot_) return;

    // Dark earthy colour for fill blocks extruded below exposed cliff edges.
    SharedPtr<Material> fillMat = MakeFlatMaterial(Color(0.22f, 0.19f, 0.17f));
    static constexpr int kFillDepth = 3;
    static constexpr int kNbDir[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

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

                            // Edge fill: if any horizontal neighbour is air or out-of-bounds,
                            // extrude kFillDepth dark blocks downward to create cliff depth.
                            int iwx = static_cast<int>(wx), iwz = static_cast<int>(wz);
                            bool isEdge = false;
                            for (auto& nb : kNbDir) {
                                int nx = iwx + nb[0], nz = iwz + nb[1];
                                if (nx < 0 || nx >= 100 || nz < 0 || nz >= 100) {
                                    isEdge = true; break;
                                }
                                if (world_->getBlock(
                                        static_cast<uint16_t>(nx), wy,
                                        static_cast<uint16_t>(nz)).isAir()) {
                                    isEdge = true; break;
                                }
                            }
                            if (isEdge) {
                                for (int dy = 1; dy <= kFillDepth; ++dy) {
                                    auto* fill = terrainRoot_->CreateChild("BlockFill");
                                    fill->SetPosition(Vector3(
                                        static_cast<float>(iwx) - kWCX,
                                        static_cast<float>(wy)  - static_cast<float>(dy),
                                        static_cast<float>(iwz) - kWCZ));
                                    auto* fSm = fill->CreateComponent<StaticModel>();
                                    fSm->SetModel(boxModel);
                                    fSm->SetMaterial(fillMat);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

bool GalaxyEggbertGame::LoadMobileEggbertTerrain(const char* path) {
    // Tile size in mobile-eggbert world coordinates: 64 px per tile.
    // Positions in MoveObject and blupiPos are absolute pixel coords.
    // The 100x100 decor grid starts at tile (0,0) = pixel (0,0).
    // posDecor in the header is the initial scroll/camera offset — not the grid origin.
    static constexpr int kMobTile = 64;

    std::ifstream f(path);
    if (!f) return false;

    int blupiPosX = 0, blupiPosY = 0;
    {
        std::string header;
        if (!std::getline(f, header)) return false;
        const char* bp = std::strstr(header.c_str(), "blupiPos=");
        if (bp) std::sscanf(bp, "blupiPos=%d;%d", &blupiPosX, &blupiPosY);
        const char* rg = std::strstr(header.c_str(), "region=");
        if (rg) std::sscanf(rg, "region=%d", &skyRegion_);
    }

    const int blupiTileCol = blupiPosX / kMobTile;
    const int blupiTileRow = blupiPosY / kMobTile;

    // Store Blupi's world-space 3D spawn position.
    // world_x = tile_col (colOff=0), 3D X = world_x - kWCX = tile_col - kWCX.
    blupiSpawn_ = Vector3(
        static_cast<float>(blupiTileCol - (int)kWCX),
        Blupi::kHalfH + 0.5f,
        static_cast<float>(blupiTileRow - (int)kWCZ));

    mobileObjects_.clear();

    std::string line;
    int decorRow = 0;
    bool inDecor = false;

    while (std::getline(f, line)) {
        if (line.find("Decor:") == 0) { inDecor = true; continue; }

        if (line.rfind("MoveObject:", 0) == 0) {
            inDecor = false;
            int type = 0, psx = 0, psy = 0, pex = 0, pey = 0, stepAdv = 1;
            std::sscanf(line.c_str(),
                "MoveObject: type=%d stepAdvance=%d %*s %*s %*s posStart=%d;%d posEnd=%d;%d",
                &type, &stepAdv, &psx, &psy, &pex, &pey);

            bool supported = (type == 1  || type == 2  || type == 3  || type == 4  || type == 5  ||
                              type == 6  || type == 7  || type == 12 || type == 13 || type == 16 ||
                              type == 17 || type == 20 || type == 25 || type == 30 || type == 33 ||
                              type == 49 ||
                              type == 50 || type == 51);
            if (!supported) continue;

            // pixel → 3D: tile = px/64, 3D = tile - kW (colOff=0, rowOff=0)
            auto pixToV3 = [&](int px, int py) -> Vector3 {
                return Vector3(
                    static_cast<float>(px / kMobTile - (int)kWCX),
                    1.0f,
                    static_cast<float>(py / kMobTile - (int)kWCZ));
            };
            MobileObjSpec spec;
            spec.type     = static_cast<ObjectType>(type);
            spec.posStart = pixToV3(psx, psy);
            spec.posEnd   = pixToV3(pex, pey);
            spec.speed    = std::max(0.5f, static_cast<float>(stepAdv) / 3.0f);

            // Patrol enemies stored with posStart==posEnd — give default ±2 tile X patrol.
            bool isPatrol = (type == 2 || type == 3 || type == 4 || type == 20 || type == 33);
            if (isPatrol && spec.posStart.x_ == spec.posEnd.x_ &&
                            spec.posStart.z_ == spec.posEnd.z_) {
                spec.posStart.x_ -= 2.0f;
                spec.posEnd.x_   += 2.0f;
            }
            // Birds fly above ground — lift them to aerial altitude.
            if (type == 20) {
                spec.posStart.y_ = 3.0f;
                spec.posEnd.y_   = 3.0f;
            }
            // Spiders hang at ceiling height and drop to ground (vertical oscillation).
            if (type == 16) {
                spec.posStart.y_ = 4.0f;
                spec.posEnd.y_   = 1.0f;
            }
            mobileObjects_.push_back(spec);
            continue;
        }

        if (!inDecor || decorRow >= 100) continue;

        // Parse CSV row of tile IDs; decor row r maps directly to world_z=r.
        std::stringstream ss(line);
        std::string token;
        int col = 0;
        while (col < 100 && std::getline(ss, token, ',')) {
            if (!token.empty()) {
                int tileId = std::stoi(token);
                if (tileId > 0) {
                    uint16_t bt = BlockTypes::fromMobileIconId(tileId);
                    // col = world_x, decorRow = world_z (colOff=rowOff=0)
                    world_->setBlock(static_cast<uint16_t>(col), 0,
                                     static_cast<uint16_t>(decorRow),
                                     Block::make(bt));
                }
            }
            ++col;
        }
        ++decorRow;
    }
    return true;
}

void GalaxyEggbertGame::LoadWorld(int worldNum) {
    if (terrainRoot_) terrainRoot_->RemoveAllChildren();
    tileMatCache_.clear();
    mobileObjects_.clear();
    skyRegion_ = 0;

    objectSheet_ = context_->GetSubsystem<ResourceCache>()
        ->GetResource<Texture2D>("icons/object-m.png");

    auto* fs = context_->GetSubsystem<FileSystem>();
    char fname[64];

    std::snprintf(fname, sizeof(fname), "worlds/world%03d.vwr", worldNum);
    const String worldVwr = fs->GetProgramDir() + fname;

    std::snprintf(fname, sizeof(fname), "worlds/world%03d.txt", worldNum);
    const String worldTxt = fs->GetProgramDir() + fname;

    world_.reset();
    if (fs->FileExists(worldVwr)) {
        try {
            world_ = std::make_unique<World>(World::loadFromFile(worldVwr.CString()));
        } catch (...) { world_.reset(); }
    }
    if (!world_ && fs->FileExists(worldTxt)) {
        world_ = std::make_unique<World>();
        if (!LoadMobileEggbertTerrain(worldTxt.CString()))
            world_.reset();
    }
    if (!world_) {
        world_ = std::make_unique<World>();
        BuildDemoWorld();
        std::filesystem::create_directories(
            std::filesystem::path(worldVwr.CString()).parent_path());
        try { world_->saveToFile(worldVwr.CString()); } catch (...) {}
    }

    SpawnTerrainNodes();
    ApplyWorldSky(scene_.Get(), worldNum);
    UpdateSkyDome(skyRegion_);
    if (camera_) camera_->SetCollisionWorld(world_.get(), kWCX, kWCZ);
}

// ─── phase transitions ───────────────────────────────────────────────────────

void GalaxyEggbertGame::CreateDemoObjects() {
    using OT = GalaxyEggbert::ObjectType;
    decor_ = std::make_unique<Decor>(context_, scene_.Get());

    // Use real mobile-eggbert objects when a .txt world was loaded.
    if (!mobileObjects_.empty()) {
        for (auto& s : mobileObjects_)
            decor_->PlaceObject(s.type, s.posStart, s.posEnd, s.speed);
        return;
    }
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
    auto* input = context_->GetSubsystem<Input>();
    switch (next) {
        case GamePhase::Play:
            if (!blupi_)
                blupi_ = std::make_unique<Blupi>(context_, scene_.Get(), world_.get(), kWCX, kWCZ);
            if (!decor_) CreateDemoObjects();
            blupi_->SetSpawnPoint(blupiSpawn_);
            blupi_->Respawn();
            hud_->ShowPlay(lives_, 0, decor_ ? decor_->GetTotalTreasures() : 0,
                           0, 0, 0, 0.0f, currentWorld_);
            hud_->SetVisible(true);
            if (input) input->SetMouseVisible(false);
            break;
        default:
            hud_->SetVisible(false);
            if (input) input->SetMouseVisible(true);
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
    lives_             = gameData_.GetNbVies();
    currentWorld_      = gameData_.GetLastWorld();
    controlsHintTimer_ = 8.0f;
    bonusLifeAwarded_  = false;
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

    // Debug world jump: F1-F5 skips directly to that world.
    {
        static const Key kFKeys[] = { KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5 };
        for (int i = 0; i < 5; ++i) {
            if (input->GetKeyPress(kFKeys[i])) {
                currentWorld_           = i + 1;
                controlsHintTimer_      = 8.0f;
                bonusLifeAwarded_       = false;
                prevCollected_          = 0;
                keysRed_ = keysGreen_ = keysBlue_ = 0;
                shieldTimer_            = 0.0f;
                respawnInvincibleTimer_ = 0.0f;
                decor_.reset();
                blupi_.reset();
                LoadWorld(currentWorld_);
                EnterPhase(GamePhase::Play);
                return;
            }
        }
    }

    if (blupi_) blupi_->Update(dt);
    if (blupi_ && sound_) {
        if (blupi_->WasJumpedThisFrame())  sound_->Play(SoundChannel::SoundChannel1);
        if (blupi_->WasLandedThisFrame())  sound_->Play(SoundChannel::SoundChannel4);
    }

    // Fall death: deduct life, respawn with invincibility
    if (blupi_ && blupi_->WasFallDeath()) {
        blupi_->ClearFallDeath();
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
        respawnInvincibleTimer_ = 2.0f;
        blupi_->StartFlash(2.0f);
    }

    Vector3 pos = blupi_ ? blupi_->GetPosition() : Vector3::ZERO;
    float   yaw = blupi_ ? blupi_->GetFacingYaw() : 0.0f;
    camera_->Update(dt, pos, yaw);

    // Countdown timers
    if (shieldTimer_ > 0.0f)           shieldTimer_            -= dt;
    if (respawnInvincibleTimer_ > 0.0f) respawnInvincibleTimer_ -= dt;
    if (controlsHintTimer_ > 0.0f)     controlsHintTimer_      -= dt;
    if (blupi_) blupi_->SetShieldActive(shieldTimer_ > 0.0f);

    // Tile hazard check — only when standing on ground and not invincible
    if (blupi_ && blupi_->IsOnGround() && world_ && shieldTimer_ <= 0.0f && respawnInvincibleTimer_ <= 0.0f) {
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
                respawnInvincibleTimer_ = 2.0f;
                blupi_->StartFlash(2.0f);
                pos = blupi_->GetPosition();
            }
        }
    }

    if (decor_) {
        float velY = blupi_ ? blupi_->GetVelY() : 0.0f;
        decor_->Update(dt, pos, velY);

        // Stomp kill: Blupi jumped on an enemy.
        if (decor_->WasStompKill()) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel5);
            if (blupi_) blupi_->Bounce();
        }

        // Shield pickup
        if (decor_->WasShieldCollected()) {
            shieldTimer_ = 5.0f;
            if (sound_) sound_->Play(SoundChannel::SoundChannel42);
        }

        // Key pickup — track per type so HUD shows correct icon colours.
        {
            int k49 = decor_->GetKeys49(), k50 = decor_->GetKeys50(), k51 = decor_->GetKeys51();
            if (k49 > keysRed_ || k50 > keysGreen_ || k51 > keysBlue_) {
                if (sound_) sound_->Play(SoundChannel::SoundChannel11);
                keysRed_ = k49; keysGreen_ = k50; keysBlue_ = k51;
            }
        }

        // Treasure pickup
        int collected = decor_->GetCollected();
        if (collected > prevCollected_) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel10);
            prevCollected_ = collected;
        }

        // Egg pickup: grants +1 life (cap 9).
        if (decor_->WasEggCollected()) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel42);
            if (lives_ < 9) {
                ++lives_;
                gameData_.SetNbVies(lives_);
                gameData_.Write(savePath_);
            }
        }

        // Drink pickup: grants +1 life (cap 9).
        if (decor_->WasDrinkCollected()) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel42);
            if (lives_ < 9) {
                ++lives_;
                gameData_.SetNbVies(lives_);
                gameData_.Write(savePath_);
            }
        }

        // Bonus life when all treasures collected (once per level, capped at 9).
        int total = decor_->GetTotalTreasures();
        if (!bonusLifeAwarded_ && total > 0 && collected >= total) {
            bonusLifeAwarded_ = true;
            if (lives_ < 9) {
                ++lives_;
                gameData_.SetNbVies(lives_);
                gameData_.Write(savePath_);
                if (sound_) sound_->Play(SoundChannel::SoundChannel42);
            }
        }

        // Apply platform carry after all object updates.
        Vector3 carry = decor_->GetPlatformDelta();
        if ((carry.x_ != 0.0f || carry.z_ != 0.0f) && blupi_)
            blupi_->ApplyExternalDelta(carry);

        if (decor_->WasExitReached()) {
            if (sound_) sound_->Play(SoundChannel::SoundChannel57);
            ++currentWorld_;
            gameData_.SetNbVies(lives_);
            gameData_.SetLastWorld(currentWorld_);
            gameData_.Write(savePath_);
            EnterPhase(GamePhase::Win);
            return;
        }
        if (decor_->WasBlupiHit() && shieldTimer_ <= 0.0f && respawnInvincibleTimer_ <= 0.0f) {
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
            if (blupi_) { blupi_->Respawn(); blupi_->StartFlash(2.0f); }
            respawnInvincibleTimer_ = 2.0f;
        }
        decor_->ClearEvents();
    }

    hud_->ShowPlay(lives_,
                   decor_ ? decor_->GetCollected() : 0,
                   decor_ ? decor_->GetTotalTreasures() : 0,
                   keysRed_, keysGreen_, keysBlue_,
                   shieldTimer_, currentWorld_,
                   controlsHintTimer_ > 0.0f);
}

void GalaxyEggbertGame::AdvanceToNextWorld() {
    if (currentWorld_ > kMaxWorld) {
        currentWorld_ = 1;
        gameData_.SetLastWorld(1);
        gameData_.Write(savePath_);
    }
    prevCollected_          = 0;
    keysRed_ = keysGreen_ = keysBlue_ = 0;
    shieldTimer_            = 0.0f;
    respawnInvincibleTimer_ = 0.0f;
    controlsHintTimer_      = 8.0f;
    bonusLifeAwarded_       = false;
    decor_.reset();
    blupi_.reset();
    LoadWorld(currentWorld_);
    EnterPhase(GamePhase::Play);
}

void GalaxyEggbertGame::UpdateWin(float dt) {
    (void)dt;
    int completedWorld = currentWorld_ - 1;
    if (completedWorld < 1) completedWorld = 1;
    if (completedWorld > 5) completedWorld = 5;
    const char* wname = WorldName(completedWorld);
    int collected = decor_ ? decor_->GetCollected()      : 0;
    int total     = decor_ ? decor_->GetTotalTreasures() : 0;
    const char* header = (completedWorld >= kMaxWorld)
                         ? "ALL WORLDS COMPLETE!" : "LEVEL COMPLETE!";
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "%s\n\nWorld %d: %s\nTreasures: %d/%d  |  Lives: %d\n\nPress any key...",
        header, completedWorld, wname, collected, total, lives_);
    phases_->SetOverlayText(buf);

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
    const char* wname = WorldName(currentWorld_);
    char buf[256];
    std::snprintf(buf, sizeof(buf),
        "GAME OVER\n\nFell on World %d: %s\n\nPress any key to restart...",
        currentWorld_, wname);
    phases_->SetOverlayText(buf);

    auto* input = context_->GetSubsystem<Input>();
    const Key keys[] = { KEY_SPACE, KEY_RETURN, KEY_ESCAPE,
                         KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT };
    for (Key k : keys) {
        if (input->GetKeyPress(k)) { ResetLevel(); return; }
    }
    if (input->GetMouseButtonPress(MOUSEB_LEFT)) ResetLevel();
}

void GalaxyEggbertGame::ResetLevel() {
    lives_                  = 3;
    currentWorld_           = 1;
    prevCollected_          = 0;
    keysRed_ = keysGreen_ = keysBlue_ = 0;
    shieldTimer_            = 0.0f;
    respawnInvincibleTimer_ = 0.0f;
    controlsHintTimer_      = 8.0f;
    bonusLifeAwarded_       = false;
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

    const char* wname = WorldName(currentWorld_);
    int pauseCollected = decor_ ? decor_->GetCollected()      : 0;
    int pauseTotal     = decor_ ? decor_->GetTotalTreasures() : 0;
    char keyLine[64] = "";
    if (keysRed_ + keysGreen_ + keysBlue_ > 0)
        std::snprintf(keyLine, sizeof(keyLine), "\nKeys:%s%s%s",
            keysRed_ > 0 ? " Red"   : "",
            keysGreen_ > 0 ? " Green" : "",
            keysBlue_ > 0 ? " Blue"  : "");
    char buf[320];
    std::snprintf(buf, sizeof(buf),
        "PAUSED\n\nWorld %d: %s\nLives: %d   Treasures: %d/%d%s\n\nESC: resume   S: settings",
        currentWorld_, wname, lives_, pauseCollected, pauseTotal, keyLine);
    phases_->SetOverlayText(buf);

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
    camera_->SetCollisionWorld(world_.get(), kWCX, kWCZ);
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
    if (auto* input = context_->GetSubsystem<Input>()) input->SetMouseVisible(true);
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

    if (input->GetKeyPress(KEY_F12)) drawDebug_ = !drawDebug_;

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
