#pragma once

// The 3D effects' finish (the Setup's "3D effects"): the world is drawn into a render target, then put on
// the screen with what shines glowing (bloom: the bright parts, blurred, laid over the picture -- lava,
// the lightning, explosions, the sun on water) and a light grade (a touch more colour, the corners a
// little darker), then smoothed (FXAA: the stair-steps along the blocks' edges blended away). The HUD is
// drawn after it, untouched.
//
// Render targets and GLSL source (desktop GLSL or GLSL ES, ShaderEffect); where either is missing,
// Begin() is false and the world is drawn straight to the screen as without the effects.

#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/GraphicsDevice.hpp>
#include <Microsoft/Xna/Framework/Graphics/RenderTarget2D.hpp>
#include <Microsoft/Xna/Framework/Graphics/ShaderEffect.hpp>
#include <Microsoft/Xna/Framework/Graphics/SpriteBatch.hpp>

#include <memory>

namespace GalaxyEggbert::Game
{
    // The full-screen passes' vertex shader (SpriteBatch's vertices: TexCoord across the source) and
    // the FXAA fragment shader, shared with the Ultra finish (UltraFinish.hpp).
    [[nodiscard]] const char* PostVertexSource() noexcept;
    [[nodiscard]] const char* FxaaFragmentSource() noexcept;

    class PostProcess
    {
    public:
        // Draws what follows into the scene target (cleared to `clear`, its depth too). False when the
        // device cannot: then nothing is bound and the caller draws to the screen.
        // `width` x `height` (when given) is the target's size in pixels where it differs from the
        // viewport's (ultra: the window's own pixels, the viewport being the 800x480 logical surface's).
        bool Begin(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                   const Microsoft::Xna::Framework::Color& clear, int width = 0, int height = 0);
        // Back to the screen: the scene smoothed (FXAA), with its glow and grade when `glow`.
        void End(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, bool glow = true);

    private:
        bool Prepare(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device, int width, int height);
        void Pass(Microsoft::Xna::Framework::Graphics::GraphicsDevice& device,
                  Microsoft::Xna::Framework::Graphics::RenderTarget2D* target,
                  Microsoft::Xna::Framework::Graphics::Texture2D& source,
                  Microsoft::Xna::Framework::Graphics::ShaderEffect& effect, int width, int height);

        bool m_failed = false;
        int m_width = 0;       // the targets'
        int m_height = 0;
        int m_screenWidth = 0; // the viewport's, where the result goes
        int m_screenHeight = 0;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> m_scene;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> m_bright;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> m_blur;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::RenderTarget2D> m_graded; // before the smoothing
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_brightPass;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_blurPass;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_composite;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::ShaderEffect> m_fxaa;
        std::unique_ptr<Microsoft::Xna::Framework::Graphics::SpriteBatch> m_batch;
    };
}
