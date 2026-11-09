// VerifyUltra: what the 3D effects' Ultra level computes on the CPU (UltraLogic.hpp), without a GPU.

#include <GalaxyEggbert/Game/ProceduralMaterial.hpp>
#include <GalaxyEggbert/Game/TerrainGi.hpp>
#include <GalaxyEggbert/Game/UltraLogic.hpp>

#include <sstream>

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
    using Microsoft::Xna::Framework::Vector3;

    int failures = 0;

    void Check(bool ok, const char* what)
    {
        std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
        if (!ok)
        {
            ++failures;
        }
    }

    // The table's entry for the corner (r, g, b) of the 32-cube.
    const std::uint8_t* Entry(const std::vector<std::uint8_t>& table, int r, int g, int b)
    {
        constexpr int n = GalaxyEggbert::Game::kGradeTableSize;
        return &table[(static_cast<std::size_t>(g) * (n * n) + static_cast<std::size_t>(b) * n + static_cast<std::size_t>(r)) * 4];
    }
}

int main()
{
    using GalaxyEggbert::Game::AcesFitted;
    using GalaxyEggbert::Game::BuildGradeTable;
    constexpr int n = GalaxyEggbert::Game::kGradeTableSize;

    // The tonemapping: white at the usual exposure (1.35) comes out near white once divided by 0.79, mid
    // grey where it was (display 0.46 -> 0.46), and the curve never decreases.
    const auto display = [](float linear) { return std::pow(std::min(AcesFitted(linear * 1.35f) / 0.79f, 1.0f), 1.0f / 2.2f); };
    Check(std::fabs(display(1.0f) - 0.952f) < 0.01f, "white at the usual exposure comes out at 0.95");
    Check(std::fabs(display(std::pow(0.46f, 2.2f)) - 0.46f) < 0.03f, "mid grey stays mid grey");
    bool rising = true;
    for (float v = 0.0f; v < 16.0f; v += 0.01f)
    {
        rising = rising && AcesFitted(v + 0.01f) >= AcesFitted(v);
    }
    Check(rising && AcesFitted(0.0f) == 0.0f && AcesFitted(1000.0f) <= 1.0f, "the curve rises from 0 and stays under 1");

    // The grade: black stays black, white near white, greys near grey, and it keeps every channel's order.
    const auto table = BuildGradeTable(Vector3(0.3f, 0.5f, 0.9f));
    Check(table.size() == static_cast<std::size_t>(n * n * n * 4), "32 x 32 x 32 entries of four bytes");
    const auto* black = Entry(table, 0, 0, 0);
    Check(black[0] <= 2 && black[1] <= 2 && black[2] <= 2 && black[3] == 255, "black stays black");
    const auto* white = Entry(table, n - 1, n - 1, n - 1);
    Check(white[0] >= 240 && white[1] >= 240 && white[2] >= 240, "white stays near white");
    bool greys = true;
    bool ordered = true;
    for (int i = 0; i < n; ++i)
    {
        const auto* grey = Entry(table, i, i, i);
        greys = greys && std::abs(grey[0] - grey[1]) <= 12 && std::abs(grey[2] - grey[1]) <= 12;
        if (i > 0)
        {
            const auto* below = Entry(table, i - 1, 10, 20);
            const auto* here = Entry(table, i, 10, 20);
            ordered = ordered && here[0] >= below[0];
        }
    }
    Check(greys, "greys stay near grey (a light tint only)");
    Check(ordered, "more red in stays more red out");
    const auto dull = BuildGradeTable(Vector3(0.0f, 0.0f, 0.0f));
    Check(Entry(dull, 16, 16, 16)[0] != 0, "a black sky grades without a tint");

    // The sky: an even grey sky lights every way alike, at the ambient the other levels give; a sky
    // bright above and dark below lights from above.
    using GalaxyEggbert::Game::BuildSkyEnvironment;
    using GalaxyEggbert::Game::kUltraAmbient;
    using GalaxyEggbert::Game::SkyIrradiance;
    const auto even = BuildSkyEnvironment(nullptr, 0, 0, 4.0f, Vector3(0.5f, 0.5f, 0.5f));
    Check(even.width == 128 && even.height == 96 && even.levels.size() == 8 && even.levels.back().size() == 4,
          "the sky's levels: 128 x 96 halved down to 1x1 (8)");
    bool alike = true;
    for (const Vector3 n : {Vector3(0, 1, 0), Vector3(0, -1, 0), Vector3(1, 0, 0), Vector3(0, 0, -1)})
    {
        const auto e = SkyIrradiance(even, n);
        alike = alike && std::fabs(e.X - kUltraAmbient) < 0.01f && std::fabs(e.Z - kUltraAmbient) < 0.01f;
    }
    Check(alike, "an even sky lights every way at the ambient");
    std::vector<std::uint8_t> picture(64 * 48 * 4, 255);
    for (int y = 24; y < 48; ++y)
    {
        for (int x = 0; x < 64; ++x)
        {
            auto* t = &picture[static_cast<std::size_t>((y * 64 + x) * 4)];
            t[0] = t[1] = 40;
            t[2] = 20;
        }
    }
    const auto split = BuildSkyEnvironment(picture.data(), 64, 48, 4.0f, Vector3());
    const auto up = SkyIrradiance(split, Vector3(0, 1, 0));
    const auto down = SkyIrradiance(split, Vector3(0, -1, 0));
    Check(up.Y > down.Y * 2.0f, "a sky bright above lights from above");
    Check(down.Z < down.X, "the dark ground's colour, partly (its blue lower)");
    const auto horizon = GalaxyEggbert::Game::SkyUv(Vector3(0, 0, -1), 4.0f);
    const auto zenith = GalaxyEggbert::Game::SkyUv(Vector3(0, 1, 0), 4.0f);
    Check(std::fabs(horizon.Y - 0.5f) < 1e-5f && std::fabs(horizon.X) < 1e-5f && zenith.Y < 0.0f,
          "the picture's middle is the horizon ahead; the zenith above its top");

    // A part's surface from its Ray Dream shader.
    using GalaxyEggbert::Game::SurfaceFromRayDream;
    const auto plain = SurfaceFromRayDream(0.5f, 0.5f, 0.0f);
    const auto shiny = SurfaceFromRayDream(0.85f, 0.9f, 0.6f);
    Check(std::fabs(plain.roughness - 0.575f) < 1e-4f && plain.metalness == 0.0f && std::fabs(plain.specular - 1.0f) < 1e-4f,
          "Ray Dream's defaults: rough 0.575, no metal, 4 %");
    Check(shiny.roughness < 0.3f && shiny.metalness > 0.7f, "sharp and reflective: smooth metal");
    std::istringstream text("material rd_ffcc00\nsurface 0.8468 0.8978 0.6019 1 0.5 0\n");
    const auto materials = GalaxyEggbert::Game::ReadProceduralMaterials(text);
    const auto* gold = GalaxyEggbert::Game::FindPartMaterial(materials, "coin_rd_ffcc00");
    Check(gold != nullptr && gold->hasSurface && !gold->hasColour && std::fabs(gold->reflection - 0.6019f) < 1e-4f &&
              gold->glow.X == 1.0f,
          "a surface line: highlight, shininess, reflection, glow");

    // The light from around (TerrainGi.hpp): a floor under the open sky gets the sky's (1); under a roof
    // a little above it, much less; beside a green wall, a green tint.
    {
        using GalaxyEggbert::Worlds::Block;
        using GalaxyEggbert::Worlds::World;
        const std::vector<GalaxyEggbert::Game::TileCoverage> none;
        std::uint16_t solid = 0;
        for (std::uint16_t type = 1; type < 400 && solid == 0; ++type)
        {
            if (GalaxyEggbert::GetBlockDefinition(type).collisionMask != 0 &&
                GalaxyEggbert::Game::IsTerrainOccluder(Block::make(type), none))
            {
                solid = type;
            }
        }
        Check(solid != 0, "the registry has a whole opaque cube");
        World world(2); // 20 blocks a side
        for (int x = 0; x < 20; ++x)
            for (int z = 0; z < 20; ++z)
                world.setBlock(static_cast<std::uint16_t>(x), 2, static_cast<std::uint16_t>(z), Block::make(solid));
        GalaxyEggbert::Game::TerrainGiSettings settings;
        settings.albedo.assign(1024, Vector3(0.3f, 0.3f, 0.3f));
        settings.sunDirection = Vector3(0.25f, -0.9f, -0.35f);
        settings.sunDirection.Normalize();
        settings.minimum = 0.0f;
        const Vector3 up(0.0f, 1.0f, 0.0f);
        const auto open = GalaxyEggbert::Game::FaceIndirectLight(world, none, 10, 3, 10, up, settings);
        Check(std::fabs(open.X - 1.0f) < 0.05f && std::fabs(open.Z - 1.0f) < 0.05f, "an open floor gets the open sky's light (1)");
        // A roof two cells over a 7 x 7 patch.
        for (int x = 7; x < 14; ++x)
            for (int z = 7; z < 14; ++z)
                world.setBlock(static_cast<std::uint16_t>(x), 5, static_cast<std::uint16_t>(z), Block::make(solid));
        const auto covered = GalaxyEggbert::Game::FaceIndirectLight(world, none, 10, 3, 10, up, settings);
        Check(covered.X < 0.5f && covered.X > 0.0f, "under a roof: less than half (the roof's underside, the sides open)");
        // A green wall beside an open floor: the floor's light turns green.
        GalaxyEggbert::Game::TerrainGiSettings green = settings;
        green.albedo[solid] = Vector3(0.05f, 0.6f, 0.05f);
        for (int y = 3; y < 9; ++y)
            for (int z = 0; z < 20; ++z)
                world.setBlock(2, static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z), Block::make(solid));
        const auto beside = GalaxyEggbert::Game::FaceIndirectLight(world, none, 3, 3, 3, up, green);
        Check(beside.Y > beside.X + 0.02f, "beside a green wall the light from around is greener");
        // The bake: a face's corners carry it halved in rgb, the occlusion in alpha.
        CnaEngine::Graphics::MeshData face;
        for (const float dx : {-0.5f, 0.5f})
            for (const float dz : {-0.5f, 0.5f})
            {
                face.positions.emplace_back(16.0f + dx, 2.5f, 16.0f + dz);
                face.normals.emplace_back(0.0f, 1.0f, 0.0f);
            }
        face.indices = {0, 1, 2, 2, 1, 3};
        GalaxyEggbert::Game::BakeGlobalIllumination(face, world, none, 0, 0, settings);
        Check(face.colors.size() == 4 && std::abs(static_cast<int>(face.colors[0].getRProperty()) - 128) <= 8 &&
                  face.colors[0].getAProperty() == 255,
              "an open face's corners: the open sky's light halved (128), no occlusion (255)");
    }

    std::printf("%s\n", failures == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
