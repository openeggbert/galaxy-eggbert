#pragma once

// The water's surface at the 3D effects' Ultra level: a height field in world space (three swells and
// two scales of chop, drifting), so its waves run on across the cells, calmer far off where the chop
// would only shimmer. GLSL shared by the water's shader (SurfaceShaders.cpp), which lights it, and
// Ultra's geometry pass (SceneShaders.cpp), whose normals make the screen-space reflections ripple.

namespace GalaxyEggbert::Game
{
    inline constexpr const char* kWaterWavesGlsl = R"(
float WaveHash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}
float WaveNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(WaveHash(i), WaveHash(i + vec2(1.0, 0.0)), u.x),
               mix(WaveHash(i + vec2(0.0, 1.0)), WaveHash(i + vec2(1.0, 1.0)), u.x), u.y);
}
float WaveHeight(vec2 p, float t, float detail)
{
    float h = sin(dot(p, vec2(0.82, 0.57)) * 1.9 + t * 1.25) * 0.045;
    h += sin(dot(p, vec2(-0.43, 0.90)) * 2.7 + t * 1.6) * 0.03;
    h += sin(dot(p, vec2(0.97, -0.24)) * 4.1 - t * 2.1) * 0.018;
    h += (WaveNoise(p * 3.1 + vec2(t * 0.35, t * 0.21)) - 0.5) * 0.04 * detail;
    h += (WaveNoise(p * 7.3 - vec2(t * 0.52, -t * 0.4)) - 0.5) * 0.02 * detail;
    return h;
}
// The surface's normal (world space, up) at p at time t, seen from `distance` blocks away.
vec3 WaveNormal(vec2 p, float t, float distance)
{
    float detail = clamp(1.0 - distance / 40.0, 0.0, 1.0);
    const float e = 0.03;
    float h = WaveHeight(p, t, detail);
    vec2 slope = vec2(WaveHeight(p + vec2(e, 0.0), t, detail) - h, WaveHeight(p + vec2(0.0, e), t, detail) - h) / e;
    slope *= mix(0.35, 1.0, detail);
    return normalize(vec3(-slope.x, 1.0, -slope.y));
}
)";
}
