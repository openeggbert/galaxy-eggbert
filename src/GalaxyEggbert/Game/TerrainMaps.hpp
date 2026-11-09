#pragma once

// The 3D effects' Ultra level: the terrain atlas made finer and given a surface (SceneShaders.cpp, Ultra's
// terrain shader). Made from the atlas's own texels on Ultra's first frame; pure, tools/VerifyUltra.cpp
// tests it.
//
//  - Upscale: up to twice the texels each way (HiDef's 4096 the limit), Catmull-Rom (the tiles are rendered pictures, not pixel
//    art: a smooth curve through the texels, not their stairs), so the blocks close to the camera are
//    not a blur of bilinear texels at Ultra's resolution.
//  - BuildSurfaceMaps: per texel, a normal (the tile's brightness taken for its height: its relief, the
//    stones' bulges and the plates' rivets, catches the light) and a surface: roughness, metalness and
//    cavity (what lies below its surroundings, darker). Each tile is first told what it is from its
//    texels (grey, even and bright: metal; grey and busy: stone; brown: wood; colourful and even:
//    paint), each texel then a little rougher in its hollows.

#include <algorithm>
#include <array>
#include <future>
#include <thread>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Game
{
    namespace MapsDetail
    {
        // f(first, last) over rows [0, rows) in bands, on up to eight threads (one on the web).
        template <typename F>
        void ParallelRows(int rows, F&& f)
        {
#if defined(__EMSCRIPTEN__)
            f(0, rows);
#else
            const int threads = std::clamp(static_cast<int>(std::thread::hardware_concurrency()) / 2, 1, 8);
            if (threads <= 1 || rows < 64)
            {
                f(0, rows);
                return;
            }
            std::vector<std::future<void>> bands;
            const int band = (rows + threads - 1) / threads;
            for (int first = 0; first < rows; first += band)
            {
                bands.push_back(std::async(std::launch::async, [&f, first, last = std::min(rows, first + band)] { f(first, last); }));
            }
            for (auto& done : bands)
            {
                done.get();
            }
#endif
        }

        inline float CatmullRom(float p0, float p1, float p2, float p3, float t) noexcept
        {
            return 0.5f * (2.0f * p1 + (-p0 + p2) * t + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t * t +
                           (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t * t * t);
        }

        inline float Luma(const std::uint8_t* t) noexcept
        {
            return (0.2126f * t[0] + 0.7152f * t[1] + 0.0722f * t[2]) / 255.0f;
        }
    }

    // RGBA8 `w` x `h` to `ow` x `oh` (clamped at the edges; the padded atlas's tiles carry borders of their
    // own edge, so nothing bleeds between them, whatever the scale).
    [[nodiscard]] inline std::vector<std::uint8_t> Upscale(const std::uint8_t* rgba, int w, int h, int ow, int oh)
    {
        const float scaleX = static_cast<float>(w) / static_cast<float>(ow);
        const float scaleY = static_cast<float>(h) / static_cast<float>(oh);
        std::vector<std::uint8_t> out(static_cast<std::size_t>(ow) * static_cast<std::size_t>(oh) * 4);
        const auto at = [&](int x, int y, int c)
        {
            x = std::clamp(x, 0, w - 1);
            y = std::clamp(y, 0, h - 1);
            return static_cast<float>(rgba[(static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4 +
                                           static_cast<std::size_t>(c)]);
        };
        MapsDetail::ParallelRows(oh, [&](int firstRow, int lastRow) {
        for (int oy = firstRow; oy < lastRow; ++oy)
        {
            // The output texel's centre in the source's texel space.
            const float sy = (static_cast<float>(oy) + 0.5f) * scaleY - 0.5f;
            const int y1 = static_cast<int>(std::floor(sy));
            const float ty = sy - static_cast<float>(y1);
            for (int ox = 0; ox < ow; ++ox)
            {
                const float sx = (static_cast<float>(ox) + 0.5f) * scaleX - 0.5f;
                const int x1 = static_cast<int>(std::floor(sx));
                const float tx = sx - static_cast<float>(x1);
                for (int c = 0; c < 4; ++c)
                {
                    float rows[4];
                    for (int j = 0; j < 4; ++j)
                    {
                        const int y = y1 - 1 + j;
                        rows[j] = MapsDetail::CatmullRom(at(x1 - 1, y, c), at(x1, y, c), at(x1 + 1, y, c), at(x1 + 2, y, c), tx);
                    }
                    const float v = MapsDetail::CatmullRom(rows[0], rows[1], rows[2], rows[3], ty);
                    out[(static_cast<std::size_t>(oy) * static_cast<std::size_t>(ow) + static_cast<std::size_t>(ox)) * 4 +
                        static_cast<std::size_t>(c)] = static_cast<std::uint8_t>(std::clamp(std::lround(v), 0L, 255L));
                }
            }
        }
        });
        return out;
    }

    // The size the atlas is made finer to: twice each way, or as near as `limit` texels a side allows.
    [[nodiscard]] inline std::array<int, 2> UpscaledSize(int w, int h, int limit = 4096) noexcept
    {
        const float scale = std::min(2.0f, static_cast<float>(limit) / static_cast<float>(std::max(w, h)));
        return {std::min(limit, static_cast<int>(static_cast<float>(w) * scale)),
                std::min(limit, static_cast<int>(static_cast<float>(h) * scale))};
    }

    enum class SurfaceClass : int
    {
        Plain = 0,
        Metal = 1,
        Stone = 2,
        Wood = 3,
        Paint = 4,
    };

    struct SurfaceMaps
    {
        std::vector<std::uint8_t> normals; // RGBA8: the normal in the texture's space (x along u, y along v), 0..1
        std::vector<std::uint8_t> surface; // RGBA8: roughness, metalness, cavity, 255
        std::vector<SurfaceClass> classes; // per tile (row-major, `pitch` texels a tile)
    };

    // What a tile of texels is, from their saturation, brightness and busyness (the spread of brightness
    // between neighbours), its clear texels left out.
    [[nodiscard]] inline SurfaceClass ClassifyTile(const std::uint8_t* rgba, int w, int x0, int y0, int x1, int y1)
    {
        double saturation = 0.0, luma = 0.0, busy = 0.0, hue = 0.0;
        long count = 0;
        for (int y = y0; y + 1 < y1; ++y)
        {
            for (int x = x0; x + 1 < x1; ++x)
            {
                const std::uint8_t* t = rgba + (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4;
                if (t[3] < 128)
                {
                    continue;
                }
                const float r = t[0] / 255.0f, g = t[1] / 255.0f, b = t[2] / 255.0f;
                const float high = std::max({r, g, b});
                const float low = std::min({r, g, b});
                const float l = MapsDetail::Luma(t);
                saturation += high > 0.0f ? (high - low) / high : 0.0f;
                luma += l;
                const std::uint8_t* right = t + 4;
                const std::uint8_t* down = t + static_cast<std::size_t>(w) * 4;
                busy += std::fabs(MapsDetail::Luma(right) - l) + std::fabs(MapsDetail::Luma(down) - l);
                // Brown: red over green over blue.
                hue += (r > g && g > b && (r - b) > 0.12f) ? 1.0 : 0.0;
                ++count;
            }
        }
        if (count == 0)
        {
            return SurfaceClass::Plain;
        }
        const double n = static_cast<double>(count);
        const double s = saturation / n, l = luma / n, k = busy / n, brown = hue / n;
        // Metal only when plainly so (very grey, very even, bright): a stone variant taken for metal
        // lost its diffuse light and stood out dark among its neighbours.
        if (s < 0.08 && k < 0.03 && l > 0.45)
        {
            return SurfaceClass::Metal;
        }
        if (s < 0.25 && k >= 0.06)
        {
            return SurfaceClass::Stone;
        }
        if (brown > 0.6 && s > 0.25)
        {
            return SurfaceClass::Wood;
        }
        if (s >= 0.35 && k < 0.08)
        {
            return SurfaceClass::Paint;
        }
        return SurfaceClass::Plain;
    }

    // The atlas's maps, texel for texel with it (`w` x `h`, tiles every `pitch` texels).
    [[nodiscard]] inline SurfaceMaps BuildSurfaceMaps(const std::uint8_t* rgba, int w, int h, int pitch,
                                                      float strength = 3.0f)
    {
        SurfaceMaps maps;
        const std::size_t texels = static_cast<std::size_t>(w) * static_cast<std::size_t>(h);
        maps.normals.assign(texels * 4, 0);
        maps.surface.assign(texels * 4, 255);
        // The height: brightness, softened over 3x3 (the texels' noise is not relief).
        std::vector<float> luma(texels);
        for (std::size_t i = 0; i < texels; ++i)
        {
            luma[i] = MapsDetail::Luma(rgba + i * 4);
        }
        const auto L = [&](int x, int y)
        {
            return luma[static_cast<std::size_t>(std::clamp(y, 0, h - 1)) * static_cast<std::size_t>(w) +
                        static_cast<std::size_t>(std::clamp(x, 0, w - 1))];
        };
        std::vector<float> height(texels);
        MapsDetail::ParallelRows(h, [&](int firstRow, int lastRow) {
        for (int y = firstRow; y < lastRow; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                float sum = 0.0f;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx)
                        sum += L(x + dx, y + dy) * (dx == 0 && dy == 0 ? 4.0f : (dx == 0 || dy == 0) ? 2.0f : 1.0f);
                height[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] = sum / 16.0f;
            }
        }
        });
        const auto H = [&](int x, int y)
        {
            return height[static_cast<std::size_t>(std::clamp(y, 0, h - 1)) * static_cast<std::size_t>(w) +
                          static_cast<std::size_t>(std::clamp(x, 0, w - 1))];
        };
        // Each tile's class.
        const int columns = std::max(1, w / std::max(1, pitch));
        const int rows = std::max(1, h / std::max(1, pitch));
        maps.classes.resize(static_cast<std::size_t>(columns) * static_cast<std::size_t>(rows));
        for (int ty = 0; ty < rows; ++ty)
            for (int tx = 0; tx < columns; ++tx)
                maps.classes[static_cast<std::size_t>(ty * columns + tx)] =
                    ClassifyTile(rgba, w, tx * pitch, ty * pitch, std::min(w, (tx + 1) * pitch), std::min(h, (ty + 1) * pitch));
        // Roughness, metalness of each class (UltraLogic.hpp's surfaces: smoother is shinier).
        static constexpr float kRough[5] = {0.7f, 0.32f, 0.85f, 0.65f, 0.38f};
        static constexpr float kMetal[5] = {0.0f, 0.5f, 0.0f, 0.0f, 0.0f};
        MapsDetail::ParallelRows(h, [&](int firstRow, int lastRow) {
        for (int y = firstRow; y < lastRow; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                const std::size_t i = (static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)) * 4;
                // Sobel of the height: its slope along u (x) and v (y).
                const float du = (H(x + 1, y - 1) + 2.0f * H(x + 1, y) + H(x + 1, y + 1)) -
                                 (H(x - 1, y - 1) + 2.0f * H(x - 1, y) + H(x - 1, y + 1));
                const float dv = (H(x - 1, y + 1) + 2.0f * H(x, y + 1) + H(x + 1, y + 1)) -
                                 (H(x - 1, y - 1) + 2.0f * H(x, y - 1) + H(x + 1, y - 1));
                float nx = -du * strength;
                float ny = -dv * strength;
                float nz = 1.0f;
                const float length = std::sqrt(nx * nx + ny * ny + nz * nz);
                nx /= length;
                ny /= length;
                nz /= length;
                maps.normals[i + 0] = static_cast<std::uint8_t>(std::lround((nx * 0.5f + 0.5f) * 255.0f));
                maps.normals[i + 1] = static_cast<std::uint8_t>(std::lround((ny * 0.5f + 0.5f) * 255.0f));
                maps.normals[i + 2] = static_cast<std::uint8_t>(std::lround((nz * 0.5f + 0.5f) * 255.0f));
                maps.normals[i + 3] = 255;
                // Cavity: how far below the mean of a 5x5 round it the texel lies.
                float around = 0.0f;
                for (int dy = -2; dy <= 2; ++dy)
                    for (int dx = -2; dx <= 2; ++dx)
                        around += H(x + dx * 2, y + dy * 2);
                around /= 25.0f;
                const float hollow = std::clamp((around - H(x, y)) * 4.0f, 0.0f, 1.0f);
                const auto cls = static_cast<std::size_t>(
                    maps.classes[static_cast<std::size_t>(std::min(rows - 1, y / std::max(1, pitch)) * columns +
                                                          std::min(columns - 1, x / std::max(1, pitch)))]);
                const float rough = std::clamp(kRough[cls] + hollow * 0.2f, 0.05f, 1.0f);
                maps.surface[i + 0] = static_cast<std::uint8_t>(std::lround(rough * 255.0f));
                maps.surface[i + 1] = static_cast<std::uint8_t>(std::lround(kMetal[cls] * 255.0f));
                maps.surface[i + 2] = static_cast<std::uint8_t>(std::lround((1.0f - hollow * 0.6f) * 255.0f));
                maps.surface[i + 3] = 255;
            }
        }
        });
        return maps;
    }
}
