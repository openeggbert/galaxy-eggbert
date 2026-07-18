#pragma once

#include <string>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // One named group of BlockTypes icon ids for the editor's palette
    // (plan.md EDITOR-106).
    struct PaletteCategory
    {
        std::string name;
        std::vector<int> iconIds;
    };

    // Hand-curated categories built ONLY from BlockTypes.hpp's own already-
    // documented named constants/comments -- ~430 of the 441 object-m.png
    // icons have no confirmed semantic identity in this codebase, so
    // inventing category names for them would be unverified data
    // authoring, out of scope here. See AllBlockIconIdsInOrder() below for
    // full numeric coverage of every icon, identified or not.
    [[nodiscard]] std::vector<PaletteCategory> ConfirmedBlockCategories();

    // Every valid block type id (1..440; 0 is Air, never placeable), in
    // order -- the palette's "All Icons" fallback tab, guaranteeing full
    // coverage regardless of what is or isn't in ConfirmedBlockCategories().
    [[nodiscard]] std::vector<int> AllBlockIconIdsInOrder();
}
