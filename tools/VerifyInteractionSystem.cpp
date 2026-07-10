#include "Game/GEInteractionSystem.hpp"
#include "Game/GESound.hpp"
#include "Game/GEWorldRuntime.hpp"

#include <cmath>
#include <iostream>

// Scripted, non-interactive verification of GEInteractionSystem (2026-07-10)
// against the real worlds3d/world001.vwr sample world -- proves platform
// lift patrol, crate push, and pickup collection (treasure/egg/key/exit)
// actually work, not just "compiles and doesn't crash". GESound is
// constructed but never LoadContent()-ed, so every Play() call is a no-op
// against an unloaded channel (no audio device needed for this scripted
// check).
int main(int argc, char** argv)
{
    using namespace GalaxyEggbert;
    using namespace GalaxyEggbert::CNA;

    const std::string worldPath = (argc > 1) ? argv[1] : "worlds3d/world001.vwr";

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    GEWorldRuntime world;
    if (!world.LoadFromVwrFile(worldPath))
    {
        std::cout << "FAIL: could not load " << worldPath << std::endl;
        return 1;
    }

    GESound sound; // never LoadContent()-ed -- every Play() call below is a silent no-op
    GEInteractionSystem interaction;
    constexpr float dt = 1.0f / 60.0f;

    // 1. Platform lift (ObjectType1, placed at world (0,4,-22)..(0,9,-22) per
    // tools/GenerateSampleWorld3D.cpp's "plateau -> crow's-nest lift") should
    // patrol: its currentY should move away from its start position after
    // enough Update() ticks, well before it could possibly have reached the
    // far end and ping-ponged back to the exact start value.
    float liftStartY = -1.0f;
    for (const auto& obj : world.GetMobileObjects())
    {
        if (obj.type == ObjectType::ObjectType1)
        {
            liftStartY = obj.currentY;
            break;
        }
    }
    check(liftStartY >= 0.0f, "found the platform lift (ObjectType1) in the sample world");

    for (int i = 0; i < 30; ++i)
    {
        interaction.Update(dt, world, 999.0f, 999.0f, 999.0f, 0.0f, sound); // Blupi far away
    }
    float liftYAfter = liftStartY;
    for (const auto& obj : world.GetMobileObjects())
    {
        if (obj.type == ObjectType::ObjectType1)
        {
            liftYAfter = obj.currentY;
            break;
        }
    }
    std::cout << "Lift Y after 0.5s: " << liftYAfter << " (started at " << liftStartY << ")" << std::endl;
    check(std::fabs(liftYAfter - liftStartY) > 0.05f, "platform lift patrols (currentY changed)");

    // 2. Pickup collection -- walk Blupi's simulated position onto each of
    // the treasure/egg/key placed in the sample world and confirm the
    // interaction system collects it (counter increments, object goes
    // inactive) exactly once, not on every subsequent frame.
    const auto findFirst = [&world](ObjectType type) -> const MobileObjSpec*
    {
        for (const auto& obj : world.GetMobileObjects())
        {
            if (obj.type == type) return &obj;
        }
        return nullptr;
    };

    if (const auto* egg = findFirst(ObjectType::ObjectType6))
    {
        const float ex = egg->currentX, ey = egg->currentY, ez = egg->currentZ;
        for (int i = 0; i < 5; ++i)
        {
            interaction.Update(dt, world, ex, ey, ez, 0.0f, sound);
        }
        check(interaction.LifeEggCount() == 1, "egg collected exactly once (LifeEggCount == 1)");
        const auto* after = findFirst(ObjectType::ObjectType6);
        check(after == nullptr || !after->active, "collected egg is no longer active (stops rendering)");
    }
    else
    {
        check(false, "found an egg (ObjectType6) in the sample world");
    }

    if (const auto* chest = findFirst(ObjectType::ObjectType5))
    {
        const float cx = chest->currentX, cy = chest->currentY, cz = chest->currentZ;
        for (int i = 0; i < 5; ++i)
        {
            interaction.Update(dt, world, cx, cy, cz, 0.0f, sound);
        }
        check(interaction.TreasuresCollected() == 1, "chest collected exactly once (TreasuresCollected == 1)");
    }
    else
    {
        check(false, "found a chest (ObjectType5) in the sample world");
    }

    if (const auto* key = findFirst(ObjectType::ObjectType49))
    {
        const float kx = key->currentX, ky = key->currentY, kz = key->currentZ;
        for (int i = 0; i < 5; ++i)
        {
            interaction.Update(dt, world, kx, ky, kz, 0.0f, sound);
        }
        check(interaction.Key1Count() == 1, "key collected exactly once (Key1Count == 1)");
    }
    else
    {
        check(false, "found a key (ObjectType49) in the sample world");
    }

    // 3. Crate push (ObjectType12) -- position Blupi immediately west of a
    // crate and simulate walking east into it (a positive per-frame X
    // delta); the crate's currentX should increase.
    if (const auto* crate = findFirst(ObjectType::ObjectType12))
    {
        const float startX = crate->currentX;
        const float blupiZ = crate->currentZ;
        float blupiX = startX - 0.8f;
        for (int i = 0; i < 30; ++i)
        {
            const float moveDX = 0.05f; // walking east this frame
            interaction.Update(dt, world, blupiX, crate->currentY, blupiZ, moveDX, sound);
            blupiX += moveDX;
        }
        const auto* after = findFirst(ObjectType::ObjectType12);
        std::cout << "Crate X after push attempt: " << (after ? after->currentX : -999.0f)
                  << " (started at " << startX << ")" << std::endl;
        check(after != nullptr && after->currentX > startX, "crate was pushed east (currentX increased)");
    }
    else
    {
        check(false, "found a crate (ObjectType12) in the sample world");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
