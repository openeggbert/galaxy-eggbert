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

    std::remove("savedata.txt");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
