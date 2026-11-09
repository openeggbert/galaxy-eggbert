// VerifyUltra: what the 3D effects' Ultra level computes on the CPU (UltraLogic.hpp), without a GPU.

#include <GalaxyEggbert/Game/UltraLogic.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace
{
    using Microsoft::Xna::Framework::Vector3;

    int failures = 0;

    void Check(bool ok, const char* what)
    {
        std::printf("%s %s\n", ok ? "ok  " : "FAIL", what);
        if (!ok)
        {
            ++failures;
        }
    }

    // The table's entry for the corner (r, g, b) of the 32-cube.
    const std::uint8_t* Entry(const std::vector<std::uint8_t>& table, int r, int g, int b)
    {
        constexpr int n = GalaxyEggbert::Game::kGradeTableSize;
        return &table[(static_cast<std::size_t>(g) * (n * n) + static_cast<std::size_t>(b) * n + static_cast<std::size_t>(r)) * 4];
    }
}

int main()
{
    using GalaxyEggbert::Game::AcesFitted;
    using GalaxyEggbert::Game::BuildGradeTable;
    constexpr int n = GalaxyEggbert::Game::kGradeTableSize;

    // The tonemapping: white at the usual exposure (1.35) comes out near white once divided by 0.79, mid
    // grey where it was (display 0.46 -> 0.46), and the curve never decreases.
    const auto display = [](float linear) { return std::pow(std::min(AcesFitted(linear * 1.35f) / 0.79f, 1.0f), 1.0f / 2.2f); };
    Check(std::fabs(display(1.0f) - 0.952f) < 0.01f, "white at the usual exposure comes out at 0.95");
    Check(std::fabs(display(std::pow(0.46f, 2.2f)) - 0.46f) < 0.03f, "mid grey stays mid grey");
    bool rising = true;
    for (float v = 0.0f; v < 16.0f; v += 0.01f)
    {
        rising = rising && AcesFitted(v + 0.01f) >= AcesFitted(v);
    }
    Check(rising && AcesFitted(0.0f) == 0.0f && AcesFitted(1000.0f) <= 1.0f, "the curve rises from 0 and stays under 1");

    // The grade: black stays black, white near white, greys near grey, and it keeps every channel's order.
    const auto table = BuildGradeTable(Vector3(0.3f, 0.5f, 0.9f));
    Check(table.size() == static_cast<std::size_t>(n * n * n * 4), "32 x 32 x 32 entries of four bytes");
    const auto* black = Entry(table, 0, 0, 0);
    Check(black[0] <= 2 && black[1] <= 2 && black[2] <= 2 && black[3] == 255, "black stays black");
    const auto* white = Entry(table, n - 1, n - 1, n - 1);
    Check(white[0] >= 240 && white[1] >= 240 && white[2] >= 240, "white stays near white");
    bool greys = true;
    bool ordered = true;
    for (int i = 0; i < n; ++i)
    {
        const auto* grey = Entry(table, i, i, i);
        greys = greys && std::abs(grey[0] - grey[1]) <= 12 && std::abs(grey[2] - grey[1]) <= 12;
        if (i > 0)
        {
            const auto* below = Entry(table, i - 1, 10, 20);
            const auto* here = Entry(table, i, 10, 20);
            ordered = ordered && here[0] >= below[0];
        }
    }
    Check(greys, "greys stay near grey (a light tint only)");
    Check(ordered, "more red in stays more red out");
    const auto dull = BuildGradeTable(Vector3(0.0f, 0.0f, 0.0f));
    Check(Entry(dull, 16, 16, 16)[0] != 0, "a black sky grades without a tint");

    std::printf("%s\n", failures == 0 ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED");
    return failures == 0 ? 0 : 1;
}
