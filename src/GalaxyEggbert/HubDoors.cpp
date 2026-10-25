#include "GalaxyEggbert/HubDoors.hpp"

#include "GalaxyEggbert/BlockTypes.hpp"

#include <algorithm>
#include <cmath>

namespace GalaxyEggbert {

namespace {

constexpr int kAxes[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};

std::uint16_t IconAt(const Worlds::World& world, int x, int y, int z) {
    const int n = static_cast<int>(world.blocksPerAxis());
    if (x < 0 || y < 0 || z < 0 || x >= n || y >= n || z >= n) {
        return 0;
    }
    return world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y), static_cast<std::uint16_t>(z))
        .type();
}

template <typename Predicate>
std::vector<HubCell> FindCells(const Worlds::World& world, Predicate matches) {
    std::vector<HubCell> cells;
    const int n = static_cast<int>(world.blocksPerAxis());
    for (int y = 0; y < n; ++y) {
        for (int z = 0; z < n; ++z) {
            for (int x = 0; x < n; ++x) {
                if (matches(IconAt(world, x, y, z))) {
                    cells.push_back({x, y, z});
                }
            }
        }
    }
    return cells;
}

// Tables::world_terminal: world N's sign and its completed icon.
bool WorldSignIcons(int worldIndex, std::uint16_t& sign, std::uint16_t& done) {
    if (worldIndex >= 1 && worldIndex <= 8) {
        sign = static_cast<std::uint16_t>(BlockTypes::WorldSelect1 + worldIndex - 1);
        done = static_cast<std::uint16_t>(BlockTypes::WorldDone1 + worldIndex - 1);
        return true;
    }
    if (worldIndex == 9) {
        sign = BlockTypes::WorldSelect9;
        done = BlockTypes::WorldDone9;
        return true;
    }
    if (worldIndex >= 10 && worldIndex <= 14) {
        sign = static_cast<std::uint16_t>(BlockTypes::WorldSelect10 + worldIndex - 10);
        done = static_cast<std::uint16_t>(BlockTypes::WorldDone10 + worldIndex - 10);
        return true;
    }
    return false;
}

bool FreeAndFloored(const Worlds::World& world, const HubCell& cell) {
    return cell.y > 0 && IconAt(world, cell.x, cell.y, cell.z) == 0 && IconAt(world, cell.x, cell.y - 1, cell.z) != 0;
}

float YawFacing(int dx, int dz) {
    return std::atan2(static_cast<float>(dx), static_cast<float>(-dz));
}

} // namespace

std::vector<MissionDoor> FindMissionDoors(const Worlds::World& world, int sign) {
    std::vector<MissionDoor> doors;
    if (sign < 1 || sign > 8) {
        return doors;
    }
    const auto signIcon = static_cast<std::uint16_t>(BlockTypes::MissionSign1 + sign - 1);
    for (const auto& cell : FindCells(world, [&](std::uint16_t icon) { return icon == signIcon; })) {
        for (const auto& axis : kAxes) {
            for (int distance = 1; distance <= 2; ++distance) {
                const HubCell door{cell.x + axis[0] * distance, cell.y, cell.z + axis[1] * distance};
                if (IconAt(world, door.x, door.y, door.z) == BlockTypes::HubDoor &&
                    std::none_of(doors.begin(), doors.end(), [&](const MissionDoor& d) { return d.door == door; })) {
                    doors.push_back({door, cell});
                }
            }
        }
    }
    return doors;
}

std::vector<HubCell> FindGoldDoors(const Worlds::World& world) {
    auto doors = FindCells(world, [](std::uint16_t icon) { return icon == BlockTypes::GoldDoor; });
    std::sort(doors.begin(), doors.end(), [](const HubCell& a, const HubCell& b) {
        if (a.y != b.y) return a.y < b.y;
        if (a.x != b.x) return a.x > b.x;
        return a.z > b.z;
    });
    return doors;
}

std::vector<HubCell> FindWorldSigns(const Worlds::World& world, int worldIndex) {
    std::uint16_t sign = 0, done = 0;
    if (!WorldSignIcons(worldIndex, sign, done)) {
        return {};
    }
    return FindCells(world, [&](std::uint16_t icon) { return icon == sign || icon == done; });
}

std::uint16_t CompletedWorldSignIcon(std::uint16_t icon) noexcept {
    for (int worldIndex = 1; worldIndex <= 14; ++worldIndex) {
        std::uint16_t sign = 0, done = 0;
        if (WorldSignIcons(worldIndex, sign, done) && icon == sign) {
            return done;
        }
    }
    return icon;
}

std::optional<HubArrival> ArrivalBesideSign(const Worlds::World& world, HubCell sign) {
    for (const auto& axis : kAxes) {
        const HubCell cell{sign.x + axis[0], sign.y, sign.z + axis[1]};
        if (FreeAndFloored(world, cell)) {
            return HubArrival{cell, YawFacing(-axis[0], -axis[1])};
        }
    }
    return std::nullopt;
}

std::optional<HubArrival> ArrivalPastDoor(const Worlds::World& world, HubCell door, HubCell sign) {
    const int dx = (door.x > sign.x) - (door.x < sign.x);
    const int dz = (door.z > sign.z) - (door.z < sign.z);
    if ((dx == 0) == (dz == 0)) {
        return std::nullopt; // not beside the sign along one axis
    }
    const HubCell cell{door.x + dx, door.y, door.z + dz};
    if (!FreeAndFloored(world, cell)) {
        return std::nullopt;
    }
    return HubArrival{cell, YawFacing(-dx, -dz)};
}

} // namespace GalaxyEggbert
