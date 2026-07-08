#include "GETerrainRenderer.hpp"
#include "GEDirectionalCubeTiles.hpp"
#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <Easy3D/CubeBatch.hpp>
#include <Easy3D/CubeMesh.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Matches GEWorldRuntime::kWorldCenterX/kWorldCenterZ (== Simple3D's
        // GEWorldRuntime::kWCX/kWCZ) — centers the 100x100 grid on the origin,
        // one world unit per tile.
        constexpr int kWorldCenterX = GEWorldRuntime::kWorldCenterX;
        constexpr int kWorldCenterZ = GEWorldRuntime::kWorldCenterZ;

        // Animation frame tables — ported 1:1 from GalaxyEggbertSimple3D's
        // already-shipped GETerrainRenderer.cpp (same galaxy-eggbert repo;
        // originally sourced from mobile-eggbert Tables.cpp with approval).
        constexpr int kAnimLava[8]     = {68, 69, 70, 71, 72, 71, 70, 69};
        constexpr int kAnimSpike[16]   = {374,374,373,347,373,374,374,374,373,347,347,373,374,374,374,374};
        constexpr int kAnimCrusher[10] = {317,317,318,319,320,321,322,323,323,323};
        constexpr int kAnimSaw[6]      = {378,379,380,381,382,383};
        constexpr int kAnimWater1[6]   = {92,93,94,95,94,93};
        constexpr int kAnimWater2[6]   = {91,96,97,98,97,96};
        constexpr int kAnimTemp[20]    = {328,328,327,327,326,326,325,325,324,324,
                                           325,325,326,326,327,329,328,328,-1,-1};
        constexpr int kAnimMarine[11]  = {203,204,205,206,207,208,207,206,205,204,203};

        int AnimIcon(std::uint16_t base, int phase)
        {
            using namespace GalaxyEggbert::BlockTypes;
            switch (base)
            {
                case Lava:     return kAnimLava[phase % 8];
                case Spike:    return kAnimSpike[phase % 16];
                case Crusher:  return kAnimCrusher[phase % 10];
                case Saw:      return kAnimSaw[phase % 6];
                case Water1:   return kAnimWater1[phase % 6];
                case Water2:   return kAnimWater2[phase % 6];
                case Temp:     return kAnimTemp[phase % 20];
                case FanLeft:  return 126 + (phase % 3);
                case FanRight: return 129 + (phase % 3);
                case FanUp:    return 132 + (phase % 3);
                case FanDown:  return 135 + (phase % 3);
                case Marine:   return kAnimMarine[phase % 11];
                default:       return static_cast<int>(base);
            }
        }

        bool IsAnimated(std::uint16_t base)
        {
            using namespace GalaxyEggbert::BlockTypes;
            return base == Lava    || base == Spike    || base == Crusher || base == Saw ||
                   base == Water1  || base == Water2   || base == Temp    ||
                   base == FanLeft || base == FanRight  || base == FanUp   || base == FanDown ||
                   base == Marine;
        }
    }

    GETerrainRenderer::GETerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Worlds::World& world,
                                         const GETileAtlas& tileAtlas)
        : m_tileAtlas(&tileAtlas)
    {
        Easy3D::CubeBatch staticBatch;
        std::vector<Easy3D::DirectionalCubeItem> directionalItems;
        double sumX = 0.0;
        double sumY = 0.0;
        double sumZ = 0.0;
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int y = 0; y < blocksPerAxis; ++y)
        {
            for (int z = 0; z < blocksPerAxis; ++z)
            {
                for (int x = 0; x < blocksPerAxis; ++x)
                {
                    const auto block = world.getBlock(static_cast<std::uint16_t>(x),
                                                       static_cast<std::uint16_t>(y),
                                                       static_cast<std::uint16_t>(z));
                    if (block.isAir())
                    {
                        continue;
                    }

                    const float worldX = static_cast<float>(x - kWorldCenterX);
                    const float worldY = static_cast<float>(y);
                    const float worldZ = static_cast<float>(z - kWorldCenterZ);
                    ++m_blockCount;
                    sumX += worldX;
                    sumY += worldY;
                    sumZ += worldZ;

                    const std::uint16_t animBase = GalaxyEggbert::BlockTypes::tileAnimBase(block.type());
                    if (IsAnimated(animBase))
                    {
                        m_animBlocks.push_back({worldX, worldY, worldZ, animBase});
                        continue;
                    }

                    const Easy3D::CubeBatch::Vector3 center(worldX, worldY, worldZ);
                    const Easy3D::CubeBatch::Vector3 size(1.0f, 1.0f, 1.0f);
                    const auto tileUv = tileAtlas.GetTileUv(static_cast<int>(block.type()));

                    Easy3D::DirectionalCubeFace directionalFaces[6];
                    if (TryGetDirectionalCubeFaces(static_cast<int>(block.type()), tileUv, directionalFaces))
                    {
                        Easy3D::DirectionalCubeItem item;
                        item.Center = center;
                        item.Size = size;
                        for (int face = 0; face < 6; ++face)
                        {
                            item.Faces[face] = directionalFaces[face];
                        }
                        directionalItems.push_back(item);
                        continue;
                    }

                    staticBatch.Add(center, size, tileUv);
                }
            }
        }

        if (m_blockCount > 0)
        {
            m_centroidX = static_cast<float>(sumX / m_blockCount);
            m_centroidY = static_cast<float>(sumY / m_blockCount);
            m_centroidZ = static_cast<float>(sumZ / m_blockCount);
        }

        std::vector<Easy3D::CubeVertex> staticVertices;
        std::vector<std::uint32_t> staticIndices;
        Easy3D::BuildCubeMesh(staticBatch, staticVertices, staticIndices);
        for (const auto& item : directionalItems)
        {
            // Appended after the uniform-cube batch, not merged into it:
            // AppendDirectionalCubeMesh/AppendCubeMesh both offset indices by
            // the vertex count already present, so concatenation order
            // doesn't matter for correctness -- this just keeps the two
            // code paths visually separate here.
            Easy3D::AppendDirectionalCubeMesh(item, staticVertices, staticIndices);
        }
        m_staticRenderer = std::make_unique<Easy3D::CubeMeshRenderer>(device, staticVertices, staticIndices);

        Update(device, 0);
    }

    void GETerrainRenderer::Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase)
    {
        if (animPhase == m_lastAnimPhase || m_animBlocks.empty())
        {
            m_lastAnimPhase = animPhase;
            return;
        }
        m_lastAnimPhase = animPhase;

        Easy3D::CubeBatch animBatch;
        std::vector<Easy3D::DirectionalCubeItem> animDirectionalItems;
        for (const auto& block : m_animBlocks)
        {
            const int icon = AnimIcon(block.base, animPhase);
            if (icon < 0)
            {
                // Temp tile's invisible frames — omit the cube entirely.
                continue;
            }
            const Easy3D::CubeBatch::Vector3 center(block.x, block.y, block.z);
            const Easy3D::CubeBatch::Vector3 size(1.0f, 1.0f, 1.0f);
            const auto tileUv = m_tileAtlas->GetTileUv(icon);

            // The 4 fan tiles are both animated (blade spin) AND, per
            // GEDirectionalCubeTiles.cpp, DirectionalCube (base + open face)
            // -- check the table here too, or fan blocks would always fall
            // through to a plain untextured-on-every-face UniformCube and
            // the table entry would never actually be used (found
            // 2026-07-08 via a live block/vertex-count mismatch on the fan
            // demo block in worlds3d/world001.vwr). Looked up by block.base
            // (the animation group's base icon, e.g. FanLeft=126), NOT by
            // icon (the current frame, e.g. 126/127/128) -- the table only
            // has an entry for the base icon, and the face
            // pattern/visibility must stay constant across the animation;
            // only tileUv (the actually-sampled texture) should change
            // frame to frame.
            Easy3D::DirectionalCubeFace directionalFaces[6];
            if (TryGetDirectionalCubeFaces(static_cast<int>(block.base), tileUv, directionalFaces))
            {
                Easy3D::DirectionalCubeItem item;
                item.Center = center;
                item.Size = size;
                for (int face = 0; face < 6; ++face)
                {
                    item.Faces[face] = directionalFaces[face];
                }
                animDirectionalItems.push_back(item);
                continue;
            }

            animBatch.Add(center, size, tileUv);
        }

        if (animBatch.Empty() && animDirectionalItems.empty())
        {
            // All animated tiles are in a hidden frame this phase (e.g. Temp's
            // 2 blank frames out of 20) — avoid constructing a zero-size GPU
            // buffer; just draw nothing until the next non-empty phase.
            m_animRenderer.reset();
            return;
        }

        std::vector<Easy3D::CubeVertex> animVertices;
        std::vector<std::uint32_t> animIndices;
        Easy3D::BuildCubeMesh(animBatch, animVertices, animIndices);
        for (const auto& item : animDirectionalItems)
        {
            Easy3D::AppendDirectionalCubeMesh(item, animVertices, animIndices);
        }
        m_animRenderer = std::make_unique<Easy3D::CubeMeshRenderer>(device, animVertices, animIndices);
    }

    void GETerrainRenderer::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                 Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const
    {
        if (m_staticRenderer)
        {
            m_staticRenderer->Draw(device, effect);
        }
        if (m_animRenderer)
        {
            m_animRenderer->Draw(device, effect);
        }
    }

    int GETerrainRenderer::VertexCount() const noexcept
    {
        return (m_staticRenderer ? m_staticRenderer->VertexCount() : 0) +
               (m_animRenderer ? m_animRenderer->VertexCount() : 0);
    }

    int GETerrainRenderer::PrimitiveCount() const noexcept
    {
        return (m_staticRenderer ? m_staticRenderer->PrimitiveCount() : 0) +
               (m_animRenderer ? m_animRenderer->PrimitiveCount() : 0);
    }
}
