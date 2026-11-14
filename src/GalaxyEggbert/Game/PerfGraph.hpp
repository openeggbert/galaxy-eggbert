#pragma once

// The `perf` cheat's record (MODERN, brief E of the owner's cheats, 2026-11-14): for each drawn frame
// the real time since the frame before it (std::chrono::steady_clock, from the end of one Draw() to the
// end of the next), how long the Update() passes in between and the frame's own Draw() took, and how
// many of the game's 1/60 s steps those passes ran (at 60 frames a second one, two when the loop
// catches up; above 60 most frames run none or one, the frames between steps being interpolated).
// Engine-independent: the host times, this keeps the last four seconds and their statistics.

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <vector>

namespace GalaxyEggbert::Game
{
    struct PerfSample
    {
        float frameMs = 0.0f;  // real time since the previous drawn frame
        float updateMs = 0.0f; // the Update() passes run since it
        float drawMs = 0.0f;   // this frame's Draw()
        int ticks = 0;         // the game's 1/60 s steps those passes ran
    };

    struct PerfStats
    {
        int frames = 0;
        float averageMs = 0.0f;        // the frames' mean time
        float worstMs = 0.0f;          // the longest frame
        float slowestPercentMs = 0.0f; // the mean of the slowest 1 % of the frames (at least one frame)
        float updateAverageMs = 0.0f, updateWorstMs = 0.0f;
        float drawAverageMs = 0.0f, drawWorstMs = 0.0f;
        std::array<int, 3> ticks{};    // the frames that ran 0, 1, and 2 or more steps
    };

    // How many of @p frames are the slowest 1 %: a hundredth, rounded up, and at least one.
    [[nodiscard]] inline std::size_t PerfSlowestCount(std::size_t frames) noexcept
    {
        return frames == 0 ? 0 : std::max<std::size_t>(1, (frames + 99) / 100);
    }

    // The statistics of @p count samples, in any order.
    [[nodiscard]] inline PerfStats ComputePerfStats(const PerfSample* samples, std::size_t count)
    {
        PerfStats stats;
        if (samples == nullptr || count == 0) return stats;
        std::vector<float> frames(count);
        double frameSum = 0.0, updateSum = 0.0, drawSum = 0.0;
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto& sample = samples[i];
            frames[i] = sample.frameMs;
            frameSum += sample.frameMs;
            updateSum += sample.updateMs;
            drawSum += sample.drawMs;
            stats.worstMs = std::max(stats.worstMs, sample.frameMs);
            stats.updateWorstMs = std::max(stats.updateWorstMs, sample.updateMs);
            stats.drawWorstMs = std::max(stats.drawWorstMs, sample.drawMs);
            ++stats.ticks[static_cast<std::size_t>(std::clamp(sample.ticks, 0, 2))];
        }
        const std::size_t slowest = PerfSlowestCount(count);
        // The `slowest` longest frames first (in no particular order among themselves).
        std::nth_element(frames.begin(), frames.begin() + static_cast<std::ptrdiff_t>(slowest - 1), frames.end(),
                         std::greater<float>());
        double slowSum = 0.0;
        for (std::size_t i = 0; i < slowest; ++i) slowSum += frames[i];
        const double n = static_cast<double>(count);
        stats.frames = static_cast<int>(count);
        stats.averageMs = static_cast<float>(frameSum / n);
        stats.slowestPercentMs = static_cast<float>(slowSum / static_cast<double>(slowest));
        stats.updateAverageMs = static_cast<float>(updateSum / n);
        stats.drawAverageMs = static_cast<float>(drawSum / n);
        return stats;
    }

    // The last frames, in a fixed ring (no allocation while drawing).
    class PerfHistory
    {
    public:
        static constexpr std::size_t kCapacity = 1024; // four seconds at 240 frames a second are 960
        static constexpr float kWindowMs = 4000.0f;    // what the graph and the statistics cover

        void Clear() noexcept { size_ = next_ = 0; }
        void Push(const PerfSample& sample) noexcept
        {
            ring_[next_] = sample;
            next_ = (next_ + 1) % kCapacity;
            if (size_ < kCapacity) ++size_;
        }
        [[nodiscard]] std::size_t Size() const noexcept { return size_; }
        // The @p i-th oldest frame kept.
        [[nodiscard]] const PerfSample& At(std::size_t i) const noexcept
        {
            return ring_[(next_ + kCapacity - size_ + i) % kCapacity];
        }
        // How many of the newest frames make the last kWindowMs (the one reaching past it included).
        [[nodiscard]] std::size_t WindowCount() const noexcept
        {
            float covered = 0.0f;
            std::size_t n = 0;
            while (n < size_ && covered < kWindowMs) covered += At(size_ - 1 - n++).frameMs;
            return n;
        }
        // The statistics of the window's frames.
        [[nodiscard]] PerfStats Stats() const
        {
            const std::size_t n = WindowCount();
            std::vector<PerfSample> window;
            window.reserve(n);
            for (std::size_t i = size_ - n; i < size_; ++i) window.push_back(At(i));
            return ComputePerfStats(window.data(), window.size());
        }

    private:
        std::array<PerfSample, kCapacity> ring_{};
        std::size_t size_ = 0, next_ = 0;
    };

    // The graph's height in milliseconds: two steps (1/30 s), or the first of 50, 100, 250, 500 ms
    // that holds the worst frame (taller ones are cut at the top).
    [[nodiscard]] inline float PerfGraphScaleMs(float worstMs) noexcept
    {
        for (const float scale : {1000.0f / 30.0f, 50.0f, 100.0f, 250.0f})
            if (worstMs <= scale) return scale;
        return 500.0f;
    }
}
