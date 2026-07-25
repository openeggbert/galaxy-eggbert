#include "Game/GEWorldRuntime.hpp"
#include "Game/GEObjectVerticalPlacement.hpp"

#include <algorithm>
#include <filesystem>
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
        bool grounded = false;
        for (const auto& obj : runtime.GetMobileObjects())
        {
            if (static_cast<int>(obj.type) == c.type)
            {
                found = true;
                grounded =
                    obj.posStartY == GalaxyEggbert::CNA::kGroundObjectCenterY &&
                    obj.posEndY == GalaxyEggbert::CNA::kGroundObjectCenterY &&
                    obj.currentY == GalaxyEggbert::CNA::kGroundObjectCenterY;
                break;
            }
        }

        std::cout << (found ? "PASS" : "FAIL") << ": type=" << c.type << " (" << c.name
                  << ") spawned from " << c.file << std::endl;
        if (!found)
        {
            allOk = false;
        }
        std::cout << (grounded ? "PASS" : "FAIL")
                  << ": imported 2D object occupies the Y=1 ground cell" << std::endl;
        if (!grounded)
        {
            allOk = false;
        }
    }

    // plan.md TEST-003 (2026-07-14): the checks above only cover a curated
    // subset chosen per ObjectType example -- this sweeps EVERY real
    // mobile-eggbert world file and confirms LoadFromMobileEggbertFile()
    // doesn't fail on any of them, closing the literal "all world files
    // parse without error" ask. Enumerated at runtime (not a hardcoded
    // list) so it stays accurate if ../mobile-eggbert/worlds/ ever gains
    // or loses files.
    {
        const std::filesystem::path worldsDir = "../mobile-eggbert/worlds";
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::directory_iterator(worldsDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".txt")
            {
                files.push_back(entry.path().string());
            }
        }
        std::sort(files.begin(), files.end());

        if (files.empty())
        {
            std::cout << "FAIL: no world files found under " << worldsDir << std::endl;
            allOk = false;
        }

        int failCount = 0;
        for (const auto& file : files)
        {
            GalaxyEggbert::CNA::GEWorldRuntime runtime;
            if (!runtime.LoadFromMobileEggbertFile(file))
            {
                std::cout << "FAIL: could not parse " << file << std::endl;
                ++failCount;
                allOk = false;
            }
        }
        std::cout << (failCount == 0 ? "PASS" : "FAIL") << ": all " << files.size()
                  << " real mobile-eggbert world files parse without error (" << failCount
                  << " failed)" << std::endl;
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
