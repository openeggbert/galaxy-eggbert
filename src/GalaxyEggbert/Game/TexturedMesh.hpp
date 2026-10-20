#pragma once

#include <CnaEngine/Graphics/GpuMesh.hpp>
#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/VertexLayouts.hpp>

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <memory>

namespace GalaxyEggbert::Game
{
    // Every Galaxy Eggbert mesh -- terrain, cube objects, billboards, HUD and
    // editor quads -- is positions plus one texture coordinate (XNA's
    // VertexPositionTexture), built with cna-engine's MeshBuilder and drawn
    // with the caller's own BasicEffect through GpuMesh::Draw(device, effect).
    [[nodiscard]] inline CnaEngine::Graphics::MeshBuilder MakeTexturedMeshBuilder()
    {
        return CnaEngine::Graphics::MeshBuilder(
            {CnaEngine::Graphics::VertexAttribute::Position, CnaEngine::Graphics::VertexAttribute::TexCoord0});
    }

    [[nodiscard]] inline CnaEngine::Graphics::BuilderVertex TexturedVertex(
        const Microsoft::Xna::Framework::Vector3& position, const Microsoft::Xna::Framework::Vector2& uv)
    {
        CnaEngine::Graphics::BuilderVertex vertex;
        vertex.position = position;
        vertex.texCoord0 = uv;
        return vertex;
    }

    // A box of `size` centered on `center`: the corner arithmetic every cube
    // here has always used (center -/+ size * 0.5).
    inline void AddCenteredBox(CnaEngine::Graphics::MeshBuilder& mesh,
                               const Microsoft::Xna::Framework::Vector3& center,
                               const Microsoft::Xna::Framework::Vector3& size,
                               const CnaEngine::Graphics::BoxFaces& faces)
    {
        const Microsoft::Xna::Framework::Vector3 half = size * 0.5f;
        mesh.AddBox(center - half, center + half, faces);
    }

    // The same with one texture region on all six faces.
    inline void AddCenteredBox(CnaEngine::Graphics::MeshBuilder& mesh,
                               const Microsoft::Xna::Framework::Vector3& center,
                               const Microsoft::Xna::Framework::Vector3& size,
                               const CnaEngine::Graphics::UvRect& uv)
    {
        CnaEngine::Graphics::BoxFaces faces;
        faces.fill(CnaEngine::Graphics::BoxFaceStyle{true, uv});
        AddCenteredBox(mesh, center, size, faces);
    }

    // Uploads a mesh with at least one triangle (GpuMesh::Upload rejects an
    // empty one; callers check Mesh().Empty() first).
    [[nodiscard]] inline std::unique_ptr<CnaEngine::Graphics::GpuMesh> UploadTexturedMesh(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const CnaEngine::Graphics::MeshData& mesh)
    {
        return std::make_unique<CnaEngine::Graphics::GpuMesh>(
            CnaEngine::Graphics::GpuMesh::Upload(device, mesh, CnaEngine::Graphics::VertexLayout::PositionTexture()));
    }
}
