#pragma once

#include "NetProtocol.hpp"

#include <GalaxyEggbert/Game/WorldRuntime.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace GalaxyEggbert::Net
{
    // What a player's Blupi changes in the world, sent to the others, and theirs applied here
    // (plans/plan_gemultiplayer.md §7.5; the original's MODIFDECOR, OBJECTDELETE and OBJECTSTART,
    // decmove.cpp:166-200). Each machine runs its own world; around its step:
    //
    //   Apply() every event that arrived, then Snapshot(), then the step, then Diff().
    //
    // The others' changes are applied before the snapshot, so they are never sent back. The
    // world's own objects are known by their index (the same file loaded everywhere); what a
    // player makes (IsReplicatedSpawn) by a net id, its maker's place in the high byte.
    class Replicator
    {
    public:
        // The most objects others' events add to a world (beyond its own); more are refused.
        static constexpr std::size_t kMaxSpawned = 1024;
        // A match begins on the world as loaded; @p slot the local player's place (0-3).
        // @p firstSpawn: the first number of his spawned objects (one back in a match after a drop
        // starts past those he made before, NextSpawnFor()).
        void Begin(const std::vector<Game::MobileObjSpec>& objects, int slot, std::uint32_t firstSpawn = 1);

        void Apply(const WorldEvent& event, Worlds::World& world, std::vector<Game::MobileObjSpec>& objects);
        void Snapshot(const Worlds::World& world, const std::vector<Game::MobileObjSpec>& objects);
        [[nodiscard]] std::vector<WorldEvent> Diff(const Worlds::World& world,
                                                   const std::vector<Game::MobileObjSpec>& objects);

        // Events applied, and those refused (out of the world, an object that is not there).
        // The place (0-3) of the player who made the object at @p index (another's bomb, his shot),
        // -1 for the world's own and ours.
        [[nodiscard]] int OwnerOf(std::size_t index) const
        {
            const auto it = owners_.find(index);
            return it != owners_.end() ? it->second : -1;
        }
        // One back in the match (Galaxy's, Q-022): what changed since the start, as events he applies
        // to the world loaded fresh (@p start, the world as the match began): the blocks changed, the
        // world's objects gone, the spawned ones still there (with their makers' numbers).
        [[nodiscard]] std::vector<WorldEvent> Rejoining(const Worlds::World& start, const Worlds::World& world,
                                                        const std::vector<Game::MobileObjSpec>& objects) const;
        // Past every number of @p slot's spawned objects known here.
        [[nodiscard]] std::uint32_t NextSpawnFor(int slot) const;
        [[nodiscard]] int Applied() const noexcept { return applied_; }
        [[nodiscard]] int Refused() const noexcept { return refused_; }
        // The block changes lost to an overflow of the world's journal (none expected: 256 a step).
        [[nodiscard]] int Missed() const noexcept { return missed_; }

    private:
        std::vector<Def::ObjectType> baseTypes_;
        std::vector<bool> baseActive_;
        std::uint32_t slot_ = 0;
        std::int32_t nextSpawn_ = 1;
        std::uint32_t revision_ = 0;
        std::vector<bool> activeBefore_;
        std::vector<Def::ObjectType> typeBefore_;
        std::unordered_map<std::size_t, std::int32_t> spawnIds_; // object index -> net id (bombs)
        std::unordered_map<std::size_t, int> owners_;           // object index -> the maker's place (another's)
        int applied_ = 0, refused_ = 0, missed_ = 0;
    };
}
