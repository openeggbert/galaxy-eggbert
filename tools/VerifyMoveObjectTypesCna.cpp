#include "Game/GEWorldRuntime.hpp"

#include <iostream>
#include <string>
#include <vector>

// Scripted verification that GalaxyEggbert::CNA::GEWorldRuntime now parses
// MoveObject: lines (previously ignored entirely) using the same supported
// ObjectType allowlist as GESimple3D::GEWorldRuntime. Each (type, file) pair
// below is a real mobile-eggbert level file confirmed (by grepping
// ../mobile-eggbert/worlds/*.txt) to contain a MoveObject: line of that
// exact type, so this is grounded in real level data, not synthetic.
int main()
{
    struct Case { int type; const char* file; const char* name; };
    static const std::vector<Case> kCases = {
        {19, "../mobile-eggbert/worlds/world065.txt", "jeep"},
        {21, "../mobile-eggbert/worlds/world014.txt", "secret-level exit"},
        {24, "../mobile-eggbert/worlds/world025.txt", "skateboard"},
        {26, "../mobile-eggbert/worlds/world034.txt", "suction-cup"},
        {32, "../mobile-eggbert/worlds/world054.txt", "blupih"},
        {40, "../mobile-eggbert/worlds/world044.txt", "mirror/invert"},
        {44, "../mobile-eggbert/worlds/world055.txt", "wasp/bee"},
        {46, "../mobile-eggbert/worlds/world065.txt", "balloon"},
        {47, "../mobile-eggbert/worlds/world102.txt", "platform lift"},
        {54, "../mobile-eggbert/worlds/world062.txt", "large creature"},
        {55, "../mobile-eggbert/worlds/world025.txt", "dynamite"},
        {96, "../mobile-eggbert/worlds/world052.txt", "follower"},
    };

    bool allOk = true;
    for (const auto& c : kCases)
    {
        GalaxyEggbert::CNA::GEWorldRuntime runtime;
        if (!runtime.LoadFromMobileEggbertFile(c.file))
        {
            std::cout << "FAIL: could not load " << c.file << std::endl;
            allOk = false;
            continue;
        }

        bool found = false;
        for (const auto& obj : runtime.GetMobileObjects())
        {
            if (static_cast<int>(obj.type) == c.type)
            {
                found = true;
                break;
            }
        }

        std::cout << (found ? "PASS" : "FAIL") << ": type=" << c.type << " (" << c.name
                  << ") spawned from " << c.file << std::endl;
        if (!found)
        {
            allOk = false;
        }
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
