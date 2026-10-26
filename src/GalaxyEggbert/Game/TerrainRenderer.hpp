#pragma once

#include "TileAtlas.hpp"
#include "TileCoverage.hpp"

#include <GalaxyEggbert/Worlds/World.hpp>

#include <GalaxyEggbert/Game/MeshParts.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp>

#include <cstdint>
#include <memory>
#include <unordered_set>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Builds one static GpuMesh covering every non-animated
    // opaque non-air cell of a loaded World, a second static renderer for
    // the small set of non-animated blocks that ALSO need alpha blending
    // (icons 30/31 -- see Draw()'s comment), a third for the icon-107
    // grass-top overlay (its own texture, drawn via DrawGrass() with a
    // separate effect), plus a fourth GpuMesh for the animated,
    // alpha-blended subset (lava/crusher/saw/spike/fan/marine/temp -- drawn
    // opaque before 2026-07-09, fixed once object-m.png pixel sampling
    // showed these tiles are 44-81% transparent, not near-opaque like icons
    // 30/31) and a fifth for the water subset (Water1/Water2 -- animated AND
    // alpha-blended), the latter two rebuilt whenever Update() is given a
    // new animation phase (plan.md E3D-MIG-054/055). One 1x1x1 cube per
    // block, textured via TileAtlas. Since 2026-10-26 the static blocks whose tile is a
    // hard-edged sprite (TileCoverage::Cutout: doors, pillars, girders, flat-plate decor) are a
    // sixth mesh drawn with an alpha test, and the faint ones (TileCoverage::Translucent:
    // cobwebs) join the alpha-blended static mesh.
    class TerrainRenderer
    {
    public:
        TerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          const Worlds::World& world,
                          const TileAtlas& tileAtlas,
                          const std::vector<TileCoverage>& tileCoverage = {});

        // Rebuilds the animated-tile mesh for animPhase (no-op if unchanged
        // since the last call). Call once per frame with
        // WorldRuntime::GetAnimPhase(). No effect on the static mesh.
        void Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase);

        // The cutout pass takes @p effect's World/View/Projection and texture.
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const;

        // Draws the grass-top overlay (icon 107 blocks' real grass texture,
        // NEXT.md §8 task 3) with a SEPARATE effect/texture bind from the
        // main object-m.png-based Draw() -- BasicEffect only binds one
        // texture at a time, and grass_top.png is a genuinely separate,
        // galaxy-eggbert-owned asset, not part of the object-m.png atlas.
        // Caller (GalaxyEggbertGame) owns @p grassEffect and must bind
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
        // mesh (in parts when it is over a buffer's limit, MeshParts.hpp). Returns nullptr if every block resolved to a
        // hidden frame (e.g. Temp's blank frames) — caller should just clear
        // its renderer pointer in that case rather than upload an empty mesh.
        [[nodiscard]] std::unique_ptr<GpuMeshParts> RebuildAnimatedRenderer(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
            const std::vector<AnimBlock>& blocks, int animPhase) const;

        std::unique_ptr<GpuMeshParts> m_staticRenderer;
        std::unique_ptr<GpuMeshParts> m_transparentStaticRenderer;
        // Lit by vertex colour (AlphaTestEffect has no lights: BakeTerrainLighting()).
        std::unique_ptr<GpuMeshParts> m_cutoutRenderer;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::AlphaTestEffect> m_cutoutEffect;
        std::unique_ptr<GpuMeshParts> m_animRenderer;
        std::unique_ptr<GpuMeshParts> m_waterRenderer;
        std::unique_ptr<GpuMeshParts> m_grassRenderer;
        std::vector<AnimBlock> m_animBlocks;
        std::vector<AnimBlock> m_waterBlocks;
        // Positions with InnerFlatPlate rotation metadata set (plan.md
        // E3D-MIG-149, 2026-07-11 -- see InnerFlatPlateTiles.hpp's
        // kPlateRotationMetadataType comment), collected once here so
        // per-block lookups during (re)builds are O(1) instead of an
        // extra world query per block. Packed key: (x<<20)|(y<<10)|z,
        // matching PackPlatePositionKey() in the .cpp.
        std::unordered_set<std::uint32_t> m_rotatedPlatePositions;
        const TileAtlas* m_tileAtlas = nullptr;
        int m_blockCount = 0;
        float m_centroidX = 0.0f;
        float m_centroidY = 0.0f;
        float m_centroidZ = 0.0f;
        int m_lastAnimPhase = -1;
    };
}
