#pragma once

#include "BoltPaths.hpp"
#include "TerrainLighting.hpp"

#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/TextureAtlas.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace GalaxyEggbert::Game
{
    // The lightning in 3D (owner 2026-11-04, Q-002). The original has no scene for the bolt, only its
    // painted frames 305-307; here each frame is thin tubes along that frame's centre lines
    // (BoltPaths.hpp, traced from the sprites), as thick as the painted bolt, in its white core's
    // colour and evenly bright (the normals face the light: it glows rather than shades). A cell's
    // bolt is turned about the vertical by `yawRadians` so a column of them is not all in one plane;
    // its ends lie on the cell's axis, so the cells of a column join.
    inline constexpr int kBoltSides = 6;

    // Ring point k of a tube about the segment a -> b (in the frame's plane, z 0) at radius r.
    inline void AppendBoltCell(CnaEngine::Graphics::MeshBuilder& mesh, const Microsoft::Xna::Framework::Vector3& center,
                               int frame, const CnaEngine::Graphics::UvRect& tile, float yawRadians)
    {
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        frame = std::clamp(frame, 0, 2);
        const auto& core = kBoltCoreTexel[static_cast<std::size_t>(frame)];
        const Vector2 uv(tile.u0 + core[0] * (tile.u1 - tile.u0), tile.v0 + core[1] * (tile.v1 - tile.v0));
        const Vector3 lit = Vector3::Normalize(
            Vector3(-kTerrainLight.directionX, -kTerrainLight.directionY, -kTerrainLight.directionZ));
        const float c = std::cos(yawRadians), s = std::sin(yawRadians);
        const auto place = [&](const Vector3& p) { return Vector3(center.X + p.X * c + p.Z * s, center.Y + p.Y, center.Z - p.X * s + p.Z * c); };
        const auto& range = kBoltFrames[static_cast<std::size_t>(frame)];
        for (int line = range.first; line < range.first + range.count; ++line)
        {
            const auto& poly = kBoltPolylines[static_cast<std::size_t>(line)];
            for (int k = poly.first; k + 1 < poly.first + poly.count; ++k)
            {
                const auto& a = kBoltPoints[static_cast<std::size_t>(k)];
                const auto& b = kBoltPoints[static_cast<std::size_t>(k + 1)];
                const Vector3 pa(a.x, a.y, 0.0f), pb(b.x, b.y, 0.0f);
                Vector3 dir = pb - pa;
                if (dir.Length() < 1e-5f)
                {
                    continue;
                }
                dir = Vector3::Normalize(dir);
                const Vector3 n1(0.0f, 0.0f, 1.0f);
                const Vector3 n2 = Vector3::Normalize(Vector3::Cross(dir, n1));
                std::uint32_t ring[2][kBoltSides];
                for (int end = 0; end < 2; ++end)
                {
                    const Vector3& p = end == 0 ? pa : pb;
                    const float r = end == 0 ? a.halfWidth : b.halfWidth;
                    for (int i = 0; i < kBoltSides; ++i)
                    {
                        const float t = 6.2831853f * static_cast<float>(i) / static_cast<float>(kBoltSides);
                        CnaEngine::Graphics::BuilderVertex v;
                        v.position = place(p + (n1 * std::cos(t) + n2 * std::sin(t)) * r);
                        v.normal = lit;
                        v.texCoord0 = uv;
                        ring[end][i] = mesh.AddVertex(v);
                    }
                }
                // Each side's two triangles wound as the builder's front faces (cross(b - a, c - a)
                // against the outward normal), so the tube is seen from outside.
                const Vector3 axisMid = place((pa + pb) * 0.5f);
                const auto tri = [&](std::uint32_t i0, std::uint32_t i1, std::uint32_t i2, const Vector3& p0,
                                     const Vector3& p1, const Vector3& p2)
                {
                    const Vector3 outward = (p0 + p1 + p2) * (1.0f / 3.0f) - axisMid;
                    if (Vector3::Dot(Vector3::Cross(p1 - p0, p2 - p0), outward) > 0.0f)
                    {
                        mesh.AddTriangle(i0, i2, i1);
                    }
                    else
                    {
                        mesh.AddTriangle(i0, i1, i2);
                    }
                };
                for (int i = 0; i < kBoltSides; ++i)
                {
                    const int j = (i + 1) % kBoltSides;
                    const float ti = 6.2831853f * static_cast<float>(i) / static_cast<float>(kBoltSides);
                    const float tj = 6.2831853f * static_cast<float>(j) / static_cast<float>(kBoltSides);
                    const Vector3 a0 = place(pa + (n1 * std::cos(ti) + n2 * std::sin(ti)) * a.halfWidth);
                    const Vector3 a1 = place(pa + (n1 * std::cos(tj) + n2 * std::sin(tj)) * a.halfWidth);
                    const Vector3 b0 = place(pb + (n1 * std::cos(ti) + n2 * std::sin(ti)) * b.halfWidth);
                    const Vector3 b1 = place(pb + (n1 * std::cos(tj) + n2 * std::sin(tj)) * b.halfWidth);
                    tri(ring[0][i], ring[0][j], ring[1][j], a0, a1, b1);
                    tri(ring[0][i], ring[1][j], ring[1][i], a0, b1, b0);
                }
            }
        }
    }

    // A cell's bolt turn about the vertical, from its place (so it holds still between frames).
    [[nodiscard]] inline float BoltCellYaw(int x, int y, int z) noexcept
    {
        const std::uint32_t h = static_cast<std::uint32_t>(x) * 73856093u ^ static_cast<std::uint32_t>(z) * 19349663u ^
                                static_cast<std::uint32_t>(y / 8) * 83492791u;
        return static_cast<float>(h % 360u) * 0.0174533f;
    }
}
