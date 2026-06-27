#include "GETerrainRenderer.hpp"
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <algorithm>

using namespace Simple3D;
using namespace GalaxyEggbert;
using namespace GalaxyEggbert::Worlds;

namespace GESimple3D {

static constexpr int kWCX = GEWorldRuntime::kWCX;
static constexpr int kWCZ = GEWorldRuntime::kWCZ;
static constexpr int kFillDepth = 3;
static const int kNbDir[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};

// Animation frame tables from mobile-eggbert Tables.cpp
static const int kAnimLava[8]     = {68, 69, 70, 71, 72, 71, 70, 69};
static const int kAnimSpike[16]   = {374,374,373,347,373,374,374,374,373,347,347,373,374,374,374,374};
static const int kAnimCrusher[10] = {317,317,318,319,320,321,322,323,323,323};
static const int kAnimSaw[6]      = {378,379,380,381,382,383};
static const int kAnimWater1[6]   = {92,93,94,95,94,93};
static const int kAnimWater2[6]   = {91,96,97,98,97,96};
// table_decor_temp (20 frames): tile oscillates and vanishes for frames 18-19 (-1 = hide).
static const int kAnimTemp[20]    = {328,328,327,327,326,326,325,325,324,324,
                                      325,325,326,326,327,329,328,328,-1,-1};
// table_marine (11 frames) from mobile-eggbert Tables.cpp.
static const int kAnimMarine[11]  = {203,204,205,206,207,208,207,206,205,204,203};

static int animIcon(uint16_t base, int phase) {
    switch (base) {
        case BlockTypes::Lava:    return kAnimLava[phase % 8];
        case BlockTypes::Spike:   return kAnimSpike[phase % 16];
        case BlockTypes::Crusher: return kAnimCrusher[phase % 10];
        case BlockTypes::Saw:     return kAnimSaw[phase % 6];
        case BlockTypes::Water1:  return kAnimWater1[phase % 6];
        case BlockTypes::Water2:  return kAnimWater2[phase % 6];
        case BlockTypes::Temp:    return kAnimTemp[phase % 20];
        case BlockTypes::FanLeft: return 126 + (phase % 3);
        case BlockTypes::FanRight:return 129 + (phase % 3);
        case BlockTypes::FanUp:   return 132 + (phase % 3);
        case BlockTypes::FanDown:  return 135 + (phase % 3);
        case BlockTypes::Marine:   return kAnimMarine[phase % 11];
        default: return static_cast<int>(base);
    }
}

static bool isAnimated(uint16_t base) {
    return base == BlockTypes::Lava    || base == BlockTypes::Spike    ||
           base == BlockTypes::Crusher || base == BlockTypes::Saw      ||
           base == BlockTypes::Water1  || base == BlockTypes::Water2   ||
           base == BlockTypes::Temp    ||
           base == BlockTypes::FanLeft || base == BlockTypes::FanRight ||
           base == BlockTypes::FanUp   || base == BlockTypes::FanDown ||
           base == BlockTypes::Marine;
}

static constexpr float kDepthBiasConst = -0.0001f;
static constexpr float kDepthBiasSlope = -0.5f;

void GETerrainRenderer::Build(Game& game, const GEWorldRuntime& worldRuntime) {
    Clear(game);

    const World* world = worldRuntime.GetWorld();
    if (!world) return;

    const uint8_t cpa = world->chunksPerAxis();
    int blockIndex = 0;

    for (uint8_t ccy = 0; ccy < cpa; ++ccy) {
        for (uint8_t ccz = 0; ccz < cpa; ++ccz) {
            for (uint8_t ccx = 0; ccx < cpa; ++ccx) {
                if (world->chunk(ccx, ccy, ccz).isEmpty()) continue;
                for (uint8_t ly = 0; ly < 10; ++ly) {
                    for (uint8_t lz = 0; lz < 10; ++lz) {
                        for (uint8_t lx = 0; lx < 10; ++lx) {
                            const uint16_t wx = ccx * 10 + lx;
                            const uint16_t wy = ccy * 10 + ly;
                            const uint16_t wz = ccz * 10 + lz;
                            const Block b = world->getBlock(wx, wy, wz);
                            if (b.isAir()) continue;

                            const float fx = static_cast<float>(wx) - kWCX;
                            const float fy = static_cast<float>(wy);
                            const float fz = static_cast<float>(wz) - kWCZ;

                            auto* e = game.CreateEntity("Block" + std::to_string(blockIndex++));
                            e->AddModel("Models/Box.mdl");
                            e->SetMaterialColor(Color(0.28f, 0.22f, 0.18f));
                            e->SetPosition(fx, fy, fz);
                            e->AddRigidBody(0.0f);
                            e->AddBoxCollider(Vector3(1.0f, 1.0f, 1.0f));
                            e->SetCollisionLayer(CollisionLayer::StaticGeometry);
                            terrainEntities_.push_back(e);

                            auto* top = game.CreateEntity("Block" + std::to_string(blockIndex++) + "_top");
                            top->AddModel("Models/Plane.mdl");
                            {
                                float uOff, vOff, uS, vS;
                                BlockTypes::tileUV(b.type(), uOff, vOff, uS, vS);
                                top->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
                            }
                            top->SetDepthBias(kDepthBiasConst, kDepthBiasSlope);
                            top->SetPosition(fx, fy + 0.5f, fz);
                            terrainEntities_.push_back(top);

                            if (wy == 0)
                                tileEntityMap_[{wx, wz}] = top;

                            uint16_t animBase = BlockTypes::tileAnimBase(b.type());
                            if (isAnimated(animBase))
                                animTiles_.push_back({top, e, animBase, true});

                            if (BlockTypes::isDoor(b.type()))
                                doorEntities_[{wx, wz}] = {e, top};

                            // Edge-fill: extrude dark blocks downward at cliff edges
                            int iwx = static_cast<int>(wx);
                            int iwz = static_cast<int>(wz);
                            bool isEdge = false;
                            for (auto& nb : kNbDir) {
                                int nx = iwx + nb[0], nz = iwz + nb[1];
                                if (nx < 0 || nx >= 100 || nz < 0 || nz >= 100) {
                                    isEdge = true; break;
                                }
                                if (world->getBlock(
                                        static_cast<uint16_t>(nx), wy,
                                        static_cast<uint16_t>(nz)).isAir()) {
                                    isEdge = true; break;
                                }
                            }
                            if (isEdge) {
                                for (int dy = 1; dy <= kFillDepth; ++dy) {
                                    auto* fe = game.CreateEntity("BlockFill" + std::to_string(blockIndex++));
                                    fe->AddModel("Models/Box.mdl");
                                    fe->SetMaterialColor(Color(0.22f, 0.19f, 0.17f));
                                    fe->SetPosition(fx, fy - static_cast<float>(dy), fz);
                                    fe->AddRigidBody(0.0f);
                                    fe->AddBoxCollider(Vector3(1.0f, 1.0f, 1.0f));
                                    fe->SetCollisionLayer(CollisionLayer::StaticGeometry);
                                    terrainEntities_.push_back(fe);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void GETerrainRenderer::Clear(Game& game) {
    for (auto* e : terrainEntities_)
        game.DestroyEntity(e);
    terrainEntities_.clear();
    animTiles_.clear();
    doorEntities_.clear();
    tileEntityMap_.clear();
    lastAnimPhase_ = -1;
}

bool GETerrainRenderer::OpenDoor(Game& game, int wx, int wz,
                                  GalaxyEggbert::Worlds::World* world) {
    auto it = doorEntities_.find({wx, wz});
    if (it == doorEntities_.end()) return false;
    Entity* box = it->second.box;
    Entity* top = it->second.top;
    terrainEntities_.erase(
        std::remove(terrainEntities_.begin(), terrainEntities_.end(), box),
        terrainEntities_.end());
    terrainEntities_.erase(
        std::remove(terrainEntities_.begin(), terrainEntities_.end(), top),
        terrainEntities_.end());
    doorEntities_.erase(it);
    game.DestroyEntity(box);
    game.DestroyEntity(top);
    if (world) world->setBlock(
        static_cast<uint16_t>(wx), 0, static_cast<uint16_t>(wz),
        GalaxyEggbert::Worlds::Block::make(BlockTypes::Air));
    return true;
}

void GETerrainRenderer::Update(int animPhase) {
    if (animPhase == lastAnimPhase_) return;
    lastAnimPhase_ = animPhase;
    for (auto& at : animTiles_) {
        if (!at.active) continue;
        int icon = animIcon(at.base, animPhase);
        if (at.base == BlockTypes::Temp) {
            at.parent->SetActive(icon >= 0);
            at.entity->SetActive(icon >= 0);
            if (icon < 0) continue;
        }
        float uOff, vOff, uS, vS;
        BlockTypes::tileUV(icon, uOff, vOff, uS, vS);
        at.entity->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
        at.entity->SetDepthBias(kDepthBiasConst, kDepthBiasSlope);
    }
}

bool GETerrainRenderer::ToggleSwitch(int wx, int wz,
                                      GalaxyEggbert::Worlds::World* world) {
    using namespace GalaxyEggbert;
    auto it = tileEntityMap_.find({wx, wz});
    if (it == tileEntityMap_.end()) return false;

    uint16_t cur = world->getBlock(
        static_cast<uint16_t>(wx), 0, static_cast<uint16_t>(wz)).type();
    if (!BlockTypes::isSwitch(cur)) return false;

    // Flip switch icon (384↔385) and determine whether we are activating saws.
    bool activate = (cur == BlockTypes::SwitchOff);  // true → saws start spinning
    uint16_t newSwitch = activate ? BlockTypes::Switch : BlockTypes::SwitchOff;
    world->setBlock(static_cast<uint16_t>(wx), 0, static_cast<uint16_t>(wz),
                    GalaxyEggbert::Worlds::Block::make(newSwitch));
    float uOff, vOff, uS, vS;
    BlockTypes::tileUV(newSwitch, uOff, vOff, uS, vS);
    it->second->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
    it->second->SetDepthBias(kDepthBiasConst, kDepthBiasSlope);

    // Scan ±20 tiles in X at same Z for linked saw tiles, toggle 378↔379.
    uint16_t fromSaw = activate ? BlockTypes::SawStopped : BlockTypes::Saw;
    uint16_t toSaw   = activate ? BlockTypes::Saw : BlockTypes::SawStopped;
    for (int sx = wx - 20; sx <= wx + 20; ++sx) {
        if (sx < 0 || sx >= 100) continue;
        uint16_t sawTile = world->getBlock(
            static_cast<uint16_t>(sx), 0, static_cast<uint16_t>(wz)).type();
        if (sawTile != fromSaw) continue;
        world->setBlock(static_cast<uint16_t>(sx), 0, static_cast<uint16_t>(wz),
                        GalaxyEggbert::Worlds::Block::make(toSaw));
        auto sit = tileEntityMap_.find({sx, wz});
        if (sit == tileEntityMap_.end()) continue;
        // Toggle animTile active flag for this entity
        for (auto& at : animTiles_) {
            if (at.entity != sit->second) continue;
            at.active = activate;
            if (!activate) {
                // Stopped: set UV to static SawStopped icon
                BlockTypes::tileUV(BlockTypes::SawStopped, uOff, vOff, uS, vS);
                sit->second->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
                sit->second->SetDepthBias(kDepthBiasConst, kDepthBiasSlope);
            }
            break;
        }
        if (activate) {
            // Reactivating: set UV to Saw base icon immediately
            BlockTypes::tileUV(BlockTypes::Saw, uOff, vOff, uS, vS);
            sit->second->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
            sit->second->SetDepthBias(kDepthBiasConst, kDepthBiasSlope);
        }
    }
    return true;
}

bool GETerrainRenderer::SetTileIcon(int wx, int wz, int icon) {
    auto it = tileEntityMap_.find({wx, wz});
    if (it == tileEntityMap_.end()) return false;
    if (icon < 0) {
        it->second->SetActive(false);
    } else {
        it->second->SetActive(true);
        float uOff, vOff, uS, vS;
        BlockTypes::tileUV(icon, uOff, vOff, uS, vS);
        it->second->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
        it->second->SetDepthBias(kDepthBiasConst, kDepthBiasSlope);
    }
    return true;
}

} // namespace GESimple3D
