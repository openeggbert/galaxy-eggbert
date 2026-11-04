// The faces DirectionalCube tiles show (src/GalaxyEggbert/Game/DirectionalCubeTiles.cpp): which
// side of a fan is its base and which is open, and where flat-colour faces sample their tile.
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/BarRows.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/BoltMesh.hpp>
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>
#include <GalaxyEggbert/Game/LavaSurface.hpp>
#include <GalaxyEggbert/Game/MeshParts.hpp>
#include <GalaxyEggbert/Game/TerrainRelief.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <GalaxyEggbert/Game/TileAtlas.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <vector>

int main()
{
    using CnaEngine::Graphics::BoxFace;
    using CnaEngine::Graphics::BoxFaces;
    using GalaxyEggbert::Game::ConfigureDirectionalCubeFaces;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* what)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << what << std::endl;
        allOk = allOk && condition;
    };
    const GalaxyEggbert::Game::TileAtlas atlas;
    const auto faces = [&atlas](int icon)
    {
        BoxFaces out;
        ConfigureDirectionalCubeFaces(icon, atlas.GetTileUv(icon), atlas.GetTileUv(107), out);
        return out;
    };
    const auto face = [](const BoxFaces& f, BoxFace which) { return f[static_cast<int>(which)]; };
    const float texelU = 1.0f / static_cast<float>(GalaxyEggbert::BlockTypes::kSheetW);

    // Fans: open toward where they blow, the base (flat colour) on the other side.
    const auto fanLeft = faces(GalaxyEggbert::BlockTypes::FanLeft);
    check(!face(fanLeft, BoxFace::NegativeX).visible && face(fanLeft, BoxFace::PositiveX).visible,
          "FanLeft (126) blows -X: open at -X, its base at +X (its crop's lights panel on the right)");
    const auto fanRight = faces(GalaxyEggbert::BlockTypes::FanRight);
    check(!face(fanRight, BoxFace::PositiveX).visible && face(fanRight, BoxFace::NegativeX).visible,
          "FanRight (129) blows +X: open at +X, its base at -X");
    check(face(fanLeft, BoxFace::NegativeZ).uv.u0 > face(fanLeft, BoxFace::NegativeZ).uv.u1 &&
              face(fanLeft, BoxFace::PositiveZ).uv.u0 < face(fanLeft, BoxFace::PositiveZ).uv.u1 &&
              face(fanLeft, BoxFace::PositiveY).uv.u0 < face(fanLeft, BoxFace::PositiveY).uv.u1,
          "a horizontal fan's -Z face is mirrored, +Z and the top are not: its base on one side from every view");
    const auto fanUp = faces(GalaxyEggbert::BlockTypes::FanUp);
    check(!face(fanUp, BoxFace::PositiveY).visible && face(fanUp, BoxFace::NegativeY).visible,
          "FanUp (132) blows up: open at the top, base at the bottom");
    const auto fanDown = faces(GalaxyEggbert::BlockTypes::FanDown);
    check(!face(fanDown, BoxFace::NegativeY).visible && face(fanDown, BoxFace::PositiveY).visible,
          "FanDown (135) blows down: open at the bottom, base at the top");

    // Flat-colour faces: a few texels of the tile, so they read as one colour; the facades'
    // from their wall's corner.
    const auto window = faces(190);
    const auto top = face(window, BoxFace::PositiveY).uv;
    const auto tile = atlas.GetTileUv(190);
    check(std::fabs(top.u1 - top.u0) <= 4.0f * texelU, "a flat-colour face samples at most four texels across");
    const float centreU = ((top.u0 + top.u1) * 0.5f - tile.u0) / (tile.u1 - tile.u0);
    const float centreV = ((top.v0 + top.v1) * 0.5f - tile.v0) / (tile.v1 - tile.v0);
    check(centreU < 0.1f && centreV > 0.9f, "a facade tile's flat faces sample its wall at a bottom corner");
    check(face(window, BoxFace::PositiveZ).uv.u0 == tile.u0 && face(window, BoxFace::PositiveZ).uv.u1 == tile.u1,
          "its front (+Z) shows the whole tile");

    // Closed cubes occlude (D47): the machine panel draws all six faces, the grate (200) has no
    // top or bottom, icon 107 no top.
    check(GalaxyEggbert::Game::IsClosedDirectionalCube(10), "the machine panel (10) is a closed cube");
    check(!GalaxyEggbert::Game::IsClosedDirectionalCube(200) && !GalaxyEggbert::Game::IsClosedDirectionalCube(107),
          "the grate (200) and icon 107 have open sides");

    // Plates read upright and unmirrored from both sides (D48, fixed in cna-engine `2a93b7e`): on
    // every triangle, seen from its front, the region's top (v0) is at the top and u grows to the
    // viewer's right.
    const auto plateReadsRight = [](const CnaEngine::Graphics::MeshData& mesh)
    {
        using Microsoft::Xna::Framework::Vector3;
        for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3)
        {
            const std::uint32_t idx[3] = {mesh.indices[t], mesh.indices[t + 1], mesh.indices[t + 2]};
            const Vector3& a = mesh.positions[idx[0]];
            // Clockwise seen from the front: the front normal is cross(c - a, b - a).
            const Vector3 front = Vector3::Normalize(
                Vector3::Cross(mesh.positions[idx[2]] - a, mesh.positions[idx[1]] - a));
            const Vector3 viewerRight = Vector3::Cross(-front, Vector3::UnitY);
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    const Vector3 d = mesh.positions[idx[j]] - mesh.positions[idx[i]];
                    const auto& uvI = mesh.texCoords0[idx[i]];
                    const auto& uvJ = mesh.texCoords0[idx[j]];
                    if (d.Y > 0.01f && !(uvJ.Y < uvI.Y)) return false;
                    if (Vector3::Dot(d, viewerRight) > 0.01f && !(uvJ.X > uvI.X)) return false;
                }
            }
        }
        return true;
    };
    const CnaEngine::Graphics::UvRect region{0.25f, 0.5f, 0.375f, 0.625f};
    for (const auto axis : {CnaEngine::Graphics::PlateAxis::Z, CnaEngine::Graphics::PlateAxis::X})
    {
        auto mesh = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        mesh.AddPlate(Microsoft::Xna::Framework::Vector3(3, 4, 5), 0.75f, 0.9f, axis, region);
        check(plateReadsRight(mesh.Mesh()), axis == CnaEngine::Graphics::PlateAxis::Z
                                                ? "a Z plate (a sign) reads upright and unmirrored from +Z and -Z"
                                                : "an X plate (a turned sign) reads upright and unmirrored from +X and -X");
    }
    {
        auto mesh = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        mesh.AddCrossedPlates(Microsoft::Xna::Framework::Vector3(3, 4, 5), 0.8f, 1.0f, 3, region);
        check(plateReadsRight(mesh.Mesh()), "a crossed-plate sprite's three planes read upright and unmirrored");
    }

    // A terrain over a buffer's limit is uploaded in parts (D47): every part within the limit,
    // the triangles all there, in order, with their corners and texture coordinates.
    {
        auto mesh = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        for (int i = 0; i < 25; ++i)
        {
            GalaxyEggbert::Game::AddCenteredBox(mesh, Microsoft::Xna::Framework::Vector3(static_cast<float>(i), 0, 0),
                                                Microsoft::Xna::Framework::Vector3(1, 1, 1),
                                                CnaEngine::Graphics::UvRect{0.1f * (i % 7), 0.0f, 0.5f, 0.5f});
        }
        const auto& whole = mesh.Mesh();
        const auto parts = GalaxyEggbert::Game::SplitMeshData(whole, 50);
        bool within = parts.size() > 1;
        std::vector<Microsoft::Xna::Framework::Vector3> corners;
        std::vector<Microsoft::Xna::Framework::Vector2> uvs;
        for (const auto& part : parts)
        {
            within = within && part.VertexCount() <= 50 && part.Validate().empty() &&
                     part.normals.size() == part.positions.size();
            for (const std::uint32_t index : part.indices)
            {
                corners.push_back(part.positions[index]);
                uvs.push_back(part.texCoords0[index]);
            }
        }
        bool same = corners.size() == whole.indices.size();
        for (std::size_t k = 0; same && k < corners.size(); ++k)
        {
            const auto& p = whole.positions[whole.indices[k]];
            const auto& uv = whole.texCoords0[whole.indices[k]];
            same = corners[k].X == p.X && corners[k].Y == p.Y && corners[k].Z == p.Z && uvs[k].X == uv.X &&
                   uvs[k].Y == uv.Y;
        }
        check(within && same, "a mesh over the limit splits into parts within it, every triangle kept in order (D47)");
        check(GalaxyEggbert::Game::SplitMeshData(whole).size() == 1, "a mesh within the limit stays whole");
    }

    // Lava in 3D (VIS-0069): a pool with the original frames' wave on top, wound as the builder's own
    // faces, its rim flat so cells join, walls only where no lava is beside it.
    {
        using Microsoft::Xna::Framework::Vector3;
        const auto frontFacesAgreeWithNormals = [](const CnaEngine::Graphics::MeshData& m)
        {
            for (std::size_t t = 0; t + 2 < m.indices.size(); t += 3)
            {
                const Vector3 a = m.positions[m.indices[t]];
                const Vector3 b = m.positions[m.indices[t + 1]];
                const Vector3 c = m.positions[m.indices[t + 2]];
                const Vector3 n = m.normals[m.indices[t]] + m.normals[m.indices[t + 1]] + m.normals[m.indices[t + 2]];
                if (Vector3::Dot(Vector3::Cross(b - a, c - a), n) >= 0.0f)
                {
                    return false;
                }
            }
            return true;
        };
        CnaEngine::Graphics::MeshBuilder box = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        box.AddBox(Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));
        check(frontFacesAgreeWithNormals(box.Mesh()), "the builder's box: cross(b - a, c - a) points against the normal");
        const CnaEngine::Graphics::UvRect tile{0.0f, 0.0f, 1.0f, 1.0f};
        for (int frame = 0; frame < GalaxyEggbert::Game::kLavaFrames; ++frame)
        {
            CnaEngine::Graphics::MeshBuilder lava = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
            GalaxyEggbert::Game::AppendLavaCell(lava, Vector3(0.0f, 0.5f, 0.0f), frame, tile, {});
            const auto& m = lava.Mesh();
            bool inRange = true;
            for (const auto& p : m.positions)
                inRange = inRange && p.Y >= -1e-5f && p.Y <= 0.55f;
            check(frontFacesAgreeWithNormals(m) && inRange,
                  "a lava cell is wound as the builder's faces and stays under 0.55 of the cell");
        }
        bool rimFlat = true;
        for (int frame = 0; frame < GalaxyEggbert::Game::kLavaFrames; ++frame)
            for (float t = 0.0f; t <= 1.0f; t += 0.1f)
                rimFlat = rimFlat &&
                          std::fabs(GalaxyEggbert::Game::LavaSurfaceHeight(frame, 0.0f, t) - GalaxyEggbert::Game::kLavaRimHeight) < 1e-5f &&
                          std::fabs(GalaxyEggbert::Game::LavaSurfaceHeight(frame, 1.0f, t) - GalaxyEggbert::Game::kLavaRimHeight) < 1e-5f &&
                          std::fabs(GalaxyEggbert::Game::LavaSurfaceHeight(frame, t, 0.0f) - GalaxyEggbert::Game::kLavaRimHeight) < 1e-5f &&
                          std::fabs(GalaxyEggbert::Game::LavaSurfaceHeight(frame, t, 1.0f) - GalaxyEggbert::Game::kLavaRimHeight) < 1e-5f;
        check(rimFlat, "every frame's rim is at the trough level, so lava cells side by side join without a step");
        float crest = 0.0f;
        for (int frame = 0; frame < GalaxyEggbert::Game::kLavaFrames; ++frame)
            for (float u = 0.0f; u <= 1.0f; u += 0.05f)
                for (float w = 0.0f; w <= 1.0f; w += 0.05f)
                    crest = std::max(crest, GalaxyEggbert::Game::LavaSurfaceHeight(frame, u, w));
        check(crest > 0.40f && crest < 0.55f, "the wave's crests rise to about half a cell, as the tiles' spikes");
        CnaEngine::Graphics::MeshBuilder open = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        GalaxyEggbert::Game::AppendLavaCell(open, Vector3(0.0f, 0.5f, 0.0f), 0, tile, {});
        CnaEngine::Graphics::MeshBuilder surrounded = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        GalaxyEggbert::Game::AppendLavaCell(surrounded, Vector3(0.0f, 0.5f, 0.0f), 0, tile, {true, true, true, true, true});
        const std::size_t top = static_cast<std::size_t>(GalaxyEggbert::Game::kLavaGridX * GalaxyEggbert::Game::kLavaGridZ);
        const std::size_t innerTop =
            static_cast<std::size_t>(GalaxyEggbert::Game::kLavaInnerGridX * GalaxyEggbert::Game::kLavaInnerGridZ);
        check(surrounded.Mesh().VertexCount() == innerTop && open.Mesh().VertexCount() == top + 5 * 4,
              "walls and floor only where no lava (or, below, nothing solid) is beside the cell");
        CnaEngine::Graphics::MeshBuilder edge = GalaxyEggbert::Game::MakeLitTexturedMeshBuilder();
        GalaxyEggbert::Game::AppendLavaCell(edge, Vector3(0.0f, 0.5f, 0.0f), 0, tile, {true, false, true, true, true});
        check(innerTop * 4 < top && edge.Mesh().VertexCount() == top + 4 && frontFacesAgreeWithNormals(surrounded.Mesh()),
              "a pool's inner cell has a coarser wave (Q-008), its edge cells the full one");
    }

    // The fan model (VIS-0127) blows up; turned by FanModelRoll it blows each fan cell's way:
    // 126 -X, 129 +X, 132 +Y, 135 -Y (the cells' gameplay and face tables).
    {
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector3;
        const auto blows = [](std::uint16_t base)
        { return Vector3::Transform(Vector3(0.0f, 1.0f, 0.0f), Matrix::CreateRotationZ(GalaxyEggbert::Game::FanModelRoll(base))); };
        const auto near = [](const Vector3& a, const Vector3& b) { return Vector3::Distance(a, b) < 1e-4f; };
        check(near(blows(126), Vector3(-1.0f, 0.0f, 0.0f)) && near(blows(129), Vector3(1.0f, 0.0f, 0.0f)) &&
                  near(blows(132), Vector3(0.0f, 1.0f, 0.0f)) && near(blows(135), Vector3(0.0f, -1.0f, 0.0f)),
              "the fan model turned for each fan cell blows the cell's way (-X, +X, up, down)");
    }

    // The hanging bar shows its ceiling mounts (138) only under a tile they hold to, as the original's
    // editor (Decor::IsRightBorder): under air, another bar, or a tile it rules out, the plain pipe
    // 202; under passable decor such as the girder 201 the mounts all the same. Both bars are one row.
    {
        using GalaxyEggbert::Game::ShownBarIcon;
        check(ShownBarIcon(0) == 202 && ShownBarIcon(10) == 138 && ShownBarIcon(138) == 202 &&
                  ShownBarIcon(202) == 202 && ShownBarIcon(25) == 202 && ShownBarIcon(15) == 202 &&
                  ShownBarIcon(230) == 138 && ShownBarIcon(201) == 138 && ShownBarIcon(264) == 202,
              "a bar shows its mounts only under a tile they hold to (Decor::IsRightBorder)");
        check(GalaxyEggbert::Game::ModelRowFamily(202) == GalaxyEggbert::Game::ModelRowFamily(138) &&
                  GalaxyEggbert::Game::ModelRowFamily(201) == 201,
              "the mounted and the plain bar continue one row");
    }

    // A bar cell's row (BarRows.hpp): along its bar neighbours, a lone cell along X, a field where
    // it has them on both axes.
    {
        using GalaxyEggbert::Game::BarRow;
        using GalaxyEggbert::Game::ClassifyBarRow;
        check(ClassifyBarRow(true, false) == BarRow::AlongX && ClassifyBarRow(false, true) == BarRow::AlongZ &&
                  ClassifyBarRow(false, false) == BarRow::AlongX && ClassifyBarRow(true, true) == BarRow::Field,
              "a bar runs along its row; a lone bar along X; neighbours on both axes make a field");
    }

    // A model with a front (the teleporters, BlockModels.hpp) faces +Z unturned; FrontModelYaw turns
    // it toward its cell's open side: 0 +X, 1 -X, 2 +Z, 3 -Z.
    {
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector3;
        const auto front = [](int side)
        { return Vector3::Transform(Vector3(0.0f, 0.0f, 1.0f), Matrix::CreateRotationY(GalaxyEggbert::Game::FrontModelYaw(side))); };
        const auto near = [](const Vector3& a, const Vector3& b) { return Vector3::Distance(a, b) < 1e-4f; };
        check(near(front(0), Vector3(1.0f, 0.0f, 0.0f)) && near(front(1), Vector3(-1.0f, 0.0f, 0.0f)) &&
                  near(front(2), Vector3(0.0f, 0.0f, 1.0f)) && near(front(3), Vector3(0.0f, 0.0f, -1.0f)),
              "a teleporter's front turns to its open side (+X, -X, +Z, -Z)");
    }

    // The clay bridge's row of lumps is drawn across its whole cell (BlockModels.hpp, owner
    // 2026-11-02): the rows, placed and turned, span the cell from edge to edge without a gap,
    // every other one turned round.
    {
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector3;
        namespace G = GalaxyEggbert::Game;
        float lo = 1.0f, hi = -1.0f, lastFar = -0.5f;
        bool noGap = true, alternate = true;
        for (int row = 0; row < G::kBridgeRows; ++row)
        {
            const auto placement = G::BridgeRowPlacement(row);
            const auto centre = Vector3::Transform(Vector3(0.0f, 0.0f, G::kBridgeRowCentreZ),
                                                   Matrix::CreateRotationY(placement.yaw));
            const float near = centre.Z + placement.dz - G::kBridgeRowDepth / 2.0f;
            const float far = centre.Z + placement.dz + G::kBridgeRowDepth / 2.0f;
            noGap = noGap && near <= lastFar + 1e-4f;
            lastFar = far;
            lo = std::min(lo, near);
            hi = std::max(hi, far);
            alternate = alternate && ((row % 2 == 1) == (placement.yaw > 1.0f));
        }
        check(std::fabs(lo + 0.5f) < 1e-3f && std::fabs(hi - 0.5f) < 1e-3f && noGap,
              "the clay bridge's rows cover its cell from edge to edge without a gap");
        check(alternate, "every other row of the clay bridge is turned round");
        check(G::IsClayBridgeIcon(364) && G::IsClayBridgeIcon(372) && !G::IsClayBridgeIcon(363) &&
                  !G::IsClayBridgeIcon(373),
              "the clay bridge's icons are 364-372");
    }

    // A teleporter's body is its blue box; its letter plate and lights are not (OR-0056: those are
    // drawn on all four sides).
    {
        namespace G = GalaxyEggbert::Game;
        bool bodies = true, fronts = true;
        for (const char* name : {"teleporter1_rd_1099c6", "teleporter2_rd_109bc9", "teleporter3_rd_119ac7",
                                 "teleporter4_rd_1099c6"})
        {
            bodies = bodies && G::IsBoothBodyPart(name);
        }
        for (const char* name : {"teleporter1_rd_c71010", "teleporter1_rd_c6c610", "teleporter2_rd_ca1010",
                                 "teleporter2_rd_c9c910", "teleporter3_rd_c81111", "teleporter3_rd_c7c711",
                                 "teleporter1_rd_tex_p08_mur_sud", "teleporter4_rd_tex_p08_mur_sud"})
        {
            fronts = fronts && !G::IsBoothBodyPart(name);
        }
        check(bodies, "each teleporter's blue box is its body");
        check(fronts, "its lights and letter plate are not its body");
        check(G::IsTeleporterIcon(330) && G::IsTeleporterIcon(333) && !G::IsTeleporterIcon(334),
              "the teleporters are 330-333");
    }

    // A tile textured on one side only turns that side to the room (the owner's answer: its side is
    // per block; it was always +Z, so most walls of them showed their plain colour).
    {
        namespace G = GalaxyEggbert::Game;
        constexpr std::uint8_t kPosZ = 1u << 0, kNegZ = 1u << 1, kPosX = 1u << 2;
        const int open[4] = {0, 0, 0, 5};
        check(G::SingleTexturedSide(190, 0, open) == 0, "a window tile open on its own side (+Z) keeps it");
        check(G::SingleTexturedSide(190, kPosZ | kNegZ | kPosX, open) == 3,
              "a window tile open only on -X shows its window there");
        const int room[4] = {0, 2, 6, 0};
        check(G::SingleTexturedSide(10, kPosZ, room) == 2, "of two open sides, the one with more room");
        check(G::SingleTexturedSide(10, 0x0F, room) == 0, "no open side: its default");
        check(G::SingleTexturedSide(392, 0, open) == 3, "the trim tile 392 keeps its own default side (-X)");
        check(G::SingleTexturedSide(2, 0, open) == -1, "a tile textured all round has no single side");
        BoxFaces turned;
        ConfigureDirectionalCubeFaces(190, atlas.GetTileUv(190), atlas.GetTileUv(107), turned, 2);
        const auto window = atlas.GetTileUv(190);
        check(face(turned, BoxFace::PositiveX).uv.u0 == window.u0 && face(turned, BoxFace::PositiveX).uv.u1 == window.u1 &&
                  face(turned, BoxFace::PositiveZ).uv.u1 - face(turned, BoxFace::PositiveZ).uv.u0 <
                      window.u1 - window.u0,
              "turned to +X, the window is on +X and +Z takes the plain colour");
    }

    // The machine panels' plain sides take the panel's base blue from its top-left corner, not a
    // stripe of its bolts or lamps from the middle (play pass 2026-11-03: a green wedge on 114's floor).
    {
        bool cornered = true;
        for (const int icon : {3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 48})
        {
            const auto f = faces(icon);
            const auto tile = atlas.GetTileUv(icon);
            const auto& side = face(f, BoxFace::NegativeX).uv;
            const float u = ((side.u0 + side.u1) * 0.5f - tile.u0) / (tile.u1 - tile.u0);
            const float v = ((side.v0 + side.v1) * 0.5f - tile.v0) / (tile.v1 - tile.v0);
            cornered = cornered && u < 0.1f && v < 0.1f;
        }
        check(cornered, "the machine panels' plain sides sample the panel's base at its top-left corner");
    }

    // The drip's smallest drops are the smallest scene's drop scaled to their sprites' heights.
    {
        namespace G = GalaxyEggbert::Game;
        check(G::DripShrink(407).modelIcon == 407 && G::DripShrink(407).scale == 1.0f, "the drop 407 is its own scene");
        check(G::DripShrink(408).modelIcon == 407 && G::DripShrink(410).modelIcon == 407 &&
                  G::DripShrink(408).scale > G::DripShrink(409).scale && G::DripShrink(409).scale > G::DripShrink(410).scale &&
                  G::DripShrink(410).scale > 0.2f && G::DripShrink(410).scale < 0.25f,
              "408-410 are 407's drop at 27, 18 and 8 of its 36 px");
    }

    // The terrain's padded sheet: each tile 64 x 64 inside a 16 px border of its own edge texels.
    {
        namespace G = GalaxyEggbert::Game;
        const G::TileAtlas padded(G::TileAtlas::Layout::Padded);
        const auto uv = padded.GetTileUv(10);
        const float u0 = 16.0f / G::TileAtlas::kPaddedWidth, v0 = 16.0f / G::TileAtlas::kPaddedHeight;
        check(std::fabs(uv.u0 - (u0 + 10 * 96.0f / G::TileAtlas::kPaddedWidth)) < 1e-6f && std::fabs(uv.v0 - v0) < 1e-6f &&
                  std::fabs((uv.u1 - uv.u0) * G::TileAtlas::kPaddedWidth - 64.0f) < 1e-3f,
              "the padded atlas puts icon 10 at column 10, 64 texels inside its 16 px border");
        // A 2 x 1 tile sheet of the real layout: tile 0 red, tile 1 green.
        constexpr int kW = 2 * 65 + 1, kH = 65 + 1;
        std::vector<std::uint8_t> sheet(static_cast<std::size_t>(kW) * kH * 4, 0);
        for (int y = 1; y < 65; ++y)
        {
            for (int x = 1; x < kW; ++x)
            {
                std::uint8_t* t = &sheet[(static_cast<std::size_t>(y) * kW + x) * 4];
                const bool green = x > 65;
                t[0] = green ? 0 : 255;
                t[1] = green ? 255 : 0;
                t[3] = 255;
            }
        }
        const auto out = G::BuildPaddedSheet(sheet.data(), kW, kH);
        const auto at = [&](int x, int y) { return &out[(static_cast<std::size_t>(y) * G::TileAtlas::kPaddedWidth + x) * 4]; };
        check(at(0, 0)[0] == 255 && at(95, 50)[0] == 255 && at(96, 0)[1] == 255 && at(96 + 95, 95)[1] == 255,
              "each tile's border is its own edge colour, no neighbour bleeding in");
        std::vector<std::uint8_t> two{0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 255, 255, 255, 255};
        const auto half = G::DownsampleHalf(two, 2, 2);
        check(half.size() == 4 && half[0] == 128 && half[3] == 255, "a mip texel is the mean of its 2 x 2 block");
    }

    // The lightning's 3D bolt (Q-002): tubes along each frame's painted path, inside the cell, its
    // trunk reaching the cell's top and bottom (columns join), wound as the builder's faces.
    {
        namespace G = GalaxyEggbert::Game;
        bool inside = true, reachesTop = false, reachesBottom = false, wound = true;
        for (int frame = 0; frame < 3; ++frame)
        {
            CnaEngine::Graphics::MeshBuilder bolt = G::MakeLitTexturedMeshBuilder();
            G::AppendBoltCell(bolt, Microsoft::Xna::Framework::Vector3(0.0f, 0.0f, 0.0f), frame, atlas.GetTileUv(305), 0.7f);
            const auto& m = bolt.Mesh();
            for (const auto& p : m.positions)
            {
                inside = inside && std::fabs(p.X) <= 0.56f && std::fabs(p.Y) <= 0.56f && std::fabs(p.Z) <= 0.56f;
                reachesTop = reachesTop || p.Y > 0.48f;
                reachesBottom = reachesBottom || p.Y < -0.48f;
            }
            wound = wound && m.VertexCount() > 100;
        }
        check(inside && reachesTop && reachesBottom && wound,
              "the 3D bolt stays in its cell and runs from its top to its bottom");
    }

    // A front model turns to the side with the most room to stand in (OR-0056 reopened): a model
    // standing on the floor counts the floor right beside it, a booth over Blupi's head the floor
    // two cells down; without room anywhere a row faces across itself.
    {
        namespace G = GalaxyEggbert::Game;
        // A room: floor at y 0, walls at x 0 and x 10, z 0 and z 10; `solid` adds cells.
        const auto room = [](std::initializer_list<std::array<int, 3>> solid)
        {
            return [solid](int x, int y, int z)
            {
                if (y <= 0 || x <= 0 || x >= 10 || z <= 0 || z >= 10)
                {
                    return false;
                }
                for (const auto& c : solid)
                {
                    if (c[0] == x && c[1] == y && c[2] == z)
                    {
                        return false;
                    }
                }
                return true;
            };
        };
        // A sign on the floor against the north wall (z 1): its room is to the south, +Z.
        check(G::FrontModelOpenSide(5, 1, 1, 0, room({})) == 2,
              "a sign on the floor against a wall faces the room, not +X");
        // A sign on the floor against the east wall (x 9): -X.
        check(G::FrontModelOpenSide(9, 1, 5, 0, room({})) == 1, "a sign against the east wall faces -X");
        // A booth hanging at y 2 over Blupi, in a corridor along X (walls at z 4 and z 6 from y 1 up).
        const auto corridor = [](int x, int y, int z) { return y > 0 && z == 5 && x > 0 && x < 10; };
        const auto side = G::FrontModelOpenSide(2, 2, 5, 0, corridor);
        check(side == 0, "a booth in a corridor faces along it, toward the longer run");
        // No room on any side: a row along X faces +Z, a row along Z +X.
        const auto shut = [](int, int, int) { return false; };
        check(G::FrontModelOpenSide(5, 5, 5, 1 | 2, shut) == 2, "a ceiling row along X faces across it (+Z)");
        check(G::FrontModelOpenSide(5, 5, 5, 4 | 8, shut) == 0, "a ceiling row along Z faces across it (+X)");
        check(G::FrontModelOpenSide(5, 5, 5, 0, shut) == 0, "a lone model without room faces +X");
    }

    // The terrain reliefs (Q-012, TerrainRelief.hpp).
    {
        namespace G = GalaxyEggbert::Game;
        using Microsoft::Xna::Framework::Vector3;
        bool frames = true;
        for (const auto& f : G::kReliefFaceFrames)
        {
            const int cx = f.r[1] * f.u[2] - f.r[2] * f.u[1];
            const int cy = f.r[2] * f.u[0] - f.r[0] * f.u[2];
            const int cz = f.r[0] * f.u[1] - f.r[1] * f.u[0];
            frames = frames && cx == f.n[0] && cy == f.n[1] && cz == f.n[2];
        }
        check(frames, "every relief face frame has right x up = its outward normal (the relief is not mirrored)");
        // The top (+Y) face of a cell with a solid neighbour only at +X: its overhangs into -X, +y (-Z)
        // and -y (+Z) lead into air.
        const auto top = G::ReliefAirSides(4, [](int dx, int dy, int dz) { return dx == 1 && dy == 0 && dz == 0; });
        check(top == (2 | 4 | 8), "a relief hangs only into solid neighbours (top face, solid at +X only)");

        // A relief of two triangles, written and read back: one in the face's square, one overhanging +x.
        G::TerrainRelief relief;
        relief.vertices = {{-0.25f, -0.25f, 0.0f, 0, 0, 1, 0.25f, 0.75f},
                           {0.25f, -0.25f, -0.1f, 0, 0, 1, 0.75f, 0.75f},
                           {0.0f, 0.25f, -0.2f, 0, 0, 1, 0.5f, 0.25f},
                           {0.75f, 0.0f, 0.0f, 0, 0, 1, 0.25f, 0.5f}};
        relief.triangles = {{0, 1, 2}, {1, 3, 2}};
        relief.overhang = {0, 1};
        {
            std::ofstream out("verify_relief.gtr", std::ios::binary);
            const std::uint32_t counts[2] = {4, 2};
            out.write("GTR1", 4);
            out.write(reinterpret_cast<const char*>(counts), sizeof(counts));
            out.write(reinterpret_cast<const char*>(relief.vertices.data()), 4 * sizeof(G::TerrainRelief::Vertex));
            out.write(reinterpret_cast<const char*>(relief.triangles.data()), 2 * sizeof(relief.triangles[0]));
            out.write(reinterpret_cast<const char*>(relief.overhang.data()), 2);
        }
        G::TerrainRelief read;
        check(G::ReadTerrainRelief("verify_relief.gtr", read) && read.vertices.size() == 4 &&
                  read.triangles.size() == 2 && read.overhang[1] == 1 && std::fabs(read.depth - 0.2f) < 1e-6f,
              "a .gtr relief reads back with its depth (0.2)");
        std::remove("verify_relief.gtr");

        const CnaEngine::Graphics::UvRect tile{0.5f, 0.25f, 0.75f, 0.5f};
        const Vector3 center(10.0f, 2.0f, -3.0f);
        auto mesh = G::MakeLitTexturedMeshBuilder();
        G::AppendReliefFace(mesh, read, center, 0, tile, 1, true); // +Z face, the +x overhang into air
        const auto& data = mesh.Mesh();
        // Backing: the square and, toward the solid -x, +y and -y, five more squares (the +x side is air).
        check(data.indices.size() == 3 * (1 + 2 * 6),
              "the overhang into air is left out; the backing reaches only into the solid sides");
        bool inward = true;
        for (std::size_t t = 0; t + 2 < data.indices.size(); t += 3)
        {
            const Vector3 a = data.positions[data.indices[t]], b = data.positions[data.indices[t + 1]],
                          c = data.positions[data.indices[t + 2]];
            const Vector3 cross = Vector3::Cross(b - a, c - a);
            inward = inward && cross.Z < 0.0f; // the builder's front faces turn clockwise seen from outside
        }
        check(inward, "relief and backing triangles face out of the cell (+Z)");
        const Vector3 first = data.positions[data.indices[0]];
        check(std::fabs(first.X - 9.75f) < 1e-5f && std::fabs(first.Y - 1.75f) < 1e-5f &&
                  std::fabs(first.Z - -2.5f) < 1e-5f,
              "a relief vertex lies at the face's plane plus its offset (front-most at the plane)");
        const auto uv = data.texCoords0[data.indices[0]];
        check(std::fabs(uv.X - 0.5625f) < 1e-6f && std::fabs(uv.Y - 0.4375f) < 1e-6f,
              "a relief's texture coordinates map into the tile's rectangle");
        const Vector3 back = data.positions[data.indices[3 * 1]];
        check(std::fabs(back.Z - (-2.5f - 0.2f * 0.9f)) < 1e-5f, "the backing lies at nine tenths of the relief's depth");
        float lowX = 1e9f, highX = -1e9f;
        for (std::size_t k = 3; k < data.indices.size(); ++k)
        {
            lowX = std::min(lowX, data.positions[data.indices[k]].X);
            highX = std::max(highX, data.positions[data.indices[k]].X);
        }
        check(std::fabs(lowX - 9.0f) < 1e-5f && std::fabs(highX - 10.5f) < 1e-5f,
              "the backing reaches half a block into the solid -x side and stops at the air +x edge");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
