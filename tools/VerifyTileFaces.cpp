// The faces DirectionalCube tiles show (src/GalaxyEggbert/Game/DirectionalCubeTiles.cpp): which
// side of a fan is its base and which is open, and where flat-colour faces sample their tile.
#include <GalaxyEggbert/BlockTypes.hpp>
#include <GalaxyEggbert/Game/DirectionalCubeTiles.hpp>
#include <GalaxyEggbert/Game/TileAtlas.hpp>

#include <cmath>
#include <iostream>

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

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
