#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace GalaxyEggbert::Game
{
    // The folder of the player's own files (the owner 2026-11-10, before a release): savedata.txt,
    // resume.txt, demos/, customworlds/ and screenshots/. The host chooses it at start (main.cpp,
    // ChooseUserFolder()): on the desktop the system's per-user folder (CNA's StorageDevice root:
    // %LOCALAPPDATA%\Galaxy Eggbert, $XDG_DATA_HOME or ~/.local/share/Galaxy Eggbert), or another
    // with --user DIR or GALAXY_USER_DIR (the test scripts: their run folder); on the web /save
    // (the IDBFS mount CNA's storage preRun hook persists); on Android the app's private storage,
    // the working folder. Until the host sets one, the working folder (the tools and tests).
    void SetUserFolder(std::filesystem::path folder);
    [[nodiscard]] const std::filesystem::path& UserFolder() noexcept;
    // @p relative inside the user folder ("savedata.txt", "demos").
    [[nodiscard]] std::filesystem::path UserPath(std::string_view relative);
    [[nodiscard]] std::string UserPathString(std::string_view relative);

    // The files a user folder holds, relative to it.
    inline constexpr std::string_view kUserFiles[] = {"savedata.txt", "resume.txt", "multiplayer.txt"};
    inline constexpr std::string_view kUserFolders[] = {"demos", "customworlds"};

    // Copies a player's files from @p from (the working folder, where every build before
    // 2026-11-10 kept them) into @p to when @p to has no savedata.txt and @p from has one: an
    // existing player keeps their progress, galaxies and recordings. Nothing is moved or
    // overwritten. Returns the number of files copied (0 when there was nothing to do).
    int CopyOldUserFiles(const std::filesystem::path& from, const std::filesystem::path& to);
}
