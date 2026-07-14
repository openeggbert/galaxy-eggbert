#include <GalaxyEggbert/BlockTypes.hpp>

#include <iostream>

// Scripted verification of BlockTypes::tileUV() (plan.md TEST-004) --
// confirms every icon in the real confirmed-valid range (0..439) maps to
// an atlas rect that actually fits inside object-m.png's real bounds, and
// locks in the known icon-440 atlas-bounds gap found 2026-07-14 (see
// plan.md TEST-004/NEXT.md's own writeup) as an explicit, tracked
// exception rather than a silent one -- if a future fix makes icon 440
// valid too, this test will need updating, which is the point: it should
// not stay silently green forever on a stale assumption.
int main()
{
    using namespace GalaxyEggbert::BlockTypes;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    constexpr float kEpsilon = 1e-4f;
    const auto fitsInAtlas = [](int icon)
    {
        float uOff = 0.0f, vOff = 0.0f, uScale = 0.0f, vScale = 0.0f;
        tileUV(icon, uOff, vOff, uScale, vScale);
        return uOff >= 0.0f && vOff >= 0.0f && uOff + uScale <= 1.0f + kEpsilon &&
               vOff + vScale <= 1.0f + kEpsilon;
    };

    int firstBadIcon = -1;
    for (int icon = 0; icon <= 439; ++icon)
    {
        if (!fitsInAtlas(icon))
        {
            firstBadIcon = icon;
            break;
        }
    }
    check(firstBadIcon == -1,
          "Every icon 0..439 maps to an atlas rect that fits inside object-m.png's real bounds");
    if (firstBadIcon != -1)
    {
        std::cout << "  first bad icon: " << firstBadIcon << std::endl;
    }

    // Known, tracked exception (plan.md TEST-004, 2026-07-14): icon 440's
    // computed rect is entirely outside the real 1301x1431 image -- see
    // GenerateSampleWorld3D.cpp's own exhibition-loop comment for why this
    // is excluded there instead of guessing a fix in tileUV()/kPassable
    // itself.
    check(!fitsInAtlas(440), "Icon 440 is the known, confirmed exception (does NOT fit in the atlas)");

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
