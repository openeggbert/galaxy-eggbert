#pragma once

#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/TextureAtlas.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace GalaxyEggbert::Game
{
    // A terrain tile's Ray Dream scene as the relief laid on a cell's face (owner 2026-11-04,
    // Q-001/Q-004/Q-012; tools/terrain_relief/build_relief.py). The scenes are slabs modelled for
    // the front view only, so a cell shows its scene's front half on each exposed face, textured by
    // the tile's sprite projected from the front. Relief space: +x right, +y up, +z out of the face,
    // in blocks; the face's square is |x|, |y| <= 0.5, the relief lies at z <= 0 (inside the cell);
    // u, v in [0, 1] across the tile (v down). A triangle outside the square overhangs a neighbour's
    // (overhang bits: 1 +x, 2 -x, 4 +y, 8 -y).
    struct TerrainRelief
    {
        struct Vertex
        {
            float x, y, z, nx, ny, nz, u, v;
        };
        std::vector<Vertex> vertices;
        std::vector<std::array<std::uint32_t, 3>> triangles;
        std::vector<std::uint8_t> overhang;
        float depth = 0.0f; // how far the relief reaches into the cell
    };

    // Reads a .gtr file (build_relief.py); false when it is missing or malformed.
    bool ReadTerrainRelief(const std::string& path, TerrainRelief& out);

    // The reliefs of avatars3d/terrain/ by tile icon (index.txt: "icon <icon> <file>").
    class TerrainReliefTable
    {
    public:
        // Loads <directory>/index.txt and the files it names; returns how many icons have a relief.
        int Load(const std::string& directory);
        [[nodiscard]] const TerrainRelief* Find(std::uint16_t icon) const;
        [[nodiscard]] bool Empty() const noexcept { return m_icons.empty(); }

    private:
        std::unordered_map<std::string, std::shared_ptr<TerrainRelief>> m_files;
        std::unordered_map<std::uint16_t, const TerrainRelief*> m_icons;
    };

    // A cell face's frame, in CnaEngine::Graphics::BoxFace order (+Z, -Z, +X, -X, +Y, -Y): the outward
    // normal and the relief's right and up (right x up = normal, so the relief is not mirrored).
    struct ReliefFaceFrame
    {
        int n[3];
        int r[3];
        int u[3];
    };
    inline constexpr std::array<ReliefFaceFrame, 6> kReliefFaceFrames{{
        {{0, 0, 1}, {1, 0, 0}, {0, 1, 0}},
        {{0, 0, -1}, {-1, 0, 0}, {0, 1, 0}},
        {{1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
        {{-1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
        {{0, 1, 0}, {1, 0, 0}, {0, 0, -1}},
        {{0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
    }};

    // The overhang bits of `face` whose neighbour (the cell beside this one across that edge of the
    // face) is not solid: a relief hangs only into solid cells, never into the air Blupi moves in
    // (Q-012 B). `solid(dx, dy, dz)` tells whether the cell at that offset is solid.
    template <typename Solid>
    [[nodiscard]] std::uint8_t ReliefAirSides(int face, Solid&& solid)
    {
        const auto& f = kReliefFaceFrames[static_cast<std::size_t>(face)];
        std::uint8_t air = 0;
        if (!solid(f.r[0], f.r[1], f.r[2])) air |= 1;
        if (!solid(-f.r[0], -f.r[1], -f.r[2])) air |= 2;
        if (!solid(f.u[0], f.u[1], f.u[2])) air |= 4;
        if (!solid(-f.u[0], -f.u[1], -f.u[2])) air |= 8;
        return air;
    }

    // Appends `relief` on face `face` of the cell centred at `center`, its triangles overhanging into
    // `airSides` left out, textured by the atlas's `tile`; with `backing`, also the face's square at
    // the relief's depth (it fills what the lumps leave open), textured by the tile too.
    void AppendReliefFace(CnaEngine::Graphics::MeshBuilder& mesh, const TerrainRelief& relief,
                          const Microsoft::Xna::Framework::Vector3& center, int face,
                          const CnaEngine::Graphics::UvRect& tile, std::uint8_t airSides, bool backing);
}
