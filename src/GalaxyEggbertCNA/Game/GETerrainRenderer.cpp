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

#include <cmath>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Matches GEWorldRuntime::kWorldCenterX/kWorldCenterZ (== Simple3D's
        // GEWorldRuntime::kWCX/kWCZ) — centers the 100x100 grid on the origin,
        // one world unit per tile.
        constexpr int kWorldCenterX = GEWorldRuntime::kWorldCenterX;
        constexpr int kWorldCenterZ = GEWorldRuntime::kWorldCenterZ;

        // Packs raw grid coordinates (0..blocksPerAxis-1, comfortably under
        // 1024 each) into one key for m_rotatedPlatePositions' hash set --
        // see GETerrainRenderer.hpp's own comment.
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

        // Saw/SawStopped (2026-07-11, user feedback, 2 rounds): the saw
        // should sit AT the walkable floor plane, not floating mid-block
        // like every other confirmed InnerFlatPlate icon (small centered
        // props -- signposts/screens -- where filling most of the block
        // height AND centering both make sense), and NOT sunk below it
        // either. Round 1 tried bottom-anchoring (flush with the block's
        // own -0.5 bottom face) -- still wrong, since neighboring floor
        // tiles' own walkable surface sits at +0.5 (their solid tops), not
        // -0.5, so a bottom-anchored blade sits entirely BELOW the visible
        // floor line, reading as buried/cutting into the ground. Round 2:
        // anchored to the block's TOP face instead (matching where Blupi's
        // feet actually are), extending downward from there. Short (not
        // the shared 0.9 height -- a full-height panel reads as a wall
        // panel regardless of anchor), so it reads as a blade emerging
        // from a floor-level slot. A Saw-specific positioning override,
        // not a change to the shared InnerFlatPlate default (every other
        // confirmed icon keeps the original full-height centered look).
        constexpr float kSawPlateHeight = 0.5f;

        bool IsGroundAnchoredPlateIcon(int icon)
        {
            return icon == GalaxyEggbert::BlockTypes::Saw || icon == GalaxyEggbert::BlockTypes::SawStopped;
        }

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
        //     rendered opaque (fixed via `NeedsAlphaBlend`, unrelated to
        //     the box-vs-pyramid choice), but the box itself then showed a
        //     new problem: its 4 flat side faces (each showing a triangle
        //     via an alpha cutout) don't share a common vertex the way a
        //     real pyramid's 4 triangular faces do, so adjacent faces'
        //     triangle graphics visibly failed to connect at the block's 4
        //     vertical edges ("ty hroty 4 textury trojúhelníku nejsou dole
        //     svázané k sobě");
        // (3) reverted to a genuine tapering pyramid (`Easy3D::
        //     PyramidTipItem`/`AppendPyramidTipMesh()`, re-added to
        //     `../easy-3d` after round 1 had removed it) -- now combined
        //     with the alpha-blend fix from round 2, which independently
        //     fixes the original "black background" problem regardless of
        //     shape, so the pyramid's structurally-shared apex vertex
        //     gives a seamless cone with no black and no seams.
        // Base spans most of the block's footprint, height stops just
        // short of a full block-height so it never pokes through a floor
        // one cell below an open teleporter room.
        constexpr float kTipBaseSize = 0.7f;
        constexpr float kTipHeight = 0.6f;

        bool IsTeleporterTipIcon(int icon)
        {
            return icon == GalaxyEggbert::BlockTypes::Teleport1 ||
                   icon == GalaxyEggbert::BlockTypes::Teleport2 ||
                   icon == GalaxyEggbert::BlockTypes::Teleport3 ||
                   icon == GalaxyEggbert::BlockTypes::Teleport4;
        }

        // The tip's 4 side faces reuse the teleporter's own tile texture,
        // cropped to its lower two-thirds (the dark post graphic) --
        // confirmed by direct inspection of the real icon crop (icon330.png
        // etc.): the top third is the flat panel (colored dots + emblem
        // letter), the bottom two-thirds is the post itself. V grows
        // downward in this atlas (tileUV's vOff comes straight from the
        // row's top pixel), so "lower" means the higher-V end of the tile's
        // own rect.
        Easy3D::UvRect TeleporterTipUv(const Easy3D::UvRect& tileUv)
        {
            const float v0 = tileUv.V0 + (tileUv.V1 - tileUv.V0) / 3.0f;
            return Easy3D::UvRect{tileUv.U0, v0, tileUv.U1, tileUv.V1};
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
        Easy3D::UvRect TeleporterPanelUv(const Easy3D::UvRect& tileUv)
        {
            const float v1 = tileUv.V0 + (tileUv.V1 - tileUv.V0) / 3.0f;
            return Easy3D::UvRect{tileUv.U0, tileUv.V0, tileUv.U1, v1};
        }

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
        // @p plateRotated is only consulted for the InnerFlatPlate branch
        // (see GEInnerFlatPlateTiles.hpp's kPlateRotationMetadataType) --
        // callers pass whether THIS block's own position has rotation
        // metadata set, false for anything not even a candidate.
        bool AppendSpecialGeometry(int lookupIcon, const Easy3D::UvRect& tileUv,
                                   const Easy3D::UvRect& icon107Uv,
                                   const Easy3D::CubeBatch::Vector3& center,
                                   bool plateRotated,
                                   std::vector<Easy3D::CubeVertex>& vertices,
                                   std::vector<std::uint32_t>& indices)
        {
            Easy3D::DirectionalCubeFace directionalFaces[6];
            if (TryGetDirectionalCubeFaces(lookupIcon, tileUv, icon107Uv, directionalFaces))
            {
                Easy3D::DirectionalCubeItem item;
                item.Center = center;
                item.Size = Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f);
                for (int face = 0; face < 6; ++face)
                {
                    item.Faces[face] = directionalFaces[face];
                }

                if (IsTeleporterTipIcon(lookupIcon))
                {
                    // Crop the 4 side faces to the panel (top third) only --
                    // see TeleporterPanelUv's own comment for why (removes
                    // the fake, flattened "second tip" duplicate).
                    const auto panelUv = TeleporterPanelUv(tileUv);
                    item.Faces[static_cast<int>(Easy3D::CubeFace::PosZ)].Uv = panelUv;
                    item.Faces[static_cast<int>(Easy3D::CubeFace::NegZ)].Uv = panelUv;
                    item.Faces[static_cast<int>(Easy3D::CubeFace::PosX)].Uv = panelUv;
                    item.Faces[static_cast<int>(Easy3D::CubeFace::NegX)].Uv = panelUv;
                }

                Easy3D::AppendDirectionalCubeMesh(item, vertices, indices);

                if (IsTeleporterTipIcon(lookupIcon))
                {
                    Easy3D::PyramidTipItem tip;
                    tip.Center = Easy3D::CubeBatch::Vector3(center.X, center.Y - 0.5f, center.Z);
                    tip.BaseSize = kTipBaseSize;
                    tip.Height = kTipHeight;
                    tip.Uv = TeleporterTipUv(tileUv);
                    Easy3D::AppendPyramidTipMesh(tip, vertices, indices);
                }

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
                item.Width = kInnerFlatPlateWidth;
                item.Uv = tileUv;
                item.Axis = GetInnerFlatPlateAxis(lookupIcon, plateRotated);
                if (IsGroundAnchoredPlateIcon(lookupIcon))
                {
                    // Saw specifically (2026-07-11, user feedback -- 2nd
                    // round: "pila je obracene reze do zeme ale mela by
                    // rezat nahoru", i.e. it was anchored to the block's
                    // own BOTTOM face, sinking the whole blade below the
                    // walkable floor plane of every neighboring tile
                    // (their solid tops sit at +0.5, not this block's own
                    // -0.5) -- it read as buried, cutting DOWN into the
                    // ground instead of poking UP into the space Blupi
                    // actually walks through. Anchored to the block's TOP
                    // face instead (matching the neighboring floor tiles'
                    // own walkable surface, where Blupi's feet are),
                    // extending downward from there -- shorter than the
                    // shared kInnerFlatPlateHeight so it still reads as a
                    // blade emerging from a floor-level slot, not a
                    // wall-height panel.
                    item.Height = kSawPlateHeight;
                    item.Center = Easy3D::CubeBatch::Vector3(
                        center.X, center.Y + 0.5f - kSawPlateHeight * 0.5f, center.Z);
                }
                else
                {
                    item.Height = kInnerFlatPlateHeight;
                    item.Center = center;
                }
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

        // Real per-type tick divisor against the 20fps raw tick
        // (GEWorldRuntime::GetAnimPhase(), fixed 2026-07-09 -- see its own
        // comment) -- mobile-eggbert's real Decor.cpp tile-animation logic
        // (table_decor_scie/lave/eau1/eau2/ecraseur/piege1/piege2/temp,
        // table_marine) divides the same 20fps base tick per type via
        // Config::ScaleDiv(N), NOT a flat rate shared by every animated
        // tile -- confirmed by direct source line, e.g. `table_decor_lave[
        // ... + m_time/ScaleDiv(2)]`. Water2/Marine's real divisor also
        // varies per-instance (3 + position%3, a visual ripple-offset
        // detail) -- not modeled here, every instance of a given type
        // shares one phase; only the base speed is fixed.
        //
        // Fan's divisor is NOT from mobile-eggbert source -- the FanLeft/
        // Right/Up/Down BLOCK icons (126-137) have no dedicated
        // table_decor_* animation entry in Decor.cpp at all (only a
        // same-numbered-looking but unrelated table_decor_ventg/ventd/
        // venth/ventb exists, animating icons 110-125, a separate wind
        // "particle stream" decor effect, not these cube blocks -- an
        // initial 2026-07-09 fix wrongly matched fans to that table's
        // divisor 1/50ms and was reported live as "now too fast"; reverted
        // to divisor 3/150ms here, the same rate as the other real
        // mechanical/moving elements (Water1/Crusher/Marine) for lack of a
        // real per-type source value to match.
        int AnimDivisor(std::uint16_t base)
        {
            using namespace GalaxyEggbert::BlockTypes;
            switch (base)
            {
                case Saw:                                              return 1; // 50ms/frame
                case Lava:                                              return 2; // 100ms/frame
                case FanLeft: case FanRight: case FanUp: case FanDown:
                case Water1: case Crusher: case Water2: case Marine:    return 3; // 150ms/frame
                case Spike: case Temp:                                  return 4; // 200ms/frame
                default:                                                return 3;
            }
        }

        int AnimIcon(std::uint16_t base, int rawTick)
        {
            using namespace GalaxyEggbert::BlockTypes;
            const int phase = rawTick / AnimDivisor(base);
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
        //
        // Teleporter pillars (330-333) added 2026-07-11: direct pixel
        // inspection of the real icon crop (confirmed via a small script
        // sampling object-m.png's alpha channel) found the "black" area the
        // user reported around the post/tip graphic is genuine alpha=0
        // (fully transparent), not painted black -- rendering it opaque was
        // the actual bug, same category as icons 30/31 above, not a
        // geometry problem (the tip's pyramid-vs-prism shape was a separate,
        // real issue, already fixed independently -- see AppendSpecialGeometry).
        bool NeedsAlphaBlend(int icon)
        {
            return icon == 30 || icon == 31 ||
                   icon == GalaxyEggbert::BlockTypes::Teleport1 || icon == GalaxyEggbert::BlockTypes::Teleport2 ||
                   icon == GalaxyEggbert::BlockTypes::Teleport3 || icon == GalaxyEggbert::BlockTypes::Teleport4;
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
            // Icons 108/109 (2026-07-09) reuse icon 107's real top surface
            // too -- their own DirectionalCube entry (GEDirectionalCubeTiles.cpp)
            // leaves PosY open on purpose, same reasoning as icon 107 itself.
            return icon == 107 || icon == 108 || icon == 109;
        }

        // True if @p icon uses one of the "new geometry" render modes
        // (AppendSpecialGeometry above) -- these are smaller than a full
        // block or leave faces intentionally open, so they must never be
        // treated as occluders by IsOccluderBlock below.
        bool IsSpecialGeometryIcon(int icon)
        {
            Easy3D::DirectionalCubeFace unusedFaces[6];
            return TryGetDirectionalCubeFaces(icon, Easy3D::UvRect{}, Easy3D::UvRect{}, unusedFaces) ||
                   TryGetInnerPillarBoxFaces(icon, Easy3D::UvRect{}, unusedFaces) ||
                   IsInnerFlatPlateIcon(icon) ||
                   IsTripleCrossBillboardIcon(icon);
        }

        // Face-culling occlusion test (NEXT.md §8 -- "add face-culling/
        // occlusion to GETerrainRenderer"): true only for a block that is
        // guaranteed to be a plain, fully opaque 1x1x1 cube on every side --
        // non-air, in bounds, not water, not animated (some animated icons
        // use special/holed geometry -- see AppendSpecialGeometry -- and
        // conservatively excluding ALL of them, not just the holed ones,
        // keeps this check simple and never over-culls), not alpha-blended,
        // and not itself a special-geometry icon. Deliberately conservative:
        // a face is only culled when its neighbor is DEFINITELY solid on
        // that side, never a guess -- worth revisiting later for e.g.
        // DirectionalCube neighbors (their own per-face pattern is already
        // known via TryGetDirectionalCubeFaces, so they COULD occlude on
        // their opaque faces), but that's extra complexity for a case this
        // task doesn't need yet (§8's own note: "not needed at the current
        // ~2700-block scale").
        bool IsOccluderBlock(const Worlds::World& world, int blocksPerAxis, int x, int y, int z)
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

            const std::uint16_t animBase = GalaxyEggbert::BlockTypes::tileAnimBase(block.type());
            if (IsWater(animBase) || IsAnimated(animBase))
            {
                return false;
            }

            const int icon = static_cast<int>(block.type());
            if (NeedsAlphaBlend(icon) || IsSpecialGeometryIcon(icon))
            {
                return false;
            }

            return true;
        }
    }

    GETerrainRenderer::GETerrainRenderer(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Worlds::World& world,
                                         const GETileAtlas& tileAtlas)
        : m_tileAtlas(&tileAtlas)
    {
        // Icons 108/109's DirectionalCube entry reuses icon 107's own side
        // texture (GEDirectionalCubeTiles.cpp) -- computed once here since
        // it never changes, threaded through AppendSpecialGeometry.
        const auto icon107Uv = tileAtlas.GetTileUv(107);
        for (const auto& pos : CollectRotatedPlatePositions(world))
        {
            m_rotatedPlatePositions.insert(PackPlatePositionKey(pos[0], pos[1], pos[2]));
        }
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
                    const bool plateRotated = m_rotatedPlatePositions.count(PackPlatePositionKey(x, y, z)) != 0;

                    if (NeedsAlphaBlend(icon))
                    {
                        // Not animated, so it skips m_waterBlocks entirely --
                        // goes straight into its own static-but-transparent
                        // buffer, built once here and never rebuilt by
                        // Update() (same lifecycle as m_staticRenderer).
                        AppendSpecialGeometry(icon, tileUv, icon107Uv, center, plateRotated, transparentStaticVertices, transparentStaticIndices);
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

                    if (AppendSpecialGeometry(icon, tileUv, icon107Uv, center, plateRotated, staticVertices, staticIndices))
                    {
                        continue;
                    }

                    // Face culling (NEXT.md §8): only emit a face when its
                    // neighbor doesn't fully occlude it (see IsOccluderBlock).
                    // Uses the same DirectionalCubeItem geometry AppendSpecialGeometry
                    // already uses for holed render modes -- with all 6 faces
                    // Visible and the same Uv, it's byte-for-byte equivalent
                    // to a plain UniformCube (confirmed by ../easy-3d's own
                    // "all-visible-faces parity with AppendCubeMesh" test),
                    // so this doesn't change anything for a block with no
                    // solid neighbor (e.g. every isolated demo block in this
                    // sample world) -- only interior faces between two solid
                    // blocks (the common case in bulk fills like the ground
                    // floor/walls/staircase) actually get culled.
                    Easy3D::DirectionalCubeItem item;
                    item.Center = center;
                    item.Size = Easy3D::CubeBatch::Vector3(1.0f, 1.0f, 1.0f);
                    item.Faces[static_cast<int>(Easy3D::CubeFace::PosZ)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y, z + 1), tileUv};
                    item.Faces[static_cast<int>(Easy3D::CubeFace::NegZ)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y, z - 1), tileUv};
                    item.Faces[static_cast<int>(Easy3D::CubeFace::PosX)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x + 1, y, z), tileUv};
                    item.Faces[static_cast<int>(Easy3D::CubeFace::NegX)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x - 1, y, z), tileUv};
                    item.Faces[static_cast<int>(Easy3D::CubeFace::PosY)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y + 1, z), tileUv};
                    item.Faces[static_cast<int>(Easy3D::CubeFace::NegY)] =
                        {!IsOccluderBlock(world, blocksPerAxis, x, y - 1, z), tileUv};
                    Easy3D::AppendDirectionalCubeMesh(item, staticVertices, staticIndices);
                }
            }
        }

        if (m_blockCount > 0)
        {
            m_centroidX = static_cast<float>(sumX / m_blockCount);
            m_centroidY = static_cast<float>(sumY / m_blockCount);
            m_centroidZ = static_cast<float>(sumZ / m_blockCount);
        }

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
        const auto icon107Uv = m_tileAtlas->GetTileUv(107);
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
            // GEDirectionalCubeTiles entry was silently dead code.
            if (AppendSpecialGeometry(static_cast<int>(block.base), tileUv, icon107Uv, center, plateRotated, vertices, indices))
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
