#include <GalaxyEggbert/Game/SceneShaders.hpp>
#include <GalaxyEggbert/Game/WaterWaves.hpp>

#include <GalaxyEggbert/Game/SurfaceShaders.hpp>
#include <GalaxyEggbert/Game/TerrainLighting.hpp>
#include <GalaxyEggbert/Game/UltraLogic.hpp>

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
        std::string TerrainVertex(bool occluded, bool ultra = false)
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
            if (ultra)
            {
                // Ultra: the baked colour whole (the light from around, UltraLogic.hpp's bake), as vGi.
                source = "out vec4 vGi;\n" + source;
                source += occluded ? "    vGi = aColor;\n" : "    vGi = vec4(0.5, 0.5, 0.5, 1.0);\n";
            }
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
    if (uDebug > 1.5 && uDebug < 2.5) { FragColor = vec4(vec3(vOcclusion), 1.0); return; }
    if (uDebug > 0.5 && uDebug < 1.5) { FragColor = vec4(vec3(SunLight()), 1.0); return; }
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
)";
        const char* const kModelMain = R"(void main()
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

        // ---- Ultra's geometry pass (UltraFinish.hpp): what each pixel shows, before it is lit -- its normal
        // in view space (from the screen-space derivatives of its position: any vertex layout, the face's
        // own), its distance along the view and how much of a mirror it is (water). ----
        const char* const kGeometryCommon = R"(
uniform float uReflect;
in vec3 vView;
out vec4 FragColor;
vec2 OctEncode(vec3 n)
{
    n /= abs(n.x) + abs(n.y) + abs(n.z);
    vec2 s = vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
    return n.z >= 0.0 ? n.xy : (1.0 - abs(n.yx)) * s;
}
vec4 Geometry()
{
    vec3 n = normalize(cross(dFdx(vView), dFdy(vView)));
    return vec4(OctEncode(n), -vView.z, uReflect);
}
)";
        const char* const kGeometrySolidVertex = R"(
layout(location = 0) in vec3 aPosition;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out vec3 vView;
void main()
{
    vec4 view = View * (World * vec4(aPosition, 1.0));
    vView = view.xyz;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
}
)";
        const char* const kGeometrySolidFragment = R"(
void main()
{
    FragColor = Geometry();
}
)";
        // The water's top in the geometry pass: its waves' normal (WaterWaves.hpp), so the reflections
        // ripple; its sides and the rest as Geometry() makes them.
        const char* const kGeometryWaterVertex = R"(
layout(location = 0) in vec3 aPosition;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out vec3 vView;
out vec3 vWorldPos;
void main()
{
    vec4 world = World * vec4(aPosition, 1.0);
    vec4 view = View * world;
    vView = view.xyz;
    vWorldPos = world.xyz;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
}
)";
        const char* const kGeometryWaterFragment = R"(
uniform mat4 View;
uniform float uTime;
in vec3 vWorldPos;
void main()
{
    vec4 g = Geometry();
    vec3 n = normalize(cross(dFdx(vView), dFdy(vView)));
    vec3 up = mat3(View) * vec3(0.0, 1.0, 0.0);
    float along = dot(n, up);
    if (abs(along) > 0.7)
    {
        vec3 waved = mat3(View) * WaveNormal(vWorldPos.xz, uTime, -vView.z);
        g.xy = OctEncode(normalize(along > 0.0 ? waved : -waved));
    }
    FragColor = g;
}
)";
        const char* const kGeometryCutoutVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aTexCoord;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out vec3 vView;
out vec2 vUv;
void main()
{
    vec4 view = View * (World * vec4(aPosition, 1.0));
    vView = view.xyz;
    vUv = aTexCoord;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
}
)";
        const char* const kGeometryCutoutFragment = R"(
uniform sampler2D uTexture;
in vec2 vUv;
void main()
{
    if (texture(uTexture, vUv).a < 0.5) discard;
    FragColor = Geometry();
}
)";
        // The terrain's own: its texture coordinates (at `uvLocation`: 3 after the baked colour, 2 without)
        // for the atlas's surface map, its metal and its smoothness a mirror for the reflections (a
        // polished floor, a metal plate).
        std::string GeometryTerrainVertex(int uvLocation)
        {
            return "layout(location = 0) in vec3 aPosition;\nlayout(location = " + std::to_string(uvLocation) +
                   ") in vec2 aTexCoord;\n" + R"(
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out vec3 vView;
out vec2 vUv;
void main()
{
    vec4 view = View * (World * vec4(aPosition, 1.0));
    vView = view.xyz;
    vUv = aTexCoord;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
}
)";
        }
        const char* const kGeometryTerrainFragment = R"(
uniform sampler2D uSurfaceMap; // unit 4: roughness, metalness, cavity
in vec2 vUv;
void main()
{
    vec4 surface = texture(uSurfaceMap, vUv);
    float mirror = clamp(surface.g * (1.0 - surface.r) * 1.1 + max(0.0, 0.42 - surface.r) * 1.2, 0.0, 0.7);
    vec4 g = Geometry();
    FragColor = vec4(g.rgb, max(uReflect, mirror));
}
)";
        const char* const kGeometrySkinnedVertex = R"(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec4 aWeights;
layout(location = 4) in vec4 aIndices;
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
out vec3 vView;
void main()
{
    vec4 view = View * (World * (Skin(aIndices, aWeights) * vec4(aPosition, 1.0)));
    vView = view.xyz;
    gl_Position = Projection * view;
    gl_Position.z = gl_Position.z * 2.0 - gl_Position.w;
}
)";

        // ---- Ultra (UltraFinish.hpp): physically based light, in linear light, written as display colour
        // unclipped (the HDR target keeps what goes above 1). GGX (Trowbridge-Reitz) highlights with
        // Schlick's Fresnel and Smith's visibility; the sky's light from the world's background picture
        // (its irradiance as spherical harmonics, its reflection from ever softer levels by roughness, Karis'
        // analytic split-sum); the sun and the point lights as the other levels place them.
        const char* const kUltraLighting = R"(
uniform sampler2D uEnv;           // unit 3: the sky picture, its levels ever softer
uniform sampler2D uSurfaceMap;    // unit 4: roughness, metalness, cavity
uniform sampler2D uNormalMap;     // unit 5: the surface's normals (uv space)
uniform sampler2D uScreenOcclusion; // unit 6: r ambient, g contact shadow
uniform vec3 uSh[9];
uniform vec4 uEnvInfo;            // repeats, the levels' last, -, -
uniform vec4 uMaps;               // normal map, surface map, screen occlusion (on 1), -
uniform vec2 uScreenTexel;
uniform vec3 uSunDir;
uniform vec2 uSunLight;           // the sun's light, the sky's (multiplying the irradiance)
uniform vec3 uEyePos;
const float PI = 3.14159265;
vec3 ToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
vec3 ToDisplay(vec3 c) { return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2)); }
vec3 Irradiance(vec3 n)
{
    vec3 e = uSh[0] * 0.282095;
    e += uSh[1] * (0.488603 * n.y) + uSh[2] * (0.488603 * n.z) + uSh[3] * (0.488603 * n.x);
    e += uSh[4] * (1.092548 * n.x * n.y) + uSh[5] * (1.092548 * n.y * n.z) + uSh[6] * (0.315392 * (3.0 * n.z * n.z - 1.0));
    e += uSh[7] * (1.092548 * n.x * n.z) + uSh[8] * (0.546274 * (n.x * n.x - n.y * n.y));
    return max(e, vec3(0.0)) * uSunLight.y;
}
vec3 Environment(vec3 d, float roughness)
{
    float wide = 6.2831853 / max(1.0, uEnvInfo.x);
    vec2 uv = vec2(atan(d.x, -d.z) / wide, 0.5 - asin(clamp(d.y, -1.0, 1.0)) / (wide * 0.75));
    return ToLinear(textureLod(uEnv, uv, roughness * uEnvInfo.y).rgb);
}
vec4 ScreenOcclusion()
{
    if (uMaps.z < 0.5) return vec4(1.0);
    return texelFetch(uScreenOcclusion, ivec2(gl_FragCoord.xy), 0);
}
// The normal map's normal on a surface turned n at uv (the frame from the screen-space derivatives).
vec3 NormalMapped(vec3 n, vec2 uv)
{
    if (uMaps.x < 0.5) return n;
    vec3 m = texture(uNormalMap, uv).xyz * 2.0 - 1.0;
    vec3 dp1 = dFdx(vWorld);
    vec3 dp2 = dFdy(vWorld);
    vec2 duv1 = dFdx(uv);
    vec2 duv2 = dFdy(uv);
    vec3 dp2perp = cross(dp2, n);
    vec3 dp1perp = cross(n, dp1);
    vec3 t = dp2perp * duv1.x + dp1perp * duv2.x;
    vec3 b = dp2perp * duv1.y + dp1perp * duv2.y;
    float scale = max(dot(t, t), dot(b, b));
    if (scale <= 0.0) return n;
    scale = inversesqrt(scale);
    return normalize(t * (m.x * scale) + b * (m.y * scale) + n * m.z);
}
// What one light (from l) gives a surface: Lambert's diffuse and the GGX highlight, times n.l.
vec3 Direct(vec3 diffuse, vec3 f0, vec3 n, vec3 v, vec3 l, float a, float nv)
{
    float nl = max(dot(n, l), 0.0);
    if (nl <= 0.0) return vec3(0.0);
    vec3 h = normalize(l + v);
    float nh = max(dot(n, h), 0.0);
    float vh = max(dot(v, h), 0.0);
    vec3 f = f0 + (1.0 - f0) * pow(1.0 - vh, 5.0);
    float a2 = a * a;
    float d = nh * nh * (a2 - 1.0) + 1.0;
    float D = a2 / (PI * d * d);
    float k = a * 0.5;
    float V = 0.25 / ((nl * (1.0 - k) + k) * (nv * (1.0 - k) + k));
    return (diffuse * (1.0 - f) + D * V * f * PI) * nl;
}
// Karis' analytic fit of the split-sum's scale and bias.
vec3 EnvironmentBrdf(vec3 f0, float roughness, float nv)
{
    const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
    const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
    vec4 r = roughness * c0 + c1;
    float a004 = min(r.x * r.x, exp2(-9.28 * nv)) * r.x + r.y;
    vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
    return f0 * ab.x + ab.y;
}
// A surface's light: albedo (linear), its normal, the way to the eye, roughness, metalness, its
// dielectric reflectance's scale (1: 4 %), the sun's share reaching it (shadow), the ambient's (occlusion)
// and the light from around (gi: 1 the open sky; the bake's colour).
vec3 Shade(vec3 albedo, vec3 n, vec3 v, float roughness, float metalness, float specular, float sun, float ao, vec3 gi)
{
    roughness = clamp(roughness, 0.04, 1.0);
    vec3 f0 = mix(vec3(0.04 * specular), albedo, metalness);
    vec3 diffuse = albedo * (1.0 - metalness);
    float a = roughness * roughness;
    float nv = max(dot(n, v), 0.0001);
    vec3 color = Direct(diffuse, f0, n, v, -uSunDir, a, nv) * (uSunLight.x * sun);
    for (int i = 0; i < 8; ++i)
    {
        if (i >= uPointCount) break;
        vec3 toLight = uPointPos[i] - vWorld;
        float distance = length(toLight);
        float reach = clamp(1.0 - distance / uPointRadius[i], 0.0, 1.0);
        if (reach <= 0.0) continue;
        color += Direct(diffuse, f0, n, v, toLight / max(distance, 0.001), a, nv) * ToLinear(uPointColor[i]) * (reach * reach * 1.4);
    }
    float sky = clamp(dot(gi, vec3(0.3333)), 0.0, 1.0);
    color += diffuse * Irradiance(n) * gi * ao;
    color += Environment(reflect(-v, n), roughness) * EnvironmentBrdf(f0, roughness, nv) * (ao * sky);
    return color;
}
)";

        // Ultra's terrain: the atlas's surface and normals, the sun shadowed and its contact shadow, the
        // baked light from around (vGi: its colour twice over, its occlusion) and the screen's occlusion.
        const char* const kTerrainUltraFragment = R"(
uniform float uDebug;
uniform sampler2D uReliefGi;  // unit 7: a relief chunk's light from around (TerrainGi.hpp ReliefChunkGi)
uniform vec4 uReliefGiBox;    // its lowest corner, on (w)
in vec2 vUv;
in vec3 vNormal;
in vec4 vGi;
// A relief's light from around: its chunk's faces round its cell as an ambient cube (each axis's face
// toward the normal, weighted by the normal's square along it), the bake's halving undone.
vec3 ReliefGi(vec3 n)
{
    ivec3 c = clamp(ivec3(floor(vWorld - n * 0.3 - uReliefGiBox.xyz)), ivec3(0), ivec3(7));
    vec3 w = n * n;
    int across = c.x + 8 * c.z;
    vec3 gx = texelFetch(uReliefGi, ivec2(across, c.y + 8 * (n.x > 0.0 ? 1 : 0)), 0).rgb;
    vec3 gy = texelFetch(uReliefGi, ivec2(across, c.y + 8 * (n.y > 0.0 ? 3 : 2)), 0).rgb;
    vec3 gz = texelFetch(uReliefGi, ivec2(across, c.y + 8 * (n.z > 0.0 ? 5 : 4)), 0).rgb;
    return (gx * w.x + gy * w.y + gz * w.z) * 2.0;
}
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec4 surface = uMaps.y > 0.5 ? texture(uSurfaceMap, vUv) : vec4(0.75, 0.0, 1.0, 1.0);
    vec3 n = NormalMapped(normalize(vNormal), vUv);
    vec4 screen = ScreenOcclusion();
    float sun = SunLight() * screen.g;
    vec3 gi = uReliefGiBox.w > 0.5 ? ReliefGi(normalize(vNormal)) : vGi.rgb * 2.0;
    if (uDebug > 2.5 && uDebug < 3.5) { FragColor = vec4(gi * 0.5, 1.0); return; }
    if (uDebug > 0.5 && uDebug < 2.5) { FragColor = vec4(vec3(uDebug > 1.5 ? vGi.a * screen.r : sun), 1.0); return; }
    vec3 v = normalize(uEyePos - vWorld);
    vec3 color = Shade(ToLinear(tex.rgb), n, v, surface.r, surface.g, 1.0, sun, vGi.a * screen.r * surface.b, gi);
    FragColor = Fogged(vec4(ToDisplay(color), tex.a));
}
)";

        // Ultra's models: the part's colour (its texture, its Ray Dream pattern, its tint), its surface
        // (uSurface: roughness, metalness, reflectance; from its Ray Dream shader or its specular power),
        // its glow (linear, above white: it blooms).
        const char* const kModelUltraMain = R"(
uniform vec4 uSurface;
uniform vec3 uGlow;
void main()
{
    vec4 tex = texture(uTexture, vUv);
    vec3 n = normalize(vNormal);
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
    vec3 albedo = ToLinear(tex.rgb * uDiffuse);
    vec4 screen = ScreenOcclusion();
    vec3 v = normalize(uEyePos - vWorld);
    vec3 color = Shade(albedo, n, v, uSurface.x, uSurface.y, uSurface.z, SunLight() * screen.g, screen.r, vec3(1.0));
    color += albedo * ToLinear(uEmissive) + uGlow;
    FragColor = Fogged(vec4(ToDisplay(color), tex.a * uMaterial.y));
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

        // Ultra's billboards (position, uv) and particles (position, colour, uv): lit as the rest at Ultra
        // -- the sun's whole light through its shadow (a cut-out stands across it whatever its turn), the
        // sky's irradiance from above, the point lights round it -- so in the open sun a sprite is as
        // drawn (the sun and the sky together make the classic's 1.05) and in shadow as dark as the
        // ground beside it. What shines (uGlow above 1: an explosion's fire, sparks, stars) is not lit:
        // its colour times uGlow, above white, so it blooms. The texture's colours are premultiplied:
        // ToDisplay(ToLinear(c) * k) is c * k^(1/2.2), its alpha kept.
        std::string SpriteUltraVertex(bool colored)
        {
            std::string source = colored ? "layout(location = 0) in vec3 aPosition;\n"
                                           "layout(location = 1) in vec4 aColor;\n"
                                           "layout(location = 2) in vec2 aTexCoord;\n"
                                         : "layout(location = 0) in vec3 aPosition;\n"
                                           "layout(location = 1) in vec2 aTexCoord;\n";
            source += R"(
uniform mat4 World;
uniform mat4 View;
uniform mat4 Projection;
uniform mat4 uLightViewProj;
uniform mat4 uLightViewProjFar;
uniform vec3 uLightDir;
out vec2 vUv;
out vec4 vColor;
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
)";
            source += colored ? "    vColor = aColor;\n" : "    vColor = vec4(1.0);\n";
            source += R"(
    vWorld = world.xyz;
    vDepth = -view.z;
    vec4 probe = vec4(world.xyz - uLightDir * 0.7, 1.0);
    vLightPos = uLightViewProj * probe;
    vLightPosFar = uLightViewProjFar * probe;
}
)";
            return source;
        }
        const char* const kSpriteUltraFragment = R"(
uniform float uAlpha;
uniform float uGlow;
uniform float uDebug; // 4: GALAXY_EFFECTS_DEBUG=sprites, the light it takes (halved), magenta what shines
in vec2 vUv;
in vec4 vColor;
void main()
{
    vec4 tex = texture(uTexture, vUv) * vColor;
    vec3 albedo = ToLinear(tex.rgb);
    vec3 color;
    if (uGlow > 1.0)
    {
        color = albedo * uGlow;
    }
    else
    {
        vec3 light = vec3(uSunLight.x * SunLight()) + Irradiance(vec3(0.0, 1.0, 0.0));
        for (int i = 0; i < 8; ++i)
        {
            if (i >= uPointCount) break;
            float reach = clamp(1.0 - length(uPointPos[i] - vWorld) / uPointRadius[i], 0.0, 1.0);
            light += ToLinear(uPointColor[i]) * (reach * reach * 0.8);
        }
        color = albedo * light;
        if (uDebug > 3.5) { FragColor = vec4(ToDisplay(light) * 0.5, tex.a * uAlpha); return; }
    }
    if (uDebug > 3.5) { FragColor = vec4(1.0, 0.0, 1.0, tex.a * uAlpha); return; }
    FragColor = Fogged(vec4(ToDisplay(color), tex.a * uAlpha));
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
        m_model = CompileShaderEffect(device, "the model shader", skin + kModelVertex,
                                      lighting + kModelFragment + kModelMain);
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
            // =gi (Ultra): the light from around, halved; =sprites (Ultra): the sprites' and particles'
            // light, halved (magenta: what shines).
            m_debug = std::string(debug) == "shadow"      ? 1.0f
                      : std::string(debug) == "occlusion" ? 2.0f
                      : std::string(debug) == "gi"        ? 3.0f
                      : std::string(debug) == "sprites"   ? 4.0f
                                                          : 0.0f;
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

    Graphics::ShaderEffect& SceneShaders::SpriteWith(Graphics::Texture2D* texture, float alpha, float glow)
    {
        if (auto* ultra = UltraSprite(m_ultraSprite.get(), texture, glow))
        {
            ultra->SetUniformFloat("uAlpha", alpha);
            return *ultra;
        }
        if (texture != nullptr)
        {
            m_sprite->SetTexture(0, *texture);
        }
        m_sprite->SetUniformFloat("uAlpha", alpha);
        return *m_sprite;
    }

    void SceneShaders::SetReliefGi(Graphics::Texture2D* table, const Vector3& boxMin)
    {
        if (!m_ultraActive)
        {
            return;
        }
        if (table != nullptr)
        {
            m_ultraLit->SetTexture(7, *table);
            m_ultraLit->SetUniformVec4("uReliefGiBox", boxMin.X, boxMin.Y, boxMin.Z, 1.0f);
        }
        else
        {
            m_ultraLit->SetUniformVec4("uReliefGiBox", 0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    Graphics::ShaderEffect* SceneShaders::UltraParticleWith(Graphics::Texture2D* texture, float glow)
    {
        auto* ultra = UltraSprite(m_ultraParticle.get(), texture, glow);
        if (ultra != nullptr)
        {
            ultra->SetUniformFloat("uAlpha", 1.0f);
        }
        return ultra;
    }

    Graphics::ShaderEffect* SceneShaders::UltraSprite(Graphics::ShaderEffect* effect, Graphics::Texture2D* texture, float glow)
    {
        if (!m_ultraActive || effect == nullptr)
        {
            return nullptr;
        }
        if (texture != nullptr)
        {
            effect->SetTexture(0, *texture);
        }
        // The shadow maps again: the textures are bound as they are set, and the passes since may have
        // taken their units.
        if (m_shadowValid)
        {
            effect->SetTexture(1, *m_shadowMaps[0]);
            effect->SetTexture(2, *m_shadowMaps[1]);
        }
        effect->SetUniformFloat("uGlow", glow);
        return effect;
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

    void SceneShaders::SetFrame(Graphics::GraphicsDevice& device, const SceneFrame& frame, const Graphics::BasicEffect& terrain,
                                const UltraSceneAssets* ultra)
    {
        m_frame = &frame;
        m_ultraActive = false;
        if (!m_ready)
        {
            return;
        }
        if (ultra != nullptr && frame.effects.ultra)
        {
            PrepareUltra(device);
            m_ultraActive = m_ultraReady;
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
        if (m_ultraActive)
        {
            for (auto* effect : {m_ultraOccluded.get(), m_ultraLit.get()})
            {
                SetCommon(*effect);
                effect->setWorldProperty(terrain.World);
                if (auto* texture = terrain.getTextureProperty())
                {
                    effect->SetTexture(0, *texture);
                }
                effect->SetUniformFloat("uDebug", m_debug);
            }
            SetCommon(*m_ultraModel);
            for (auto* effect : {m_ultraSprite.get(), m_ultraParticle.get()})
            {
                if (effect == nullptr) continue;
                SetCommon(*effect);
                effect->setWorldProperty(Matrix::getIdentityProperty());
                effect->SetUniformVec3("uLightDir", direction.X, direction.Y, direction.Z);
                effect->SetUniformFloat("uDebug", m_debug);
            }
            SetUltra(device, *ultra);
        }
    }

    bool SceneShaders::PrepareUltra(Graphics::GraphicsDevice& device)
    {
        if (!m_ready)
        {
            return false;
        }
        if (!m_ultraTried)
        {
            // Ultra's programs: the same vertex shaders (the terrain's with its baked colour whole).
            m_ultraTried = true;
            const std::string lighting = std::string(kLighting) + kUltraLighting;
            const std::string skin = kSkin;
            m_ultraOccluded = CompileShaderEffect(device, "Ultra's terrain shader", TerrainVertex(true, true),
                                                  lighting + kTerrainUltraFragment);
            m_ultraLit = CompileShaderEffect(device, "Ultra's relief shader", TerrainVertex(false, true),
                                             lighting + kTerrainUltraFragment);
            m_ultraModel = CompileShaderEffect(device, "Ultra's model shader", skin + kModelVertex,
                                               lighting + kModelFragment + kModelUltraMain);
            m_geometrySolid = CompileShaderEffect(device, "Ultra's geometry pass (solid)", kGeometrySolidVertex,
                                                  std::string(kGeometryCommon) + kGeometrySolidFragment);
            m_geometryCutout = CompileShaderEffect(device, "Ultra's geometry pass (cutouts)", kGeometryCutoutVertex,
                                                   std::string(kGeometryCommon) + kGeometryCutoutFragment);
            m_geometrySkinned = CompileShaderEffect(device, "Ultra's geometry pass (models)", skin + kGeometrySkinnedVertex,
                                                    std::string(kGeometryCommon) + kGeometrySolidFragment);
            m_geometryOccluded = CompileShaderEffect(device, "Ultra's geometry pass (terrain)", GeometryTerrainVertex(3),
                                                     std::string(kGeometryCommon) + kGeometryTerrainFragment);
            m_geometryLit = CompileShaderEffect(device, "Ultra's geometry pass (reliefs)", GeometryTerrainVertex(2),
                                                std::string(kGeometryCommon) + kGeometryTerrainFragment);
            m_geometryWater = CompileShaderEffect(device, "Ultra's geometry pass (water)", kGeometryWaterVertex,
                                                  std::string(kGeometryCommon) + kWaterWavesGlsl + kGeometryWaterFragment);
            m_ultraReady = m_ultraOccluded && m_ultraLit && m_ultraModel && m_geometrySolid && m_geometryCutout &&
                           m_geometrySkinned && m_geometryOccluded && m_geometryLit;
            // The sprites' and particles': where they do not compile, High's sprite shader and BasicEffect.
            m_ultraSprite = CompileShaderEffect(device, "Ultra's sprite shader", SpriteUltraVertex(false),
                                                lighting + kSpriteUltraFragment);
            m_ultraParticle = CompileShaderEffect(device, "Ultra's particle shader", SpriteUltraVertex(true),
                                                  lighting + kSpriteUltraFragment);
            if (m_ultraReady)
            {
                for (auto* effect : {m_ultraOccluded.get(), m_ultraLit.get(), m_ultraModel.get(), m_ultraSprite.get(),
                                     m_ultraParticle.get()})
                {
                    if (effect == nullptr) continue;
                    effect->SetUniformInt("uTexture", 0);
                    effect->SetUniformInt("uShadowMap", 1);
                    effect->SetUniformInt("uShadowMapFar", 2);
                    effect->SetUniformInt("uEnv", 3);
                    effect->SetUniformInt("uSurfaceMap", 4);
                    effect->SetUniformInt("uNormalMap", 5);
                    effect->SetUniformInt("uScreenOcclusion", 6);
                    effect->SetUniformInt("uReliefGi", 7);
                }
                m_white = std::make_unique<Graphics::Texture2D>(device, 1, 1);
                const Microsoft::Xna::Framework::Color white(255, 255, 255, 255);
                m_white->SetData(&white, 1);
                // The sky wraps round the horizon and stops at its top and bottom; blended between its
                // levels.
                m_envSampler.setFilterProperty(Graphics::TextureFilter::Linear);
                m_envSampler.setAddressUProperty(Graphics::TextureAddressMode::Wrap);
                m_envSampler.setAddressVProperty(Graphics::TextureAddressMode::Clamp);
                m_envSampler.setAddressWProperty(Graphics::TextureAddressMode::Clamp);
            }
        }
        return m_ultraReady;
    }

    bool SceneShaders::BeginGeometry(Graphics::GraphicsDevice& device, const SceneFrame& frame)
    {
        if (!Prepare(device) || !PrepareUltra(device))
        {
            return false;
        }
        for (auto* effect : {m_geometrySolid.get(), m_geometryCutout.get(), m_geometrySkinned.get(), m_geometryOccluded.get(),
                             m_geometryLit.get()})
        {
            effect->setWorldProperty(Matrix::getIdentityProperty());
            effect->setViewProperty(frame.camera.view);
            effect->setProjectionProperty(frame.camera.projection);
            effect->SetUniformFloat("uReflect", 0.0f);
        }
        if (m_geometryWater)
        {
            m_geometryWater->setWorldProperty(Matrix::getIdentityProperty());
            m_geometryWater->setViewProperty(frame.camera.view);
            m_geometryWater->setProjectionProperty(frame.camera.projection);
            m_geometryWater->SetUniformFloat("uTime", frame.effects.seconds);
        }
        m_geometryCutout->SetUniformInt("uTexture", 0);
        m_geometryOccluded->SetUniformInt("uSurfaceMap", 4);
        m_geometryLit->SetUniformInt("uSurfaceMap", 4);
        return true;
    }

    Graphics::ShaderEffect* SceneShaders::SkinnedGeometry(const std::vector<Matrix>& bones, const Matrix& world)
    {
        if (!m_ultraReady || bones.empty() || bones.size() > static_cast<std::size_t>(kMaxBones))
        {
            return nullptr;
        }
        m_geometrySkinned->setWorldProperty(world);
        SetBones(*m_geometrySkinned, bones);
        return m_geometrySkinned.get();
    }

    void SceneShaders::SetUltra(Graphics::GraphicsDevice& device, const UltraSceneAssets& ultra)
    {
        const auto& frame = *m_frame;
        const Vector3 sun = SunDirection();
        const auto& eye = frame.camera.position;
        // The background's wraps round the horizon, as WorldRenderer::DrawBackground lays it.
        constexpr float kTwoPi = 6.28318530718f;
        const float horizontalFov = 2.0f * std::atan(std::tan(frame.camera.fieldOfView * 0.5f) * frame.camera.aspectRatio);
        const float repeats = std::max(1.0f, std::round(kTwoPi / std::max(horizontalFov, 0.01f)));
        float sh[27];
        for (std::size_t i = 0; i < 9; ++i)
        {
            sh[i * 3] = ultra.irradiance[i].X;
            sh[i * 3 + 1] = ultra.irradiance[i].Y;
            sh[i * 3 + 2] = ultra.irradiance[i].Z;
        }
        const auto& viewport = device.getViewportProperty();
        for (auto* effect : {m_ultraOccluded.get(), m_ultraLit.get(), m_ultraModel.get()})
        {
            effect->SetUniformVec3("uSunDir", sun.X, sun.Y, sun.Z);
            effect->SetUniformVec2("uSunLight", kUltraSun, 1.0f);
            effect->SetUniformVec3("uEyePos", eye.X, eye.Y, eye.Z);
            effect->SetUniformVec3Array("uSh", sh, 9);
            effect->SetUniformVec4("uEnvInfo", repeats, static_cast<float>(std::max(0, ultra.environmentLevels - 1)), 0.0f, 0.0f);
            m_ultraScreenOcclusion = ultra.screenOcclusion != nullptr;
            effect->SetUniformVec4("uMaps", ultra.terrainNormals != nullptr ? 1.0f : 0.0f,
                                   ultra.terrainSurface != nullptr ? 1.0f : 0.0f,
                                   ultra.screenOcclusion != nullptr ? 1.0f : 0.0f, 0.0f);
            effect->SetUniformVec2("uScreenTexel", 1.0f / std::max(1, viewport.getWidthProperty()),
                                   1.0f / std::max(1, viewport.getHeightProperty()));
            effect->SetTexture(3, ultra.environment != nullptr ? *ultra.environment : *m_white);
            if (effect != m_ultraModel.get() && ultra.terrainAlbedo != nullptr)
            {
                effect->SetTexture(0, *ultra.terrainAlbedo); // the atlas, finer
            }
            effect->SetTexture(4, ultra.terrainSurface != nullptr ? *ultra.terrainSurface : *m_white);
            effect->SetTexture(5, ultra.terrainNormals != nullptr ? *ultra.terrainNormals : *m_white);
            effect->SetTexture(6, ultra.screenOcclusion != nullptr ? *ultra.screenOcclusion : *m_white);
        }
        for (auto* effect : {m_ultraSprite.get(), m_ultraParticle.get()})
        {
            if (effect == nullptr) continue;
            effect->SetUniformVec3("uSunDir", sun.X, sun.Y, sun.Z);
            effect->SetUniformVec2("uSunLight", kUltraSun, 1.0f);
            effect->SetUniformVec3Array("uSh", sh, 9);
            effect->SetUniformVec4("uMaps", 0.0f, 0.0f, 0.0f, 0.0f);
        }
        auto& samplers = device.getSamplerStatesProperty();
        samplers[3] = m_envSampler;
        samplers[4] = Graphics::SamplerState::LinearWrap;
        samplers[5] = Graphics::SamplerState::LinearWrap;
        samplers[6] = Graphics::SamplerState::PointClamp;
        samplers[7] = Graphics::SamplerState::PointClamp;
    }

    void SceneShaders::SetPartMaterial(const ProceduralMaterial* material)
    {
        auto& effect = Model();
        if (m_ultraActive)
        {
            // Ultra: the part's surface from its Ray Dream shader (else the model's, PrepareModel's), its glow.
            if (material != nullptr && material->hasSurface)
            {
                const auto surface = SurfaceFromRayDream(material->highlight, material->shininess, material->reflection);
                effect.SetUniformVec4("uSurface", surface.roughness, surface.metalness, surface.specular, 0.0f);
                effect.SetUniformVec3("uGlow", material->glow.X * 2.5f, material->glow.Y * 2.5f, material->glow.Z * 2.5f);
            }
            else
            {
                effect.SetUniformVec4("uSurface", m_modelSurface.roughness, m_modelSurface.metalness, m_modelSurface.specular,
                                      0.0f);
                effect.SetUniformVec3("uGlow", 0.0f, 0.0f, 0.0f);
            }
        }
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
        auto& lit = *Lit();
        if (m_ultraActive)
        {
            // Another texture than the atlas (the cubes, the grass): the atlas's maps do not fit it.
            lit.SetUniformVec4("uMaps", 0.0f, 0.0f, m_ultraScreenOcclusion ? 1.0f : 0.0f, 0.0f);
        }
        lit.setWorldProperty(world);
        if (texture != nullptr)
        {
            lit.SetTexture(0, *texture);
        }
        return lit;
    }

    bool SceneShaders::PrepareModel(Graphics::SkinnedEffect& stock, const std::vector<Matrix>& bones, const Matrix& world,
                                    float alpha, bool tinted, const Vector3& tint)
    {
        if (!m_ready || bones.empty() || bones.size() > static_cast<std::size_t>(kMaxBones))
        {
            return false;
        }
        auto& effect = Model();
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
        if (m_ultraActive)
        {
            m_modelSurface = SurfaceFromSpecularPower(stock.getSpecularPowerProperty());
        }
        return true;
    }
}
