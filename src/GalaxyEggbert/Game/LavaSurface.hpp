#pragma once

#include "LavaHeight.hpp"

#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/TextureAtlas.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace GalaxyEggbert::Game
{
    // Lava in 3D (VIS-0069, 2026-11-01). The original's lava tiles (object-m.png 68-72) are a pool
    // rendered from Daniel Roux's Amapi slab (Decor/lave/lave-1..5.a3d): an orange body to a quarter
    // of the cell under a spiky wave that changes frame to frame. Drawn as a cube with the tile on
    // every face it was a box of fire; here each lava cell is that pool: a body up to the rim and
    // a lit height-field top from the original frames (LavaSurfaceData.hpp), cycled as the tiles.
    //
    // Colour: the tile's own pixels, sampled in its opaque body (the lowest 0.30 of the tile, under
    // every column's silhouette) at a height proportional to the vertex's: troughs dark orange,
    // crests the yellow of the tile's wave.
    // A pool's inner cells' coarser wave grid (AppendLavaCell).
    inline constexpr int kLavaInnerGridX = 4;
    inline constexpr int kLavaInnerGridZ = 3;

    struct LavaNeighbours
    {
        // Whether the cell on that side is lava too (no wall drawn between two lava cells) or, for
        // the bottom, anything solid.
        bool posX = false, negX = false, posZ = false, negZ = false, below = false;
    };

    namespace LavaSurfaceDetail
    {
        // The tile height whose colour a surface height takes (inside the tile's opaque body).
        inline constexpr float kSampleTop = 0.30f;
        inline constexpr float kHighest = 0.50f;

        [[nodiscard]] inline Microsoft::Xna::Framework::Vector2 Uv(const CnaEngine::Graphics::UvRect& tile, float u,
                                                                   float height) noexcept
        {
            const float sampled = std::clamp(height, 0.0f, kHighest) * (kSampleTop / kHighest);
            return Microsoft::Xna::Framework::Vector2(tile.u0 + std::clamp(u, 0.02f, 0.98f) * (tile.u1 - tile.u0),
                                                       tile.v0 + (1.0f - sampled) * (tile.v1 - tile.v0));
        }

    }

    // Appends one lava cell centred at `center` (a unit cell: y from center.y - 0.5): its wave top
    // and the walls of its body where no lava is beside it.
    inline void AppendLavaCell(CnaEngine::Graphics::MeshBuilder& mesh, const Microsoft::Xna::Framework::Vector3& center,
                               int frame, const CnaEngine::Graphics::UvRect& tile, const LavaNeighbours& beside)
    {
        using Microsoft::Xna::Framework::Vector3;
        using namespace LavaSurfaceDetail;
        frame = std::clamp(frame, 0, kLavaFrames - 1);
        const float x0 = center.X - 0.5f;
        const float z0 = center.Z - 0.5f;
        const float y0 = center.Y - 0.5f;
        // A cell with lava on all four sides, inside a pool, draws its wave on a coarser grid (the
        // same surface, bilinearly sampled): a pool's cells all move at every animation step and
        // the animated terrain is rebuilt then, so a large pool (105's floor, 61 x 61 cells) was
        // ~6x slower to play (Q-008). The full grid stays on the cells at a pool's edges.
        const bool inner = beside.posX && beside.negX && beside.posZ && beside.negZ;
        const int nx = inner ? kLavaInnerGridX : kLavaGridX;
        const int nz = inner ? kLavaInnerGridZ : kLavaGridZ;
        const auto heightAt = [&](int ix, int iz)
        {
            return LavaSurfaceHeight(frame, static_cast<float>(ix) / static_cast<float>(nx - 1),
                                     static_cast<float>(iz) / static_cast<float>(nz - 1));
        };
        // The top: one vertex per grid point, its normal from the neighbouring heights.
        std::uint32_t first = 0;
        for (int iz = 0; iz < nz; ++iz)
        {
            for (int ix = 0; ix < nx; ++ix)
            {
                const float u = static_cast<float>(ix) / static_cast<float>(nx - 1);
                const float w = static_cast<float>(iz) / static_cast<float>(nz - 1);
                const float dhdx = (heightAt(std::min(ix + 1, nx - 1), iz) - heightAt(std::max(ix - 1, 0), iz)) /
                                   (static_cast<float>(std::min(ix + 1, nx - 1) - std::max(ix - 1, 0)) /
                                    static_cast<float>(nx - 1));
                const float dhdz = (heightAt(ix, std::min(iz + 1, nz - 1)) - heightAt(ix, std::max(iz - 1, 0))) /
                                   (static_cast<float>(std::min(iz + 1, nz - 1) - std::max(iz - 1, 0)) /
                                    static_cast<float>(nz - 1));
                CnaEngine::Graphics::BuilderVertex v;
                v.position = Vector3(x0 + u, y0 + heightAt(ix, iz), z0 + w);
                v.normal = Vector3::Normalize(Vector3(-dhdx, 1.0f, -dhdz));
                v.texCoord0 = Uv(tile, u, heightAt(ix, iz));
                const std::uint32_t index = mesh.AddVertex(v);
                if (ix == 0 && iz == 0)
                {
                    first = index;
                }
            }
        }
        for (int iz = 0; iz + 1 < nz; ++iz)
        {
            for (int ix = 0; ix + 1 < nx; ++ix)
            {
                const std::uint32_t a = first + static_cast<std::uint32_t>(iz * nx + ix);
                const std::uint32_t b = a + 1;
                const std::uint32_t c = a + static_cast<std::uint32_t>(nx);
                const std::uint32_t d = c + 1;
                // Wound as the builder's own AddGrid (front faces: cross(b - a, c - a) points
                // against the normal).
                mesh.AddTriangle(a, b, d);
                mesh.AddTriangle(a, d, c);
            }
        }
        // The body's walls up to the rim (the grid's border is the rim), and its floor.
        const float rim = y0 + kLavaRimHeight;
        const auto wall = [&](const Vector3& p0, const Vector3& p1, const Vector3& normal)
        {
            // p0 -> p1 along the bottom edge, seen from outside left to right.
            CnaEngine::Graphics::BuilderVertex v;
            v.normal = normal;
            const std::uint32_t base = mesh.AddVertex(
                [&] { v.position = p0; v.texCoord0 = Uv(tile, 0.0f, 0.0f); return v; }());
            mesh.AddVertex([&] { v.position = Vector3(p1.X, p0.Y, p1.Z); v.texCoord0 = Uv(tile, 1.0f, 0.0f); return v; }());
            mesh.AddVertex([&] { v.position = Vector3(p1.X, rim, p1.Z); v.texCoord0 = Uv(tile, 1.0f, kLavaRimHeight); return v; }());
            mesh.AddVertex([&] { v.position = Vector3(p0.X, rim, p0.Z); v.texCoord0 = Uv(tile, 0.0f, kLavaRimHeight); return v; }());
            mesh.AddTriangle(base, base + 2, base + 1);
            mesh.AddTriangle(base, base + 3, base + 2);
        };
        const float x1 = x0 + 1.0f;
        const float z1 = z0 + 1.0f;
        if (!beside.posZ) wall(Vector3(x0, y0, z1), Vector3(x1, y0, z1), Vector3(0.0f, 0.0f, 1.0f));
        if (!beside.negZ) wall(Vector3(x1, y0, z0), Vector3(x0, y0, z0), Vector3(0.0f, 0.0f, -1.0f));
        if (!beside.posX) wall(Vector3(x1, y0, z1), Vector3(x1, y0, z0), Vector3(1.0f, 0.0f, 0.0f));
        if (!beside.negX) wall(Vector3(x0, y0, z0), Vector3(x0, y0, z1), Vector3(-1.0f, 0.0f, 0.0f));
        if (!beside.below)
        {
            CnaEngine::Graphics::BuilderVertex v;
            v.normal = Vector3(0.0f, -1.0f, 0.0f);
            v.texCoord0 = Uv(tile, 0.5f, 0.0f);
            v.position = Vector3(x0, y0, z0);
            const std::uint32_t base = mesh.AddVertex(v);
            v.position = Vector3(x1, y0, z0);
            mesh.AddVertex(v);
            v.position = Vector3(x1, y0, z1);
            mesh.AddVertex(v);
            v.position = Vector3(x0, y0, z1);
            mesh.AddVertex(v);
            mesh.AddTriangle(base, base + 2, base + 1);
            mesh.AddTriangle(base, base + 3, base + 2);
        }
    }
}
