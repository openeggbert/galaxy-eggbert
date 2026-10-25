#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <filesystem>
#include <vector>

namespace GalaxyEggbert::Editor
{
    // Filesystem-backed storage for player-authored .vwr worlds (plan.md
    // EDITOR-107) -- deliberately NOT tracked in SaveData (a persisted
    // list risks desyncing from disk on rename/delete); a plain directory
    // scan is simpler and can't drift. Worlds are scoped per existing
    // gamer slot (SaveData::GetSelectedGamer(), 0-2), matching how a
    // player already picks their slot on the Init screen.

    // Directory holding @p gamerSlot's (0-2) custom worlds. Same platform-
    // path convention as SaveData::kSavePath (CNA mounts that Emscripten/IDBFS
    // location before main; this is a persistence difference, not an engine-API one).
    [[nodiscard]] std::filesystem::path CustomWorldsDir(int gamerSlot);

    // Every *.vwr file directly inside CustomWorldsDir(gamerSlot), sorted
    // by filename. Empty if the directory doesn't exist yet (a gamer slot
    // that has never created a world).
    [[nodiscard]] std::vector<std::filesystem::path> ListCustomWorlds(int gamerSlot);

    // Each gamer has ten custom-world slots (owner, 2026-10-25). Slot n (1-10)
    // is "custom_00n.vwr" ("custom_010.vwr" for 10) in CustomWorldsDir(), the
    // names the first ten worlds already had, so existing worlds keep their
    // slots.
    constexpr int kCustomWorldSlotCount = 10;

    // Slot @p slot's (1-10) file, which may not exist yet.
    [[nodiscard]] std::filesystem::path SlotWorldPath(int gamerSlot, int slot);

    // Every other .vwr in CustomWorldsDir(): custom_011.vwr and up from the
    // time worlds were numbered to 999, and any other name. They are kept and
    // listed by the editor, never deleted or hidden; CopyIntoFreeSlot() brings
    // one into a slot (the original stays untouched).
    [[nodiscard]] std::vector<std::filesystem::path> ListOlderCustomWorlds(int gamerSlot);

    // The first free slot's path, creating the directory if needed but not the
    // file; empty when all ten slots hold a world.
    [[nodiscard]] std::filesystem::path NextNewWorldPath(int gamerSlot);

    // Copies @p source into the first free slot; returns the slot's path, or
    // empty when every slot is taken or the copy failed.
    [[nodiscard]] std::filesystem::path CopyIntoFreeSlot(int gamerSlot, const std::filesystem::path& source);

    // Seeds slot 1 with @p showcase (worlds3d/world999.vwr) the first time a
    // gamer opens the editor, and only if slot 1 is empty: a marker file
    // (".demo-seeded") records that it happened, so a deleted or edited slot 1
    // is never seeded again and nothing is overwritten. Returns true when it
    // copied the showcase.
    bool SeedShowcaseSlot(int gamerSlot, const std::filesystem::path& showcase);

    // The first scene a new Galaxy 3D editor world opens with. It is a
    // compact playable tutorial rather than an all-air volume: a 3x3 Rock
    // Pile board, treasure chest, exit arrow and a clear Blupi start at the
    // opposite edge. World files do not encode a spawn point, so callers
    // use EditorStarterSpawn* below for both editing and play-test.
    [[nodiscard]] GalaxyEggbert::Worlds::World CreateEditorStarterWorld();
    constexpr float kEditorStarterSpawnRenderX = 1.0f;
    constexpr float kEditorStarterSpawnY = 1.0f;
    constexpr float kEditorStarterSpawnRenderZ = 0.0f;
}
