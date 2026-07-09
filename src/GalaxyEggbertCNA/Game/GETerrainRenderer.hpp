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
    // opaque non-air cell of a loaded World, a second static renderer for
    // the small set of non-animated blocks that ALSO need alpha blending
    // (icons 30/31 -- see Draw()'s comment), a third for the icon-107
    // grass-top overlay (its own texture, drawn via DrawGrass() with a
    // separate effect), plus a fourth CubeMeshRenderer for the animated,
    // alpha-blended subset (lava/crusher/saw/spike/fan/marine/temp -- drawn
    // opaque before 2026-07-09, fixed once object-m.png pixel sampling
    // showed these tiles are 44-81% transparent, not near-opaque like icons
    // 30/31) and a fifth for the water subset (Water1/Water2 -- animated AND
    // alpha-blended), the latter two rebuilt whenever Update() is given a
    // new animation phase (plan.md E3D-MIG-054/055). One 1x1x1 cube per
    // block, textured via GETileAtlas.
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

        // Draws the grass-top overlay (icon 107 blocks' real grass texture,
        // NEXT.md §8 task 3) with a SEPARATE effect/texture bind from the
        // main object-m.png-based Draw() -- BasicEffect only binds one
        // texture at a time, and grass_top.png is a genuinely separate,
        // galaxy-eggbert-owned asset, not part of the object-m.png atlas.
        // Caller (GalaxyEggbertCnaGame) owns @p grassEffect and must bind
        // its own grass texture + View/Projection/World before calling.
        // No-op if no icon-107 blocks exist in this World.
        void DrawGrass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                       Microsoft::Xna::Framework::Graphics::BasicEffect& grassEffect) const;

        [[nodiscard]] int BlockCount() const noexcept { return m_blockCount; }
        [[nodiscard]] int AnimatedBlockCount() const noexcept { return static_cast<int>(m_animBlocks.size()); }
        [[nodiscard]] int WaterBlockCount() const noexcept { return static_cast<int>(m_waterBlocks.size()); }
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

        // Shared rebuild logic for m_animRenderer/m_waterRenderer: computes
        // each block's current-frame icon via AnimIcon(), applies
        // AppendSpecialGeometry() where applicable, and uploads a fresh
        // CubeMeshRenderer. Returns nullptr if every block resolved to a
        // hidden frame (e.g. Temp's blank frames) — caller should just clear
        // its renderer pointer in that case rather than upload an empty mesh.
        [[nodiscard]] std::unique_ptr<Easy3D::CubeMeshRenderer> RebuildAnimatedRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            const std::vector<AnimBlock>& blocks, int animPhase) const;

        std::unique_ptr<Easy3D::CubeMeshRenderer> m_staticRenderer;
        std::unique_ptr<Easy3D::CubeMeshRenderer> m_transparentStaticRenderer;
        std::unique_ptr<Easy3D::CubeMeshRenderer> m_animRenderer;
        std::unique_ptr<Easy3D::CubeMeshRenderer> m_waterRenderer;
        std::unique_ptr<Easy3D::CubeMeshRenderer> m_grassRenderer;
        std::vector<AnimBlock> m_animBlocks;
        std::vector<AnimBlock> m_waterBlocks;
        const GETileAtlas* m_tileAtlas = nullptr;
        int m_blockCount = 0;
        float m_centroidX = 0.0f;
        float m_centroidY = 0.0f;
        float m_centroidZ = 0.0f;
        int m_lastAnimPhase = -1;
    };
}
