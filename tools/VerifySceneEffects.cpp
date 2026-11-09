// VerifySceneEffects: what the 3D effects draw, decided without a GPU (SceneEffectsLogic.hpp) -- where a
// soft shadow falls and how dark it is, which lights a frame keeps, how a terrain corner is occluded.

#include <GalaxyEggbert/Game/ProceduralMaterial.hpp>
#include <GalaxyEggbert/Game/SceneEffectsLogic.hpp>

#include <cmath>
#include <cstdio>
#include <sstream>

namespace
{
    using GalaxyEggbert::Game::BakeAmbientOcclusion;
    using GalaxyEggbert::Game::GroundBelow;
    using GalaxyEggbert::Game::NearAnyLight;
    using GalaxyEggbert::Game::PickLavaLightCells;
    using GalaxyEggbert::Game::PlaceShadow;
    using GalaxyEggbert::Game::ScenePointLight;
    using GalaxyEggbert::Game::SelectSceneLights;
    using GalaxyEggbert::Worlds::Block;
    using GalaxyEggbert::Worlds::World;
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

    bool Near(float a, float b, float tolerance = 1e-4f) { return std::fabs(a - b) <= tolerance; }

    // A tile that is a whole opaque cube (it occludes and it is solid), found in the registry.
    std::uint16_t SolidCube()
    {
        const std::vector<GalaxyEggbert::Game::TileCoverage> none;
        for (std::uint16_t type = 1; type < 400; ++type)
        {
            const auto& definition = GalaxyEggbert::GetBlockDefinition(type);
            if (definition.collisionMask != 0 && GalaxyEggbert::Game::IsTerrainOccluder(Block::make(type), none))
            {
                return type;
            }
        }
        return 0;
    }

    // The top face of the cell (x, y, z) as four corner vertices facing +Y.
    CnaEngine::Graphics::MeshData TopFace(float x, float y, float z)
    {
        CnaEngine::Graphics::MeshData mesh;
        for (const float dx : {-0.5f, 0.5f})
        {
            for (const float dz : {-0.5f, 0.5f})
            {
                mesh.positions.emplace_back(x + dx, y + 0.5f, z + dz);
                mesh.normals.emplace_back(0.0f, 1.0f, 0.0f);
            }
        }
        return mesh;
    }

    float Grey(const CnaEngine::Graphics::MeshData& mesh, std::size_t i)
    {
        return static_cast<float>(mesh.colors[i].getRProperty()) / 255.0f;
    }
}

int main()
{
    const std::uint16_t solid = SolidCube();
    Check(solid != 0, "the registry has a whole opaque cube");

    // A floor at y 5 over x, z 0..15 (the grid uncentred: centre 0).
    World world(2); // 20 blocks a side
    for (int x = 0; x < 16; ++x)
    {
        for (int z = 0; z < 16; ++z)
        {
            world.setBlock(static_cast<std::uint16_t>(x), 5, static_cast<std::uint16_t>(z), Block::make(solid));
        }
    }

    // --- Ground and shadows -------------------------------------------------------------------------
    const auto ground = GroundBelow(world, 8.0f, 6.0f, 8.0f, 0, 0);
    Check(ground && Near(*ground, 5.5f), "the ground under a point on the floor is its top (5.5)");
    Check(!GroundBelow(world, 60.0f, 6.0f, 8.0f, 0, 0), "no ground outside the world");

    const auto standing = PlaceShadow(world, 8.0f, 5.5f, 8.0f, 0.34f, 0.55f, true, 0, 0);
    Check(standing && Near(standing->center.X, 8.0f) && Near(standing->center.Y, 5.5f) &&
              Near(standing->strength, 0.55f) && Near(standing->radius, 0.34f) && standing->model,
          "standing on it: the shadow under his feet, whole and as wide as he is");

    const auto jumping = PlaceShadow(world, 8.0f, 8.5f, 8.0f, 0.34f, 0.55f, false, 0, 0);
    const auto& light = GalaxyEggbert::Game::kTerrainLight;
    Check(jumping && Near(jumping->center.X, 8.0f + light.directionX / -light.directionY * 3.0f) &&
              Near(jumping->center.Z, 8.0f + light.directionZ / -light.directionY * 3.0f) &&
              Near(jumping->center.Y, 5.5f),
          "three blocks up: the shadow thrown along the sun's way onto the floor");
    Check(jumping && jumping->strength < standing->strength && jumping->radius > standing->radius,
          "higher: paler and wider");
    Check(!PlaceShadow(world, 8.0f, 16.0f, 8.0f, 0.34f, 0.55f, false, 0, 0), "ten blocks up and more: none");

    // --- Lights ---------------------------------------------------------------------------------------
    const Vector3 target(0.0f, 0.0f, 0.0f);
    std::vector<ScenePointLight> blasts;
    for (const float x : {1.0f, 1.5f, 2.0f, 9.0f})
    {
        const Vector3 at(x, 0.0f, 0.0f);
        if (!NearAnyLight(blasts, at, 3.0f))
        {
            blasts.push_back(ScenePointLight{at, Vector3(1.0f, 0.6f, 0.3f), 4.5f});
        }
    }
    Check(blasts.size() == 2, "a blast of three objects together is one light, one apart its own");

    std::vector<Vector3> lava;
    for (int x = 0; x < 12; ++x)
    {
        lava.emplace_back(static_cast<float>(x), 0.0f, 0.0f);
    }
    lava.emplace_back(0.0f, 0.0f, 40.0f); // far off
    const auto chosen = PickLavaLightCells(lava, target);
    bool spaced = true;
    for (std::size_t i = 0; i < chosen.size(); ++i)
    {
        for (std::size_t j = i + 1; j < chosen.size(); ++j)
        {
            spaced = spaced && std::fabs(chosen[i].X - chosen[j].X) >= 3.0f;
        }
    }
    Check(chosen.size() == 4 && spaced && Near(chosen[0].X, 0.0f), "four lava lights, the nearest first, three blocks apart");

    std::vector<ScenePointLight> weak;
    for (int i = 0; i < 10; ++i)
    {
        weak.push_back(ScenePointLight{Vector3(static_cast<float>(i), 0.0f, 0.0f), Vector3(1.0f, 0.4f, 0.1f), 3.5f});
    }
    weak.push_back(ScenePointLight{Vector3(50.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f), 3.0f});
    std::vector<ScenePointLight> strong{ScenePointLight{Vector3(20.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f), 6.0f}};
    const auto lights = SelectSceneLights(target, strong, weak, 8);
    Check(lights.size() == 8 && Near(lights[0].position.X, 20.0f) && Near(lights[1].position.X, 0.0f),
          "eight lights: the strong one first, then the nearest weak ones");
    Check(std::none_of(lights.begin(), lights.end(), [](const ScenePointLight& l) { return l.position.X > 30.0f; }),
          "none beyond 30 blocks");

    // --- Ambient occlusion ----------------------------------------------------------------------------
    // A wall cell on the floor at (9, 6, 8): the floor cell (8, 5, 8)'s top has two corners at x 8.5
    // touching it (one cell of four occluded), the other two at x 7.5 open.
    World room(2);
    room.setBlock(8, 5, 8, Block::make(solid));
    room.setBlock(9, 6, 8, Block::make(solid));
    const std::vector<GalaxyEggbert::Game::TileCoverage> coverage;
    auto top = TopFace(8.0f, 5.0f, 8.0f);
    BakeAmbientOcclusion(top, room, coverage, 0, 0);
    Check(top.colors.size() == 4, "every vertex gets an occlusion");
    Check(Near(Grey(top, 0), 1.0f, 0.01f) && Near(Grey(top, 1), 1.0f, 0.01f), "the corners away from the wall stay lit");
    Check(Near(Grey(top, 2), 0.8f, 0.01f) && Near(Grey(top, 3), 0.8f, 0.01f), "the corners at the wall are darker (0.8)");

    // In a corner of two walls, the corner between them darkest of the face.
    room.setBlock(8, 6, 9, Block::make(solid));
    room.setBlock(9, 6, 9, Block::make(solid));
    auto corner = TopFace(8.0f, 5.0f, 8.0f);
    BakeAmbientOcclusion(corner, room, coverage, 0, 0);
    Check(Near(Grey(corner, 3), 0.56f, 0.01f) && Grey(corner, 3) < Grey(corner, 2), "shut in by three cells: 0.56");

    // A vertex off the grid (a plate, a relief) is left white.
    CnaEngine::Graphics::MeshData plate;
    plate.positions.emplace_back(8.3f, 5.5f, 8.1f);
    plate.normals.emplace_back(0.0f, 1.0f, 0.0f);
    BakeAmbientOcclusion(plate, room, coverage, 0, 0);
    Check(Near(Grey(plate, 0), 1.0f, 0.01f), "a vertex off the grid stays white");

    // --- The Ray Dream materials (materials.txt) ----------------------------------------------------
    std::istringstream text(
        "# a palm\n"
        "material rd_be5f1e\n"
        "colour 0.766 0.5 0.258\n"
        "layer marble 0.844 0.41 0.227 2.9788 7.6187 2.5712 0 0.9681\n"
        "layer wood 0.688 0.293 0 4.096 3.4918 2.8093 1 0.6983\n"
        "layer marble 1 1 1 1 1 1 0 0.5\n"
        "bump spots 0.5 1 0.5873\n"
        "material rd_000000\n"
        "bump grain 0.35 1 0.5\n"
        "nonsense line\n");
    const auto materials = GalaxyEggbert::Game::ReadProceduralMaterials(text);
    const auto* palm = GalaxyEggbert::Game::FindPartMaterial(materials, "bd_plante1a_rd_be5f1e");
    Check(palm != nullptr && palm->hasColour && Near(palm->colour.X, 0.766f), "a part finds its material by its colour suffix");
    Check(palm != nullptr && palm->layerCount == 2 &&
              palm->layers[0].pattern == GalaxyEggbert::Game::MaterialPattern::Marble &&
              palm->layers[1].pattern == GalaxyEggbert::Game::MaterialPattern::Wood && palm->layers[1].axis == 1,
          "two layers kept (a third dropped): marble, then wood along y");
    Check(palm != nullptr && palm->bump == GalaxyEggbert::Game::MaterialBump::Spots && Near(palm->bumpThreshold, 0.5873f),
          "its spots bump");
    const auto* grain = GalaxyEggbert::Game::FindPartMaterial(materials, "x_rd_000000");
    Check(grain != nullptr && !grain->hasColour && grain->bump == GalaxyEggbert::Game::MaterialBump::Grain,
          "a bump alone (an image's: grain)");
    Check(GalaxyEggbert::Game::FindPartMaterial(materials, "bd_plante1a_rd_tex_p08_Assistant_3D") == nullptr &&
              GalaxyEggbert::Game::FindPartMaterial(materials, "no_suffix") == nullptr,
          "a textured part and an unnamed one have none");

    std::printf("%s\n", failures == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
