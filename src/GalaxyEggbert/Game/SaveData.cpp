#include "SaveData.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace GalaxyEggbert::Game
{
    void SaveData::Load()
    {
        std::ifstream in(kSavePath);
        if (!in.is_open())
        {
            return;
        }
        std::string line;
        while (std::getline(in, line))
        {
            const auto eq = line.find('=');
            if (eq == std::string::npos)
            {
                continue;
            }
            const std::string key = line.substr(0, eq);
            const std::string value = line.substr(eq + 1);
            if (key == "soundEnabled")
            {
                soundEnabled_ = (value == "1");
            }
            else if (key == "selectedGamer")
            {
                // Out-of-range guard (a hand-edited or corrupted save file
                // is the real, reachable path here -- every GetLives()/
                // SetLives()/etc. accessor below indexes gamers_[selectedGamer_]
                // with no bounds check of its own, so an unvalidated value
                // here is a real out-of-bounds array access later, not just
                // a logic bug). Same "reject, leave at whatever it already
                // was" shape as the sibling gamer.N.field parser just below.
                const int parsed = std::atoi(value.c_str());
                if (parsed >= 0 && parsed < kGamerCount)
                {
                    selectedGamer_ = parsed;
                }
            }
            else if (key.rfind("gamer", 0) == 0 && key.size() > 6 && key[6] == '.')
            {
                const int gamer = key[5] - '0';
                if (gamer < 0 || gamer >= kGamerCount) continue;
                const std::string field = key.substr(7);
                if (field == "lives")
                {
                    gamers_[gamer].lives = std::atoi(value.c_str());
                }
                else if (field == "missionNumber")
                {
                    gamers_[gamer].missionNumber = std::atoi(value.c_str());
                }
                else if (field == "hasProgress")
                {
                    gamers_[gamer].hasProgress = (value == "1");
                }
                else if (field == "doorsUnlocked")
                {
                    auto& doors = gamers_[gamer].doorsUnlocked;
                    for (std::size_t i = 0; i < doors.size() && i < value.size(); ++i)
                    {
                        doors[i] = (value[i] == '1');
                    }
                }
            }
        }
    }

    void SaveData::Save() const
    {
        std::ofstream out(kSavePath, std::ios::trunc);
        if (!out.is_open())
        {
            std::printf("SaveData: could not write %s\n", kSavePath);
            return;
        }
        out << "soundEnabled=" << (soundEnabled_ ? "1" : "0") << "\n";
        out << "selectedGamer=" << selectedGamer_ << "\n";
        for (int gamer = 0; gamer < kGamerCount; ++gamer)
        {
            out << "gamer" << gamer << ".lives=" << gamers_[gamer].lives << "\n";
            out << "gamer" << gamer << ".missionNumber=" << gamers_[gamer].missionNumber << "\n";
            out << "gamer" << gamer << ".hasProgress=" << (gamers_[gamer].hasProgress ? "1" : "0") << "\n";
            out << "gamer" << gamer << ".doorsUnlocked=";
            for (bool unlocked : gamers_[gamer].doorsUnlocked)
            {
                out << (unlocked ? '1' : '0');
            }
            out << "\n";
        }
    }
}
