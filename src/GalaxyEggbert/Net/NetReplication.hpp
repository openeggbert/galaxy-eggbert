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
        // A match begins on the world as loaded; @p slot the local player's place (0-3).
        void Begin(const std::vector<Game::MobileObjSpec>& objects, int slot);

        void Apply(const WorldEvent& event, Worlds::World& world, std::vector<Game::MobileObjSpec>& objects);
        void Snapshot(const Worlds::World& world, const std::vector<Game::MobileObjSpec>& objects);
        [[nodiscard]] std::vector<WorldEvent> Diff(const Worlds::World& world,
                                                   const std::vector<Game::MobileObjSpec>& objects);

        // Events applied, and those refused (out of the world, an object that is not there).
        [[nodiscard]] int Applied() const noexcept { return applied_; }
        [[nodiscard]] int Refused() const noexcept { return refused_; }
        // The block changes lost to an overflow of the world's journal (none expected: 256 a step).
        [[nodiscard]] int Missed() const noexcept { return missed_; }

    private:
        std::vector<Def::ObjectType> baseTypes_;
        std::uint32_t slot_ = 0;
        std::int32_t nextSpawn_ = 1;
        std::uint32_t revision_ = 0;
        std::vector<bool> activeBefore_;
        std::vector<Def::ObjectType> typeBefore_;
        std::unordered_map<std::size_t, std::int32_t> spawnIds_; // object index -> net id (bombs)
        int applied_ = 0, refused_ = 0, missed_ = 0;
    };
}
