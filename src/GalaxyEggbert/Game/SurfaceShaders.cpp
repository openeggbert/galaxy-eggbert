#include <GalaxyEggbert/Game/SurfaceShaders.hpp>

#include <GalaxyEggbert/Game/TerrainLighting.hpp>

#include <CNA/ShaderDialectEXT.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>

namespace GalaxyEggbert::Game
{
    namespace
    {
        namespace Graphics = Microsoft::Xna::Framework::Graphics;

        // The terrain's vertices (VertexPositionNormalTexture), lit and fogged as BasicEffect does:
        // stock depth (XNA's [0, 1], as CNA's own shaders write it), the view-space depth for the fog.
        const char* const kSurfaceVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out vec2 vUv;
out vec3 vNormal;
out vec3 vWorld;
out float vDepth;
void main()
{
    vec4 world = World * vec4(aPosition, 1.0);
    vec4 view = View * world;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
    vUv = aTexCoord;
    vNormal = mat3(World) * aNormal;
    vWorld = world.xyz;
    vDepth = -view.z;
}
)";

        // Shared by both fragment shaders: the terrain's light and BasicEffect's fog.
        const char* const kSurfaceCommon = R"(
uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec2 uLight;      // ambient, diffuse
uniform vec3 uCamera;
uniform vec3 uFogColor;
uniform vec3 uFog;        // start, end, on
uniform vec3 uSky;
uniform float uTime;
in vec2 vUv;
in vec3 vNormal;
in vec3 vWorld;
in float vDepth;
out vec4 FragColor;
vec3 Lit(vec3 albedo, vec3 n)
{
    return albedo * (uLight.x + uLight.y * max(0.0, dot(n, -uLightDir)));
}
vec4 Fogged(vec4 color)
{
    if (uFog.z > 0.5)
    {
        float f = clamp((vDepth - uFog.x) / max(0.001, uFog.y - uFog.x), 0.0, 1.0);
        color.rgb = mix(color.rgb, uFogColor * color.a, f);
    }
    return color;
}
float Hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float Noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(Hash(i), Hash(i + vec2(1.0, 0.0)), u.x), mix(Hash(i + vec2(0.0, 1.0)), Hash(i + vec2(1.0, 1.0)), u.x), u.y);
}
)";

        // Water: its own tile, its top rippling (moving waves bend the light), the sky's colour on it at a
        // glancing view and the sun glinting off it; a little clearer looked into straight down.
        const char* const kWaterFragment = R"(
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
    float top = clamp(n.y, 0.0, 1.0);
    vec2 q = vWorld.xz;
    float t = uTime;
    vec2 slope = vec2(cos(q.x * 3.1 + t * 1.7) * 0.6 + cos((q.x + q.y) * 4.3 - t * 2.3) * 0.4,
                      cos(q.y * 2.7 - t * 1.3) * 0.6 + cos((q.y - q.x) * 3.7 + t * 1.9) * 0.4);
    vec3 waved = normalize(n + vec3(slope.x, 0.0, slope.y) * 0.16 * top);
    vec3 color = Lit(tex.rgb, waved);
    // Light dancing on the surface (caustics), slowly drifting.
    float dance = sin(q.x * 5.3 + t * 2.1) * sin(q.y * 4.7 - t * 1.7) + sin((q.x - q.y) * 3.1 + t * 1.3) * 0.5;
    color *= 1.0 + 0.09 * dance * top;
    vec3 toEye = normalize(uCamera - vWorld);
    float glance = pow(1.0 - max(0.0, dot(waved, toEye)), 3.0) * top;
    color = mix(color, uSky, glance * 0.45);
    vec3 halfway = normalize(-uLightDir + toEye);
    color += vec3(1.0, 0.97, 0.9) * pow(max(0.0, dot(waved, halfway)), 80.0) * 0.55 * top;
    float alpha = tex.a * mix(0.82, 1.0, glance);
    FragColor = Fogged(vec4(color, alpha));
}
)";

        // Lava: its own tile, glowing from within: a slow pulse, and hot spots crawling over it, brighter
        // than the light alone could make them (they bloom).
        const char* const kLavaFragment = R"(
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
    vec3 color = Lit(tex.rgb, n);
    float t = uTime;
    vec2 q = vWorld.xz * 1.7 + vec2(t * 0.11, -t * 0.07);
    float heat = Noise(q) * 0.6 + Noise(q * 2.3 - vec2(t * 0.17, t * 0.13)) * 0.4;
    float pulse = 0.5 + 0.5 * sin(t * 2.1 + Noise(vWorld.xz * 0.6) * 6.2831);
    color += tex.rgb * (0.08 + 0.1 * pulse);
    color += vec3(1.0, 0.55, 0.12) * smoothstep(0.62, 0.95, heat) * 0.45;
    FragColor = Fogged(vec4(min(color, vec3(1.0)), tex.a));
}
)";
    }

    namespace
    {
        // Ultra (UltraFinish.hpp): in linear light, written unclipped. The water reflects the sky (the
        // world's background, by Schlick's Fresnel; what stands round it, UltraFinish's screen-space
        // reflections) and the sun glints off it far above white; the lava glows above white.
        const char* const kSurfaceUltra = R"(
uniform sampler2D uEnv;     // unit 3: the sky picture, its levels ever softer
uniform vec4 uEnvInfo;      // repeats, the levels' last
vec3 ToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
vec3 ToDisplay(vec3 c) { return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2)); }
vec3 Environment(vec3 d, float roughness)
{
    float wide = 6.2831853 / max(1.0, uEnvInfo.x);
    vec2 uv = vec2(atan(d.x, -d.z) / wide, 0.5 - asin(clamp(d.y, -1.0, 1.0)) / (wide * 0.75));
    return ToLinear(textureLod(uEnv, uv, roughness * uEnvInfo.y).rgb);
}
)";
        const char* const kWaterUltraFragment = R"(
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
    float top = clamp(n.y, 0.0, 1.0);
    vec2 q = vWorld.xz;
    float t = uTime;
    vec2 slope = vec2(cos(q.x * 3.1 + t * 1.7) * 0.6 + cos((q.x + q.y) * 4.3 - t * 2.3) * 0.4,
                      cos(q.y * 2.7 - t * 1.3) * 0.6 + cos((q.y - q.x) * 3.7 + t * 1.9) * 0.4);
    vec3 waved = normalize(n + vec3(slope.x, 0.0, slope.y) * 0.12 * top);
    vec3 body = ToLinear(Lit(tex.rgb, waved));
    float dance = sin(q.x * 5.3 + t * 2.1) * sin(q.y * 4.7 - t * 1.7) + sin((q.x - q.y) * 3.1 + t * 1.3) * 0.5;
    body *= 1.0 + 0.12 * dance * top;
    vec3 toEye = normalize(uCamera - vWorld);
    float nv = max(dot(waved, toEye), 0.0);
    float fresnel = 0.02 + 0.98 * pow(1.0 - nv, 5.0);
    vec3 sky = Environment(reflect(-toEye, waved), 0.08);
    vec3 color = mix(body, sky, fresnel * top);
    // The sun's glint: GGX, roughness 0.12, its light 0.75 (UltraLogic.hpp kUltraSun).
    vec3 h = normalize(-uLightDir + toEye);
    float nh = max(dot(waved, h), 0.0);
    float a2 = 0.12 * 0.12 * 0.12 * 0.12;
    float d = nh * nh * (a2 - 1.0) + 1.0;
    color += vec3(1.0, 0.97, 0.9) * (a2 / (3.14159 * d * d)) * 0.25 * fresnel * max(0.0, dot(waved, -uLightDir)) * top;
    float alpha = tex.a * mix(0.78, 1.0, fresnel);
    FragColor = Fogged(vec4(ToDisplay(color), alpha));
}
)";
        const char* const kLavaUltraFragment = R"(
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
    vec3 color = ToLinear(Lit(tex.rgb, n));
    float t = uTime;
    vec2 q = vWorld.xz * 1.7 + vec2(t * 0.11, -t * 0.07);
    float heat = Noise(q) * 0.6 + Noise(q * 2.3 - vec2(t * 0.17, t * 0.13)) * 0.4;
    float pulse = 0.5 + 0.5 * sin(t * 2.1 + Noise(vWorld.xz * 0.6) * 6.2831);
    vec3 glow = ToLinear(tex.rgb) * (0.6 + 0.5 * pulse);
    glow += vec3(1.0, 0.42, 0.06) * smoothstep(0.55, 0.95, heat) * 2.6;
    FragColor = Fogged(vec4(ToDisplay(color + glow), tex.a));
}
)";
    }

    std::string GlslPreamble(const Graphics::GraphicsDevice& device)
    {
        switch (device.GetShaderDialectEXT())
        {
            case CNA::ShaderDialectEXT::GlslDesktop: return "#version 330 core\n";
            case CNA::ShaderDialectEXT::GlslEs: return "#version 300 es\nprecision highp float;\n";
            default: return {};
        }
    }

    std::unique_ptr<Graphics::ShaderEffect> CompileShaderEffect(Graphics::GraphicsDevice& device, const char* name,
                                                                const std::string& vertexBody,
                                                                const std::string& fragmentBody)
    {
        const std::string preamble = GlslPreamble(device);
        if (preamble.empty())
        {
            std::cout << "GalaxyEggbertCNA: 3D effects: no GLSL on this renderer, " << name << " left out." << std::endl;
            return nullptr;
        }
        try
        {
            auto effect = std::make_unique<Graphics::ShaderEffect>(device, preamble + vertexBody, preamble + fragmentBody);
            if (!effect->IsEffectValid())
            {
                std::cout << "GalaxyEggbertCNA: 3D effects: " << name << " did not compile: " << effect->GetCompileErrorEXT()
                          << std::endl;
                return nullptr;
            }
            return effect;
        }
        catch (const std::exception& error)
        {
            std::cout << "GalaxyEggbertCNA: 3D effects: " << name << " failed: " << error.what() << std::endl;
            return nullptr;
        }
    }

    bool SurfaceShaders::Prepare(Graphics::GraphicsDevice& device)
    {
        if (!m_tried)
        {
            m_tried = true;
            const std::string common = kSurfaceCommon;
            m_water = CompileShaderEffect(device, "the water shader", kSurfaceVertex, common + kWaterFragment);
            m_lava = CompileShaderEffect(device, "the lava shader", kSurfaceVertex, common + kLavaFragment);
        }
        return m_water || m_lava;
    }

    void SurfaceShaders::SetCommon(Graphics::ShaderEffect& effect, const Graphics::BasicEffect& terrain,
                                   const SceneFrame& frame)
    {
        effect.setWorldProperty(terrain.World);
        effect.setViewProperty(terrain.View);
        effect.setProjectionProperty(terrain.Projection);
        if (auto* texture = terrain.getTextureProperty())
        {
            effect.SetTexture(0, *texture);
        }
        const auto& light = kTerrainLight;
        const float length = std::sqrt(light.directionX * light.directionX + light.directionY * light.directionY +
                                       light.directionZ * light.directionZ);
        effect.SetUniformVec3("uLightDir", light.directionX / length, light.directionY / length, light.directionZ / length);
        effect.SetUniformVec2("uLight", light.ambient, light.diffuse);
        const auto& camera = frame.camera.position;
        effect.SetUniformVec3("uCamera", camera.X, camera.Y, camera.Z);
        const auto& effects = frame.effects;
        effect.SetUniformVec3("uFogColor", effects.fogColor.X, effects.fogColor.Y, effects.fogColor.Z);
        effect.SetUniformVec3("uFog", effects.fogStart, effects.fogEnd, effects.enabled ? 1.0f : 0.0f);
        effect.SetUniformVec3("uSky", effects.fogColor.X, effects.fogColor.Y, effects.fogColor.Z);
        effect.SetUniformFloat("uTime", effects.seconds);
    }

    void SurfaceShaders::Update(Graphics::GraphicsDevice& device, const Graphics::BasicEffect& terrain, const SceneFrame& frame,
                                const UltraSceneAssets* ultra)
    {
        m_ultraActive = false;
        if (ultra != nullptr && frame.effects.ultra && ultra->environment != nullptr)
        {
            if (!m_ultraTried)
            {
                m_ultraTried = true;
                const std::string common = std::string(kSurfaceCommon) + kSurfaceUltra;
                m_ultraWater = CompileShaderEffect(device, "Ultra's water shader", kSurfaceVertex, common + kWaterUltraFragment);
                m_ultraLava = CompileShaderEffect(device, "Ultra's lava shader", kSurfaceVertex, common + kLavaUltraFragment);
            }
            m_ultraActive = m_ultraWater && m_ultraLava;
            if (m_ultraActive)
            {
                constexpr float kTwoPi = 6.28318530718f;
                const float horizontalFov =
                    2.0f * std::atan(std::tan(frame.camera.fieldOfView * 0.5f) * frame.camera.aspectRatio);
                const float repeats = std::max(1.0f, std::round(kTwoPi / std::max(horizontalFov, 0.01f)));
                for (auto* effect : {m_ultraWater.get(), m_ultraLava.get()})
                {
                    SetCommon(*effect, terrain, frame);
                    effect->SetUniformInt("uEnv", 3);
                    effect->SetTexture(3, *ultra->environment);
                    effect->SetUniformVec4("uEnvInfo", repeats, static_cast<float>(std::max(0, ultra->environmentLevels - 1)),
                                           0.0f, 0.0f);
                }
                return;
            }
        }
        if (m_water)
        {
            SetCommon(*m_water, terrain, frame);
        }
        if (m_lava)
        {
            SetCommon(*m_lava, terrain, frame);
        }
    }
}
