#pragma once

#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <CnaEngine/Graphics/GpuMesh.hpp>
#include <CnaEngine/Graphics/MeshData.hpp>

#include <Microsoft/Xna/Framework/Graphics/Effect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::Game
{
    // The most vertices one terrain buffer holds: CNA refuses a vertex or index buffer over
    // 67108863 bytes ("exceeds the active graphics profile limit"), so a world whose terrain
    // has more (an editor world can) is uploaded in parts of at most this many, well under
    // that limit at every terrain vertex size (D47).
    constexpr std::size_t kMaxVerticesPerMeshPart = std::size_t{1} << 20;

    // `mesh` cut into meshes of at most `maxVertices` vertices each (one when it already
    // fits), its triangles in their order, each part holding the vertices its triangles use
    // with every attribute stream `mesh` has.
    [[nodiscard]] inline std::vector<CnaEngine::Graphics::MeshData> SplitMeshData(
        const CnaEngine::Graphics::MeshData& mesh, std::size_t maxVertices = kMaxVerticesPerMeshPart)
    {
        std::vector<CnaEngine::Graphics::MeshData> parts;
        if (mesh.VertexCount() <= maxVertices || maxVertices < 3)
        {
            parts.push_back(mesh);
            return parts;
        }
        // remap[v]: v's index in the part being built, valid while owner[v] names that part.
        std::vector<std::uint32_t> remap(mesh.VertexCount(), 0);
        std::vector<std::size_t> owner(mesh.VertexCount(), SIZE_MAX);
        const auto copyVertex = [&mesh](CnaEngine::Graphics::MeshData& part, std::uint32_t v)
        {
            part.positions.push_back(mesh.positions[v]);
            if (!mesh.normals.empty()) part.normals.push_back(mesh.normals[v]);
            if (!mesh.tangents.empty()) part.tangents.push_back(mesh.tangents[v]);
            if (!mesh.colors.empty()) part.colors.push_back(mesh.colors[v]);
            if (!mesh.texCoords0.empty()) part.texCoords0.push_back(mesh.texCoords0[v]);
            if (!mesh.texCoords1.empty()) part.texCoords1.push_back(mesh.texCoords1[v]);
            if (!mesh.blendIndices.empty()) part.blendIndices.push_back(mesh.blendIndices[v]);
            if (!mesh.blendWeights.empty()) part.blendWeights.push_back(mesh.blendWeights[v]);
        };
        parts.emplace_back();
        for (std::size_t t = 0; t + 2 < mesh.indices.size(); t += 3)
        {
            std::size_t fresh = 0;
            for (std::size_t k = 0; k < 3; ++k)
            {
                fresh += owner[mesh.indices[t + k]] == parts.size() - 1 ? 0 : 1;
            }
            if (parts.back().VertexCount() + fresh > maxVertices)
            {
                parts.emplace_back();
            }
            auto& part = parts.back();
            const std::size_t partId = parts.size() - 1;
            for (std::size_t k = 0; k < 3; ++k)
            {
                const std::uint32_t v = mesh.indices[t + k];
                if (owner[v] != partId)
                {
                    owner[v] = partId;
                    remap[v] = static_cast<std::uint32_t>(part.positions.size());
                    copyVertex(part, v);
                }
                part.indices.push_back(remap[v]);
            }
        }
        return parts;
    }

    // A mesh uploaded as one or more GPU meshes (SplitMeshData), drawn and counted as one.
    class GpuMeshParts
    {
    public:
        explicit GpuMeshParts(std::vector<CnaEngine::Graphics::GpuMesh> parts) : m_parts(std::move(parts)) {}

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::Effect& effect) const
        {
            for (const auto& part : m_parts)
            {
                part.Draw(device, effect);
            }
        }

        [[nodiscard]] int VertexCount() const noexcept
        {
            int count = 0;
            for (const auto& part : m_parts) count += part.VertexCount();
            return count;
        }

        [[nodiscard]] int PrimitiveCount() const noexcept
        {
            int count = 0;
            for (const auto& part : m_parts) count += part.PrimitiveCount();
            return count;
        }

        [[nodiscard]] std::size_t PartCount() const noexcept { return m_parts.size(); }

    private:
        std::vector<CnaEngine::Graphics::GpuMesh> m_parts;
    };

    // UploadLitTexturedMesh() / UploadBakedLitTexturedMesh() for a mesh of any size.
    [[nodiscard]] inline std::unique_ptr<GpuMeshParts> UploadLitTexturedMeshParts(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const CnaEngine::Graphics::MeshData& mesh)
    {
        std::vector<CnaEngine::Graphics::GpuMesh> parts;
        if (mesh.VertexCount() <= kMaxVerticesPerMeshPart)
        {
            parts.push_back(std::move(*UploadLitTexturedMesh(device, mesh)));
        }
        else
        {
            for (const auto& part : SplitMeshData(mesh))
            {
                parts.push_back(std::move(*UploadLitTexturedMesh(device, part)));
            }
        }
        return std::make_unique<GpuMeshParts>(std::move(parts));
    }

    [[nodiscard]] inline std::unique_ptr<GpuMeshParts> UploadBakedLitTexturedMeshParts(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, CnaEngine::Graphics::MeshData mesh)
    {
        std::vector<CnaEngine::Graphics::GpuMesh> parts;
        if (mesh.VertexCount() <= kMaxVerticesPerMeshPart)
        {
            parts.push_back(std::move(*UploadBakedLitTexturedMesh(device, std::move(mesh))));
        }
        else
        {
            for (auto& part : SplitMeshData(mesh))
            {
                parts.push_back(std::move(*UploadBakedLitTexturedMesh(device, std::move(part))));
            }
        }
        return std::make_unique<GpuMeshParts>(std::move(parts));
    }
}
