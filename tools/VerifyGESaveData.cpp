#include <GalaxyEggbert/Game/SaveData.hpp>

#include <cstdio>
#include <fstream>
#include <iostream>

// Scripted, non-interactive verification of SaveData (2026-07-13,
// plan.md MENU-067) -- proves Load()/Save() actually round-trip
// soundEnabled correctly, not just "compiles and doesn't crash". Uses the
// class's own real save path (no override exists, by design -- see
// SaveData.hpp), so this tool cleans up after itself before and after
// running.
int main()
{
    using namespace GalaxyEggbert::Game;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    std::remove("savedata.txt");

    {
        SaveData data;
        data.Load();
        check(data.GetSoundEnabled(), "Load(): defaults soundEnabled to true when no save file exists");
        check(data.GetLives() == 3, "Load(): defaults lives to 3 when no save file exists");
        check(data.GetMissionNumber() == 0, "Load(): defaults missionNumber to 0 when no save file exists");
        check(!data.GetHasProgress(), "Load(): defaults hasProgress to false when no save file exists");
    }
    {
        SaveData data;
        data.SetSoundEnabled(false);
        data.Save();
        SaveData reloaded;
        reloaded.Load();
        check(!reloaded.GetSoundEnabled(), "Save()/Load(): round-trips soundEnabled=false correctly");
    }
    {
        SaveData data;
        data.SetSoundEnabled(true);
        data.Save();
        SaveData reloaded;
        reloaded.Load();
        check(reloaded.GetSoundEnabled(), "Save()/Load(): round-trips soundEnabled=true correctly");
    }
    {
        SaveData data;
        data.SetLives(1);
        data.SetMissionNumber(11);
        data.SetHasProgress(true);
        data.Save();
        SaveData reloaded;
        reloaded.Load();
        check(reloaded.GetLives() == 1 && reloaded.GetMissionNumber() == 11 && reloaded.GetHasProgress(),
              "Save()/Load(): round-trips lives/missionNumber/hasProgress correctly (Win/Lost checkpoint fields)");
    }
    {
        // Real per-sublevel door-unlock flags (plan.md hub/mission-
        // progression system, found 2026-07-17, `Decor::AdaptDoors()`'s
        // `m_doors[]`) -- default false, settable/persisted per mission
        // number directly.
        SaveData data;
        check(!data.IsMissionDoorUnlocked(12), "IsMissionDoorUnlocked() defaults to false for a fresh save");
        data.UnlockMissionDoor(12);
        data.UnlockMissionDoor(52);
        check(data.IsMissionDoorUnlocked(12) && data.IsMissionDoorUnlocked(52),
              "UnlockMissionDoor() sets the flag for the exact mission number given");
        check(!data.IsMissionDoorUnlocked(13), "UnlockMissionDoor(12) does not also unlock a neighboring mission");
        data.Save();
        SaveData reloaded;
        reloaded.Load();
        check(reloaded.IsMissionDoorUnlocked(12) && reloaded.IsMissionDoorUnlocked(52) &&
                  !reloaded.IsMissionDoorUnlocked(13),
              "Save()/Load(): round-trips the door-unlock bitset correctly");
    }

    // Phase 3 (2026-07-13, plan.md MENU-006..020): 3 independent gamer
    // slots, selected via GetSelectedGamer()/SetSelectedGamer(), each
    // with its own lives/missionNumber/hasProgress -- GetLives()/
    // SetLives()/etc. above operate on whichever slot is selected.
    {
        SaveData data;
        check(data.GetSelectedGamer() == 0, "Load(): defaults selectedGamer to 0 when no save file exists");
        check(data.GetLivesForGamer(1) == 3 && data.GetLivesForGamer(2) == 3,
              "Load(): every gamer slot defaults lives to 3 when no save file exists");
    }
    {
        SaveData data;
        data.SetSelectedGamer(1);
        data.SetLives(7);
        data.SetMissionNumber(4);
        data.SetHasProgress(true);
        check(data.GetLivesForGamer(0) == 3, "SetLives() on gamer 1 does not affect gamer 0's slot");
        data.Save();
        SaveData reloaded;
        reloaded.Load();
        check(reloaded.GetSelectedGamer() == 1, "Save()/Load(): round-trips selectedGamer correctly");
        reloaded.SetSelectedGamer(1);
        check(reloaded.GetLives() == 7 && reloaded.GetMissionNumber() == 4 && reloaded.GetHasProgress(),
              "Save()/Load(): round-trips gamer-1's own lives/missionNumber/hasProgress correctly");
        check(reloaded.GetLivesForGamer(0) == 3 && !reloaded.GetHasProgressForGamer(0),
              "Save()/Load(): gamer 0's slot is untouched by gamer 1's own data");
    }
    {
        SaveData data;
        data.SetSelectedGamer(2);
        data.SetLives(1);
        data.SetHasProgress(true);
        data.Reset();
        check(data.GetSelectedGamer() == 0 && data.GetLivesForGamer(2) == 3 && !data.GetHasProgressForGamer(2),
              "Reset(): restores selectedGamer and every gamer slot to its default (real gameData.Reset())");
    }

    // Out-of-range selectedGamer guard -- a real, reachable path is a
    // hand-edited or corrupted save file, not just a hypothetical: every
    // GetLives()/SetLives()/etc. accessor indexes gamers_[selectedGamer_]
    // with no bounds check of its own, so an unvalidated value here was a
    // genuine out-of-bounds array access on the very next accessor call,
    // not just a logic bug (found via a fresh code audit, 2026-07-23).
    {
        std::ofstream out("savedata.txt");
        out << "selectedGamer=99\n";
        out.close();
        SaveData data;
        data.Load();
        check(data.GetSelectedGamer() == 0,
              "Load() rejects an out-of-range selectedGamer (99) from a corrupted save file, "
              "keeping the safe default (0) instead of an out-of-bounds index");
        // Confirms the rejection genuinely leaves the object in a usable
        // state -- not just that the getter reports 0, but that indexing
        // through it (GetLives(), which the real Init/Resume/Win-Lost
        // code paths call unconditionally) doesn't touch anything outside
        // the real 3-slot array.
        check(data.GetLives() == 3, "GetLives() is still safe to call after rejecting an invalid selectedGamer");
    }
    {
        std::ofstream out("savedata.txt");
        out << "selectedGamer=-1\n";
        out.close();
        SaveData data;
        data.Load();
        check(data.GetSelectedGamer() == 0,
              "Load() rejects a negative selectedGamer (-1) the same way as an out-of-range positive one");
    }
    {
        // SetSelectedGamer() itself clamps too -- a public setter shouldn't
        // rely on every future caller only ever passing 0/1/2.
        SaveData data;
        data.SetSelectedGamer(99);
        check(data.GetSelectedGamer() == SaveData::kGamerCount - 1,
              "SetSelectedGamer() clamps an out-of-range value to the last real gamer slot");
        data.SetSelectedGamer(-5);
        check(data.GetSelectedGamer() == 0, "SetSelectedGamer() clamps a negative value to gamer slot 0");
    }

    std::remove("savedata.txt");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
