#pragma once

#include <filesystem>
#include <string>

namespace GalaxyEggbert::Game
{
    // A file name in @p dir that is not taken yet (2026-11-09: a second export or import no longer
    // replaces the first): stem + extension, else stem-2, stem-3 ... + extension.
    [[nodiscard]] inline std::filesystem::path FreeFileName(const std::filesystem::path& dir, const std::string& stem,
                                                            const std::string& extension)
    {
        std::error_code error;
        std::filesystem::path path = dir / (stem + extension);
        for (int n = 2; std::filesystem::exists(path, error) && n < 10000; ++n)
            path = dir / (stem + "-" + std::to_string(n) + extension);
        return path;
    }
}
