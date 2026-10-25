// The terrain light (src/GalaxyEggbert/Game/TerrainLighting.hpp) must keep top faces at the
// tile's own colours and set every side face visibly apart from them, so edges and steps read.
#include <GalaxyEggbert/Game/TerrainLighting.hpp>

#include <iostream>

int main()
{
    using GalaxyEggbert::Game::TerrainFaceBrightness;
    using GalaxyEggbert::Game::kTerrainLight;

    bool allOk = true;
    const auto check = [&allOk](bool condition, const char* what)
    {
        std::cout << (condition ? "PASS" : "FAIL") << ": " << what << std::endl;
        allOk = allOk && condition;
    };

    const float top = TerrainFaceBrightness(0.0f, 1.0f, 0.0f);
    const float bottom = TerrainFaceBrightness(0.0f, -1.0f, 0.0f);
    const float sides[4] = {
        TerrainFaceBrightness(1.0f, 0.0f, 0.0f), TerrainFaceBrightness(-1.0f, 0.0f, 0.0f),
        TerrainFaceBrightness(0.0f, 0.0f, 1.0f), TerrainFaceBrightness(0.0f, 0.0f, -1.0f)};

    check(top > 0.97f && top <= 1.0f, "Top faces keep the tile's own colours (brightness ~1, never above)");
    bool sidesApart = true;
    for (const float side : sides)
    {
        sidesApart = sidesApart && side >= kTerrainLight.ambient && top - side >= 0.15f;
    }
    check(sidesApart, "Every side face is at least 0.15 darker than the top, never below ambient");
    check(sides[1] > sides[0] && sides[2] > sides[3],
          "The faces turned to the light (-X, +Z: the camera's left and front) are the brighter sides");
    check(bottom == kTerrainLight.ambient, "Bottom faces get ambient only");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
