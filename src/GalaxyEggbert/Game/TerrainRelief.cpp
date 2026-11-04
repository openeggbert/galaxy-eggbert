#include "TerrainRelief.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>

namespace GalaxyEggbert::Game
{
    bool ReadTerrainRelief(const std::string& path, TerrainRelief& out)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            return false;
        }
        char magic[4] = {};
        std::uint32_t counts[2] = {};
        file.read(magic, 4);
        file.read(reinterpret_cast<char*>(counts), sizeof(counts));
        if (!file || std::memcmp(magic, "GTR1", 4) != 0 || counts[0] == 0 || counts[1] == 0 ||
            counts[0] > (1u << 22) || counts[1] > (1u << 22))
        {
            return false;
        }
        out.vertices.resize(counts[0]);
        out.triangles.resize(counts[1]);
        out.overhang.resize(counts[1]);
        file.read(reinterpret_cast<char*>(out.vertices.data()),
                  static_cast<std::streamsize>(out.vertices.size() * sizeof(TerrainRelief::Vertex)));
        file.read(reinterpret_cast<char*>(out.triangles.data()),
                  static_cast<std::streamsize>(out.triangles.size() * sizeof(out.triangles[0])));
        file.read(reinterpret_cast<char*>(out.overhang.data()), static_cast<std::streamsize>(out.overhang.size()));
        if (!file)
        {
            return false;
        }
        float lowest = 0.0f;
        for (const auto& v : out.vertices)
        {
            lowest = std::min(lowest, v.z);
        }
        out.depth = -lowest;
        for (const auto& t : out.triangles)
        {
            if (t[0] >= counts[0] || t[1] >= counts[0] || t[2] >= counts[0])
            {
                return false;
            }
        }
        return true;
    }

    int TerrainReliefTable::Load(const std::string& directory)
    {
        m_files.clear();
        m_icons.clear();
        std::ifstream index(directory + "/index.txt");
        std::string line;
        while (std::getline(index, line))
        {
            std::istringstream words(line);
            std::string tag, file;
            int icon = -1;
            if (!(words >> tag >> icon >> file) || tag != "icon" || icon < 0 ||
                icon > std::numeric_limits<std::uint16_t>::max())
            {
                continue;
            }
            auto found = m_files.find(file);
            if (found == m_files.end())
            {
                auto relief = std::make_shared<TerrainRelief>();
                if (!ReadTerrainRelief(directory + "/" + file, *relief))
                {
                    relief.reset();
                }
                found = m_files.emplace(file, std::move(relief)).first;
            }
            if (found->second)
            {
                m_icons[static_cast<std::uint16_t>(icon)] = found->second.get();
            }
        }
        return static_cast<int>(m_icons.size());
    }

    const TerrainRelief* TerrainReliefTable::Find(std::uint16_t icon) const
    {
        const auto found = m_icons.find(icon);
        return found == m_icons.end() ? nullptr : found->second;
    }

    void AppendReliefFace(CnaEngine::Graphics::MeshBuilder& mesh, const TerrainRelief& relief,
                          const Microsoft::Xna::Framework::Vector3& center, int face,
                          const CnaEngine::Graphics::UvRect& tile, std::uint8_t airSides, bool backing)
    {
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        const auto& f = kReliefFaceFrames[static_cast<std::size_t>(face)];
        const Vector3 n(static_cast<float>(f.n[0]), static_cast<float>(f.n[1]), static_cast<float>(f.n[2]));
        const Vector3 r(static_cast<float>(f.r[0]), static_cast<float>(f.r[1]), static_cast<float>(f.r[2]));
        const Vector3 u(static_cast<float>(f.u[0]), static_cast<float>(f.u[1]), static_cast<float>(f.u[2]));
        const Vector3 origin = center + n * 0.5f;
        const float du = tile.u1 - tile.u0;
        const float dv = tile.v1 - tile.v0;
        constexpr std::uint32_t kUnset = std::numeric_limits<std::uint32_t>::max();
        std::vector<std::uint32_t> remap(relief.vertices.size(), kUnset);
        const auto vertex = [&](std::uint32_t i)
        {
            if (remap[i] == kUnset)
            {
                const auto& v = relief.vertices[i];
                CnaEngine::Graphics::BuilderVertex out;
                out.position = origin + r * v.x + u * v.y + n * v.z;
                out.normal = r * v.nx + u * v.ny + n * v.nz;
                out.texCoord0 = Vector2(tile.u0 + v.u * du, tile.v0 + v.v * dv);
                remap[i] = mesh.AddVertex(out);
            }
            return remap[i];
        };
        for (std::size_t t = 0; t < relief.triangles.size(); ++t)
        {
            if ((relief.overhang[t] & airSides) != 0)
            {
                continue;
            }
            const auto& tri = relief.triangles[t];
            // The file's triangles turn counter-clockwise seen from outside; the builder's front
            // faces turn the other way (BoltMesh.hpp AppendPolylineTubes).
            mesh.AddTriangle(vertex(tri[0]), vertex(tri[2]), vertex(tri[1]));
        }
        if (backing)
        {
            const Vector3 base = origin - n * (relief.depth * 0.9f);
            const Vector3 corners[4] = {base - r * 0.5f + u * 0.5f, base + r * 0.5f + u * 0.5f,
                                        base + r * 0.5f - u * 0.5f, base - r * 0.5f - u * 0.5f};
            const Vector2 uvs[4] = {Vector2(tile.u0, tile.v0), Vector2(tile.u1, tile.v0), Vector2(tile.u1, tile.v1),
                                    Vector2(tile.u0, tile.v1)};
            std::uint32_t ids[4];
            for (int k = 0; k < 4; ++k)
            {
                CnaEngine::Graphics::BuilderVertex out;
                out.position = corners[k];
                out.normal = n;
                out.texCoord0 = uvs[k];
                ids[k] = mesh.AddVertex(out);
            }
            // Top-left, top-right, bottom-right turn clockwise seen from outside (right x up = out).
            mesh.AddTriangle(ids[0], ids[1], ids[2]);
            mesh.AddTriangle(ids[0], ids[2], ids[3]);
        }
    }
}
