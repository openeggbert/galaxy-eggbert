#pragma once

namespace GalaxyEggbert::CNA
{
    // When this build was made: "YYYY-MM-DD HH:MM", the build machine's local time, shown under the
    // version left of Play on the main menu (the owner, 2026-11-11). Defined in a source CMake
    // writes again whenever the game is linked with something new (cmake/BuildStamp.cmake), so a
    // build that changes nothing keeps it and does not relink.
    [[nodiscard]] const char* BuildStampText() noexcept;
}
