#include "TextureScale.hpp"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <unordered_map>

namespace GalaxyEggbert::Game
{
    namespace
    {
        int g_setting = 0;
        int g_wanted = 1;
        std::mutex g_mutex;
        std::unordered_map<const void*, int>& Scales()
        {
            static std::unordered_map<const void*, int> scales;
            return scales;
        }

        std::string Replace(const std::string& path, const char* from, const char* to)
        {
            const std::size_t at = path.find(from);
            if (at == std::string::npos)
            {
                return {};
            }
            return path.substr(0, at) + to + path.substr(at + std::strlen(from));
        }
    }

    void SetTextureScaleSetting(int setting) noexcept
    {
        g_setting = setting == 1 || setting == 2 ? setting : 0;
    }

    int TextureScaleSetting() noexcept
    {
        return g_setting;
    }

    int ResolveTextureScale(int displayHeight, bool nativeWorld) noexcept
    {
        int setting = g_setting;
        if (const char* env = std::getenv("GALAXY_TEXTURES"); env != nullptr && *env != '\0')
        {
            const std::string v(env);
            setting = v == "2" || v == "2x" ? 2 : v == "1" || v == "1x" ? 1 : 0;
        }
#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
        // The web and Android packages carry only the 1x sheets.
        (void)displayHeight;
        (void)nativeWorld;
        g_wanted = 1;
        (void)setting;
#else
        g_wanted = setting == 0 ? (nativeWorld && displayHeight >= 1000 ? 2 : 1) : setting;
#endif
        return g_wanted;
    }

    int WantedTextureScale() noexcept
    {
        return g_wanted;
    }

    bool PngSize(const std::string& path, int& width, int& height)
    {
        std::ifstream in(path, std::ios::binary);
        unsigned char header[24] = {};
        if (!in.read(reinterpret_cast<char*>(header), sizeof(header)))
        {
            return false;
        }
        static const unsigned char kSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
        if (std::memcmp(header, kSignature, 8) != 0 || std::memcmp(header + 12, "IHDR", 4) != 0)
        {
            return false;
        }
        width = (header[16] << 24) | (header[17] << 16) | (header[18] << 8) | header[19];
        height = (header[20] << 24) | (header[21] << 16) | (header[22] << 8) | header[23];
        return width > 0 && height > 0;
    }

    std::string ScaledContentPath(const std::string& path, int* scale)
    {
        if (scale != nullptr)
        {
            *scale = 1;
        }
        if (g_wanted != 2)
        {
            return path;
        }
        std::string twice = Replace(path, "Content/icons/", "Content/icons2x/");
        if (twice.empty())
        {
            twice = Replace(path, "Content/backgrounds/", "Content/backgrounds2x/");
        }
        int w1 = 0, h1 = 0, w2 = 0, h2 = 0;
        if (twice.empty() || !PngSize(path, w1, h1) || !PngSize(twice, w2, h2))
        {
            return path;
        }
        if (w2 != 2 * w1 || h2 != 2 * h1)
        {
            std::cout << "GalaxyEggbertCNA: warning: " << twice << " is " << w2 << "x" << h2 << ", not twice "
                      << path << " (" << w1 << "x" << h1 << "); using the 1x texture." << std::endl;
            return path;
        }
        if (scale != nullptr)
        {
            *scale = 2;
        }
        return twice;
    }

    void SetTextureScale(const void* texture, int scale)
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (scale <= 1)
        {
            Scales().erase(texture);
        }
        else
        {
            Scales()[texture] = scale;
        }
    }

    int TextureScaleOf(const void* texture) noexcept
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        const auto it = Scales().find(texture);
        return it == Scales().end() ? 1 : it->second;
    }
}
