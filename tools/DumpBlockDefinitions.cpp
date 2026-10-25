// Writes BlockDefinitionRegistry as CSV for the Python world tools (tools/worldkit.py's level
// checks): per object-m.png icon its render mode, collision mask, hazard and special roles.
// With --check <csv> it compares instead and fails when the file is stale, so the committed
// tools/data/blocks.csv cannot drift from the C++ table (CTest).
//
//   DumpBlockDefinitions > tools/data/blocks.csv
//   DumpBlockDefinitions --check tools/data/blocks.csv

#include <GalaxyEggbert/BlockDefinitionRegistry.hpp>
#include <GalaxyEggbert/BlockTypes.hpp>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
    std::string Csv()
    {
        using namespace GalaxyEggbert;
        std::ostringstream out;
        out << "icon,render,collision_mask,solid,water,teleporter,fan,spring,bridge,door_key,hazard,"
               "world_select,progress_door,animated\n";
        for (std::uint16_t icon = 0; icon <= BlockDefinitionRegistry::MaxObjectMIcon; ++icon)
        {
            const auto& d = GetBlockDefinition(icon);
            // Standing-surface solidity as BlupiController::IsPointSolid() decides it without
            // sub-cell precision: any block but air, teleporters, fans and water.
            const bool solid = icon != 0 && !BlockTypes::isTeleporter(icon) && !BlockTypes::isFan(icon) &&
                               !BlockTypes::isWater(icon);
            out << icon << ',' << static_cast<int>(d.renderMode) << ',' << d.collisionMask << ',' << solid << ','
                << d.water << ',' << BlockTypes::isTeleporter(icon) << ',' << BlockTypes::isFan(icon) << ','
                << d.spring << ',' << d.bridge << ',' << static_cast<int>(d.doorKeyType) << ','
                << static_cast<int>(d.hazard) << ',' << static_cast<int>(d.worldSelectIndex) << ','
                << static_cast<int>(d.progressDoorIndex) << ',' << d.animation.IsAnimated() << '\n';
        }
        return out.str();
    }
}

int main(int argc, char** argv)
{
    const std::string csv = Csv();
    if (argc == 3 && std::string(argv[1]) == "--check")
    {
        std::ifstream in(argv[2]);
        std::stringstream file;
        file << in.rdbuf();
        if (file.str() != csv)
        {
            std::cout << "FAIL: " << argv[2] << " is stale; regenerate it with DumpBlockDefinitions" << std::endl;
            return 1;
        }
        std::cout << "PASS: " << argv[2] << " matches BlockDefinitionRegistry" << std::endl;
        return 0;
    }
    std::cout << csv;
    return 0;
}
