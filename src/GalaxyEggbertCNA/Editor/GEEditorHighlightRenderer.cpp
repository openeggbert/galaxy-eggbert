#include "GEEditorHighlightRenderer.hpp"

#include <Easy3D/CubeBatch.hpp>
#include <Easy3D/CubeMesh.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Slightly larger than a real 1x1x1 block so the highlight doesn't
        // z-fight with the actual terrain face it's tracking.
        constexpr float kHighlightSize = 1.05f;
    }

    void GEEditorHighlightRenderer::ShowCell(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                             float centerX, float centerY, float centerZ)
    {
        if (visible_ && hasLastCenter_ &&
            centerX == lastCenterX_ && centerY == lastCenterY_ && centerZ == lastCenterZ_)
        {
            return; // same cell already shown -- no rebuild needed
        }

        if (!effect_)
        {
            effect_ = std::make_unique<Microsoft::Xna::Framework::Graphics::BasicEffect>(device);
            effect_->setTextureEnabledProperty(false);
            effect_->VertexColorEnabled = false;
            // Translucent cyan -- reads clearly against both terrain and sky.
            effect_->setDiffuseColorProperty(Microsoft::Xna::Framework::Vector3(0.2f, 0.9f, 1.0f));
            effect_->setAlphaProperty(0.35f);
        }

        Easy3D::CubeBatch batch;
        batch.Add(Easy3D::CubeBatch::Vector3(centerX, centerY, centerZ),
                  Easy3D::CubeBatch::Vector3(kHighlightSize, kHighlightSize, kHighlightSize));
        std::vector<Easy3D::CubeVertex> vertices;
        std::vector<std::uint32_t> indices;
        Easy3D::BuildCubeMesh(batch, vertices, indices);
        mesh_ = std::make_unique<Easy3D::CubeMeshRenderer>(device, vertices, indices);

        lastCenterX_ = centerX;
        lastCenterY_ = centerY;
        lastCenterZ_ = centerZ;
        hasLastCenter_ = true;
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
