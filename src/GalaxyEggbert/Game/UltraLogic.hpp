#pragma once

// What the 3D effects' Ultra level computes on the CPU, without a GPU (tools/VerifyUltra.cpp tests it):
// the grade's colour table and the tonemapping curve (UltraFinish.hpp).

#include <Microsoft/Xna/Framework/Vector3.hpp>

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
        const Vector3 world = Mix(white, tint, 0.12f);
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
                    const Vector3 shadow = Mix(white, cool, (1.0f - l) * (1.0f - l) * 0.5f);
                    const Vector3 high = Mix(white, warm, l * l * 0.5f);
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
}
