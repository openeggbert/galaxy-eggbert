#include <GalaxyEggbert/Game/WorldRenderer.hpp>

#include <GalaxyEggbert/Game/ModelSubdivision.hpp>

#include <GalaxyEggbert/Game/BlockModels.hpp>
#include <GalaxyEggbert/Game/TerrainRenderer.hpp>
#include <GalaxyEggbert/Game/TexturedMesh.hpp>

#include <Microsoft/Xna/Framework/BoundingFrustum.hpp>
#include <Microsoft/Xna/Framework/BoundingSphere.hpp>
#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/ContainmentType.hpp>
#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMeshPart.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector2;
        using Microsoft::Xna::Framework::Vector3;
        namespace Graphics = Microsoft::Xna::Framework::Graphics;

        void SetCamera(Graphics::BasicEffect& effect, const SceneCamera& camera)
        {
            effect.View = camera.view;
            effect.Projection = camera.projection;
            effect.World = Matrix::getIdentityProperty();
        }

        void DrawPart(Graphics::GraphicsDevice& device, const Graphics::SkinnedModelEXT::PartEXT& part)
        {
            device.SetVertexBuffer(part.Part->getVertexBufferProperty());
            device.SetIndexBuffer(part.Part->getIndexBufferProperty());
            device.DrawIndexedPrimitives(Graphics::PrimitiveType::TriangleList, part.Part->getVertexOffsetProperty(), 0,
                                         part.Part->getNumVerticesProperty(), part.Part->getStartIndexProperty(),
                                         part.Part->getPrimitiveCountProperty());
        }

        bool PartShown(const SceneModel& model, const Graphics::SkinnedModelEXT::PartEXT& part)
        {
            if (model.sawSeconds >= 0.0f && !IsVisibleSawPart(part.Name, model.sawSeconds))
            {
                return false;
            }
            // A teleporter's other sides: its plate and lights only.
            return !(model.boothFrontOnly && IsBoothBodyPart(part.Name));
        }

        // The decal effect is made on first use: fogged as the rest of the world (the effects layer).
        void FogDecals(Graphics::BasicEffect& effect, const SceneFrame& frame)
        {
            effect.setFogEnabledProperty(frame.effects.enabled);
            if (frame.effects.enabled)
            {
                effect.setFogColorProperty(frame.effects.fogColor);
                effect.setFogStartProperty(frame.effects.fogStart);
                effect.setFogEndProperty(frame.effects.fogEnd);
            }
        }

        CnaEngine::Graphics::MeshBuilder QuadMesh(const std::vector<SceneQuad>& quads)
        {
            auto mesh = MakeTexturedMeshBuilder();
            for (const auto& quad : quads)
            {
                mesh.AddBillboard(quad.center, quad.size, quad.right, quad.up, quad.uv);
            }
            return mesh;
        }
    }

    void WorldRenderer::Draw(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                             const WorldRenderAssets& assets, const TerrainDrawnHook& terrainDrawn)
    {
        if (!frame.world)
        {
            return;
        }
        ApplyFog(device, frame, assets);
        DrawBackground(device, frame, assets);
        if (frame.terrain && assets.terrainRenderer != nullptr && assets.terrain != nullptr)
        {
            DrawTerrainPass(device, frame, assets, terrainDrawn);
        }
        // Real alpha transparency for every billboard (their sheets have straight alpha:
        // Texture2D(path) loads PNGs unpremultiplied, which NonPremultiplied blends right).
        device.setBlendStateProperty(Graphics::BlendState::NonPremultiplied);
        DrawBillboards(device, frame, assets);
        if (frame.effects.enabled)
        {
            DrawParticles(device, frame, assets);
        }
    }

    void WorldRenderer::DrawBackground(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                       const WorldRenderAssets& assets)
    {
        // One huge camera-facing quad far behind the scene, its edges outside the view at
        // kBackgroundDistance whatever the window's aspect (kBackgroundMargin covers the diagonal).
        if (assets.background == nullptr)
        {
            return;
        }
        const auto& camera = frame.camera;
        const auto invView = Matrix::Invert(camera.view);
        const auto cameraRight = invView.getRightProperty();
        const auto cameraUp = invView.getUpProperty();

        constexpr float kBackgroundDistance = 900.0f; // < the camera's 1000 far plane
        constexpr float kBackgroundMargin = 1.3f;
        const float halfHeight = kBackgroundDistance * std::tan(camera.fieldOfView * 0.5f) * kBackgroundMargin;
        const float halfWidth = halfHeight * camera.aspectRatio;

        const auto& camPos = camera.position;
        const auto& camTarget = camera.target;
        Vector3 forward(camTarget.X - camPos.X, camTarget.Y - camPos.Y, camTarget.Z - camPos.Z);
        forward.Normalize();
        const Vector3 quadCenter(camPos.X + forward.X * kBackgroundDistance, camPos.Y + forward.Y * kBackgroundDistance,
                                 camPos.Z + forward.Z * kBackgroundDistance);

        // A distant panorama (owner 2026-11-02, OR-0053). The original tiles its 640 x 480 picture
        // and scrolls it at 2/3 of the world's scroll (Decor.cpp:650-700); here the picture is
        // wrapped round the horizon N times, N the whole number of screen widths in a turn, turning
        // the camera scrolls it, and the camera's height moves it up and down at 2/3.
        constexpr float kTwoPi = 6.28318530718f;
        const float verticalFov = camera.fieldOfView;
        const float horizontalFov = 2.0f * std::atan(std::tan(verticalFov * 0.5f) * camera.aspectRatio);
        const float repeats = std::max(1.0f, std::round(kTwoPi / horizontalFov));
        const float imageAngleWide = kTwoPi / repeats;      // one picture's width, radians
        const float imageAngleHigh = imageAngleWide * 0.75f; // 640 x 480
        const float quadAngleWide = 2.0f * std::atan(halfWidth / kBackgroundDistance);
        const float quadAngleHigh = 2.0f * std::atan(halfHeight / kBackgroundDistance);
        const float yaw = std::atan2(forward.X, -forward.Z);
        const float pitch = std::asin(std::clamp(forward.Y, -1.0f, 1.0f));
        const float uWidth = quadAngleWide / imageAngleWide;
        const float vHeight = quadAngleHigh / imageAngleHigh;
        const float uCenter = yaw / imageAngleWide;
        constexpr float kParallax = 2.0f / 3.0f;
        const float vCenter = 0.5f - (camPos.Y * 64.0f * kParallax) / 480.0f - pitch / imageAngleHigh;
        auto mesh = MakeTexturedMeshBuilder();
        mesh.AddBillboard(quadCenter, Vector2(halfWidth * 2.0f, halfHeight * 2.0f), cameraRight, cameraUp,
                          CnaEngine::Graphics::UvRect{uCenter - uWidth * 0.5f, vCenter - vHeight * 0.5f,
                                                      uCenter + uWidth * 0.5f, vCenter + vHeight * 0.5f});
        if (!mesh.Mesh().Empty())
        {
            m_backgroundMesh = UploadTexturedMesh(device, mesh.Mesh());
            SetCamera(*assets.background, camera);
            // The panorama wraps (its UVs run past 0..1).
            device.getSamplerStatesProperty()[0] = Graphics::SamplerState::LinearWrap;
            m_backgroundMesh->Draw(device, *assets.background);
        }
    }

    void WorldRenderer::DrawTerrainPass(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                        const WorldRenderAssets& assets, const TerrainDrawnHook& terrainDrawn)
    {
        auto& terrain = *assets.terrainRenderer;
        SetCamera(*assets.terrain, frame.camera);
        // The 3D effects' shaders, where the device compiles them: water and lava, and the terrain's
        // shadowed, occluded, point-lit shading (SceneShaders.hpp).
        TerrainShading shading;
        if (frame.effects.surfaces && m_surfaces.Prepare(device))
        {
            m_surfaces.Update(device, *assets.terrain, frame, frame.effects.ultra ? assets.ultra : nullptr);
            shading.lava = m_surfaces.Lava();
            shading.water = m_surfaces.Water();
        }
        // Medium: the terrain's occlusion (its shaders, no shadow, no lights); high: the shadow map and
        // the lights on the terrain, the cubes, the grass and the models too.
        m_sceneShading = (frame.effects.occlusion || frame.effects.shadowMap || frame.effects.pointLights) &&
                         m_scene.Prepare(device);
        m_modelShading = m_sceneShading && (frame.effects.shadowMap || frame.effects.pointLights);
        if (m_sceneShading)
        {
            m_scene.SetFrame(device, frame, *assets.terrain, frame.effects.ultra ? assets.ultra : nullptr);
            shading.occluded = m_scene.Occluded();
            shading.lit = m_scene.Lit();
            shading.cutout = m_scene.Cutout();
        }
        // The reliefs near the camera's target (Q-012), then the opaque terrain; its semi-transparent
        // pass (water) after the models.
        {
            const auto reliefStart = std::chrono::steady_clock::now();
            const int shownBefore = terrain.ReliefChunksShown();
            terrain.UpdateRelief(device, frame.reliefTarget, frame.reliefRadius, frame.reliefBudget);
            if (frame.reportReliefTiming)
            {
                const auto ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - reliefStart);
                std::cout << "GalaxyEggbertCNA: relief: " << terrain.ReliefChunksShown() - shownBefore
                          << " chunk(s), " << terrain.ReliefPrimitiveCount() << " triangles built in "
                          << ms.count() << " ms." << std::endl;
            }
        }
        terrain.DrawOpaque(device, *assets.terrain, &shading);
        if (frame.effects.enabled)
        {
            DrawShadows(device, frame);
        }

        DrawCubes(device, frame, assets);

        if (assets.grass != nullptr)
        {
            // Icon 107's grass-top overlay: its own texture and effect, right after the opaque terrain.
            SetCamera(*assets.grass, frame.camera);
            if (m_modelShading)
            {
                terrain.DrawGrass(device, m_scene.LitWith(assets.grass->getTextureProperty(), Matrix::getIdentityProperty()));
            }
            else
            {
                terrain.DrawGrass(device, *assets.grass);
            }
        }

        if (assets.models != nullptr)
        {
            for (std::size_t i = 0; i < frame.opaqueModels.size(); ++i)
            {
                if (static_cast<int>(i) == frame.vehiclePassStart)
                {
                    device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
                }
                DrawModel(device, frame, *assets.models, frame.opaqueModels[i]);
            }
            if (frame.vehiclePassStart == static_cast<int>(frame.opaqueModels.size()))
            {
                device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
            }
            for (const auto& model : frame.objectModels)
            {
                DrawModel(device, frame, *assets.models, model);
            }
            if (frame.objectModelPass)
            {
                device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
            }
        }

        DrawDecals(device, frame, assets);

        if (assets.models != nullptr)
        {
            // Blended over the completed opaque scene: depth read, so walls in front hide it while
            // what lies behind shows through.
            for (const auto& model : frame.translucentModels)
            {
                DrawModel(device, frame, *assets.models, model);
            }
            for (const auto& model : frame.editorStarts)
            {
                DrawModel(device, frame, *assets.models, model);
            }
        }

        // The terrain's semi-transparent pass after Blupi and every model, so water covers what lies
        // in it (OR-0052: a chest under water looked dry from above).
        device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
        if (shading.water != nullptr)
        {
            m_surfaces.BindTextures(*assets.terrain, frame.effects.ultra ? assets.ultra : nullptr);
        }
        terrain.DrawTransparent(device, *assets.terrain, &shading);

        if (terrainDrawn)
        {
            terrainDrawn(device);
        }
    }

    void WorldRenderer::DrawCubes(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                  const WorldRenderAssets& assets)
    {
        // The lift and crate objects: lit textured cubes on the terrain's sheet, rebuilt every frame
        // (their icons animate); then the crates blown away, each its cube in its own transform.
        if (assets.cubes == nullptr)
        {
            return;
        }
        auto cubeMesh = MakeLitTexturedMeshBuilder();
        for (const auto& cube : frame.cubes)
        {
            AddCenteredBox(cubeMesh, cube.center, cube.size, cube.faces);
        }
        // With the effects' shading the cubes take the terrain's light, shadows and point lights.
        const auto cubeEffect = [&](const Matrix& world) -> Graphics::Effect& {
            if (m_modelShading)
            {
                return m_scene.LitWith(assets.cubes->getTextureProperty(), world);
            }
            SetCamera(*assets.cubes, frame.camera);
            assets.cubes->World = world;
            return *assets.cubes;
        };
        if (!cubeMesh.Mesh().Empty())
        {
            m_cubeMesh = UploadLitTexturedMesh(device, cubeMesh.Mesh());
            m_cubeMesh->Draw(device, cubeEffect(Matrix::getIdentityProperty()));
        }
        for (const auto& piece : frame.cubePieces)
        {
            auto pieceMesh = MakeLitTexturedMeshBuilder();
            AddCenteredBox(pieceMesh, Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f), piece.uv);
            auto pieceRenderer = UploadLitTexturedMesh(device, pieceMesh.Mesh());
            pieceRenderer->Draw(device, cubeEffect(piece.world));
            assets.cubes->World = Matrix::getIdentityProperty();
        }
    }

    void WorldRenderer::DrawModel(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                  Graphics::SkinnedEffect& effect, const SceneModel& model)
    {
        if (model.model == nullptr)
        {
            return;
        }
        if (model.cullRadius >= 0.0f)
        {
            // Out of view: not drawn (its sphere widened by the caller for depth scale and offset).
            const Microsoft::Xna::Framework::BoundingFrustum viewFrustum(frame.camera.view * frame.camera.projection);
            if (viewFrustum.Contains(Microsoft::Xna::Framework::BoundingSphere(model.cullCenter, model.cullRadius)) ==
                Microsoft::Xna::Framework::ContainmentType::Disjoint)
            {
                ++m_statistics.modelsCulled;
                return;
            }
        }
        model.model->ComputeBoneTransformsEXT(model.clip, System::TimeSpan::FromSeconds(model.seconds), model.loop,
                                              m_bones);
        // Mirroring turns the triangles' winding round (Clockwise); XNA's default otherwise.
        device.setRasterizerStateProperty(model.cull == SceneCull::Clockwise
                                              ? Graphics::RasterizerState::CullClockwise
                                              : Graphics::RasterizerState::CullCounterClockwise);
        // With the effects' shading: the model shader (shadow, point lights), lit as `effect` is set up.
        const bool shaded = m_modelShading && m_scene.PrepareModel(effect, m_bones, model.world, model.alpha,
                                                                   model.tinted, model.tint);
        if (shaded)
        {
            if (model.translucent)
            {
                device.setBlendStateProperty(Graphics::BlendState::AlphaBlend);
                device.setDepthStencilStateProperty(Graphics::DepthStencilState::DepthRead);
            }
            auto& shader = m_scene.Model();
            const auto& parts = model.model->Parts;
            for (std::size_t i = 0; i < parts.size(); ++i)
            {
                const auto& part = parts[i];
                if (!PartShown(model, part))
                {
                    continue;
                }
                Graphics::Texture2D* texture =
                    i < model.partTextures.size() && model.partTextures[i] != nullptr ? model.partTextures[i] : part.Texture;
                if (texture != nullptr)
                {
                    shader.SetTexture(0, *texture);
                }
                m_scene.SetPartMaterial(model.partMaterials != nullptr && i < model.partMaterials->size()
                                            ? &(*model.partMaterials)[i]
                                            : nullptr);
                shader.Apply();
                // Ultra: the part smoother (its subdivided copy).
                if (!m_scene.UltraActive() || !DrawSmoothPart(device, *part.Part))
                {
                    DrawPart(device, part);
                }
                if (model.counted)
                {
                    m_statistics.modelTriangles += part.Part->getPrimitiveCountProperty();
                }
            }
            if (model.translucent)
            {
                device.setBlendStateProperty(Graphics::BlendState::Opaque);
                device.setDepthStencilStateProperty(Graphics::DepthStencilState::Default);
            }
            if (model.editorSelected)
            {
                // The selection overlay stays the stock effect's: it takes this pose. Its skinning rounds
                // a little otherwise than the model shader's, so its red lay just behind the very mesh it
                // marks and the depth test hid it: pulled toward the camera a hair.
                effect.setWorldProperty(model.world);
                effect.setViewProperty(frame.camera.view);
                effect.setProjectionProperty(frame.camera.projection);
                effect.SetBoneTransforms(m_bones);
                const Graphics::RasterizerState raster = device.getRasterizerStateProperty();
                Graphics::RasterizerState biased = raster;
                biased.setDepthBiasProperty(-0.00002f);
                biased.setSlopeScaleDepthBiasProperty(-1.0f);
                device.setRasterizerStateProperty(biased);
                DrawSelectionOverlay(device, effect, model);
                device.setRasterizerStateProperty(raster);
            }
            if (model.counted)
            {
                ++m_statistics.modelDraws;
            }
            return;
        }
        effect.setWorldProperty(model.world);
        effect.setViewProperty(frame.camera.view);
        effect.setProjectionProperty(frame.camera.projection);
        effect.SetBoneTransforms(m_bones);
        if (model.translucent)
        {
            device.setBlendStateProperty(Graphics::BlendState::AlphaBlend);
            device.setDepthStencilStateProperty(Graphics::DepthStencilState::DepthRead);
        }
        effect.setAlphaProperty(model.alpha);
        if (model.tinted)
        {
            effect.setDiffuseColorProperty(model.tint);
            effect.setSpecularColorProperty(Vector3::Zero);
        }
        const auto& parts = model.model->Parts;
        for (std::size_t i = 0; i < parts.size(); ++i)
        {
            const auto& part = parts[i];
            if (!PartShown(model, part))
            {
                continue;
            }
            Graphics::Texture2D* texture =
                i < model.partTextures.size() && model.partTextures[i] != nullptr ? model.partTextures[i] : part.Texture;
            effect.setTextureProperty(texture);
            effect.Apply();
            DrawPart(device, part);
            if (model.counted)
            {
                m_statistics.modelTriangles += part.Part->getPrimitiveCountProperty();
            }
        }
        if (model.tinted)
        {
            effect.setDiffuseColorProperty(Vector3::One);
            effect.setSpecularColorProperty(Vector3::One);
        }
        effect.setAlphaProperty(1.0f);
        if (model.translucent)
        {
            device.setBlendStateProperty(Graphics::BlendState::Opaque);
            device.setDepthStencilStateProperty(Graphics::DepthStencilState::Default);
        }
        if (model.editorSelected)
        {
            DrawSelectionOverlay(device, effect, model);
        }
        if (model.counted)
        {
            ++m_statistics.modelDraws;
        }
    }

    bool WorldRenderer::DrawSmoothPart(Graphics::GraphicsDevice& device, const Graphics::ModelMeshPart& part)
    {
        auto found = m_smoothParts.find(&part);
        if (found == m_smoothParts.end())
        {
            SmoothPart smooth;
            try
            {
                auto* vertexBuffer = part.getVertexBufferProperty();
                auto* indexBuffer = part.getIndexBufferProperty();
                const int primitives = part.getPrimitiveCountProperty();
                if (vertexBuffer != nullptr && indexBuffer != nullptr && primitives > 0 && primitives <= 60000)
                {
                    const auto& declaration = vertexBuffer->getVertexDeclarationProperty();
                    SubdivisionLayout layout;
                    layout.stride = declaration.getVertexStrideProperty();
                    for (const auto& element : declaration.GetVertexElements())
                    {
                        const auto usage = element.getVertexElementUsageProperty();
                        const auto format = element.getVertexElementFormatProperty();
                        const int offset = element.getOffsetProperty();
                        if (usage == Graphics::VertexElementUsage::Position && format == Graphics::VertexElementFormat::Vector3)
                            layout.position = offset;
                        else if (usage == Graphics::VertexElementUsage::Normal && format == Graphics::VertexElementFormat::Vector3)
                            layout.normal = offset;
                        else if (usage == Graphics::VertexElementUsage::TextureCoordinate &&
                                 format == Graphics::VertexElementFormat::Vector2 && element.getUsageIndexProperty() == 0)
                            layout.uv = offset;
                        else if (usage == Graphics::VertexElementUsage::BlendWeight && format == Graphics::VertexElementFormat::Vector4)
                            layout.weights = offset;
                        else if (usage == Graphics::VertexElementUsage::BlendIndices && format == Graphics::VertexElementFormat::Byte4)
                            layout.bones = offset;
                    }
                    const int vertexCount = vertexBuffer->getVertexCountProperty();
                    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(vertexCount) * static_cast<std::size_t>(layout.stride));
                    vertexBuffer->GetDataRawEXT(0, bytes.data(), vertexCount, layout.stride);
                    const int count = primitives * 3;
                    std::vector<std::uint32_t> indices(static_cast<std::size_t>(count));
                    if (indexBuffer->getIndexElementSizeProperty() == Graphics::IndexElementSize::SixteenBits)
                    {
                        std::vector<std::uint16_t> narrow(static_cast<std::size_t>(count));
                        indexBuffer->GetData(narrow.data(), part.getStartIndexProperty(), count);
                        std::copy(narrow.begin(), narrow.end(), indices.begin());
                    }
                    else
                    {
                        indexBuffer->GetData(indices.data(), part.getStartIndexProperty(), count);
                    }
                    for (auto& index : indices)
                    {
                        index += static_cast<std::uint32_t>(part.getVertexOffsetProperty());
                    }
                    // Three cuts an edge where the part is small, two where it is already fine.
                    const auto mesh = SubdivideMesh(bytes.data(), vertexCount, indices, layout, primitives <= 4000 ? 3 : 2);
                    if (layout.position >= 0 && mesh.vertexCount > 0 && !mesh.indices.empty())
                    {
                        smooth.vertices = std::make_unique<Graphics::VertexBuffer>(device, declaration, mesh.vertexCount,
                                                                                    Graphics::BufferUsage::None);
                        smooth.vertices->SetDataRaw(mesh.vertices.data(), mesh.vertexCount, layout.stride);
                        smooth.indices = std::make_unique<Graphics::IndexBuffer>(
                            device, Graphics::IndexElementSize::ThirtyTwoBits, static_cast<int>(mesh.indices.size()),
                            Graphics::BufferUsage::None);
                        smooth.indices->SetData(mesh.indices.data(), static_cast<int>(mesh.indices.size()));
                        smooth.vertexCount = mesh.vertexCount;
                        smooth.primitiveCount = static_cast<int>(mesh.indices.size() / 3);
                    }
                }
            }
            catch (const std::exception& error)
            {
                std::cout << "GalaxyEggbertCNA: Ultra: a model part not made smoother (" << error.what() << ")." << std::endl;
                smooth = SmoothPart{};
            }
            found = m_smoothParts.emplace(&part, std::move(smooth)).first;
        }
        const auto& smooth = found->second;
        if (!smooth.vertices || !smooth.indices)
        {
            return false;
        }
        device.SetVertexBuffer(smooth.vertices.get());
        device.SetIndexBuffer(smooth.indices.get());
        device.DrawIndexedPrimitives(Graphics::PrimitiveType::TriangleList, 0, 0, smooth.vertexCount, 0, smooth.primitiveCount);
        return true;
    }

    void WorldRenderer::DrawSelectionOverlay(Graphics::GraphicsDevice& device, Graphics::SkinnedEffect& effect,
                                             const SceneModel& model)
    {
        // The editor's selection: the exact posed mesh again, in depth-tested translucent red.
        if (!m_tintTexture)
        {
            m_tintTexture = std::make_unique<Graphics::Texture2D>(device, 1, 1);
            const auto white = Microsoft::Xna::Framework::Color::White;
            m_tintTexture->SetData(&white, 1);
        }
        const auto diffuse = effect.getDiffuseColorProperty();
        const auto alpha = effect.getAlphaProperty();
        const auto emission = effect.getEmissiveColorProperty();
        effect.setEmissiveColorProperty({1.0f, 0.0f, 0.0f});
        effect.setDiffuseColorProperty({0.0f, 0.0f, 0.0f});
        effect.setAlphaProperty(0.35f);
        effect.setTextureProperty(m_tintTexture.get());
        device.setBlendStateProperty(Graphics::BlendState::NonPremultiplied);
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::DepthRead);
        for (const auto& part : model.model->Parts)
        {
            if (!PartShown(model, part))
            {
                continue;
            }
            effect.Apply();
            DrawPart(device, part);
        }
        effect.setDiffuseColorProperty(diffuse);
        effect.setAlphaProperty(alpha);
        effect.setEmissiveColorProperty(emission);
        device.setBlendStateProperty(Graphics::BlendState::Opaque);
        device.setDepthStencilStateProperty(Graphics::DepthStencilState::Default);
    }

    Graphics::BasicEffect& WorldRenderer::DecalEffect(Graphics::GraphicsDevice& device)
    {
        if (!m_decalEffect)
        {
            m_decalEffect = std::make_unique<Graphics::BasicEffect>(device);
            m_decalEffect->VertexColorEnabled = false;
            m_decalEffect->setTextureEnabledProperty(true);
        }
        return *m_decalEffect;
    }

    void WorldRenderer::DrawDecals(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                   const WorldRenderAssets& assets)
    {
        // The signs' painted faces (MR-0006), each face only from its own side: drawn from both, the
        // plate on the far side of a thin numbered post showed mirrored beside it.
        if (frame.signDecalPass)
        {
            auto& effect = DecalEffect(device);
            FogDecals(effect, frame);
            SetCamera(effect, frame.camera);
            device.setBlendStateProperty(Graphics::BlendState::AlphaBlend);
            device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
            for (const auto& decal : frame.signDecals)
            {
                const auto mesh = QuadMesh(decal.quads);
                if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                {
                    effect.setTextureProperty(decal.texture);
                    renderer->Draw(device, effect);
                }
            }
            device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
            device.setBlendStateProperty(Graphics::BlendState::Opaque);
        }

        // The glue death's strands, in the glue model's plane.
        if (frame.glueStrands.shown && assets.element != nullptr && !frame.glueStrands.mesh.Empty())
        {
            if (auto renderer = UploadTexturedMesh(device, frame.glueStrands.mesh))
            {
                SetCamera(*assets.element, frame.camera);
                renderer->Draw(device, *assets.element);
            }
        }

        // Painted sparks facing the camera over their models (Q-005).
        if (!frame.electrodeSparks.empty() || !frame.dynamiteSparks.empty())
        {
            auto& effect = DecalEffect(device);
            FogDecals(effect, frame);
            SetCamera(effect, frame.camera);
            device.setBlendStateProperty(Graphics::BlendState::AlphaBlend);
            device.setRasterizerStateProperty(Graphics::RasterizerState::CullNone);
            for (const auto* sparks : {&frame.electrodeSparks, &frame.dynamiteSparks})
            {
                for (const auto& spark : *sparks)
                {
                    const auto mesh = QuadMesh(spark.quads);
                    if (auto renderer = UploadTexturedMesh(device, mesh.Mesh()))
                    {
                        effect.setTextureProperty(spark.texture);
                        renderer->Draw(device, effect);
                    }
                }
            }
            device.setRasterizerStateProperty(Graphics::RasterizerState::CullCounterClockwise);
            device.setBlendStateProperty(Graphics::BlendState::Opaque);
        }
    }

    Graphics::BasicEffect* WorldRenderer::SheetEffect(SceneSheet sheet, const WorldRenderAssets& assets) const
    {
        switch (sheet)
        {
            case SceneSheet::Element: return assets.element;
            case SceneSheet::ObjectM: return assets.objectM;
            case SceneSheet::Explo: return assets.explo;
            case SceneSheet::Blupi: return assets.blupi;
            case SceneSheet::Blupi1: return assets.blupi1;
            case SceneSheet::BigDecor: return assets.bigDecor;
        }
        return nullptr;
    }

    void WorldRenderer::DrawBillboards(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                       const WorldRenderAssets& assets)
    {
        // Rebuilt every frame: a billboard's corners follow the camera's right and up.
        const bool shadowedSprites = m_modelShading && m_scene.ShadowMapped();
        if (m_billboardMeshes.size() < frame.billboards.size())
        {
            m_billboardMeshes.resize(frame.billboards.size());
        }
        for (std::size_t i = 0; i < frame.billboards.size(); ++i)
        {
            const auto& batch = frame.billboards[i];
            auto* effect = SheetEffect(batch.sheet, assets);
            if (effect == nullptr || batch.quads.empty())
            {
                continue;
            }
            const auto mesh = QuadMesh(batch.quads);
            if (mesh.Mesh().Empty())
            {
                continue;
            }
            m_billboardMeshes[i] = UploadTexturedMesh(device, mesh.Mesh());
            if (shadowedSprites)
            {
                // High: the sprite darkened where the sun's shadow falls on it.
                m_billboardMeshes[i]->Draw(device, m_scene.SpriteWith(effect->getTextureProperty(), batch.alpha));
                continue;
            }
            SetCamera(*effect, frame.camera);
            if (batch.alpha != 1.0f)
            {
                effect->setAlphaProperty(batch.alpha);
            }
            m_billboardMeshes[i]->Draw(device, *effect);
            if (batch.alpha != 1.0f)
            {
                effect->setAlphaProperty(1.0f);
            }
        }
        // The big decor's marine cells: the object channel's animation, on object-m.png.
        if (assets.objectM != nullptr && !frame.marine.empty())
        {
            const auto mesh = QuadMesh(frame.marine);
            if (!mesh.Mesh().Empty())
            {
                m_marineMesh = UploadTexturedMesh(device, mesh.Mesh());
                if (shadowedSprites)
                {
                    m_marineMesh->Draw(device, m_scene.SpriteWith(assets.objectM->getTextureProperty(), 1.0f));
                    return;
                }
                SetCamera(*assets.objectM, frame.camera);
                m_marineMesh->Draw(device, *assets.objectM);
            }
        }
    }
}
