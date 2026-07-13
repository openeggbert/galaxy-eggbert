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
    // Scope is deliberately minimal (phase 1): only `soundEnabled`, the
    // one setting already wired to real behavior (`GESound::
    // SetEnabled()`/`IsEnabled()`, the SetupSounds toggle). Real
    // `GameData::Write()` fires two ways -- automatically at Win/Lost
    // (lives/doors) and manually right after every Setup toggle press
    // (confirmed via `Game1.cpp`'s real `SetupSounds` handler: `gameData.
    // setSoundActiveProperty(...); gameData.Write();`) -- this class's
    // own `Save()` is called the same way, from the SetupSounds toggle
    // handler, not on a timer or at every frame. Lives/mission fields
    // would follow the same automatic-Win/Lost-checkpoint pattern if
    // added later, once a real multi-level flow exists to check-point.
    //
    // Plain `key=value` text, not JSON/binary: no JSON library is linked
    // in this project, and for one bool a hand-rolled parser is simpler
    // than adding a new dependency for it.
    class GESaveData
    {
    public:
        // Reads Content-relative kSavePath if it exists; leaves
        // soundEnabled_ at its default (true) if the file is missing or
        // unparsable, matching GESound's own default-enabled state.
        void Load();

        // Writes the current state to kSavePath, overwriting it.
        void Save() const;

        [[nodiscard]] bool GetSoundEnabled() const noexcept { return soundEnabled_; }
        void SetSoundEnabled(bool enabled) noexcept { soundEnabled_ = enabled; }

    private:
        static constexpr const char* kSavePath = "savedata.txt";
        bool soundEnabled_ = true;
    };
}
