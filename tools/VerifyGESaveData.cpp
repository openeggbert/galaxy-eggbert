#include "Game/GESaveData.hpp"

#include <cstdio>
#include <iostream>

// Scripted, non-interactive verification of GESaveData (2026-07-13,
// plan.md MENU-067) -- proves Load()/Save() actually round-trip
// soundEnabled correctly, not just "compiles and doesn't crash". Uses the
// class's own real save path (no override exists, by design -- see
// GESaveData.hpp), so this tool cleans up after itself before and after
// running.
int main()
{
    using namespace GalaxyEggbert::CNA;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    std::remove("savedata.txt");

    {
        GESaveData data;
        data.Load();
        check(data.GetSoundEnabled(), "Load(): defaults soundEnabled to true when no save file exists");
        check(data.GetLives() == 3, "Load(): defaults lives to 3 when no save file exists");
        check(data.GetMissionNumber() == 0, "Load(): defaults missionNumber to 0 when no save file exists");
        check(!data.GetHasProgress(), "Load(): defaults hasProgress to false when no save file exists");
    }
    {
        GESaveData data;
        data.SetSoundEnabled(false);
        data.Save();
        GESaveData reloaded;
        reloaded.Load();
        check(!reloaded.GetSoundEnabled(), "Save()/Load(): round-trips soundEnabled=false correctly");
    }
    {
        GESaveData data;
        data.SetSoundEnabled(true);
        data.Save();
        GESaveData reloaded;
        reloaded.Load();
        check(reloaded.GetSoundEnabled(), "Save()/Load(): round-trips soundEnabled=true correctly");
    }
    {
        GESaveData data;
        data.SetLives(1);
        data.SetMissionNumber(11);
        data.SetHasProgress(true);
        data.Save();
        GESaveData reloaded;
        reloaded.Load();
        check(reloaded.GetLives() == 1 && reloaded.GetMissionNumber() == 11 && reloaded.GetHasProgress(),
              "Save()/Load(): round-trips lives/missionNumber/hasProgress correctly (Win/Lost checkpoint fields)");
    }

    // Phase 3 (2026-07-13, plan.md MENU-006..020): 3 independent gamer
    // slots, selected via GetSelectedGamer()/SetSelectedGamer(), each
    // with its own lives/missionNumber/hasProgress -- GetLives()/
    // SetLives()/etc. above operate on whichever slot is selected.
    {
        GESaveData data;
        check(data.GetSelectedGamer() == 0, "Load(): defaults selectedGamer to 0 when no save file exists");
        check(data.GetLivesForGamer(1) == 3 && data.GetLivesForGamer(2) == 3,
              "Load(): every gamer slot defaults lives to 3 when no save file exists");
    }
    {
        GESaveData data;
        data.SetSelectedGamer(1);
        data.SetLives(7);
        data.SetMissionNumber(4);
        data.SetHasProgress(true);
        check(data.GetLivesForGamer(0) == 3, "SetLives() on gamer 1 does not affect gamer 0's slot");
        data.Save();
        GESaveData reloaded;
        reloaded.Load();
        check(reloaded.GetSelectedGamer() == 1, "Save()/Load(): round-trips selectedGamer correctly");
        reloaded.SetSelectedGamer(1);
        check(reloaded.GetLives() == 7 && reloaded.GetMissionNumber() == 4 && reloaded.GetHasProgress(),
              "Save()/Load(): round-trips gamer-1's own lives/missionNumber/hasProgress correctly");
        check(reloaded.GetLivesForGamer(0) == 3 && !reloaded.GetHasProgressForGamer(0),
              "Save()/Load(): gamer 0's slot is untouched by gamer 1's own data");
    }
    {
        GESaveData data;
        data.SetSelectedGamer(2);
        data.SetLives(1);
        data.SetHasProgress(true);
        data.Reset();
        check(data.GetSelectedGamer() == 0 && data.GetLivesForGamer(2) == 3 && !data.GetHasProgressForGamer(2),
              "Reset(): restores selectedGamer and every gamer slot to its default (real gameData.Reset())");
    }

    std::remove("savedata.txt");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
