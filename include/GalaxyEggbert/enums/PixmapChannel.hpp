#pragma once
#include "SharpRuntime/SharpRuntimeHelper.hpp"

namespace GalaxyEggbert
{
    enum class PixmapChannel : SharpRuntime::ushortcs
    {
        PixmapChannel0 = 0,
        Object = 1,
        Blupi = 2,
        Background = 3,
        Button = 4,
        Jauge = 5,
        Text = 6,
        Explosion = 9,
        Element = 10,
        Blupi1_11 = 11,
        Blupi1_12 = 12,
        Blupi1_13 = 13,
        Pad = 14,
        SpeedyBlupiBackground = 15,
        BlupiYoupieBackground = 16,
        GearBackground = 17

    };

    static constexpr auto ToRaw(PixmapChannel type) -> SharpRuntime::ushortcs
    {
        return static_cast<SharpRuntime::ushortcs>(type);
    }

    static constexpr auto ToPixmapChannel(const int value) -> PixmapChannel
    {
        return static_cast<PixmapChannel>(
            static_cast<SharpRuntime::ushortcs>(value)
        );
    }
    //
    // constexpr bool operator<(const PixmapChannel lhs, const PixmapChannel rhs) noexcept
    // {
    //     return ToRaw(lhs) < ToRaw(rhs);
    // }
    //
    // constexpr bool operator>(const PixmapChannel lhs, const PixmapChannel rhs) noexcept
    // {
    //     return ToRaw(lhs) > ToRaw(rhs);
    // }
    //
    // constexpr bool operator<=(const PixmapChannel lhs, const PixmapChannel rhs) noexcept
    // {
    //     return ToRaw(lhs) <= ToRaw(rhs);
    // }
    //
    // constexpr bool operator>=(const PixmapChannel lhs, const PixmapChannel rhs) noexcept
    // {
    //     return ToRaw(lhs) >= ToRaw(rhs);
    // }
}
