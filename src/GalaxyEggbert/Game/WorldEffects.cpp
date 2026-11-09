// WorldRenderer's effects layer (SceneEffects; the Setup's "3D effects"): the fog on every world
// effect, the soft shadows on the ground, the particles. Off, nothing here draws and the fog is
// switched off again, so the world comes out exactly as without the layer.

#include <GalaxyEggbert/Game/WorldRenderer.hpp>

#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <CnaEngine/Graphics/VertexLayouts.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/IEffectFog.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMeshPart.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp>
#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/TerrainRenderer.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace GalaxyEggbert::Game
{
    namespace
    {
        using Microsoft::Xna::Framework::Color;
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        namespace Graphics = Microsoft::Xna::Framework::Graphics;

        void SetFog(Graphics::IEffectFog* effect, const SceneEffects& effects)
        {
            if (effect == nullptr)
            {
                return;
            }
            effect->setFogEnabledProperty(effects.enabled);
            if (effects.enabled)
            {
                effect->setFogColorProperty(effects.fogColor);
                effect->setFogStartProperty(effects.fogStart);
                effect->setFogEndProperty(effects.fogEnd);
            }
        }

        CnaEngine::Graphics::MeshBuilder ColoredQuadBuilder()
        {
            return CnaEngine::Graphics::MeshBuilder({CnaEngine::Graphics::VertexAttribute::Position,
                                                     CnaEngine::Graphics::VertexAttribute::Color,
                                                     CnaEngine::Graphics::VertexAttribute::TexCoord0});
        }

        std::unique_ptr<CnaEngine::Graphics::GpuMesh> UploadColored(Graphics::GraphicsDevice& device,
                                                                    const CnaEngine::Graphics::MeshData& mesh)
        {
            return std::make_unique<CnaEngine::Graphics::GpuMesh>(CnaEngine::Graphics::GpuMesh::Upload(
                device, mesh, CnaEngine::Graphics::VertexLayout::PositionColorTexture()));
        }
    }

    void WorldRenderer::ApplyFog(Graphics::GraphicsDevice&, const SceneFrame& frame, const WorldRenderAssets& assets)
    {
        // The far world fades into the colour of the world's own sky (its background picture), as the
        // distance haze of a 1998 3D game; the background itself stays as it is.
        const auto& effects = frame.effects;
        SetFog(assets.terrain, effects);
        SetFog(assets.grass, effects);
        SetFog(assets.cubes, effects);
        SetFog(assets.models, effects);
        SetFog(assets.element, effects);
        SetFog(assets.objectM, effects);
        SetFog(assets.explo, effects);
        SetFog(assets.blupi, effects);
        SetFog(assets.blupi1, effects);
        SetFog(assets.bigDecor, effects);
        SetFog(m_decalEffect.get(), effects);
    }

    void WorldRenderer::DrawShadows(Graphics::GraphicsDevice& device, const SceneFrame& frame)
    {
        // Soft round shadows on the ground under Blupi and the objects, thrown a little away from the
        // sun (TerrainLighting.hpp's light) and paler the higher what throws them is; drawn over the
        // opaque terrain before the models, depth tested but not written.
        if (frame.effects.shadows.empty())
        {
            return;
        }
        if (!m_shadowTexture)
        {
            constexpr int kSize = 64;
            std::vector<Color> texels(static_cast<std::size_t>(kSize * kSize));
            for (int y = 0; y < kSize; ++y)
            {
                for (int x = 0; x < kSize; ++x)
                {
                    const float dx = (static_cast<float>(x) + 0.5f) / kSize * 2.0f - 1.0f;
                    const float dy = (static_cast<float>(y) + 0.5f) / kSize * 2.0f - 1.0f;
                    const float r = std::sqrt(dx * dx + dy * dy);
                    // Dark in the middle, a soft rim (smoothstep from 0.45 to 1).
                    const float t = std::clamp((r - 0.45f) / 0.55f, 0.0f, 1.0f);
                    const float alpha = 1.0f - t * t * (3.0f - 2.0f * t);
                    texels[static_cast<std::size_t>(y * kSize + x)] =
                        Color(0, 0, 0, static_cast<int>(std::lround(alpha * 255.0f)));
                }
            }
            m_shadowTexture = std::make_unique<Graphics::Texture2D>(device, kSize, kSize);
            m_shadowTexture->SetData(texels.data(), static_cast<int>(texels.size()));
            m_shadowEffect = std::make_unique<Graphics::BasicEffect>(device);
            m_shadowEffect->VertexColorEnabled = true;
            m_shadowEffect->setTextureEnabledProperty(true);
            m_shadowEffect->setTextureProperty(m_shadowTexture.get());
        }
        auto mesh = ColoredQuadBuilder();
        constexpr float kLift = 0.015f; // a hair over the ground, against depth fighting
        const bool mapped = m_scene.ShadowMapped();
        for (const auto& shadow : frame.effects.shadows)
        {
            if (mapped && shadow.model)
            {
                continue; // its model throws its real shadow (the shadow map)
            }
            const float alpha = std::clamp(shadow.strength, 0.0f, 1.0f);
            mesh.SetColor(Color(255, 255, 255, static_cast<int>(std::lround(alpha * 255.0f))));
            // Lying on the ground: right along +X, up along -Z (its front faces +Y).
            mesh.AddBillboard(shadow.center + Vector3(0.0f, kLift, 0.0f), Vector2(shadow.radius * 2.0f, shadow.radius * 2.0f),
                              Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f));
        }
        if (mesh.Mesh().Empty())
        {
            return;
        }
        m_shadowMesh = UploadColored(device, mesh.Mesh());
        m_shadowEffect->View = frame.camera.view;
        m_shadowEffect->Projection = frame.camera.projection;
        m_shadowEffect->World = Matrix::getIdentityProperty();
        device.setBlendStateProperty(Graphics::BlendState::NonPremultiplied);
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::DepthRead);
        device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
        m_shadowMesh->Draw(device, *m_shadowEffect);
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::Default);
        device.setBlendStateProperty(Graphics::BlendState::Opaque);
    }

    void WorldRenderer::RenderShadowMap(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                        const WorldRenderAssets& assets)
    {
        if (!frame.world || !frame.effects.shadowMap || !m_scene.Prepare(device))
        {
            m_scene.DropShadowMap(); // none this frame (a lower level): the shaders must not read the last
            return;
        }
        m_scene.BeginShadowPass(device, frame);
        if (!m_scene.ShadowMapped())
        {
            return;
        }
        device.setBlendStateProperty(Graphics::BlendState::Opaque);
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::Default);
        // Both sides: the terrain's meshes hold only its open faces, a model may be open too.
        device.setRasterizerStateProperty(Graphics::RasterizerState::CullNone);
        auto& solid = m_scene.DepthSolid();
        auto& cutout = m_scene.DepthCutout();
        if (frame.terrain && assets.terrainRenderer != nullptr && assets.terrain != nullptr)
        {
            if (auto* texture = assets.terrain->getTextureProperty())
            {
                cutout.SetTexture(0, *texture);
            }
            solid.setWorldProperty(Matrix::getIdentityProperty());
            cutout.setWorldProperty(Matrix::getIdentityProperty());
            assets.terrainRenderer->DrawShadowCasters(device, solid, cutout);

            // The lifts and crates.
            auto cubes = MakeLitTexturedMeshBuilder();
            for (const auto& cube : frame.cubes)
            {
                AddCenteredBox(cubes, cube.center, cube.size, cube.faces);
            }
            if (!cubes.Mesh().Empty())
            {
                UploadLitTexturedMesh(device, cubes.Mesh())->Draw(device, solid);
            }
        }
        // The models near enough to throw a shadow into the map (it spans 44 blocks round the target).
        const auto& focus = frame.camera.target;
        const auto cast = [&](const SceneModel& model) {
            if (model.model == nullptr || model.translucent)
            {
                return;
            }
            const auto at = model.world.getTranslationProperty();
            const float dx = at.X - focus.X, dy = at.Y - focus.Y, dz = at.Z - focus.Z;
            if (dx * dx + dy * dy + dz * dz > 32.0f * 32.0f)
            {
                return;
            }
            model.model->ComputeBoneTransformsEXT(model.clip, System::TimeSpan::FromSeconds(model.seconds), model.loop,
                                                  m_bones);
            auto* caster = m_scene.SkinnedCaster(m_bones, model.world);
            if (caster == nullptr)
            {
                return;
            }
            for (const auto& part : model.model->Parts)
            {
                if (model.sawSeconds >= 0.0f && !IsVisibleSawPart(part.Name, model.sawSeconds))
                {
                    continue;
                }
                if (model.boothFrontOnly && IsBoothBodyPart(part.Name))
                {
                    continue;
                }
                caster->Apply();
                device.SetVertexBuffer(part.Part->getVertexBufferProperty());
                device.SetIndexBuffer(part.Part->getIndexBufferProperty());
                device.DrawIndexedPrimitives(Graphics::PrimitiveType::TriangleList, part.Part->getVertexOffsetProperty(), 0,
                                             part.Part->getNumVerticesProperty(), part.Part->getStartIndexProperty(),
                                             part.Part->getPrimitiveCountProperty());
            }
        };
        if (assets.models != nullptr)
        {
            for (const auto& model : frame.opaqueModels)
            {
                cast(model);
            }
            for (const auto& model : frame.objectModels)
            {
                cast(model);
            }
            for (const auto& model : frame.translucentModels)
            {
                cast(model); // the translucent ones throw none (cast() skips them)
            }
        }
        m_scene.EndShadowPass(device);
        device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
    }

    void WorldRenderer::DrawParticles(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                      const WorldRenderAssets& assets)
    {
        // The explosions, splashes and sparks as particles: pieces of their own sprites, each its own
        // tint and fade (vertex colour), over the billboards, depth tested, not written.
        if (frame.effects.particles.empty())
        {
            return;
        }
        if (m_particleMeshes.size() < frame.effects.particles.size())
        {
            m_particleMeshes.resize(frame.effects.particles.size());
        }
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::DepthRead);
        for (std::size_t i = 0; i < frame.effects.particles.size(); ++i)
        {
            const auto& batch = frame.effects.particles[i];
            auto* effect = SheetEffect(batch.sheet, assets);
            if (effect == nullptr || batch.quads.empty())
            {
                continue;
            }
            auto mesh = ColoredQuadBuilder();
            for (const auto& quad : batch.quads)
            {
                mesh.SetColor(quad.color);
                mesh.AddBillboard(quad.center, quad.size, quad.right, quad.up, quad.uv, Vector2(0.5f, 0.5f),
                                  quad.rotation);
            }
            m_particleMeshes[i] = UploadColored(device, mesh.Mesh());
            effect->View = frame.camera.view;
            effect->Projection = frame.camera.projection;
            effect->World = Matrix::getIdentityProperty();
            const bool vertexColors = effect->VertexColorEnabled;
            effect->VertexColorEnabled = true;
            m_particleMeshes[i]->Draw(device, *effect);
            effect->VertexColorEnabled = vertexColors;
        }
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::Default);
    }
}
