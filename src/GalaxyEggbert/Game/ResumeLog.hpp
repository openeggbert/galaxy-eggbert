#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace GalaxyEggbert::Game
{
    struct ResumeTouch
    {
        int id = 0;
        float x = 0.0f, y = 0.0f;
        int state = 0;
        bool operator==(const ResumeTouch&) const = default;
    };

    // Raw input after the gamepad/keyboard merge, before game interpretation.
    struct ResumeInput
    {
        std::vector<int> keys;
        int mouseX = 0, mouseY = 0, wheel = 0;
        unsigned buttons = 0;
        int viewportWidth = 1, viewportHeight = 1;
        std::vector<ResumeTouch> touches;
        bool accelValid = false;
        bool accelReset = false;
        float accelY = 0.0f; // The only accelerometer component read by play.
        bool operator==(const ResumeInput&) const = default;
    };

    struct ResumeStart
    {
        std::string build;
        int mission = 1, lives = 3, speed = 1;
        std::uint32_t seed = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f;
        bool quickCheat = false, openDoors = false, showSecret = false;
        int cameraMode = 1;
        // Settings and campaign doors as they were at the start, before any pause/setup edits.
        std::string saveData;
        bool operator==(const ResumeStart&) const = default;
    };

    struct ResumeRun
    {
        std::uint32_t count = 1;
        ResumeInput input;
        bool operator==(const ResumeRun&) const = default;
    };

    // Reconstructs CurrentWrite/CurrentRead's moment by replay (Decor.cpp:10974-11300).
    class ResumeLog
    {
    public:
        ResumeStart start;
        std::vector<ResumeRun> runs;
        void Append(const ResumeInput& input);
        [[nodiscard]] std::uint64_t TickCount() const;
        [[nodiscard]] std::string Serialize() const;
        // Transactional: on failure the destination is unchanged. Requires a complete checksum footer.
        static bool Parse(const std::string& text, const std::string& build, ResumeLog& result);
#if defined(__EMSCRIPTEN__)
        static constexpr const char* kPath = "/save/resume.txt";
#else
        static constexpr const char* kPath = "resume.txt";
#endif
    };
}
