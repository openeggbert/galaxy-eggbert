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
//
// Ultra (SceneEffects::ultra) draws the terrain and the models with programs of their own, compiled
// apart (the other levels' programs are untouched): physically based light in linear light, the sky's
// light from the world's background picture, the atlas's surface maps, the screen's occlusion
// (UltraAssets.hpp). Where they do not compile, Ultra's world is lit as High's.

#include <GalaxyEggbert/Game/SceneFrame.hpp>
#include <GalaxyEggbert/Game/UltraAssets.hpp>
#include <GalaxyEggbert/Game/UltraLogic.hpp>

#include <Microsoft/Xna/Framework/Graphics/BasicEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SkinnedModelEXT.hpp>
#include <Microsoft/Xna/Framework/Graphics/Viewport.hpp>
#include <Microsoft/Xna/Framework/Matrix.hpp>

#include <array>
#include <memory>
#include <vector>

namespace GalaxyEggbert::Game
{
    class SceneShaders
    {
    public:
        static constexpr int kMaxBones = 56;     // the real Blupi's skeleton; WebGL 2's uniform floor
        static constexpr int kMaxLights = 8;
        static constexpr int kShadowSize = 1024; // texels a side, each cascade
        static constexpr std::size_t kCascades = 2;
        static constexpr float kNearSpan = 44.0f; // blocks across: a 23rd of a block a texel
        static constexpr float kFarSpan = 160.0f;

        // Compiles everything on first use; false when the device cannot (never tried again).
        bool Prepare(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        [[nodiscard]] bool Ready() const noexcept { return m_ready; }

        // --- The shadow map: the casters drawn from the sun ---------------------------------------
        // Binds cascade `cascade`'s target (0 near, 1 far), its camera from the sun. Draw the casters
        // into each in turn; EndShadowPass() unbinds the last (back to the screen) and restores the
        // viewport.
        void BeginShadowPass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                             int cascade);
        void EndShadowPass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        [[nodiscard]] bool Casting() const noexcept { return m_casting; }
        // The cascade's span (blocks across) and the way the sunlight goes.
        [[nodiscard]] float CascadeSpan(int cascade) const noexcept { return m_cascadeSpan[static_cast<std::size_t>(cascade)]; }
        [[nodiscard]] static Microsoft::Xna::Framework::Vector3 SunDirection();
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect& DepthSolid() { return *m_depthSolid; }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect& DepthCutout() { return *m_depthCutout; }
        // The skinned casters' effect set to these bones and world; nullptr when the skeleton is too big.
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* SkinnedCaster(
            const std::vector<Microsoft::Xna::Framework::Matrix>& bones, const Microsoft::Xna::Framework::Matrix& world);
        [[nodiscard]] bool ShadowMapped() const noexcept { return m_shadowValid; }
        void DropShadowMap() noexcept { m_shadowValid = m_farReady = false; }

        // --- The main pass ----------------------------------------------------------------------------
        // The frame's camera, fog, lights and shadow for every shader; the terrain's texture and camera.
        // `ultra` (Ultra's lending, nullptr otherwise) switches the terrain's and the models' programs to
        // Ultra's for this frame.
        void SetFrame(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame,
                      const Microsoft::Xna::Framework::Graphics::BasicEffect& terrain,
                      const UltraSceneAssets* ultra = nullptr);
        [[nodiscard]] bool UltraActive() const noexcept { return m_ultraActive; }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Occluded() const
        {
            return m_ultraActive ? m_ultraOccluded.get() : m_occluded.get();
        }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Lit() const
        {
            return m_ultraActive ? m_ultraLit.get() : m_lit.get();
        }
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect* Cutout() const { return m_cutout.get(); }
        // A lit (position, normal, uv) mesh with its own texture and world (the cubes, the grass).
        Microsoft::Xna::Framework::Graphics::ShaderEffect& LitWith(Microsoft::Xna::Framework::Graphics::Texture2D* texture,
                                                                  const Microsoft::Xna::Framework::Matrix& world);
        // The models: their SkinnedEffect's lights and material, these bones, world, alpha and tint.
        bool PrepareModel(Microsoft::Xna::Framework::Graphics::SkinnedEffect& stock,
                          const std::vector<Microsoft::Xna::Framework::Matrix>& bones,
                          const Microsoft::Xna::Framework::Matrix& world, float alpha, bool tinted,
                          const Microsoft::Xna::Framework::Vector3& tint);
        [[nodiscard]] Microsoft::Xna::Framework::Graphics::ShaderEffect& Model()
        {
            return m_ultraActive ? *m_ultraModel : *m_model;
        }
        // The part about to be drawn with Model(): its Ray Dream material (nullptr: its texture as it is).
        void SetPartMaterial(const ProceduralMaterial* material);
        // The billboards (position, uv) on this sheet's texture, at this alpha, darkened in the sun's shadow.
        Microsoft::Xna::Framework::Graphics::ShaderEffect& SpriteWith(Microsoft::Xna::Framework::Graphics::Texture2D* texture,
                                                                     float alpha);

    private:
        void SetCommon(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect);
        void SetUltra(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const UltraSceneAssets& ultra);
        static void SetBones(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect,
                             const std::vector<Microsoft::Xna::Framework::Matrix>& bones);

        bool m_tried = false;
        bool m_ready = false;
        bool m_shadowValid = false;
        int m_shadowSize = 2048;
        float m_debug = 0.0f;
        bool m_casting = false;
        bool m_farReady = false; // the far cascade drawn since the shadow map was last dropped
        std::array<Microsoft::Xna::Framework::Matrix, kCascades> m_lightViewProjection{};
        std::array<float, kCascades> m_cascadeSpan{};
        Microsoft::Xna::Framework::Graphics::Viewport m_savedViewport;
        const SceneFrame* m_frame = nullptr;
        std::vector<float> m_boneData;
        std::array<std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D>, kCascades> m_shadowMaps;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_depthSolid;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_depthCutout;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_depthSkinned;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_occluded;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_lit;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_cutout;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_model;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_sprite;
        // Ultra's programs, compiled on its first frame.
        bool m_ultraTried = false;
        bool m_ultraReady = false;
        bool m_ultraActive = false;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_ultraOccluded;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_ultraLit;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_ultraModel;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> m_white; // 1x1, for what is missing
        Microsoft::Xna::Framework::Graphics::SamplerState m_envSampler;
        UltraSurface m_modelSurface; // the model being drawn's own (its specular power)
    };
}
