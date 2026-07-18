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

    // Hand-curated MoveObject categories for the palette's Objects mode
    // (plan.md EDITOR-109), built ONLY from ObjectType.hpp's own already-
    // documented comment groups -- same "don't invent semantics" rule as
    // ConfirmedBlockCategories() above.
    //
    // Deliberately covers only DIRECTLY PLACEABLE types: lifts, enemies,
    // collectibles, pickups, Blupi skins. ObjectType.hpp's own
    // "Explosions and visual effects", "Water / goo effects",
    // "Projectiles", and "Moving level objects" groups are transient
    // effects the game spawns itself (auto-expiring animations, fired
    // projectiles, door/bridge animations), not things a level author
    // anchors to a cell, and its "Unidentified / reserved" group has no
    // confirmed behavior at all. All of them stay reachable through
    // AllObjectTypeIdsInOrder() below.
    [[nodiscard]] std::vector<PaletteCategory> ConfirmedObjectCategories();

    // Every declared ObjectType id (1..203; ObjectType0 is the null/
    // inactive slot, never placeable), in order -- the Objects mode's
    // "All Types" fallback tab, the exact counterpart of
    // AllBlockIconIdsInOrder().
    [[nodiscard]] std::vector<int> AllObjectTypeIdsInOrder();
}
