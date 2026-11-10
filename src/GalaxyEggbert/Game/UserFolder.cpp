#include "UserFolder.hpp"

#include <system_error>

namespace GalaxyEggbert::Game
{
    namespace
    {
        std::filesystem::path& Folder()
        {
#if defined(__EMSCRIPTEN__)
            static std::filesystem::path folder = "/save";
#else
            static std::filesystem::path folder;
#endif
            return folder;
        }
    }

    void SetUserFolder(std::filesystem::path folder) { Folder() = std::move(folder); }

    const std::filesystem::path& UserFolder() noexcept { return Folder(); }

    std::filesystem::path UserPath(std::string_view relative)
    {
        const std::filesystem::path& folder = Folder();
        return folder.empty() ? std::filesystem::path(relative) : folder / relative;
    }

    std::string UserPathString(std::string_view relative) { return UserPath(relative).string(); }

    int CopyOldUserFiles(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        namespace fs = std::filesystem;
        std::error_code error;
        if (!fs::is_regular_file(from / "savedata.txt", error) || fs::exists(to / "savedata.txt", error)) return 0;
        if (fs::equivalent(from, to, error)) return 0;
        fs::create_directories(to, error);
        int copied = 0;
        for (const std::string_view name : kUserFiles)
        {
            error.clear();
            if (fs::is_regular_file(from / name, error) &&
                fs::copy_file(from / name, to / name, fs::copy_options::skip_existing, error))
                ++copied;
        }
        for (const std::string_view name : kUserFolders)
        {
            error.clear();
            if (!fs::is_directory(from / name, error)) continue;
            fs::create_directories(to / name, error);
            for (fs::recursive_directory_iterator it(from / name, error), end; !error && it != end; it.increment(error))
            {
                std::error_code step;
                const fs::path target = to / name / it->path().lexically_relative(from / name);
                if (it->is_directory(step))
                    fs::create_directories(target, step);
                else if (it->is_regular_file(step) &&
                         fs::copy_file(it->path(), target, fs::copy_options::skip_existing, step))
                    ++copied;
            }
        }
        return copied;
    }
}
