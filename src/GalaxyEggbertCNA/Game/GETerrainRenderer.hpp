#pragma once

#include "GETileAtlas.hpp"

#include <GalaxyEggbert/Worlds/World.hpp>

#include <Easy3D/CubeMeshRenderer.hpp>

#include <memory>

namespace GalaxyEggbert::CNA
{
    // Builds one static (non-animated) Easy3D::CubeMeshRenderer covering
    // every non-air cell of a loaded World (plan.md E3D-MIG-054). No
    // animated tiles yet (lava/crusher/saw/etc — a later phase, matching
    // Simple3D's already-shipped behavior); one 1x1x1 cube per block,
    // textured via GETileAtlas.
    class GETerrainRenderer
    {
    public:
        GETerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          const Worlds::World& world,
                          const GETileAtlas& tileAtlas);

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const;

        [[nodiscard]] int BlockCount() const noexcept { return m_blockCount; }
        [[nodiscard]] int VertexCount() const noexcept { return m_renderer ? m_renderer->VertexCount() : 0; }
        [[nodiscard]] int PrimitiveCount() const noexcept { return m_renderer ? m_renderer->PrimitiveCount() : 0; }

        // World-space X/Z centroid of every non-air block (Y is always 0 for
        // the current flat mobile-eggbert-derived worlds) — a reliable camera
        // look-at target, since the spawn tile itself may be an open-air cell
        // Blupi stands in rather than a solid block.
        [[nodiscard]] float CentroidX() const noexcept { return m_centroidX; }
        [[nodiscard]] float CentroidZ() const noexcept { return m_centroidZ; }

    private:
        std::unique_ptr<Easy3D::CubeMeshRenderer> m_renderer;
        int m_blockCount = 0;
        float m_centroidX = 0.0f;
        float m_centroidZ = 0.0f;
    };
}
