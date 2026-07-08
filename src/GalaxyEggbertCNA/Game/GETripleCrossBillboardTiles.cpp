#include "GETripleCrossBillboardTiles.hpp"

#include <algorithm>
#include <iterator>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Source: mobile-eggbert-reference/questionnaire-all-remaining-tiles.md,
        // every icon whose "Render mód?" answer is `TripleCrossBillboard`,
        // confirmed 2026-07-08.
        constexpr int kTripleCrossBillboardIcons[] = {53, 54, 55, 56, 57, 58, 59, 60, 63, 64};
    }

    bool IsTripleCrossBillboardIcon(int icon)
    {
        return std::find(std::begin(kTripleCrossBillboardIcons), std::end(kTripleCrossBillboardIcons), icon) !=
               std::end(kTripleCrossBillboardIcons);
    }
}
