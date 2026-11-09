#pragma once

// The 3D effects' lighting (the Setup's "3D effects"): the terrain and the models drawn with GLSL of their
// own instead of the stock effects, so that
//  - the terrain's corners are shaded by how shut in they are (ambient occlusion, baked by TerrainRenderer),
//  - everything solid throws a real shadow from the sun (a shadow map: the world seen from the sun, its
//    depth kept in a render target, looked up by the terrain and the models),
//  - lava, explosions, the lightning and the electrodes light what is near them (point lights).
// The terrain is lit as BasicEffect lights it (TerrainLighting.hpp), the models as their SkinnedEffect
// is set up (its own lights read from it), so the classic look stays where no effect falls.
//
// Desktop GLSL 3.30 or GLSL ES 3.00 (ShaderEffect); where they do not compile, Ready() is false and the
// stock effects draw as without the effects (the fog, the soft round shadows and the particles remain).

#include <GalaxyEggbert/Game/SceneFrame.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp>
#include <Microsoft/Xna/Framework/Graphics/Viewport.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <memory>
#include <vector>

namespace GalaxyEggbert::Game
{
    class SceneShaders
    {
    public:
        static constexpr int kMaxBones = 56;     // the real Blupi's skeleton; WebGL 2's uniform floor
        static constexpr int kMaxLights = 8;

        // Compiles everything on first use; false when the device cannot (never tried again).
        bool Prepare(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        [[nodiscard]] bool Ready() const noexcept { return m_ready; }

        // --- The shadow map: the casters drawn from the sun ---------------------------------------
        // Binds the shadow target, its camera round the frame's camera target. EndShadowPass() unbinds
        // it (back to the screen) and restores the viewport.
        void BeginShadowPass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame);
        void EndShadowPass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect& DepthSolid() { return *m_depthSolid; }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect& DepthCutout() { return *m_depthCutout; }
        // The skinned casters' effect set to these bones and world; nullptr when the skeleton is too big.
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* SkinnedCaster(
            const std::vector<Microsoft::Xna::Framework::Matrix>& bones, const Microsoft::Xna::Framework::Matrix& world);
        [[nodiscard]] bool ShadowMapped() const noexcept { return m_shadowValid; }

        // --- The main pass ----------------------------------------------------------------------------
        // The frame's camera, fog, lights and shadow for every shader; the terrain's texture and camera.
        void SetFrame(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                      const Microsoft::Xna::Framework::Graphics::BasicEffect& terrain);
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Occluded() const { return m_occluded.get(); }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Lit() const { return m_lit.get(); }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Cutout() const { return m_cutout.get(); }
        // A lit (position, normal, uv) mesh with its own texture and world (the cubes, the grass).
        Microsoft::Xna::Framework::Graphics::ShaderEffect& LitWith(Microsoft::Xna::Framework::Graphics::Texture2D* texture,
                                                                  const Microsoft::Xna::Framework::Matrix& world);
        // The models: their SkinnedEffect's lights and material, these bones, world, alpha and tint.
        bool PrepareModel(Microsoft::Xna::Framework::Graphics::SkinnedEffect& stock,
                          const std::vector<Microsoft::Xna::Framework::Matrix>& bones,
                          const Microsoft::Xna::Framework::Matrix& world, float alpha, bool tinted,
                          const Microsoft::Xna::Framework::Vector3& tint);
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect& Model() { return *m_model; }

    private:
        void SetCommon(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
        static void SetBones(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect,
                             const std::vector<Microsoft::Xna::Framework::Matrix>& bones);

        bool m_tried = false;
        bool m_ready = false;
        bool m_shadowValid = false;
        int m_shadowSize = 2048;
        float m_debug = 0.0f;
        Microsoft::Xna::Framework::Matrix m_lightView;
        Microsoft::Xna::Framework::Matrix m_lightProjection;
        Microsoft::Xna::Framework::Matrix m_lightViewProjection;
        Microsoft::Xna::Framework::Graphics::Viewport m_savedViewport;
        const SceneFrame* m_frame = nullptr;
        std::vector<float> m_boneData;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> m_shadowMap;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_depthSolid;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_depthCutout;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_depthSkinned;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_occluded;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_lit;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_cutout;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_model;
    };
}
