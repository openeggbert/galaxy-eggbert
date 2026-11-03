#pragma once

// Android only (OR-0011): the game reads its files by relative paths with std::ifstream and
// std::filesystem (worlds3d/, avatars3d/, Content/, the save file), which on Android see neither
// the APK's assets nor a writable working directory. Before the game starts, the host copies the
// packaged files into the app's internal storage and makes it the working directory, so that every
// relative path reads the copies and the save file and custom worlds are written beside them.
//
// The APK carries assets.txt (android/app/build.gradle's GalaxyAssetManifest): a stamp line, then
// every packaged file. The copy is kept until the stamp changes (a new build of the assets).

namespace GalaxyEggbert::CNA
{
    // Copies the assets when they are missing or stale and changes into internal storage. False
    // (logged) when the storage or the manifest cannot be read, or a file cannot be copied.
    bool PrepareAndroidAssets();

    // Sends the process's stdout and stderr (the game's and CNA's reports) to logcat under the tag
    // GalaxyEggbert, line by line, from a thread of its own; Android discards them otherwise.
    void RedirectStdioToLogcat();
}
