#pragma once

// The 3D effects' Ultra level: the Ray Dream models drawn smoother (WorldRenderer: each part's
// subdivided copy, made the first time Ultra draws it). PN triangles (Vlachos et al., "Curved PN
// Triangles"): each triangle becomes a cubic patch through its corners, bent toward their normals, and is
// cut into `level` x `level` smaller ones on it. Round parts (Blupi's body, a ball, a barrel) lose their
// facets; hard edges stay hard:
//
//  - the patch is bent by each corner's normal averaged over every vertex at its position, so the two
//    triangles of an edge bend it alike and no crack opens between them;
//  - a corner where the normals of its vertices part by more than kCreaseDegrees (a box's corner, a
//    rim) keeps its edges straight;
//  - the new vertices' shading normals, texture coordinates and skin are their corners' blended (the
//    skin: every corner's bones weighted, the four strongest kept).
//
// Generic over the vertex layout (byte offsets of position, normal, texture coordinates, bone weights and
// indices); pure, tools/VerifyUltra.cpp tests it.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace GalaxyEggbert::Game
{
    struct SubdivisionLayout
    {
        int stride = 0;
        int position = -1; // three floats
        int normal = -1;   // three floats
        int uv = -1;       // two floats
        int weights = -1;  // four floats
        int bones = -1;    // four bytes
    };

    struct SubdividedMesh
    {
        std::vector<std::uint8_t> vertices; // `layout.stride` bytes each
        int vertexCount = 0;
        std::vector<std::uint32_t> indices; // a triangle list
    };

    inline constexpr float kCreaseDegrees = 40.0f;

    namespace SubdivisionDetail
    {
        struct V3
        {
            float x = 0.0f, y = 0.0f, z = 0.0f;
        };
        inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
        inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
        inline V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
        inline float Dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
        inline V3 Normalized(V3 a)
        {
            const float l = std::sqrt(Dot(a, a));
            return l > 1e-12f ? a * (1.0f / l) : V3{0.0f, 1.0f, 0.0f};
        }

        inline V3 Read3(const std::uint8_t* v, int offset)
        {
            V3 r;
            std::memcpy(&r.x, v + offset, 4);
            std::memcpy(&r.y, v + offset + 4, 4);
            std::memcpy(&r.z, v + offset + 8, 4);
            return r;
        }
        inline void Write3(std::uint8_t* v, int offset, V3 a)
        {
            std::memcpy(v + offset, &a.x, 4);
            std::memcpy(v + offset + 4, &a.y, 4);
            std::memcpy(v + offset + 8, &a.z, 4);
        }
        inline std::tuple<long, long, long> Key(V3 p)
        {
            // Positions a ten-thousandth apart are one.
            return {std::lround(p.x * 1e4f), std::lround(p.y * 1e4f), std::lround(p.z * 1e4f)};
        }
    }

    // Subdivides a triangle list (`indices` into `vertices`, `vertexCount` of `layout.stride` bytes) into
    // level x level triangles each on its PN patch. level 1 copies it.
    [[nodiscard]] inline SubdividedMesh SubdivideMesh(const std::uint8_t* vertices, int vertexCount,
                                                      const std::vector<std::uint32_t>& indices,
                                                      const SubdivisionLayout& layout, int level)
    {
        using namespace SubdivisionDetail;
        SubdividedMesh out;
        level = std::max(1, level);
        const int stride = layout.stride;
        if (stride <= 0 || layout.position < 0 || vertexCount <= 0)
        {
            return out;
        }
        // Each position's normal averaged over its vertices, and whether they part (a crease).
        std::map<std::tuple<long, long, long>, std::pair<V3, std::vector<V3>>> byPosition;
        const auto vertex = [&](std::uint32_t i) { return vertices + static_cast<std::size_t>(i) * static_cast<std::size_t>(stride); };
        for (int i = 0; i < vertexCount; ++i)
        {
            const auto p = Read3(vertex(static_cast<std::uint32_t>(i)), layout.position);
            const V3 n = layout.normal >= 0 ? Normalized(Read3(vertex(static_cast<std::uint32_t>(i)), layout.normal)) : V3{};
            auto& entry = byPosition[Key(p)];
            entry.first = entry.first + n;
            entry.second.push_back(n);
        }
        const float creaseCos = std::cos(kCreaseDegrees * 3.14159265f / 180.0f);
        std::map<std::tuple<long, long, long>, std::pair<V3, bool>> geometry; // averaged normal, sharp
        for (const auto& [key, entry] : byPosition)
        {
            const V3 average = Normalized(entry.first);
            bool sharp = layout.normal < 0;
            for (const auto& n : entry.second)
            {
                sharp = sharp || Dot(n, average) < creaseCos;
            }
            geometry[key] = {average, sharp};
        }

        const int perTriangle = (level + 1) * (level + 2) / 2;
        out.vertices.reserve(indices.size() / 3 * static_cast<std::size_t>(perTriangle) * static_cast<std::size_t>(stride));
        for (std::size_t t = 0; t + 2 < indices.size(); t += 3)
        {
            const std::uint32_t c[3] = {indices[t], indices[t + 1], indices[t + 2]};
            if (c[0] >= static_cast<std::uint32_t>(vertexCount) || c[1] >= static_cast<std::uint32_t>(vertexCount) ||
                c[2] >= static_cast<std::uint32_t>(vertexCount))
            {
                continue;
            }
            V3 p[3], n[3];
            bool sharp[3];
            for (int k = 0; k < 3; ++k)
            {
                p[k] = Read3(vertex(c[k]), layout.position);
                const auto& g = geometry[Key(p[k])];
                n[k] = g.first;
                sharp[k] = g.second;
            }
            // The control points: b300 = P1 (w), b030 = P2 (u), b003 = P3 (v); an edge's two inner
            // points pulled onto the corners' tangent planes, or left on the edge where it is creased.
            const auto inner = [&](int i, int j)
            {
                if (sharp[i] || sharp[j])
                {
                    return (p[i] * 2.0f + p[j]) * (1.0f / 3.0f);
                }
                const float w = Dot(p[j] - p[i], n[i]);
                return (p[i] * 2.0f + p[j] - n[i] * w) * (1.0f / 3.0f);
            };
            const V3 b210 = inner(0, 1), b120 = inner(1, 0), b021 = inner(1, 2), b012 = inner(2, 1), b102 = inner(2, 0),
                     b201 = inner(0, 2);
            const V3 e = (b210 + b120 + b021 + b012 + b102 + b201) * (1.0f / 6.0f);
            const V3 centre = (p[0] + p[1] + p[2]) * (1.0f / 3.0f);
            const V3 b111 = e + (e - centre) * 0.5f;
            const std::uint32_t base = static_cast<std::uint32_t>(out.vertexCount);
            // The grid's points: row r (0..level) along P1 -> P3, column s along it.
            for (int r = 0; r <= level; ++r)
            {
                for (int s = 0; s <= level - r; ++s)
                {
                    const float v = static_cast<float>(r) / static_cast<float>(level); // toward P3
                    const float u = static_cast<float>(s) / static_cast<float>(level); // toward P2
                    const float w = 1.0f - u - v;                                       // toward P1
                    const V3 position = p[0] * (w * w * w) + p[1] * (u * u * u) + p[2] * (v * v * v) +
                                        b210 * (3.0f * w * w * u) + b120 * (3.0f * w * u * u) + b201 * (3.0f * w * w * v) +
                                        b021 * (3.0f * u * u * v) + b102 * (3.0f * w * v * v) + b012 * (3.0f * u * v * v) +
                                        b111 * (6.0f * w * u * v);
                    const float bary[3] = {w, u, v};
                    // The new vertex: corner 0's bytes, then its blended attributes.
                    const std::size_t at = out.vertices.size();
                    out.vertices.resize(at + static_cast<std::size_t>(stride));
                    std::uint8_t* dst = out.vertices.data() + at;
                    const int nearest = (w >= u && w >= v) ? 0 : (u >= v ? 1 : 2);
                    std::memcpy(dst, vertex(c[nearest]), static_cast<std::size_t>(stride));
                    Write3(dst, layout.position, position);
                    if (layout.normal >= 0)
                    {
                        V3 shading{};
                        for (int k = 0; k < 3; ++k)
                        {
                            shading = shading + Normalized(Read3(vertex(c[k]), layout.normal)) * bary[k];
                        }
                        Write3(dst, layout.normal, Normalized(shading));
                    }
                    if (layout.uv >= 0)
                    {
                        float uvSum[2] = {0.0f, 0.0f};
                        for (int k = 0; k < 3; ++k)
                        {
                            float uv[2];
                            std::memcpy(uv, vertex(c[k]) + layout.uv, 8);
                            uvSum[0] += uv[0] * bary[k];
                            uvSum[1] += uv[1] * bary[k];
                        }
                        std::memcpy(dst + layout.uv, uvSum, 8);
                    }
                    if (layout.weights >= 0 && layout.bones >= 0)
                    {
                        // Every corner's bones, weighted by how near it is; the four strongest kept.
                        std::map<int, float> influence;
                        for (int k = 0; k < 3; ++k)
                        {
                            float weight[4];
                            std::memcpy(weight, vertex(c[k]) + layout.weights, 16);
                            const std::uint8_t* bone = vertex(c[k]) + layout.bones;
                            for (int b = 0; b < 4; ++b)
                            {
                                if (weight[b] > 0.0f)
                                {
                                    influence[bone[b]] += weight[b] * bary[k];
                                }
                            }
                        }
                        std::vector<std::pair<float, int>> strongest;
                        for (const auto& [bone, weight] : influence)
                        {
                            strongest.emplace_back(weight, bone);
                        }
                        std::sort(strongest.begin(), strongest.end(), [](const auto& a, const auto& b) {
                            return a.first != b.first ? a.first > b.first : a.second < b.second;
                        });
                        float weight[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                        std::uint8_t bone[4] = {0, 0, 0, 0};
                        float total = 0.0f;
                        for (std::size_t b = 0; b < 4 && b < strongest.size(); ++b)
                        {
                            weight[b] = strongest[b].first;
                            bone[b] = static_cast<std::uint8_t>(strongest[b].second);
                            total += weight[b];
                        }
                        if (total > 0.0f)
                        {
                            for (float& value : weight)
                            {
                                value /= total;
                            }
                            std::memcpy(dst + layout.weights, weight, 16);
                            std::memcpy(dst + layout.bones, bone, 4);
                        }
                    }
                    ++out.vertexCount;
                }
            }
            // The grid's triangles, wound as the original (P1, P2, P3).
            const auto index = [&](int r, int s)
            {
                // Rows before r hold (level + 1) + level + ... points.
                int offset = 0;
                for (int k = 0; k < r; ++k)
                {
                    offset += level + 1 - k;
                }
                return base + static_cast<std::uint32_t>(offset + s);
            };
            for (int r = 0; r < level; ++r)
            {
                for (int s = 0; s < level - r; ++s)
                {
                    out.indices.insert(out.indices.end(), {index(r, s), index(r, s + 1), index(r + 1, s)});
                    if (s + 1 < level - r)
                    {
                        out.indices.insert(out.indices.end(), {index(r, s + 1), index(r + 1, s + 1), index(r + 1, s)});
                    }
                }
            }
        }
        return out;
    }
}
