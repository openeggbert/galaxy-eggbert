#pragma once

#include <GalaxyEggbert/Worlds/World.hpp>

#include <map>

#include <filesystem>
#include <optional>
#include <utility>
#include <string>
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

    // Backups (ED-0011; the owner's choice of 2026-11-06 among the editor proposals): the first save
    // of a world after it was opened from the browser keeps the file as it was, three generations
    // deep, in CustomWorldsDir()/backups/ (custom_003.1.vwr the newest, .3 the oldest); a slot
    // deleted in the browser keeps its world the same way, so a deletion can be taken back.
    constexpr int kBackupGenerations = 3;

    // Generation @p generation's (1-3) backup of @p world, which may not exist.
    [[nodiscard]] std::filesystem::path BackupPath(const std::filesystem::path& world, int generation);

    // How many backups @p world has (0-3).
    [[nodiscard]] int BackupCount(const std::filesystem::path& world);

    // Keeps @p world as it is now as the newest backup, the older ones one generation further, the
    // oldest dropped. False when there is no file to keep.
    bool BackUpWorld(const std::filesystem::path& world);

    // The same, the backups kept in @p backupDir (world011.1.vwr ...) instead of beside the world:
    // a shipped world edited with xnjttjpo (BD-0118) keeps its backups in CampaignBackupsDir().
    bool BackUpWorldTo(const std::filesystem::path& world, const std::filesystem::path& backupDir);

    // The folder holding every gamer's folder ("customworlds", on the web /save/customworlds).
    [[nodiscard]] std::filesystem::path CustomWorldsRoot();

    // Where the shipped worlds edited in play (xnjttjpo) keep their backups: the three versions
    // before the edits of each visit, as a slot's backups (the shipped folder stays clean).
    [[nodiscard]] std::filesystem::path CampaignBackupsDir();

    // Where the web build keeps the shipped worlds edited with xnjttjpo (2026-11-09): there the
    // shipped worlds are the page's packed data, in memory, so a save of one is also copied here
    // (in the browser's storage), and ApplyCampaignEdits() lays the copies over the shipped worlds
    // at every start. The desktop edits its worlds3d/ files themselves and keeps nothing here.
    [[nodiscard]] std::filesystem::path CampaignEditsDir();
    // Copies every worldNNN.vwr of CampaignEditsDir() over @p worldsDir's file of that name; how
    // many it copied.
    int ApplyCampaignEdits(const std::filesystem::path& worldsDir);

    // Restores the newest backup of @p world: it takes the world's place, the older ones move up a
    // generation and the world as it was becomes the oldest -- so restoring again steps further
    // back and comes round to where it began; nothing is lost. A deleted slot (no world) takes its
    // newest backup. The world's thumbnail goes (it showed the other one). False when there is no
    // backup.
    bool RestoreBackup(const std::filesystem::path& world);

    // Deletes a slot's world into its backups (and its thumbnail).
    bool DeleteSlotWorld(const std::filesystem::path& world);

    // The world's thumbnail beside it (custom_003.png): the editor's view when it was last saved.
    [[nodiscard]] std::filesystem::path ThumbnailPath(const std::filesystem::path& world);

    // Worlds shared between the players and with other computers (ED-0012): customworlds/shared/
    // (on the web /save/customworlds/shared). An export lands there (on the web it is downloaded
    // too, PlatformFiles.hpp); every .vwr there is listed beside the gamer's older worlds, and a
    // click copies it into a free slot, the file staying.
    [[nodiscard]] std::filesystem::path SharedWorldsDir();

    // Its .vwr files, the newest first.
    [[nodiscard]] std::vector<std::filesystem::path> ListSharedWorlds();

    // The name a slot's export takes: "A-world-3.vwr" (players A-C), "player7-world-3.vwr" otherwise.
    [[nodiscard]] std::string ExportName(int gamerSlot, int slot);

    // Copies slot @p slot's world into SharedWorldsDir() as ExportName() (an earlier export of it
    // replaced); its path, or empty when the slot has no world or the copy failed.
    [[nodiscard]] std::filesystem::path ExportWorld(int gamerSlot, int slot);

    // Seeds slot 1 with @p showcase (worlds3d/world999.vwr) the first time a
    // gamer opens the editor, and only if slot 1 is empty: a marker file
    // (".demo-seeded") records that it happened, so a deleted or edited slot 1
    // is never seeded again and nothing is overwritten. Returns true when it
    // copied the showcase.
    bool SeedShowcaseSlot(int gamerSlot, const std::filesystem::path& showcase);

    // The gallery (worlds3d/world998.vwr: every block, object and BigDecor in one place; owner
    // 2026-11-02, OR-0044) in the gamer's first free slot, once; afterwards the copy follows the
    // shipped gallery for as long as it is left unedited (the marker `.gallery-seeded` keeps the
    // slot and the hash of what was copied). Returns true when it wrote the slot.
    bool SeedGallerySlot(int gamerSlot, const std::filesystem::path& gallery);

    // ---- The player's own galaxy (BD-0119) ----
    // Each gamer builds a whole campaign of their own: a main hub (mission 1) whose world signs
    // 1-14 lead to world hubs (missions 10, 20, ... 140), whose mission signs 1-8 lead to the
    // missions (11-18, ... 141-148), and the bonus world (199) after the main hub's exit -- the
    // campaign's own numbering, so the hubs, doors, gold bars and signs work as the campaign's.
    // The files are CustomWorldsDir()/galaxy/world%03d.vwr; its progress is SaveData's galaxy slot.
    constexpr int kGalaxyWorldCount = 14;      // the main hub's world signs (BlockTypes WorldSelect1-14)
    constexpr int kGalaxyMissionsPerWorld = 8; // a world hub's mission signs (MissionSign1-8)
    constexpr int kGalaxyBonusMission = 199;

    [[nodiscard]] std::filesystem::path GalaxyDir(int gamerSlot);
    // Mission @p mission's file in the gamer's galaxy (which may not exist yet).
    [[nodiscard]] std::filesystem::path GalaxyWorldPath(int gamerSlot, int mission);
    // Whether @p mission is a place of a galaxy: 1, 10-140 by tens, their missions 1-8, 199.
    [[nodiscard]] bool IsGalaxyMission(int mission) noexcept;
    // The gamer and mission of a galaxy world's path as GalaxyWorldPath() spells it (generic
    // form), nothing for any other path: a demo names its world by this path.
    [[nodiscard]] std::optional<std::pair<int, int>> ParseGalaxyWorldPath(const std::string& path);
    // "Main hub", "World 3", "World 3, mission 2", "Bonus world".
    [[nodiscard]] std::string GalaxyMissionName(int mission);
    // A new galaxy world to start from: the starter board, with world sign 1 for the main hub,
    // mission sign 1 and the return sign for a world hub, the chest and exit for a mission; Blupi's
    // start set in the file.
    [[nodiscard]] GalaxyEggbert::Worlds::World CreateGalaxyStarterWorld(int mission);

    // ---- A whole galaxy in one file (2026-11-09) ----
    // "GALAXY_PACK 1\n", then each place as "world <mission> <size>\n" and its .vwr bytes, then the
    // player's demo recordings made in the galaxy as "demo <position> <size>\n" and the .gdemo's
    // bytes, then "END <FNV-1a of everything before it, 16 hex digits>\n". Shared as a world is
    // (ED-0012): an export lands in SharedWorldsDir() (and is downloaded on the web), an import is
    // read from there (the web's file picker writes it there).
    constexpr const char* kGalaxyPackExtension = ".galaxy";
    struct GalaxyPack
    {
        std::map<int, std::string> worlds; // by mission
        std::map<int, std::string> demos;  // by the demo position they were recorded into
    };
    // The demo positions (1-10) holding a recording of the player's made in @p gamerSlot's galaxy
    // (DemoReplay UserDemoPath(), its world one of the galaxy's files).
    [[nodiscard]] std::vector<int> GalaxyRecordings(int gamerSlot);
    // The gamer's galaxy packed, with GalaxyRecordings(); empty when it has no place yet.
    [[nodiscard]] std::string PackGalaxy(int gamerSlot);
    // A pack's worlds and demos; nothing when it is damaged or truncated, of another version, holds
    // a place or a position twice, a place that is not a galaxy's, or a world that does not load. A
    // demo this game cannot play (another simulation revision) is left out, the galaxy kept.
    [[nodiscard]] std::optional<GalaxyPack> UnpackGalaxy(const std::string& pack);
    // "A-galaxy.galaxy" (players A-C), "player7-galaxy.galaxy" otherwise.
    [[nodiscard]] std::string GalaxyExportName(int gamerSlot);
    // PackGalaxy() written into SharedWorldsDir() as GalaxyExportName() (an earlier one replaced);
    // its path, or empty when there is nothing to export or the write failed.
    [[nodiscard]] std::filesystem::path ExportGalaxy(int gamerSlot);
    // The packs in SharedWorldsDir(), the newest first.
    [[nodiscard]] std::vector<std::filesystem::path> ListSharedGalaxies();
    // Makes @p pack the gamer's galaxy: every place the galaxy has now goes into its backups
    // first (DeleteSlotWorld(), so Backups brings it back), then the pack's worlds are written. Its
    // demos go to the shared demos (DemoReplay SharedDemoDir(), "<pack name>-demo-<position>.gdemo"),
    // listed by the demo positions' Import: no position of the player's is overwritten. The number
    // of places imported (@p demos: of demos); -1 when the pack is not valid, nothing touched.
    int ImportGalaxy(int gamerSlot, const std::filesystem::path& pack, int* demos = nullptr);

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
