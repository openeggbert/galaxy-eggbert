#include "SaveData.hpp"
#include "UserFolder.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace GalaxyEggbert::Game
{
    std::string SaveData::SavePath() { return UserPathString("savedata.txt"); }

    void SaveData::Load()
    {
        std::ifstream in(SavePath());
        if (!in.is_open())
        {
            return;
        }
        Read(in);
    }

    void SaveData::Read(std::istream& in)
    {
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
            if (key == "fullscreen")
            {
                fullscreen_ = (value == "1");
            }
            else if (key == "soundEnabled")
            {
                soundEnabled_ = (value == "1");
            }
            else if (key == "jumpRight")
            {
                jumpRight_ = (value == "1");
            }
            else if (key == "autoZoom")
            {
                autoZoom_ = (value == "1");
            }
            else if (key == "accelActive")
            {
                accelActive_ = (value == "1");
            }
            else if (key == "effectsVolume")
            {
                effectsVolume_ = ClampVolume(std::atoi(value.c_str()));
            }
            else if (key == "musicVolume")
            {
                musicVolume_ = ClampVolume(std::atoi(value.c_str()));
            }
            else if (key == "terrainRelief")
            {
                terrainRelief_ = (value == "1");
            }
            else if (key == "effectsLevel")
            {
                SetEffectsLevel(std::atoi(value.c_str()));
            }
            else if (key == "language")
            {
                // A code the game knows, else none (the system's language).
                language_ = ParseLanguage(value) ? std::string(LanguageCode(*ParseLanguage(value))) : std::string();
            }
            else if (key == "frameRate")
            {
                SetFrameRate(std::atoi(value.c_str()));
            }
            else if (key == "textureScale")
            {
                SetTextureScale(std::atoi(value.c_str()));
            }
            else if (key == "effects3d")
            {
                // The first build's switch: on was everything (high).
                SetEffectsLevel(value == "1" ? 3 : 0);
            }
            else if (key == "accelSensitivity")
            {
                accelSensitivity_ = ClampVolume(std::atoi(value.c_str()));
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
            else if ((key.rfind("gamer", 0) == 0 && key.size() > 6 && key[6] == '.') ||
                     (key.rfind("galaxy", 0) == 0 && key.size() > 7 && key[7] == '.'))
            {
                // gamerN.field: the campaign's progress; galaxyN.field: the player's galaxy (BD-0119).
                const bool galaxy = key[2] == 'l'; // "galaxy", not "gamer"
                const std::size_t digit = galaxy ? 6 : 5;
                const int gamer = key[digit] - '0';
                if (gamer < 0 || gamer >= kGamerCount) continue;
                const std::string field = key.substr(digit + 2);
                if (!galaxy && field == "nickname")
                {
                    nicknames_[static_cast<std::size_t>(gamer)] = CleanNickname(value);
                    continue;
                }
                GamerSlot& slot = galaxy ? galaxies_[gamer] : gamers_[gamer];
                if (field == "lives")
                {
                    slot.lives = std::atoi(value.c_str());
                }
                else if (field == "missionNumber")
                {
                    slot.missionNumber = std::atoi(value.c_str());
                }
                else if (field == "lastWorld")
                {
                    slot.lastWorld = std::atoi(value.c_str());
                }
                else if (field == "hasProgress")
                {
                    slot.hasProgress = (value == "1");
                }
                else if (field == "doorsUnlocked")
                {
                    auto& doors = slot.doorsUnlocked;
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
        const std::string path = SavePath();
        std::ofstream out(path, std::ios::trunc);
        if (!out.is_open())
        {
            std::printf("SaveData: could not write %s\n", path.c_str());
            return;
        }
        Write(out);
    }

    void SaveData::Write(std::ostream& out) const
    {
        out << "soundEnabled=" << (soundEnabled_ ? "1" : "0") << "\n";
        out << "jumpRight=" << (jumpRight_ ? "1" : "0") << "\n";
        out << "autoZoom=" << (autoZoom_ ? "1" : "0") << "\n";
        out << "accelActive=" << (accelActive_ ? "1" : "0") << "\n";
        out << "effectsVolume=" << effectsVolume_ << "\n";
        out << "musicVolume=" << musicVolume_ << "\n";
        out << "accelSensitivity=" << accelSensitivity_ << "\n";
        out << "fullscreen=" << (fullscreen_ ? "1" : "0") << "\n";
        out << "terrainRelief=" << (terrainRelief_ ? "1" : "0") << "\n";
        out << "effectsLevel=" << effectsLevel_ << "\n";
        out << "frameRate=" << frameRate_ << "\n";
        if (!language_.empty()) out << "language=" << language_ << "\n";
        out << "textureScale=" << textureScale_ << "\n";
        out << "selectedGamer=" << selectedGamer_ << "\n";
        const auto writeSlot = [&out](const char* prefix, int gamer, const GamerSlot& slot)
        {
            out << prefix << gamer << ".lives=" << slot.lives << "\n";
            out << prefix << gamer << ".missionNumber=" << slot.missionNumber << "\n";
            out << prefix << gamer << ".lastWorld=" << slot.lastWorld << "\n";
            out << prefix << gamer << ".hasProgress=" << (slot.hasProgress ? "1" : "0") << "\n";
            out << prefix << gamer << ".doorsUnlocked=";
            for (bool unlocked : slot.doorsUnlocked)
            {
                out << (unlocked ? '1' : '0');
            }
            out << "\n";
        };
        for (int gamer = 0; gamer < kGamerCount; ++gamer)
        {
            writeSlot("gamer", gamer, gamers_[gamer]);
            if (!nicknames_[static_cast<std::size_t>(gamer)].empty())
                out << "gamer" << gamer << ".nickname=" << nicknames_[static_cast<std::size_t>(gamer)] << "\n";
        }
        // A galaxy never played leaves the file as it was before galaxies existed.
        for (int gamer = 0; gamer < kGamerCount; ++gamer)
        {
            if (!(galaxies_[gamer] == GamerSlot{})) writeSlot("galaxy", gamer, galaxies_[gamer]);
        }
    }
}
