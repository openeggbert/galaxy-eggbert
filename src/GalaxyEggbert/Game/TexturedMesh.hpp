#pragma once

#include <CnaEngine/Graphics/GpuMesh.hpp>
#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/VertexLayouts.hpp>

#include <GalaxyEggbert/Game/TerrainLighting.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>

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

    // The same with face normals, for what is drawn lit: the terrain and the cube objects
    // standing in it (TerrainLighting.hpp). Upload with UploadLitTexturedMesh().
    [[nodiscard]] inline CnaEngine::Graphics::MeshBuilder MakeLitTexturedMeshBuilder()
    {
        return CnaEngine::Graphics::MeshBuilder({CnaEngine::Graphics::VertexAttribute::Position,
                                                 CnaEngine::Graphics::VertexAttribute::Normal,
                                                 CnaEngine::Graphics::VertexAttribute::TexCoord0});
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

    // An open cube's inner faces must survive back-face culling. Each face has its own
    // reverse-wound copy with upright UVs; this leaves the global rasterizer unchanged.
    inline void AddDoubleSidedCenteredBox(CnaEngine::Graphics::MeshBuilder& mesh,
        const Microsoft::Xna::Framework::Vector3& center, const CnaEngine::Graphics::BoxFaces& faces)
    {
        using V=Microsoft::Xna::Framework::Vector3;
        using UV=Microsoft::Xna::Framework::Vector2;
        const V normals[]={{0,0,1},{0,0,-1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0}};
        const V right[]={{1,0,0},{-1,0,0},{0,0,-1},{0,0,1},{1,0,0},{1,0,0}};
        const V up[]={{0,1,0},{0,1,0},{0,1,0},{0,1,0},{0,0,-1},{0,0,1}};
        for (int face=0;face<6;++face)
        {
            if (!faces[face].visible) continue;
            const auto c=center+normals[face]*.5f,r=right[face]*.5f,u=up[face]*.5f;
            const auto& uv=faces[face].uv;
            mesh.AddDoubleSidedQuad({c-r-u,c+r-u,c+r+u,c-r+u},
                {UV(uv.u0,uv.v1),UV(uv.u1,uv.v1),UV(uv.u1,uv.v0),UV(uv.u0,uv.v0)});
        }
    }

    // Uploads a mesh with at least one triangle (GpuMesh::Upload rejects an
    // empty one; callers check Mesh().Empty() first).
    [[nodiscard]] inline std::unique_ptr<CnaEngine::Graphics::GpuMesh> UploadTexturedMesh(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const CnaEngine::Graphics::MeshData& mesh)
    {
        return std::make_unique<CnaEngine::Graphics::GpuMesh>(
            CnaEngine::Graphics::GpuMesh::Upload(device, mesh, CnaEngine::Graphics::VertexLayout::PositionTexture()));
    }

    // Uploads a MakeLitTexturedMeshBuilder() mesh for an effect without lights (AlphaTestEffect,
    // the terrain's cutout pass): each vertex coloured with the brightness kTerrainLight gives
    // its normal, so it is lit as UploadLitTexturedMesh()'s meshes are (VertexPositionColorTexture).
    [[nodiscard]] inline std::unique_ptr<CnaEngine::Graphics::GpuMesh> UploadBakedLitTexturedMesh(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, CnaEngine::Graphics::MeshData mesh)
    {
        mesh.colors.resize(mesh.positions.size());
        for (std::size_t i = 0; i < mesh.normals.size() && i < mesh.colors.size(); ++i)
        {
            const auto& normal = mesh.normals[i];
            const float brightness = TerrainFaceBrightness(normal.X, normal.Y, normal.Z);
            mesh.colors[i] = Microsoft::Xna::Framework::Color(brightness, brightness, brightness, 1.0f);
        }
        return std::make_unique<CnaEngine::Graphics::GpuMesh>(CnaEngine::Graphics::GpuMesh::Upload(
            device, mesh, CnaEngine::Graphics::VertexLayout::PositionColorTexture()));
    }

    // Uploads a MakeLitTexturedMeshBuilder() mesh (VertexPositionNormalTexture).
    [[nodiscard]] inline std::unique_ptr<CnaEngine::Graphics::GpuMesh> UploadLitTexturedMesh(
        Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const CnaEngine::Graphics::MeshData& mesh)
    {
        return std::make_unique<CnaEngine::Graphics::GpuMesh>(CnaEngine::Graphics::GpuMesh::Upload(
            device, mesh, CnaEngine::Graphics::VertexLayout::PositionNormalTexture()));
    }
}
