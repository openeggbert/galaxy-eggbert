#include "GEEditorHighlightRenderer.hpp"

#include <Easy3D/CubeBatch.hpp>
#include <Easy3D/CubeMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Slightly larger than a real 1x1x1 block so the single-cell
        // highlight doesn't z-fight with the actual terrain face it's
        // tracking.
        constexpr float kCellHighlightPadding = 0.05f;
    }

    void GEEditorHighlightRenderer::ShowCell(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                             float centerX, float centerY, float centerZ)
    {
        const float half = 0.5f + kCellHighlightPadding * 0.5f;
        // Translucent cyan -- reads clearly against both terrain and sky.
        Rebuild(device, centerX - half, centerY - half, centerZ - half,
                centerX + half, centerY + half, centerZ + half,
                0.2f, 0.9f, 1.0f);
    }

    void GEEditorHighlightRenderer::ShowBox(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                            float minCenterX, float minCenterY, float minCenterZ,
                                            float maxCenterX, float maxCenterY, float maxCenterZ)
    {
        const float half = 0.5f + kCellHighlightPadding * 0.5f;
        // Translucent yellow -- visually distinct from the single-cell
        // cyan crosshair, so a box-fill selection reads as clearly
        // different from ordinary block picking.
        Rebuild(device, minCenterX - half, minCenterY - half, minCenterZ - half,
                maxCenterX + half, maxCenterY + half, maxCenterZ + half,
                1.0f, 0.9f, 0.2f);
    }

    void GEEditorHighlightRenderer::ShowSelectedObject(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                                       float centerX, float centerY, float centerZ)
    {
        const float half = 0.5f + kCellHighlightPadding * 0.5f;
        // Translucent magenta -- distinct from both the cyan crosshair and
        // the yellow box-fill overlay.
        Rebuild(device, centerX - half, centerY - half, centerZ - half,
                centerX + half, centerY + half, centerZ + half,
                1.0f, 0.25f, 0.85f);
    }

    void GEEditorHighlightRenderer::Rebuild(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                            float minX, float minY, float minZ,
                                            float maxX, float maxY, float maxZ,
                                            float r, float g, float b)
    {
        if (visible_ && hasLastBounds_ &&
            minX == lastMinX_ && minY == lastMinY_ && minZ == lastMinZ_ &&
            maxX == lastMaxX_ && maxY == lastMaxY_ && maxZ == lastMaxZ_)
        {
            return; // same bounds already shown -- no rebuild needed
        }

        if (!effect_)
        {
            effect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
            effect_->setTextureEnabledProperty(false);
            effect_->VertexColorEnabled = false;
            effect_->setAlphaProperty(0.35f);
        }
        effect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(r, g, b));

        const Easy3D::CubeBatch::Vector3 center(
            (minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
        const Easy3D::CubeBatch::Vector3 size(maxX - minX, maxY - minY, maxZ - minZ);

        Easy3D::CubeBatch batch;
        batch.Add(center, size);
        std::vector<Easy3D::CubeVertex> vertices;
        std::vector<std::uint32_t> indices;
        Easy3D::BuildCubeMesh(batch, vertices, indices);
        mesh_ = std::make_unique<Easy3D::CubeMeshRenderer>(device, vertices, indices);

        lastMinX_ = minX; lastMinY_ = minY; lastMinZ_ = minZ;
        lastMaxX_ = maxX; lastMaxY_ = maxY; lastMaxZ_ = maxZ;
        hasLastBounds_ = true;
        visible_ = true;
    }

    void GEEditorHighlightRenderer::Hide() noexcept
    {
        visible_ = false;
    }

    void GEEditorHighlightRenderer::Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                         const Easy3D::Camera3D& camera)
    {
        if (!visible_ || !mesh_ || !effect_)
        {
            return;
        }

        using Microsoft::Xna::Framework::Graphics::BlendState;
        using Microsoft::Xna::Framework::Graphics::DepthStencilState;

        effect_->World = Microsoft::Xna::Framework::Matrix::getIdentityProperty();
        effect_->View = camera.GetViewMatrix();
        effect_->Projection = camera.GetProjectionMatrix();

        device.setBlendStateProperty(BlendState::NonPremultiplied);
        device.setDepthStencilStateProperty(DepthStencilState::DepthRead);
        mesh_->Draw(device, *effect_);
        device.setDepthStencilStateProperty(DepthStencilState::Default);
        device.setBlendStateProperty(BlendState::Opaque);
    }
}
