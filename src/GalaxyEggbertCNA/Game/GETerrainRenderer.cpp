#include "GETerrainRenderer.hpp"
#include "GEWorldRuntime.hpp"

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
    }

    GETerrainRenderer::GETerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Worlds::World& world,
                                         const GETileAtlas& tileAtlas)
    {
        Easy3D::CubeBatch batch;
        double sumX = 0.0;
        double sumZ = 0.0;
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        for (int z = 0; z < blocksPerAxis; ++z)
        {
            for (int x = 0; x < blocksPerAxis; ++x)
            {
                const auto block = world.getBlock(static_cast<std::uint16_t>(x), 0,
                                                   static_cast<std::uint16_t>(z));
                if (block.isAir())
                {
                    continue;
                }

                const float worldX = static_cast<float>(x - kWorldCenterX);
                const float worldZ = static_cast<float>(z - kWorldCenterZ);
                const Easy3D::CubeBatch::Vector3 center(worldX, 0.0f, worldZ);
                const Easy3D::CubeBatch::Vector3 size(1.0f, 1.0f, 1.0f);
                batch.Add(center, size, tileAtlas.GetTileUv(static_cast<int>(block.type())));
                ++m_blockCount;
                sumX += worldX;
                sumZ += worldZ;
            }
        }

        if (m_blockCount > 0)
        {
            m_centroidX = static_cast<float>(sumX / m_blockCount);
            m_centroidZ = static_cast<float>(sumZ / m_blockCount);
        }

        std::vector<Easy3D::CubeVertex> vertices;
        std::vector<std::uint32_t> indices;
        Easy3D::BuildCubeMesh(batch, vertices, indices);

        m_renderer = std::make_unique<Easy3D::CubeMeshRenderer>(device, vertices, indices);
    }

    void GETerrainRenderer::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                 Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const
    {
        if (m_renderer)
        {
            m_renderer->Draw(device, effect);
        }
    }
}
