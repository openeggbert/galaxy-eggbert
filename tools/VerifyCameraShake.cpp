#include <GalaxyEggbert/Game/CameraShake.hpp>

#include <iostream>

// Scripted verification of CameraShake (plan.md CAM-008..013) -- the
// real per-frame (dx,dy) camera-shake table (Tables::table_decor_action,
// verbatim-transcribed and independently byte-verified against the real
// source), the real x3 multiplier, the real 20Hz tick rate, and the real
// self-clear-to-None-on-exhaustion behavior. No graphics context needed.
int main()
{
    using namespace GalaxyEggbert::Game;

    bool allOk = true;
    const auto check = [&allOk](bool cond, const char* what)
    {
        std::cout << (cond ? "PASS" : "FAIL") << ": " << what << std::endl;
        if (!cond) allOk = false;
    };

    constexpr float dt = CameraShake::kTickSeconds; // exactly one real tick per Update()

    // --- SmallShake: 32 frames, first frame (-4,4)*3, self-clears after ---
    {
        CameraShake shake;
        check(shake.GetActiveType() == CameraShakeType::None, "starts with no active shake");
        check(shake.GetOffsetX() == 0.0f && shake.GetOffsetY() == 0.0f, "starts with zero offset");

        shake.Trigger(CameraShakeType::Small);
        check(shake.GetActiveType() == CameraShakeType::Small, "Trigger(Small) sets the active type");
        check(shake.GetPhase() == 0, "Trigger() resets phase to 0");

        shake.Update(dt);
        check(shake.GetOffsetX() == -12.0f && shake.GetOffsetY() == 12.0f,
              "SmallShake frame 1 offset is the real (-4,4)*3 = (-12,12)");

        // 31 more calls consumes frames 1..31 (32 total frames consumed);
        // one further call is needed to observe the self-clear, since
        // m_phase==frameCount is only detected on the NEXT Update() after
        // the last real frame was processed.
        for (int i = 0; i < 31; ++i)
        {
            shake.Update(dt);
        }
        shake.Update(dt);
        check(shake.GetActiveType() == CameraShakeType::None,
              "SmallShake self-clears to None after its real 32 frames");
        check(shake.GetOffsetX() == 0.0f && shake.GetOffsetY() == 0.0f,
              "offset resets to zero once the shake self-clears");
    }

    // --- BigShake: 32 frames, pure horizontal, first frame (-4,0)*3 ---
    {
        CameraShake shake;
        shake.Trigger(CameraShakeType::Big);
        shake.Update(dt);
        check(shake.GetOffsetX() == -12.0f && shake.GetOffsetY() == 0.0f,
              "BigShake frame 1 offset is the real (-4,0)*3 = (-12,0), purely horizontal");
        for (int i = 0; i < 31; ++i)
        {
            shake.Update(dt);
        }
        shake.Update(dt);
        check(shake.GetActiveType() == CameraShakeType::None,
              "BigShake self-clears to None after its real 32 frames");
    }

    // --- ElectricShake: 192 frames, first frame (0,-32)*3, purely vertical start ---
    {
        CameraShake shake;
        shake.Trigger(CameraShakeType::Electric);
        shake.Update(dt);
        check(shake.GetOffsetX() == 0.0f && shake.GetOffsetY() == -96.0f,
              "ElectricShake frame 1 offset is the real (0,-32)*3 = (0,-96)");
        for (int i = 0; i < 191; ++i)
        {
            shake.Update(dt);
        }
        shake.Update(dt);
        check(shake.GetActiveType() == CameraShakeType::None,
              "ElectricShake self-clears to None after its real 192 frames");
    }

    // --- Re-trigger while active restarts unconditionally (real: a plain
    // assignment, no priority gating) ---
    {
        CameraShake shake;
        shake.Trigger(CameraShakeType::Big);
        shake.Update(dt);
        shake.Update(dt);
        shake.Trigger(CameraShakeType::Small);
        check(shake.GetActiveType() == CameraShakeType::Small,
              "triggering a new shake while one is active unconditionally overrides it");
        check(shake.GetPhase() == 0, "the override restarts phase back to 0");
        shake.Update(dt);
        check(shake.GetOffsetX() == -12.0f && shake.GetOffsetY() == 12.0f,
              "the overriding shake starts from its own frame 1, not a leftover offset");
    }

    // --- Update() with no active shake is a no-op ---
    {
        CameraShake shake;
        shake.Update(dt);
        shake.Update(dt);
        check(shake.GetActiveType() == CameraShakeType::None, "Update() with no active shake stays None");
        check(shake.GetOffsetX() == 0.0f && shake.GetOffsetY() == 0.0f,
              "Update() with no active shake keeps the offset at zero");
    }

    // --- Sub-tick dt accumulates correctly (real behavior driven at a
    // fixed 20Hz regardless of the caller's actual frame rate) ---
    {
        CameraShake shake;
        shake.Trigger(CameraShakeType::Small);
        // Two half-ticks should advance exactly one real frame, not zero
        // and not two.
        shake.Update(dt * 0.5f);
        check(shake.GetPhase() == 0, "a sub-tick dt alone doesn't yet advance a full frame");
        shake.Update(dt * 0.5f);
        check(shake.GetPhase() == 1, "two half-ticks together advance exactly one real frame");
    }

    std::cout << (allOk ? "ALL CHECKS PASSED" : "SOME CHECKS FAILED") << std::endl;
    return allOk ? 0 : 1;
}
