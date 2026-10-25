#include "TerrainRenderer.hpp"
#include "DirectionalCubeTiles.hpp"
#include "InnerFlatPlateTiles.hpp"
#include "InnerPillarBoxTiles.hpp"
#include "ThinBarTiles.hpp"
#include "WorldRuntime.hpp"

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>

#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/CompareFunction.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>

#include <cmath>

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

        // InnerFlatPlate/TripleCrossBillboard geometry sizing — "spans most
        // of the block" per the questionnaire wording, leaving a visible
        // margin so the plate/cross doesn't clip through neighboring blocks'
        // faces.
        constexpr float kInnerFlatPlateWidth = 0.75f;
        constexpr float kInnerFlatPlateHeight = 0.9f;
        constexpr float kTripleCrossWidth = 0.8f;
        constexpr float kTripleCrossHeight = 1.0f;

        // Saw/SawStopped: 5 rounds of live user feedback, all 2026-07-11
        // except the last two (2026-07-20). Several further rounds the
        // same day (exact 180-degree in-plane rotation, size, and
        // position/offset tweaks) were tried and explicitly reverted --
        // this 5-round state is the confirmed-good one; do not re-attempt
        // those without new, explicit user direction.
        // (1) bottom-anchored vertical plate -- sank below the visible
        //     floor line, read as buried/cutting into the ground;
        // (2) top-anchored vertical plate, hanging down from the block's
        //     top face -- fixed the sinking, but neither round questioned
        //     the VERTICAL axis itself;
        // (3) a live screenshot showed the vertical plate as a thin
        //     diagonal wedge jammed into the corridor's side wall, clearly
        //     wrong -- read at the time as "should lie FLAT on the floor
        //     instead" (PlateAxis::Y);
        // (4) making it an EXCLUSIVE InnerFlatPlate (hollow block, no solid
        //     cube at all -- the same category every other InnerFlatPlate
        //     icon uses) then read as a hole/pit in the floor, since
        //     nothing filled the gap where an ordinary solid floor block
        //     would be -- fixed additively (a normal solid floor cube PLUS
        //     the blade as an overlay, same idiom as the registry's grass-top
        //     grass-top overlay just below in the main Build() loop);
        // (5) with the hole fixed, the user's final word against THIS
        //     round's own screenshot: the blade should be upright again
        //     after all (round 3's actual problem was being hidden in a
        //     hole/wrong-looking, not the vertical axis itself) -- rooted
        //     AT the floor (bottom edge at the walkable surface, extending
        //     UP), same idiom as every other InnerFlatPlate icon's vertical
        //     default, still additive on top of the solid floor cube from
        //     round 4 (not reverting to an exclusive/hollow InnerFlatPlate).
        // The solid floor cube uses the standard RockPile tile (NOT the
        // confusingly-named Ground/icon 10, a machine-piece graphic per
        // BlockTypes.hpp's own NOTE -- no per-placement floor-texture
        // concept exists in the world format either way).
        // Shifts the underlying floor cube DOWN so its own top face sits
        // strictly clear of the blade plate's bottom edge (real,
        // unambiguous geometry, not same-buffer depth-test tie-breaking --
        // see RebuildAnimatedRenderer()'s own comment for why that
        // mattered when the plate was still horizontal).
        constexpr float kSawPlateEpsilon = 0.02f;

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
                                   CnaEngine::Graphics::MeshBuilder& mesh)
        {
            using CnaEngine::Graphics::BoxFace;
            const int lookupIcon = static_cast<int>(definition.type);
            CnaEngine::Graphics::BoxFaces directionalFaces;
            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::DirectionalCube)
            {
                ConfigureDirectionalCubeFaces(
                    lookupIcon, tileUv, icon107Uv, directionalFaces);

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

        // True if @p icon uses one of the "new geometry" render modes
        // (AppendSpecialGeometry above) -- these are smaller than a full
        // block or leave faces intentionally open, so they must never be
        // treated as occluders by IsOccluderBlock below.
        bool IsSpecialGeometryIcon(int icon)
        {
            const auto mode =
                GalaxyEggbert::GetBlockDefinition(static_cast<std::uint16_t>(icon)).renderMode;
            return mode != GalaxyEggbert::BlockRenderMode::Air &&
                   mode != GalaxyEggbert::BlockRenderMode::UniformCube;
        }

        // Face-culling occlusion test (NEXT.md §8 -- "add face-culling/
        // occlusion to TerrainRenderer"): true only for a block that is
        // guaranteed to be a plain, fully opaque 1x1x1 cube on every side --
        // non-air, in bounds, not water, not animated (some animated icons
        // use special/holed geometry -- see AppendSpecialGeometry -- and
        // conservatively excluding ALL of them, not just the holed ones,
        // keeps this check simple and never over-culls), not alpha-blended,
        // and not itself a special-geometry icon. Deliberately conservative:
        // a face is only culled when its neighbor is DEFINITELY solid on
        // that side, never a guess -- worth revisiting later for e.g.
        // DirectionalCube neighbors (their own per-face pattern is already
        // known via ConfigureDirectionalCubeFaces, so they COULD occlude on
        // their opaque faces), but that's extra complexity for a case this
        // task doesn't need yet (§8's own note: "not needed at the current
        // ~2700-block scale"). A tile with clear texels (TileCoverage) hides
        // nothing behind it either.
        bool IsOccluderBlock(const Worlds::World& world, int blocksPerAxis, int x, int y, int z,
                             const std::vector<TileCoverage>& tileCoverage)
        {
            if (x < 0 || y < 0 || z < 0 || x >= blocksPerAxis || y >= blocksPerAxis || z >= blocksPerAxis)
            {
                return false;
            }

            const auto block = world.getBlock(static_cast<std::uint16_t>(x),
                                               static_cast<std::uint16_t>(y),
                                               static_cast<std::uint16_t>(z));
            if (block.isAir())
            {
                return false;
            }

            const auto& definition = GalaxyEggbert::GetBlockDefinition(block.type());
            const std::uint16_t animBase = definition.animation.baseIcon;
            if (IsWater(animBase) || IsAnimated(animBase))
            {
                return false;
            }

            const int icon = static_cast<int>(block.type());
            if (definition.alphaBlend || IsSpecialGeometryIcon(icon))
            {
                return false;
            }

            return CoverageOf(tileCoverage, icon) == TileCoverage::Opaque;
        }
    }

    TerrainRenderer::TerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Worlds::World& world,
                                         const TileAtlas& tileAtlas,
                                         const std::vector<TileCoverage>& tileCoverage)
        : m_tileAtlas(&tileAtlas)
    {
        // Icons 108/109's DirectionalCube entry reuses icon 107's own side
        // texture (DirectionalCubeTiles.cpp) -- computed once here since
        // it never changes, threaded through AppendSpecialGeometry.
        const auto icon107Uv = tileAtlas.GetTileUv(107);
        for (const auto& pos : CollectRotatedPlatePositions(world))
        {
            m_rotatedPlatePositions.insert(PackPlatePositionKey(pos[0], pos[1], pos[2]));
        }
        auto staticMesh = MakeLitTexturedMeshBuilder();
        auto transparentStaticMesh = MakeLitTexturedMeshBuilder();
        auto cutoutMesh = MakeLitTexturedMeshBuilder();
        auto grassMesh = MakeLitTexturedMeshBuilder();
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

                    const auto& definition = GalaxyEggbert::GetBlockDefinition(block.type());
                    const std::uint16_t animBase = definition.animation.baseIcon;
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

                    const Microsoft::Xna::Framework::Vector3 center(worldX, worldY, worldZ);
                    const int icon = static_cast<int>(block.type());
                    const auto tileUv = tileAtlas.GetTileUv(icon);
                    const bool plateRotated = m_rotatedPlatePositions.count(PackPlatePositionKey(x, y, z)) != 0;

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
                    // Note: Saw/SawStopped (GroundAnchoredPlate mode) never
                    // reaches this point -- both are always animated
                    // (IsAnimated(Saw) above), so they're diverted into
                    // m_animBlocks earlier in this same loop; their ground-
                    // anchored-overlay handling lives in
                    // RebuildAnimatedRenderer() instead (see
                    // GroundAnchoredPlate mode's own comment above and that
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
                    if (AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated, *mesh))
                    {
                        continue;
                    }

                    using CnaEngine::Graphics::BoxFace;
                    CnaEngine::Graphics::BoxFaces faces;
                    faces[static_cast<int>(BoxFace::PositiveZ)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y, z + 1, tileCoverage), tileUv};
                    faces[static_cast<int>(BoxFace::NegativeZ)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y, z - 1, tileCoverage), tileUv};
                    faces[static_cast<int>(BoxFace::PositiveX)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x + 1, y, z, tileCoverage), tileUv};
                    faces[static_cast<int>(BoxFace::NegativeX)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x - 1, y, z, tileCoverage), tileUv};
                    faces[static_cast<int>(BoxFace::PositiveY)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y + 1, z, tileCoverage), tileUv};
                    faces[static_cast<int>(BoxFace::NegativeY)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y - 1, z, tileCoverage), tileUv};
                    AddCenteredBox(*mesh, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), faces);
                }
            }
        }

        if (m_blockCount > 0)
        {
            m_centroidX = static_cast<float>(sumX / m_blockCount);
            m_centroidY = static_cast<float>(sumY / m_blockCount);
            m_centroidZ = static_cast<float>(sumZ / m_blockCount);
        }

        // Guarded the same way m_transparentStaticRenderer/m_grassRenderer
        // already are below (found 2026-07-18, plan.md EDITOR-107): a
        // genuinely empty (all-air) World -- e.g. the editor's brand-new
        // "New World" -- has zero static blocks; easy-3d's mesh renderer then
        // drew with primitiveCount 0, which CNA rejects with an
        // ArgumentOutOfRangeException, and GpuMesh::Upload rejects an empty
        // mesh outright. Every hand-authored
        // .vwr world before the editor existed always had at least some
        // terrain, so this was never reachable until now.
        if (!staticMesh.Mesh().Empty())
        {
            m_staticRenderer = UploadLitTexturedMesh(device, staticMesh.Mesh());
        }

        if (!cutoutMesh.Mesh().Empty())
        {
            m_cutoutRenderer = UploadBakedLitTexturedMesh(device, cutoutMesh.Take());
            m_cutoutEffect = std::make_unique<Microsoft::Xna::Framework::Graphics::AlphaTestEffect>(device);
            m_cutoutEffect->setVertexColorEnabledProperty(true);
            m_cutoutEffect->setAlphaFunctionProperty(
                Microsoft::Xna::Framework::Graphics::CompareFunction::GreaterEqual);
            m_cutoutEffect->setReferenceAlphaProperty(kCutoutReferenceAlpha);
        }

        if (!transparentStaticMesh.Mesh().Empty())
        {
            m_transparentStaticRenderer =
                UploadLitTexturedMesh(device, transparentStaticMesh.Mesh());
        }

        if (!grassMesh.Mesh().Empty())
        {
            m_grassRenderer = UploadLitTexturedMesh(device, grassMesh.Mesh());
        }

        Update(device, 0);
    }

    std::unique_ptr<CnaEngine::Graphics::GpuMesh> TerrainRenderer::RebuildAnimatedRenderer(
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
            const auto& definition = GalaxyEggbert::GetBlockDefinition(block.base);
            const int icon = AnimIcon(block.base, animPhase);
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
            if (definition.renderMode == GalaxyEggbert::BlockRenderMode::GroundAnchoredPlate)
            {
                // Saw/SawStopped (2026-07-20): animated (378-383 cycle), so
                // this path -- NOT the constructor's main Build() loop
                // (which routes these into m_animBlocks before it would
                // ever reach an equivalent check) -- is what actually
                // renders it; see the constructor's own GroundAnchoredPlate
                // comment block for the full round-by-round history. No
                // face-culling here since RebuildAnimatedRenderer never had
                // any (every animated block already renders all 6 faces via
                // plain `batch.Add()` below) -- consistent with that, the
                // underlying floor cube is likewise always-6-faces-visible.
                //
                // Uses RockPile, NOT Ground (icon 10) -- despite the name,
                // `Ground` is a confirmed machine-piece DirectionalCube
                // graphic (BlockTypes.hpp's own NOTE), not bulk floor
                // material; it rendered as a jarring blue riveted plate
                // here. RockPile is confirmed genuine bulk UniformCube
                // terrain, matching the surrounding demo corridor's own
                // floor. kSawPlateEpsilon shifts the cube's own top face
                // down, strictly clear of the vertical plate's bottom edge
                // (from back when the plate was horizontal and exactly
                // coplanar with the cube's top face, a genuine same-buffer
                // depth-test tie; kept even now that the plate is vertical
                // again, since a hairline gap there is harmless and
                // removing it would need re-verifying that concern).
                // Vertical again (2026-07-20, 5th round): the user's own
                // "flat on the floor" read of round 3's screenshot turned
                // out to mean "don't hide it in a hole" (round 4's actual
                // fix), not "the blade itself should lie flat" -- final
                // word, against this round's own screenshot: upright,
                // rooted AT the floor (bottom edge at the walkable surface,
                // extending UP), same idiom as every other InnerFlatPlate
                // icon's vertical default. Axis still respects the per-
                // placement rotation flag (PlateRotationMetadata.hpp) so
                // it can face whichever way its own corridor runs. This is
                // the confirmed-good state as of this round -- do not
                // pursue exact-180-degree in-plane rotation or size/offset
                // tweaks again without explicit new user direction; several
                // rounds of that were tried and reverted.
                {
                    constexpr float kSawPlateHeight = 0.6f;
                    mesh.AddPlate(
                        Microsoft::Xna::Framework::Vector3(block.x, block.y + 0.5f + kSawPlateHeight * 0.5f, block.z),
                        kInnerFlatPlateWidth, kSawPlateHeight,
                        plateRotated ? CnaEngine::Graphics::PlateAxis::X : CnaEngine::Graphics::PlateAxis::Z, tileUv);
                }

                // Floor cube shifted down by kSawPlateEpsilon (not the plate
                // shifted up) -- the plain cubes (`cubes`) are only appended
                // to `mesh` AFTER the whole per-block loop finishes, always
                // landing later in the shared vertex
                // buffer than every plate appended during the loop, so an
                // apparent depth-test tie always resolved in the cube's
                // favor regardless of a same-buffer Y epsilon on the plate
                // (confirmed live -- a 0.05 plate-side epsilon still didn't
                // show the blade). Moving the cube's OWN top face strictly
                // below y+0.5 instead removes the ambiguity as real,
                // unambiguous geometry rather than relying on depth-test
                // tie-breaking at all.
                AddCenteredBox(cubes, Microsoft::Xna::Framework::Vector3(block.x, block.y - kSawPlateEpsilon, block.z),
                               Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f),
                               m_tileAtlas->GetTileUv(GalaxyEggbert::BlockTypes::RockPile));
                continue;
            }

            if (AppendSpecialGeometry(definition, tileUv, icon107Uv, center, plateRotated,
                                      mesh))
            {
                continue;
            }

            AddCenteredBox(cubes, center, Microsoft::Xna::Framework::Vector3(1.0f, 1.0f, 1.0f), tileUv);
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
        return UploadLitTexturedMesh(device, combined);
    }

    void TerrainRenderer::Update(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int animPhase)
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

    void TerrainRenderer::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                 Microsoft::Xna::Framework::Graphics::BasicEffect& effect) const
    {
        if (m_staticRenderer)
        {
            m_staticRenderer->Draw(device, effect);
        }
        if (m_cutoutRenderer)
        {
            // Opaque where drawn, so it writes depth like the static mesh;
            // the clear texels are discarded by the alpha test.
            m_cutoutEffect->setWorldProperty(effect.getWorldProperty());
            m_cutoutEffect->setViewProperty(effect.getViewProperty());
            m_cutoutEffect->setProjectionProperty(effect.getProjectionProperty());
            m_cutoutEffect->setTextureProperty(effect.getTextureProperty());
            m_cutoutRenderer->Draw(device, *m_cutoutEffect);
        }
        if (m_animRenderer || m_waterRenderer || m_transparentStaticRenderer)
        {
            // Semi-transparent pass (2026-07-08 design decision, see
            // NEXT.md §8, extended 2026-07-09 to also cover m_animRenderer):
            // drawn last, with alpha blending and depth WRITES disabled (but
            // depth TESTING still on, via DepthRead) so transparent geometry
            // composites correctly over opaque terrain already in the depth
            // buffer without blocking whatever's drawn after it. object-m.png
            // has real per-pixel alpha (confirmed 2026-07-08); NonPremultiplied
            // matches its un-premultiplied RGB. m_animRenderer (lava/crusher/
            // saw/spike/fan/marine/temp) moved into this pass 2026-07-09 --
            // it used to draw opaque like m_staticRenderer, which was wrong:
            // direct pixel sampling of object-m.png shows these tiles'
            // textures are 44-81% transparent pixels (not the near-opaque
            // case icons 30/31 originally motivated this pass for), so
            // drawing them opaque rendered most of each tile's transparent
            // background as solid black -- reported live as "instead of
            // transparency there's black, you can see inside the cube."
            // Covers water (animated), the static-but-transparent icons
            // 30/31, AND the animated non-water set -- same blend/depth
            // state for all three, so one state change covers all draws.
            // State is restored to Opaque/Default afterward so the caller's
            // own state isn't disturbed.
            using Microsoft::Xna::Framework::Graphics::BlendState;
            using Microsoft::Xna::Framework::Graphics::DepthStencilState;
            device.setBlendStateProperty(BlendState::NonPremultiplied);
            device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
            if (m_transparentStaticRenderer)
            {
                m_transparentStaticRenderer->Draw(device, effect);
            }
            if (m_animRenderer)
            {
                m_animRenderer->Draw(device, effect);
            }
            if (m_waterRenderer)
            {
                m_waterRenderer->Draw(device, effect);
            }
            device.setDepthStencilStateProperty(DepthStencilState::Default);
            device.setBlendStateProperty(BlendState::Opaque);
        }
    }

    void TerrainRenderer::DrawGrass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                      Microsoft::Xna::Framework::Graphics::BasicEffect& grassEffect) const
    {
        if (m_grassRenderer)
        {
            m_grassRenderer->Draw(device, grassEffect);
        }
    }

    int TerrainRenderer::VertexCount() const noexcept
    {
        return (m_staticRenderer ? m_staticRenderer->VertexCount() : 0) +
               (m_transparentStaticRenderer ? m_transparentStaticRenderer->VertexCount() : 0) +
               (m_cutoutRenderer ? m_cutoutRenderer->VertexCount() : 0) +
               (m_animRenderer ? m_animRenderer->VertexCount() : 0) +
               (m_waterRenderer ? m_waterRenderer->VertexCount() : 0) +
               (m_grassRenderer ? m_grassRenderer->VertexCount() : 0);
    }

    int TerrainRenderer::PrimitiveCount() const noexcept
    {
        return (m_staticRenderer ? m_staticRenderer->PrimitiveCount() : 0) +
               (m_transparentStaticRenderer ? m_transparentStaticRenderer->PrimitiveCount() : 0) +
               (m_cutoutRenderer ? m_cutoutRenderer->PrimitiveCount() : 0) +
               (m_animRenderer ? m_animRenderer->PrimitiveCount() : 0) +
               (m_waterRenderer ? m_waterRenderer->PrimitiveCount() : 0) +
               (m_grassRenderer ? m_grassRenderer->PrimitiveCount() : 0);
    }
}
