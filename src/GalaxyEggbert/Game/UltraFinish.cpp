#include <GalaxyEggbert/Game/UltraFinish.hpp>

#include <GalaxyEggbert/Game/PostProcess.hpp>
#include <GalaxyEggbert/Game/SurfaceShaders.hpp>

#include <Microsoft/Xna/Framework/Graphics/BlendState.hpp>
#include <Microsoft/Xna/Framework/Graphics/DepthStencilState.hpp>
#include <Microsoft/Xna/Framework/Graphics/RasterizerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/SamplerState.hpp>
#include <Microsoft/Xna/Framework/Graphics/Viewport.hpp>
#include <Microsoft/Xna/Framework/Rectangle.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

namespace GalaxyEggbert::Game
{
    namespace
    {
        namespace Graphics = Microsoft::Xna::Framework::Graphics;
        using Microsoft::Xna::Framework::Vector3;

        // What every pass shares: the display colour to linear light (the world's shaders write display
        // colours, a gamma of 2.2), luminance, and texel fetches (RGBA16F samples only point-filtered).
        const char* const kCommon = R"(
in vec2 TexCoord;
out vec4 FragColor;
vec3 Linear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
float Luminance(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }
vec4 Fetch(sampler2D s, ivec2 p)
{
    return texelFetch(s, clamp(p, ivec2(0), textureSize(s, 0) - 1), 0);
}
vec4 Bilinear(sampler2D s, vec2 uv)
{
    vec2 p = uv * vec2(textureSize(s, 0)) - 0.5;
    ivec2 i = ivec2(floor(p));
    vec2 f = p - floor(p);
    return mix(mix(Fetch(s, i), Fetch(s, i + ivec2(1, 0)), f.x),
               mix(Fetch(s, i + ivec2(0, 1)), Fetch(s, i + ivec2(1, 1)), f.x), f.y);
}
)";

        // The geometry pass's pixels (SceneShaders.hpp): an octahedral view-space normal, the distance along
        // the view (0: nothing, the sky), how much of a mirror; positions in view space from it and back.
        const char* const kGeometry = R"(
uniform vec2 uTanHalf;  // tan(fov / 2) * aspect, tan(fov / 2)
uniform float uFrame;   // the frame's index: the noise turns with it
vec3 OctDecode(vec2 e)
{
    vec3 n = vec3(e, 1.0 - abs(e.x) - abs(e.y));
    if (n.z < 0.0) n.xy = (1.0 - abs(n.yx)) * vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);
    return normalize(n);
}
vec3 ViewAt(vec2 uv, float depth) { return vec3((uv * 2.0 - 1.0) * uTanHalf * depth, -depth); }
vec2 Project(vec3 p) { return p.xy / (-p.z) / uTanHalf * 0.5 + 0.5; }
// Interleaved gradient noise (Jimenez): even, cheap, a new pattern each frame.
float Noise(vec2 p) { return fract(52.9829189 * fract(dot(p + uFrame * 5.588238, vec2(0.06711056, 0.00583715)))); }
)";

        // Ambient occlusion (twelve rays over the hemisphere, cosine-weighted, half a block and more, a
        // rotation per pixel) and the sun's contact shadow (a march toward the sun through the depth, a
        // block at most: the small shadows the shadow map's texels are too coarse for). r and g.
        const char* const kOcclusion = R"(
uniform sampler2D uTexture; // the geometry
uniform vec3 uSunView;      // the way to the sun, in view space
uniform vec3 uUpView;       // the world's up, in view space
float DepthAt(vec2 uv)
{
    ivec2 size = textureSize(uTexture, 0);
    return texelFetch(uTexture, clamp(ivec2(uv * vec2(size)), ivec2(0), size - 1), 0).b;
}
void main()
{
    ivec2 size = textureSize(uTexture, 0);
    ivec2 at = ivec2(gl_FragCoord.xy);
    vec4 g = texelFetch(uTexture, at, 0);
    if (g.b <= 0.0) { FragColor = vec4(1.0); return; }
    vec2 uv = (vec2(at) + 0.5) / vec2(size);
    vec3 p = ViewAt(uv, g.b);
    vec3 n = OctDecode(g.rg);
    float noise = Noise(gl_FragCoord.xy);
    vec3 t = normalize(abs(n.y) < 0.99 ? cross(n, vec3(0.0, 1.0, 0.0)) : cross(n, vec3(1.0, 0.0, 0.0)));
    vec3 b = cross(n, t);
    float angle = noise * 6.2831853;
    t = t * cos(angle) + b * sin(angle);
    b = cross(n, t);
    const float radius = 0.7;
    float occluded = 0.0;
    for (int i = 0; i < 12; ++i)
    {
        float fi = float(i);
        float r = (fi + 0.5) / 12.0;
        float phi = fi * 2.3999632;
        vec3 dir = t * (cos(phi) * sqrt(r)) + b * (sin(phi) * sqrt(r)) + n * sqrt(1.0 - r);
        vec3 s = p + n * 0.03 + dir * (radius * mix(0.2, 1.0, fract(fi * 0.618034 + noise)));
        vec2 suv = Project(s);
        if (suv.x < 0.0 || suv.y < 0.0 || suv.x > 1.0 || suv.y > 1.0) continue;
        float d = DepthAt(suv);
        float diff = -s.z - d;
        if (d > 0.0 && diff > 0.02) occluded += 1.0 - smoothstep(radius * 0.5, radius * 1.5, diff);
    }
    float ao = pow(clamp(1.0 - occluded / 12.0, 0.0, 1.0), 1.4);
    float contact = 1.0;
    // On what faces up only (floors, tops: where things stand and the small shadows at their feet
    // matter): on walls seen edge on and on round bodies the march found their own surface again in the
    // depth, a fine grid of false shadow.
    if (dot(n, uSunView) > 0.2 && dot(n, uUpView) > 0.6 && g.b < 40.0)
    {
        // Thin occluders only (a tenth of a block): a thick one, Blupi's body, hides what lies behind it
        // in the depth, and the march would take that for its own shadow. Nor a surface turned as this one
        // is (its own plane, its own rounded body).
        float step = 0.04;
        float travelled = step * (1.0 + noise * 0.5);
        for (int i = 0; i < 16; ++i)
        {
            vec3 q = p + n * 0.04 + uSunView * travelled;
            vec2 quv = Project(q);
            if (quv.x < 0.0 || quv.y < 0.0 || quv.x > 1.0 || quv.y > 1.0) break;
            ivec2 hitAt = clamp(ivec2(quv * vec2(size)), ivec2(0), size - 1);
            vec4 h = texelFetch(uTexture, hitAt, 0);
            float d = h.b;
            float behind = -q.z - d;
            // An occluder nearer the camera than the point itself: the far side of the same surface,
            // or of a body seen round its edge, is not one.
            if (d > 0.0 && d < g.b - 0.02 && behind > 0.02 && behind < 0.06 && dot(OctDecode(h.rg), n) < 0.85)
            {
                contact = mix(0.5, 1.0, travelled / 0.6);
                break;
            }
            travelled += step;
        }
        contact = mix(contact, 1.0, smoothstep(25.0, 40.0, g.b));
    }
    FragColor = vec4(ao, contact, 0.0, 1.0);
}
)";

        // The occlusion smoothed along uStep (across, then down: two passes), nine taps on a Gaussian,
        // where the depth agrees (its edges kept): the rays' per-pixel turn left no dither behind.
        const char* const kOcclusionBlur = R"(
uniform sampler2D uTexture;  // the occlusion
uniform sampler2D uGeometry;
uniform vec2 uStep;
void main()
{
    ivec2 size = textureSize(uTexture, 0);
    ivec2 at = ivec2(gl_FragCoord.xy);
    vec4 g0 = texelFetch(uGeometry, at, 0);
    float d0 = g0.b;
    vec3 n0 = OctDecode(g0.rg);
    vec2 sum = vec2(0.0);
    float total = 0.0;
    for (int i = -4; i <= 4; ++i)
    {
        ivec2 q = clamp(at + ivec2(uStep) * i, ivec2(0), size - 1);
        vec4 g = texelFetch(uGeometry, q, 0);
        // The same surface: turned alike and near in depth (a surface seen edge on changes depth fast
        // from pixel to pixel; its normal does not).
        float w = exp(-float(i * i) / 10.0) * pow(max(dot(OctDecode(g.rg), n0), 0.0), 8.0) *
                  exp(-abs(g.b - d0) / (0.1 * d0 + 0.1));
        sum += texelFetch(uTexture, q, 0).rg * w;
        total += w;
    }
    FragColor = vec4(sum / max(total, 0.0001), 0.0, 1.0);
}
)";

        // Screen-space reflections (the water): the view reflected off each mirror pixel, marched through the
        // depth in growing steps, the crossing refined by halving; the scene's colour there, faded toward the
        // screen's edges and far off, weighted by the mirror and Schlick's Fresnel. rgb the colour, a its weight.
        const char* const kReflections = R"(
uniform sampler2D uTexture;  // the scene
uniform sampler2D uGeometry;
float DepthAt(vec2 uv)
{
    ivec2 size = textureSize(uGeometry, 0);
    return texelFetch(uGeometry, clamp(ivec2(uv * vec2(size)), ivec2(0), size - 1), 0).b;
}
void main()
{
    ivec2 size = textureSize(uGeometry, 0);
    ivec2 at = ivec2(gl_FragCoord.xy);
    vec4 g = texelFetch(uGeometry, at, 0);
    if (g.a < 0.01 || g.b <= 0.0) { FragColor = vec4(0.0); return; }
    vec2 uv = (vec2(at) + 0.5) / vec2(size);
    vec3 p = ViewAt(uv, g.b);
    vec3 n = OctDecode(g.rg);
    vec3 v = normalize(-p);
    vec3 r = reflect(-v, n);
    float stepLength = 0.25;
    float t = stepLength * (0.5 + Noise(gl_FragCoord.xy));
    bool found = false;
    float hit = 0.0;
    for (int i = 0; i < 40; ++i)
    {
        vec3 q = p + r * t;
        if (q.z > -0.05) break;
        vec2 quv = Project(q);
        if (quv.x < 0.0 || quv.y < 0.0 || quv.x > 1.0 || quv.y > 1.0) break;
        float d = DepthAt(quv);
        float behind = -q.z - d;
        if (d > 0.0 && behind > 0.0 && behind < stepLength * 1.5 + 0.15)
        {
            float lo = t - stepLength;
            float hi = t;
            for (int k = 0; k < 5; ++k)
            {
                float mid = (lo + hi) * 0.5;
                vec3 m = p + r * mid;
                if (-m.z - DepthAt(Project(m)) > 0.0) hi = mid; else lo = mid;
            }
            hit = hi;
            found = true;
            break;
        }
        t += stepLength;
        stepLength *= 1.12;
    }
    if (!found) { FragColor = vec4(0.0); return; }
    vec2 huv = Project(p + r * hit);
    vec2 edge = min(huv, 1.0 - huv);
    float fade = clamp(min(edge.x, edge.y) * 10.0, 0.0, 1.0) * (1.0 - smoothstep(30.0, 60.0, hit));
    float fresnel = 0.02 + 0.98 * pow(1.0 - max(dot(n, v), 0.0), 5.0);
    vec3 c = texelFetch(uTexture, clamp(ivec2(huv * vec2(size)), ivec2(0), size - 1), 0).rgb;
    FragColor = vec4(c, g.a * fade * mix(0.25, 1.0, fresnel));
}
)";

        // The air (half the size): along each pixel's view, to what it shows (none over the sky), 20
        // steps through a height fog (thicker below Blupi's level, thinning above it); at each the sun's
        // light where the shadow map lets it through (god rays: Henyey-Greenstein, forward), the sky's haze
        // everywhere. rgb the light scattered toward the eye (linear), a what is left of the scene behind.
        const char* const kFog = R"(
uniform sampler2D uTexture;     // the geometry, full size
uniform sampler2D uShadowNear;  // unit 1
uniform sampler2D uShadowFar;   // unit 2
uniform mat4 uInvView;
uniform mat4 uLightNear;
uniform mat4 uLightFar;
uniform vec3 uSunDir;           // the way the sunlight goes (world)
uniform vec3 uSunColor;         // linear
uniform vec3 uFogColor;         // linear: the sky's haze
uniform vec4 uFogInfo;          // density a block, falloff a block up, the reference height, the shadow map on
float UnpackDepth(vec4 c) { return dot(c, vec4(1.0, 1.0 / 255.0, 1.0 / 65025.0, 1.0 / 16581375.0)); }
float Lit(sampler2D map, mat4 light, vec3 w, out bool inside)
{
    vec4 p = light * vec4(w, 1.0);
    vec3 n = p.xyz / p.w;
    vec2 uv = n.xy * 0.5 + 0.5;
    inside = uv.x > 0.0 && uv.y > 0.0 && uv.x < 1.0 && uv.y < 1.0 && n.z < 1.0;
    if (!inside) return 1.0;
    return n.z - 0.003 <= UnpackDepth(texture(map, uv)) ? 1.0 : 0.0;
}
float SunAt(vec3 w)
{
    if (uFogInfo.w < 0.5) return 1.0;
    bool inside;
    float near = Lit(uShadowNear, uLightNear, w, inside);
    if (inside) return near;
    return Lit(uShadowFar, uLightFar, w, inside);
}
void main()
{
    ivec2 full = textureSize(uTexture, 0);
    ivec2 at = clamp(ivec2(gl_FragCoord.xy * 2.0), ivec2(0), full - 1);
    vec2 uv = (vec2(at) + 1.0) / vec2(full);
    float depth = texelFetch(uTexture, at, 0).b;
    // The sky: its picture is painted with its own air (a night sky turned white under ours).
    if (depth <= 0.0) { FragColor = vec4(0.0, 0.0, 0.0, 1.0); return; }
    vec3 eye = (uInvView * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 end = (uInvView * vec4(ViewAt(uv, min(depth, 100.0)), 1.0)).xyz;
    vec3 ray = end - eye;
    float len = length(ray);
    vec3 dir = ray / max(len, 0.0001);
    const int kSteps = 20;
    float stepLength = len / float(kSteps);
    float jitter = Noise(gl_FragCoord.xy);
    float g = 0.65;
    float cosTheta = dot(dir, -uSunDir);
    float hg = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
    float phase = mix(0.6, hg, 0.75);
    vec3 scattered = vec3(0.0);
    float transmittance = 1.0;
    for (int i = 0; i < kSteps; ++i)
    {
        vec3 w = eye + dir * ((float(i) + jitter) * stepLength);
        float density = uFogInfo.x * exp(-max(w.y - uFogInfo.z, 0.0) * uFogInfo.y);
        vec3 light = uSunColor * (SunAt(w) * phase) + uFogColor * 0.6;
        scattered += transmittance * density * stepLength * light;
        transmittance *= exp(-density * stepLength);
    }
    FragColor = vec4(scattered, transmittance);
}
)";

        // Temporal anti-aliasing: each frame's projection is moved by a fraction of a pixel (Jitter); this
        // frame's pixel blended with last frame's at the same point of the world (found again from the
        // depth and last frame's camera), last frame's held within the colours round this pixel (so what
        // moved leaves no ghost). In display colour (after the tonemapping), where its blend is even.
        const char* const kTemporal = R"(
uniform sampler2D uTexture;   // this frame, graded
uniform sampler2D uHistory;   // unit 1: the last result (bilinear)
uniform sampler2D uGeometry;  // unit 2
uniform mat4 uInvView;
uniform mat4 uPrevViewProj;
uniform float uHistoryValid;
void main()
{
    ivec2 size = textureSize(uTexture, 0);
    ivec2 at = ivec2(TexCoord * vec2(size));
    vec3 current = Fetch(uTexture, at).rgb;
    vec3 low = current;
    vec3 high = current;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec3 c = Fetch(uTexture, at + ivec2(x, y)).rgb;
            low = min(low, c);
            high = max(high, c);
        }
    }
    float depth = Fetch(uGeometry, at).b;
    if (depth <= 0.0) depth = 1000.0;
    vec2 uv = (vec2(at) + 0.5) / vec2(size);
    vec3 world = (uInvView * vec4(ViewAt(uv, depth), 1.0)).xyz;
    vec4 previous = uPrevViewProj * vec4(world, 1.0);
    vec2 back = previous.xy / previous.w * 0.5 + 0.5;
    float valid = uHistoryValid * (back.x > 0.0 && back.y > 0.0 && back.x < 1.0 && back.y < 1.0 ? 1.0 : 0.0);
    vec3 history = clamp(texture(uHistory, back).rgb, low, high);
    FragColor = vec4(mix(current, history, 0.88 * valid), 1.0);
}
)";

        // The last touches, onto the screen: the far scenery out of focus (the focus on Blupi; a disc of
        // twelve taps, nearer pixels kept out of a farther one's blur), the camera's own motion blurred
        // along each pixel's way since the last frame (eight taps, half a frame's shutter), film grain.
        const char* const kLens = R"(
uniform sampler2D uTexture;   // the anti-aliased frame (bilinear)
uniform sampler2D uGeometry;  // unit 1
uniform mat4 uInvView;
uniform mat4 uPrevViewProj;
uniform vec4 uFocus;          // the focus distance, the sharp range beyond it, the most blur (pixels), on
uniform vec2 uMotion;         // the shutter (fraction of a frame), on
uniform float uGrain;
float DepthAt(vec2 uv)
{
    ivec2 size = textureSize(uGeometry, 0);
    float d = texelFetch(uGeometry, clamp(ivec2(uv * vec2(size)), ivec2(0), size - 1), 0).b;
    return d <= 0.0 ? 1000.0 : d;
}
float Blur(float depth)
{
    return uFocus.w * clamp((depth - uFocus.x - uFocus.y) / max(depth, 1.0) * 2.5, 0.0, 1.0) * uFocus.z;
}
void main()
{
    vec2 size = vec2(textureSize(uTexture, 0));
    vec2 uv = TexCoord;
    float depth = DepthAt(uv);
    vec3 color = texture(uTexture, uv).rgb;
    // Depth of field (far only: what Blupi stands in front of stays sharp).
    float radius = Blur(depth);
    if (radius > 0.5)
    {
        vec3 sum = color;
        float total = 1.0;
        for (int i = 0; i < 12; ++i)
        {
            float r = sqrt((float(i) + 0.5) / 12.0) * radius;
            float a = float(i) * 2.3999632;
            vec2 at = uv + vec2(cos(a), sin(a)) * r / size;
            float w = Blur(DepthAt(at)) >= r * 0.5 ? 1.0 : 0.0;
            sum += texture(uTexture, at).rgb * w;
            total += w;
        }
        color = sum / total;
    }
    // Motion blur (the camera's).
    if (uMotion.y > 0.5)
    {
        vec3 world = (uInvView * vec4(ViewAt(uv, depth), 1.0)).xyz;
        vec4 previous = uPrevViewProj * vec4(world, 1.0);
        vec2 back = previous.xy / previous.w * 0.5 + 0.5;
        vec2 velocity = (uv - back) * uMotion.x;
        float pixels = length(velocity * size);
        if (pixels > 1.0)
        {
            velocity *= min(1.0, 24.0 / pixels);
            vec3 sum = color;
            for (int i = 1; i < 8; ++i)
            {
                sum += texture(uTexture, uv - velocity * (float(i) / 7.0 - 0.5)).rgb;
            }
            color = sum / 8.0;
        }
    }
    // Film grain: finer in the light.
    float grain = Noise(gl_FragCoord.xy) - 0.5;
    color += grain * uGrain * (1.0 - 0.6 * dot(color, vec3(0.3333)));
    FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
)";

        // The scene's mean log luminance per cell of a 64x64 grid (16 samples each).
        const char* const kLogLuminance = R"(
uniform sampler2D uTexture;
void main()
{
    vec2 cell = vec2(textureSize(uTexture, 0)) / 64.0;
    vec2 at = floor(gl_FragCoord.xy);
    float sum = 0.0;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            vec2 p = (at + (vec2(float(x), float(y)) + 0.5) / 4.0) * cell;
            sum += log(max(Luminance(Linear(Fetch(uTexture, ivec2(p)).rgb)), 0.0001));
        }
    }
    FragColor = vec4(sum / 16.0, 0.0, 0.0, 1.0);
}
)";

        // A quarter of the size each way: the mean of each 4x4 block.
        const char* const kReduce = R"(
uniform sampler2D uTexture;
void main()
{
    ivec2 base = ivec2(floor(gl_FragCoord.xy)) * 4;
    float sum = 0.0;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            sum += Fetch(uTexture, base + ivec2(x, y)).r;
        }
    }
    FragColor = vec4(sum / 16.0, 0.0, 0.0, 1.0);
}
)";

        // The exposure the picture wants (mid grey kept where the picture is of middling brightness,
        // half of the way to it where darker or brighter: night stays night), followed over time.
        // 1.35 keeps a middling picture's mid grey where it was (AcesFitted(0.18 * 1.35) / 0.79 = 0.2).
        const char* const kAdapt = R"(
uniform sampler2D uTexture;
uniform sampler2D uPrevious;
uniform float uRate;
void main()
{
    float mean = exp(texelFetch(uTexture, ivec2(0), 0).r);
    float target = clamp(1.35 * sqrt(0.17 / max(mean, 0.001)), 0.85, 1.8);
    float previous = texelFetch(uPrevious, ivec2(0), 0).r;
    FragColor = vec4(uRate >= 1.0 ? target : mix(previous, target, uRate), 0.0, 0.0, 1.0);
}
)";

        // Half the size: a binomial 4x4 of the exposed scene, each texel weighted down by its own
        // brightness (Karis: one blazing pixel does not flicker the glow), then what is above white.
        const char* const kBloomPrefilter = R"(
uniform sampler2D uTexture;
uniform sampler2D uExposure;
void main()
{
    float exposure = texelFetch(uExposure, ivec2(0), 0).r;
    ivec2 base = ivec2(floor(TexCoord * vec2(textureSize(uTexture, 0)) - 1.0));
    vec4 w = vec4(1.0, 3.0, 3.0, 1.0);
    vec3 sum = vec3(0.0);
    float total = 0.0;
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            vec3 c = Linear(Fetch(uTexture, base + ivec2(x, y)).rgb) * exposure;
            float weight = w[x] * w[y] / (1.0 + Luminance(c));
            sum += c * weight;
            total += weight;
        }
    }
    vec3 c = sum / total;
    float l = Luminance(c);
    const float threshold = 1.0;
    const float knee = 0.5;
    float soft = clamp(l - threshold + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    FragColor = vec4(c * (max(soft, l - threshold) / max(l, 0.00001)), 1.0);
}
)";

        // Half the size: a binomial 4x4.
        const char* const kBloomDown = R"(
uniform sampler2D uTexture;
void main()
{
    ivec2 base = ivec2(floor(TexCoord * vec2(textureSize(uTexture, 0)) - 1.0));
    vec4 w = vec4(1.0, 3.0, 3.0, 1.0);
    vec3 sum = vec3(0.0);
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            sum += Fetch(uTexture, base + ivec2(x, y)).rgb * (w[x] * w[y]);
        }
    }
    FragColor = vec4(sum / 64.0, 1.0);
}
)";

        // Twice the size, added to what the level above holds: a 3x3 tent of the level below.
        const char* const kBloomUp = R"(
uniform sampler2D uTexture;
uniform float uWeight;
void main()
{
    vec2 texel = 1.0 / vec2(textureSize(uTexture, 0));
    vec3 sum = Bilinear(uTexture, TexCoord).rgb * 4.0;
    sum += (Bilinear(uTexture, TexCoord + vec2(texel.x, 0.0)).rgb + Bilinear(uTexture, TexCoord - vec2(texel.x, 0.0)).rgb +
            Bilinear(uTexture, TexCoord + vec2(0.0, texel.y)).rgb + Bilinear(uTexture, TexCoord - vec2(0.0, texel.y)).rgb) * 2.0;
    sum += Bilinear(uTexture, TexCoord + texel).rgb + Bilinear(uTexture, TexCoord - texel).rgb +
           Bilinear(uTexture, TexCoord + vec2(texel.x, -texel.y)).rgb + Bilinear(uTexture, TexCoord + vec2(-texel.x, texel.y)).rgb;
    FragColor = vec4(sum / 16.0 * uWeight, 1.0);
}
)";

        // Exposed, the glow added, ACES (Hill's fit of the RRT and ODT; white at the usual exposure, 1.35,
        // comes out at 0.95 rather than the fit's 0.71: divided by 0.79), back to display colour, graded by
        // the world's table, the corners a little darker; the reflections laid in first. GALAXY_ULTRA_DEBUG:
        // 1 no grade, 2 no glow, 3 a fixed exposure, 4 the scene as drawn, 5 the exposure (a third of it, as
        // grey), 6 the ambient occlusion, 7 the contact shadow, 8 the reflections' weight, 9 the light the
        // air scatters (four times over).
        const char* const kTonemap = R"(
uniform sampler2D uTexture;
uniform sampler2D uBloom;
uniform sampler2D uExposure;
uniform sampler2D uGrade;
uniform sampler2D uReflection; // unit 4: rgb, weight
uniform sampler2D uOcclusion;  // unit 5: the screen's occlusion (shown by GALAXY_ULTRA_DEBUG 6, 7)
uniform sampler2D uFog;        // unit 6: the air, half the size (rgb scattered in, a left through)
uniform float uBloomStrength;
uniform int uDebug;
const mat3 kAcesInput = mat3(0.59719, 0.07600, 0.02840, 0.35458, 0.90834, 0.13383, 0.04823, 0.01566, 0.83777);
const mat3 kAcesOutput = mat3(1.60475, -0.10208, -0.00327, -0.53108, 1.10813, -0.07276, -0.07367, -0.00605, 1.07602);
vec3 Aces(vec3 v)
{
    v = kAcesInput * v;
    v = (v * (v + 0.0245786) - 0.000090537) / (v * (0.983729 * v + 0.4329510) + 0.238081);
    return clamp(kAcesOutput * v, 0.0, 1.0);
}
vec3 Grade(vec3 c)
{
    const float n = 32.0;
    float slice = c.b * (n - 1.0);
    float s0 = floor(slice);
    float s1 = min(s0 + 1.0, n - 1.0);
    vec2 uv = vec2((c.r * (n - 1.0) + 0.5) / (n * n), (c.g * (n - 1.0) + 0.5) / n);
    return mix(texture(uGrade, uv + vec2(s0 / n, 0.0)).rgb, texture(uGrade, uv + vec2(s1 / n, 0.0)).rgb, slice - s0);
}
void main()
{
    ivec2 at = ivec2(TexCoord * vec2(textureSize(uTexture, 0)));
    vec3 c = Linear(Fetch(uTexture, at).rgb);
    vec4 reflection = Fetch(uReflection, at);
    c = mix(c, Linear(reflection.rgb), clamp(reflection.a, 0.0, 1.0));
    vec4 air = Bilinear(uFog, TexCoord);
    c = c * air.a + air.rgb;
    float exposure = texelFetch(uExposure, ivec2(0), 0).r;
    if (uDebug == 3) exposure = 1.35;
    vec3 hdr = c * exposure + (uDebug == 2 ? vec3(0.0) : Bilinear(uBloom, TexCoord).rgb * uBloomStrength);
    vec3 display = pow(min(Aces(hdr) / 0.79, vec3(1.0)), vec3(1.0 / 2.2));
    if (uDebug != 1) display = Grade(display);
    if (uDebug == 4) display = pow(c, vec3(1.0 / 2.2));
    if (uDebug == 5) display = vec3(exposure / 3.0);
    if (uDebug == 6) display = vec3(Fetch(uOcclusion, at).r);
    if (uDebug == 7) display = vec3(Fetch(uOcclusion, at).g);
    if (uDebug == 8) display = vec3(reflection.a);
    if (uDebug == 9) display = air.rgb * 4.0;
    vec2 d = TexCoord - 0.5;
    display *= 1.0 - dot(d, d) * 0.35;
    FragColor = vec4(clamp(display, 0.0, 1.0), 1.0);
}
)";

    }

    bool UltraFinish::Prepare(Graphics::GraphicsDevice& device, int width, int height)
    {
        if (m_failed)
        {
            return false;
        }
        if (!m_tonemap)
        {
            const std::string common = kCommon;
            const auto compile = [&](const char* name, const char* body)
            { return CompileShaderEffect(device, name, PostVertexSource(), common + body); };
            m_logLuminance = compile("the exposure's luminance", kLogLuminance);
            m_reduce = compile("the exposure's reduction", kReduce);
            m_adapt = compile("the exposure's adaptation", kAdapt);
            m_bloomPrefilter = compile("the HDR glow's bright pass", kBloomPrefilter);
            m_bloomDown = compile("the HDR glow's down pass", kBloomDown);
            m_bloomUp = compile("the HDR glow's up pass", kBloomUp);
            m_tonemap = compile("the tonemapping", kTonemap);
            const std::string geometry = std::string(kCommon) + kGeometry;
            m_occlusion = CompileShaderEffect(device, "the screen's occlusion", PostVertexSource(), geometry + kOcclusion);
            m_occlusionBlur = CompileShaderEffect(device, "the occlusion's blur", PostVertexSource(), geometry + kOcclusionBlur);
            m_reflections = CompileShaderEffect(device, "the screen-space reflections", PostVertexSource(),
                                                geometry + kReflections);
            m_fogPass = CompileShaderEffect(device, "the volumetric fog", PostVertexSource(), geometry + kFog);
            m_temporal = CompileShaderEffect(device, "the temporal anti-aliasing", PostVertexSource(), geometry + kTemporal);
            m_lens = CompileShaderEffect(device, "the lens (depth of field, motion blur, grain)", PostVertexSource(),
                                         geometry + kLens);
            m_fxaa = CompileShaderEffect(device, "the Ultra edge smoothing (FXAA)", PostVertexSource(), FxaaFragmentSource());
            if (!m_logLuminance || !m_reduce || !m_adapt || !m_bloomPrefilter || !m_bloomDown || !m_bloomUp || !m_tonemap ||
                !m_fxaa || !m_occlusion || !m_occlusionBlur || !m_reflections || !m_fogPass || !m_temporal || !m_lens)
            {
                m_failed = true;
                m_tonemap.reset();
                return false;
            }
            m_adapt->SetUniformInt("uPrevious", 1);
            m_bloomPrefilter->SetUniformInt("uExposure", 1);
            m_tonemap->SetUniformInt("uBloom", 1);
            m_tonemap->SetUniformInt("uExposure", 2);
            m_tonemap->SetUniformInt("uGrade", 3);
            m_tonemap->SetUniformInt("uReflection", 4);
            m_tonemap->SetUniformInt("uOcclusion", 5);
            m_tonemap->SetUniformInt("uFog", 6);
            m_temporal->SetUniformInt("uHistory", 1);
            m_temporal->SetUniformInt("uGeometry", 2);
            m_lens->SetUniformInt("uGeometry", 1);
            m_fogPass->SetUniformInt("uShadowNear", 1);
            m_fogPass->SetUniformInt("uShadowFar", 2);
            m_occlusionBlur->SetUniformInt("uGeometry", 1);
            m_reflections->SetUniformInt("uGeometry", 1);
            m_batch = std::make_unique<Graphics::SpriteBatch>(device);
        }
        if (width != m_width || height != m_height || !m_scene)
        {
            try
            {
                // Kept when bound again (PreserveContents): the glow's levels are added to, and XNA fills a
                // discarded target with its purple.
                const auto target = [&](int w, int h, Graphics::SurfaceFormat format, bool depth)
                {
                    return std::make_unique<Graphics::RenderTarget2D>(
                        device, std::max(1, w), std::max(1, h), false, format,
                        depth ? Graphics::DepthFormat::Depth24Stencil8 : Graphics::DepthFormat::None, 0,
                        Graphics::RenderTargetUsage::PreserveContents);
                };
                const auto hdr = Graphics::SurfaceFormat::HdrBlendable;
                m_scene = target(width, height, hdr, true);
                for (int i = 0, size = 64; i < static_cast<int>(m_luminance.size()); ++i, size /= 4)
                {
                    m_luminance[static_cast<std::size_t>(i)] = target(size, size, hdr, false);
                }
                for (auto& exposure : m_exposure)
                {
                    exposure = target(1, 1, hdr, false);
                }
                for (int i = 0; i < kBloomLevels; ++i)
                {
                    m_bloom[static_cast<std::size_t>(i)] = target(width >> (i + 1), height >> (i + 1), hdr, false);
                }
                m_graded = target(width, height, Graphics::SurfaceFormat::Color, false);
                // Full floats: a half float's depth is a block in 2048 coarse, a few hundredths of a block
                // far off -- the occlusion and the contact shadows saw surfaces in themselves.
                m_geometry = target(width, height, Graphics::SurfaceFormat::Vector4, true);
                m_occlusionRaw = target(width, height, Graphics::SurfaceFormat::Color, false);
                m_occlusionSmooth = target(width, height, Graphics::SurfaceFormat::Color, false);
                m_occlusionAcross = target(width, height, Graphics::SurfaceFormat::Color, false);
                m_reflection = target(width, height, hdr, false);
                m_fog = target(width / 2, height / 2, hdr, false);
                for (auto& history : m_history)
                {
                    history = target(width, height, Graphics::SurfaceFormat::Color, false);
                }
                m_historyValid = false;
                m_geometryValid = false;
                m_width = width;
                m_height = height;
                m_exposureFresh = true;
            }
            catch (const std::exception& error)
            {
                std::cout << "GalaxyEggbertCNA: 3D effects: no HDR render targets (" << error.what()
                          << "), Ultra drawn as High." << std::endl;
                m_failed = true;
                m_scene.reset();
                return false;
            }
        }
        return true;
    }

    bool UltraFinish::Begin(Graphics::GraphicsDevice& device, const Microsoft::Xna::Framework::Color& clear, int width,
                            int height)
    {
        const auto viewport = device.getViewportProperty();
        m_screenWidth = viewport.getWidthProperty();
        m_screenHeight = viewport.getHeightProperty();
        if (width <= 0 || height <= 0)
        {
            width = m_screenWidth;
            height = m_screenHeight;
        }
        if (m_screenWidth <= 0 || m_screenHeight <= 0 || width <= 0 || height <= 0 || !Prepare(device, width, height))
        {
            return false;
        }
        device.SetRenderTarget(m_scene.get());
        device.Clear(clear);
        return true;
    }

    Microsoft::Xna::Framework::Matrix UltraFinish::Jitter(const Microsoft::Xna::Framework::Matrix& projection, int width,
                                                          int height)
    {
        // Halton (2, 3), eight points a cycle: a different fraction of a pixel each frame, evenly over it.
        const auto halton = [](int index, int base)
        {
            float f = 1.0f, r = 0.0f;
            for (int i = index; i > 0; i /= base)
            {
                f /= static_cast<float>(base);
                r += f * static_cast<float>(i % base);
            }
            return r;
        };
        m_projection = projection;
        if (width <= 0 || height <= 0)
        {
            return projection;
        }
        const int index = static_cast<int>(m_frame % 8u) + 1;
        const float jx = (halton(index, 2) - 0.5f) * 2.0f / static_cast<float>(width);
        const float jy = (halton(index, 3) - 0.5f) * 2.0f / static_cast<float>(height);
        // XNA's perspective divides by -z (M34 = -1): moving M31 and M32 by -j moves every point by j.
        auto jittered = projection;
        jittered.M31 -= jx;
        jittered.M32 -= jy;
        return jittered;
    }

    bool UltraFinish::BeginGeometry(Graphics::GraphicsDevice& device, int width, int height)
    {
        m_geometryValid = false;
        const auto viewport = device.getViewportProperty();
        if (width <= 0 || height <= 0)
        {
            width = viewport.getWidthProperty();
            height = viewport.getHeightProperty();
        }
        if (width <= 0 || height <= 0 || !Prepare(device, width, height))
        {
            return false;
        }
        device.SetRenderTarget(m_geometry.get());
        device.Clear(Microsoft::Xna::Framework::Color(0, 0, 0, 0)); // depth 0: nothing (the sky)
        return true;
    }

    void UltraFinish::SetCamera(Graphics::ShaderEffect& effect, const SceneFrame& frame) const
    {
        // From the projection itself (its M11 and M22 are 1 / tan of the half angles), not the camera's
        // fields: what the scene was drawn with.
        const auto& projection = frame.camera.projection;
        effect.SetUniformVec2("uTanHalf", 1.0f / projection.M11, 1.0f / projection.M22);
        effect.SetUniformFloat("uFrame", static_cast<float>(m_frame % 64));
    }

    Graphics::Texture2D* UltraFinish::EndGeometry(Graphics::GraphicsDevice& device, const SceneFrame& frame,
                                                  const Microsoft::Xna::Framework::Vector3& sunDirection)
    {
        // The screen's occlusion from the geometry just drawn: its ambient occlusion and the sun's contact
        // shadow, then smoothed where the depth agrees.
        const Graphics::BlendState blend = device.getBlendStateProperty();
        const Graphics::DepthStencilState depth = device.getDepthStencilStateProperty();
        const Graphics::RasterizerState raster = device.getRasterizerStateProperty();
        auto& samplers = device.getSamplerStatesProperty();
        const Graphics::SamplerState saved1 = samplers[1];
        samplers[1] = Graphics::SamplerState::PointClamp;
        SetCamera(*m_occlusion, frame);
        const auto toSun = Vector3::TransformNormal(Vector3(-sunDirection.X, -sunDirection.Y, -sunDirection.Z), frame.camera.view);
        const float length = std::sqrt(toSun.X * toSun.X + toSun.Y * toSun.Y + toSun.Z * toSun.Z);
        m_occlusion->SetUniformVec3("uSunView", toSun.X / length, toSun.Y / length, toSun.Z / length);
        const auto up = Vector3::TransformNormal(Vector3(0.0f, 1.0f, 0.0f), frame.camera.view);
        m_occlusion->SetUniformVec3("uUpView", up.X, up.Y, up.Z);
        Pass(device, m_occlusionRaw.get(), *m_geometry, *m_occlusion, m_width, m_height);
        m_occlusionBlur->SetTexture(1, *m_geometry);
        m_occlusionBlur->SetUniformVec2("uStep", 1.0f, 0.0f);
        Pass(device, m_occlusionAcross.get(), *m_occlusionRaw, *m_occlusionBlur, m_width, m_height);
        m_occlusionBlur->SetTexture(1, *m_geometry);
        m_occlusionBlur->SetUniformVec2("uStep", 0.0f, 1.0f);
        Pass(device, m_occlusionSmooth.get(), *m_occlusionAcross, *m_occlusionBlur, m_width, m_height);
        device.SetRenderTarget(nullptr);
        samplers[1] = saved1;
        device.setBlendStateProperty(blend);
        device.setDepthStencilStateProperty(depth);
        device.setRasterizerStateProperty(raster);
        m_geometryValid = true;
        return m_occlusionSmooth.get();
    }

    void UltraFinish::Pass(Graphics::GraphicsDevice& device, Graphics::RenderTarget2D* target, Graphics::Texture2D& source,
                           Graphics::ShaderEffect& effect, int width, int height, bool add)
    {
        device.SetRenderTarget(target);
        m_batch->Begin(Graphics::SpriteSortMode::Immediate, add ? &Graphics::BlendState::Additive : &Graphics::BlendState::Opaque,
                       &Graphics::SamplerState::PointClamp, &Graphics::DepthStencilState::None,
                       &Graphics::RasterizerState::CullNone, &effect);
        m_batch->Draw(source, Microsoft::Xna::Framework::Rectangle(0, 0, width, height),
                      Microsoft::Xna::Framework::Rectangle(0, 0, source.getWidthProperty(), source.getHeightProperty()),
                      Microsoft::Xna::Framework::Color::White);
        m_batch->End();
    }

    void UltraFinish::Exposure(Graphics::GraphicsDevice& device)
    {
        Pass(device, m_luminance[0].get(), *m_scene, *m_logLuminance, 64, 64);
        for (std::size_t i = 1; i < m_luminance.size(); ++i)
        {
            const int size = m_luminance[i]->getWidthProperty();
            Pass(device, m_luminance[i].get(), *m_luminance[i - 1], *m_reduce, size, size);
        }
        const int next = 1 - m_exposureIndex;
        // About a second to follow a change (at 60 frames a second).
        m_adapt->SetUniformFloat("uRate", m_exposureFresh ? 1.0f : 0.04f);
        m_adapt->SetTexture(1, *m_exposure[static_cast<std::size_t>(m_exposureIndex)]);
        Pass(device, m_exposure[static_cast<std::size_t>(next)].get(), *m_luminance.back(), *m_adapt, 1, 1);
        m_exposureIndex = next;
        m_exposureFresh = false;
    }

    void UltraFinish::Bloom(Graphics::GraphicsDevice& device)
    {
        auto& exposure = *m_exposure[static_cast<std::size_t>(m_exposureIndex)];
        m_bloomPrefilter->SetTexture(1, exposure);
        Pass(device, m_bloom[0].get(), *m_scene, *m_bloomPrefilter, m_bloom[0]->getWidthProperty(),
             m_bloom[0]->getHeightProperty());
        for (int i = 1; i < kBloomLevels; ++i)
        {
            auto& below = *m_bloom[static_cast<std::size_t>(i)];
            Pass(device, &below, *m_bloom[static_cast<std::size_t>(i - 1)], *m_bloomDown, below.getWidthProperty(),
                 below.getHeightProperty());
        }
        for (int i = kBloomLevels - 1; i > 0; --i)
        {
            auto& above = *m_bloom[static_cast<std::size_t>(i - 1)];
            m_bloomUp->SetUniformFloat("uWeight", 0.85f);
            Pass(device, &above, *m_bloom[static_cast<std::size_t>(i)], *m_bloomUp, above.getWidthProperty(),
                 above.getHeightProperty(), true);
        }
    }

    void UltraFinish::UpdateGrade(Graphics::GraphicsDevice& device, const Vector3& sky)
    {
        if (m_grade && m_gradeReady && sky.X == m_gradeSky.X && sky.Y == m_gradeSky.Y && sky.Z == m_gradeSky.Z)
        {
            return;
        }
        const auto table = BuildGradeTable(sky);
        constexpr int n = kGradeSize;
        std::vector<Microsoft::Xna::Framework::Color> colors(static_cast<std::size_t>(n * n * n));
        for (std::size_t i = 0; i < colors.size(); ++i)
        {
            colors[i] = Microsoft::Xna::Framework::Color(static_cast<int>(table[i * 4 + 0]), static_cast<int>(table[i * 4 + 1]),
                                                         static_cast<int>(table[i * 4 + 2]), 255);
        }
        if (!m_grade)
        {
            m_grade = std::make_unique<Graphics::Texture2D>(device, n * n, n);
        }
        m_grade->SetData(colors.data(), static_cast<int>(colors.size()));
        m_gradeSky = sky;
        m_gradeReady = true;
    }

    void UltraFinish::End(Graphics::GraphicsDevice& device, const SceneFrame& frame, const UltraSun& sun)
    {
        // The device's states as the world left them, for what is drawn after (the HUD).
        const Graphics::BlendState blend = device.getBlendStateProperty();
        const Graphics::DepthStencilState depth = device.getDepthStencilStateProperty();
        const Graphics::RasterizerState raster = device.getRasterizerStateProperty();
        auto& samplers = device.getSamplerStatesProperty();
        const std::array<Graphics::SamplerState, 7> saved{samplers[0], samplers[1], samplers[2], samplers[3],
                                                          samplers[4], samplers[5], samplers[6]};
        samplers[1] = Graphics::SamplerState::PointClamp;
        samplers[2] = Graphics::SamplerState::PointClamp;
        samplers[3] = Graphics::SamplerState::LinearClamp;
        samplers[4] = Graphics::SamplerState::PointClamp;
        samplers[5] = Graphics::SamplerState::PointClamp;
        samplers[6] = Graphics::SamplerState::PointClamp;

        // The reflections, from the lit scene and the geometry (none without this frame's geometry).
        const bool reflections = m_geometryValid;
        if (reflections)
        {
            SetCamera(*m_reflections, frame);
            m_reflections->SetTexture(1, *m_geometry);
            Pass(device, m_reflection.get(), *m_scene, *m_reflections, m_width, m_height);
        }
        else
        {
            device.SetRenderTarget(m_reflection.get());
            device.Clear(Microsoft::Xna::Framework::Color(0, 0, 0, 0));
        }
        // The air: a height fog lit by the sun through its shadow map (god rays) and by the sky's haze.
        if (m_geometryValid)
        {
            const auto setMatrix = [](Graphics::ShaderEffect& effect, const char* name, const Microsoft::Xna::Framework::Matrix& m)
            {
                const float values[16] = {m.M11, m.M12, m.M13, m.M14, m.M21, m.M22, m.M23, m.M24,
                                          m.M31, m.M32, m.M33, m.M34, m.M41, m.M42, m.M43, m.M44};
                effect.SetUniformMat4(name, values);
            };
            auto& fog = *m_fogPass;
            SetCamera(fog, frame);
            setMatrix(fog, "uInvView", Microsoft::Xna::Framework::Matrix::Invert(frame.camera.view));
            setMatrix(fog, "uLightNear", sun.nearViewProjection);
            setMatrix(fog, "uLightFar", sun.farViewProjection);
            fog.SetUniformVec3("uSunDir", sun.direction.X, sun.direction.Y, sun.direction.Z);
            // The sun's light in the air as bright as the world's sky is (a night world's air holds
            // almost none of it).
            const auto& haze = frame.effects.fogColor;
            const float daylight = std::clamp((0.2126f * haze.X + 0.7152f * haze.Y + 0.0722f * haze.Z) * 2.0f, 0.1f, 1.0f);
            fog.SetUniformVec3("uSunColor", kUltraSun * daylight, kUltraSun * 0.95f * daylight, kUltraSun * 0.85f * daylight);
            fog.SetUniformVec3("uFogColor", std::pow(haze.X, 2.2f), std::pow(haze.Y, 2.2f), std::pow(haze.Z, 2.2f));
            const bool shadowed = sun.shadowed && sun.shadowNear != nullptr && sun.shadowFar != nullptr;
            // A thin haze, thicker below Blupi's level (three blocks under his camera's target).
            fog.SetUniformVec4("uFogInfo", 0.014f, 0.1f, frame.camera.target.Y - 3.0f, shadowed ? 1.0f : 0.0f);
            fog.SetTexture(1, shadowed ? *sun.shadowNear : static_cast<Graphics::Texture2D&>(*m_reflection));
            fog.SetTexture(2, shadowed ? *sun.shadowFar : static_cast<Graphics::Texture2D&>(*m_reflection));
            Pass(device, m_fog.get(), *m_geometry, fog, m_fog->getWidthProperty(), m_fog->getHeightProperty());
        }
        else
        {
            device.SetRenderTarget(m_fog.get());
            device.Clear(Microsoft::Xna::Framework::Color(0, 0, 0, 255)); // no air: all left through
        }
        Exposure(device);
        Bloom(device);
        UpdateGrade(device, frame.effects.fogColor);
        m_tonemap->SetTexture(1, *m_bloom[0]);
        m_tonemap->SetTexture(2, *m_exposure[static_cast<std::size_t>(m_exposureIndex)]);
        m_tonemap->SetTexture(3, *m_grade);
        m_tonemap->SetTexture(4, *m_reflection);
        m_tonemap->SetTexture(5, m_geometryValid ? *m_occlusionSmooth : *m_reflection);
        m_tonemap->SetTexture(6, *m_fog);
        m_tonemap->SetUniformFloat("uBloomStrength", 0.12f);
        static const int debug = [] { const char* d = std::getenv("GALAXY_ULTRA_DEBUG"); return d ? std::atoi(d) : 0; }();
        m_tonemap->SetUniformInt("uDebug", debug);
        Pass(device, m_graded.get(), *m_scene, *m_tonemap, m_width, m_height);

        // Anti-aliased over the frames (TAA) with this frame's geometry and last frame's camera; without
        // the geometry, FXAA as the other levels.
        const auto setMatrix = [](Graphics::ShaderEffect& effect, const char* name, const Microsoft::Xna::Framework::Matrix& m)
        {
            const float values[16] = {m.M11, m.M12, m.M13, m.M14, m.M21, m.M22, m.M23, m.M24,
                                      m.M31, m.M32, m.M33, m.M34, m.M41, m.M42, m.M43, m.M44};
            effect.SetUniformMat4(name, values);
        };
        const auto viewProjection = frame.camera.view * m_projection;
        const auto inverseView = Microsoft::Xna::Framework::Matrix::Invert(frame.camera.view);
        if (m_geometryValid)
        {
            auto& history = *m_history[static_cast<std::size_t>(m_historyIndex)];
            auto& result = *m_history[static_cast<std::size_t>(1 - m_historyIndex)];
            samplers[1] = Graphics::SamplerState::LinearClamp;
            samplers[2] = Graphics::SamplerState::PointClamp;
            SetCamera(*m_temporal, frame);
            setMatrix(*m_temporal, "uInvView", inverseView);
            setMatrix(*m_temporal, "uPrevViewProj", m_previousViewProjection);
            m_temporal->SetUniformFloat("uHistoryValid", m_historyValid && !frame.effects.capture ? 1.0f : 0.0f);
            m_temporal->SetTexture(1, history);
            m_temporal->SetTexture(2, *m_geometry);
            Pass(device, &result, *m_graded, *m_temporal, m_width, m_height);
            m_historyIndex = 1 - m_historyIndex;
            m_historyValid = true;

            // The lens, onto the screen.
            device.SetRenderTarget(nullptr);
            samplers[1] = Graphics::SamplerState::PointClamp;
            SetCamera(*m_lens, frame);
            setMatrix(*m_lens, "uInvView", inverseView);
            setMatrix(*m_lens, "uPrevViewProj", m_previousViewProjection);
            const auto& eye = frame.camera.position;
            const auto& target = frame.camera.target;
            const float focus = std::sqrt((target.X - eye.X) * (target.X - eye.X) + (target.Y - eye.Y) * (target.Y - eye.Y) +
                                          (target.Z - eye.Z) * (target.Z - eye.Z));
            // Sharp to twelve blocks past Blupi; at most 5 pixels a 1080-line screen's blur.
            m_lens->SetUniformVec4("uFocus", focus, 12.0f, 5.0f * static_cast<float>(m_height) / 1080.0f, 1.0f);
            // Half a 60 Hz frame's shutter, whatever the frame's own length: a slow machine moves the
            // camera farther between its frames, not for longer while the shutter is open.
            const auto now = std::chrono::steady_clock::now();
            const float frameSeconds = m_previousValid ? std::chrono::duration<float>(now - m_previousTime).count() : 1.0f / 60.0f;
            m_previousTime = now;
            const float shutter = std::clamp(0.5f * (1.0f / 60.0f) / std::max(frameSeconds, 1.0f / 240.0f), 0.0f, 0.5f);
            m_lens->SetUniformVec2("uMotion", shutter, m_previousValid && !frame.effects.capture ? 1.0f : 0.0f);
            m_lens->SetUniformFloat("uGrain", 0.035f);
            m_lens->SetTexture(1, *m_geometry);
            m_batch->Begin(Graphics::SpriteSortMode::Immediate, &Graphics::BlendState::Opaque, &Graphics::SamplerState::LinearClamp,
                           &Graphics::DepthStencilState::None, &Graphics::RasterizerState::CullNone, m_lens.get());
            m_batch->Draw(result, Microsoft::Xna::Framework::Rectangle(0, 0, m_screenWidth, m_screenHeight),
                          Microsoft::Xna::Framework::Rectangle(0, 0, m_width, m_height), Microsoft::Xna::Framework::Color::White);
            m_batch->End();
        }
        else
        {
            m_historyValid = false;
            device.SetRenderTarget(nullptr);
            samplers[0] = Graphics::SamplerState::LinearClamp;
            m_fxaa->SetUniformVec2("uTexel", 1.0f / static_cast<float>(m_width), 1.0f / static_cast<float>(m_height));
            m_batch->Begin(Graphics::SpriteSortMode::Immediate, &Graphics::BlendState::Opaque, &Graphics::SamplerState::LinearClamp,
                           &Graphics::DepthStencilState::None, &Graphics::RasterizerState::CullNone, m_fxaa.get());
            m_batch->Draw(*m_graded, Microsoft::Xna::Framework::Rectangle(0, 0, m_screenWidth, m_screenHeight),
                          Microsoft::Xna::Framework::Rectangle(0, 0, m_width, m_height), Microsoft::Xna::Framework::Color::White);
            m_batch->End();
        }
        m_previousViewProjection = viewProjection;
        m_previousValid = true;

        device.setBlendStateProperty(blend);
        device.setDepthStencilStateProperty(depth);
        device.setRasterizerStateProperty(raster);
        for (std::size_t i = 0; i < saved.size(); ++i)
        {
            samplers[static_cast<int>(i)] = saved[i];
        }
        m_geometryValid = false;
        ++m_frame;
    }
}
