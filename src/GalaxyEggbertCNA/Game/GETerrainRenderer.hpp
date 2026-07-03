#pragma once

#include "GETileAtlas.hpp"

#include <GalaxyEggbert/Worlds/World.hpp>

#include <Easy3D/CubeMeshRenderer.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Builds one static Easy3D::CubeMeshRenderer covering every non-animated
    // non-air cell of a loaded World, plus a second CubeMeshRenderer for the
    // animated subset (lava/crusher/saw/spike/water/fan/marine/temp) that is
    // rebuilt whenever Update() is given a new animation phase (plan.md
    // E3D-MIG-054/055). One 1x1x1 cube per block, textured via GETileAtlas.
    class GETerrainRenderer
    {
    public:
        GETerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          const Worlds::World& world,
                          const GETileAtlas& tileAtlas);

        // Rebuilds the animated-tile mesh for animPhase (no-op if unchanged
        // since the last call). Call once per frame with
        // GEWorldRuntime::GetAnimPhase(). No effect on the static mesh.
        void Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase);

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const;

        [[nodiscard]] int BlockCount() const noexcept { return m_blockCount; }
        [[nodiscard]] int AnimatedBlockCount() const noexcept { return static_cast<int>(m_animBlocks.size()); }
        [[nodiscard]] int VertexCount() const noexcept;
        [[nodiscard]] int PrimitiveCount() const noexcept;

        // World-space centroid of every non-air block across all Y layers —
        // a reliable camera look-at target, since the spawn tile itself may
        // be an open-air cell Blupi stands in rather than a solid block.
        [[nodiscard]] float CentroidX() const noexcept { return m_centroidX; }
        [[nodiscard]] float CentroidY() const noexcept { return m_centroidY; }
        [[nodiscard]] float CentroidZ() const noexcept { return m_centroidZ; }

    private:
        struct AnimBlock
        {
            float x;
            float y;
            float z;
            std::uint16_t base;
        };

        std::unique_ptr<Easy3D::CubeMeshRenderer> m_staticRenderer;
        std::unique_ptr<Easy3D::CubeMeshRenderer> m_animRenderer;
        std::vector<AnimBlock> m_animBlocks;
        const GETileAtlas* m_tileAtlas = nullptr;
        int m_blockCount = 0;
        float m_centroidX = 0.0f;
        float m_centroidY = 0.0f;
        float m_centroidZ = 0.0f;
        int m_lastAnimPhase = -1;
    };
}
