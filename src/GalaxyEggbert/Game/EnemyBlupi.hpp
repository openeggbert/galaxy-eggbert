#pragma once

// The enemy Blupis (OR-0063): ObjectType32 (blupih) on his helicopter, dropping bombs, and
// ObjectType33 (blupit) in his tank, firing to both sides. The original draws them from blupi1.png
// (the channel the world file gives them, 11-13, all one sheet: Pixmap.cpp:717-719), which is his
// own blupi.png with every hue turned 25 degrees (the yellow body green, the cyan vehicles blue;
// saturation and brightness kept: 99.8 % of the sheet's pixels within 2 levels), at the icons of
// table_blupih_* / table_blupit_* (Tables.cpp:1314-1360). Those are his own frames: the
// helicopter's flight (MarchHelico, 61-70), its turn (TurnHelico, 71-75) and its glue drop
// (HelicoGlu, 271-275); the tank's drive (MarchTank, 237-239 / 248-250), its shot (FireTank,
// 251 / 252) and its turn (TurnTank, 238-249). So they are drawn as his real 3D model in those
// clips, its textures recoloured the same way. Engine-independent: VerifyDefinitionRegistries
// checks it.

#include <cmath>
#include <cstdint>

namespace GalaxyEggbert::Game
{
    // blupi.png -> blupi1.png: the hue turned 25 degrees (HSV), greys unchanged.
    inline void RecolorEnemyBlupi(std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) noexcept
    {
        const float rf = r / 255.0f;
        const float gf = g / 255.0f;
        const float bf = b / 255.0f;
        const float max = std::fmax(rf, std::fmax(gf, bf));
        const float min = std::fmin(rf, std::fmin(gf, bf));
        const float delta = max - min;
        if (delta <= 0.0f)
        {
            return;
        }
        float hue = 0.0f; // in sixths of the circle
        if (max == rf)
        {
            hue = std::fmod((gf - bf) / delta + 6.0f, 6.0f);
        }
        else if (max == gf)
        {
            hue = (bf - rf) / delta + 2.0f;
        }
        else
        {
            hue = (rf - gf) / delta + 4.0f;
        }
        hue = std::fmod(hue + 25.0f / 60.0f, 6.0f);
        const float chroma = delta; // value * saturation
        const float x = chroma * (1.0f - std::fabs(std::fmod(hue, 2.0f) - 1.0f));
        float r1 = 0.0f;
        float g1 = 0.0f;
        float b1 = 0.0f;
        switch (static_cast<int>(hue))
        {
            case 0: r1 = chroma; g1 = x; break;
            case 1: r1 = x; g1 = chroma; break;
            case 2: g1 = chroma; b1 = x; break;
            case 3: g1 = x; b1 = chroma; break;
            case 4: r1 = x; b1 = chroma; break;
            default: r1 = chroma; b1 = x; break;
        }
        const auto channel = [min](float value)
        {
            return static_cast<std::uint8_t>(std::lround(std::fmin(1.0f, value + min) * 255.0f));
        };
        r = channel(r1);
        g = channel(g1);
        b = channel(b1);
    }

    enum class EnemyBlupiClip : std::uint8_t
    {
        MarchHelico,
        HelicoGlu,
        MarchTank,
        FireTank,
    };

    [[nodiscard]] inline const char* EnemyBlupiClipName(EnemyBlupiClip clip) noexcept
    {
        switch (clip)
        {
            case EnemyBlupiClip::MarchHelico: return "MarchHelico";
            case EnemyBlupiClip::HelicoGlu: return "HelicoGlu";
            case EnemyBlupiClip::MarchTank: return "MarchTank";
            case EnemyBlupiClip::FireTank: return "FireTank";
        }
        return "MarchHelico";
    }

    struct EnemyBlupiPose
    {
        EnemyBlupiClip clip = EnemyBlupiClip::MarchHelico;
        float clipTicks = 0.0f; // into the clip, in the original's ticks (20 a second)
        // Which way he faces: 1 toward posEnd (as from posStart), 0 toward posStart, between while
        // he turns.
        float towardEnd = 1.0f;
    };

    // His pose at a patrol's step (1 dwelling at posStart, turning to face posEnd; 2 on the way to
    // posEnd; 3 dwelling there, turning back; 4 on the way back) and its time in ticks, as the
    // original's tables index it (time % length, Decor.cpp:8838-8931): the turn tables run their
    // frames once per tick from the dwell's start (and again should a dwell outlast them).
    [[nodiscard]] inline EnemyBlupiPose ResolveEnemyBlupiPose(bool tank, int patrolStep, float patrolTimeTicks) noexcept
    {
        const float t = patrolTimeTicks < 0.0f ? 0.0f : patrolTimeTicks;
        EnemyBlupiPose pose;
        const bool turning = patrolStep == 1 || patrolStep == 3;
        if (!turning)
        {
            pose.clip = tank ? EnemyBlupiClip::MarchTank : EnemyBlupiClip::MarchHelico;
            pose.clipTicks = t;
            pose.towardEnd = patrolStep == 4 ? 0.0f : 1.0f;
            return pose;
        }
        float turned = 0.0f; // how far through the turn, 0 the way he came, 1 the way he goes
        if (!tank)
        {
            // table_blupih_turn2l/2r: 71-75 two ticks each, then two of the new way (68 / 63),
            // then the glue drop (274/275, 271-273: HelicoGlu, 14 frames), its bomb at 21.
            const float k = std::fmod(t, 26.0f);
            turned = std::fmin(1.0f, (k + 1.0f) / 11.0f);
            if (k < 12.0f)
            {
                pose.clip = EnemyBlupiClip::MarchHelico;
                pose.clipTicks = t;
            }
            else
            {
                pose.clip = EnemyBlupiClip::HelicoGlu;
                pose.clipTicks = k - 12.0f;
            }
        }
        else
        {
            // table_blupit_turn2l/2r: two ticks standing, the shot the way he came (251 / 252 and
            // two standing: FireTank, its shell at 3), the turn (239-248, a tick each), three
            // standing, the shot the new way (at 21) and two standing.
            const float k = std::fmod(t, 24.0f);
            pose.clip = EnemyBlupiClip::MarchTank;
            pose.clipTicks = 0.0f;
            if (k >= 2.0f && k < 6.0f)
            {
                pose.clip = EnemyBlupiClip::FireTank;
                pose.clipTicks = k - 2.0f;
            }
            else if (k >= 20.0f)
            {
                pose.clip = EnemyBlupiClip::FireTank;
                pose.clipTicks = k - 20.0f;
            }
            turned = k < 6.0f ? 0.0f : std::fmin(1.0f, (k - 6.0f) / 11.0f);
        }
        pose.towardEnd = patrolStep == 1 ? turned : 1.0f - turned;
        return pose;
    }
}
