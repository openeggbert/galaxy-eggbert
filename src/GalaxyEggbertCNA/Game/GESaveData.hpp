#pragma once

#include <string>

namespace GalaxyEggbert::CNA
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
    // wired to real behavior (`GESound::SetEnabled()`/`IsEnabled()`, the
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
    // (`GEInteractionSystem` reacts to door BLOCKS directly in the world,
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
    // screen/buttons themselves (see `GEInputPad::UpdateResume()`).
    //
    // Plain `key=value` text, not JSON/binary: no JSON library is linked
    // in this project, and for a handful of scalars a hand-rolled parser
    // is simpler than adding a new dependency for it.
    class GESaveData
    {
    public:
        // Reads Content-relative kSavePath if it exists; leaves every
        // field at its default (soundEnabled=true, lives=3,
        // missionNumber=0, hasProgress=false) if the file is missing or
        // unparsable.
        void Load();

        // Writes the current state to kSavePath, overwriting it.
        void Save() const;

        [[nodiscard]] bool GetSoundEnabled() const noexcept { return soundEnabled_; }
        void SetSoundEnabled(bool enabled) noexcept { soundEnabled_ = enabled; }

        [[nodiscard]] int GetLives() const noexcept { return lives_; }
        void SetLives(int lives) noexcept { lives_ = lives; }

        [[nodiscard]] int GetMissionNumber() const noexcept { return missionNumber_; }
        void SetMissionNumber(int missionNumber) noexcept { missionNumber_ = missionNumber; }

        [[nodiscard]] bool GetHasProgress() const noexcept { return hasProgress_; }
        void SetHasProgress(bool hasProgress) noexcept { hasProgress_ = hasProgress; }

    private:
        static constexpr const char* kSavePath = "savedata.txt";
        bool soundEnabled_ = true;
        int lives_ = 3; // matches GEInteractionSystem's own real GameData-derived default
        int missionNumber_ = 0;
        bool hasProgress_ = false;
    };
}
