#pragma once

#include <algorithm>

#include "TerrainRelief.hpp"
#include "TileAtlas.hpp"
#include "TileCoverage.hpp"

#include <GalaxyEggbert/Worlds/World.hpp>

#include <GalaxyEggbert/Game/MeshParts.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <Microsoft/Xna/Framework/Graphics/AlphaTestEffect.hpp>

#include <array>
#include <cstdint>
#include <future>
#include <memory>
#include <unordered_map>
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
                          const std::vector<TileCoverage>& tileCoverage = {},
                          // Draw the flight barrier (214), invisible in play (ShowSecret, D49).
                          bool drawSecret = false,
                          // Still (not animated) icons the caller draws as models (BlockModels.hpp:
                          // the pillars): left out of the mesh, listed by ModelCells().
                          std::vector<std::uint16_t> modelDrawnIcons = {},
                          // Tiles drawn as their scene's relief near the focus (UpdateRelief, Q-012);
                          // must outlive the renderer.
                          const TerrainReliefTable* reliefs = nullptr);

        // Rebuilds the animated-tile mesh for animPhase (no-op if unchanged
        // since the last call). Call once per frame with
        // WorldRuntime::GetAnimPhase(). No effect on the static mesh.
        void Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase);

        // The terrain reliefs (Q-001/Q-004/Q-012): the cells whose tile has a relief are kept in
        // chunks of kReliefChunkCells^3, each drawn as textured cubes, or as its cells' reliefs while
        // the chunk lies within `radius` blocks of `focus` (built here when it comes near, the nearest
        // first, at most `maxBuilds` chunks a call, all of them when negative; released when it is
        // kReliefRelease blocks farther). A capped call builds on worker threads (at most
        // kReliefBuildsInFlight at a time; on the web, where there are none, `maxBuilds` chunks in the
        // call) and uploads what they finished; a chunk shows its cubes until its relief is ready.
        // Call once a frame before drawing; never called, every chunk stays cubes.
        void UpdateRelief(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                          const Microsoft::Xna::Framework::Vector3& focus, float radius, int maxBuilds = 2);
        // Takes over `previous`'s built (or building) reliefs of the chunks whose cells are the same,
        // so a terrain built again for a few changed blocks (a door opened) does not build every near
        // relief again: only the changed chunks are, a frame or two later.
        void AdoptReliefs(TerrainRenderer& previous);
        // Keep the overhangs into the air too (Q-012 C), for comparison; takes effect on rebuilt chunks.
        void SetReliefOverhangs(bool keep) noexcept { m_reliefOverhangs = keep; }
        [[nodiscard]] int ReliefChunkCount() const noexcept { return static_cast<int>(m_reliefChunks.size()); }
        [[nodiscard]] int ReliefChunksShown() const noexcept;
        [[nodiscard]] int ReliefPrimitiveCount() const noexcept;
        static constexpr int kReliefChunkCells = 8;
        static constexpr float kReliefRelease = 4.0f;
        static constexpr int kReliefBuildsInFlight = 2;

        // The cutout pass takes @p effect's World/View/Projection and texture.
        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const;
        // Draw() in two halves, for a caller that draws models in between: the opaque terrain
        // (writing depth), then the semi-transparent pass (water, animated tiles, the
        // transparent icons; depth-read), which must come after everything opaque, or water
        // does not cover what lies in it (owner 2026-11-02, OR-0052: a chest under water
        // looked dry).
        void DrawOpaque(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                        Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const;
        void DrawTransparent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
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
        [[nodiscard]] int AnimatedBlockCount() const noexcept
        {
            return static_cast<int>(m_animBlocks.size() + m_lavaBlocks.size() + m_blitzBlocks.size());
        }
        [[nodiscard]] int WaterBlockCount() const noexcept { return static_cast<int>(m_waterBlocks.size()); }
        [[nodiscard]] int VertexCount() const noexcept;
        [[nodiscard]] int PrimitiveCount() const noexcept;

        // World-space centroid of every non-air block across all Y layers —
        // a reliable camera look-at target, since the spawn tile itself may
        // be an open-air cell Blupi stands in rather than a solid block.
        [[nodiscard]] float CentroidX() const noexcept { return m_centroidX; }
        [[nodiscard]] float CentroidY() const noexcept { return m_centroidY; }
        [[nodiscard]] float CentroidZ() const noexcept { return m_centroidZ; }

        // Cells of these animation bases are left to the caller, who draws them as models
        // (BlockModels.hpp: the fan, VIS-0127) instead of textured cubes; the next Update()
        // rebuilds the animated mesh without them.
        void SetModelDrawnBases(std::vector<std::uint16_t> bases)
        {
            m_modelBases = std::move(bases);
            m_lastAnimPhase = -1;
        }
        // Cells of these bases stay in the mesh as the floor around them (AnimBlock::floorIcon) and
        // are listed by ModelCells() one cell up, where the caller draws their model standing on
        // that floor (the saw, BlockModels.hpp kModelOverFloorBases; owner 2026-11-02).
        void SetModelOverFloorBases(std::vector<std::uint16_t> bases)
        {
            m_modelOverFloorBases = std::move(bases);
            m_lastAnimPhase = -1;
        }
        struct ModelCell
        {
            float x, y, z; // the cell's centre (world space)
            std::uint16_t base;
            // Still cells: which side neighbours are the same icon (1 +X, 2 -X, 4 +Z, 8 -Z).
            std::uint8_t sameSides = 0;
            // Still cells: the side (0 +X, 1 -X, 2 +Z, 3 -Z) facing the most room to stand in, the way
            // into the room (BlockModels.hpp).
            std::uint8_t openSide = 2;
        };
        // The cells of the bases set above, and of the still icons given to the constructor.
        [[nodiscard]] std::vector<ModelCell> ModelCells() const
        {
            std::vector<ModelCell> cells = m_staticModelCells;
            for (const auto& block : m_animBlocks)
            {
                if (std::find(m_modelBases.begin(), m_modelBases.end(), block.base) != m_modelBases.end())
                {
                    // openSide 0: the tile is turned (a side fan blowing along Z, M3D-0005).
                    cells.push_back({block.x, block.y, block.z, block.base, 0,
                                     static_cast<std::uint8_t>(block.rotated ? 0 : 2)});
                }
                else if (std::find(m_modelOverFloorBases.begin(), m_modelOverFloorBases.end(), block.base) !=
                         m_modelOverFloorBases.end())
                {
                    // openSide carries the blade's turn: 0 (+X) when turned to X, else 2 (+Z).
                    cells.push_back({block.x, block.y + 1.0f, block.z, block.base, 0,
                                     static_cast<std::uint8_t>(block.rotated ? 0 : 2)});
                }
            }
            return cells;
        }

    private:
        struct AnimBlock
        {
            float x;
            float y;
            float z;
            std::uint16_t base;
            // Lava (VIS-0069): bit 0..3 lava at +X, -X, +Z, -Z; bit 4 something under it.
            std::uint8_t lavaSides = 0;
            // A saw's cell (the floor it stands on, Galaxy's convention): the icon of the floor around
            // it, drawn in its place (0: none found, the rock); and whether its blade turns to X.
            std::uint16_t floorIcon = 0;
            bool rotated = false;
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

        // A relief tile's cell: its centre, icon, the faces shown (bit per BoxFace) and per face the
        // overhang sides that lead into air (TerrainRelief.hpp ReliefAirSides).
        struct ReliefCell
        {
            Microsoft::Xna::Framework::Vector3 center;
            std::uint16_t icon = 0;
            std::uint8_t faces = 0;
            std::array<std::uint8_t, 6> air{};
            bool grass = false; // laid as the grass's strips (TerrainRelief.hpp kGrassAirMask)
            bool plate = false; // an InnerFlatPlate: the relief both ways from the cell's middle
            bool cutout = false; // its sprite has clear texels: the relief alpha-tested, no backing
            // Faces drawn flat near the focus too, with their own texture rectangle: a DirectionalCube's
            // faces that do not wear the tile (the machine panels' plain sides, SingleTexturedSide).
            std::uint8_t flatFaces = 0;
            std::array<CnaEngine::Graphics::UvRect, 6> flatUv{};
        };
        // A chunk's reliefs: the opaque tiles' (lit) and the clear-texelled tiles' (alpha-tested,
        // drawn with the cutout effect).
        struct ReliefMeshData
        {
            CnaEngine::Graphics::MeshData lit;
            CnaEngine::Graphics::MeshData cut;
        };
        struct ReliefMeshes
        {
            std::unique_ptr<GpuMeshParts> lit;
            std::unique_ptr<GpuMeshParts> cut;
            [[nodiscard]] int PrimitiveCount() const noexcept
            {
                return (lit ? lit->PrimitiveCount() : 0) + (cut ? cut->PrimitiveCount() : 0);
            }
        };
        struct ReliefChunk
        {
            Microsoft::Xna::Framework::Vector3 boxMin;
            Microsoft::Xna::Framework::Vector3 boxMax;
            std::vector<ReliefCell> cells;
            std::unique_ptr<GpuMeshParts> cubes;
            std::unique_ptr<GpuMeshParts> grassTops; // far: the grass cells' top plates (DrawGrass)
            std::unique_ptr<GpuMeshParts> cutouts;   // far: the plates, alpha-tested (the cutout pass)
            std::unique_ptr<ReliefMeshes> relief;               // built (null: not yet, or released)
            std::future<ReliefMeshData> pending;                 // its relief being built
            std::uint64_t signature = 0;                         // of its cells (AdoptReliefs)
            bool near = false;
        };
        // The chunk's relief mesh; touches nothing of the renderer (it runs on a worker thread).
        static ReliefMeshData BuildReliefMesh(const TerrainReliefTable& reliefs, const TileAtlas& atlas,
                                                             bool overhangs, const std::vector<ReliefCell>& cells);
        static std::uint64_t ReliefSignature(const std::vector<ReliefCell>& cells);
        const TerrainReliefTable* m_reliefs = nullptr;
        std::vector<ReliefChunk> m_reliefChunks;
        bool m_reliefOverhangs = false;

        std::unique_ptr<GpuMeshParts> m_staticRenderer;
        std::unique_ptr<GpuMeshParts> m_transparentStaticRenderer;
        // Lit by vertex colour (AlphaTestEffect has no lights: BakeTerrainLighting()).
        std::unique_ptr<GpuMeshParts> m_cutoutRenderer;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::AlphaTestEffect> m_cutoutEffect;
        std::unique_ptr<GpuMeshParts> m_animRenderer;
        // The saws' floor cells, opaque (built once; see the constructor).
        std::unique_ptr<GpuMeshParts> m_sawFloorRenderer;
        std::unique_ptr<GpuMeshParts> m_waterRenderer;
        std::unique_ptr<GpuMeshParts> m_grassRenderer;
        std::vector<AnimBlock> m_animBlocks;
        std::vector<AnimBlock> m_waterBlocks;
        // The lava cells, apart from m_animBlocks: all of them show the same frame (68-72) at a time,
        // so each frame's mesh is built once, when first shown, and kept (Q-008: rebuilding a large
        // pool at every animation step made 105's lava floor ~2.7-6x slower to play).
        std::vector<AnimBlock> m_lavaBlocks;
        std::unordered_map<int, std::unique_ptr<GpuMeshParts>> m_lavaFrames;
        int m_lavaIcon = -1;
        // The lightning cells likewise (Q-002: each frame's 3D bolt built once).
        std::vector<AnimBlock> m_blitzBlocks;
        std::unordered_map<int, std::unique_ptr<GpuMeshParts>> m_blitzFrames;
        int m_blitzIcon = -1;
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
        std::vector<std::uint16_t> m_modelBases;
        std::vector<std::uint16_t> m_modelOverFloorBases;
        std::vector<ModelCell> m_staticModelCells;
    };
}
