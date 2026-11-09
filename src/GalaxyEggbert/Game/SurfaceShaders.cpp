#include <GalaxyEggbert/Game/SurfaceShaders.hpp>

#include <GalaxyEggbert/Game/TerrainLighting.hpp>
#include <GalaxyEggbert/Game/WaterWaves.hpp>

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

        // The water's body (both levels): its colour and clearness (uWater1/2: its tiles' TerrainGi.hpp
        // TileTint, linear), not the tile itself (Mobile Eggbert's sprite, whose wavy lines and edges
        // repeated on every cell's face, the owner 2026-11-09); the tile where they are not known.
        const char* const kWaterBody = R"(
uniform vec4 uWater1;   // Water1's body: linear colour, clearness (w 0: the tile as it is)
uniform vec4 uWater2;   // Water2's
vec4 WaterBodyLinear()
{
    vec2 cell = floor(vUv * vec2(20.0, 22.0)); // the padded atlas (TileAtlas.hpp): 20 x 22 tiles
    float icon = cell.y * 20.0 + cell.x;
    vec4 body = icon >= 91.5 && icon <= 95.5 ? uWater1 : uWater2;
    if (body.w <= 0.0)
    {
        vec4 tex = texture(uTexture, vUv);
        return vec4(pow(max(tex.rgb, vec3(0.0)), vec3(2.2)), tex.a);
    }
    return body;
}
)";

        // Water: its body lit as the terrain, its top WaterWaves.hpp's waves (they run on across the
        // cells), light playing over it, the sun glinting off it, a pale sheen at a glancing view, where
        // it also hides what lies in it. Not the sky's colour at a glancing view: the haze is the
        // background's middle, often green or white, and the water seen from above turned that colour,
        // changing as the camera moved (reported). Seen from inside, darker.
        const char* const kWaterFragment = R"(
void main()
{
    vec4 water = WaterBodyLinear();
    vec3 n = normalize(vNormal);
    float top = clamp(n.y, 0.0, 1.0);
    vec3 waved = top > 0.5 ? WaveNormal(vWorld.xz, uTime, vDepth) : n;
    vec3 toEye = normalize(uCamera - vWorld);
    bool inside = dot(n, uCamera - vWorld) < 0.0;
    float nv = max(dot(inside ? -waved : waved, toEye), 0.0);
    float fresnel = 0.02 + 0.98 * pow(1.0 - nv, 5.0);
    vec3 color = Lit(pow(water.rgb, vec3(1.0 / 2.2)), waved);
    vec2 q = vWorld.xz;
    float t = uTime;
    float dance = sin(q.x * 5.3 + t * 2.1) * sin(q.y * 4.7 - t * 1.7) + sin((q.x - q.y) * 3.1 + t * 1.3) * 0.5;
    color *= 1.0 + 0.06 * dance * top;
    float alpha;
    if (inside)
    {
        color *= 0.75;
        alpha = 0.85;
    }
    else
    {
        color = mix(color, vec3(0.8, 0.9, 1.0) * (uLight.x + uLight.y), fresnel * 0.3 * top);
        vec3 halfway = normalize(-uLightDir + toEye);
        color += vec3(1.0, 0.97, 0.9) * pow(max(0.0, dot(waved, halfway)), 120.0) * 0.45 * top;
        alpha = clamp(mix(water.a * 1.2, 1.0, fresnel * 0.85), 0.0, 1.0);
    }
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
        // Ultra's water: not the tile (Mobile Eggbert's sprite, whose wavy lines and edges repeated on every
        // cell's face, the owner 2026-11-09) but its body: the water's colour and clearness (uWater1/2, the
        // tiles' TileTint, kWaterBody), lit as the terrain; its surface WaterWaves.hpp's height field. The
        // sky reflected by Fresnel (what stands round it: the screen-space reflections), the sun glinting
        // above white; at a glancing view it hides what lies in it. Seen from inside, the surface lets the
        // sky through only within Snell's window and mirrors the water's depth outside it.
        const char* const kWaterUltraFragment = R"(
void main()
{
    vec4 water = WaterBodyLinear();
    vec3 n = normalize(vNormal);
    vec3 toEye = normalize(uCamera - vWorld);
    float top = clamp(n.y, 0.0, 1.0);
    float detail = clamp(1.0 - vDepth / 40.0, 0.0, 1.0);
    vec3 waved = top > 0.5 ? WaveNormal(vWorld.xz, uTime, vDepth) : n;
    bool inside = dot(n, uCamera - vWorld) < 0.0;
    vec3 facing = inside ? -waved : waved;
    float nv = max(dot(facing, toEye), 0.0);
    float fresnel = 0.02 + 0.98 * pow(1.0 - nv, 5.0);
    // The body: its colour, the sun through it, the light from around.
    vec3 body = water.rgb * ToLinear(vec3(uLight.x + uLight.y * max(0.0, dot(waved, -uLightDir))));
    float dance = sin(vWorld.x * 5.3 + uTime * 2.1) * sin(vWorld.z * 4.7 - uTime * 1.7) +
                  sin((vWorld.x - vWorld.z) * 3.1 + uTime * 1.3) * 0.5;
    body *= 1.0 + 0.07 * dance * top * detail;
    vec3 color;
    float alpha;
    if (inside)
    {
        // Snell's window: the sky within about 49 degrees of straight up, the water's depth beyond it.
        float window = smoothstep(0.62, 0.72, nv);
        vec3 sky = Environment(refract(-toEye, facing, 1.33), 0.2);
        color = mix(body * 0.55, sky * 0.9 + body * 0.2, window);
        alpha = mix(0.92, 0.6, window);
    }
    else
    {
        // The sky's reflection, half of Fresnel's: the picture stands far off, not over the water (a
        // room's pool would show it through its ceiling); what does stand round it, the screen-space
        // reflections.
        vec3 sky = Environment(reflect(-toEye, waved), 0.06);
        color = mix(body, sky, fresnel * 0.5 * top);
        // The sun's glint: GGX, roughness 0.1, its light 0.75 (UltraLogic.hpp kUltraSun), above white.
        vec3 h = normalize(-uLightDir + toEye);
        float nh = max(dot(waved, h), 0.0);
        float a2 = 0.1 * 0.1 * 0.1 * 0.1;
        float d = nh * nh * (a2 - 1.0) + 1.0;
        color += vec3(1.0, 0.97, 0.9) * (a2 / (3.14159 * d * d)) * 0.3 * fresnel * max(0.0, dot(waved, -uLightDir)) * top;
        // Clear looked down into, hiding what lies in it at a glancing view.
        alpha = clamp(mix(water.a * 1.2, 1.0, fresnel * 0.85), 0.0, 1.0);
    }
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
            m_water = CompileShaderEffect(device, "the water shader", kSurfaceVertex,
                                          common + kWaterWavesGlsl + kWaterBody + kWaterFragment);
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
        effect.SetUniformVec4("uWater1", effects.water[0].X, effects.water[0].Y, effects.water[0].Z, effects.water[0].W);
        effect.SetUniformVec4("uWater2", effects.water[1].X, effects.water[1].Y, effects.water[1].Z, effects.water[1].W);
    }

    void SurfaceShaders::BindTextures(const Graphics::BasicEffect& terrain, const UltraSceneAssets* ultra)
    {
        auto* texture = terrain.getTextureProperty();
        for (auto* effect : {Water(), Lava()})
        {
            if (effect != nullptr && texture != nullptr)
            {
                effect->SetTexture(0, *texture);
            }
            if (effect != nullptr && m_ultraActive && ultra != nullptr && ultra->environment != nullptr)
            {
                effect->SetTexture(3, *ultra->environment);
            }
        }
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
                m_ultraWater = CompileShaderEffect(device, "Ultra's water shader", kSurfaceVertex,
                                                   common + kWaterWavesGlsl + kWaterBody + kWaterUltraFragment);
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
