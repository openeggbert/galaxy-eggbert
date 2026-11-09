#include <GalaxyEggbert/Game/SceneShaders.hpp>

#include <GalaxyEggbert/Game/SurfaceShaders.hpp>
#include <GalaxyEggbert/Game/TerrainLighting.hpp>

#include <CNA/ShaderDialectEXT.hpp>

#include <Microsoft/Xna/Framework/Color.hpp>
#include <Microsoft/Xna/Framework/Graphics/ModelMeshPart.hpp>
#include <Microsoft/Xna/Framework/Graphics/PrimitiveType.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace GalaxyEggbert::Game
{
    namespace
    {
        namespace Graphics = Microsoft::Xna::Framework::Graphics;
        using Microsoft::Xna::Framework::Matrix;
        using Microsoft::Xna::Framework::Vector3;

        // ---- The shadow map's depth: kept in an RGBA8 target (sampleable everywhere, WebGL 2 too). ----
        const char* const kPack = R"(
vec4 PackDepth(float depth)
{
    vec4 e = fract(clamp(depth, 0.0, 0.999999) * vec4(1.0, 255.0, 65025.0, 16581375.0));
    return e - e.yzww * vec4(1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0, 0.0);
}
)";

        const char* const kDepthSolidVertex = R"(
layout(location = 0) in vec3 aPosition;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out float vDepth;
void main()
{
    gl_Position = Projection * View * World * vec4(aPosition, 1.0);
    vDepth = gl_Position.z / gl_Position.w;
}
)";
        const char* const kDepthSolidFragment = R"(
in float vDepth;
out vec4 FragColor;
void main()
{
    FragColor = PackDepth(vDepth);
}
)";
        // The cutouts (position, baked colour, uv): their clear texels throw no shadow.
        const char* const kDepthCutoutVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out float vDepth;
out vec2 vUv;
void main()
{
    gl_Position = Projection * View * World * vec4(aPosition, 1.0);
    vDepth = gl_Position.z / gl_Position.w;
    vUv = aTexCoord;
}
)";
        const char* const kDepthCutoutFragment = R"(
uniform sampler2D uTexture;
in float vDepth;
in vec2 vUv;
out vec4 FragColor;
void main()
{
    if (texture(uTexture, vUv).a < 0.5) discard;
    FragColor = PackDepth(vDepth);
}
)";

        // ---- Skinning, as CNA's stock SkinnedEffect: four weights, bone matrices uploaded M11..M44. ----
        const char* const kSkin = R"(
uniform mat4 uBones[56];
mat4 Skin(vec4 indices, vec4 weights)
{
    return uBones[int(indices.x)] * weights.x + uBones[int(indices.y)] * weights.y +
           uBones[int(indices.z)] * weights.z + uBones[int(indices.w)] * weights.w;
}
)";
        const char* const kDepthSkinnedVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec4 aIndices;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out float vDepth;
void main()
{
    vec4 local = Skin(aIndices, aWeights) * vec4(aPosition, 1.0);
    gl_Position = Projection * View * World * local;
    vDepth = gl_Position.z / gl_Position.w;
}
)";

        // ---- The main pass: the shadow lookup, the point lights and BasicEffect's fog. ----
        const char* const kLighting = R"(
uniform sampler2D uTexture;
uniform sampler2D uShadowMap;    // the near cascade (44 blocks round the target)
uniform sampler2D uShadowMapFar; // the far one (160 blocks, ahead of the camera)
uniform vec4 uShadow;          // on, the near map's texel, bias, how dark (0..1) a shadow makes the sun's part
uniform vec2 uShadowFar;       // the far map's texel, bias
uniform vec3 uFogColor;
uniform vec3 uFog;             // start, end, on
uniform vec3 uPointPos[8];
uniform vec3 uPointColor[8];
uniform float uPointRadius[8];
uniform int uPointCount;
in vec3 vWorld;
in float vDepth;
in vec4 vLightPos;
in vec4 vLightPosFar;
out vec4 FragColor;
float UnpackDepth(vec4 c)
{
    return dot(c, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 16581375.0));
}
// One map's light at p (its clip space): a 3x3 tent of comparisons, bilinearly weighted (16 samples),
// so a shadow's edge is soft, not stepped.
float Pcf(sampler2D map, vec3 p, float texel, float bias)
{
    vec2 coord = (p.xy * 0.5 + 0.5) / texel - 0.5;
    vec2 base = floor(coord);
    vec2 f = coord - base;
    float lit = 0.0;
    for (int y = 0; y < 4; ++y)
    {
        float wy = y == 0 ? 1.0 - f.y : (y == 3 ? f.y : 1.0);
        for (int x = 0; x < 4; ++x)
        {
            float wx = x == 0 ? 1.0 - f.x : (x == 3 ? f.x : 1.0);
            float stored = UnpackDepth(texture(map, (base + vec2(float(x) - 1.0, float(y) - 1.0) + 0.5) * texel));
            lit += (p.z - bias <= stored ? 1.0 : 0.0) * wx * wy;
        }
    }
    return lit / 9.0;
}
// How far inside a map's square p is (0 at its edge, 1 a tenth in).
float Inside(vec3 p)
{
    vec2 uv = p.xy * 0.5 + 0.5;
    if (uv.x <= 0.0 || uv.y <= 0.0 || uv.x >= 1.0 || uv.y >= 1.0 || p.z >= 1.0) return 0.0;
    vec2 edge = min(uv, 1.0 - uv);
    return clamp(min(edge.x, edge.y) * 10.0, 0.0, 1.0);
}
// 1 in the sun, down to 0 in shadow; the near map where it reaches, blended into the far one at its
// edge, 1 beyond both.
float SunLight()
{
    if (uShadow.x < 0.5) return 1.0;
    vec3 near = vLightPos.xyz / vLightPos.w;
    vec3 far = vLightPosFar.xyz / vLightPosFar.w;
    float inNear = Inside(near);
    float inFar = Inside(far);
    float farLight = inFar > 0.0 ? mix(1.0, Pcf(uShadowMapFar, far, uShadowFar.x, uShadowFar.y), inFar) : 1.0;
    if (inNear <= 0.0) return farLight;
    return mix(farLight, Pcf(uShadowMap, near, uShadow.y, uShadow.z), inNear);
}
// The point lights on a surface turned `n` (or facing them all when hasNormal is false).
vec3 PointLights(vec3 n, bool hasNormal)
{
    vec3 sum = vec3(0.0);
    for (int i = 0; i < 8; ++i)
    {
        if (i >= uPointCount) break;
        vec3 toLight = uPointPos[i] - vWorld;
        float distance = length(toLight);
        float reach = clamp(1.0 - distance / uPointRadius[i], 0.0, 1.0);
        float facing = hasNormal ? max(0.0, dot(n, toLight / max(distance, 0.001))) : 0.6;
        sum += uPointColor[i] * reach * reach * facing;
    }
    return min(sum, vec3(0.9));
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
)";

        // The terrain, lit as BasicEffect with the terrain light; the sun's part shadowed, the corners
        // occluded (vertex colour), the point lights added.
        std::string TerrainVertex(bool occluded)
        {
            std::string source = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
)";
            source += occluded ? "layout(location = 2) in vec4 aColor;\nlayout(location = 3) in vec2 aTexCoord;\n"
                               : "layout(location = 2) in vec2 aTexCoord;\n";
            source += R"(
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 uLightViewProj;
uniform mat4 uLightViewProjFar;
out vec2 vUv;
out vec3 vNormal;
out vec3 vWorld;
out float vDepth;
out vec4 vLightPos;
out vec4 vLightPosFar;
out float vOcclusion;
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
    vLightPos = uLightViewProj * vec4(world.xyz + normalize(vNormal) * 0.04, 1.0);
    vLightPosFar = uLightViewProjFar * vec4(world.xyz + normalize(vNormal) * 0.12, 1.0);
)";
            source += occluded ? "    vOcclusion = aColor.r;\n}\n" : "    vOcclusion = 1.0;\n}\n";
            return source;
        }
        const char* const kTerrainFragment = R"(
uniform vec3 uLightDir;
uniform vec2 uLight;           // ambient, diffuse
uniform float uDebug;          // GALAXY_EFFECTS_DEBUG: 1 the sun's shadow, 2 the occlusion
in vec2 vUv;
in vec3 vNormal;
in float vOcclusion;
void main()
{
    if (uDebug > 1.5) { FragColor = vec4(vec3(vOcclusion), 1.0); return; }
    if (uDebug > 0.5) { FragColor = vec4(vec3(SunLight()), 1.0); return; }
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
    float sun = max(0.0, dot(n, -uLightDir)) * mix(1.0, SunLight(), uShadow.w);
    vec3 light = (uLight.x + uLight.y * sun) * vOcclusion + PointLights(n, true);
    FragColor = Fogged(vec4(tex.rgb * light, tex.a));
}
)";

        // The cutouts: their light baked in the vertex colour (TerrainFaceBrightness, times the
        // occlusion); the sun's share of it darkened in shadow; their clear texels cut away.
        const char* const kCutoutVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 uLightViewProj;
uniform mat4 uLightViewProjFar;
out vec2 vUv;
out vec3 vWorld;
out float vDepth;
out vec4 vLightPos;
out vec4 vLightPosFar;
out vec4 vColor;
void main()
{
    vec4 world = World * vec4(aPosition, 1.0);
    vec4 view = View * world;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
    vUv = aTexCoord;
    vWorld = world.xyz;
    vDepth = -view.z;
    vLightPos = uLightViewProj * world;
    vLightPosFar = uLightViewProjFar * world;
    vColor = aColor;
}
)";
        const char* const kCutoutFragment = R"(
uniform vec2 uLight;
in vec2 vUv;
in vec4 vColor;
void main()
{
    vec4 tex = texture(uTexture, vUv);
    if (tex.a < 0.5) discard;
    float baked = vColor.r;
    float shade = mix(1.0, SunLight(), uShadow.w);
    float light = min(baked, uLight.x) + max(0.0, baked - uLight.x) * shade;
    FragColor = Fogged(vec4(tex.rgb * (light + PointLights(vec3(0.0, 1.0, 0.0), false)), 1.0));
}
)";

        // The models: SkinnedEffect's own lighting (ambient, a key and a fill light, the key's
        // highlight), the key light darkened in the sun's shadow, the point lights added.
        const char* const kModelVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec4 aIndices;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 uLightViewProj;
uniform mat4 uLightViewProjFar;
out vec2 vUv;
out vec3 vNormal;
out vec3 vWorld;
out float vDepth;
out vec4 vLightPos;
out vec4 vLightPosFar;
out vec3 vLocal;
void main()
{
    vLocal = aPosition;
    mat4 skin = Skin(aIndices, aWeights);
    vec4 world = World * (skin * vec4(aPosition, 1.0));
    vec4 view = View * world;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
    vUv = aTexCoord;
    vNormal = mat3(World) * (mat3(skin) * aNormal);
    vWorld = world.xyz;
    vDepth = -view.z;
    vLightPos = uLightViewProj * vec4(world.xyz + normalize(vNormal) * 0.05, 1.0);
    vLightPosFar = uLightViewProjFar * vec4(world.xyz + normalize(vNormal) * 0.12, 1.0);
}
)";
        const char* const kModelFragment = R"(
uniform vec3 uAmbient;
uniform vec3 uKeyDir;
uniform vec3 uKeyColor;
uniform vec3 uKeySpecular;
uniform vec3 uFillDir;
uniform vec3 uFillColor;
uniform vec3 uDiffuse;
uniform vec3 uSpecular;
uniform vec3 uEmissive;
uniform vec2 uMaterial;        // specular power, alpha
uniform vec3 uEye;
// The part's Ray Dream material (ProceduralMaterial.hpp): layers, bump kind, bump amplitude, its threshold;
// the colours; each layer's pattern, scale, spacing, perturbation; the layers' axes and blends.
uniform vec4 uMatInfo;
uniform vec3 uMatColour0;
uniform vec3 uMatColour1;
uniform vec3 uMatColour2;
uniform vec4 uMatLayer0;
uniform vec4 uMatLayer1;
uniform vec4 uMatAxes;
in vec2 vUv;
in vec3 vNormal;
in vec3 vLocal;
float Hash3(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}
float Noise3(vec3 x)
{
    vec3 i = floor(x);
    vec3 f = fract(x);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(Hash3(i), Hash3(i + vec3(1.0, 0.0, 0.0)), f.x),
                   mix(Hash3(i + vec3(0.0, 1.0, 0.0)), Hash3(i + vec3(1.0, 1.0, 0.0)), f.x), f.y),
               mix(mix(Hash3(i + vec3(0.0, 0.0, 1.0)), Hash3(i + vec3(1.0, 0.0, 1.0)), f.x),
                   mix(Hash3(i + vec3(0.0, 1.0, 1.0)), Hash3(i + vec3(1.0, 1.0, 1.0)), f.x), f.y), f.z);
}
float Fbm(vec3 p)
{
    return (Noise3(p) * 0.5 + Noise3(p * 2.03) * 0.25 + Noise3(p * 4.01) * 0.125 + Noise3(p * 7.97) * 0.0625) / 0.9375;
}
// How much of a layer's colour shows (0..1): marble veins along its axis, wood rings round it, spots;
// bent by the noise, `blend` softening the edges. `rd` in Ray Dream units.
float Pattern(vec4 layer, float axis, float blend, vec3 rd)
{
    float noise = Fbm(rd / max(layer.y, 0.05));
    float along = axis < 0.5 ? rd.x : (axis < 1.5 ? rd.y : rd.z);
    float t;
    if (layer.x < 1.5)
    {
        t = 0.5 + 0.5 * sin(along * 6.2831853 / max(layer.z, 0.05) + layer.w * (noise - 0.5) * 6.2831853);
    }
    else if (layer.x < 2.5)
    {
        vec2 across = axis < 0.5 ? rd.yz : (axis < 1.5 ? rd.xz : rd.xy);
        t = fract(length(across) / max(layer.z, 0.05) + layer.w * (noise - 0.5));
        t = 1.0 - abs(t * 2.0 - 1.0);
    }
    else
    {
        t = noise;
    }
    float soft = max(0.03, 0.5 * blend);
    return smoothstep(0.5 - soft, 0.5 + soft, t);
}
// A surface's normal bent by a height field h over it (screen-space derivatives).
vec3 Bumped(vec3 n, float h)
{
    vec3 dpdx = dFdx(vWorld);
    vec3 dpdy = dFdy(vWorld);
    float dhdx = dFdx(h);
    float dhdy = dFdy(h);
    vec3 r1 = cross(dpdy, n);
    vec3 r2 = cross(n, dpdx);
    float det = dot(dpdx, r1);
    vec3 grad = sign(det) * (dhdx * r1 + dhdy * r2);
    return normalize(abs(det) * n - grad);
}
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
    // A block is 21.116 Ray Dream units (the object models are in blocks).
    vec3 rd = vLocal * 21.116;
    if (uMatInfo.x > 0.5)
    {
        vec3 c = mix(uMatColour0, uMatColour1, Pattern(uMatLayer0, uMatAxes.x, uMatAxes.y, rd));
        if (uMatInfo.x > 1.5) c = mix(c, uMatColour2, Pattern(uMatLayer1, uMatAxes.z, uMatAxes.w, rd));
        tex.rgb = c;
    }
    if (uMatInfo.y > 0.5)
    {
        float h = uMatInfo.y < 1.5 ? smoothstep(uMatInfo.w - 0.06, uMatInfo.w + 0.06, Fbm(rd / 3.0))
                                   : Fbm(rd * 0.8);
        n = Bumped(n, h * uMatInfo.z * (uMatInfo.y < 1.5 ? 0.03 : 0.012));
    }
    float sun = mix(1.0, mix(0.4, 1.0, SunLight()), uShadow.w);
    float key = max(0.0, dot(n, -uKeyDir));
    vec3 diffuse = uAmbient + uKeyColor * key * sun + uFillColor * max(0.0, dot(n, -uFillDir));
    vec3 toEye = normalize(uEye - vWorld);
    float highlight = key > 0.0 ? pow(max(0.0, dot(n, normalize(-uKeyDir + toEye))), uMaterial.x) : 0.0;
    vec3 color = tex.rgb * (diffuse * uDiffuse + uEmissive + PointLights(n, true)) +
                 uKeySpecular * highlight * sun * uSpecular;
    FragColor = Fogged(vec4(color, tex.a * uMaterial.y));
}
)";

        // The billboards (position, uv; the sprites' sheets), unlit as BasicEffect draws them, darkened
        // where the sun's shadow falls on them: looked up a little toward the sun from the quad, past
        // the shadow the sprite throws itself.
        const char* const kSpriteVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 uLightViewProj;
uniform mat4 uLightViewProjFar;
uniform vec3 uLightDir;
out vec2 vUv;
out vec3 vWorld;
out float vDepth;
out vec4 vLightPos;
out vec4 vLightPosFar;
void main()
{
    vec4 world = World * vec4(aPosition, 1.0);
    vec4 view = View * world;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
    vUv = aTexCoord;
    vWorld = world.xyz;
    vDepth = -view.z;
    vec4 probe = vec4(world.xyz - uLightDir * 0.7, 1.0);
    vLightPos = uLightViewProj * probe;
    vLightPosFar = uLightViewProjFar * probe;
}
)";
        const char* const kSpriteFragment = R"(
uniform float uAlpha;
in vec2 vUv;
void main()
{
    vec4 tex = texture(uTexture, vUv);
    float shade = mix(1.0, mix(0.62, 1.0, SunLight()), uShadow.w);
    FragColor = Fogged(vec4(tex.rgb * shade, tex.a * uAlpha));
}
)";

        Vector3 Normalized(const Vector3& v)
        {
            const float length = std::sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
            return length > 0.0f ? Vector3(v.X / length, v.Y / length, v.Z / length) : Vector3(0.0f, -1.0f, 0.0f);
        }

        void SetMatrix(Graphics::ShaderEffect& effect, const char* name, const Matrix& m)
        {
            const float values[16] = {m.M11, m.M12, m.M13, m.M14, m.M21, m.M22, m.M23, m.M24,
                                      m.M31, m.M32, m.M33, m.M34, m.M41, m.M42, m.M43, m.M44};
            effect.SetUniformMat4(name, values);
        }
    }

    bool SceneShaders::Prepare(Graphics::GraphicsDevice& device)
    {
        if (m_tried)
        {
            return m_ready;
        }
        m_tried = true;
        const std::string pack = kPack;
        const std::string skin = kSkin;
        const std::string lighting = kLighting;
        m_depthSolid = CompileShaderEffect(device, "the shadow map (solid)", kDepthSolidVertex, pack + kDepthSolidFragment);
        m_depthCutout = CompileShaderEffect(device, "the shadow map (cutouts)", kDepthCutoutVertex, pack + kDepthCutoutFragment);
        m_depthSkinned = CompileShaderEffect(device, "the shadow map (models)", skin + kDepthSkinnedVertex,
                                             pack + kDepthSolidFragment);
        m_occluded = CompileShaderEffect(device, "the terrain shader", TerrainVertex(true), lighting + kTerrainFragment);
        m_lit = CompileShaderEffect(device, "the relief shader", TerrainVertex(false), lighting + kTerrainFragment);
        m_cutout = CompileShaderEffect(device, "the cutout shader", kCutoutVertex, lighting + kCutoutFragment);
        m_model = CompileShaderEffect(device, "the model shader", skin + kModelVertex, lighting + kModelFragment);
        m_sprite = CompileShaderEffect(device, "the sprite shader", kSpriteVertex, lighting + kSpriteFragment);
        m_ready = m_depthSolid && m_depthCutout && m_depthSkinned && m_occluded && m_lit && m_cutout && m_model && m_sprite;
        if (!m_ready)
        {
            return false;
        }
        for (auto* effect : {m_occluded.get(), m_lit.get(), m_cutout.get(), m_model.get(), m_sprite.get()})
        {
            effect->SetUniformInt("uTexture", 0);
            effect->SetUniformInt("uShadowMap", 1);
            effect->SetUniformInt("uShadowMapFar", 2);
        }
        m_shadowSize = kShadowSize;
        // GALAXY_EFFECTS_DEBUG=shadow or =occlusion draws the terrain as that factor alone (grey).
        if (const char* debug = std::getenv("GALAXY_EFFECTS_DEBUG"))
        {
            m_debug = std::string(debug) == "shadow" ? 1.0f : std::string(debug) == "occlusion" ? 2.0f : 0.0f;
        }
        try
        {
            for (auto& map : m_shadowMaps)
            {
                map = std::make_unique<Graphics::RenderTarget2D>(device, m_shadowSize, m_shadowSize, false,
                                                                 Graphics::SurfaceFormat::Color, Graphics::DepthFormat::Depth24);
            }
        }
        catch (const std::exception& error)
        {
            std::cout << "GalaxyEggbertCNA: 3D effects: no shadow map (" << error.what() << ")." << std::endl;
            for (auto& map : m_shadowMaps)
            {
                map.reset();
            }
        }
        return m_ready;
    }

    void SceneShaders::BeginShadowPass(Graphics::GraphicsDevice& device, const SceneFrame& frame, int cascade)
    {
        if (cascade == 0)
        {
            m_shadowValid = false;
        }
        auto& map = m_shadowMaps[static_cast<std::size_t>(cascade)];
        if (!m_ready || !map)
        {
            m_casting = false;
            return;
        }
        // The sun (TerrainLighting.hpp's light) looking from far up its way, orthographic: the near
        // cascade 44 blocks across round the camera's target, the far one 160 blocks across, ahead of
        // the camera; each moved by whole texels only, so the shadows do not crawl as Blupi walks.
        const auto& light = kTerrainLight;
        const Vector3 direction = Normalized(Vector3(light.directionX, light.directionY, light.directionZ));
        Vector3 focus = frame.camera.target;
        float span = kNearSpan;
        if (cascade == 1)
        {
            Vector3 ahead(frame.camera.target.X - frame.camera.position.X, 0.0f,
                          frame.camera.target.Z - frame.camera.position.Z);
            const float length = std::sqrt(ahead.X * ahead.X + ahead.Z * ahead.Z);
            if (length > 0.001f)
            {
                focus = focus + ahead * (kFarSpan * 0.3f / length);
            }
            span = kFarSpan;
        }
        constexpr float kBack = 120.0f;
        Matrix view = Matrix::CreateLookAt(focus - direction * kBack, focus, Vector3(0.0f, 1.0f, 0.0f));
        const float texel = span / static_cast<float>(m_shadowSize);
        const Vector3 centre = Vector3::Transform(focus, view);
        const float snapX = std::floor(centre.X / texel) * texel - centre.X;
        const float snapY = std::floor(centre.Y / texel) * texel - centre.Y;
        view = view * Matrix::CreateTranslation(snapX, snapY, 0.0f);
        const Matrix projection = Matrix::CreateOrthographic(span, span, 1.0f, 260.0f);
        m_lightViewProjection[static_cast<std::size_t>(cascade)] = view * projection;
        m_cascadeSpan[static_cast<std::size_t>(cascade)] = span;

        if (cascade == 0)
        {
            m_savedViewport = device.getViewportProperty();
        }
        device.SetRenderTarget(map.get());
        device.Clear(Microsoft::Xna::Framework::Color::White); // the farthest depth there is
        for (auto* effect : {m_depthSolid.get(), m_depthCutout.get(), m_depthSkinned.get()})
        {
            effect->setWorldProperty(Matrix::getIdentityProperty());
            effect->setViewProperty(view);
            effect->setProjectionProperty(projection);
        }
        m_casting = true;
        if (cascade == static_cast<int>(kCascades) - 1)
        {
            m_farReady = true;
        }
        m_shadowValid = m_farReady;
    }

    void SceneShaders::EndShadowPass(Graphics::GraphicsDevice& device)
    {
        device.SetRenderTarget(nullptr);
        device.setViewportProperty(m_savedViewport);
        m_casting = false;
    }

    Microsoft::Xna::Framework::Vector3 SceneShaders::SunDirection()
    {
        const auto& light = kTerrainLight;
        return Normalized(Vector3(light.directionX, light.directionY, light.directionZ));
    }

    Graphics::ShaderEffect& SceneShaders::SpriteWith(Graphics::Texture2D* texture, float alpha)
    {
        if (texture != nullptr)
        {
            m_sprite->SetTexture(0, *texture);
        }
        m_sprite->SetUniformFloat("uAlpha", alpha);
        return *m_sprite;
    }

    void SceneShaders::SetBones(Graphics::ShaderEffect& effect, const std::vector<Matrix>& bones)
    {
        static thread_local std::vector<float> data;
        data.resize(bones.size() * 16);
        for (std::size_t i = 0; i < bones.size(); ++i)
        {
            const auto& m = bones[i];
            const float values[16] = {m.M11, m.M12, m.M13, m.M14, m.M21, m.M22, m.M23, m.M24,
                                      m.M31, m.M32, m.M33, m.M34, m.M41, m.M42, m.M43, m.M44};
            std::copy(values, values + 16, data.begin() + static_cast<std::ptrdiff_t>(i * 16));
        }
        effect.SetUniformMat4Array("uBones", data.data(), static_cast<int>(bones.size()));
    }

    Graphics::ShaderEffect* SceneShaders::SkinnedCaster(const std::vector<Matrix>& bones, const Matrix& world)
    {
        if (!m_casting || bones.empty() || bones.size() > static_cast<std::size_t>(kMaxBones))
        {
            return nullptr;
        }
        m_depthSkinned->setWorldProperty(world);
        SetBones(*m_depthSkinned, bones);
        return m_depthSkinned.get();
    }

    void SceneShaders::SetCommon(Graphics::ShaderEffect& effect)
    {
        const auto& frame = *m_frame;
        const auto& effects = frame.effects;
        effect.setViewProperty(frame.camera.view);
        effect.setProjectionProperty(frame.camera.projection);
        SetMatrix(effect, "uLightViewProj", m_lightViewProjection[0]);
        SetMatrix(effect, "uLightViewProjFar", m_lightViewProjection[1]);
        const float texel = 1.0f / static_cast<float>(m_shadowSize);
        // A shadow takes 55 % of the sun's light (the terrain); the bias about a sixth of a block of depth
        // (the far cascade's texels are larger: a larger bias).
        effect.SetUniformVec4("uShadow", m_shadowValid ? 1.0f : 0.0f, texel, 0.0012f, 0.55f);
        effect.SetUniformVec2("uShadowFar", texel, 0.0035f);
        if (m_shadowValid)
        {
            effect.SetTexture(1, *m_shadowMaps[0]);
            effect.SetTexture(2, *m_shadowMaps[1]);
        }
        effect.SetUniformVec3("uFogColor", effects.fogColor.X, effects.fogColor.Y, effects.fogColor.Z);
        effect.SetUniformVec3("uFog", effects.fogStart, effects.fogEnd, effects.enabled ? 1.0f : 0.0f);
        float positions[kMaxLights * 3] = {};
        float colors[kMaxLights * 3] = {};
        float radii[kMaxLights] = {};
        const int count = std::min(static_cast<int>(effects.lights.size()), kMaxLights);
        for (int i = 0; i < count; ++i)
        {
            const auto& light = effects.lights[static_cast<std::size_t>(i)];
            positions[i * 3] = light.position.X;
            positions[i * 3 + 1] = light.position.Y;
            positions[i * 3 + 2] = light.position.Z;
            colors[i * 3] = light.color.X;
            colors[i * 3 + 1] = light.color.Y;
            colors[i * 3 + 2] = light.color.Z;
            radii[i] = std::max(0.01f, light.radius);
        }
        effect.SetUniformVec3Array("uPointPos", positions, kMaxLights);
        effect.SetUniformVec3Array("uPointColor", colors, kMaxLights);
        effect.SetUniformFloatArray("uPointRadius", radii, kMaxLights);
        effect.SetUniformInt("uPointCount", count);
    }

    void SceneShaders::SetFrame(Graphics::GraphicsDevice& device, const SceneFrame& frame, const Graphics::BasicEffect& terrain)
    {
        m_frame = &frame;
        if (!m_ready)
        {
            return;
        }
        // The shadow map holds packed depth: read it texel by texel, never blended.
        device.getSamplerStatesProperty()[1] = Graphics::SamplerState::PointClamp;
        device.getSamplerStatesProperty()[2] = Graphics::SamplerState::PointClamp;
        const auto& light = kTerrainLight;
        const Vector3 direction = Normalized(Vector3(light.directionX, light.directionY, light.directionZ));
        for (auto* effect : {m_occluded.get(), m_lit.get(), m_cutout.get()})
        {
            SetCommon(*effect);
            effect->setWorldProperty(terrain.World);
            if (auto* texture = terrain.getTextureProperty())
            {
                effect->SetTexture(0, *texture);
            }
            effect->SetUniformVec3("uLightDir", direction.X, direction.Y, direction.Z);
            effect->SetUniformVec2("uLight", light.ambient, light.diffuse);
            effect->SetUniformFloat("uDebug", m_debug);
        }
        SetCommon(*m_sprite);
        m_sprite->setWorldProperty(Matrix::getIdentityProperty());
        m_sprite->SetUniformVec3("uLightDir", direction.X, direction.Y, direction.Z);
        SetCommon(*m_model);
        const auto& eye = frame.camera.position;
        m_model->SetUniformVec3("uEye", eye.X, eye.Y, eye.Z);
    }

    void SceneShaders::SetPartMaterial(const ProceduralMaterial* material)
    {
        auto& effect = *m_model;
        if (material == nullptr || (!material->hasColour && material->bump == MaterialBump::None))
        {
            effect.SetUniformVec4("uMatInfo", 0.0f, 0.0f, 0.0f, 0.0f);
            return;
        }
        const float layers = material->hasColour ? static_cast<float>(std::max(1, material->layerCount)) : 0.0f;
        const float hasLayers = material->hasColour && material->layerCount > 0 ? layers : 0.0f;
        effect.SetUniformVec4("uMatInfo", hasLayers, static_cast<float>(material->bump), material->bumpAmplitude,
                              material->bumpThreshold);
        const auto& c0 = material->colour;
        effect.SetUniformVec3("uMatColour0", c0.X, c0.Y, c0.Z);
        float axes[4] = {0.0f, 0.5f, 0.0f, 0.5f};
        const char* const colours[2] = {"uMatColour1", "uMatColour2"};
        const char* const params[2] = {"uMatLayer0", "uMatLayer1"};
        for (int i = 0; i < 2; ++i)
        {
            const auto& layer = material->layers[static_cast<std::size_t>(std::min(i, std::max(0, material->layerCount - 1)))];
            effect.SetUniformVec3(colours[i], layer.colour.X, layer.colour.Y, layer.colour.Z);
            effect.SetUniformVec4(params[i], static_cast<float>(layer.pattern), layer.scale, layer.spacing,
                                  layer.perturbation);
            axes[i * 2] = static_cast<float>(layer.axis);
            axes[i * 2 + 1] = layer.blend;
        }
        effect.SetUniformVec4("uMatAxes", axes[0], axes[1], axes[2], axes[3]);
    }

    Graphics::ShaderEffect& SceneShaders::LitWith(Graphics::Texture2D* texture, const Matrix& world)
    {
        m_lit->setWorldProperty(world);
        if (texture != nullptr)
        {
            m_lit->SetTexture(0, *texture);
        }
        return *m_lit;
    }

    bool SceneShaders::PrepareModel(Graphics::SkinnedEffect& stock, const std::vector<Matrix>& bones, const Matrix& world,
                                    float alpha, bool tinted, const Vector3& tint)
    {
        if (!m_ready || bones.empty() || bones.size() > static_cast<std::size_t>(kMaxBones))
        {
            return false;
        }
        auto& effect = *m_model;
        effect.setWorldProperty(world);
        SetBones(effect, bones);
        const auto ambient = stock.getAmbientLightColorProperty();
        effect.SetUniformVec3("uAmbient", ambient.X, ambient.Y, ambient.Z);
        const auto setLight = [&](Graphics::DirectionalLight& light, const char* direction, const char* color,
                                  const char* specular) {
            const bool on = light.getEnabledProperty();
            const Vector3 d = Normalized(light.getDirectionProperty());
            const Vector3 c = on ? light.getDiffuseColorProperty() : Vector3::Zero;
            effect.SetUniformVec3(direction, d.X, d.Y, d.Z);
            effect.SetUniformVec3(color, c.X, c.Y, c.Z);
            if (specular != nullptr)
            {
                const Vector3 s = on ? light.getSpecularColorProperty() : Vector3::Zero;
                effect.SetUniformVec3(specular, s.X, s.Y, s.Z);
            }
        };
        setLight(stock.getDirectionalLight0Property(), "uKeyDir", "uKeyColor", "uKeySpecular");
        setLight(stock.getDirectionalLight1Property(), "uFillDir", "uFillColor", nullptr);
        const Vector3 diffuse = tinted ? tint : stock.getDiffuseColorProperty();
        const Vector3 specular = tinted ? Vector3::Zero : stock.getSpecularColorProperty();
        const Vector3 emissive = stock.getEmissiveColorProperty();
        effect.SetUniformVec3("uDiffuse", diffuse.X, diffuse.Y, diffuse.Z);
        effect.SetUniformVec3("uSpecular", specular.X, specular.Y, specular.Z);
        effect.SetUniformVec3("uEmissive", emissive.X, emissive.Y, emissive.Z);
        effect.SetUniformVec2("uMaterial", std::max(1.0f, stock.getSpecularPowerProperty()), alpha);
        return true;
    }
}
