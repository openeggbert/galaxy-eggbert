#include <GalaxyEggbert/Game/TerrainAnimDivisor.hpp>

#include <GalaxyEggbert/BlockTypes.hpp>

#include <iostream>

// Scripted verification of TerrainAnimDivisor's AnimDivisor() (plan.md
// TEST-007) -- locks in the real per-type ScaleDiv() divisor mapping
// against regression: Saw=1 (50ms/frame), Lava=2 (100ms/frame),
// Water1/Crusher/Water2/Marine/the 4 Fan icons=3 (150ms/frame),
// Spike/Temp=4 (200ms/frame). No graphics context needed -- AnimDivisor()
// is a pure function of a BlockTypes icon id.
int main()
{
    using namespace GalaxyEggbert::Game;
    using namespace GalaxyEggbert::BlockTypes;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    check(AnimDivisor(Saw) == 1, "Saw divisor is 1 (50ms/frame)");
    check(AnimDivisor(Lava) == 2, "Lava divisor is 2 (100ms/frame)");
    check(AnimDivisor(Water1) == 3, "Water1 divisor is 3 (150ms/frame)");
    check(AnimDivisor(Crusher) == 3, "Crusher divisor is 3 (150ms/frame)");
    check(AnimDivisor(Water2) == 3, "Water2 divisor is 3 (150ms/frame)");
    check(AnimDivisor(Marine) == 3, "Marine divisor is 3 (150ms/frame)");
    check(AnimDivisor(FanLeft) == 3, "FanLeft divisor is 3 (150ms/frame)");
    check(AnimDivisor(FanRight) == 3, "FanRight divisor is 3 (150ms/frame)");
    check(AnimDivisor(FanUp) == 3, "FanUp divisor is 3 (150ms/frame)");
    check(AnimDivisor(FanDown) == 3, "FanDown divisor is 3 (150ms/frame)");
    check(AnimDivisor(Spike) == 4, "Spike divisor is 4 (200ms/frame)");
    check(AnimDivisor(Temp) == 4, "Temp divisor is 4 (200ms/frame)");

    // Non-animated icons fall through to the default divisor (3) -- not a
    // real per-type value (nothing animates them), just AnimIcon()'s own
    // safe no-op divisor for its `phase` computation on icons it never
    // actually looks up a table for.
    check(AnimDivisor(RockPile) == 3, "Unrelated non-animated icon (RockPile) falls through to default divisor 3");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
