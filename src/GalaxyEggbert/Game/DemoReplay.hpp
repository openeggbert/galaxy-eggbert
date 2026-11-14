#pragma once

// The main menu's demos (Speedy Blupi's CEvent::DemoPlayStart/DemoStep/DemoPlayStop, blupi_recon
// src/event.cpp:3712-3825, and TreatEvent's key/button check, event.cpp:4074-4090): after a while
// without input in the menu, or from the menu's Demo button, a recorded game plays with the normal
// simulation, picture and sound; a new key, button or touch stops it and returns to the menu; mouse
// movement alone does not. At its end the next demo starts, after the last one the first again.
//
// The original's .BLP files hold key changes for its 2D engine; Galaxy's demos hold the same input
// history as a resume log (ResumeLog.hpp: the start state, the seed and the raw input of each 1/60 s
// tick) under their own header. Resume files are refused by any other build, since replaying them
// needs the identical binary; a shipped demo instead names the simulation revision it was recorded
// with (kDemoSimulationRevision) and the hash of its world. When a simulation change makes a demo
// replay differently, tools/verify_demos.sh reports it through the end-state signature stored in the
// file; bump the revision and record the demos again (tools/record_demo.sh). A demo whose revision or
// world differs is skipped unless the recording carries its original world bytes. Revision
// opt-in is reserved for the future private collection; menu demos remain strict.
//
// Engine-independent: GalaxyEggbertGame feeds this with its live input and plays the recording.

#include "ResumeLog.hpp"
#include "Localization.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace GalaxyEggbert::Game
{
    inline constexpr int kDemoFormatVersion = 2;
    // Bump whenever a simulation change makes tools/verify_demos.sh report a shipped demo as
    // different, then record the demos again; the old files are then skipped as incompatible.
    inline constexpr int kDemoSimulationRevision = 1;
    // The original's DEF_TIME_DEMO (1000 ticks, its "about 30 seconds"); the owner asked for 40 s.
    inline constexpr float kDemoIdleSeconds = 40.0f;
    // Content/demos/demo001.gdemo ... (the original's data\demo200.blp ...), numbered from 1.
    inline constexpr int kDemoMaxCount = 99;
    // Ten minutes of 1/60 s ticks: a demo is a short show, not a playthrough.
    inline constexpr std::uint64_t kDemoMaxTicks = 60ull * 60ull * 10ull;

    [[nodiscard]] std::string DemoPath(int number);

    // The ten demo positions the menu plays in turn (owner, 2026-11-08: ten demos). Position n plays
    // the player's own recording of it (BD-0116: Pause, Record) when there is one, the shipped
    // DemoPath(n) otherwise -- and that one too when the recording cannot be played.
    inline constexpr int kDemoSlotCount = 10;
    // The player's recordings: demos/demo%03d.gdemo beside savedata.txt (/save/demos/ on the web).
    [[nodiscard]] std::string UserDemoPath(int number);
    [[nodiscard]] std::string UserDemoDir();

    // Sharing recordings (2026-11-09), as the editor shares worlds (ED-0012): an export is a copy in
    // SharedDemoDir() (UserDemoDir()/shared; downloaded on the web as well), an import copies a
    // .gdemo from there (the web's file picker writes it there) into a position.
    [[nodiscard]] std::string SharedDemoDir();
    // "A-demo-5.gdemo" (players A-C), "player7-demo-5.gdemo" otherwise; a demo with a name of its
    // own (@p title, not the automatic one) "<its name>.gdemo", spaces as '-'.
    [[nodiscard]] std::string DemoExportName(int gamer, int slot, const std::string& title = {});
    // Copies @p source (a position's demo) into SharedDemoDir() as DemoExportName() (with the
    // demo's own title), numbered when that is taken (A-demo-5-2.gdemo ...: an earlier export
    // stays); its path, or empty when there is no such file or the copy failed.
    [[nodiscard]] std::string ExportDemo(const std::string& source, int gamer, int slot);

    // A demo's name (2026-11-10, the owner: a recording named as a galaxy is): the title kept in
    // its file. A recording starts with an automatic one ("Recorded by player A: mission 11");
    // the player can give it a name of letters, digits, spaces, '-' and '\'', kDemoNameMax at most.
    inline constexpr std::size_t kDemoNameMax = 24;
    [[nodiscard]] std::string SanitizeDemoName(const std::string& name);
    [[nodiscard]] bool IsAutomaticDemoTitle(const std::string& title) noexcept;
    // The title of the demo in @p path; empty when it is not a demo this game reads.
    [[nodiscard]] std::string DemoTitle(const std::string& path);
    // Writes @p name (SanitizeDemoName(), not empty) as the title of the demo in @p path, the rest
    // of the file as it was; false when it is not a demo this game reads or the write failed.
    bool RenameDemo(const std::string& path, const std::string& name, bool acceptOtherRevision = false);
    // Collection rename: title and filename, without replacing another recording. Empty on failure.
    [[nodiscard]] std::string RenameDemoFile(const std::string& path, const std::string& name);
    bool DeleteDemoFile(const std::string& path);
    // The .gdemo files in SharedDemoDir(), the newest first.
    [[nodiscard]] std::vector<std::string> ListSharedDemos();

    struct DemoRecording
    {
        int simulationRevision = kDemoSimulationRevision;
        std::string title;
        std::string gameVersion;
        std::string recordedAt; // UTC ISO 8601, supplied by the host when recording ends
        std::optional<std::string> worldBytes; // exact loaded snapshot; absent in developer demos
        // The start state and the input runs. log.start.build names the recording build, for
        // information only; it is not compared.
        ResumeLog log;
        // GalaxyEggbertGame::ResumeSignature() after the last tick, each entry hashed.
        std::vector<std::pair<std::string, std::uint64_t>> endSignature;
        [[nodiscard]] std::string Serialize() const;
    };

    enum class DemoLoadStatus
    {
        Ok,
        Missing,
        Corrupt,            // truncated, damaged, or not a demo at all
        IncompatibleFormat, // another kDemoFormatVersion
        IncompatibleRevision,
        WorldMissing,
        WorldChanged,
        OwnWorld,           // playable from its embedded world instead of the named file
    };
    inline const char* DemoLoadStatusText(DemoLoadStatus status) noexcept
    {
        switch (status)
        {
            case DemoLoadStatus::Ok: return "ok";
            case DemoLoadStatus::Missing: return TrMark("missing");
            case DemoLoadStatus::Corrupt: return TrMark("damaged or not a demo");
            case DemoLoadStatus::IncompatibleFormat: return TrMark("another demo format");
            case DemoLoadStatus::IncompatibleRevision: return TrMark("recorded with another simulation revision");
            case DemoLoadStatus::WorldMissing: return TrMark("its world is missing");
            case DemoLoadStatus::OwnWorld: return TrMark("plays from its own world copy");
            case DemoLoadStatus::WorldChanged: return TrMark("its world has changed");
        }
        return TrMark("unknown");
    }

    // Parses and validates a demo's text: header, checksum, every start/input field (ResumeLog's own
    // checks), normal game speed and kDemoMaxTicks. Transactional: `out` is unchanged on failure.
    // Embedded bytes must match worldHash; the host checks the named file (DemoWorldStatus()).
    // acceptOtherRevision bypasses only the revision check, retaining the recorded revision.
    // Menu demos, imports and verification keep the default refusal.
    [[nodiscard]] DemoLoadStatus ParseDemo(const std::string& text, int expectedRevision, DemoRecording& out,
                                            bool acceptOtherRevision = false);
    // Copies @p source into position @p slot (UserDemoPath()) once ParseDemo() accepts it (its world
    // is the host's to find when it plays); the status, Ok when copied, and nothing written otherwise.
    [[nodiscard]] DemoLoadStatus ImportDemo(const std::string& source, int slot);
    // Whether the recording's world is the file whose bytes these are (nullopt: unreadable).
    [[nodiscard]] DemoLoadStatus DemoWorldStatus(const DemoRecording& demo, const std::string& path,
                                                 const std::optional<std::string>& worldBytes);

    struct DemoCollectionEntry
    {
        std::string path, name, metadata;
        int mission = 0;
        std::uint64_t seconds = 0;
        bool galaxy = false;
        DemoLoadStatus status = DemoLoadStatus::Missing;
        bool olderRevision = false;
        [[nodiscard]] bool Playable() const noexcept
        { return status == DemoLoadStatus::Ok || status == DemoLoadStatus::OwnWorld; }
    };
    // Files and row data, newest modification first (path order breaks ties). World resolution is
    // supplied by the host so campaign and shared galaxy recordings use the same rules as playback.
    [[nodiscard]] std::vector<DemoCollectionEntry> ListDemoCollection(
        const std::function<std::string(DemoRecording&)>& resolveWorld);

    // Materializes only a validated embedded snapshot, atomically, at demos/worlds/<hash>.vwr.
    // Reuses a matching cached file; repairs a damaged one. No campaign/galaxy writes.
    [[nodiscard]] std::optional<std::string> CacheDemoWorld(const DemoRecording& demo);

    [[nodiscard]] std::vector<std::pair<std::string, std::uint64_t>> HashDemoSignature(
        const std::map<std::string, std::string>& signature);
    // The entries that differ, are missing on either side, or were added.
    [[nodiscard]] std::vector<std::string> DemoSignatureDifferences(
        const std::vector<std::pair<std::string, std::uint64_t>>& expected,
        const std::vector<std::pair<std::string, std::uint64_t>>& actual);

    // The demos to try after `current` (0: none yet), in the original's order: the next number,
    // wrapping to the first after the last (DemoStep: m_demoNumber++, else 0).
    [[nodiscard]] std::vector<int> DemoOrderAfter(int current, int count);

    // The player's live input, once per update: in the menu for the idle timer, during a demo for
    // stopping it. The host fills it from the keyboard, the native key events since the last
    // update (presses shorter than an update), the mouse, the first gamepad (GamepadKeys() bits)
    // and the touch panel.
    struct DemoLiveInput
    {
        std::vector<int> keys;      // held, sorted
        std::vector<int> keyEvents; // pressed by a native event since the previous snapshot
        int mouseX = 0, mouseY = 0, wheel = 0;
        unsigned mouseButtons = 0;
        unsigned padButtons = 0;
        int touches = 0;       // fingers on the screen
        int touchPresses = 0;  // fingers put down since the previous snapshot
    };
    // Anything the player does, mouse movement included: resets the idle timer.
    [[nodiscard]] bool DemoActivity(const DemoLiveInput& previous, const DemoLiveInput& now) noexcept;
    // A new press: a key, a mouse or gamepad button, the wheel or a finger. Not a movement of the
    // mouse, nor what was already held at the previous snapshot (an auto-repeating key included),
    // nor a release: what started the demo cannot stop it.
    [[nodiscard]] bool DemoNewPress(const DemoLiveInput& previous, const DemoLiveInput& now) noexcept;
    // Anything still held; the menu ignores input after a demo until nothing is.
    [[nodiscard]] bool DemoInputHeld(const DemoLiveInput& now) noexcept;

    // Seconds without input in the main menu.
    class DemoIdleTimer
    {
    public:
        explicit DemoIdleTimer(float seconds = kDemoIdleSeconds) noexcept : seconds_(seconds) {}
        // `eligible`: the settled main menu of the active application, nothing else open. Time
        // counts only then; activity or leaving it starts again from zero. True once when the
        // time is up (then from zero again).
        bool Update(float elapsedSeconds, bool eligible, bool activity) noexcept;
        void Reset() noexcept { elapsed_ = 0.0f; }
        [[nodiscard]] float Elapsed() const noexcept { return elapsed_; }

    private:
        float seconds_;
        float elapsed_ = 0.0f;
    };
}
