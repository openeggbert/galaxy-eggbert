#pragma once

#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
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
        bool touchControlsEnabled = false; // capability at capture time, also used on replay
        int cheatLetter = -1; // -1: keyboard snapshot; 0: no text event; a-z: ordered event
        bool accelValid = false;
        bool accelReset = false;
        float accelY = 0.0f; // The only accelerometer component read by play.
        bool operator==(const ResumeInput&) const = default;
    };

    struct ResumeStart
    {
        std::string build;
        std::string worldPath;
        std::uint64_t worldHash = 0;
        int mission = 1, lives = 3, speed = 1;
        std::uint32_t seed = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f, yaw = 0.0f;
        bool quickCheat = false, openDoors = false, showSecret = false;
        bool compass = false;
        bool debug = false;
        bool photo = false, freeze = false, xray = false, hitbox = false;
        bool trail = false, ids = false, sounds = false, chunks = false;
        int cheatsLocked = 0; // 1 fairplay's lock, 2 honest's (the rest of the run; InputPad.hpp)
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
        std::uint64_t DropLastTicks(std::uint64_t n);
        [[nodiscard]] std::string Serialize() const;
        // Transactional: on failure the destination is unchanged. Requires a complete checksum footer.
        static bool Parse(const std::string& text, const std::string& build, ResumeLog& result);
        [[nodiscard]] bool MatchesWorld(const std::string& path, std::uint64_t hash) const;
        // The format's shared body (world, start state, input runs), also carried by the shipped
        // demos (DemoReplay.hpp) under their own header. ReadBody validates every field.
        void WriteBody(std::ostream& out) const;
        static bool ReadBody(std::istream& in, ResumeLog& parsed);
        // Classic locale and the float precision that round-trips exactly.
        static void ImbueStream(std::ios& stream);
        // The "END <FNV-1a of everything before it>" footer.
        static std::string AppendChecksum(const std::string& body);
        // The text before a complete, matching footer; nullopt for truncated or damaged text.
        static std::optional<std::string> VerifiedBody(const std::string& text);
        // FNV-1a over raw bytes, including NULs. File failures never produce a valid hash.
        static std::uint64_t HashBytes(std::string_view bytes);
        static std::optional<std::uint64_t> HashFile(const std::filesystem::path& path);
        // resume.txt in the player's folder (UserFolder.hpp).
        [[nodiscard]] static std::string Path();
    };
}
