#include "TerrainRenderer.hpp"
#include "SceneEffectsLogic.hpp"

#include "LavaSurface.hpp"
#include "BlockModels.hpp"
#include "BoltMesh.hpp"
#include "BrickEdges.hpp"
#include "DirectionalCubeTiles.hpp"
#include "InnerFlatPlateTiles.hpp"
#include "InnerPillarBoxTiles.hpp"
#include "ThinBarTiles.hpp"
#include "WorldRuntime.hpp"
#include "WaterSurface.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/CompareFunction.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <tuple>
#include <set>

namespace GalaxyEggbert::Game
{
    namespace
    {
        // Matches WorldRuntime::kWorldCenterX/kWorldCenterZ — centers the
        // 100x100 reference grid on the origin, one world unit per tile.
        constexpr int kWorldCenterX = WorldRuntime::kWorldCenterX;
        constexpr int kWorldCenterZ = WorldRuntime::kWorldCenterZ;

        // Packs raw grid coordinates (0..blocksPerAxis-1, comfortably under
        // 1024 each) into one key for m_rotatedPlatePositions' hash set --
        // see TerrainRenderer.hpp's own comment.
        std::uint32_t PackPlatePositionKey(int x, int y, int z)
        {
            return (static_cast<std::uint32_t>(x) << 20) |
                   (static_cast<std::uint32_t>(y) << 10) |
                   static_cast<std::uint32_t>(z);
        }

        // A relief chunk's key: its cell coordinates over kReliefChunkCells, packed as above.
        std::uint32_t ReliefChunkKey(int x, int y, int z)
        {
            constexpr int k = TerrainRenderer::kReliefChunkCells;
            return static_cast<std::uint32_t>((x / k) << 20 | (y / k) << 10 | (z / k));
        }

        // InnerFlatPlate/TripleCrossBillboard geometry sizing — "spans most
        // of the block" per the questionnaire wording, leaving a visible
        // margin so the plate/cross doesn't clip through neighboring blocks'
        // faces.
        constexpr float kInnerFlatPlateWidth = 0.75f;
        constexpr float kInnerFlatPlateHeight = 0.9f;
        constexpr float kTripleCrossWidth = 0.8f;
        constexpr float kTripleCrossHeight = 1.0f;

        // Since 2026-11-02 the teleporters are their original model in one cell
        // (BlockModels.hpp, OR-0025): this cube and tip are only the fallback
        // without avatars3d/objects.
        // Teleporter pillars (330-333, plan.md E3D-MIG-147): the "hrot"
        // (spike/tip) hanging below the block, per direct user Q&A live in
        // session 2026-07-11 ("pod teleporterem by měl být prostor... zbytek
        // ten hrot teleporteru by měl být renderován pod teleporterem").
        // Went through 3 rounds of live user feedback the same day:
        // (1) first modeled as a tapering pyramid -- a live screenshot
        //     showed solid BLACK filling the lower/wider portion of each
        //     triangular face, which read as a straight-sided shape
        //     ("čtyřhranol") rather than a cone;
        // (2) rebuilt as a non-tapering box (4 straight `DirectionalCube`
        //     side faces) -- this also (separately) surfaced that the
        //     "black" was actually real alpha=0 transparency being
        //     rendered opaque (fixed via the registry's `alphaBlend`, unrelated to
        //     the box-vs-pyramid choice), but the box itself then showed a
        //     new problem: its 4 flat side faces (each showing a triangle
        //     via an alpha cutout) don't share a common vertex the way a
        //     real pyramid's 4 triangular faces do, so adjacent faces'
        //     triangle graphics visibly failed to connect at the block's 4
        //     vertical edges ("ty hroty 4 textury trojúhelníku nejsou dole
        //     svázané k sobě");
        // (3) reverted to a genuine tapering pyramid (then easy-3d's
        //     `PyramidTipItem`/`AppendPyramidTipMesh()`, re-added there after
        //     round 1 had removed it; since 2026-10-20 AddPyramidTip()
        //     below, same vertices) -- now combined
        //     with the alpha-blend fix from round 2, which independently
        //     fixes the original "black background" problem regardless of
        //     shape, so the pyramid's structurally-shared apex vertex
        //     gives a seamless cone with no black and no seams.
        // Base spans most of the block's footprint, height stops just
        // short of a full block-height so it never pokes through a floor
        // one cell below an open teleporter room.
        constexpr float kTipBaseSize = 0.7f;
        constexpr float kTipHeight = 0.6f;

        // Square top face centered on `topCenter`, tapering to an apex
        // `height` below it -- the tip hanging under a teleporter pillar
        // (above). The top square keeps the -Y face's corner order (clockwise
        // seen from below); each side triangle runs from one top corner to
        // the next and down to the shared apex, so the four sides meet
        // seamlessly. One texture region for all five faces: the square
        // takes all of it, each triangle its top edge and the middle of its
        // bottom edge.
        void AddPyramidTip(CnaEngine::Graphics::MeshBuilder& mesh,
                           const Microsoft::Xna::Framework::Vector3& topCenter,
                           float baseSize, float height, const CnaEngine::Graphics::UvRect& uv)
        {
            using Microsoft::Xna::Framework::Vector2;
            using Microsoft::Xna::Framework::Vector3;
            const float half = baseSize * 0.5f;
            const Vector3 baseCorners[4] = {
                Vector3(topCenter.X - half, topCenter.Y, topCenter.Z + half),
                Vector3(topCenter.X + half, topCenter.Y, topCenter.Z + half),
                Vector3(topCenter.X + half, topCenter.Y, topCenter.Z - half),
                Vector3(topCenter.X - half, topCenter.Y, topCenter.Z - half),
            };
            const Vector3 apex(topCenter.X, topCenter.Y - height, topCenter.Z);

            const std::uint32_t top0 = mesh.AddVertex(TexturedVertex(baseCorners[0], Vector2(uv.u0, uv.v0)));
            const std::uint32_t top1 = mesh.AddVertex(TexturedVertex(baseCorners[1], Vector2(uv.u1, uv.v0)));
            const std::uint32_t top2 = mesh.AddVertex(TexturedVertex(baseCorners[2], Vector2(uv.u1, uv.v1)));
            const std::uint32_t top3 = mesh.AddVertex(TexturedVertex(baseCorners[3], Vector2(uv.u0, uv.v1)));
            mesh.AddQuad(top0, top1, top2, top3);

            const Vector2 uvA(uv.u0, uv.v0);
            const Vector2 uvB(uv.u1, uv.v0);
            const Vector2 uvApex((uv.u0 + uv.u1) * 0.5f, uv.v1);
            for (int i = 0; i < 4; ++i)
            {
                const std::uint32_t a = mesh.AddVertex(TexturedVertex(baseCorners[i], uvA));
                const std::uint32_t b = mesh.AddVertex(TexturedVertex(baseCorners[(i + 1) % 4], uvB));
                const std::uint32_t c = mesh.AddVertex(TexturedVertex(apex, uvApex));
                mesh.AddTriangle(a, b, c);
            }
        }

        bool IsTeleporterTipIcon(int icon)
        {
            return GalaxyEggbert::GetBlockDefinition(static_cast<std::uint16_t>(icon)).teleporterTip;
        }

        // The tip's 4 side faces reuse the teleporter's own tile texture,
        // cropped to its lower two-thirds (the dark post graphic) --
        // confirmed by direct inspection of the real icon crop (icon330.png
        // etc.): the top third is the flat panel (colored dots + emblem
        // letter), the bottom two-thirds is the post itself. V grows
        // downward in this atlas (tileUV's vOff comes straight from the
        // row's top pixel), so "lower" means the higher-V end of the tile's
        // own rect.
        CnaEngine::Graphics::UvRect TeleporterTipUv(const CnaEngine::Graphics::UvRect& tileUv)
        {
            const float v0 = tileUv.v0 + (tileUv.v1 - tileUv.v0) / 3.0f;
            return CnaEngine::Graphics::UvRect{tileUv.u0, v0, tileUv.u1, tileUv.v1};
        }

        // The cube's own 4 side faces used to reuse the WHOLE tile texture
        // (the shared kSymmetricEntries convention every other icon in that
        // table uses) -- which meant the post/spike graphic (the same lower
        // two-thirds TeleporterTipUv() crops out for the real 3D tip below)
        // ALSO appeared flattened onto the cube's own side faces, right
        // where the real 3D tip already hangs. Reported live 2026-07-12
        // (user, Czech): "ty hroty tam jsou dvakrat" (the tips are there
        // twice) -- confirmed by a live headless screenshot (with the whole
        // world isolated down to just this one pillar) showing two distinct
        // cone shapes stacked with a gap: the top one was this flat, fake
        // cutout on the cube's own face, the bottom one the genuine 3D
        // pyramid. Cropping the side faces to just the top third (the real
        // panel graphic) removes the fake/duplicate one, leaving only the
        // real pyramid tip hanging below (already flush with the cube's
        // bottom face, no reposition needed).
        CnaEngine::Graphics::UvRect TeleporterPanelUv(const CnaEngine::Graphics::UvRect& tileUv)
        {
            const float v1 = tileUv.v0 + (tileUv.v1 - tileUv.v0) / 3.0f;
            return CnaEngine::Graphics::UvRect{tileUv.u0, tileUv.v0, tileUv.u1, v1};
        }

        // Appends whichever "new geometry" render mode (if any) @p lookupIcon
        // uses, into @p mesh, textured with @p tileUv.
        // Returns true if it handled the icon (caller should skip its normal
        // UniformCube path); false if @p lookupIcon isn't one of these
        // modes. @p lookupIcon and @p tileUv's icon are the same for static
        // (non-animated) blocks, but deliberately different for animated
        // ones: @p lookupIcon is the animation group's fixed base icon (so
        // the render-mode/face-pattern lookup doesn't change frame to
        // frame), while @p tileUv is always the currently-sampled frame's
        // actual texture (see TerrainRenderer::Update's fan-tile comment).
        // @p plateRotated is only consulted for the InnerFlatPlate branch
        // (see InnerFlatPlateTiles.hpp's kPlateRotationMetadataType) --
        // callers pass whether THIS block's own position has rotation
        // metadata set, false for anything not even a candidate.
        bool AppendSpecialGeometry(const GalaxyEggbert::BlockDefinition& definition,
                                   const CnaEngine::Graphics::UvRect& tileUv,
                                   const CnaEngine::Graphics::UvRect& icon107Uv,
                                   const Microsoft::Xna::Framework::Vector3& center,
                                   bool plateRotated,
                                   CnaEngine::Graphics::MeshBuilder& mesh,
                                   std::uint8_t hiddenFaces = 0, int texturedSide = -1)
        {
            using CnaEngine::Graphics::BoxFace;
            const int lookupIcon = static_cast<int>(definition.type);
            CnaEngine::Graphics::BoxFaces directionalFaces;
            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube)
            {
                ConfigureDirectionalCubeFaces(
                    lookupIcon, tileUv, icon107Uv, directionalFaces, texturedSide);
                for (int face = 0; face < 6; ++face)
                {
                    if (hiddenFaces & (1u << face))
                    {
                        directionalFaces[face].visible = false;
                    }
                }

                if (IsTeleporterTipIcon(lookupIcon))
                {
                    // Crop the 4 side faces to the panel (top third) only --
                    // see TeleporterPanelUv's own comment for why (removes
                    // the fake, flattened "second tip" duplicate).
                    const auto panelUv = TeleporterPanelUv(tileUv);
                    directionalFaces[static_cast<int>(BoxFace::PositiveZ)].uv = panelUv;
                    directionalFaces[static_cast<int>(BoxFace::NegativeZ)].uv = panelUv;
                    directionalFaces[static_cast<int>(BoxFace::PositiveX)].uv = panelUv;
                    directionalFaces[static_cast<int>(BoxFace::NegativeX)].uv = panelUv;
                }

                if (definition.doubleSidedCutout)
                    AddDoubleSidedCenteredBox(mesh,center,directionalFaces);
                else
                AddCenteredBox(mesh, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), directionalFaces);

                if (IsTeleporterTipIcon(lookupIcon))
                {
                    AddPyramidTip(mesh, Microsoft::Xna::Framework::Vector3(center.X, center.Y - 0.5f, center.Z),
                                  kTipBaseSize, kTipHeight, TeleporterTipUv(tileUv));
                }

                return true;
            }

            CnaEngine::Graphics::BoxFaces pillarFaces;
            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::InnerPillarBox)
            {
                ConfigureInnerPillarBoxFaces(lookupIcon, tileUv, pillarFaces);
                AddCenteredBox(mesh, center,
                               Microsoft::Xna::Framework::Vector3(kInnerPillarWidth, kInnerPillarHeight, kInnerPillarWidth),
                               pillarFaces);
                return true;
            }

            CnaEngine::Graphics::BoxFaces thinBarFaces;
            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::ThinBar)
            {
                ConfigureThinBarFaces(lookupIcon, tileUv, thinBarFaces);
                // Full block width along X (the bar's own long axis, so
                // adjacent bar blocks connect seamlessly), thin in Y/Z --
                // see ThinBarTiles.hpp's own comment for the real
                // orientation justification.
                AddCenteredBox(mesh, center,
                               Microsoft::Xna::Framework::Vector3(1.0f, kThinBarThickness, kThinBarThickness),
                               thinBarFaces);
                return true;
            }

            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::InnerFlatPlate)
            {
                mesh.AddPlate(center, kInnerFlatPlateWidth, kInnerFlatPlateHeight,
                              GetInnerFlatPlateAxis(lookupIcon, plateRotated), tileUv);
                return true;
            }

            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::TripleCrossBillboard)
            {
                mesh.AddCrossedPlates(center, kTripleCrossWidth, kTripleCrossHeight, 3, tileUv);
                return true;
            }

            return false;
        }

        int AnimIcon(std::uint16_t base, int rawTick)
        {
            return GalaxyEggbert::ResolveBlockAnimationIcon(base, rawTick);
        }

        // The cell's own frame (its offset in the cycle, BD-0065).
        int AnimIconAt(std::uint16_t base, int rawTick, float x, float y, float z, bool blupiFocused)
        {
            return GalaxyEggbert::ResolveBlockAnimationIconAt(base, rawTick, static_cast<int>(std::lround(x)),
                                                              static_cast<int>(std::lround(y)),
                                                              static_cast<int>(std::lround(z)), blupiFocused);
        }

        // Water is animated too, but drawn as its own semi-transparent pass
        // (see TerrainRenderer::Draw) -- IsAnimated() deliberately
        // EXCLUDES it so the constructor/Update() route water blocks to
        // m_waterBlocks/m_waterRenderer instead of m_animBlocks/m_animRenderer.
        bool IsWater(std::uint16_t base)
        {
            return GalaxyEggbert::GetBlockDefinition(base).water;
        }

        bool IsAnimated(std::uint16_t base)
        {
            const auto& definition = GalaxyEggbert::GetBlockDefinition(base);
            return definition.animation.IsAnimated() && !definition.water;
        }

        // Icons 30/31: confirmed DirectionalCube (DirectionalCubeTiles.cpp)
        // whose own side-face texture has real per-pixel alpha ("textura má
        // i průhlednost") -- editor-only start-position markers for a
        // moveable object, not used in actual gameplay worlds, but still
        // need genuine alpha blending to render correctly if ever placed.
        // Not animated, so they don't go through m_animBlocks/m_waterBlocks
        // -- routed to their own static-but-transparent renderer instead.
        //
        // Teleporter pillars (330-333) added 2026-07-11: direct pixel
        // inspection of the real icon crop (confirmed via a small script
        // sampling object-m.png's alpha channel) found the "black" area the
        // user reported around the post/tip graphic is genuine alpha=0
        // (fully transparent), not painted black -- rendering it opaque was
        // the actual bug, same category as icons 30/31 above, not a
        // geometry problem (the tip's pyramid-vs-prism shape was a separate,
        // real issue, already fixed independently -- see AppendSpecialGeometry).
        // Icon 107: confirmed DirectionalCube with its top face intentionally
        // left open in DirectionalCubeTiles.cpp (see that file's comment) --
        // its real top surface is this separate grass_top.png overlay
        // (NEXT.md §8 task 3), not part of object-m.png at all. Icons
        // 108/109 need the SAME grass texture plus more per-side work
        // (DirectionalCubeTiles.hpp) -- not included here yet.
        constexpr float kGrassPlateWidth = 1.0f;
        constexpr float kGrassPlateDepth = 1.0f;

        // `towardFace`: the neighbour's face (BoxFace) that touches the face asked about; the block's
        // own faces count then (IsTerrainOccluderFace), -1 the whole block.
        bool IsOccluderBlock(const Worlds::World& world, int blocksPerAxis, int x, int y, int z,
                             const std::vector<TileCoverage>& tileCoverage, int towardFace = -1)
        {
            if (x < 0 || y < 0 || z < 0 || x >= blocksPerAxis || y >= blocksPerAxis || z >= blocksPerAxis)
                return false;
            return towardFace < 0 ? IsTerrainOccluder(world.getBlock(x, y, z), tileCoverage)
                                  : IsTerrainOccluderFace(world.getBlock(x, y, z), tileCoverage, towardFace);
        }

        // A face of a DirectionalCube hidden by its neighbour (IsOccluderBlock), as a plain cube's
        // faces are: the machine panel (10) and its kin fill whole worlds, and drawing every face
        // between two of them overflowed the GPU's vertex buffer (D47).
        std::uint8_t HiddenFaceMask(const Worlds::World& world, int blocksPerAxis, int x, int y, int z,
                                    const std::vector<TileCoverage>& tileCoverage)
        {
            using CnaEngine::Graphics::BoxFace;
            struct Side { BoxFace face; int dx, dy, dz; };
            static constexpr Side kSides[] = {
                {BoxFace::PositiveZ, 0, 0, 1}, {BoxFace::NegativeZ, 0, 0, -1}, {BoxFace::PositiveX, 1, 0, 0},
                {BoxFace::NegativeX, -1, 0, 0}, {BoxFace::PositiveY, 0, 1, 0}, {BoxFace::NegativeY, 0, -1, 0},
            };
            std::uint8_t mask = 0;
            for (const auto& side : kSides)
            {
                // The neighbour's face against this one: BoxFace pairs (+Z, -Z), (+X, -X), (+Y, -Y).
                if (IsOccluderBlock(world, blocksPerAxis, x + side.dx, y + side.dy, z + side.dz, tileCoverage,
                                    static_cast<int>(side.face) ^ 1))
                {
                    mask |= static_cast<std::uint8_t>(1u << static_cast<unsigned>(side.face));
                }
            }
            return mask;
        }

        // What lies across each face of a ragged brick edge cell (BrickEdges.hpp), in BoxFace order.
        enum class BrickNeighbour : std::uint8_t
        {
            Open,   // air, a cell drawn otherwise: the face is seen
            Whole,  // a block whose face towards the cell is hidden: the cell covers what it does not
            Ragged, // another ragged brick edge cell: each draws its own faces
        };

        // A brick edge cell drawn as its four courses (Q-011, BrickEdges.hpp): each course a box ending
        // where its bricks end on the cell's ragged sides; the faces textured by position, only the parts
        // that can be seen. A face across a ragged end wears the edge piece @p side (its bricks end with
        // the course), every other side face and the parts drawn for a Whole neighbour (its face over a
        // ragged end, hidden by the cell) the whole brick @p whole; top and bottom @p top and @p bottom.
        void AppendRaggedBrick(CnaEngine::Graphics::MeshBuilder& mesh, const Microsoft::Xna::Framework::Vector3& center,
                               const BrickEdges::Shape& shape, const CnaEngine::Graphics::UvRect& side,
                               const CnaEngine::Graphics::UvRect& whole, const CnaEngine::Graphics::UvRect& top,
                               const CnaEngine::Graphics::UvRect& bottom, const std::array<BrickNeighbour, 6>& across,
                               const BrickEdges::Shape& above, const BrickEdges::Shape& below)
        {
            using CnaEngine::Graphics::BoxFace;
            using Microsoft::Xna::Framework::Vector3;
            const Vector3 origin = center - Vector3(0.5f, 0.5f, 0.5f);
            const auto lerp = [](float a, float b, float t) { return a + (b - a) * t; };
            const bool raggedX = (shape.raggedSides & ((1u << BrickEdges::PositiveX) | (1u << BrickEdges::NegativeX))) != 0;
            const bool raggedZ = (shape.raggedSides & ((1u << BrickEdges::PositiveZ) | (1u << BrickEdges::NegativeZ))) != 0;
            // One face of the box [lo, hi] (cell units, a zero thickness for a lone quad); a cap is a
            // Whole neighbour's face.
            const auto face = [&](BoxFace f, Vector3 lo, Vector3 hi, bool cap = false) {
                const bool alongX = f == BoxFace::PositiveZ || f == BoxFace::NegativeZ;
                const auto& rect = !cap && (alongX ? raggedX : raggedZ) ? side : whole;
                const auto sideU = [&](float l) { return lerp(rect.u0, rect.u1, l); };
                const auto sideV = [&](float ly) { return lerp(rect.v0, rect.v1, 1.0f - ly); };
                CnaEngine::Graphics::BoxFaces faces;
                for (auto& style : faces) style.visible = false;
                auto& style = faces[static_cast<int>(f)];
                style.visible = true;
                CnaEngine::Graphics::UvRect uv;
                switch (f)
                {
                case BoxFace::PositiveZ: uv = {sideU(lo.X), sideV(hi.Y), sideU(hi.X), sideV(lo.Y)}; break;
                case BoxFace::NegativeZ: uv = {sideU(hi.X), sideV(hi.Y), sideU(lo.X), sideV(lo.Y)}; break;
                case BoxFace::PositiveX: uv = {sideU(hi.Z), sideV(hi.Y), sideU(lo.Z), sideV(lo.Y)}; break;
                case BoxFace::NegativeX: uv = {sideU(lo.Z), sideV(hi.Y), sideU(hi.Z), sideV(lo.Y)}; break;
                case BoxFace::PositiveY:
                    uv = {lerp(top.u0, top.u1, lo.X), lerp(top.v0, top.v1, lo.Z), lerp(top.u0, top.u1, hi.X),
                          lerp(top.v0, top.v1, hi.Z)};
                    break;
                case BoxFace::NegativeY:
                    uv = {lerp(bottom.u0, bottom.u1, lo.X), lerp(bottom.v0, bottom.v1, 1.0f - hi.Z),
                          lerp(bottom.u0, bottom.u1, hi.X), lerp(bottom.v0, bottom.v1, 1.0f - lo.Z)};
                    break;
                }
                style.uv = uv;
                mesh.AddBox(origin + lo, origin + hi, faces);
            };
            const auto flat = [&](BoxFace f, const BrickEdges::Rect& r, float y) {
                face(f, Vector3(r.x0, y, r.z0), Vector3(r.x1, y, r.z1));
            };
            static constexpr BoxFace kSideFaces[4] = {BoxFace::PositiveZ, BoxFace::NegativeZ, BoxFace::PositiveX,
                                                      BoxFace::NegativeX};
            const BrickEdges::Rect cell{};
            for (int course = 0; course < BrickEdges::kCourseCount; ++course)
            {
                const float yTop = 1.0f - static_cast<float>(BrickEdges::kCourseRows[static_cast<std::size_t>(course)]) /
                                              BrickEdges::kTileTexels;
                const float yBottom =
                    1.0f - static_cast<float>(BrickEdges::kCourseRows[static_cast<std::size_t>(course) + 1]) /
                               BrickEdges::kTileTexels;
                const auto r = BrickEdges::CourseRect(shape, course);
                const auto& inset = shape.inset[static_cast<std::size_t>(course)];
                for (int s = 0; s < 4; ++s)
                {
                    const bool onBoundary = inset[static_cast<std::size_t>(s)] <= BrickEdges::kSliver;
                    const auto neighbour = across[static_cast<std::size_t>(s)];
                    if (!onBoundary || neighbour != BrickNeighbour::Whole)
                    {
                        face(kSideFaces[s], Vector3(r.x0, yBottom, r.z0), Vector3(r.x1, yTop, r.z1));
                    }
                    if (!onBoundary || neighbour != BrickNeighbour::Whole) continue;
                    // The Whole neighbour's face over the ends beside this course (its own is hidden).
                    const bool alongX = s == BrickEdges::PositiveZ || s == BrickEdges::NegativeZ;
                    const float plane = s == BrickEdges::PositiveZ || s == BrickEdges::PositiveX ? 1.0f : 0.0f;
                    const auto gaps = alongX ? BrickEdges::Outside(r.x0, r.x1) : BrickEdges::Outside(r.z0, r.z1);
                    const BoxFace inward = kSideFaces[BrickEdges::Opposite(s)];
                    for (const auto& [a, b] : gaps)
                    {
                        if (alongX) face(inward, Vector3(a, yBottom, plane), Vector3(b, yTop, plane), true);
                        else face(inward, Vector3(plane, yBottom, a), Vector3(plane, yTop, b), true);
                    }
                }
                // Top: the cell's top over its neighbour above, else the step to the course above.
                if (course == 0)
                {
                    const auto up = across[static_cast<int>(BoxFace::PositiveY)];
                    if (up == BrickNeighbour::Open)
                        flat(BoxFace::PositiveY, r, yTop);
                    else
                    {
                        const auto cover = up == BrickNeighbour::Whole
                                               ? cell
                                               : BrickEdges::CourseRect(above, BrickEdges::kCourseCount - 1);
                        for (const auto& part : BrickEdges::Subtract(r, cover)) flat(BoxFace::PositiveY, part, yTop);
                        if (up == BrickNeighbour::Whole)
                            for (const auto& part : BrickEdges::Subtract(cell, r)) flat(BoxFace::NegativeY, part, yTop);
                    }
                }
                else
                {
                    for (const auto& part : BrickEdges::Subtract(r, BrickEdges::CourseRect(shape, course - 1)))
                        flat(BoxFace::PositiveY, part, yTop);
                }
                if (course == BrickEdges::kCourseCount - 1)
                {
                    const auto down = across[static_cast<int>(BoxFace::NegativeY)];
                    if (down == BrickNeighbour::Open)
                        flat(BoxFace::NegativeY, r, yBottom);
                    else
                    {
                        const auto cover = down == BrickNeighbour::Whole ? cell : BrickEdges::CourseRect(below, 0);
                        for (const auto& part : BrickEdges::Subtract(r, cover)) flat(BoxFace::NegativeY, part, yBottom);
                        if (down == BrickNeighbour::Whole)
                            for (const auto& part : BrickEdges::Subtract(cell, r)) flat(BoxFace::PositiveY, part, yBottom);
                    }
                }
                else
                {
                    for (const auto& part : BrickEdges::Subtract(r, BrickEdges::CourseRect(shape, course + 1)))
                        flat(BoxFace::NegativeY, part, yBottom);
                }
            }
        }
    }

    namespace
    {
        // The relief chunks' cube meshes, counted as the static terrain they replace.
        template <typename Chunks>
        int ReliefCubeCount(const Chunks& chunks, bool vertices)
        {
            int count = 0;
            for (const auto& chunk : chunks)
            {
                if (chunk.cubes)
                {
                    count += vertices ? chunk.cubes->VertexCount() : chunk.cubes->PrimitiveCount();
                }
            }
            return count;
        }
    }

    bool TerrainRenderer::PatchModelCells(const Worlds::World& world, const Worlds::World::BlockChanges& changes,
                                         const std::vector<std::uint16_t>& icons, bool drawSecret,
                                         const std::vector<TileCoverage>& coverage)
    {
        // A model load/unload changes which cells belong to GPU meshes, even without a grid edit.
        // A switch's saws (378 <-> 379) change only their anim blocks' bases, looked up before anything
        // changes so that a refused batch leaves the renderer as it was.
        std::vector<std::pair<std::size_t, std::uint16_t>> sawSwaps;
        if (icons != m_stillModelIcons || !FindSawAnimSwaps(changes, m_animBlocks, sawSwaps) ||
            !PatchTerrainModelCells(world, changes, icons, drawSecret, coverage,
                [&](std::uint16_t icon) { return m_reliefs && m_reliefs->Find(icon); }, m_staticModelCells))
            return false;
        for (const auto& [index, base] : sawSwaps)
        {
            m_animBlocks[index].base = base;
        }
        if (!sawSwaps.empty())
        {
            m_lastAnimPhase = -1; // the anim mesh shows the new frames at the next Update
        }
        for (const auto& c : changes.cells)
        {
            // Its chunk's count too: FinishChunks sums the chunks after a rebuild.
            auto& chunk = m_staticChunks[static_cast<std::size_t>(ChunkIndex(c.x / kStaticChunkCells, c.y / kStaticChunkCells,
                                                                             c.z / kStaticChunkCells))];
            if (Worlds::Block(c.oldRaw).isAir()) { ++m_blockCount; ++chunk.blocks; }
            if (Worlds::Block(c.newRaw).isAir()) { --m_blockCount; --chunk.blocks; }
        }
        m_centroidX = m_blockCount ? static_cast<float>(m_nonModelSumX / m_blockCount) : 0.0f;
        m_centroidY = m_blockCount ? static_cast<float>(m_nonModelSumY / m_blockCount) : 0.0f;
        m_centroidZ = m_blockCount ? static_cast<float>(m_nonModelSumZ / m_blockCount) : 0.0f;
        return true;
    }

    TerrainRenderer::TerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Worlds::World& world,
                                         const TileAtlas& tileAtlas,
                                         const std::vector<TileCoverage>& tileCoverage, bool drawSecret,
                                         std::vector<std::uint16_t> modelDrawnIcons,
                                         const TerrainReliefTable* reliefs, bool ambientOcclusion,
                                         const TerrainGiSettings* globalIllumination)
        : m_reliefs(reliefs != nullptr && !reliefs->Empty() ? reliefs : nullptr), m_tileAtlas(&tileAtlas)
    {
        m_ambientOcclusion = ambientOcclusion;
        m_globalIllumination = ambientOcclusion && globalIllumination != nullptr;
        if (m_globalIllumination)
        {
            m_giSettings = *globalIllumination;
        }
        m_drawSecret = drawSecret;
        m_stillModelIcons = std::move(modelDrawnIcons);
        m_rotatedPlatePositions = RotatedPlates(world);
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        m_blocksPerAxis = blocksPerAxis;
        m_chunksPerAxis = (blocksPerAxis + kStaticChunkCells - 1) / kStaticChunkCells;
        m_staticChunks.resize(static_cast<std::size_t>(m_chunksPerAxis) * m_chunksPerAxis * m_chunksPerAxis);
        const ChunkBuildInputs inputs{&world, &tileAtlas, &tileCoverage, drawSecret, &m_stillModelIcons, m_reliefs,
                                      &m_rotatedPlatePositions};
        // Chunk by chunk, each in the grid's own order (y, z, x) as the one mesh was before them.
        for (int cy = 0; cy < m_chunksPerAxis; ++cy)
        {
            for (int cz = 0; cz < m_chunksPerAxis; ++cz)
            {
                for (int cx = 0; cx < m_chunksPerAxis; ++cx)
                {
                    UploadChunk(device, world, tileCoverage, ChunkIndex(cx, cy, cz), BuildChunk(inputs, cx, cy, cz), false);
                }
            }
        }
        FinishChunks(device);
        Update(device, 0);
    }

    std::unordered_set<std::uint32_t> TerrainRenderer::RotatedPlates(const Worlds::World& world)
    {
        std::unordered_set<std::uint32_t> plates;
        for (const auto& pos : CollectRotatedPlatePositions(world))
        {
            plates.insert(PackPlatePositionKey(pos[0], pos[1], pos[2]));
        }
        return plates;
    }

    int TerrainRenderer::ChunkIndex(int cx, int cy, int cz) const noexcept
    {
        return (cy * m_chunksPerAxis + cz) * m_chunksPerAxis + cx;
    }

    TerrainRenderer::ChunkBuild TerrainRenderer::BuildChunk(const ChunkBuildInputs& inputs, int chunkX, int chunkY,
                                                            int chunkZ)
    {
        const Worlds::World& world = *inputs.world;
        const TileAtlas& tileAtlas = *inputs.tileAtlas;
        const std::vector<TileCoverage>& tileCoverage = *inputs.tileCoverage;
        const bool drawSecret = inputs.drawSecret;
        const std::vector<std::uint16_t>& modelDrawnIcons = *inputs.modelDrawnIcons;
        const TerrainReliefTable* const reliefs = inputs.reliefs;
        const std::unordered_set<std::uint32_t>& rotatedPlates = *inputs.rotatedPlates;
        ChunkBuild out;
        auto& staticMesh = out.opaque;
        auto& transparentStaticMesh = out.transparent;
        auto& cutoutMesh = out.cutout;
        auto& grassMesh = out.grass;
        // Icons 108/109's DirectionalCube entry reuses icon 107's own side
        // texture (DirectionalCubeTiles.cpp) -- computed once here since
        // it never changes, threaded through AppendSpecialGeometry.
        const auto icon107Uv = tileAtlas.GetTileUv(107);
        // The relief tiles' cubes, by relief chunk (UpdateRelief); a static chunk holds whole relief chunks.
        std::unordered_map<std::uint32_t, std::size_t> reliefChunkOf;
        const auto reliefChunkFor = [&](int x, int y, int z)
        {
            const int k = TerrainRenderer::kReliefChunkCells;
            const auto key = ReliefChunkKey(x, y, z);
            auto found = reliefChunkOf.find(key);
            if (found == reliefChunkOf.end())
            {
                found = reliefChunkOf.emplace(key, out.reliefs.size()).first;
                ChunkBuild::Relief chunk;
                chunk.key = key;
                const Microsoft::Xna::Framework::Vector3 low(static_cast<float>((x / k) * k - kWorldCenterX) - 0.5f,
                                                             static_cast<float>((y / k) * k) - 0.5f,
                                                             static_cast<float>((z / k) * k - kWorldCenterZ) - 0.5f);
                chunk.boxMin = low;
                chunk.boxMax = low + Microsoft::Xna::Framework::Vector3(static_cast<float>(k));
                out.reliefs.push_back(std::move(chunk));
            }
            return found->second;
        };

        // A relief tile's cell, near the focus, is its relief, which lies inside the cell: it does not
        // hide the faces of a neighbour drawn otherwise (they would leave a gap down to its lumps).
        const auto isReliefCell = [&](int nx, int ny, int nz)
        {
            if (reliefs == nullptr || nx < 0 || ny < 0 || nz < 0 || nx >= static_cast<int>(world.blocksPerAxis()) ||
                ny >= static_cast<int>(world.blocksPerAxis()) || nz >= static_cast<int>(world.blocksPerAxis()))
            {
                return false;
            }
            const auto other = world.getBlock(static_cast<std::uint16_t>(nx), static_cast<std::uint16_t>(ny),
                                              static_cast<std::uint16_t>(nz));
            return !other.isAir() && reliefs->Find(other.type()) != nullptr;
        };
        // The faces (bit per BoxFace) whose neighbour is a relief tile's cell.
        // A relief overhangs only into solid cells: not into a cell drawn as its model (the teleporter's
        // booth fills its cell; the rocks beside a teleporter's niche hid its ends, owner 2026-11-04).
        const auto reliefSolid = [&](int cx, int cy, int cz)
        {
            if (!IsOccluderBlock(world, static_cast<int>(world.blocksPerAxis()), cx, cy, cz, tileCoverage))
            {
                return false;
            }
            const auto type = world.getBlock(static_cast<std::uint16_t>(cx), static_cast<std::uint16_t>(cy),
                                             static_cast<std::uint16_t>(cz)).type();
            return std::find(modelDrawnIcons.begin(), modelDrawnIcons.end(), type) == modelDrawnIcons.end();
        };
        const auto reliefNeighbourFaces = [&](int cx, int cy, int cz)
        {
            static constexpr int kOffsets[6][3] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
            std::uint8_t mask = 0;
            for (int face = 0; face < 6; ++face)
            {
                if (isReliefCell(cx + kOffsets[face][0], cy + kOffsets[face][1], cz + kOffsets[face][2]))
                {
                    mask = static_cast<std::uint8_t>(mask | (1u << face));
                }
            }
            return mask;
        };
        // The brick edge pieces' shape (Q-011, BrickEdges.hpp) from their horizontal neighbours.
        const auto brickShapeAt = [&](int cx, int cy, int cz) {
            const int n = static_cast<int>(world.blocksPerAxis());
            if (cx < 0 || cy < 0 || cz < 0 || cx >= n || cy >= n || cz >= n) return BrickEdges::Shape{};
            const int icon = static_cast<int>(world.getBlock(static_cast<std::uint16_t>(cx), static_cast<std::uint16_t>(cy),
                                                             static_cast<std::uint16_t>(cz)).type());
            if (!BrickEdges::IsBrickEdgeIcon(icon)) return BrickEdges::Shape{};
            return BrickEdges::ShapeFor(icon, [&](int s) {
                const int nx = cx + BrickEdges::kSideDx[static_cast<std::size_t>(s)];
                const int nz = cz + BrickEdges::kSideDz[static_cast<std::size_t>(s)];
                if (nx < 0 || nz < 0 || nx >= n || nz >= n) return BrickEdges::Neighbour::Outside;
                return world.getBlock(static_cast<std::uint16_t>(nx), static_cast<std::uint16_t>(cy),
                                      static_cast<std::uint16_t>(nz)).isAir()
                           ? BrickEdges::Neighbour::Air
                           : BrickEdges::Neighbour::Solid;
            });
        };
        const int blocksPerAxis = static_cast<int>(world.blocksPerAxis());
        const int x0 = chunkX * kStaticChunkCells, x1 = std::min(blocksPerAxis, x0 + kStaticChunkCells);
        const int y0 = chunkY * kStaticChunkCells, y1 = std::min(blocksPerAxis, y0 + kStaticChunkCells);
        const int z0 = chunkZ * kStaticChunkCells, z1 = std::min(blocksPerAxis, z0 + kStaticChunkCells);
        for (int y = y0; y < y1; ++y)
        {
            for (int z = z0; z < z1; ++z)
            {
                for (int x = x0; x < x1; ++x)
                {
                    const auto block = world.getBlock(static_cast<std::uint16_t>(x),
                                                       static_cast<std::uint16_t>(y),
                                                       static_cast<std::uint16_t>(z));
                    if (block.isAir())
                    {
                        continue;
                    }
                    // The flight barrier (214) is invisible in play: Decor::DrawDecor skips it
                    // unless m_bDrawSecret (the ShowSecret cheat, Decor.cpp:930).
                    if (!drawSecret && block.type() == GalaxyEggbert::BlockTypes::FlightBarrier)
                    {
                        continue;
                    }

                    const float worldX = static_cast<float>(x - kWorldCenterX);
                    const float worldY = static_cast<float>(y);
                    const float worldZ = static_cast<float>(z - kWorldCenterZ);
                    ++out.blocks;
                    if (std::find(modelDrawnIcons.begin(), modelDrawnIcons.end(), block.type()) != modelDrawnIcons.end())
                    {
                        out.modelCells.push_back(*ComputeTerrainModelCell(world, x, y, z, modelDrawnIcons, drawSecret));
                        continue;
                    }
                    out.sumX += worldX;
                    out.sumY += worldY;
                    out.sumZ += worldZ;

                    const auto& definition = GalaxyEggbert::GetBlockDefinition(block.type());
                    const std::uint16_t animBase = definition.animation.baseIcon;
                    if (GalaxyEggbert::BlockTypes::isWind(block.type()))
                    {
                        // Its family's first frame (110, 114, 118, 122); BuildWindFrame draws it.
                        AnimBlock wind{worldX, worldY, worldZ,
                                       static_cast<std::uint16_t>(110 + (block.type() - 110) / 4 * 4)};
                        wind.rotated = rotatedPlates.count(PackPlatePositionKey(x, y, z)) != 0;
                        out.wind.push_back(wind);
                        continue;
                    }
                    if (IsWater(animBase))
                    {
                        AnimBlock water{worldX, worldY, worldZ, animBase};
                        water.waterFaces = WaterSurfaceFaces(world, x, y, z);
                        out.water.push_back(water);
                        continue;
                    }
                    if (IsAnimated(animBase))
                    {
                        std::uint8_t lavaSides = 0;
                        if (animBase == GalaxyEggbert::BlockTypes::Lava)
                        {
                            const auto sideIs = [&](int dx, int dy, int dz, bool lavaOnly)
                            {
                                const int nx = x + dx, ny = y + dy, nz = z + dz;
                                if (nx < 0 || ny < 0 || nz < 0 || nx >= blocksPerAxis || ny >= blocksPerAxis ||
                                    nz >= blocksPerAxis)
                                {
                                    return false;
                                }
                                const auto other = world.getBlock(static_cast<std::uint16_t>(nx),
                                                                  static_cast<std::uint16_t>(ny),
                                                                  static_cast<std::uint16_t>(nz));
                                if (lavaOnly)
                                {
                                    return !other.isAir() && GalaxyEggbert::GetBlockDefinition(other.type())
                                                                     .animation.baseIcon == GalaxyEggbert::BlockTypes::Lava;
                                }
                                return !other.isAir();
                            };
                            lavaSides = static_cast<std::uint8_t>(
                                (sideIs(1, 0, 0, true) ? 1 : 0) | (sideIs(-1, 0, 0, true) ? 2 : 0) |
                                (sideIs(0, 0, 1, true) ? 4 : 0) | (sideIs(0, 0, -1, true) ? 8 : 0) |
                                (sideIs(0, -1, 0, false) ? 16 : 0));
                        }
                        AnimBlock animBlock{worldX,worldY,worldZ,animBase,lavaSides};
                        animBlock.rotated = rotatedPlates.count(PackPlatePositionKey(x, y, z)) != 0;
                        if (animBase == GalaxyEggbert::BlockTypes::Lava)
                        {
                            out.lava.push_back(animBlock); // its frames built once each (Update)
                        }
                        else if (animBase == GalaxyEggbert::BlockTypes::Blitz)
                        {
                            out.blitz.push_back(animBlock); // the same: one frame for all at a time
                        }
                        else
                        {
                            out.anim.push_back(animBlock);
                        }
                        continue;
                    }

                    const Microsoft::Xna::Framework::Vector3 center(worldX, worldY, worldZ);
                    const int icon = static_cast<int>(block.type());
                    const auto tileUv = tileAtlas.GetTileUv(icon);
                    const bool plateRotated = rotatedPlates.count(PackPlatePositionKey(x, y, z)) != 0;

                    if (definition.alphaBlend)
                    {
                        // Not animated, so it skips m_waterBlocks entirely --
                        // goes straight into its own static-but-transparent
                        // buffer, built once here and never rebuilt by
                        // Update() (same lifecycle as m_staticRenderer).
                        AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated,
                                              transparentStaticMesh);
                        continue;
                    }

                    const bool grassRelief = definition.grassTop && reliefs != nullptr &&
                                             reliefs->Find(block.type()) != nullptr &&
                                             CoverageOf(tileCoverage, icon) == TileCoverage::Opaque;
                    if (grassRelief)
                    {
                        // The grass as its scene's strips near the focus (TerrainRelief.hpp
                        // kGrassAirMask); far, its cube and top plate as below, kept in its chunk.
                        const std::size_t chunkIndex = reliefChunkFor(x, y, z);
                        out.reliefs[chunkIndex].grass.AddPlate(
                            Microsoft::Xna::Framework::Vector3(worldX, worldY + 0.5f, worldZ), kGrassPlateWidth,
                            kGrassPlateDepth, CnaEngine::Graphics::PlateAxis::Y,
                            CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                        // Hidden by its neighbours as any cell (a relief cell does not show its faces to
                        // other relief cells: reliefNeighbourFaces is for the cells drawn otherwise).
                        const std::uint8_t hidden = HiddenFaceMask(world, blocksPerAxis, x, y, z, tileCoverage);
                        AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated,
                                              out.reliefs[chunkIndex].cubes, hidden, -1);
                        ReliefCell cell;
                        cell.center = center;
                        cell.icon = block.type();
                        cell.grass = true;
                        const bool topOpen = (hidden & (1u << 4)) == 0;
                        for (int face = 0; face < 6; ++face)
                        {
                            const bool open = (hidden & (1u << face)) == 0;
                            const bool strip = face <= 1 && topOpen; // +Z and -Z: the leaves on top
                            if (face == 4 || !(open || strip))
                            {
                                continue;
                            }
                            cell.faces = static_cast<std::uint8_t>(cell.faces | (1u << face));
                            cell.air[static_cast<std::size_t>(face)] = static_cast<std::uint8_t>(
                                ReliefAirSides(face, [&](int dx, int dy, int dz) {
                                    return reliefSolid(x + dx, y + dy, z + dz);
                                }) & kGrassAirMask);
                        }
                        if (cell.faces != 0)
                        {
                            out.reliefs[chunkIndex].cells.push_back(cell);
                        }
                        continue;
                    }
                    if (definition.grassTop)
                    {
                        // Additive, not exclusive: the block's sides/bottom
                        // still go through the normal DirectionalCube path
                        // below (which leaves the top face open on purpose,
                        // see DirectionalCubeTiles.cpp) -- this only adds
                        // the separate grass-textured top plate, offset to
                        // sit exactly at the block's top face (Y + 0.5).
                        grassMesh.AddPlate(Microsoft::Xna::Framework::Vector3(worldX, worldY + 0.5f, worldZ),
                                           kGrassPlateWidth, kGrassPlateDepth, CnaEngine::Graphics::PlateAxis::Y,
                                           CnaEngine::Graphics::UvRect{0.0f, 0.0f, 1.0f, 1.0f});
                    }

                    // Face culling (NEXT.md §8): only emit a face when its
                    // neighbor doesn't fully occlude it (see IsOccluderBlock).
                    // Uses the same faced-box geometry AppendSpecialGeometry
                    // already uses for holed render modes -- with all 6 faces
                    // visible and the same uv, it's byte-for-byte equivalent
                    // to a plain UniformCube (MeshBuilder::AddBox has one
                    // corner table for both; cna-engine's Easy3DEquivalenceTests),
                    // so this doesn't change anything for a block with no
                    // solid neighbor (e.g. every isolated demo block in this
                    // sample world) -- only interior faces between two solid
                    // blocks (the common case in bulk fills like the ground
                    // floor/walls/staircase) actually get culled.
                    //
                    // Note: Saw/SawStopped (SawBlade mode) never
                    // reaches this point -- both are always animated
                    // (IsAnimated(Saw) above), so they're diverted into
                    // m_animBlocks earlier in this same loop; their ground-
                    // anchored-overlay handling lives in
                    // RebuildAnimatedRenderer() instead (see
                    // SawBlade mode's own comment above and that
                    // call site for the full history).
                    //
                    // The tile's own texels pick the pass (TileCoverage.hpp).
                    auto* mesh = &staticMesh;
                    switch (CoverageOf(tileCoverage, icon))
                    {
                    case TileCoverage::Cutout:
                        mesh = &cutoutMesh;
                        break;
                    case TileCoverage::Translucent:
                        mesh = &transparentStaticMesh;
                        break;
                    case TileCoverage::Opaque:
                        break;
                    }
                    if (definition.doubleSidedCutout) mesh=&cutoutMesh;
                    // The faces its neighbours hide; a relief neighbour does not hide the faces of a cell
                    // drawn otherwise (reliefNeighbourFaces), but which side is the room's is the same.
                    const std::uint8_t occludedFaces =
                        definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube
                            ? HiddenFaceMask(world, blocksPerAxis, x, y, z, tileCoverage)
                            : std::uint8_t{0};
                    const std::uint8_t hiddenFaces = static_cast<std::uint8_t>(occludedFaces & ~reliefNeighbourFaces(x, y, z));
                    // A tile textured on one side only turns that side to the room (DirectionalCubeTiles.hpp
                    // SingleTexturedSide): the free cells in a row before each side, up to kRoomRun.
                    int texturedSide = -1;
                    if (definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube)
                    {
                        constexpr int kRoomRun = 8;
                        constexpr int kSideDx[4] = {0, 0, 1, -1}; // BoxFace order: +Z, -Z, +X, -X
                        constexpr int kSideDz[4] = {1, -1, 0, 0};
                        int room[4] = {0, 0, 0, 0};
                        if (SingleTexturedSide(icon, 0, room) >= 0)
                        {
                            for (int side = 0; side < 4; ++side)
                            {
                                while (room[side] < kRoomRun &&
                                       !IsOccluderBlock(world, blocksPerAxis, x + kSideDx[side] * (room[side] + 1), y,
                                                        z + kSideDz[side] * (room[side] + 1), tileCoverage))
                                {
                                    ++room[side];
                                }
                            }
                            texturedSide = SingleTexturedSide(icon, occludedFaces, room);
                            texturedSide = TurnTexturedSide(texturedSide,ObjectQuarterTurns(world,x,y,z));
                        }
                    }
                    if (reliefs != nullptr && (mesh == &staticMesh || mesh == &cutoutMesh) &&
                        definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube &&
                        !IsTeleporterTipIcon(icon) && reliefs->Find(block.type()) != nullptr)
                    {
                        // A relief tile that is a DirectionalCube (the machine panel 10 and its kin): its
                        // faces that wear the tile become the relief near the focus, the others stay flat.
                        CnaEngine::Graphics::BoxFaces cubeFaces;
                        ConfigureDirectionalCubeFaces(icon, tileUv, icon107Uv, cubeFaces, texturedSide);
                        const std::uint8_t ownHidden = occludedFaces;
                        const std::size_t chunkIndex = reliefChunkFor(x, y, z);
                        ReliefCell cell;
                        cell.center = center;
                        cell.icon = block.type();
                        for (int face = 0; face < 6; ++face)
                        {
                            auto& style = cubeFaces[face];
                            if ((ownHidden & (1u << face)) != 0)
                            {
                                style.visible = false;
                            }
                            if (!style.visible)
                            {
                                continue;
                            }
                            const bool wearsTile = style.uv.u0 == tileUv.u0 && style.uv.u1 == tileUv.u1 &&
                                                   style.uv.v0 == tileUv.v0 && style.uv.v1 == tileUv.v1;
                            if (wearsTile)
                            {
                                cell.faces = static_cast<std::uint8_t>(cell.faces | (1u << face));
                                cell.air[static_cast<std::size_t>(face)] = ReliefAirSides(face, [&](int dx, int dy, int dz) {
                                    return reliefSolid(x + dx, y + dy, z + dz);
                                });
                            }
                            else
                            {
                                cell.flatFaces = static_cast<std::uint8_t>(cell.flatFaces | (1u << face));
                                cell.flatUv[static_cast<std::size_t>(face)] = style.uv;
                            }
                        }
                        cell.cutout = mesh == &cutoutMesh;
                        AddCenteredBox(mesh == &cutoutMesh ? out.reliefs[chunkIndex].cutouts : out.reliefs[chunkIndex].cubes,
                                       center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), cubeFaces);
                        if ((cell.faces | cell.flatFaces) != 0)
                        {
                            out.reliefs[chunkIndex].cells.push_back(cell);
                        }
                        continue;
                    }
                    if (reliefs != nullptr && definition.renderMode == GalaxyEggbert::BlockRenderMode::InnerFlatPlate &&
                        mesh != &transparentStaticMesh && reliefs->Find(block.type()) != nullptr)
                    {
                        // A plate tile with a relief (the cave's edge plates 285/302): near the focus its
                        // scene both ways from the cell's middle, overhanging freely (it is decor in an
                        // empty cell); far off the plate as before, in its chunk.
                        const std::size_t chunkIndex = reliefChunkFor(x, y, z);
                        auto& far = mesh == &cutoutMesh ? out.reliefs[chunkIndex].cutouts : out.reliefs[chunkIndex].cubes;
                        AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated, far, hiddenFaces,
                                              texturedSide);
                        ReliefCell cell;
                        cell.center = center;
                        cell.icon = block.type();
                        cell.plate = true;
                        cell.cutout = mesh == &cutoutMesh;
                        const auto axis = GetInnerFlatPlateAxis(icon, plateRotated);
                        cell.faces = axis == CnaEngine::Graphics::PlateAxis::X   ? std::uint8_t{4 | 8}
                                     : axis == CnaEngine::Graphics::PlateAxis::Y ? std::uint8_t{16 | 32}
                                                                                 : std::uint8_t{1 | 2};
                        out.reliefs[chunkIndex].cells.push_back(cell);
                        continue;
                    }
                    if (mesh == &staticMesh && BrickEdges::IsBrickEdgeIcon(icon))
                    {
                        const auto shape = brickShapeAt(x, y, z);
                        if (shape.ragged)
                        {
                            // Ragged towards the open (Q-011): its courses, as the bricks end.
                            static constexpr int kOffsets[6][3] = {{0, 0, 1}, {0, 0, -1}, {1, 0, 0},
                                                                   {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}};
                            const std::uint8_t besideRelief = reliefNeighbourFaces(x, y, z);
                            std::array<BrickNeighbour, 6> across{};
                            for (int f = 0; f < 6; ++f)
                            {
                                const int nx = x + kOffsets[f][0], ny = y + kOffsets[f][1], nz = z + kOffsets[f][2];
                                if ((besideRelief & (1u << f)) != 0 ||
                                    !IsOccluderBlock(world, blocksPerAxis, nx, ny, nz, tileCoverage))
                                    across[static_cast<std::size_t>(f)] = BrickNeighbour::Open;
                                else
                                    across[static_cast<std::size_t>(f)] = brickShapeAt(nx, ny, nz).ragged
                                                                              ? BrickNeighbour::Ragged
                                                                              : BrickNeighbour::Whole;
                            }
                            // The whole brick for the faces that do not end with a course, and for the
                            // plain cube's top and bottom (283's are its flat colour, as its cube's).
                            const auto wholeUv = tileAtlas.GetTileUv(GalaxyEggbert::BlockTypes::BrickWall);
                            CnaEngine::Graphics::BoxFaces cubeFaces;
                            for (auto& style : cubeFaces) style.uv = wholeUv;
                            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube)
                                ConfigureDirectionalCubeFaces(icon, tileUv, icon107Uv, cubeFaces, -1);
                            AppendRaggedBrick(staticMesh, center, shape, tileUv, wholeUv,
                                              cubeFaces[static_cast<int>(CnaEngine::Graphics::BoxFace::PositiveY)].uv,
                                              cubeFaces[static_cast<int>(CnaEngine::Graphics::BoxFace::NegativeY)].uv,
                                              across, brickShapeAt(x, y + 1, z), brickShapeAt(x, y - 1, z));
                            ++out.ragged;
                            continue;
                        }
                    }
                    if (AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated, *mesh, hiddenFaces,
                                              texturedSide))
                    {
                        continue;
                    }

                    using CnaEngine::Graphics::BoxFace;
                    const bool reliefTile = reliefs != nullptr && (mesh == &staticMesh || mesh == &cutoutMesh) &&
                                            reliefs->Find(block.type()) != nullptr;
                    const std::uint8_t besideRelief = reliefTile ? std::uint8_t{0} : reliefNeighbourFaces(x, y, z);
                    CnaEngine::Graphics::BoxFaces faces;
                    faces[static_cast<int>(BoxFace::PositiveZ)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y, z + 1, tileCoverage, 1), tileUv};
                    faces[static_cast<int>(BoxFace::NegativeZ)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y, z - 1, tileCoverage, 0), tileUv};
                    faces[static_cast<int>(BoxFace::PositiveX)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x + 1, y, z, tileCoverage, 3), tileUv};
                    faces[static_cast<int>(BoxFace::NegativeX)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x - 1, y, z, tileCoverage, 2), tileUv};
                    faces[static_cast<int>(BoxFace::PositiveY)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y + 1, z, tileCoverage, 5), tileUv};
                    faces[static_cast<int>(BoxFace::NegativeY)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y - 1, z, tileCoverage, 4), tileUv};
                    for (int face = 0; face < 6; ++face)
                    {
                        if ((besideRelief & (1u << face)) != 0)
                        {
                            faces[face].visible = true;
                        }
                    }
                    // The toys' ball cubes: the ball's relief only on the sides; top and bottom the
                    // tile's own picture, flat (MR-0007; a plain colour there was not right, owner
                    // 2026-11-05).
                    const bool ballCube = IsBallCubeIcon(icon);
                    if (reliefTile)
                    {
                        // A relief tile (Q-012): its cube goes to its chunk, drawn as cubes far from the
                        // focus and as the cells' reliefs near it.
                        const std::size_t chunkIndex = reliefChunkFor(x, y, z);
                        AddCenteredBox(mesh == &cutoutMesh ? out.reliefs[chunkIndex].cutouts : out.reliefs[chunkIndex].cubes,
                                       center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), faces);
                        ReliefCell cell;
                        cell.center = center;
                        cell.icon = block.type();
                        cell.cutout = mesh == &cutoutMesh;
                        for (int face = 0; face < 6; ++face)
                        {
                            if (!faces[face].visible)
                            {
                                continue;
                            }
                            if (ballCube && (face == static_cast<int>(BoxFace::PositiveY) ||
                                             face == static_cast<int>(BoxFace::NegativeY)))
                            {
                                cell.flatFaces = static_cast<std::uint8_t>(cell.flatFaces | (1u << face));
                                cell.flatUv[static_cast<std::size_t>(face)] = faces[face].uv;
                                continue;
                            }
                            cell.faces = static_cast<std::uint8_t>(cell.faces | (1u << face));
                            cell.air[static_cast<std::size_t>(face)] = ReliefAirSides(face, [&](int dx, int dy, int dz) {
                                return reliefSolid(x + dx, y + dy, z + dz);
                            });
                        }
                        if ((cell.faces | cell.flatFaces) != 0)
                        {
                            out.reliefs[chunkIndex].cells.push_back(cell);
                        }
                        continue;
                    }
                    AddCenteredBox(*mesh, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), faces);
                }
            }
        }
        return out;
    }

    void TerrainRenderer::UploadChunk(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                      const Worlds::World& world, const std::vector<TileCoverage>& tileCoverage,
                                      int chunkIndex, ChunkBuild build, bool rebuilt)
    {
        // The occlusion alone, or (Ultra) the light from around with it.
        const auto bake = [&](CnaEngine::Graphics::MeshData& mesh)
        {
            if (m_globalIllumination)
            {
                BakeGlobalIllumination(mesh, world, tileCoverage, kWorldCenterX, kWorldCenterZ, *m_giSettings);
            }
            else
            {
                BakeAmbientOcclusion(mesh, world, tileCoverage, kWorldCenterX, kWorldCenterZ);
            }
        };
        StaticChunk& chunk = m_staticChunks[static_cast<std::size_t>(chunkIndex)];
        chunk = StaticChunk{};
        chunk.blocks = build.blocks;
        chunk.ragged = build.ragged;
        chunk.sumX = build.sumX;
        chunk.sumY = build.sumY;
        chunk.sumZ = build.sumZ;
        // Guarded (found 2026-07-18, plan.md EDITOR-107): an empty mesh is not uploaded (GpuMesh::Upload
        // rejects it, and CNA a draw of primitiveCount 0): an all-air World (the editor's New World), and
        // now most chunks of a world too.
        if (!build.opaque.Mesh().Empty())
        {
            if (m_ambientOcclusion)
            {
                auto occluded = build.opaque.Take();
                bake(occluded);
                chunk.opaque = UploadLitColoredTexturedMeshParts(device, occluded);
            }
            else
            {
                chunk.opaque = UploadLitTexturedMeshParts(device, build.opaque.Mesh());
            }
        }
        if (!build.cutout.Mesh().Empty())
        {
            auto cutouts = build.cutout.Take();
            if (m_ambientOcclusion)
            {
                BakeAmbientOcclusion(cutouts, world, tileCoverage, kWorldCenterX, kWorldCenterZ);
            }
            chunk.cutout = UploadBakedLitTexturedMeshParts(device, std::move(cutouts));
        }
        if (!build.transparent.Mesh().Empty())
        {
            chunk.transparent = UploadLitTexturedMeshParts(device, build.transparent.Mesh());
        }
        if (!build.grass.Mesh().Empty())
        {
            chunk.grass = UploadLitTexturedMeshParts(device, build.grass.Mesh());
        }

        // Its relief chunks. Rebuilt, the ones of this chunk are emptied first (a relief tile blown away
        // leaves none), and a relief chunk whose cells are the same keeps its built (or building) relief
        // as AdoptReliefs does across renderers.
        const int k = kStaticChunkCells;
        const int cx = chunkIndex % m_chunksPerAxis;
        const int cz = chunkIndex / m_chunksPerAxis % m_chunksPerAxis;
        const int cy = chunkIndex / (m_chunksPerAxis * m_chunksPerAxis);
        std::vector<std::pair<std::size_t, std::uint64_t>> before; // relief chunk, its signature
        if (rebuilt)
        {
            for (const auto& [key, index] : m_reliefChunkIndex)
            {
                const int rx = static_cast<int>(key >> 20) * kReliefChunkCells;
                const int ry = static_cast<int>((key >> 10) & 0x3ff) * kReliefChunkCells;
                const int rz = static_cast<int>(key & 0x3ff) * kReliefChunkCells;
                if (rx / k == cx && ry / k == cy && rz / k == cz)
                {
                    before.emplace_back(index, m_reliefChunks[index].signature);
                }
            }
        }
        std::vector<bool> refilled(m_reliefChunks.size(), false);
        for (auto& relief : build.reliefs)
        {
            auto found = m_reliefChunkIndex.find(relief.key);
            if (found == m_reliefChunkIndex.end())
            {
                found = m_reliefChunkIndex.emplace(relief.key, m_reliefChunks.size()).first;
                m_reliefChunks.emplace_back();
                refilled.push_back(false);
            }
            ReliefChunk& target = m_reliefChunks[found->second];
            refilled[found->second] = true;
            const std::uint64_t signature = ReliefSignature(relief.cells);
            if (target.signature != signature || !rebuilt)
            {
                target.relief.reset();
                target.pending = {};
            }
            target.boxMin = relief.boxMin;
            target.boxMax = relief.boxMax;
            target.cells = std::move(relief.cells);
            target.signature = signature;
            target.cubes.reset();
            target.grassTops.reset();
            target.cutouts.reset();
            target.gi.reset();
            if (!relief.cubes.Mesh().Empty())
            {
                if (m_ambientOcclusion)
                {
                    auto occluded = relief.cubes.Take();
                    bake(occluded);
                    if (m_globalIllumination)
                    {
                        const auto table = ReliefChunkGi(occluded, target.boxMin, kReliefChunkCells);
                        constexpr int kr = kReliefChunkCells;
                        target.gi = std::make_unique<Microsoft::Xna::Framework::Graphics::Texture2D>(
                            device, kr * kr, kr * kReliefGiFaces);
                        target.gi->SetData(table.data(), static_cast<int>(table.size()));
                    }
                    target.cubes = UploadLitColoredTexturedMeshParts(device, occluded);
                }
                else
                {
                    target.cubes = UploadLitTexturedMeshParts(device, relief.cubes.Mesh());
                }
            }
            if (!relief.grass.Mesh().Empty())
            {
                target.grassTops = UploadLitTexturedMeshParts(device, relief.grass.Mesh());
            }
            if (!relief.cutouts.Mesh().Empty())
            {
                auto cutouts = relief.cutouts.Take();
                if (m_ambientOcclusion)
                {
                    BakeAmbientOcclusion(cutouts, world, tileCoverage, kWorldCenterX, kWorldCenterZ);
                }
                target.cutouts = UploadBakedLitTexturedMeshParts(device, std::move(cutouts));
            }
        }
        for (const auto& [index, signature] : before)
        {
            if (!refilled[index])
            {
                ReliefChunk& emptied = m_reliefChunks[index];
                emptied.cells.clear();
                emptied.cubes.reset();
                emptied.grassTops.reset();
                emptied.cutouts.reset();
                emptied.gi.reset();
                emptied.relief.reset();
                emptied.pending = {};
                emptied.signature = ReliefSignature(emptied.cells);
            }
        }

        // The cells drawn otherwise: this chunk's in the lists, which stay in the grid's order (y, z, x).
        const auto inChunk = [&](float wx, float wy, float wz)
        {
            const int x = static_cast<int>(std::lround(wx)) + kWorldCenterX;
            const int y = static_cast<int>(std::lround(wy));
            const int z = static_cast<int>(std::lround(wz)) + kWorldCenterZ;
            return x / k == cx && y / k == cy && z / k == cz;
        };
        const auto replace = [&](std::vector<AnimBlock>& list, std::vector<AnimBlock>& added)
        {
            const std::size_t size = list.size();
            if (rebuilt)
            {
                list.erase(std::remove_if(list.begin(), list.end(),
                                          [&](const AnimBlock& b) { return inChunk(b.x, b.y, b.z); }),
                           list.end());
            }
            const bool changed = list.size() != size || !added.empty();
            list.insert(list.end(), added.begin(), added.end());
            return changed;
        };
        m_windChanged = replace(m_windBlocks, build.wind) || m_windChanged;
        m_waterChanged = replace(m_waterBlocks, build.water) || m_waterChanged;
        m_lavaChanged = replace(m_lavaBlocks, build.lava) || m_lavaChanged;
        m_blitzChanged = replace(m_blitzBlocks, build.blitz) || m_blitzChanged;
        m_animChanged = replace(m_animBlocks, build.anim) || m_animChanged;
        if (rebuilt)
        {
            m_staticModelCells.erase(std::remove_if(m_staticModelCells.begin(), m_staticModelCells.end(),
                                                    [&](const ModelCell& c) { return inChunk(c.x, c.y, c.z); }),
                                     m_staticModelCells.end());
        }
        m_staticModelCells.insert(m_staticModelCells.end(), build.modelCells.begin(), build.modelCells.end());
    }

    void TerrainRenderer::FinishChunks(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
    {
        // The lists in the grid's order (y, z, x), as one pass over the world made them: the water and
        // the other alpha-blended tiles are drawn in it, and PatchTerrainModelCells keeps its cells so.
        const auto gridOrder = [](const auto& a, const auto& b) { return std::tie(a.y, a.z, a.x) < std::tie(b.y, b.z, b.x); };
        for (auto* list : {&m_windBlocks, &m_waterBlocks, &m_lavaBlocks, &m_blitzBlocks, &m_animBlocks})
        {
            std::stable_sort(list->begin(), list->end(), gridOrder);
        }
        std::stable_sort(m_staticModelCells.begin(), m_staticModelCells.end(), gridOrder);
        // A list changed: its meshes are made again at the next Update (an emptied one is dropped here,
        // Update leaving everything as it is when there is nothing animated).
        if (m_lavaChanged)
        {
            m_lavaFrames.clear();
            m_lavaIcon = -1;
        }
        if (m_blitzChanged)
        {
            m_blitzFrames.clear();
            m_blitzIcon = -1;
        }
        if (m_windChanged)
        {
            for (auto& frame : m_windFrames) frame.reset();
            m_windFrame = -1;
        }
        if (m_animChanged && m_animBlocks.empty()) m_animRenderer.reset();
        if (m_waterChanged && m_waterBlocks.empty()) m_waterRenderer.reset();
        if (m_lavaChanged || m_blitzChanged || m_windChanged || m_animChanged || m_waterChanged)
        {
            m_lastAnimPhase = -1;
        }
        m_lavaChanged = m_blitzChanged = m_windChanged = m_animChanged = m_waterChanged = false;

        m_blockCount = 0;
        m_raggedBrickCount = 0;
        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        bool cutouts = false;
        for (const auto& chunk : m_staticChunks)
        {
            m_blockCount += chunk.blocks;
            m_raggedBrickCount += chunk.ragged;
            sumX += chunk.sumX;
            sumY += chunk.sumY;
            sumZ += chunk.sumZ;
            cutouts = cutouts || chunk.cutout != nullptr;
        }
        m_nonModelSumX = sumX;
        m_nonModelSumY = sumY;
        m_nonModelSumZ = sumZ;
        m_centroidX = m_blockCount > 0 ? static_cast<float>(sumX / m_blockCount) : 0.0f;
        m_centroidY = m_blockCount > 0 ? static_cast<float>(sumY / m_blockCount) : 0.0f;
        m_centroidZ = m_blockCount > 0 ? static_cast<float>(sumZ / m_blockCount) : 0.0f;

        cutouts = cutouts || std::any_of(m_reliefChunks.begin(), m_reliefChunks.end(),
                                         [](const ReliefChunk& chunk) { return chunk.cutouts != nullptr; });
        if (cutouts && !m_cutoutEffect)
        {
            m_cutoutEffect = std::make_unique<Microsoft::Xna::Framework::Graphics::AlphaTestEffect>(device);
            m_cutoutEffect->setVertexColorEnabledProperty(true);
            m_cutoutEffect->setAlphaFunctionProperty(
                Microsoft::Xna::Framework::Graphics::CompareFunction::GreaterEqual);
            m_cutoutEffect->setReferenceAlphaProperty(kCutoutReferenceAlpha);
        }
    }

    std::vector<std::array<int, 3>> TerrainRenderer::ChunksReachedBy(const ChunkBuildInputs& inputs,
                                                                     const Worlds::World::BlockChanges& changes,
                                                                     bool globalIllumination)
    {
        // The cells whose drawing a changed cell reaches, by chunk:
        // - within 2 every way: its neighbours' faces (1), the occlusion bake's corners (1), a brick end's
        //   neighbours' neighbours (2, BrickEdges::ShapeFor), a bar under a tile, the reliefs' sides;
        // - along the four horizontal lines from it, up to kRoomRun (8): a tile textured on one side in
        //   its row (the room before each of its sides, SingleTexturedSide), a still model there or up
        //   to kFrontModelFloorDepth above (TerrainModelDependsOn: its opening, its floor);
        // - Ultra's light from around: its rays' reach every way.
        const Worlds::World& world = *inputs.world;
        const int n = static_cast<int>(world.blocksPerAxis());
        const int k = kStaticChunkCells;
        const int chunks = (n + k - 1) / k;
        std::set<std::array<int, 3>> reached;
        const auto box = [&](int x, int y, int z, int r)
        {
            const auto chunkOf = [&](int cell) { return std::clamp(cell, 0, n - 1) / k; };
            for (int cy = chunkOf(y - r); cy <= chunkOf(y + r); ++cy)
                for (int cz = chunkOf(z - r); cz <= chunkOf(z + r); ++cz)
                    for (int cx = chunkOf(x - r); cx <= chunkOf(x + r); ++cx)
                        if (cx < chunks && cy < chunks && cz < chunks) reached.insert({cx, cy, cz});
        };
        const auto isModel = [&](std::uint16_t type) {
            return std::find(inputs.modelDrawnIcons->begin(), inputs.modelDrawnIcons->end(), type) !=
                   inputs.modelDrawnIcons->end();
        };
        const auto oneSided = [&](std::uint16_t type) {
            const int room[4] = {0, 0, 0, 0};
            return GalaxyEggbert::GetBlockDefinition(type).renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube &&
                   SingleTexturedSide(type, 0, room) >= 0;
        };
        constexpr int kRoomRun = 8; // BuildChunk's
        const int run = std::max(kRoomRun, kFrontModelOpenRun);
        for (const auto& c : changes.cells)
        {
            if (globalIllumination)
            {
                box(c.x, c.y, c.z, TerrainGiSettings{}.reach + 1);
                continue;
            }
            box(c.x, c.y, c.z, 2);
            for (int y = c.y - 1; y <= c.y + kFrontModelFloorDepth; ++y) // the column's models
            {
                if (y >= 0 && y < n && isModel(world.getBlock(c.x, static_cast<std::uint16_t>(y), c.z).type()))
                    box(c.x, y, c.z, 0);
            }
            static constexpr int kLines[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (const auto& line : kLines)
            {
                for (int d = 1; d <= run; ++d)
                {
                    const int x = c.x + line[0] * d, z = c.z + line[1] * d;
                    if (x < 0 || z < 0 || x >= n || z >= n) break;
                    for (int y = c.y; y <= c.y + kFrontModelFloorDepth && y < n; ++y)
                    {
                        const auto type = world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                         static_cast<std::uint16_t>(z)).type();
                        if ((y == c.y && d <= kRoomRun && oneSided(type)) || (d <= kFrontModelOpenRun && isModel(type)))
                            box(x, y, z, 0);
                    }
                }
            }
        }
        return {reached.begin(), reached.end()};
    }

    bool TerrainRenderer::RebuildChangedChunks(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                               const Worlds::World& world, const Worlds::World::BlockChanges& changes,
                                               const std::vector<std::uint16_t>& icons, bool drawSecret,
                                               const std::vector<TileCoverage>& coverage)
    {
        // An unknown history (the journal overflowed, metadata edited: a plate turned), another
        // ShowSecret or other models: the whole terrain is built again by the caller.
        if (!changes.known || drawSecret != m_drawSecret || icons != m_stillModelIcons ||
            static_cast<int>(world.blocksPerAxis()) != m_blocksPerAxis)
        {
            return false;
        }
        const ChunkBuildInputs inputs{&world, m_tileAtlas, &coverage, drawSecret, &m_stillModelIcons, m_reliefs,
                                      &m_rotatedPlatePositions};
        for (const auto& [cx, cy, cz] : ChunksReachedBy(inputs, changes, m_globalIllumination))
        {
            UploadChunk(device, world, coverage, ChunkIndex(cx, cy, cz), BuildChunk(inputs, cx, cy, cz), true);
        }
        FinishChunks(device);
        return true;
    }

    int TerrainRenderer::StaticChunkCount() const noexcept
    {
        return static_cast<int>(std::count_if(m_staticChunks.begin(), m_staticChunks.end(), [](const StaticChunk& c) {
            return c.opaque || c.transparent || c.cutout || c.grass;
        }));
    }

    std::uint64_t TerrainRenderer::ChunkFingerprint(const ChunkBuildInputs& inputs, int chunkX, int chunkY, int chunkZ,
                                                    const TerrainGiSettings* globalIllumination)
    {
        // FNV-1a over everything the CPU makes of the chunk, the static meshes after the bake the upload
        // gives them.
        std::uint64_t hash = 1469598103934665603ull;
        const auto bytes = [&](const void* data, std::size_t size)
        {
            const auto* p = static_cast<const unsigned char*>(data);
            for (std::size_t i = 0; i < size; ++i)
            {
                hash = (hash ^ p[i]) * 1099511628211ull;
            }
        };
        const auto value = [&](const auto& v) { bytes(&v, sizeof(v)); };
        const auto mesh = [&](CnaEngine::Graphics::MeshData data, bool baked)
        {
            if (baked)
            {
                if (globalIllumination != nullptr)
                    BakeGlobalIllumination(data, *inputs.world, *inputs.tileCoverage, kWorldCenterX, kWorldCenterZ,
                                           *globalIllumination);
                else
                    BakeAmbientOcclusion(data, *inputs.world, *inputs.tileCoverage, kWorldCenterX, kWorldCenterZ);
            }
            value(data.positions.size());
            for (const auto& v : data.positions) { value(v.X); value(v.Y); value(v.Z); }
            for (const auto& v : data.normals) { value(v.X); value(v.Y); value(v.Z); }
            for (const auto& v : data.texCoords0) { value(v.X); value(v.Y); }
            for (const auto& c : data.colors)
            {
                value(c.getRProperty()); value(c.getGProperty()); value(c.getBProperty()); value(c.getAProperty());
            }
            value(data.indices.size());
            for (const auto i : data.indices) value(i);
        };
        auto build = BuildChunk(inputs, chunkX, chunkY, chunkZ);
        mesh(build.opaque.Take(), true);
        mesh(build.cutout.Take(), true);
        mesh(build.transparent.Take(), false);
        mesh(build.grass.Take(), false);
        for (auto& relief : build.reliefs)
        {
            value(relief.key);
            value(ReliefSignature(relief.cells));
            mesh(relief.cubes.Take(), true);
            mesh(relief.grass.Take(), false);
            mesh(relief.cutouts.Take(), true);
        }
        for (const auto* list : {&build.wind, &build.water, &build.lava, &build.blitz, &build.anim})
        {
            value(list->size());
            for (const auto& b : *list)
            {
                value(b.x); value(b.y); value(b.z); value(b.base); value(b.lavaSides); value(b.rotated); value(b.waterFaces);
            }
        }
        value(build.modelCells.size());
        for (const auto& c : build.modelCells)
        {
            value(c.x); value(c.y); value(c.z); value(c.base); value(c.sameSides); value(c.openSide);
        }
        value(build.blocks);
        value(build.ragged);
        value(build.sumX);
        value(build.sumY);
        value(build.sumZ);
        return hash;
    }

    std::unique_ptr<GpuMeshParts> TerrainRenderer::RebuildAnimatedRenderer(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
        const std::vector<AnimBlock>& blocks, int animPhase) const
    {
        // Plates and special geometry go into `mesh` as the loop meets them;
        // plain cubes collect in `cubes` and follow all of them in the
        // buffer (see the floor cube's comment below for why that order
        // matters).
        auto mesh = MakeLitTexturedMeshBuilder();
        auto cubes = MakeLitTexturedMeshBuilder();
        const auto icon107Uv = m_tileAtlas->GetTileUv(107);
        for (const auto& block : blocks)
        {
            if (std::find(m_modelBases.begin(), m_modelBases.end(), block.base) != m_modelBases.end())
            {
                continue; // drawn as a model by the caller (SetModelDrawnBases)
            }
            const auto& definition = GalaxyEggbert::GetBlockDefinition(block.base);
            const int icon = AnimIconAt(block.base, animPhase, block.x, block.y, block.z, m_blupiFocused);
            if (icon < 0)
            {
                // Temp tile's invisible frames — omit the cube entirely.
                continue;
            }
            const Microsoft::Xna::Framework::Vector3 center(block.x, block.y, block.z);
            const auto tileUv = m_tileAtlas->GetTileUv(icon);
            // Recover raw grid coordinates (block.x/y/z are already
            // world-space, offset by -kWorldCenterX/Z) for the rotation
            // metadata lookup -- same packed key the constructor built
            // m_rotatedPlatePositions with.
            const int rawX = std::lround(block.x) + kWorldCenterX;
            const int rawY = std::lround(block.y);
            const int rawZ = std::lround(block.z) + kWorldCenterZ;
            const bool plateRotated = m_rotatedPlatePositions.count(PackPlatePositionKey(rawX, rawY, rawZ)) != 0;

            // Looked up by block.base (the animation group's fixed base
            // icon, e.g. FanLeft=126), NOT by icon (the current frame, e.g.
            // 126/127/128) -- a special-geometry table only has an entry for
            // the base icon, and the render mode/face pattern must stay
            // constant across the animation; only tileUv (the
            // actually-sampled texture) should change frame to frame. Found
            // 2026-07-08 via a live block/vertex-count mismatch on the fan
            // demo block: without this, fan blocks always fell through to a
            // plain untextured-on-every-face UniformCube and their
            // DirectionalCubeTiles entry was silently dead code.
            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::SawBlade)
            {
                if (std::find(m_sawModelBases.begin(),m_sawModelBases.end(),block.base)==m_sawModelBases.end())
                {
                    auto upperUv=tileUv;
                    upperUv.v1=(tileUv.v0+tileUv.v1)*.5f;
                    constexpr float height=.466f;
                    mesh.AddPlate({block.x,block.y-.5f+height*.5f,block.z},kInnerFlatPlateWidth,height,
                        plateRotated ? CnaEngine::Graphics::PlateAxis::X : CnaEngine::Graphics::PlateAxis::Z,upperUv);
                }
                continue;
            }

            if (AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated,
                                      mesh))
            {
                continue;
            }

            // The lightning (Q-002): thin tubes along the frame's painted bolt, not a cube of it.
            if (block.base == GalaxyEggbert::BlockTypes::Blitz)
            {
                AppendBoltCell(mesh, center, icon - GalaxyEggbert::BlockTypes::Blitz, tileUv,
                               BoltCellYaw(rawX, rawY, rawZ));
                continue;
            }

            // Lava (VIS-0069): a pool with the original frames' wave, not a cube of fire.
            if (block.base == GalaxyEggbert::BlockTypes::Lava)
            {
                LavaNeighbours beside;
                beside.posX = (block.lavaSides & 1) != 0;
                beside.negX = (block.lavaSides & 2) != 0;
                beside.posZ = (block.lavaSides & 4) != 0;
                beside.negZ = (block.lavaSides & 8) != 0;
                beside.below = (block.lavaSides & 16) != 0;
                AppendLavaCell(mesh, center, icon - GalaxyEggbert::BlockTypes::Lava, tileUv, beside);
                continue;
            }

            if (definition.water)
            {
                CnaEngine::Graphics::BoxFaces faces;
                for (std::size_t face = 0; face < faces.size(); ++face)
                    faces[face] = {(block.waterFaces & (1u << face)) != 0, tileUv};
                AddCenteredBox(cubes, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), faces);
            }
            else
            {
                AddCenteredBox(cubes, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), tileUv);
            }
        }

        if (cubes.Mesh().Empty() && mesh.Mesh().Empty())
        {
            // Every block in this group is in a hidden frame this phase
            // (e.g. Temp's 2 blank frames out of 20) — avoid constructing a
            // zero-size GPU buffer; caller draws nothing until the next
            // non-empty phase.
            return nullptr;
        }

        CnaEngine::Graphics::MeshData combined = mesh.Take();
        combined.Append(cubes.Mesh());
        return UploadLitTexturedMeshParts(device, combined);
    }

    void TerrainRenderer::Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase,
                                 bool blupiFocused)
    {
        if (blupiFocused != m_blupiFocused)
        {
            m_blupiFocused = blupiFocused;
            m_lastAnimPhase = -1; // the traps change their cycle now
        }
        if (animPhase == m_lastAnimPhase ||
            (m_animBlocks.empty() && m_waterBlocks.empty() && m_lavaBlocks.empty() && m_blitzBlocks.empty() &&
             m_windBlocks.empty()))
        {
            m_lastAnimPhase = animPhase;
            return;
        }
        m_lastAnimPhase = animPhase;

        if (!m_animBlocks.empty() || m_animRenderer)
        {
            m_animRenderer = RebuildAnimatedRenderer(device, m_animBlocks, animPhase);
        }
        if (!m_waterBlocks.empty() || m_waterRenderer)
        {
            m_waterRenderer = RebuildAnimatedRenderer(device, m_waterBlocks, animPhase);
        }
        if (!m_lavaBlocks.empty())
        {
            // Cached by the step of the cycle (each cell its own frame on it, BD-0065).
            const auto& lavaAnimation = GalaxyEggbert::GetBlockDefinition(GalaxyEggbert::BlockTypes::Lava).animation;
            m_lavaIcon = std::max(0, animPhase) / std::max(1, static_cast<int>(lavaAnimation.divisor)) %
                         std::max(1, static_cast<int>(lavaAnimation.frameCount));
            if (m_lavaFrames.find(m_lavaIcon) == m_lavaFrames.end())
            {
                m_lavaFrames[m_lavaIcon] = RebuildAnimatedRenderer(device, m_lavaBlocks, animPhase);
            }
        }
        if (!m_blitzBlocks.empty())
        {
            m_blitzIcon = AnimIcon(GalaxyEggbert::BlockTypes::Blitz, animPhase);
            if (m_blitzFrames.find(m_blitzIcon) == m_blitzFrames.end())
            {
                m_blitzFrames[m_blitzIcon] = RebuildAnimatedRenderer(device, m_blitzBlocks, animPhase);
            }
        }
        if (!m_windBlocks.empty())
        {
            m_windFrame = std::max(0, animPhase) % 4;
            if (!m_windFrames[static_cast<std::size_t>(m_windFrame)])
            {
                m_windFrames[static_cast<std::size_t>(m_windFrame)] = BuildWindFrame(device, m_windFrame);
            }
        }
    }

    std::unique_ptr<GpuMeshParts> TerrainRenderer::BuildWindFrame(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int frame) const
    {
        // Two crossed plates a cell across, both holding the way the wind blows, its streaks along
        // it: a side wind (110 -X, 114 +X; turned, M3D-0005, along Z) on an upright plate and a
        // level one, an updraught or downdraught (118, 122) on two upright plates.
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        auto mesh = MakeLitTexturedMeshBuilder();
        constexpr float h = 0.5f;
        for (const auto& block : m_windBlocks)
        {
            const auto uv = m_tileAtlas->GetTileUv(block.base + frame);
            const Vector3 c(block.x, block.y, block.z);
            const bool side = block.base == 110 || block.base == 114;
            if (side && !block.rotated)
            {
                mesh.AddPlate(c, 1.0f, 1.0f, CnaEngine::Graphics::PlateAxis::Z, uv);
                mesh.AddPlate(c, 1.0f, 1.0f, CnaEngine::Graphics::PlateAxis::Y, uv);
            }
            else if (side)
            {
                mesh.AddPlate(c, 1.0f, 1.0f, CnaEngine::Graphics::PlateAxis::X, uv);
                // Level, its u along +Z (AddPlate's level plate runs u along X).
                mesh.AddDoubleSidedQuad({Vector3(c.X - h, c.Y, c.Z - h), Vector3(c.X - h, c.Y, c.Z + h),
                                         Vector3(c.X + h, c.Y, c.Z + h), Vector3(c.X + h, c.Y, c.Z - h)},
                                        {Vector2(uv.u0, uv.v1), Vector2(uv.u1, uv.v1), Vector2(uv.u1, uv.v0),
                                         Vector2(uv.u0, uv.v0)});
            }
            else
            {
                mesh.AddPlate(c, 1.0f, 1.0f, CnaEngine::Graphics::PlateAxis::Z, uv);
                mesh.AddPlate(c, 1.0f, 1.0f, CnaEngine::Graphics::PlateAxis::X, uv);
            }
        }
        return mesh.Mesh().Empty() ? nullptr : UploadLitTexturedMeshParts(device, mesh.Mesh());
    }

    TerrainRenderer::ReliefMeshData TerrainRenderer::BuildReliefMesh(const TerrainReliefTable& reliefs, const TileAtlas& atlas,
                                                                     bool overhangs, const std::vector<ReliefCell>& cells)
    {
        auto litMesh = MakeLitTexturedMeshBuilder();
        auto cutMesh = MakeLitTexturedMeshBuilder();
        for (const auto& cell : cells)
        {
            auto& mesh = cell.cutout ? cutMesh : litMesh;
            const TerrainRelief* relief = reliefs.Find(cell.icon);
            if (relief == nullptr)
            {
                continue;
            }
            const auto tile = atlas.GetTileUv(reliefs.TextureIcon(cell.icon));
            if (cell.flatFaces != 0)
            {
                CnaEngine::Graphics::BoxFaces flat;
                for (int face = 0; face < 6; ++face)
                {
                    flat[face].visible = (cell.flatFaces & (1u << face)) != 0;
                    flat[face].uv = cell.flatUv[static_cast<std::size_t>(face)];
                }
                AddCenteredBox(mesh, cell.center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), flat);
            }
            for (int face = 0; face < 6; ++face)
            {
                if ((cell.faces & (1u << face)) == 0)
                {
                    continue;
                }
                if (cell.grass && face == 5)
                {
                    // The grass's underside: its soil's colour (the sprite's bottom rows), flat.
                    const float u = tile.u0 + 0.5f * (tile.u1 - tile.u0);
                    const float v = tile.v0 + 0.92f * (tile.v1 - tile.v0);
                    CnaEngine::Graphics::BoxFaces bottom;
                    for (int k = 0; k < 6; ++k)
                    {
                        bottom[k].visible = k == 5;
                        bottom[k].uv = CnaEngine::Graphics::UvRect{u, v, u, v};
                    }
                    AddCenteredBox(mesh, cell.center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), bottom);
                    continue;
                }
                if (cell.plate)
                {
                    // From the cell's middle (AppendReliefFace lays a relief half a block out).
                    const auto& f = kReliefFaceFrames[static_cast<std::size_t>(face)];
                    const Microsoft::Xna::Framework::Vector3 n(static_cast<float>(f.n[0]), static_cast<float>(f.n[1]),
                                                               static_cast<float>(f.n[2]));
                    AppendReliefFace(mesh, *relief, cell.center - n * 0.5f, face, tile, 0, false);
                    continue;
                }
                AppendReliefFace(mesh, *relief, cell.center, face, tile,
                                 overhangs ? std::uint8_t{0} : cell.air[static_cast<std::size_t>(face)],
                                 !cell.grass && !cell.cutout);
            }
        }
        return ReliefMeshData{litMesh.Take(), cutMesh.Take()};
    }

    std::uint64_t TerrainRenderer::ReliefSignature(const std::vector<ReliefCell>& cells)
    {
        // FNV-1a over what a cell's relief is made of.
        std::uint64_t hash = 1469598103934665603ull;
        const auto mix = [&hash](const void* data, std::size_t size)
        {
            const auto* bytes = static_cast<const unsigned char*>(data);
            for (std::size_t k = 0; k < size; ++k)
            {
                hash = (hash ^ bytes[k]) * 1099511628211ull;
            }
        };
        for (const auto& cell : cells)
        {
            const float centre[3] = {cell.center.X, cell.center.Y, cell.center.Z};
            mix(centre, sizeof(centre));
            mix(&cell.icon, sizeof(cell.icon));
            mix(&cell.faces, sizeof(cell.faces));
            mix(cell.air.data(), cell.air.size());
            const unsigned char kind =
                static_cast<unsigned char>((cell.grass ? 1 : 0) | (cell.plate ? 2 : 0) | (cell.cutout ? 4 : 0));
            mix(&kind, 1);
            mix(&cell.flatFaces, sizeof(cell.flatFaces));
            for (const auto& uv : cell.flatUv)
            {
                const float rect[4] = {uv.u0, uv.v0, uv.u1, uv.v1};
                mix(rect, sizeof(rect));
            }
        }
        return hash;
    }

    void TerrainRenderer::AdoptReliefs(TerrainRenderer& previous)
    {
        if (m_reliefs == nullptr || previous.m_reliefs != m_reliefs || previous.m_reliefOverhangs != m_reliefOverhangs)
        {
            return;
        }
        for (auto& chunk : m_reliefChunks)
        {
            for (auto& old : previous.m_reliefChunks)
            {
                if (old.boxMin.X == chunk.boxMin.X && old.boxMin.Y == chunk.boxMin.Y && old.boxMin.Z == chunk.boxMin.Z)
                {
                    if (old.signature == chunk.signature)
                    {
                        chunk.relief = std::move(old.relief);
                        chunk.pending = std::move(old.pending);
                    }
                    break;
                }
            }
        }
    }

    void TerrainRenderer::UpdateRelief(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                       const Microsoft::Xna::Framework::Vector3& focus, float radius, int maxBuilds)
    {
        if (m_reliefs == nullptr)
        {
            return;
        }
        const auto upload = [&device](ReliefMeshData data) -> std::unique_ptr<ReliefMeshes>
        {
            auto meshes = std::make_unique<ReliefMeshes>();
            if (!data.lit.Empty())
            {
                meshes->lit = UploadLitTexturedMeshParts(device, data.lit);
            }
            if (!data.cut.Empty())
            {
                meshes->cut = UploadBakedLitTexturedMeshParts(device, std::move(data.cut));
            }
            return meshes;
        };
        // Nearest chunks first, so a capped call builds what is closest.
        std::vector<std::pair<float, std::size_t>> wanted;
        int inFlight = 0;
        for (std::size_t i = 0; i < m_reliefChunks.size(); ++i)
        {
            auto& chunk = m_reliefChunks[i];
            const float dx = std::max({chunk.boxMin.X - focus.X, 0.0f, focus.X - chunk.boxMax.X});
            const float dy = std::max({chunk.boxMin.Y - focus.Y, 0.0f, focus.Y - chunk.boxMax.Y});
            const float dz = std::max({chunk.boxMin.Z - focus.Z, 0.0f, focus.Z - chunk.boxMax.Z});
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            chunk.near = distance <= radius;
            if (chunk.pending.valid())
            {
                if (chunk.pending.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
                {
                    ++inFlight;
                    continue;
                }
                auto data = chunk.pending.get();
                if (distance <= radius + kReliefRelease)
                {
                    chunk.relief = upload(std::move(data));
                }
            }
            if (chunk.near && !chunk.relief && !chunk.cells.empty())
            {
                wanted.emplace_back(distance, i);
            }
            else if (!chunk.near && chunk.relief && distance > radius + kReliefRelease)
            {
                chunk.relief.reset();
            }
        }
        std::sort(wanted.begin(), wanted.end());
        if (maxBuilds < 0)
        {
            // All of them now (a world just loaded), kLoadThreads at a time where there are threads.
#if defined(__EMSCRIPTEN__)
            for (const auto& entry : wanted)
            {
                auto& chunk = m_reliefChunks[entry.second];
                chunk.relief = upload(BuildReliefMesh(*m_reliefs, *m_tileAtlas, m_reliefOverhangs, chunk.cells));
            }
#else
            constexpr std::size_t kLoadThreads = 4;
            for (std::size_t first = 0; first < wanted.size(); first += kLoadThreads)
            {
                std::vector<std::future<ReliefMeshData>> batch;
                for (std::size_t k = first; k < wanted.size() && k < first + kLoadThreads; ++k)
                {
                    batch.push_back(std::async(std::launch::async, &TerrainRenderer::BuildReliefMesh,
                                               std::cref(*m_reliefs), std::cref(*m_tileAtlas), m_reliefOverhangs,
                                               std::cref(m_reliefChunks[wanted[k].second].cells)));
                }
                for (std::size_t k = 0; k < batch.size(); ++k)
                {
                    m_reliefChunks[wanted[first + k].second].relief = upload(batch[k].get());
                }
            }
#endif
            return;
        }
        for (std::size_t k = 0; k < wanted.size(); ++k)
        {
            auto& chunk = m_reliefChunks[wanted[k].second];
#if defined(__EMSCRIPTEN__)
            if (static_cast<int>(k) >= maxBuilds)
            {
                break;
            }
            chunk.relief = upload(BuildReliefMesh(*m_reliefs, *m_tileAtlas, m_reliefOverhangs, chunk.cells));
#else
            if (inFlight >= kReliefBuildsInFlight)
            {
                break;
            }
            // The task holds its own copy of the cells; the table and the atlas outlive the renderer,
            // whose destruction waits for the task (the future's).
            chunk.pending = std::async(std::launch::async, &TerrainRenderer::BuildReliefMesh, std::cref(*m_reliefs),
                                       std::cref(*m_tileAtlas), m_reliefOverhangs, chunk.cells);
            ++inFlight;
#endif
        }
    }

    int TerrainRenderer::ReliefChunksShown() const noexcept
    {
        int shown = 0;
        for (const auto& chunk : m_reliefChunks)
        {
            shown += chunk.near && chunk.relief ? 1 : 0;
        }
        return shown;
    }

    int TerrainRenderer::ReliefPrimitiveCount() const noexcept
    {
        int count = 0;
        for (const auto& chunk : m_reliefChunks)
        {
            count += chunk.near && chunk.relief ? chunk.relief->PrimitiveCount() : 0;
        }
        return count;
    }

    void TerrainRenderer::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                 Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const
    {
        DrawOpaque(device, effect);
        DrawTransparent(device, effect);
    }

    void TerrainRenderer::DrawOpaque(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                       Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                                       const TerrainShading* shading) const
    {
        Microsoft::Xna::Framework::Graphics::Effect* lavaEffect = shading != nullptr ? shading->lava : nullptr;
        // The static mesh and the far cubes: their occluded shader when the mesh carries the occlusion.
        Microsoft::Xna::Framework::Graphics::Effect& cubesEffect =
            shading != nullptr && shading->occluded != nullptr && m_ambientOcclusion
                ? *shading->occluded
                : static_cast<Microsoft::Xna::Framework::Graphics::Effect&>(effect);
        Microsoft::Xna::Framework::Graphics::Effect& reliefEffect =
            shading != nullptr && shading->lit != nullptr ? *shading->lit
                                                         : static_cast<Microsoft::Xna::Framework::Graphics::Effect&>(effect);
        // The recovered lava pool samples only the tile's solid body (AppendLavaCell).
        // Write its depth before models, including translucent ghost/Hide Blupi: drawing
        // lava in the final blended pass painted pools behind him over his silhouette.
        const auto lava = m_lavaFrames.find(m_lavaIcon);
        if (lava != m_lavaFrames.end() && lava->second)
        {
            lava->second->Draw(device, lavaEffect != nullptr ? *lavaEffect : effect);
        }
        DrawChunks(device, &StaticChunk::opaque, cubesEffect);
        const bool reliefGi = shading != nullptr && shading->reliefGi;
        for (const auto& chunk : m_reliefChunks)
        {
            if (chunk.near && chunk.relief)
            {
                if (chunk.relief->lit)
                {
                    if (reliefGi) shading->reliefGi(chunk.gi.get(), chunk.boxMin);
                    chunk.relief->lit->Draw(device, reliefEffect);
                }
            }
            else if (chunk.cubes)
            {
                chunk.cubes->Draw(device, cubesEffect);
            }
        }
        if (reliefGi) shading->reliefGi(nullptr, Microsoft::Xna::Framework::Vector3());

        if (m_cutoutEffect && shading != nullptr && shading->cutout != nullptr)
        {
            // The effects' cutout shader (it tests the alpha itself).
            DrawChunks(device, &StaticChunk::cutout, *shading->cutout);
            for (const auto& chunk : m_reliefChunks)
            {
                if (chunk.near && chunk.relief)
                {
                    if (chunk.relief->cut)
                    {
                        chunk.relief->cut->Draw(device, *shading->cutout);
                    }
                }
                else if (chunk.cutouts)
                {
                    chunk.cutouts->Draw(device, *shading->cutout);
                }
            }
        }
        else if (m_cutoutEffect)
        {
            // Opaque where drawn, so it writes depth like the static mesh;
            // the clear texels are discarded by the alpha test.
            m_cutoutEffect->setWorldProperty(effect.getWorldProperty());
            m_cutoutEffect->setViewProperty(effect.getViewProperty());
            m_cutoutEffect->setProjectionProperty(effect.getProjectionProperty());
            m_cutoutEffect->setTextureProperty(effect.getTextureProperty());
            // And its fog (the effects layer's; off otherwise).
            m_cutoutEffect->setFogEnabledProperty(effect.getFogEnabledProperty());
            if (effect.getFogEnabledProperty())
            {
                m_cutoutEffect->setFogColorProperty(effect.getFogColorProperty());
                m_cutoutEffect->setFogStartProperty(effect.getFogStartProperty());
                m_cutoutEffect->setFogEndProperty(effect.getFogEndProperty());
            }
            DrawChunks(device, &StaticChunk::cutout, *m_cutoutEffect);
            for (const auto& chunk : m_reliefChunks)
            {
                if (chunk.near && chunk.relief)
                {
                    if (chunk.relief->cut)
                    {
                        chunk.relief->cut->Draw(device, *m_cutoutEffect);
                    }
                }
                else if (chunk.cutouts)
                {
                    chunk.cutouts->Draw(device, *m_cutoutEffect);
                }
            }
        }
    }

    void TerrainRenderer::DrawTransparent(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                            Microsoft::Xna::Framework::Graphics::BasicEffect& effect,
                                            const TerrainShading* shading) const
    {
        Microsoft::Xna::Framework::Graphics::Effect* waterEffect = shading != nullptr ? shading->water : nullptr;
        const auto blitz = m_blitzFrames.find(m_blitzIcon);
        const GpuMeshParts* blitzRenderer = blitz != m_blitzFrames.end() ? blitz->second.get() : nullptr;
        const GpuMeshParts* windRenderer =
            m_windFrame >= 0 ? m_windFrames[static_cast<std::size_t>(m_windFrame)].get() : nullptr;
        if (m_animRenderer || m_waterRenderer || AnyChunk(&StaticChunk::transparent) || blitzRenderer ||
            windRenderer)
        {
            // Semi-transparent pass (2026-07-08 design decision, see
            // NEXT.md §8, extended 2026-07-09 to also cover m_animRenderer):
            // drawn last, with alpha blending and depth WRITES disabled (but
            // depth TESTING still on, via DepthRead) so transparent geometry
            // composites correctly over opaque terrain already in the depth
            // buffer without blocking whatever's drawn after it. object-m.png
            // has real per-pixel alpha (confirmed 2026-07-08); NonPremultiplied
            // matches its un-premultiplied RGB. m_animRenderer (crusher/
            // saw/spike/fan/marine/temp) moved into this pass 2026-07-09 --
            // it used to draw opaque like m_staticRenderer, which was wrong:
            // direct pixel sampling of object-m.png shows these tiles'
            // textures are 44-81% transparent pixels (not the near-opaque
            // case icons 30/31 originally motivated this pass for), so
            // drawing them opaque rendered most of each tile's transparent
            // background as solid black -- reported live as "instead of
            // transparency there's black, you can see inside the cube."
            // The recovered lava pool now belongs to DrawOpaque, since its geometry
            // samples only solid texels instead of the sprite's clear background.
            // Covers water (animated), the static-but-transparent icons
            // 30/31, AND the animated non-water set -- same blend/depth
            // state for all three, so one state change covers all draws.
            // State is restored to Opaque/Default afterward so the caller's
            // own state isn't disturbed.
            using Microsoft::Xna::Framework::Graphics::BlendState;
            using Microsoft::Xna::Framework::Graphics::DepthStencilState;
            device.setBlendStateProperty(BlendState::NonPremultiplied);
            device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
            DrawChunks(device, &StaticChunk::transparent, effect);
            if (m_animRenderer)
            {
                m_animRenderer->Draw(device, effect);
            }
            if (blitzRenderer)
            {
                blitzRenderer->Draw(device, effect);
            }
            if (windRenderer)
            {
                windRenderer->Draw(device, effect);
            }
            if (m_waterRenderer)
            {
                m_waterRenderer->Draw(device, waterEffect != nullptr ? *waterEffect : effect);
            }
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }
    }

    void TerrainRenderer::DrawGeometryPass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                           Microsoft::Xna::Framework::Graphics::Effect& colored,
                                           Microsoft::Xna::Framework::Graphics::Effect& plain,
                                           Microsoft::Xna::Framework::Graphics::Effect& cutout) const
    {
        // As DrawShadowCasters, each mesh with the program of its vertices: the baked-colour meshes
        // (with the occlusion) `colored`, the others `plain`.
        auto& baked = m_ambientOcclusion ? colored : plain;
        const auto lava = m_lavaFrames.find(m_lavaIcon);
        if (lava != m_lavaFrames.end() && lava->second)
        {
            lava->second->Draw(device, plain);
        }
        DrawChunks(device, &StaticChunk::opaque, baked);
        for (const auto& chunk : m_reliefChunks)
        {
            if (chunk.near && chunk.relief)
            {
                if (chunk.relief->lit)
                {
                    chunk.relief->lit->Draw(device, plain);
                }
                if (chunk.relief->cut)
                {
                    chunk.relief->cut->Draw(device, cutout);
                }
            }
            else
            {
                if (chunk.cubes)
                {
                    chunk.cubes->Draw(device, baked);
                }
                if (chunk.cutouts)
                {
                    chunk.cutouts->Draw(device, cutout);
                }
            }
        }
        DrawChunks(device, &StaticChunk::cutout, cutout);
    }

    void TerrainRenderer::DrawWater(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                    Microsoft::Xna::Framework::Graphics::Effect& effect) const
    {
        if (m_waterRenderer)
        {
            m_waterRenderer->Draw(device, effect);
        }
    }

    void TerrainRenderer::DrawShadowCasters(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                            Microsoft::Xna::Framework::Graphics::Effect& solid,
                                            Microsoft::Xna::Framework::Graphics::Effect& cutout) const
    {
        const auto lava = m_lavaFrames.find(m_lavaIcon);
        if (lava != m_lavaFrames.end() && lava->second)
        {
            lava->second->Draw(device, solid);
        }
        DrawChunks(device, &StaticChunk::opaque, solid);
        for (const auto& chunk : m_reliefChunks)
        {
            if (chunk.near && chunk.relief)
            {
                if (chunk.relief->lit)
                {
                    chunk.relief->lit->Draw(device, solid);
                }
                if (chunk.relief->cut)
                {
                    chunk.relief->cut->Draw(device, cutout);
                }
            }
            else
            {
                if (chunk.cubes)
                {
                    chunk.cubes->Draw(device, solid);
                }
                if (chunk.cutouts)
                {
                    chunk.cutouts->Draw(device, cutout);
                }
            }
        }
        DrawChunks(device, &StaticChunk::cutout, cutout);
    }

    std::vector<Microsoft::Xna::Framework::Vector3> TerrainRenderer::LavaCells() const
    {
        std::vector<Microsoft::Xna::Framework::Vector3> cells;
        cells.reserve(m_lavaBlocks.size());
        for (const auto& block : m_lavaBlocks)
        {
            cells.emplace_back(block.x, block.y, block.z);
        }
        return cells;
    }

    std::vector<Microsoft::Xna::Framework::Vector3> TerrainRenderer::BlitzCells() const
    {
        std::vector<Microsoft::Xna::Framework::Vector3> cells;
        cells.reserve(m_blitzBlocks.size());
        for (const auto& block : m_blitzBlocks)
        {
            cells.emplace_back(block.x, block.y, block.z);
        }
        return cells;
    }

    void TerrainRenderer::DrawGrass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                      Microsoft::Xna::Framework::Graphics::Effect& grassEffect) const
    {
        DrawChunks(device, &StaticChunk::grass, grassEffect);
        for (const auto& chunk : m_reliefChunks)
        {
            if (chunk.grassTops && !(chunk.near && chunk.relief))
            {
                chunk.grassTops->Draw(device, grassEffect);
            }
        }
    }

    int TerrainRenderer::VertexCount() const noexcept
    {
        int count = 0;
        for (const auto& chunk : m_staticChunks)
        {
            for (const auto* mesh : {chunk.opaque.get(), chunk.transparent.get(), chunk.cutout.get(), chunk.grass.get()})
            {
                count += mesh != nullptr ? mesh->VertexCount() : 0;
            }
        }
        return count +
               (m_animRenderer ? m_animRenderer->VertexCount() : 0) +
               (m_lavaFrames.count(m_lavaIcon) != 0 && m_lavaFrames.at(m_lavaIcon) ? m_lavaFrames.at(m_lavaIcon)->VertexCount() : 0) +
               (m_waterRenderer ? m_waterRenderer->VertexCount() : 0) +
               ReliefCubeCount(m_reliefChunks, true);
    }

    int TerrainRenderer::PrimitiveCount() const noexcept
    {
        int count = 0;
        for (const auto& chunk : m_staticChunks)
        {
            for (const auto* mesh : {chunk.opaque.get(), chunk.transparent.get(), chunk.cutout.get(), chunk.grass.get()})
            {
                count += mesh != nullptr ? mesh->PrimitiveCount() : 0;
            }
        }
        return count +
               (m_animRenderer ? m_animRenderer->PrimitiveCount() : 0) +
               (m_lavaFrames.count(m_lavaIcon) != 0 && m_lavaFrames.at(m_lavaIcon) ? m_lavaFrames.at(m_lavaIcon)->PrimitiveCount() : 0) +
               (m_waterRenderer ? m_waterRenderer->PrimitiveCount() : 0) +
               ReliefCubeCount(m_reliefChunks, false);
    }
}
