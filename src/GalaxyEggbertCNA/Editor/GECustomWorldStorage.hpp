#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <filesystem>
#include <vector>

namespace GalaxyEggbert::CNA
{
    // Filesystem-backed storage for player-authored .vwr worlds (plan.md
    // EDITOR-107) -- deliberately NOT tracked in GESaveData (a persisted
    // list risks desyncing from disk on rename/delete); a plain directory
    // scan is simpler and can't drift. Worlds are scoped per existing
    // gamer slot (GESaveData::GetSelectedGamer(), 0-2), matching how a
    // player already picks their slot on the Init screen.

    // Directory holding @p gamerSlot's (0-2) custom worlds. Same platform-
    // path convention as GESaveData::kSavePath (a real Emscripten/IDBFS
    // persistence difference, not an engine-API one).
    [[nodiscard]] std::filesystem::path CustomWorldsDir(int gamerSlot);

    // Every *.vwr file directly inside CustomWorldsDir(gamerSlot), sorted
    // by filename. Empty if the directory doesn't exist yet (a gamer slot
    // that has never created a world).
    [[nodiscard]] std::vector<std::filesystem::path> ListCustomWorlds(int gamerSlot);

    // A fresh, not-yet-used path for a new world in @p gamerSlot's own
    // directory ("custom_NNN.vwr", first unused NNN starting at 1) --
    // creates the directory if it doesn't exist yet, but does NOT create
    // the file itself; the caller still saves a real World there.
    [[nodiscard]] std::filesystem::path NextNewWorldPath(int gamerSlot);

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
