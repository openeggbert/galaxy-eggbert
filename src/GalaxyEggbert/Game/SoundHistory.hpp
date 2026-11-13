#pragma once
#include <array>
#include <cstddef>
#include <cmath>

namespace GalaxyEggbert::Game
{
    enum class SoundOutcome { Played, OutOfReach, Busy, Replay, Hidden, Disabled, Missing };
    inline const char* SoundOutcomeName(SoundOutcome outcome)
    {
        switch (outcome)
        {
            case SoundOutcome::Played: return "played";
            case SoundOutcome::OutOfReach: return "out of reach";
            case SoundOutcome::Busy: return "channel still playing";
            case SoundOutcome::Replay: return "muted: replay";
            case SoundOutcome::Hidden: return "muted: hidden Blupi";
            case SoundOutcome::Disabled: return "muted: sounds off";
            case SoundOutcome::Missing: return "missing channel/WAV";
        }
        return "";
    }
    // The order is Sound's existing decision order; channel 10/loops bypass busy in the caller.
    inline SoundOutcome SoundDecision(bool reachable, bool replay, bool hidden, bool enabled, bool loaded, bool busy)
    {
        if (!reachable) return SoundOutcome::OutOfReach;
        if (replay) return SoundOutcome::Replay;
        if (hidden) return SoundOutcome::Hidden;
        if (!enabled) return SoundOutcome::Disabled;
        if (!loaded) return SoundOutcome::Missing;
        if (busy) return SoundOutcome::Busy;
        return SoundOutcome::Played;
    }
    struct SoundRequest
    {
        int channel = 0;
        bool positional = false;
        float x = 0, y = 0, z = 0, distance = 0;
        SoundOutcome outcome = SoundOutcome::Played;
    };
    class SoundHistory
    {
    public:
        static constexpr std::size_t kCapacity = 10;
        void Push(SoundRequest request)
        {
            entries_[next_] = request;
            next_ = (next_ + 1) % kCapacity;
            if (count_ < kCapacity) ++count_;
        }
        std::size_t Size() const { return count_; }
        const SoundRequest& At(std::size_t oldestFirst) const
        { return entries_[(next_ + kCapacity - count_ + oldestFirst) % kCapacity]; }
    private:
        std::array<SoundRequest,kCapacity> entries_{};
        std::size_t next_ = 0, count_ = 0;
    };
}
