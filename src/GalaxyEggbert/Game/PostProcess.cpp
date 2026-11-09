#include <GalaxyEggbert/Game/PostProcess.hpp>

#include <GalaxyEggbert/Game/SurfaceShaders.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/Viewport.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>

#include <iostream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        namespace Graphics = Microsoft::Xna::Framework::Graphics;

        // SpriteBatch's vertices (CNA's sprite layout); the quad's corners map the source whole.
        const char* const kPostVertex = R"(
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
uniform mat4 projection;
out vec2 TexCoord;
void main()
{
    gl_Position = projection * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

        // What shines: the bright part of each pixel, a soft knee so the glow fades in.
        const char* const kBrightFragment = R"(
uniform sampler2D uTexture;
in vec2 TexCoord;
out vec4 FragColor;
void main()
{
    vec3 c = texture(uTexture, TexCoord).rgb;
    float luma = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float peak = max(c.r, max(c.g, c.b));
    float shine = smoothstep(0.78, 0.98, luma) * smoothstep(0.85, 1.0, peak);
    FragColor = vec4(c * shine, 1.0);
}
)";

        // A nine-tap Gaussian along uStep (one texel across, then one down).
        const char* const kBlurFragment = R"(
uniform sampler2D uTexture;
uniform vec2 uStep;
in vec2 TexCoord;
out vec4 FragColor;
void main()
{
    vec3 sum = texture(uTexture, TexCoord).rgb * 0.227027;
    sum += (texture(uTexture, TexCoord + uStep * 1.3846).rgb + texture(uTexture, TexCoord - uStep * 1.3846).rgb) * 0.3162162;
    sum += (texture(uTexture, TexCoord + uStep * 3.2308).rgb + texture(uTexture, TexCoord - uStep * 3.2308).rgb) * 0.0702703;
    FragColor = vec4(sum, 1.0);
}
)";

        // The picture with its glow laid over it (screen: it brightens without clipping), a touch more
        // colour and the corners a little darker.
        const char* const kCompositeFragment = R"(
uniform sampler2D uTexture;
uniform sampler2D uBloom;
in vec2 TexCoord;
out vec4 FragColor;
void main()
{
    vec3 scene = texture(uTexture, TexCoord).rgb;
    vec3 glow = texture(uBloom, TexCoord).rgb * 0.9;
    vec3 c = 1.0 - (1.0 - scene) * (1.0 - clamp(glow, 0.0, 1.0));
    float grey = dot(c, vec3(0.2126, 0.7152, 0.0722));
    c = mix(vec3(grey), c, 1.08);
    vec2 d = TexCoord - 0.5;
    c *= 1.0 - dot(d, d) * 0.32;
    FragColor = vec4(clamp(c, 0.0, 1.0), 1.0);
}
)";
    }

    bool PostProcess::Prepare(Graphics::GraphicsDevice& device, int width, int height)
    {
        if (m_failed)
        {
            return false;
        }
        if (!m_composite)
        {
            m_brightPass = CompileShaderEffect(device, "the glow's bright pass", kPostVertex, kBrightFragment);
            m_blurPass = CompileShaderEffect(device, "the glow's blur", kPostVertex, kBlurFragment);
            m_composite = CompileShaderEffect(device, "the glow's composite", kPostVertex, kCompositeFragment);
            if (!m_brightPass || !m_blurPass || !m_composite)
            {
                m_failed = true;
                return false;
            }
            m_composite->SetUniformInt("uBloom", 1);
            m_batch = std::make_unique<Graphics::SpriteBatch>(device);
        }
        if (width != m_width || height != m_height || !m_scene)
        {
            try
            {
                m_scene.reset();
                m_bright.reset();
                m_blur.reset();
                m_scene = std::make_unique<Graphics::RenderTarget2D>(device, width, height, false, Graphics::SurfaceFormat::Color,
                                                                     Graphics::DepthFormat::Depth24Stencil8);
                const int halfW = std::max(1, width / 2);
                const int halfH = std::max(1, height / 2);
                m_bright = std::make_unique<Graphics::RenderTarget2D>(device, halfW, halfH, false, Graphics::SurfaceFormat::Color,
                                                                      Graphics::DepthFormat::None);
                m_blur = std::make_unique<Graphics::RenderTarget2D>(device, halfW, halfH, false, Graphics::SurfaceFormat::Color,
                                                                    Graphics::DepthFormat::None);
                m_width = width;
                m_height = height;
            }
            catch (const std::exception& error)
            {
                std::cout << "GalaxyEggbertCNA: 3D effects: no render targets (" << error.what() << "), no glow." << std::endl;
                m_failed = true;
                return false;
            }
        }
        return true;
    }

    bool PostProcess::Begin(Graphics::GraphicsDevice& device, const Microsoft::Xna::Framework::Color& clear)
    {
        const auto viewport = device.getViewportProperty();
        const int width = viewport.getWidthProperty();
        const int height = viewport.getHeightProperty();
        if (width <= 0 || height <= 0 || !Prepare(device, width, height))
        {
            return false;
        }
        device.SetRenderTarget(m_scene.get());
        device.Clear(clear);
        return true;
    }

    void PostProcess::Pass(Graphics::GraphicsDevice& device, Graphics::RenderTarget2D* target, Graphics::Texture2D& source,
                           Graphics::ShaderEffect& effect, int width, int height)
    {
        device.SetRenderTarget(target);
        m_batch->Begin(Graphics::SpriteSortMode::Immediate, &Graphics::BlendState::Opaque, &Graphics::SamplerState::LinearClamp,
                       &Graphics::DepthStencilState::None, &Graphics::RasterizerState::CullNone, &effect);
        m_batch->Draw(source, Microsoft::Xna::Framework::Rectangle(0, 0, width, height),
                      Microsoft::Xna::Framework::Rectangle(0, 0, source.getWidthProperty(), source.getHeightProperty()),
                      Microsoft::Xna::Framework::Color::White);
        m_batch->End();
    }

    void PostProcess::End(Graphics::GraphicsDevice& device)
    {
        // The device's states as the world left them, for what is drawn after (the HUD).
        const Graphics::BlendState blend = device.getBlendStateProperty();
        const Graphics::DepthStencilState depth = device.getDepthStencilStateProperty();
        const Graphics::RasterizerState raster = device.getRasterizerStateProperty();
        const Graphics::SamplerState sampler = device.getSamplerStatesProperty()[0];

        const int halfW = m_bright->getWidthProperty();
        const int halfH = m_bright->getHeightProperty();
        Pass(device, m_bright.get(), *m_scene, *m_brightPass, halfW, halfH);
        m_blurPass->SetUniformVec2("uStep", 1.0f / static_cast<float>(halfW), 0.0f);
        Pass(device, m_blur.get(), *m_bright, *m_blurPass, halfW, halfH);
        m_blurPass->SetUniformVec2("uStep", 0.0f, 1.0f / static_cast<float>(halfH));
        Pass(device, m_bright.get(), *m_blur, *m_blurPass, halfW, halfH);

        device.SetRenderTarget(nullptr);
        m_composite->SetTexture(1, *m_bright);
        Pass(device, nullptr, *m_scene, *m_composite, m_width, m_height);

        device.setBlendStateProperty(blend);
        device.setDepthStencilStateProperty(depth);
        device.setRasterizerStateProperty(raster);
        device.getSamplerStatesProperty()[0] = sampler;
    }
}
