#include "GETerrainRenderer.hpp"
#include "GEDirectionalCubeTiles.hpp"
#include "GEInnerFlatPlateTiles.hpp"
#include "GEInnerPillarBoxTiles.hpp"
#include "GETripleCrossBillboardTiles.hpp"
#include "GEWorldRuntime.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <Easy3D/CubeBatch.hpp>
#include <Easy3D/CubeMesh.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Matches GEWorldRuntime::kWorldCenterX/kWorldCenterZ (== Simple3D's
        // GEWorldRuntime::kWCX/kWCZ) — centers the 100x100 grid on the origin,
        // one world unit per tile.
        constexpr int kWorldCenterX = GEWorldRuntime::kWorldCenterX;
        constexpr int kWorldCenterZ = GEWorldRuntime::kWorldCenterZ;

        // InnerFlatPlate/TripleCrossBillboard geometry sizing — "spans most
        // of the block" per the questionnaire wording, leaving a visible
        // margin so the plate/cross doesn't clip through neighboring blocks'
        // faces.
        constexpr float kInnerFlatPlateWidth = 0.75f;
        constexpr float kInnerFlatPlateHeight = 0.9f;
        constexpr float kTripleCrossWidth = 0.8f;
        constexpr float kTripleCrossHeight = 1.0f;

        // Appends whichever "new geometry" render mode (if any) @p lookupIcon
        // uses, into @p vertices/@p indices, textured with @p tileUv.
        // Returns true if it handled the icon (caller should skip its normal
        // UniformCube path); false if @p lookupIcon isn't one of these
        // modes. @p lookupIcon and @p tileUv's icon are the same for static
        // (non-animated) blocks, but deliberately different for animated
        // ones: @p lookupIcon is the animation group's fixed base icon (so
        // the render-mode/face-pattern lookup doesn't change frame to
        // frame), while @p tileUv is always the currently-sampled frame's
        // actual texture (see GETerrainRenderer::Update's fan-tile comment).
        bool AppendSpecialGeometry(int lookupIcon, const Easy3D::UvRect& tileUv,
                                   const Easy3D::CubeBatch::Vector3& center,
                                   std::vector<Easy3D::CubeVertex>& vertices,
                                   std::vector<std::uint32_t>& indices)
        {
            Easy3D::DirectionalCubeFace directionalFaces[6];
            if (TryGetDirectionalCubeFaces(lookupIcon, tileUv, directionalFaces))
            {
                Easy3D::DirectionalCubeItem item;
                item.Center = center;
                item.Size = Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f);
                for (int face = 0; face < 6; ++face)
                {
                    item.Faces[face] = directionalFaces[face];
                }
                Easy3D::AppendDirectionalCubeMesh(item, vertices, indices);
                return true;
            }

            Easy3D::DirectionalCubeFace pillarFaces[6];
            if (TryGetInnerPillarBoxFaces(lookupIcon, tileUv, pillarFaces))
            {
                Easy3D::DirectionalCubeItem item;
                item.Center = center;
                item.Size = Easy3D::CubeBatch::Vector3(kInnerPillarWidth, kInnerPillarHeight, kInnerPillarWidth);
                for (int face = 0; face < 6; ++face)
                {
                    item.Faces[face] = pillarFaces[face];
                }
                Easy3D::AppendDirectionalCubeMesh(item, vertices, indices);
                return true;
            }

            if (IsInnerFlatPlateIcon(lookupIcon))
            {
                Easy3D::PlateItem item;
                item.Center = center;
                item.Width = kInnerFlatPlateWidth;
                item.Height = kInnerFlatPlateHeight;
                item.Uv = tileUv;
                item.Axis = Easy3D::PlateAxis::Z;
                Easy3D::AppendPlateMesh(item, vertices, indices);
                return true;
            }

            if (IsTripleCrossBillboardIcon(lookupIcon))
            {
                Easy3D::TripleCrossItem item;
                item.Center = center;
                item.Width = kTripleCrossWidth;
                item.Height = kTripleCrossHeight;
                item.Uv = tileUv;
                Easy3D::AppendTripleCrossMesh(item, vertices, indices);
                return true;
            }

            return false;
        }

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

        // Water is animated too, but drawn as its own semi-transparent pass
        // (see GETerrainRenderer::Draw) -- IsAnimated() deliberately
        // EXCLUDES it so the constructor/Update() route water blocks to
        // m_waterBlocks/m_waterRenderer instead of m_animBlocks/m_animRenderer.
        bool IsWater(std::uint16_t base)
        {
            using namespace GalaxyEggbert::BlockTypes;
            return base == Water1 || base == Water2;
        }

        bool IsAnimated(std::uint16_t base)
        {
            using namespace GalaxyEggbert::BlockTypes;
            return base == Lava    || base == Spike    || base == Crusher || base == Saw ||
                   base == Temp    ||
                   base == FanLeft || base == FanRight  || base == FanUp   || base == FanDown ||
                   base == Marine;
        }

        // Icons 30/31: confirmed DirectionalCube (GEDirectionalCubeTiles.cpp)
        // whose own side-face texture has real per-pixel alpha ("textura má
        // i průhlednost") -- editor-only start-position markers for a
        // moveable object, not used in actual gameplay worlds, but still
        // need genuine alpha blending to render correctly if ever placed.
        // Not animated, so they don't go through m_animBlocks/m_waterBlocks
        // -- routed to their own static-but-transparent renderer instead.
        bool NeedsAlphaBlend(int icon)
        {
            return icon == 30 || icon == 31;
        }

        // Icon 107: confirmed DirectionalCube with its top face intentionally
        // left open in GEDirectionalCubeTiles.cpp (see that file's comment) --
        // its real top surface is this separate grass_top.png overlay
        // (NEXT.md §8 task 3), not part of object-m.png at all. Icons
        // 108/109 need the SAME grass texture plus more per-side work
        // (GEDirectionalCubeTiles.hpp) -- not included here yet.
        constexpr float kGrassPlateWidth = 1.0f;
        constexpr float kGrassPlateDepth = 1.0f;

        bool IsGrassTopIcon(int icon)
        {
            return icon == 107;
        }
    }

    GETerrainRenderer::GETerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Worlds::World& world,
                                         const GETileAtlas& tileAtlas)
        : m_tileAtlas(&tileAtlas)
    {
        Easy3D::CubeBatch staticBatch;
        std::vector<Easy3D::CubeVertex> staticVertices;
        std::vector<std::uint32_t> staticIndices;
        std::vector<Easy3D::CubeVertex> transparentStaticVertices;
        std::vector<std::uint32_t> transparentStaticIndices;
        std::vector<Easy3D::CubeVertex> grassVertices;
        std::vector<std::uint32_t> grassIndices;
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
                    if (IsWater(animBase))
                    {
                        m_waterBlocks.push_back({worldX, worldY, worldZ, animBase});
                        continue;
                    }
                    if (IsAnimated(animBase))
                    {
                        m_animBlocks.push_back({worldX, worldY, worldZ, animBase});
                        continue;
                    }

                    const Easy3D::CubeBatch::Vector3 center(worldX, worldY, worldZ);
                    const int icon = static_cast<int>(block.type());
                    const auto tileUv = tileAtlas.GetTileUv(icon);

                    if (NeedsAlphaBlend(icon))
                    {
                        // Not animated, so it skips m_waterBlocks entirely --
                        // goes straight into its own static-but-transparent
                        // buffer, built once here and never rebuilt by
                        // Update() (same lifecycle as m_staticRenderer).
                        AppendSpecialGeometry(icon, tileUv, center, transparentStaticVertices, transparentStaticIndices);
                        continue;
                    }

                    if (IsGrassTopIcon(icon))
                    {
                        // Additive, not exclusive: the block's sides/bottom
                        // still go through the normal DirectionalCube path
                        // below (which leaves the top face open on purpose,
                        // see GEDirectionalCubeTiles.cpp) -- this only adds
                        // the separate grass-textured top plate, offset to
                        // sit exactly at the block's top face (Y + 0.5).
                        Easy3D::PlateItem grassItem;
                        grassItem.Center = Easy3D::CubeBatch::Vector3(worldX, worldY + 0.5f, worldZ);
                        grassItem.Width = kGrassPlateWidth;
                        grassItem.Height = kGrassPlateDepth;
                        grassItem.Uv = Easy3D::UvRect{0.0f, 0.0f, 1.0f, 1.0f};
                        grassItem.Axis = Easy3D::PlateAxis::Y;
                        Easy3D::AppendPlateMesh(grassItem, grassVertices, grassIndices);
                    }

                    if (AppendSpecialGeometry(icon, tileUv, center, staticVertices, staticIndices))
                    {
                        continue;
                    }

                    staticBatch.Add(center, Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f), tileUv);
                }
            }
        }

        if (m_blockCount > 0)
        {
            m_centroidX = static_cast<float>(sumX / m_blockCount);
            m_centroidY = static_cast<float>(sumY / m_blockCount);
            m_centroidZ = static_cast<float>(sumZ / m_blockCount);
        }

        // Appended after the special-geometry blocks above, not merged into
        // one pass: AppendCubeMesh/AppendDirectionalCubeMesh/AppendPlateMesh/
        // AppendTripleCrossMesh all offset indices by the vertex count
        // already present, so concatenation order doesn't matter for
        // correctness.
        Easy3D::BuildCubeMesh(staticBatch, staticVertices, staticIndices);
        m_staticRenderer = std::make_unique<Easy3D::CubeMeshRenderer>(device, staticVertices, staticIndices);

        if (!transparentStaticVertices.empty())
        {
            m_transparentStaticRenderer =
                std::make_unique<Easy3D::CubeMeshRenderer>(device, transparentStaticVertices, transparentStaticIndices);
        }

        if (!grassVertices.empty())
        {
            m_grassRenderer = std::make_unique<Easy3D::CubeMeshRenderer>(device, grassVertices, grassIndices);
        }

        Update(device, 0);
    }

    std::unique_ptr<Easy3D::CubeMeshRenderer> GETerrainRenderer::RebuildAnimatedRenderer(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const std::vector<AnimBlock>& blocks, int animPhase) const
    {
        Easy3D::CubeBatch batch;
        std::vector<Easy3D::CubeVertex> vertices;
        std::vector<std::uint32_t> indices;
        for (const auto& block : blocks)
        {
            const int icon = AnimIcon(block.base, animPhase);
            if (icon < 0)
            {
                // Temp tile's invisible frames — omit the cube entirely.
                continue;
            }
            const Easy3D::CubeBatch::Vector3 center(block.x, block.y, block.z);
            const auto tileUv = m_tileAtlas->GetTileUv(icon);

            // Looked up by block.base (the animation group's fixed base
            // icon, e.g. FanLeft=126), NOT by icon (the current frame, e.g.
            // 126/127/128) -- a special-geometry table only has an entry for
            // the base icon, and the render mode/face pattern must stay
            // constant across the animation; only tileUv (the
            // actually-sampled texture) should change frame to frame. Found
            // 2026-07-08 via a live block/vertex-count mismatch on the fan
            // demo block: without this, fan blocks always fell through to a
            // plain untextured-on-every-face UniformCube and their
            // GEDirectionalCubeTiles entry was silently dead code.
            if (AppendSpecialGeometry(static_cast<int>(block.base), tileUv, center, vertices, indices))
            {
                continue;
            }

            batch.Add(center, Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f), tileUv);
        }

        if (batch.Empty() && vertices.empty())
        {
            // Every block in this group is in a hidden frame this phase
            // (e.g. Temp's 2 blank frames out of 20) — avoid constructing a
            // zero-size GPU buffer; caller draws nothing until the next
            // non-empty phase.
            return nullptr;
        }

        Easy3D::BuildCubeMesh(batch, vertices, indices);
        return std::make_unique<Easy3D::CubeMeshRenderer>(device, vertices, indices);
    }

    void GETerrainRenderer::Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase)
    {
        if (animPhase == m_lastAnimPhase || (m_animBlocks.empty() && m_waterBlocks.empty()))
        {
            m_lastAnimPhase = animPhase;
            return;
        }
        m_lastAnimPhase = animPhase;

        m_animRenderer = RebuildAnimatedRenderer(device, m_animBlocks, animPhase);
        m_waterRenderer = RebuildAnimatedRenderer(device, m_waterBlocks, animPhase);
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
        if (m_waterRenderer || m_transparentStaticRenderer)
        {
            // Semi-transparent pass (2026-07-08 design decision, see
            // NEXT.md §8): drawn last, with alpha blending and depth WRITES
            // disabled (but depth TESTING still on, via DepthRead) so
            // transparent geometry composites correctly over opaque terrain
            // already in the depth buffer without blocking whatever's drawn
            // after it. object-m.png has real per-pixel alpha (confirmed
            // 2026-07-08); NonPremultiplied matches its un-premultiplied
            // RGB. Covers both water (animated) and the static-but-
            // transparent icons 30/31 -- same blend/depth state either way,
            // so one state change covers both draws. State is restored to
            // Opaque/Default afterward so the caller's own state isn't
            // disturbed.
            using Microsoft::Xna::Framework::Graphics::BlendState;
            using Microsoft::Xna::Framework::Graphics::DepthStencilState;
            device.setBlendStateProperty(BlendState::NonPremultiplied);
            device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
            if (m_transparentStaticRenderer)
            {
                m_transparentStaticRenderer->Draw(device, effect);
            }
            if (m_waterRenderer)
            {
                m_waterRenderer->Draw(device, effect);
            }
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }
    }

    void GETerrainRenderer::DrawGrass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                      Microsoft::Xna::Framework::Graphics::BasicEffect& grassEffect) const
    {
        if (m_grassRenderer)
        {
            m_grassRenderer->Draw(device, grassEffect);
        }
    }

    int GETerrainRenderer::VertexCount() const noexcept
    {
        return (m_staticRenderer ? m_staticRenderer->VertexCount() : 0) +
               (m_transparentStaticRenderer ? m_transparentStaticRenderer->VertexCount() : 0) +
               (m_animRenderer ? m_animRenderer->VertexCount() : 0) +
               (m_waterRenderer ? m_waterRenderer->VertexCount() : 0) +
               (m_grassRenderer ? m_grassRenderer->VertexCount() : 0);
    }

    int GETerrainRenderer::PrimitiveCount() const noexcept
    {
        return (m_staticRenderer ? m_staticRenderer->PrimitiveCount() : 0) +
               (m_transparentStaticRenderer ? m_transparentStaticRenderer->PrimitiveCount() : 0) +
               (m_animRenderer ? m_animRenderer->PrimitiveCount() : 0) +
               (m_waterRenderer ? m_waterRenderer->PrimitiveCount() : 0) +
               (m_grassRenderer ? m_grassRenderer->PrimitiveCount() : 0);
    }
}
