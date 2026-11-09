#pragma once

// The 3D effects' decisions that need no GPU (SceneFrame.hpp, the Setup's "3D effects"): where a soft
// shadow falls, which lights a frame keeps, how shut in a terrain corner is. GalaxyEggbertGameScene.cpp
// and TerrainRenderer use them; VerifySceneEffects checks them on small worlds.

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/Game/SceneFrame.hpp>
#include <GalaxyEggbert/Game/TerrainLighting.hpp>
#include <GalaxyEggbert/Game/TerrainOcclusion.hpp>
#include <GalaxyEggbert/Game/TileCoverage.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <CnaEngine/Graphics/MeshData.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace GalaxyEggbert::Game
{
    // The top of the highest solid (or water) cell under (x, z) at or below `fromY`, in render space (a
    // cell's centre at its integer coordinates; x and z are the grid's minus centerX / centerZ), within
    // 12 blocks; none outside the world.
    [[nodiscard]] inline std::optional<float> GroundBelow(const Worlds::World& world, float x, float fromY, float z,
                                                          int centerX, int centerZ)
    {
        const long n = static_cast<long>(world.blocksPerAxis());
        const long gx = std::lround(x) + centerX;
        const long gz = std::lround(z) + centerZ;
        if (gx < 0 || gz < 0 || gx >= n || gz >= n || !std::isfinite(fromY))
        {
            return std::nullopt;
        }
        const long top = std::min<long>(n - 1, static_cast<long>(std::floor(fromY - 0.5f + 0.05f)));
        for (long gy = top; gy >= 0 && gy >= top - 12; --gy)
        {
            const auto& definition = GalaxyEggbert::GetBlockDefinition(
                world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy), static_cast<std::uint16_t>(gz))
                    .type());
            if (definition.collisionMask != 0 || definition.water)
            {
                return static_cast<float>(gy) + 0.5f;
            }
        }
        return std::nullopt;
    }

    // A soft round shadow for something whose feet are at (x, feetY, z): thrown away from the sun
    // (kTerrainLight) onto the ground where it lands, paler and wider the higher it is, gone at 10 blocks.
    [[nodiscard]] inline std::optional<SceneShadow> PlaceShadow(const Worlds::World& world, float x, float feetY, float z,
                                                                float radius, float darkness, bool model, int centerX,
                                                                int centerZ)
    {
        const auto under = GroundBelow(world, x, feetY, z, centerX, centerZ);
        if (!under)
        {
            return std::nullopt;
        }
        const float height = std::max(0.0f, feetY - *under);
        constexpr float kReach = 10.0f;
        if (height >= kReach)
        {
            return std::nullopt;
        }
        const float sunX = kTerrainLight.directionX / -kTerrainLight.directionY;
        const float sunZ = kTerrainLight.directionZ / -kTerrainLight.directionY;
        const float sx = x + sunX * height;
        const float sz = z + sunZ * height;
        const float ground = GroundBelow(world, sx, feetY, sz, centerX, centerZ).value_or(*under);
        const float fade = 1.0f - height / kReach;
        SceneShadow shadow;
        shadow.center = Microsoft::Xna::Framework::Vector3(sx, ground, sz);
        shadow.radius = radius * (1.0f + height * 0.08f);
        shadow.strength = darkness * fade * std::sqrt(fade);
        shadow.model = model;
        return shadow;
    }

    // Whether `p` lies within `within` blocks of one of `lights`.
    [[nodiscard]] inline bool NearAnyLight(const std::vector<ScenePointLight>& lights,
                                           const Microsoft::Xna::Framework::Vector3& p, float within)
    {
        return std::any_of(lights.begin(), lights.end(), [&](const ScenePointLight& light) {
            const float dx = light.position.X - p.X, dy = light.position.Y - p.Y, dz = light.position.Z - p.Z;
            return dx * dx + dy * dy + dz * dz < within * within;
        });
    }

    // The lava cells that light the frame: the nearest to `target` within `reach`, none closer than
    // `spacing` (across X and Z) to one already taken, at most `count`.
    [[nodiscard]] inline std::vector<Microsoft::Xna::Framework::Vector3> PickLavaLightCells(
        std::vector<Microsoft::Xna::Framework::Vector3> cells, const Microsoft::Xna::Framework::Vector3& target,
        std::size_t count = 4, float spacing = 3.0f, float reach = 18.0f)
    {
        const auto far = [&](const Microsoft::Xna::Framework::Vector3& p) {
            const float dx = p.X - target.X, dy = p.Y - target.Y, dz = p.Z - target.Z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };
        std::stable_sort(cells.begin(), cells.end(), [&](const auto& a, const auto& b) { return far(a) < far(b); });
        std::vector<Microsoft::Xna::Framework::Vector3> chosen;
        for (const auto& cell : cells)
        {
            if (chosen.size() >= count || far(cell) > reach)
            {
                break;
            }
            if (std::none_of(chosen.begin(), chosen.end(), [&](const auto& c) {
                    const float dx = c.X - cell.X, dz = c.Z - cell.Z;
                    return dx * dx + dz * dz < spacing * spacing;
                }))
            {
                chosen.push_back(cell);
            }
        }
        return chosen;
    }

    // The frame's lights: the strong ones (explosions, the lightning) first, then the weak (lava,
    // electrodes), each list nearest to `target` first, none farther than `reach`, at most `count`.
    [[nodiscard]] inline std::vector<ScenePointLight> SelectSceneLights(const Microsoft::Xna::Framework::Vector3& target,
                                                                        std::vector<ScenePointLight> strong,
                                                                        std::vector<ScenePointLight> weak,
                                                                        std::size_t count, float reach = 30.0f)
    {
        const auto far = [&](const ScenePointLight& light) {
            const float dx = light.position.X - target.X, dy = light.position.Y - target.Y,
                        dz = light.position.Z - target.Z;
            return std::sqrt(dx * dx + dy * dy + dz * dz);
        };
        const auto nearer = [&](const ScenePointLight& a, const ScenePointLight& b) { return far(a) < far(b); };
        std::stable_sort(strong.begin(), strong.end(), nearer);
        std::stable_sort(weak.begin(), weak.end(), nearer);
        std::vector<ScenePointLight> lights;
        for (const auto* list : {&strong, &weak})
        {
            for (const auto& light : *list)
            {
                if (lights.size() < count && far(light) < reach)
                {
                    lights.push_back(light);
                }
            }
        }
        return lights;
    }

    // The ambient occlusion of a terrain mesh, as a vertex colour (grey level): a cube face's corner is
    // darker the more of the four cells round it, in the layer in front of the face, shut the light
    // out (IsTerrainOccluder). Only the axis-aligned faces of whole cells take it (corners on the grid);
    // every other vertex stays white. Positions in render space (the grid's minus centerX / centerZ).
    inline void BakeAmbientOcclusion(CnaEngine::Graphics::MeshData& mesh, const Worlds::World& world,
                                     const std::vector<TileCoverage>& tileCoverage, int centerX, int centerZ)
    {
        constexpr float kLevels[5] = {1.0f, 0.8f, 0.66f, 0.56f, 0.5f};
        const int n = static_cast<int>(world.blocksPerAxis());
        mesh.colors.assign(mesh.positions.size(), Microsoft::Xna::Framework::Color::White);
        const auto onHalf = [](float value) {
            const float f = value - std::floor(value);
            return std::fabs(f - 0.5f) < 0.02f;
        };
        for (std::size_t i = 0; i < mesh.positions.size() && i < mesh.normals.size(); ++i)
        {
            const auto& p = mesh.positions[i];
            const auto& normal = mesh.normals[i];
            const float c[3] = {p.X, p.Y, p.Z};
            const float d[3] = {normal.X, normal.Y, normal.Z};
            int axis = -1;
            for (int a = 0; a < 3; ++a)
            {
                if (std::fabs(d[a]) > 0.99f)
                {
                    axis = a;
                }
            }
            if (axis < 0 || !onHalf(c[0]) || !onHalf(c[1]) || !onHalf(c[2]))
            {
                continue;
            }
            float front[3] = {c[0], c[1], c[2]};
            front[axis] += d[axis] > 0.0f ? 0.5f : -0.5f;
            const int u = (axis + 1) % 3;
            const int v = (axis + 2) % 3;
            int solid = 0;
            for (const float du : {-0.5f, 0.5f})
            {
                for (const float dv : {-0.5f, 0.5f})
                {
                    float cell[3] = {front[0], front[1], front[2]};
                    cell[u] += du;
                    cell[v] += dv;
                    const int gx = static_cast<int>(std::lround(cell[0])) + centerX;
                    const int gy = static_cast<int>(std::lround(cell[1]));
                    const int gz = static_cast<int>(std::lround(cell[2])) + centerZ;
                    if (gx >= 0 && gy >= 0 && gz >= 0 && gx < n && gy < n && gz < n &&
                        IsTerrainOccluder(world.getBlock(static_cast<std::uint16_t>(gx), static_cast<std::uint16_t>(gy),
                                                         static_cast<std::uint16_t>(gz)),
                                          tileCoverage))
                    {
                        ++solid;
                    }
                }
            }
            const float level = kLevels[solid];
            mesh.colors[i] = Microsoft::Xna::Framework::Color(level, level, level, 1.0f);
        }
    }
}
