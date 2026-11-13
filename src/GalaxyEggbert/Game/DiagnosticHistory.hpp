#pragma once
#include <array>
#include <deque>
#include <filesystem>
#include <string>

namespace GalaxyEggbert::Game
{
    class PositionTrail
    {
    public:
        static constexpr std::size_t kTicks = 600; // ten seconds at 60 ticks/s
        void Sample(float x, float y, float z)
        {
            points_.push_back({x,y,z});
            if (points_.size() > kTicks) points_.pop_front();
        }
        void Clear() { points_.clear(); }
        const auto& Points() const { return points_; }
    private:
        std::deque<std::array<float,3>> points_;
    };

    // The timestamp is supplied by the caller; suffixes handle repeated requests in one second.
    inline std::filesystem::path SnapshotPath(const std::filesystem::path& folder, const std::string& timestamp)
    {
        const std::string stem = "galaxy-" + timestamp;
        auto path = folder / (stem + ".png");
        for (unsigned n = 1; std::filesystem::exists(path); ++n)
            path = folder / (stem + "-" + std::to_string(n) + ".png");
        return path;
    }
}
