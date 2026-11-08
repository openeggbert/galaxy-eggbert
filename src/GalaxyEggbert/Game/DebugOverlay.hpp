#pragma once

#include <array>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Mobile InputPad.cpp's MODERN debug fields, adapted to raw 3D grid coordinates
    // and the world's actual 20Hz clock. Diagnostic data only; no gameplay mutation.
    struct DebugOverlayInfo
    {
        int ticks = 0, mission = 0, region = 0, lives = 0;
        int animation = 0, icon = 0, keys = 0, touches = 0;
        std::array<float, 3> position{}, velocity{};
        float yawDegrees = 0, speed = 1;
        std::string mode;
        bool gamepad = false, accelerometer = false;
    };

    [[nodiscard]] inline std::vector<std::string> DebugOverlayLines(const DebugOverlayInfo& info)
    {
        std::vector<std::string> lines{"DEBUG"};
        char text[256];
        const int seconds = info.ticks / 20;
        std::snprintf(text, sizeof(text), "time: %02d:%02d (%d ticks)", seconds / 60, seconds % 60, info.ticks);
        lines.emplace_back(text);
        lines.emplace_back("mission: " + std::to_string(info.mission) + "  region: " + std::to_string(info.region));
        lines.emplace_back("lives: " + std::to_string(info.lives));
        std::snprintf(text, sizeof(text), "pos: %.2f %.2f %.2f", info.position[0], info.position[1], info.position[2]);
        lines.emplace_back(text);
        std::snprintf(text, sizeof(text), "cell: %d %d %d", int(std::floor(info.position[0] + .5f)),
                      int(std::floor(info.position[1] + .5f)), int(std::floor(info.position[2] + .5f)));
        lines.emplace_back(text);
        std::snprintf(text, sizeof(text), "vel: %.2f %.2f %.2f blocks/s", info.velocity[0], info.velocity[1], info.velocity[2]);
        lines.emplace_back(text);
        float yaw = std::fmod(info.yawDegrees, 360.f);
        if (yaw < 0) yaw += 360.f;
        std::snprintf(text, sizeof(text), "yaw: %.1f deg", yaw);
        lines.emplace_back(text);
        lines.emplace_back("mode: " + (info.mode.empty() ? std::string("-") : info.mode));
        lines.emplace_back("anim: " + std::to_string(info.animation) + "  icon: " + std::to_string(info.icon));
        std::snprintf(text, sizeof(text), "speed: %.1fx", info.speed);
        lines.emplace_back(text);
        lines.emplace_back("keys: " + std::to_string(info.keys) + "  touches: " + std::to_string(info.touches));
        lines.emplace_back(std::string("gamepad: ") + (info.gamepad ? "on" : "off") +
                           "  accel: " + (info.accelerometer ? "on" : "off"));
        return lines;
    }
}
