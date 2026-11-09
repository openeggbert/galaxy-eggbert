#pragma once

// The 3D effects' Ultra level: the light the terrain gets from around it, baked once when the terrain
// is built (a voxel global illumination). Each face looks out over its hemisphere with sixteen rays
// through the world's grid: what reaches the open sky brings the sky's light, what meets a block brings
// the light that block's face sends back -- its colour (the atlas tile's mean, linear) times the sun on
// it (if the sun reaches it) and the sky's share. The sum, relative to an open sky's, multiplies the
// sky's irradiance in Ultra's terrain shader (SceneShaders.cpp kTerrainUltraFragment, vGi): a room is
// lit by its walls, grass tints the wall beside it, an overhang is darker underneath.
//
// Written into the vertex colour: rgb the multiplier halved (0..2), alpha the corner's ambient occlusion
// (BakeAmbientOcclusion's levels). Pure: tools/VerifyUltra.cpp tests it.

#include <GalaxyEggbert/Game/SceneEffectsLogic.hpp>
#include <GalaxyEggbert/Game/UltraLogic.hpp>

#include <CnaEngine/Graphics/MeshData.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace GalaxyEggbert::Game
{
    struct TerrainGiSettings
    {
        // Each block type's colour (linear, 0..1), by type; missing types count as mid grey.
        std::vector<Microsoft::Xna::Framework::Vector3> albedo;
        // The way the sunlight goes (unit).
        Microsoft::Xna::Framework::Vector3 sunDirection{0.0f, -1.0f, 0.0f};
        float minimum = 0.45f;  // the least light from around (a shut room is not black: the game is played)
        float maximum = 2.0f;   // the most (a bright bounce)
        int reach = 12;         // cells a ray looks along
    };

    namespace GiDetail
    {
        // Sixteen directions over the hemisphere round +Z, cosine-weighted (Hammersley points mapped to
        // the disc and lifted): each counts the same.
        inline const std::array<Microsoft::Xna::Framework::Vector3, 16>& HemisphereDirections()
        {
            static const auto directions = []
            {
                std::array<Microsoft::Xna::Framework::Vector3, 16> out{};
                for (int i = 0; i < 16; ++i)
                {
                    const float u = (static_cast<float>(i) + 0.5f) / 16.0f;
                    // Van der Corput, base 2.
                    unsigned bits = static_cast<unsigned>(i);
                    bits = (bits << 16u) | (bits >> 16u);
                    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
                    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
                    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
                    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
                    const float v = static_cast<float>(bits) * 2.3283064365386963e-10f;
                    const float radius = std::sqrt(u);
                    const float angle = 6.28318530718f * v;
                    out[static_cast<std::size_t>(i)] = Microsoft::Xna::Framework::Vector3(
                        radius * std::cos(angle), radius * std::sin(angle), std::sqrt(std::max(0.0f, 1.0f - u)));
                }
                return out;
            }();
            return directions;
        }

        struct Grid
        {
            const Worlds::World& world;
            const std::vector<TileCoverage>& coverage;
            int n;

            [[nodiscard]] bool Inside(int x, int y, int z) const noexcept
            {
                return x >= 0 && y >= 0 && z >= 0 && x < n && y < n && z < n;
            }
            [[nodiscard]] std::uint16_t Type(int x, int y, int z) const
            {
                return world
                    .getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z))
                    .type();
            }
            [[nodiscard]] bool Solid(int x, int y, int z) const
            {
                return Inside(x, y, z) &&
                       IsTerrainOccluder(world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                                        static_cast<std::uint16_t>(z)),
                                         coverage);
            }
        };

        enum class Ray
        {
            Sky,  // out over the world's sides or top, or nothing within reach: open
            Hit,  // a solid cell met
            Void, // out under the world's floor: no light
        };

        // From (x, y, z) (grid coordinates) along d (unit): the first solid cell within `reach` and the
        // cell before it (the face it is met through).
        inline Ray March(const Grid& grid, float x, float y, float z, const Microsoft::Xna::Framework::Vector3& d,
                         int reach, int hit[3], int before[3])
        {
            int last[3] = {static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)),
                           static_cast<int>(std::lround(z))};
            for (float t = 0.35f; t <= static_cast<float>(reach); t += 0.35f)
            {
                const int c[3] = {static_cast<int>(std::lround(x + d.X * t)), static_cast<int>(std::lround(y + d.Y * t)),
                                  static_cast<int>(std::lround(z + d.Z * t))};
                if (c[0] == last[0] && c[1] == last[1] && c[2] == last[2])
                {
                    continue;
                }
                if (!grid.Inside(c[0], c[1], c[2]))
                {
                    return c[1] < 0 ? Ray::Void : Ray::Sky;
                }
                if (grid.Solid(c[0], c[1], c[2]))
                {
                    for (int a = 0; a < 3; ++a)
                    {
                        hit[a] = c[a];
                        before[a] = last[a];
                    }
                    return Ray::Hit;
                }
                last[0] = c[0];
                last[1] = c[1];
                last[2] = c[2];
            }
            return Ray::Sky;
        }
    }

    // The light from around a face whose open side is cell (x, y, z) (grid coordinates) and whose
    // normal is `normal` (an axis, unit): the multiplier of an open sky's (1 under the open sky).
    [[nodiscard]] inline Microsoft::Xna::Framework::Vector3 FaceIndirectLight(const Worlds::World& world,
                                                                             const std::vector<TileCoverage>& coverage,
                                                                             int x, int y, int z,
                                                                             const Microsoft::Xna::Framework::Vector3& normal,
                                                                             const TerrainGiSettings& settings)
    {
        using Microsoft::Xna::Framework::Vector3;
        const GiDetail::Grid grid{world, coverage, static_cast<int>(world.blocksPerAxis())};
        // A frame round the normal.
        const Vector3 helper = std::fabs(normal.Y) < 0.9f ? Vector3(0.0f, 1.0f, 0.0f) : Vector3(1.0f, 0.0f, 0.0f);
        Vector3 t = Vector3::Cross(helper, normal);
        t.Normalize();
        const Vector3 b = Vector3::Cross(normal, t);
        const Vector3 toSun(-settings.sunDirection.X, -settings.sunDirection.Y, -settings.sunDirection.Z);
        // The ray starts at the face, not the cell's middle: a little out from it.
        const float ox = static_cast<float>(x) - normal.X * 0.45f;
        const float oy = static_cast<float>(y) - normal.Y * 0.45f;
        const float oz = static_cast<float>(z) - normal.Z * 0.45f;
        float sky = 0.0f;
        Vector3 bounce(0.0f, 0.0f, 0.0f);
        const auto& directions = GiDetail::HemisphereDirections();
        for (const auto& local : directions)
        {
            const Vector3 d(t.X * local.X + b.X * local.Y + normal.X * local.Z, t.Y * local.X + b.Y * local.Y + normal.Y * local.Z,
                            t.Z * local.X + b.Z * local.Y + normal.Z * local.Z);
            int hit[3] = {0, 0, 0};
            int before[3] = {0, 0, 0};
            const auto ray = GiDetail::March(grid, ox, oy, oz, d, settings.reach, hit, before);
            if (ray != GiDetail::Ray::Hit)
            {
                sky += ray == GiDetail::Ray::Sky ? 1.0f : 0.0f;
                continue;
            }
            // The face met: from the hit cell toward the cell before it.
            const Vector3 m(static_cast<float>(before[0] - hit[0]), static_cast<float>(before[1] - hit[1]),
                            static_cast<float>(before[2] - hit[2]));
            const float length = std::sqrt(m.X * m.X + m.Y * m.Y + m.Z * m.Z);
            const Vector3 face = length > 0.0f ? Vector3(m.X / length, m.Y / length, m.Z / length) : normal;
            float sun = std::max(0.0f, face.X * toSun.X + face.Y * toSun.Y + face.Z * toSun.Z);
            if (sun > 0.0f)
            {
                int unused[3];
                int unusedBefore[3];
                if (GiDetail::March(grid, static_cast<float>(before[0]), static_cast<float>(before[1]),
                                    static_cast<float>(before[2]), toSun, 48, unused, unusedBefore) == GiDetail::Ray::Hit)
                {
                    sun = 0.0f; // the sun does not reach it
                }
            }
            const std::uint16_t type = grid.Type(hit[0], hit[1], hit[2]);
            const Vector3 albedo = type < settings.albedo.size() ? settings.albedo[type] : Vector3(0.2f, 0.2f, 0.2f);
            const float light = kUltraSun * sun + kUltraAmbient * 0.5f;
            bounce = Vector3(bounce.X + albedo.X * light, bounce.Y + albedo.Y * light, bounce.Z + albedo.Z * light);
        }
        const float count = static_cast<float>(directions.size());
        const float s = sky / count;
        const float k = 1.0f / (count * kUltraAmbient);
        const auto clampOne = [&](float v) { return std::clamp(v, settings.minimum, settings.maximum); };
        return Vector3(clampOne(s + bounce.X * k), clampOne(s + bounce.Y * k), clampOne(s + bounce.Z * k));
    }

    // The terrain mesh's vertex colours for Ultra: its corners' occlusion (BakeAmbientOcclusion) in
    // alpha, the light from around each face (FaceIndirectLight, halved) in rgb.
    inline void BakeGlobalIllumination(CnaEngine::Graphics::MeshData& mesh, const Worlds::World& world,
                                       const std::vector<TileCoverage>& coverage, int centerX, int centerZ,
                                       const TerrainGiSettings& settings)
    {
        using Microsoft::Xna::Framework::Vector3;
        BakeAmbientOcclusion(mesh, world, coverage, centerX, centerZ);
        std::vector<float> occlusion(mesh.colors.size());
        for (std::size_t i = 0; i < mesh.colors.size(); ++i)
        {
            occlusion[i] = static_cast<float>(mesh.colors[i].getRProperty()) / 255.0f;
            mesh.colors[i] = Microsoft::Xna::Framework::Color(0.5f, 0.5f, 0.5f, occlusion[i]); // an open sky's
        }
        // Each triangle's face: its centroid (inside the face, unlike its corners) a little out along its
        // normal gives the open cell in front of it; the light from around that face for its corners.
        std::unordered_map<std::uint64_t, Vector3> faces;
        const auto& indices = mesh.indices;
        for (std::size_t t = 0; t + 2 < indices.size(); t += 3)
        {
            const std::uint32_t corner[3] = {indices[t], indices[t + 1], indices[t + 2]};
            if (corner[0] >= mesh.positions.size() || corner[1] >= mesh.positions.size() ||
                corner[2] >= mesh.positions.size() || corner[0] >= mesh.normals.size())
            {
                continue;
            }
            const auto& a = mesh.positions[corner[0]];
            const auto& b = mesh.positions[corner[1]];
            const auto& c = mesh.positions[corner[2]];
            const auto& normal = mesh.normals[corner[0]];
            const float d[3] = {normal.X, normal.Y, normal.Z};
            int axis = 0;
            for (int k = 1; k < 3; ++k)
            {
                if (std::fabs(d[k]) > std::fabs(d[axis]))
                {
                    axis = k;
                }
            }
            if (d[axis] == 0.0f)
            {
                continue;
            }
            const float sign = d[axis] > 0.0f ? 1.0f : -1.0f;
            float front[3] = {(a.X + b.X + c.X) / 3.0f + static_cast<float>(centerX), (a.Y + b.Y + c.Y) / 3.0f,
                              (a.Z + b.Z + c.Z) / 3.0f + static_cast<float>(centerZ)};
            front[axis] += sign * 0.5f;
            const int cell[3] = {static_cast<int>(std::lround(front[0])), static_cast<int>(std::lround(front[1])),
                                 static_cast<int>(std::lround(front[2]))};
            const int direction = axis * 2 + (sign > 0.0f ? 1 : 0);
            const std::uint64_t key = (static_cast<std::uint64_t>(static_cast<std::uint16_t>(cell[0])) << 40u) |
                                      (static_cast<std::uint64_t>(static_cast<std::uint16_t>(cell[1])) << 24u) |
                                      (static_cast<std::uint64_t>(static_cast<std::uint16_t>(cell[2])) << 8u) |
                                      static_cast<std::uint64_t>(direction);
            auto found = faces.find(key);
            if (found == faces.end())
            {
                Vector3 n(0.0f, 0.0f, 0.0f);
                (axis == 0 ? n.X : axis == 1 ? n.Y : n.Z) = sign;
                found = faces.emplace(key, FaceIndirectLight(world, coverage, cell[0], cell[1], cell[2], n, settings)).first;
            }
            const auto& light = found->second;
            for (const auto i : corner)
            {
                // A block face's corner (on the half-grid): the mean of the open cells round it on the
                // face's open side (as the occlusion counts them), so the light runs smoothly from face to
                // face; any other vertex (the reliefs' shapes), its face's.
                Vector3 sum(0.0f, 0.0f, 0.0f);
                int count = 0;
                const auto& p = mesh.positions[i];
                const float at[3] = {p.X + static_cast<float>(centerX), p.Y, p.Z + static_cast<float>(centerZ)};
                const auto onHalf = [](float value) { return std::fabs(value - std::floor(value) - 0.5f) < 0.02f; };
                const int u = (axis + 1) % 3;
                const int v = (axis + 2) % 3;
                if (onHalf(at[u]) && onHalf(at[v]))
                {
                    for (const float du : {-0.5f, 0.5f})
                    {
                        for (const float dv : {-0.5f, 0.5f})
                        {
                            int around[3] = {cell[0], cell[1], cell[2]};
                            around[u] = static_cast<int>(std::lround(at[u] + du));
                            around[v] = static_cast<int>(std::lround(at[v] + dv));
                            const GiDetail::Grid grid{world, coverage, static_cast<int>(world.blocksPerAxis())};
                            if (!grid.Inside(around[0], around[1], around[2]) || grid.Solid(around[0], around[1], around[2]))
                            {
                                continue;
                            }
                            const std::uint64_t aroundKey =
                                (static_cast<std::uint64_t>(static_cast<std::uint16_t>(around[0])) << 40u) |
                                (static_cast<std::uint64_t>(static_cast<std::uint16_t>(around[1])) << 24u) |
                                (static_cast<std::uint64_t>(static_cast<std::uint16_t>(around[2])) << 8u) |
                                static_cast<std::uint64_t>(direction);
                            auto near = faces.find(aroundKey);
                            if (near == faces.end())
                            {
                                Vector3 n(0.0f, 0.0f, 0.0f);
                                (axis == 0 ? n.X : axis == 1 ? n.Y : n.Z) = sign;
                                near = faces.emplace(aroundKey, FaceIndirectLight(world, coverage, around[0], around[1],
                                                                                  around[2], n, settings)).first;
                            }
                            sum = Vector3(sum.X + near->second.X, sum.Y + near->second.Y, sum.Z + near->second.Z);
                            ++count;
                        }
                    }
                }
                const Vector3 smooth = count > 0 ? Vector3(sum.X / count, sum.Y / count, sum.Z / count) : light;
                mesh.colors[i] = Microsoft::Xna::Framework::Color(smooth.X * 0.5f, smooth.Y * 0.5f, smooth.Z * 0.5f,
                                                                  occlusion[i]);
            }
        }
    }

    // A relief chunk's light from around, for Ultra's relief shader (SceneShaders.cpp ReliefGi), which
    // its reliefs read by their place and normal as an ambient cube: from the chunk's far cubes as baked
    // (BakeGlobalIllumination's vertex colours, `baked`), each face drawn there its corners' mean; a
    // face not drawn its cell's mean of those drawn, a cell with none the chunk's mean (an open sky's
    // where nothing is drawn). `boxMin` is the chunk's lowest corner, `cells` its cells along an axis.
    // Laid out x + cells * z across and y + cells * face down (face: axis * 2, + 1 toward +), rgb as
    // the vertex colours (the multiplier halved), alpha 255.
    inline constexpr int kReliefGiFaces = 6;
    [[nodiscard]] inline std::vector<Microsoft::Xna::Framework::Color> ReliefChunkGi(const CnaEngine::Graphics::MeshData& baked,
                                                                                    const Microsoft::Xna::Framework::Vector3& boxMin,
                                                                                    int cells)
    {
        using Microsoft::Xna::Framework::Vector3;
        const int n = std::max(1, cells);
        const std::size_t slots = static_cast<std::size_t>(n * n * n * kReliefGiFaces);
        std::vector<Vector3> sum(slots, Vector3(0.0f, 0.0f, 0.0f));
        std::vector<int> count(slots, 0);
        const auto slot = [n](int x, int y, int z, int face)
        {
            return static_cast<std::size_t>(((face * n + y) * n + z) * n + x);
        };
        for (std::size_t i = 0; i < baked.positions.size() && i < baked.normals.size() && i < baked.colors.size(); ++i)
        {
            const auto& normal = baked.normals[i];
            const float d[3] = {normal.X, normal.Y, normal.Z};
            int axis = 0;
            for (int k = 1; k < 3; ++k)
            {
                if (std::fabs(d[k]) > std::fabs(d[axis])) axis = k;
            }
            if (d[axis] == 0.0f) continue;
            // The corner lies on its face: half a cell back along the normal is inside its cell, but on
            // the cell's edges too; the face's own cell is the one a little in from it.
            const auto& p = baked.positions[i];
            const float inward = 0.25f;
            const int x = static_cast<int>(std::floor(p.X - normal.X * inward - boxMin.X));
            const int y = static_cast<int>(std::floor(p.Y - normal.Y * inward - boxMin.Y));
            const int z = static_cast<int>(std::floor(p.Z - normal.Z * inward - boxMin.Z));
            if (x < 0 || y < 0 || z < 0 || x >= n || y >= n || z >= n) continue;
            const int face = axis * 2 + (d[axis] > 0.0f ? 1 : 0);
            const auto& c = baked.colors[i];
            const std::size_t at = slot(x, y, z, face);
            sum[at] = Vector3(sum[at].X + static_cast<float>(c.getRProperty()), sum[at].Y + static_cast<float>(c.getGProperty()),
                              sum[at].Z + static_cast<float>(c.getBProperty()));
            ++count[at];
        }
        Vector3 chunkSum(0.0f, 0.0f, 0.0f);
        int chunkCount = 0;
        for (std::size_t at = 0; at < slots; ++at)
        {
            if (count[at] == 0) continue;
            sum[at] = Vector3(sum[at].X / static_cast<float>(count[at]), sum[at].Y / static_cast<float>(count[at]),
                              sum[at].Z / static_cast<float>(count[at]));
            chunkSum = Vector3(chunkSum.X + sum[at].X, chunkSum.Y + sum[at].Y, chunkSum.Z + sum[at].Z);
            ++chunkCount;
        }
        const Vector3 chunkMean = chunkCount > 0 ? Vector3(chunkSum.X / static_cast<float>(chunkCount),
                                                           chunkSum.Y / static_cast<float>(chunkCount),
                                                           chunkSum.Z / static_cast<float>(chunkCount))
                                                 : Vector3(127.5f, 127.5f, 127.5f);
        std::vector<Microsoft::Xna::Framework::Color> out(slots);
        for (int z = 0; z < n; ++z)
        {
            for (int y = 0; y < n; ++y)
            {
                for (int x = 0; x < n; ++x)
                {
                    Vector3 cellSum(0.0f, 0.0f, 0.0f);
                    int drawn = 0;
                    for (int face = 0; face < kReliefGiFaces; ++face)
                    {
                        const std::size_t at = slot(x, y, z, face);
                        if (count[at] == 0) continue;
                        cellSum = Vector3(cellSum.X + sum[at].X, cellSum.Y + sum[at].Y, cellSum.Z + sum[at].Z);
                        ++drawn;
                    }
                    const Vector3 cellMean = drawn > 0 ? Vector3(cellSum.X / static_cast<float>(drawn),
                                                                 cellSum.Y / static_cast<float>(drawn),
                                                                 cellSum.Z / static_cast<float>(drawn))
                                                       : chunkMean;
                    for (int face = 0; face < kReliefGiFaces; ++face)
                    {
                        const std::size_t at = slot(x, y, z, face);
                        const Vector3 v = count[at] > 0 ? sum[at] : cellMean;
                        const auto byte = [](float value)
                        {
                            return static_cast<int>(std::clamp(std::lround(value), 0L, 255L));
                        };
                        out[static_cast<std::size_t>((y + n * face) * (n * n) + x + n * z)] =
                            Microsoft::Xna::Framework::Color(byte(v.X), byte(v.Y), byte(v.Z), 255);
                    }
                }
            }
        }
        return out;
    }

    // A water tile's body for Ultra's water (SurfaceShaders.cpp): its texels' colour weighted by their
    // alpha, linear, and their mean alpha (its clearness): the colour without the sprite's wavy pattern,
    // which repeated on every cell's face. Sheet RGBA (not premultiplied), the tile's rectangle in texels.
    [[nodiscard]] inline Microsoft::Xna::Framework::Vector4 TileTint(const std::uint8_t* rgba, int width, int x0, int y0, int x1,
                                                                    int y1)
    {
        double r = 0.0, g = 0.0, b = 0.0, alpha = 0.0;
        long count = 0;
        for (int y = std::max(0, y0); y < y1; ++y)
        {
            for (int x = std::max(0, x0); x < x1 && x < width; ++x)
            {
                const std::uint8_t* t = rgba + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                                static_cast<std::size_t>(x)) * 4;
                const double a = t[3] / 255.0;
                r += std::pow(t[0] / 255.0, 2.2) * a;
                g += std::pow(t[1] / 255.0, 2.2) * a;
                b += std::pow(t[2] / 255.0, 2.2) * a;
                alpha += a;
                ++count;
            }
        }
        if (count == 0 || alpha <= 0.0)
        {
            return Microsoft::Xna::Framework::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        return Microsoft::Xna::Framework::Vector4(static_cast<float>(r / alpha), static_cast<float>(g / alpha),
                                                  static_cast<float>(b / alpha), static_cast<float>(alpha / count));
    }

    // A block type's colour for the bake: the mean of its atlas tile's opaque texels (sheet RGBA, the
    // tile's rectangle in texels), linear.
    [[nodiscard]] inline Microsoft::Xna::Framework::Vector3 TileAlbedo(const std::uint8_t* rgba, int width, int x0, int y0,
                                                                      int x1, int y1)
    {
        double r = 0.0, g = 0.0, b = 0.0;
        long count = 0;
        for (int y = std::max(0, y0); y < y1; ++y)
        {
            for (int x = std::max(0, x0); x < x1 && x < width; ++x)
            {
                const std::uint8_t* t = rgba + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                                                static_cast<std::size_t>(x)) * 4;
                if (t[3] < 128)
                {
                    continue;
                }
                r += std::pow(t[0] / 255.0, 2.2);
                g += std::pow(t[1] / 255.0, 2.2);
                b += std::pow(t[2] / 255.0, 2.2);
                ++count;
            }
        }
        if (count == 0)
        {
            return Microsoft::Xna::Framework::Vector3(0.2f, 0.2f, 0.2f);
        }
        return Microsoft::Xna::Framework::Vector3(static_cast<float>(r / count), static_cast<float>(g / count),
                                                  static_cast<float>(b / count));
    }
}
