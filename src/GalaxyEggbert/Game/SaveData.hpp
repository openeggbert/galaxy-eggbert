#pragma once

#include <algorithm>
#include <array>
#include <string>

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
    // way -- `GalaxyEggbertCnaGame` calls `SetLives()`/`SetMissionNumber()`/
    // `SetHasProgress(true)` + `Save()` right at the real Win/Lost
    // transition (matching the real `MemorizeGamerProgress()` call sites
    // confirmed via research), NOT continuously. Real door-state
    // checkpointing (`GameData`'s 200 main/secondary door flags) is NOT
    // ported -- this engine has no equivalent per-door tracking array
    // (`InteractionSystem` reacts to door BLOCKS directly in the world,
    // not a bespoke persisted array), a documented gap.
    //
    // `hasProgress` adapts the real trigger for entering Resume
    // (`Game1::OnActivated()`, a real WP7 app-reactivation OS lifecycle
    // event with no desktop equivalent, gated on a real serialized
    // mid-level `Decor::Current*()` snapshot existing -- a SEPARATE,
    // heavier save mechanism than `GameData` itself, and far beyond this
    // engine's own single-`.vwr`-world scope to replicate faithfully).
    // The honest, minimal adaptation used here: Resume is offered at
    // startup whenever a previous run reached Win/Lost at least once
    // (`hasProgress_==true`), not on any real OS-level reactivation
    // signal -- a documented simplification of the trigger, not the
    // screen/buttons themselves (see `InputPad::UpdateResume()`).
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
        bool hasProgress = false;
        // Real per-sublevel door-unlock flags (plan.md hub/mission-progression
        // system, found 2026-07-17, `Decor::AdaptDoors()`'s `m_doors[]` --
        // ONLY the per-sublevel half is ported; the per-world cosmetic-gold
        // half (`m_doors[180..199]`) is still not modeled, see SAVE-006/007's
        // own note). Sized to `Decor::m_doors[200]`'s own real length so
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

        // Writes the current state to kSavePath, overwriting it.
        void Save() const;

        [[nodiscard]] bool GetSoundEnabled() const noexcept { return soundEnabled_; }
        void SetSoundEnabled(bool enabled) noexcept { soundEnabled_ = enabled; }

        // Real `data[2]`/`SetGamer()` (`Game1.cpp`): a single tap on an
        // Init gamer-slot button immediately selects AND persists that
        // slot, independent of entering Play.
        [[nodiscard]] int GetSelectedGamer() const noexcept { return selectedGamer_; }
        // Clamped, not just assigned: every GetLives()/SetLives()/etc.
        // accessor below indexes gamers_[selectedGamer_] with no bounds
        // check of its own, so this is the one place that must keep
        // selectedGamer_ always valid regardless of what a caller passes.
        // The one real call site today (GalaxyEggbertCnaGame.cpp, the
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

        [[nodiscard]] int GetLives() const noexcept { return gamers_[selectedGamer_].lives; }
        void SetLives(int lives) noexcept { gamers_[selectedGamer_].lives = lives; }

        [[nodiscard]] int GetMissionNumber() const noexcept { return gamers_[selectedGamer_].missionNumber; }
        void SetMissionNumber(int missionNumber) noexcept { gamers_[selectedGamer_].missionNumber = missionNumber; }

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
        // point pre.js sets up (persists across page reloads via IndexedDB) --
        // a platform path difference, not an engine-API one, so it doesn't
        // fall under CLAUDE.md's "no #ifdef for engine differences" rule.
#if defined(__EMSCRIPTEN__)
        static constexpr const char* kSavePath = "/save/savedata.txt";
#else
        static constexpr const char* kSavePath = "savedata.txt";
#endif
        bool soundEnabled_ = true;
        int selectedGamer_ = 0;
        GamerSlot gamers_[kGamerCount];
    };
}
