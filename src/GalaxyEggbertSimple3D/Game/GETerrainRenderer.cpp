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
}

} // namespace GESimple3D
