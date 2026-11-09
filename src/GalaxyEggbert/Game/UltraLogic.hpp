#pragma once

// What the 3D effects' Ultra level computes on the CPU, without a GPU (tools/VerifyUltra.cpp tests it):
// the grade's colour table and the tonemapping curve (UltraFinish.hpp); the sky's light (the world's
// background picture as an environment: its blurred levels for reflections, its irradiance for the
// ambient light); a model part's surface from its Ray Dream shader.

#include <Microsoft/Xna/Framework/Vector2.hpp>
#include <Microsoft/Xna/Framework/Vector3.hpp>

#include <array>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    namespace UltraDetail
    {
        inline float Luma(const Microsoft::Xna::Framework::Vector3& c) noexcept
        {
            return 0.2126f * c.X + 0.7152f * c.Y + 0.0722f * c.Z;
        }

        inline Microsoft::Xna::Framework::Vector3 Mix(const Microsoft::Xna::Framework::Vector3& a,
                                                      const Microsoft::Xna::Framework::Vector3& b, float t) noexcept
        {
            return Microsoft::Xna::Framework::Vector3(a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t, a.Z + (b.Z - a.Z) * t);
        }
    }

    // The grade's colour table's size each way.
    inline constexpr int kGradeTableSize = 32;

    // ACES (Stephen Hill's fit of the RRT and ODT) of a linear grey: the fit's input and output matrices
    // keep greys grey (their rows sum to 1), so the curve alone. The tonemapping pass does the same per pixel.
    [[nodiscard]] inline float AcesFitted(float v) noexcept
    {
        const float mapped = (v * (v + 0.0245786f) - 0.000090537f) / (v * (0.983729f * v + 0.4329510f) + 0.238081f);
        return std::clamp(mapped, 0.0f, 1.0f);
    }

    // The grade's colour table (32 x 32 x 32, laid out as 32 slices of 32 x 32 side by side: 1024 x 32,
    // RGBA8; row g, column b * 32 + r): what every display colour becomes, for a world whose sky is `sky`.
    // A fifth of the way to an S-curve (a little more contrast), a light cool in the shadows and a light
    // warm in the highlights, the sky's own hue at a few percent, a touch more colour.
    [[nodiscard]] inline std::vector<std::uint8_t> BuildGradeTable(const Microsoft::Xna::Framework::Vector3& sky)
    {
        using Microsoft::Xna::Framework::Vector3;
        using UltraDetail::Luma;
        using UltraDetail::Mix;
        constexpr int n = kGradeTableSize;
        const Vector3 white(1.0f, 1.0f, 1.0f);
        // The world's tint: its sky's hue at a third of its strength, its brightness taken out.
        Vector3 tint = white;
        if (const float l = Luma(sky); l > 0.05f)
        {
            tint = Mix(white, Vector3(sky.X / l, sky.Y / l, sky.Z / l), 0.35f);
            const float tl = Luma(tint);
            tint = Vector3(tint.X / tl, tint.Y / tl, tint.Z / tl);
        }
        const Vector3 cool(0.94f, 0.99f, 1.07f);
        const Vector3 warm(1.05f, 1.0f, 0.93f);
        const Vector3 world = Mix(white, tint, 0.06f);
        std::vector<std::uint8_t> table(static_cast<std::size_t>(n) * n * n * 4);
        const auto curve = [](float x) { return x + (x * x * (3.0f - 2.0f * x) - x) * 0.2f; };
        const auto byte = [](float x) { return static_cast<std::uint8_t>(std::lround(std::clamp(x, 0.0f, 1.0f) * 255.0f)); };
        for (int b = 0; b < n; ++b)
        {
            for (int g = 0; g < n; ++g)
            {
                for (int r = 0; r < n; ++r)
                {
                    Vector3 c(curve(static_cast<float>(r) / (n - 1)), curve(static_cast<float>(g) / (n - 1)),
                              curve(static_cast<float>(b) / (n - 1)));
                    const float l = Luma(c);
                    const Vector3 shadow = Mix(white, cool, (1.0f - l) * (1.0f - l) * 0.3f);
                    const Vector3 high = Mix(white, warm, l * l * 0.2f);
                    c = Vector3(c.X * shadow.X * high.X * world.X, c.Y * shadow.Y * high.Y * world.Y,
                                c.Z * shadow.Z * high.Z * world.Z);
                    const float grey = Luma(c);
                    c = Mix(Vector3(grey, grey, grey), c, 1.06f);
                    const std::size_t at =
                        (static_cast<std::size_t>(g) * (n * n) + static_cast<std::size_t>(b) * n + static_cast<std::size_t>(r)) * 4;
                    table[at + 0] = byte(c.X);
                    table[at + 1] = byte(c.Y);
                    table[at + 2] = byte(c.Z);
                    table[at + 3] = 255;
                }
            }
        }
        return table;
    }

    // ---- The sky's light ------------------------------------------------------------------------------

    // The linear light the 3D effects' terrain gets from its surroundings and from the sun, at full sun:
    // TerrainLighting.hpp's ambient 0.63 and ambient + diffuse 1.05, as display brightness (a gamma of
    // 2.2) -- so Ultra's linear lighting comes out as bright as the other levels'.
    inline constexpr float kUltraAmbient = 0.3617f; // 0.63^2.2
    inline constexpr float kUltraSun = 0.7516f;     // 1.05^2.2 - 0.63^2.2

    // How the background picture wraps the world (WorldRenderer::DrawBackground): round the horizon
    // `repeats` times, each picture 3/4 as high as it is wide (640 x 480), its middle on the horizon.
    // The picture's (u, v) seen in direction d (u wraps, v clamps).
    [[nodiscard]] inline Microsoft::Xna::Framework::Vector2 SkyUv(const Microsoft::Xna::Framework::Vector3& d,
                                                                  float repeats) noexcept
    {
        constexpr float kTwoPi = 6.28318530718f;
        const float azimuth = std::atan2(d.X, -d.Z);
        const float elevation = std::asin(std::clamp(d.Y, -1.0f, 1.0f));
        const float wide = kTwoPi / std::max(1.0f, repeats);
        return Microsoft::Xna::Framework::Vector2(azimuth / wide, 0.5f - elevation / (wide * 0.75f));
    }

    // The nine real spherical harmonics' values in direction d (unit).
    [[nodiscard]] inline std::array<float, 9> ShBasis(const Microsoft::Xna::Framework::Vector3& d) noexcept
    {
        return {0.282095f,
                0.488603f * d.Y,
                0.488603f * d.Z,
                0.488603f * d.X,
                1.092548f * d.X * d.Y,
                1.092548f * d.Y * d.Z,
                0.315392f * (3.0f * d.Z * d.Z - 1.0f),
                1.092548f * d.X * d.Z,
                0.546274f * (d.X * d.X - d.Y * d.Y)};
    }

    struct SkyEnvironment
    {
        int width = 0;  // the first level's
        int height = 0;
        std::vector<std::vector<std::uint8_t>> levels; // RGBA8 display colours, each half the last, to 1x1
        // The ambient light a surface turned n gets (multiply by its albedo): sum of irradiance[i] *
        // ShBasis(n)[i]. Linear, scaled so that its mean brightness is kUltraAmbient.
        std::array<Microsoft::Xna::Framework::Vector3, 9> irradiance{};
    };

    // The ambient light in direction n (SkyEnvironment::irradiance).
    [[nodiscard]] inline Microsoft::Xna::Framework::Vector3 SkyIrradiance(const SkyEnvironment& sky,
                                                                          const Microsoft::Xna::Framework::Vector3& n) noexcept
    {
        const auto basis = ShBasis(n);
        Microsoft::Xna::Framework::Vector3 sum(0.0f, 0.0f, 0.0f);
        for (std::size_t i = 0; i < 9; ++i)
        {
            sum = Microsoft::Xna::Framework::Vector3(sum.X + sky.irradiance[i].X * basis[i], sum.Y + sky.irradiance[i].Y * basis[i],
                                                     sum.Z + sky.irradiance[i].Z * basis[i]);
        }
        return sum;
    }

    namespace UltraDetail
    {
        // Bilinear texel of an RGBA8 level at (u, v): u wraps, v clamps.
        inline Microsoft::Xna::Framework::Vector3 SampleWrapped(const std::vector<std::uint8_t>& level, int w, int h, float u,
                                                                float v) noexcept
        {
            const float x = u * static_cast<float>(w) - 0.5f;
            const float y = std::clamp(v, 0.0f, 1.0f) * static_cast<float>(h) - 0.5f;
            const int x0 = static_cast<int>(std::floor(x));
            const int y0 = static_cast<int>(std::floor(y));
            const float fx = x - static_cast<float>(x0);
            const float fy = y - static_cast<float>(y0);
            const auto texel = [&](int tx, int ty)
            {
                tx = ((tx % w) + w) % w;
                ty = std::clamp(ty, 0, h - 1);
                const std::size_t at = (static_cast<std::size_t>(ty) * static_cast<std::size_t>(w) + static_cast<std::size_t>(tx)) * 4;
                return Microsoft::Xna::Framework::Vector3(level[at] / 255.0f, level[at + 1] / 255.0f, level[at + 2] / 255.0f);
            };
            const auto a = Mix(texel(x0, y0), texel(x0 + 1, y0), fx);
            const auto b = Mix(texel(x0, y0 + 1), texel(x0 + 1, y0 + 1), fx);
            return Mix(a, b, fy);
        }
    }

    // The background picture (RGBA8, `w` x `h`) as the sky's light: shrunk to 128 x 96 and halved down to
    // 1x1, each level softened by a tent (wrapping round the horizon) so that the levels stand for ever
    // rougher reflections; and the irradiance of its linear light (512 directions spread evenly over the
    // sphere, the picture's top row standing for everything above it, its bottom row for everything
    // below), scaled to kUltraAmbient. No picture (w or h 0): an even sky of colour `haze`.
    [[nodiscard]] inline SkyEnvironment BuildSkyEnvironment(const std::uint8_t* rgba, int w, int h, float repeats,
                                                            const Microsoft::Xna::Framework::Vector3& haze)
    {
        using Microsoft::Xna::Framework::Vector3;
        SkyEnvironment sky;
        constexpr int kWidth = 128;
        constexpr int kHeight = 96;
        std::vector<std::uint8_t> level(static_cast<std::size_t>(kWidth) * kHeight * 4);
        for (int y = 0; y < kHeight; ++y)
        {
            for (int x = 0; x < kWidth; ++x)
            {
                float r = haze.X, g = haze.Y, b = haze.Z;
                if (rgba != nullptr && w > 0 && h > 0)
                {
                    // The box of source texels under this one.
                    const int x0 = x * w / kWidth, x1 = std::max(x0 + 1, (x + 1) * w / kWidth);
                    const int y0 = y * h / kHeight, y1 = std::max(y0 + 1, (y + 1) * h / kHeight);
                    double sr = 0.0, sg = 0.0, sb = 0.0;
                    for (int sy = y0; sy < y1; ++sy)
                    {
                        for (int sx = x0; sx < x1; ++sx)
                        {
                            const std::uint8_t* t = rgba + (static_cast<std::size_t>(sy) * static_cast<std::size_t>(w) +
                                                            static_cast<std::size_t>(sx)) * 4;
                            sr += t[0];
                            sg += t[1];
                            sb += t[2];
                        }
                    }
                    const double n = static_cast<double>((x1 - x0) * (y1 - y0)) * 255.0;
                    r = static_cast<float>(sr / n);
                    g = static_cast<float>(sg / n);
                    b = static_cast<float>(sb / n);
                }
                const std::size_t at = (static_cast<std::size_t>(y) * kWidth + static_cast<std::size_t>(x)) * 4;
                level[at] = static_cast<std::uint8_t>(std::lround(std::clamp(r, 0.0f, 1.0f) * 255.0f));
                level[at + 1] = static_cast<std::uint8_t>(std::lround(std::clamp(g, 0.0f, 1.0f) * 255.0f));
                level[at + 2] = static_cast<std::uint8_t>(std::lround(std::clamp(b, 0.0f, 1.0f) * 255.0f));
                level[at + 3] = 255;
            }
        }
        sky.width = kWidth;
        sky.height = kHeight;
        int lw = kWidth, lh = kHeight;
        sky.levels.push_back(level);
        while (lw > 1 || lh > 1)
        {
            const int nw = std::max(1, lw / 2), nh = std::max(1, lh / 2);
            std::vector<std::uint8_t> next(static_cast<std::size_t>(nw) * nh * 4);
            const auto& last = sky.levels.back();
            for (int y = 0; y < nh; ++y)
            {
                for (int x = 0; x < nw; ++x)
                {
                    // A 4x4 tent (1 3 3 1) round the 2x2 under it: halved and softened at once.
                    float sum[3] = {0.0f, 0.0f, 0.0f};
                    float total = 0.0f;
                    static constexpr float kTent[4] = {1.0f, 3.0f, 3.0f, 1.0f};
                    for (int dy = 0; dy < 4; ++dy)
                    {
                        for (int dx = 0; dx < 4; ++dx)
                        {
                            const int sx = ((x * 2 - 1 + dx) % lw + lw) % lw;
                            const int sy = std::clamp(y * 2 - 1 + dy, 0, lh - 1);
                            const float weight = kTent[dx] * kTent[dy];
                            const std::size_t at = (static_cast<std::size_t>(sy) * static_cast<std::size_t>(lw) +
                                                    static_cast<std::size_t>(sx)) * 4;
                            for (int c = 0; c < 3; ++c)
                            {
                                sum[c] += static_cast<float>(last[at + static_cast<std::size_t>(c)]) * weight;
                            }
                            total += weight;
                        }
                    }
                    const std::size_t at = (static_cast<std::size_t>(y) * static_cast<std::size_t>(nw) + static_cast<std::size_t>(x)) * 4;
                    for (int c = 0; c < 3; ++c)
                    {
                        next[at + static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(std::lround(sum[c] / total));
                    }
                    next[at + 3] = 255;
                }
            }
            sky.levels.push_back(std::move(next));
            lw = nw;
            lh = nh;
        }

        // The irradiance: the light projected on the harmonics (Fibonacci directions), each band convolved
        // with the cosine lobe (pi, 2pi/3, pi/4), divided by pi (the ambient multiplies an albedo). Its
        // hue at 40 % (kSkyTint): the picture is a backdrop, not all that stands round the world, and a
        // whole sky's colour turns grey walls and yellow Blupi to its own.
        constexpr int kDirections = 512;
        std::array<Vector3, 9> projected{};
        const auto& soft = sky.levels[std::min<std::size_t>(2, sky.levels.size() - 1)];
        const int sw = std::max(1, kWidth >> 2), sh = std::max(1, kHeight >> 2);
        for (int i = 0; i < kDirections; ++i)
        {
            const float z = 1.0f - (2.0f * static_cast<float>(i) + 1.0f) / kDirections;
            const float radius = std::sqrt(std::max(0.0f, 1.0f - z * z));
            const float angle = 2.39996323f * static_cast<float>(i);
            const Vector3 d(radius * std::cos(angle), z, radius * std::sin(angle));
            const auto uv = SkyUv(d, repeats);
            Vector3 c = UltraDetail::SampleWrapped(soft, sw, sh, uv.X, uv.Y);
            c = Vector3(std::pow(c.X, 2.2f), std::pow(c.Y, 2.2f), std::pow(c.Z, 2.2f));
            constexpr float kSkyTint = 0.4f;
            const float grey = UltraDetail::Luma(c);
            c = UltraDetail::Mix(Vector3(grey, grey, grey), c, kSkyTint);
            const auto basis = ShBasis(d);
            for (std::size_t k = 0; k < 9; ++k)
            {
                const float weight = basis[k] * (4.0f * 3.14159265f / kDirections);
                projected[k] = Vector3(projected[k].X + c.X * weight, projected[k].Y + c.Y * weight, projected[k].Z + c.Z * weight);
            }
        }
        static constexpr float kBand[9] = {1.0f, 2.0f / 3.0f, 2.0f / 3.0f, 2.0f / 3.0f, 0.25f, 0.25f, 0.25f, 0.25f, 0.25f};
        // The mean of the result over all directions is its first term times Y00: brought to kUltraAmbient.
        const float mean = UltraDetail::Luma(projected[0]) * 0.282095f;
        const float scale = mean > 1e-5f ? kUltraAmbient / mean : 0.0f;
        for (std::size_t k = 0; k < 9; ++k)
        {
            const float s = kBand[k] * scale;
            sky.irradiance[k] = Vector3(projected[k].X * s, projected[k].Y * s, projected[k].Z * s);
        }
        if (scale == 0.0f)
        {
            sky.irradiance[0] = Vector3(kUltraAmbient / 0.282095f, kUltraAmbient / 0.282095f, kUltraAmbient / 0.282095f);
        }
        return sky;
    }

    // ---- A model part's surface ------------------------------------------------------------------------

    struct UltraSurface
    {
        float roughness = 0.55f;
        float metalness = 0.0f;
        float specular = 1.0f; // the dielectric reflectance's scale (1: 4 %)
    };

    // From its Ray Dream shader's highlight (0..1, how strong), shininess (0..1, how sharp) and reflection
    // (0..1, a mirror's share): sharper is smoother, reflection is metal.
    [[nodiscard]] inline UltraSurface SurfaceFromRayDream(float highlight, float shininess, float reflection) noexcept
    {
        UltraSurface surface;
        surface.roughness = std::clamp(1.0f - std::clamp(shininess, 0.0f, 1.0f) * 0.85f, 0.12f, 0.95f);
        surface.metalness = std::clamp(reflection * 1.2f, 0.0f, 1.0f);
        surface.specular = std::clamp(highlight * 2.0f, 0.0f, 2.0f);
        return surface;
    }

    // From a SkinnedEffect's specular power (Blinn-Phong): roughness sqrt(2 / (power + 2)).
    [[nodiscard]] inline UltraSurface SurfaceFromSpecularPower(float power) noexcept
    {
        UltraSurface surface;
        surface.roughness = std::clamp(std::sqrt(2.0f / (std::max(power, 1.0f) + 2.0f)), 0.12f, 0.95f);
        return surface;
    }
}
