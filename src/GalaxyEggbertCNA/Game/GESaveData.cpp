#include "GESaveData.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace GalaxyEggbert::CNA
{
    void GESaveData::Load()
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
            else if (key == "lives")
            {
                lives_ = std::atoi(value.c_str());
            }
            else if (key == "missionNumber")
            {
                missionNumber_ = std::atoi(value.c_str());
            }
            else if (key == "hasProgress")
            {
                hasProgress_ = (value == "1");
            }
        }
    }

    void GESaveData::Save() const
    {
        std::ofstream out(kSavePath, std::ios::trunc);
        if (!out.is_open())
        {
            std::printf("GESaveData: could not write %s\n", kSavePath);
            return;
        }
        out << "soundEnabled=" << (soundEnabled_ ? "1" : "0") << "\n";
        out << "lives=" << lives_ << "\n";
        out << "missionNumber=" << missionNumber_ << "\n";
        out << "hasProgress=" << (hasProgress_ ? "1" : "0") << "\n";
    }
}
