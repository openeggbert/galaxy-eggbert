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
        // The original Free Eggbert editor uses button.png for both the
        // permanent rail and the opened row. Keep those source icon ids
        // alongside Galaxy's selectable block/object ids so the 3D editor
        // preserves the original menu's visual order.
        int buttonIconId = 0;
        // A zero entry means that the matching button.png glyph has no
        // verified Galaxy implementation yet.  It deliberately remains in
        // the menu (in the original Eggbert 2 position); clicking it shows
        // the temporary "Not yet implemented." notice instead of silently
        // selecting an unrelated 3D item.
        std::vector<int> iconIds;
        // Per-entry MoveObject counterpart. Exactly one of iconIds[i]
        // (voxel block) and objectTypeIds[i] (placed object) is normally
        // non-zero. This lets the faithful Eggbert 2 menu mix terrain,
        // hazards, collectibles, enemies and vehicles without a hidden
        // global Blocks/Objects toggle.
        std::vector<int> objectTypeIds;
        std::vector<int> buttonIconIds;
        // Non-empty only for Galaxy Eggbert's extra Background group.
        // Values are the actual World::skyRegion ids selected by the
        // matching cells; zero is valid here.
        std::vector<int> skyRegionIds;
        // Optional per-entry object-m.png icon override. Zero keeps the
        // normal ObjectType-driven icon. A negative value means "capture
        // the currently selected terrain icon" and is used by Eggbert 2's
        // Secret wooden case: the crate is camouflaged as the current
        // terrain selection instead of looking like an ordinary crate.
        std::vector<int> objectVisualIconIds;
        // Non-zero marks the matching source glyph as the editor's
        // world-level Blupi start-position tool.
        std::vector<int> spawnPointIds;
        // Populated by EDITOR-126 for source BigDecor placements.
        std::vector<int> bigDecorIconIds;
    };

    // Hand-curated categories built ONLY from BlockTypes.hpp's own already-
    // documented named constants/comments -- ~430 of the 441 object-m.png
    // icons have no confirmed semantic identity in this codebase, so
    // inventing category names for them would be unverified data
    // authoring, out of scope here. See AllBlockIconIdsInOrder() below for
    // full numeric coverage of every icon, identified or not.
    [[nodiscard]] std::vector<PaletteCategory> ConfirmedBlockCategories();

    // Galaxy-only group appended after the ten Eggbert 2 source groups.
    // Its representative remains a button.png glyph, while its contents
    // are the 32 valid sky-region choices rendered as live thumbnails.
    [[nodiscard]] PaletteCategory GalaxyBackgroundCategory();

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
