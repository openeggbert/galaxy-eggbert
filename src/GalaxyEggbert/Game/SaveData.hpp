#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <iosfwd>

namespace GalaxyEggbert::Game
{
    // Minimal settings-persistence for GalaxyEggbertCNA (2026-07-13,
    // plan.md MENU-067), a small engine-appropriate substitute for the
    // real mobile-eggbert `GameData` -- NOT byte-compatible with it and
    // deliberately so: the real format is a fixed 640-byte blob (3 gamer
    // slots x lives/last-world/200 door flags + global sound/jump/zoom/
    // accel settings) written via WP7's `IsolatedStorageFile`, an API
    // with no desktop equivalent, and shaped entirely around a 100+level/
    // 3-gamer-slot structure this engine doesn't have (a single
    // hand-authored .vwr world, no gamer slots) -- no real save file
    // could ever cross between the two engines, so matching that layout
    // would buy nothing (confirmed via direct research into
    // GameData.hpp/.cpp before implementing this).
    //
    // Phase 1 scope (2026-07-13): `soundEnabled`, the one setting already
    // wired to real behavior (`Sound::SetEnabled()`/`IsEnabled()`, the
    // SetupSounds toggle). Real `GameData::Write()` fires two ways --
    // automatically at Win/Lost (lives/doors) and manually right after
    // every Setup toggle press (confirmed via `Game1.cpp`'s real
    // `SetupSounds` handler: `gameData.setSoundActiveProperty(...);
    // gameData.Write();`) -- this class's own `Save()` is called the same
    // way, from the SetupSounds toggle handler, not on a timer or at
    // every frame.
    //
    // Phase 2 (2026-07-13, plan.md MENU-040..045, real Resume phase):
    // `lives`/`missionNumber`/`hasProgress`, checkpointed the same real
    // way -- `GalaxyEggbertGame` calls `SetLives()`/`SetMissionNumber()`/
    // `SetHasProgress(true)` + `Save()` right at the real Win/Lost
    // transition (matching the real `MemorizeGamerProgress()` call sites
    // confirmed via research), NOT continuously. Real door-state
    // checkpointing (`GameData`'s 200 main/secondary door flags) is NOT
    // ported -- this engine has no equivalent per-door tracking array
    // (`InteractionSystem` reacts to door BLOCKS directly in the world,
    // not a bespoke persisted array), a documented gap.
    //
    // ResumeLog separately records an interrupted mission (Game1.cpp:186-208,
    // Decor.cpp:10974-11300). hasProgress remains the checkpoint fallback when that log
    // is missing or incompatible. Read/Write also preserve the settings at a replay's start.
    //
    // Plain `key=value` text, not JSON/binary: no JSON library is linked
    // in this project, and for a handful of scalars a hand-rolled parser
    // is simpler than adding a new dependency for it.
    //
    // Phase 3 (2026-07-13, plan.md MENU-006..020, real Init/gamer-select
    // menu): extended to real `GameData`'s own 3-independent-gamer-slot
    // shape (confirmed via research: `data[2]` selectedGamer byte + 3x
    // per-gamer lives/lastWorld) -- `lives_`/`missionNumber_`/
    // `hasProgress_` above are now per-slot (`GamerSlot`), `soundEnabled_`
    // stays a single global field (matches the real header's global
    // sound/jump/zoom/accel bytes, which are NOT per-gamer). The
    // pre-existing single-slot accessors (`GetLives()`/`SetLives()`/etc.)
    // are UNCHANGED in signature -- they now implicitly operate on
    // `gamers_[selectedGamer_]`, so every existing call site (Win/Lost
    // checkpoint, Resume restore, Cheat4/5) keeps working unmodified.
    // Real per-gamer 200 door-flags are still NOT ported (documented gap,
    // unchanged from Phase 2) -- the Init menu's "Main gates"/"Secondary
    // gates" text lines are rendered with a static "0/12"/"0/52" (matches
    // the real STRING exactly, since the real label denominators are
    // themselves hardcoded/cosmetic and don't match the real door-array
    // capacity either -- confirmed via research), not real tracked data.
    struct GamerSlot
    {
        int lives = 3; // matches InteractionSystem's own real GameData-derived default
        int missionNumber = 0;
        // The world last played (GameData's lastWorld, 1-based): the main hub starts Blupi
        // beside its sign (Decor::MainSwitchInitialize()).
        int lastWorld = 1;
        bool hasProgress = false;
        // Real per-sublevel door-unlock flags (plan.md hub/mission-progression
        // system, found 2026-07-17, `Decor::AdaptDoors()`'s `m_doors[]` --
        // the per-world golds `m_doors[180..199]` too: HubDoors.cpp raises the
        // main hub's golden posts for them, as AdaptDoors). Sized to `Decor::m_doors[200]`'s own real length so
        // every real mission number (max 199) has a slot; index == mission
        // number directly, matching real `m_doors[mission]` semantics 1:1 --
        // no separate per-world re-indexing.
        std::array<bool, 200> doorsUnlocked{};
    };

    class SaveData
    {
    public:
        static constexpr int kGamerCount = 3; // real GameData::TotalLength = 10 + 3*210

        // Reads Content-relative kSavePath if it exists; leaves every
        // field at its default (soundEnabled=true, selectedGamer=0, every
        // gamer slot lives=3/missionNumber=0/hasProgress=false) if the
        // file is missing or unparsable.
        void Load();
        void Read(std::istream& in);
        void Write(std::ostream& out) const;

        // Writes the current state to kSavePath, overwriting it.
        void Save() const;

        [[nodiscard]] bool GetFullscreen() const noexcept { return fullscreen_; }
        void SetFullscreen(bool fullscreen) noexcept { fullscreen_ = fullscreen; }
        [[nodiscard]] bool GetSoundEnabled() const noexcept { return soundEnabled_; }
        void SetSoundEnabled(bool enabled) noexcept { soundEnabled_ = enabled; }
        // The Setup's other toggles (GameData data[4..6]; GameData::Initialize: jump right and
        // automatic zoom on, the accelerometer off). OR-0010.
        [[nodiscard]] bool GetJumpRight() const noexcept { return jumpRight_; }
        void SetJumpRight(bool on) noexcept { jumpRight_ = on; }
        [[nodiscard]] bool GetAutoZoom() const noexcept { return autoZoom_; }
        void SetAutoZoom(bool on) noexcept { autoZoom_ = on; }
        [[nodiscard]] bool GetAccelActive() const noexcept { return accelActive_; }
        void SetAccelActive(bool on) noexcept { accelActive_ = on; }

        // The Setup's two volume sliders (a Galaxy addition, the owner's request of 2026-11-02;
        // the original has Sounds on/off only): the effects' and the music's volume, 0..100,
        // 100 being the original's loudness. Saved as `effectsVolume=` / `musicVolume=`.
        [[nodiscard]] int GetEffectsVolume() const noexcept { return effectsVolume_; }
        void SetEffectsVolume(int volume) noexcept { effectsVolume_ = ClampVolume(volume); }
        // The accelerometer's sensitivity (GameData data[7]: 0..100, 50 by default; the Setup's slider
        // under the Accelerometer toggle, shown while it is on, InputPad.cpp:250, 932, 1172).
        [[nodiscard]] int GetAccelSensitivity() const noexcept { return accelSensitivity_; }
        void SetAccelSensitivity(int value) noexcept { accelSensitivity_ = ClampVolume(value); }
        // The terrain drawn in 3D relief near the camera (the Ray Dream tiles' lumps, Q-001/Q-004) or
        // as flat textured cubes: a Galaxy setting (the owner's request of 2026-11-04: the reliefs cost
        // memory and GPU time a phone may not have). Off by default everywhere (the owner, 2026-11-04);
        // the Setup's "3D terrain" turns it on. Saved as `terrainRelief=`.
        [[nodiscard]] bool GetTerrainRelief() const noexcept { return terrainRelief_; }
        void SetTerrainRelief(bool on) noexcept { terrainRelief_ = on; }
        static constexpr bool kTerrainReliefDefault = false;
        [[nodiscard]] int GetMusicVolume() const noexcept { return musicVolume_; }
        void SetMusicVolume(int volume) noexcept { musicVolume_ = ClampVolume(volume); }
        [[nodiscard]] static int ClampVolume(int volume) noexcept { return volume < 0 ? 0 : volume > 100 ? 100 : volume; }

        // Real `data[2]`/`SetGamer()` (`Game1.cpp`): a single tap on an
        // Init gamer-slot button immediately selects AND persists that
        // slot, independent of entering Play.
        [[nodiscard]] int GetSelectedGamer() const noexcept { return selectedGamer_; }
        // Clamped, not just assigned: every GetLives()/SetLives()/etc.
        // accessor below indexes gamers_[selectedGamer_] with no bounds
        // check of its own, so this is the one place that must keep
        // selectedGamer_ always valid regardless of what a caller passes.
        // The one real call site today (GalaxyEggbertGame.cpp, the
        // Init gamer-select buttons) only ever passes 0/1/2, but a public
        // setter shouldn't rely on that staying true forever.
        void SetSelectedGamer(int gamer) noexcept
        {
            selectedGamer_ = std::clamp(gamer, 0, kGamerCount - 1);
        }

        // Read-only per-slot accessors for the Init menu's 3-button
        // display (does NOT change which slot GetLives()/SetLives()/etc.
        // below operate on).
        [[nodiscard]] int GetLivesForGamer(int gamer) const noexcept { return gamers_[gamer].lives; }
        [[nodiscard]] bool GetHasProgressForGamer(int gamer) const noexcept { return gamers_[gamer].hasProgress; }
        // Real GameData::GetGamerInfo(): secondary gates are the opened doors
        // 0-179, main gates 180-199.
        [[nodiscard]] int GetSecondaryGatesForGamer(int gamer) const noexcept
        {
            const auto& doors = gamers_[gamer].doorsUnlocked;
            return static_cast<int>(std::count(doors.begin(), doors.begin() + 180, true));
        }
        [[nodiscard]] int GetMainGatesForGamer(int gamer) const noexcept
        {
            const auto& doors = gamers_[gamer].doorsUnlocked;
            return static_cast<int>(std::count(doors.begin() + 180, doors.end(), true));
        }

        [[nodiscard]] int GetLives() const noexcept { return gamers_[selectedGamer_].lives; }
        void SetLives(int lives) noexcept { gamers_[selectedGamer_].lives = lives; }

        [[nodiscard]] int GetMissionNumber() const noexcept { return gamers_[selectedGamer_].missionNumber; }
        void SetMissionNumber(int missionNumber) noexcept { gamers_[selectedGamer_].missionNumber = missionNumber; }

        [[nodiscard]] int GetLastWorld() const noexcept { return gamers_[selectedGamer_].lastWorld; }
        void SetLastWorld(int world) noexcept { gamers_[selectedGamer_].lastWorld = world; }

        [[nodiscard]] bool GetHasProgress() const noexcept { return gamers_[selectedGamer_].hasProgress; }
        void SetHasProgress(bool hasProgress) noexcept { gamers_[selectedGamer_].hasProgress = hasProgress; }

        // Real per-sublevel door-unlock state (see `GamerSlot::doorsUnlocked`'s
        // own comment) -- `mission` indexes directly, same convention as
        // real `m_doors[mission]`. Out-of-range missions are a silent no-op/
        // false (defensive; every real mission number fits comfortably).
        [[nodiscard]] bool IsMissionDoorUnlocked(int mission) const noexcept
        {
            const auto& doors = gamers_[selectedGamer_].doorsUnlocked;
            return mission >= 0 && mission < static_cast<int>(doors.size()) && doors[static_cast<std::size_t>(mission)];
        }
        void UnlockMissionDoor(int mission) noexcept
        {
            auto& doors = gamers_[selectedGamer_].doorsUnlocked;
            if (mission >= 0 && mission < static_cast<int>(doors.size()))
            {
                doors[static_cast<std::size_t>(mission)] = true;
            }
        }

        // Real Cheat5 ("R"): `gameData.Reset()` (2026-07-13, plan.md
        // `CHEAT-005`) -- restores every field (all 3 gamer slots
        // included, matching the real `Reset()`) to its default and
        // writes immediately, matching the real source's own
        // `Reset(); Write();` pair (`Game1.cpp`'s real `SetupReset`/cheat
        // handler).
        void Reset() noexcept { *this = SaveData(); }

    private:
        // Web build (2026-07-17): under /save so it lands on the IDBFS mount
        // point CNA's storage preRun hook sets up (persists across page reloads via IndexedDB) --
        // a platform path difference, not an engine-API one, so it doesn't
        // fall under CLAUDE.md's "no #ifdef for engine differences" rule.
#if defined(__EMSCRIPTEN__)
        static constexpr const char* kSavePath = "/save/savedata.txt";
#else
        static constexpr const char* kSavePath = "savedata.txt";
#endif
        bool fullscreen_ = false; // desktop preference, shared by all gamer slots
        bool soundEnabled_ = true;
        bool jumpRight_ = true;
        bool autoZoom_ = true;
        bool accelActive_ = false;
        int effectsVolume_ = 100;
        int musicVolume_ = 100;
        int accelSensitivity_ = 50;
        bool terrainRelief_ = kTerrainReliefDefault;
        int selectedGamer_ = 0;
        GamerSlot gamers_[kGamerCount];
    };
}
