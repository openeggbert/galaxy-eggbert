#pragma once

// The timeline cheats' engine-independent parts (MODERN, brief D of the owner's cheats, 2026-11-14):
// `step`'s gate, `practice`'s rewind target and `replay`'s tail bookkeeping and skip. The host
// (GalaxyEggbertGame) feeds them from the recorded tick input, so a replay of the world's log
// (back, recall, restart, Continue, replay itself) decides exactly as play did.

#include <GalaxyEggbert/Game/ResumeLog.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <tuple>
#include <utility>

namespace GalaxyEggbert::Game
{
    // `step`: while it is on the world holds, except on a tick in which the step key (F2, or the
    // cheats' keyboard's F2) goes down: that tick runs. Held on, it steps again after half a second
    // (kRepeatAfterTicks) and then every kRepeatEveryTicks-th tick: ten ticks a second.
    class StepGate
    {
    public:
        static constexpr int kRepeatAfterTicks = 30;
        static constexpr int kRepeatEveryTicks = 6;

        // One recorded tick: whether the world steps in it (when step is on).
        bool Update(bool keyDown) noexcept
        {
            if (!keyDown)
            {
                heldTicks_ = 0;
                return false;
            }
            // Bounded: past the half second it cycles through one repeat period.
            if (++heldTicks_ > kRepeatAfterTicks + kRepeatEveryTicks) heldTicks_ -= kRepeatEveryTicks;
            if (heldTicks_ == 1) return true;
            return heldTicks_ == kRepeatAfterTicks + 1;
        }
        void Reset() noexcept { heldTicks_ = 0; }
        [[nodiscard]] int HeldTicks() const noexcept { return heldTicks_; }

    private:
        int heldTicks_ = 0;
    };

    // `practice`: the tick a death at @p deathTick goes back to instead of costing a life: @p span
    // ticks (5 s) before it, the world's start at the earliest. A death within those 5 s after the
    // last practice rewind's landing (@p lastLanding) was unavoidable from there: it goes 5 s before
    // that landing instead, farther each time, so the same doomed moment never comes back for ever;
    // when the landing was the world's start already there is nothing earlier and the death is an
    // ordinary one (nullopt).
    [[nodiscard]] inline std::optional<std::uint64_t> PracticeTarget(std::uint64_t deathTick,
                                                                     std::optional<std::uint64_t> lastLanding,
                                                                     std::uint64_t span = 300) noexcept
    {
        const auto before = [span](std::uint64_t tick) { return tick > span ? tick - span : 0; };
        const std::uint64_t target = before(deathTick);
        if (!lastLanding || target > *lastLanding) return target;
        if (*lastLanding == 0) return std::nullopt;
        return before(*lastLanding);
    }

    // `replay`: the world's last kTicks played again, visibly. Begin() keeps the whole log and hands
    // back the copy to rebuild fast (its last kTicks dropped); Next() then gives the dropped ticks'
    // input one by one, which the host plays and appends to its log, so that the log ends as the
    // original did and the game where `replay` was typed.
    class InstantReplay
    {
    public:
        static constexpr std::uint64_t kTicks = 600;

        // The log to rebuild fast (the original without its last @p ticks).
        ResumeLog Begin(const ResumeLog& log, std::uint64_t ticks = kTicks)
        {
            *this = {};
            original_ = log;
            end_ = log.TickCount();
            start_ = end_ > ticks ? end_ - ticks : 0;
            active_ = true;
            ResumeLog trimmed = log;
            trimmed.DropLastTicks(end_ - start_);
            position_ = start_;
            std::tie(run_, repeat_) = Cursor(original_, start_);
            return trimmed;
        }
        // The fast rebuild reached the start: the rest plays visibly.
        void BeginVisible() noexcept { visible_ = true; }
        // The rest is to run fast and unseen from where the visible part stands.
        void EndVisible() noexcept { visible_ = false; }
        [[nodiscard]] bool Active() const noexcept { return active_; }
        [[nodiscard]] bool Visible() const noexcept { return active_ && visible_; }
        [[nodiscard]] std::uint64_t Start() const noexcept { return start_; }
        [[nodiscard]] std::uint64_t End() const noexcept { return end_; }
        [[nodiscard]] std::uint64_t Position() const noexcept { return position_; }
        [[nodiscard]] bool Done() const noexcept { return position_ >= end_; }
        [[nodiscard]] const ResumeLog& Original() const noexcept { return original_; }

        // The input of the next tick to play (the original's tick Position()), nullptr at the end.
        const ResumeInput* Next() noexcept
        {
            if (position_ >= end_ || run_ >= original_.runs.size()) return nullptr;
            const auto& run = original_.runs[run_];
            if (++repeat_ == run.count)
            {
                repeat_ = 0;
                ++run_;
            }
            ++position_;
            return &run.input;
        }

        // Where tick @p tick lies in @p log's runs: (run, ticks of it before), (runs.size(), 0) past the end.
        [[nodiscard]] static std::pair<std::size_t, std::uint32_t> Cursor(const ResumeLog& log, std::uint64_t tick) noexcept
        {
            for (std::size_t i = 0; i < log.runs.size(); ++i)
            {
                if (tick < log.runs[i].count) return {i, static_cast<std::uint32_t>(tick)};
                tick -= log.runs[i].count;
            }
            return {log.runs.size(), 0};
        }

        void Clear() { *this = {}; }

    private:
        ResumeLog original_;
        std::uint64_t start_ = 0, end_ = 0, position_ = 0;
        std::size_t run_ = 0;
        std::uint32_t repeat_ = 0;
        bool active_ = false, visible_ = false;
    };

    // A press that counts only once it is new: what is held as it starts (the letters of `replay`,
    // Enter) does not count until it has been let go.
    class PressLatch
    {
    public:
        bool Update(bool down) noexcept
        {
            const bool pressed = down && !held_;
            held_ = down;
            return pressed;
        }
        void Hold() noexcept { held_ = true; }

    private:
        bool held_ = true;
    };
}
