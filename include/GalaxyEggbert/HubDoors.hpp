#pragma once

#include "GalaxyEggbert/Worlds/World.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace GalaxyEggbert {

// The hubs' doors and signs as Mobile Eggbert finds them (Decor::SearchDoor(), SearchGold(),
// SearchWorld(), AdaptDoors()), on a 3D world: where the original looks left and right along
// its row, these look along X and Z at the same height. Raw grid cells (World's coordinates).
// Icons: BlockTypes.hpp (HubDoor 182, GoldDoor 183, the signs).
struct HubCell {
    int x = 0, y = 0, z = 0;
    friend bool operator==(const HubCell&, const HubCell&) = default;
};

// Where Blupi starts in a hub: a free cell with a floor under it, facing `yaw` (radians,
// BlupiController's convention: he moves along (sin yaw, -cos yaw)).
struct HubArrival {
    HubCell cell;
    float yaw = 0.0f;
};

// A door of a world hub and the mission sign it belongs to.
struct MissionDoor {
    HubCell door;
    HubCell sign;
};

// The doors (HubDoor) of mission sign `sign` (1-8, icons 174-181): every HubDoor one or two
// cells from one of that sign's cells along X or Z, at its height.
std::vector<MissionDoor> FindMissionDoors(const Worlds::World& world, int sign);

// The gold bars (GoldDoor) in the order the main hub opens them, one per completed world:
// the lowest first, then from +X, then from +Z (SearchGold() scans from the map's bottom right).
std::vector<HubCell> FindGoldDoors(const Worlds::World& world);

// The cells holding world `worldIndex`'s sign (1-14), in either of its icons.
std::vector<HubCell> FindWorldSigns(const Worlds::World& world, int worldIndex);

// The icon a world sign shows once its world is completed (166-173, 310, 416-420), or `icon`
// itself when it is no world sign or already shows that.
std::uint16_t CompletedWorldSignIcon(std::uint16_t icon) noexcept;

// Beside a world sign (SearchWorld()): the first free, floored neighbour along -X, +X, -Z, +Z,
// facing the sign.
std::optional<HubArrival> ArrivalBesideSign(const Worlds::World& world, HubCell sign);

// What Decor::AdaptDoors() opens in a hub, as it loads and each time the opendoors cheat is
// toggled (Decor::CheatAction(OpenDoors) flips m_bCheatDoors and adapts again). `unlocked(index)`
// reads m_doors[index]: 180 + world in the main hub (mission 1), mission + sign in a world hub
// (mission % 10 == 0); `cheatDoors` opens them all. The main hub opens one gold bar per completed
// world (FindGoldDoors()'s order) and shows the signs of completed worlds completed; a world hub
// opens the doors of every mission unlocked. A door already opened is gone from the world, so a
// second call finds only those still shut. Any other mission: nothing (AdaptDoors() returns).
struct HubAdaptation {
    std::vector<HubCell> goldDoors;         // main hub: the bars to slide up
    std::vector<HubCell> completedSigns;    // main hub: the signs to show completed
    std::vector<MissionDoor> missionDoors;  // world hub: the doors to slide up, in sign order
};
HubAdaptation AdaptHubDoors(const Worlds::World& world, int mission, const std::function<bool(int)>& unlocked,
                            bool cheatDoors);

// Past an opened door (SearchDoor()): the cell beyond `door` on the side away from `sign`,
// facing the door, when it is free and floored.
std::optional<HubArrival> ArrivalPastDoor(const Worlds::World& world, HubCell door, HubCell sign);

} // namespace GalaxyEggbert
