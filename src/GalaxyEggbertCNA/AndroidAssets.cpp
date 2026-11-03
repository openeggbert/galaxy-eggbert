#include "AndroidAssets.hpp"

// Platform wiring, Android only: XNA has no API that unpacks title files to a writable place, so
// this one file talks to SDL (CNA's platform layer) directly. "assets://" makes SDL read the APK's
// own copy even when internal storage already holds one.
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_system.h>

#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>
#include <system_error>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        constexpr const char* kManifest = "assets.txt";
        constexpr const char* kCopiedManifest = ".assets.txt"; // written last: the copy is whole

        bool ReadAsset(const std::string& name, std::string& out)
        {
            std::size_t size = 0;
            void* data = SDL_LoadFile(("assets://" + name).c_str(), &size);
            if (data == nullptr)
            {
                return false;
            }
            out.assign(static_cast<const char*>(data), size);
            SDL_free(data);
            return true;
        }

        bool WriteFile(const std::string& name, const std::string& data)
        {
            std::error_code error;
            const std::filesystem::path path(name);
            if (path.has_parent_path())
            {
                std::filesystem::create_directories(path.parent_path(), error);
            }
            const std::string part = name + ".part";
            {
                std::ofstream out(part, std::ios::binary | std::ios::trunc);
                out.write(data.data(), static_cast<std::streamsize>(data.size()));
                if (!out)
                {
                    return false;
                }
            }
            std::filesystem::rename(part, path, error);
            return !error;
        }
    }

    bool PrepareAndroidAssets()
    {
        const char* storage = SDL_GetAndroidInternalStoragePath();
        if (storage == nullptr)
        {
            SDL_Log("GalaxyEggbert: no internal storage: %s", SDL_GetError());
            return false;
        }
        std::error_code error;
        std::filesystem::current_path(storage, error);
        if (error)
        {
            SDL_Log("GalaxyEggbert: cannot work in %s: %s", storage, error.message().c_str());
            return false;
        }
        std::string manifest;
        if (!ReadAsset(kManifest, manifest))
        {
            SDL_Log("GalaxyEggbert: the APK has no %s: %s", kManifest, SDL_GetError());
            return false;
        }
        {
            std::ifstream in(kCopiedManifest, std::ios::binary);
            const std::string copied((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            if (copied == manifest)
            {
                return true;
            }
        }
        std::istringstream lines(manifest);
        std::string name;
        std::string data;
        int copied = 0;
        while (std::getline(lines, name))
        {
            if (name.empty() || name[0] == '#')
            {
                continue;
            }
            if (!ReadAsset(name, data) || !WriteFile(name, data))
            {
                SDL_Log("GalaxyEggbert: cannot copy the asset %s: %s", name.c_str(), SDL_GetError());
                return false;
            }
            ++copied;
        }
        if (!WriteFile(kCopiedManifest, manifest))
        {
            return false;
        }
        SDL_Log("GalaxyEggbert: copied %d assets into %s", copied, storage);
        return true;
    }
}
