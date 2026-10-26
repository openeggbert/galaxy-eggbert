// The faces DirectionalCube tiles show (src/GalaxyEggbert/Game/DirectionalCubeTiles.cpp): which
// side of a fan is its base and which is open, and where flat-colour faces sample their tile.
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>
#include <GalaxyEggbert/Game/MeshParts.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>
#include <GalaxyEggbert/Game/TileAtlas.hpp>

#include <cmath>
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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
