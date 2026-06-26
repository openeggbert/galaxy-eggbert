#include "GETerrainRenderer.hpp"
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>

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

static int animIcon(uint16_t base, int phase) {
    switch (base) {
        case BlockTypes::Lava:    return kAnimLava[phase % 8];
        case BlockTypes::Spike:   return kAnimSpike[phase % 16];
        case BlockTypes::Crusher: return kAnimCrusher[phase % 10];
        case BlockTypes::Saw:     return kAnimSaw[phase % 6];
        case BlockTypes::Water1:  return kAnimWater1[phase % 6];
        case BlockTypes::Water2:  return kAnimWater2[phase % 6];
        default: return static_cast<int>(base);
    }
}

static bool isAnimated(uint16_t base) {
    return base == BlockTypes::Lava   || base == BlockTypes::Spike   ||
           base == BlockTypes::Crusher|| base == BlockTypes::Saw     ||
           base == BlockTypes::Water1 || base == BlockTypes::Water2;
}

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
                            {
                                float uOff, vOff, uS, vS;
                                BlockTypes::tileUV(b.type(), uOff, vOff, uS, vS);
                                e->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
                            }
                            e->SetPosition(fx, fy, fz);
                            e->AddRigidBody(0.0f);
                            e->AddBoxCollider(Vector3(1.0f, 1.0f, 1.0f));
                            e->SetCollisionLayer(CollisionLayer::StaticGeometry);
                            terrainEntities_.push_back(e);

                            uint16_t animBase = BlockTypes::tileAnimBase(b.type());
                            if (isAnimated(animBase))
                                animTiles_.push_back({e, animBase});

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
    lastAnimPhase_ = -1;
}

void GETerrainRenderer::Update(int animPhase) {
    if (animPhase == lastAnimPhase_) return;
    lastAnimPhase_ = animPhase;
    for (auto& at : animTiles_) {
        int icon = animIcon(at.base, animPhase);
        float uOff, vOff, uS, vS;
        BlockTypes::tileUV(icon, uOff, vOff, uS, vS);
        at.entity->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS);
    }
}

} // namespace GESimple3D
