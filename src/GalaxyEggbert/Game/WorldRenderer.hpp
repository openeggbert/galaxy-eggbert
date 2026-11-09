#pragma once

// WorldRenderer -- draws the 3D world from a SceneFrame (SceneFrame.hpp: what to draw; this: how).
// It holds what drawing needs from frame to frame (the per-frame GPU meshes, the decal effect, the
// editor's tint texture); the content it draws with -- the loaded effects, textures, models and the
// terrain's meshes -- belongs to the game, which lends it each frame (WorldRenderAssets).

#include <GalaxyEggbert/Game/UltraAssets.hpp>
#include <GalaxyEggbert/Game/SceneFrame.hpp>
#include <GalaxyEggbert/Game/SceneShaders.hpp>
#include <GalaxyEggbert/Game/SurfaceShaders.hpp>

#include <CnaEngine/Graphics/GpuMesh.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>

#include <functional>
#include <memory>
#include <vector>

namespace GalaxyEggbert::Game
{
    class TerrainRenderer;

    // The game's loaded content the world is drawn with; any of it may be missing (nullptr), and
    // then what it draws is left out, as before.
    struct WorldRenderAssets
    {
        Microsoft::Xna::Framework::Graphics::BasicEffect* background = nullptr; // its panorama bound
        Microsoft::Xna::Framework::Graphics::BasicEffect* terrain = nullptr;
        Microsoft::Xna::Framework::Graphics::BasicEffect* grass = nullptr;
        Microsoft::Xna::Framework::Graphics::BasicEffect* cubes = nullptr;
        Microsoft::Xna::Framework::Graphics::SkinnedEffect* models = nullptr;
        Microsoft::Xna::Framework::Graphics::BasicEffect* element = nullptr;  // element.png
        Microsoft::Xna::Framework::Graphics::BasicEffect* objectM = nullptr;  // object-m.png
        Microsoft::Xna::Framework::Graphics::BasicEffect* explo = nullptr;    // explo.png
        Microsoft::Xna::Framework::Graphics::BasicEffect* blupi = nullptr;    // blupi.png
        Microsoft::Xna::Framework::Graphics::BasicEffect* blupi1 = nullptr;   // blupi1.png
        Microsoft::Xna::Framework::Graphics::BasicEffect* bigDecor = nullptr; // explo.png, big decor
        TerrainRenderer* terrainRenderer = nullptr;
        const UltraSceneAssets* ultra = nullptr; // Ultra's sky, maps and screen occlusion
    };

    struct WorldRenderStatistics
    {
        long long modelDraws = 0;
        long long modelTriangles = 0;
        long long modelsCulled = 0;
    };

    class WorldRenderer
    {
    public:
        // Called once the opaque and translucent terrain, every model and decal are drawn, before the
        // billboards (the place of the game's one-shot terrain check).
        using TerrainDrawnHook = std::function<void(Microsoft::Xna::Framework::Graphics::GraphicsDevice&)>;

        void Draw(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                  const WorldRenderAssets& assets, const TerrainDrawnHook& terrainDrawn = {});
        // The 3D effects' shadow map (SceneShaders.hpp): what throws a shadow, drawn from the sun into its
        // target. Before Draw() and before anything is drawn into the frame's own target; leaves the
        // screen bound. Nothing without the effects or where the device cannot.
        // Ultra's geometry pass into the bound target (UltraFinish::BeginGeometry): each pixel's normal,
        // depth and mirror. False when there is none (no world, below Ultra, no programs).
        bool DrawGeometry(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                          const WorldRenderAssets& assets);
        void RenderShadowMap(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                             const WorldRenderAssets& assets);
        // Whether the terrain should be built with its ambient occlusion: the effects' terrain shader
        // compiles on this device (compiled here on first ask).
        [[nodiscard]] bool TerrainShadersReady(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device)
        {
            return m_scene.Prepare(device);
        }

        [[nodiscard]] const WorldRenderStatistics& Statistics() const noexcept { return m_statistics; }
        void ResetStatistics() noexcept { m_statistics = {}; }

    private:
        void DrawBackground(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                            const WorldRenderAssets& assets);
        void DrawTerrainPass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                             const WorldRenderAssets& assets, const TerrainDrawnHook& terrainDrawn);
        void DrawCubes(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                       const WorldRenderAssets& assets);
        void DrawModel(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                       Microsoft::Xna::Framework::Graphics::SkinnedEffect& effect, const SceneModel& model);
        void DrawSelectionOverlay(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                  Microsoft::Xna::Framework::Graphics::SkinnedEffect& effect, const SceneModel& model);
        void DrawDecals(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                        const WorldRenderAssets& assets);
        void DrawBillboards(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                            const WorldRenderAssets& assets);
        // The effects layer (SceneEffects, WorldEffects.cpp): fog on every world effect (or off again),
        // the shadows on the ground, the particles.
        void ApplyFog(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                      const WorldRenderAssets& assets);
        void DrawShadows(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame);
        void DrawParticles(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                           const WorldRenderAssets& assets);
        Microsoft::Xna::Framework::Graphics::BasicEffect& DecalEffect(
            Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        Microsoft::Xna::Framework::Graphics::BasicEffect* SheetEffect(SceneSheet sheet,
                                                                      const WorldRenderAssets& assets) const;

        std::unique_ptr<CnaEngine::Graphics::GpuMesh> m_backgroundMesh;
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> m_cubeMesh;
        std::vector<std::unique_ptr<CnaEngine::Graphics::GpuMesh>> m_billboardMeshes; // one per batch
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> m_marineMesh;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> m_decalEffect;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> m_tintTexture;
        std::vector<Microsoft::Xna::Framework::Matrix> m_bones;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::BasicEffect> m_shadowEffect;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> m_shadowTexture;
        std::unique_ptr<CnaEngine::Graphics::GpuMesh> m_shadowMesh;
        std::vector<std::unique_ptr<CnaEngine::Graphics::GpuMesh>> m_particleMeshes;
        SurfaceShaders m_surfaces;
        SceneShaders m_scene;
        unsigned m_shadowFrame = 0;
        bool m_sceneShading = false; // this frame's terrain drawn with m_scene (medium and high)
        bool m_modelShading = false; // ... and the models, cubes and grass (high)
        WorldRenderStatistics m_statistics;
    };
}
