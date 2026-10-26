#pragma once

#include <CnaEngine/Graphics/GpuMesh.hpp>
#include <CnaEngine/Graphics/MeshBuilder.hpp>
#include <CnaEngine/Graphics/VertexLayouts.hpp>

#include <GalaxyEggbert/Game/TerrainLighting.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>

#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <cmath>
#include <cstdint>
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

    // A vertical plate `width` x `height` centered on `center` that reads upright and unmirrored
    // from both sides: one single-sided quad per side, each with the region's top-left corner at
    // its own viewer's top left. `right` (unit, horizontal) is the plate's width direction as
    // seen from its first side. MeshBuilder::AddPlate/AddCrossedPlates put the region's top
    // edge at the bottom and show one side mirrored (upstream_bugs.md UB-005, D48).
    inline void AddUprightPlate(CnaEngine::Graphics::MeshBuilder& mesh,
                                const Microsoft::Xna::Framework::Vector3& center,
                                const Microsoft::Xna::Framework::Vector3& right, float width, float height,
                                const CnaEngine::Graphics::UvRect& uv)
    {
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        const Vector3 up = Vector3::UnitY * (height * 0.5f);
        for (const float side : {1.0f, -1.0f})
        {
            const Vector3 across = right * (side * width * 0.5f);
            const Vector3 normal = Vector3::Normalize(Vector3::Cross(across, up));
            const auto corner = [&](const Vector3& position, float u, float v)
            {
                CnaEngine::Graphics::BuilderVertex vertex;
                vertex.position = position;
                vertex.normal = normal;
                vertex.texCoord0 = Vector2(u, v);
                return mesh.AddVertex(vertex);
            };
            const std::uint32_t topLeft = corner(center - across + up, uv.u0, uv.v0);
            const std::uint32_t topRight = corner(center + across + up, uv.u1, uv.v0);
            const std::uint32_t bottomRight = corner(center + across - up, uv.u1, uv.v1);
            const std::uint32_t bottomLeft = corner(center - across - up, uv.u0, uv.v1);
            mesh.AddQuad(topLeft, topRight, bottomRight, bottomLeft);
        }
    }

    // MeshBuilder::AddPlate's shape (PlateAxis says which way it faces) drawn as above; a
    // horizontal (Y) plate keeps AddPlate, whose region lies as a box's top face does.
    inline void AddUprightPlate(CnaEngine::Graphics::MeshBuilder& mesh,
                                const Microsoft::Xna::Framework::Vector3& center, float width, float height,
                                CnaEngine::Graphics::PlateAxis axis, const CnaEngine::Graphics::UvRect& uv)
    {
        switch (axis)
        {
            case CnaEngine::Graphics::PlateAxis::Z:
                AddUprightPlate(mesh, center, Microsoft::Xna::Framework::Vector3::UnitX, width, height, uv);
                break;
            case CnaEngine::Graphics::PlateAxis::X:
                AddUprightPlate(mesh, center, Microsoft::Xna::Framework::Vector3::UnitZ, width, height, uv);
                break;
            case CnaEngine::Graphics::PlateAxis::Y:
                mesh.AddPlate(center, width, height, axis, uv);
                break;
        }
    }

    // MeshBuilder::AddCrossedPlates' shape (`planes` plates turned 180/planes degrees apart
    // about +Y) drawn upright and unmirrored.
    inline void AddUprightCrossedPlates(CnaEngine::Graphics::MeshBuilder& mesh,
                                        const Microsoft::Xna::Framework::Vector3& center, float width, float height,
                                        int planes, const CnaEngine::Graphics::UvRect& uv)
    {
        constexpr float pi = 3.14159265358979323846f;
        for (int i = 0; i < planes; ++i)
        {
            const float radians = static_cast<float>(i) * pi / static_cast<float>(planes);
            AddUprightPlate(mesh, center,
                            Microsoft::Xna::Framework::Vector3(std::cos(radians), 0.0f, std::sin(radians)), width,
                            height, uv);
        }
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
