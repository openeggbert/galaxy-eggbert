#pragma once

#include <filesystem>
#include <string>

namespace GalaxyEggbert::Editor
{
    // Moving a world file in and out of the game where the player cannot reach its files (ED-0012):
    // the web build keeps them in the browser's storage (IDBFS under /save), so an export is a
    // download and an import the browser's file picker. A platform persistence difference, like
    // CustomWorldsDir()'s path, not an engine one; elsewhere these do nothing and return false (the
    // files are in customworlds/, beside the game).

    // Offers @p file to the player as a download named @p name.
    bool OfferDownload(const std::filesystem::path& file, const std::string& name);

    // Opens the browser's file picker; a .vwr chosen is written into @p directory (ConsumeUploadDone
    // tells when; the picker gives no word when it is cancelled).
    bool RequestUpload(const std::filesystem::path& directory);

    // True once after RequestUpload's file was written.
    bool ConsumeUploadDone();
}
