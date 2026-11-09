#pragma once

// The 3D effects' Ultra finish (the Setup's "3D effects", Ultra): the world drawn in HDR at the
// window's own resolution, then put on the screen as a film camera would expose it.
//
//   the scene target   RGBA16F (HdrBlendable) at the window's pixels, its depth too. What the world's
//                      shaders write is the display colour they always wrote, but no longer clipped at 1:
//                      lava, the lightning, the sun on water and the lights go above it.
//   auto exposure      the picture's mean log luminance, reduced on the GPU (64x64 .. 1x1), and an
//                      exposure that follows it over about a second (two 1x1 targets in turn)
//   HDR bloom          what is brighter than white once exposed, down a chain of halves and back up
//                      (each level's tent added to the one above): a wide soft glow, no hard cut-off
//   tonemapping        ACES (Stephen Hill's fit of the RRT and ODT): highlights roll off like film's
//   grading            a 32x32x32 colour table per world, built from its sky's colour (UltraLogic.hpp
//                      BuildGradeTable)
//   anti-aliasing      FXAA over the result (TAA replaces it where the frames' history allows)
//
// RGBA16F samples only with point filtering in XNA's rules (CNA keeps them): every pass fetches its
// texels itself (Bilinear() where it needs between them). Everything stays on the GPU; nothing is read
// back. Where render targets or GLSL are missing, Begin() is false and the caller falls back.

#include <GalaxyEggbert/Game/SceneFrame.hpp>
#include <GalaxyEggbert/Game/UltraLogic.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>
#include <Microsoft/Xna/Framework/Graphics/Texture2D.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

namespace GalaxyEggbert::Game
{
    class UltraFinish
    {
    public:
        static constexpr int kGradeSize = kGradeTableSize;
        static constexpr int kBloomLevels = 6;

        // Draws what follows into the HDR scene target of `width` x `height` pixels (cleared to `clear`,
        // its depth too). False when the device cannot: nothing is bound then.
        bool Begin(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                   const Microsoft::Xna::Framework::Color& clear, int width, int height);
        // The geometry pass (before Begin): binds its target (the same size), cleared; WorldRenderer::
        // DrawGeometry draws into it. EndGeometry() unbinds it and makes the screen's occlusion of it
        // (ambient occlusion r, the sun's contact shadow g; at the scene's size, for its shaders), which
        // it returns. Without a geometry pass this frame, no occlusion and no reflections.
        bool BeginGeometry(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int width, int height);
        Microsoft::Xna::Framework::Graphics::Texture2D* EndGeometry(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                                                                    const SceneFrame& frame,
                                                                    const Microsoft::Xna::Framework::Vector3& sunDirection);
        // Reflects (the water: screen-space reflections), exposes, glows, tonemaps and grades the scene
        // onto the screen (the viewport's logical surface).
        void End(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, const SceneFrame& frame);

        [[nodiscard]] int Width() const noexcept { return m_width; }
        [[nodiscard]] int Height() const noexcept { return m_height; }

    private:
        bool Prepare(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int width, int height);
        void Pass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::RenderTarget2D* target,
                  Microsoft::Xna::Framework::Graphics::Texture2D& source,
                  Microsoft::Xna::Framework::Graphics::ShaderEffect& effect, int width, int height,
                  bool add = false);
        void Exposure(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        void SetCamera(Microsoft::Xna::Framework::Graphics::ShaderEffect& effect, const SceneFrame& frame) const;
        void Bloom(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device);
        void UpdateGrade(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                         const Microsoft::Xna::Framework::Vector3& sky);

        bool m_failed = false;
        int m_width = 0;
        int m_height = 0;
        int m_screenWidth = 0;
        int m_screenHeight = 0;
        int m_exposureIndex = 0;   // which of the two 1x1 targets holds this frame's exposure
        bool m_exposureFresh = true; // the first frame takes the picture's exposure at once
        bool m_gradeReady = false;
        bool m_geometryValid = false; // this frame's geometry drawn
        unsigned m_frame = 0;
        Microsoft::Xna::Framework::Vector3 m_gradeSky;

        using Target = std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D>;
        using Shader = std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect>;
        Target m_scene;
        std::array<Target, 4> m_luminance;     // 64, 16, 4, 1
        std::array<Target, 2> m_exposure;      // 1x1 each, in turn
        std::array<Target, kBloomLevels> m_bloom; // halves of the scene
        Target m_graded;                       // tonemapped, before the anti-aliasing
        Target m_geometry;                     // normal (octahedral), depth, mirror; its depth buffer
        Target m_occlusionRaw;
        Target m_occlusionSmooth;              // the shaders' screen occlusion
        Target m_reflection;                   // the reflections: colour, weight
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::Texture2D> m_grade;
        Shader m_logLuminance;
        Shader m_reduce;
        Shader m_adapt;
        Shader m_bloomPrefilter;
        Shader m_bloomDown;
        Shader m_bloomUp;
        Shader m_tonemap;
        Shader m_fxaa;
        Shader m_occlusion;
        Shader m_occlusionBlur;
        Shader m_reflections;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> m_batch;
    };
}
