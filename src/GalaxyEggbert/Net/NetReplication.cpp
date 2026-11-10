#include "NetReplication.hpp"

#include "NetRules.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <tuple>

namespace GalaxyEggbert::Net
{
    void Replicator::Begin(const std::vector<Game::MobileObjSpec>& objects, int slot)
    {
        baseTypes_.clear();
        for (const auto& object : objects) baseTypes_.push_back(object.type);
        slot_ = static_cast<std::uint32_t>(slot & 0x7f);
        nextSpawn_ = 1;
        revision_ = 0;
        activeBefore_.clear();
        typeBefore_.clear();
        spawnIds_.clear();
        applied_ = refused_ = missed_ = 0;
    }

    void Replicator::Apply(const WorldEvent& event, Worlds::World& world, std::vector<Game::MobileObjSpec>& objects)
    {
        switch (event.kind)
        {
        case EventKind::BlockChanged:
        {
            const int n = static_cast<int>(world.blocksPerAxis());
            if (event.a >= n || event.b >= n || event.c >= n)
            {
                ++refused_;
                return;
            }
            world.setBlock(static_cast<std::uint16_t>(event.a), static_cast<std::uint16_t>(event.b),
                           static_cast<std::uint16_t>(event.c), Worlds::Block(static_cast<std::uint16_t>(event.d)));
            ++applied_;
            return;
        }
        case EventKind::ObjectRetired:
        {
            // The world's own object, still itself here (its slot not reused by something else).
            const auto index = static_cast<std::size_t>(event.id);
            if (index >= objects.size() || index >= baseTypes_.size() || objects[index].type != baseTypes_[index] ||
                spawnIds_.count(index) != 0)
            {
                ++refused_;
                return;
            }
            objects[index].active = false;
            ++applied_;
            return;
        }
        case EventKind::ObjectSpawned:
        {
            if (!IsReplicatedSpawn(event.a))
            {
                ++refused_;
                return;
            }
            Game::MobileObjSpec spec;
            spec.type = static_cast<Def::ObjectType>(event.a);
            spec.posStartX = spec.posEndX = spec.currentX = event.x;
            spec.posStartY = spec.posEndY = spec.currentY = event.y;
            spec.posStartZ = spec.posEndZ = spec.currentZ = event.z;
            spec.phase = 0.0f;
            spec.active = true;
            if (spec.type == Def::ObjectType::ObjectType23)
            {
                // Another player's shot: on its way (InteractionSystem's MakeBullet), and his, not
                // ours -- it kills the Blupi of this machine it meets.
                spec.posEndX = static_cast<float>(event.b) / 256.0f;
                spec.posEndY = static_cast<float>(event.c) / 256.0f;
                spec.posEndZ = static_cast<float>(event.d) / 256.0f;
                const float dist = std::max({std::fabs(spec.posEndX - spec.posStartX), std::fabs(spec.posEndY - spec.posStartY),
                                             std::fabs(spec.posEndZ - spec.posStartZ)});
                spec.patrolStep = 2;
                spec.patrolTime = 0.0f;
                spec.stepAdvanceTicks = 5.0f * dist;
                spec.firedByBlupi = false;
            }
            // As the game places what Blupi lays: the first free slot (InteractionSystem::TryPerso).
            std::size_t index = objects.size();
            for (std::size_t i = 0; i < objects.size(); ++i)
            {
                if (!objects[i].active)
                {
                    index = i;
                    break;
                }
            }
            if (index == objects.size()) objects.push_back(spec);
            else objects[index] = spec;
            for (auto it = spawnIds_.begin(); it != spawnIds_.end();)
                it = it->first == index ? spawnIds_.erase(it) : std::next(it);
            if (IsPersistentSpawn(event.a)) spawnIds_[index] = event.id;
            ++applied_;
            return;
        }
        case EventKind::SpawnRetired:
            for (auto it = spawnIds_.begin(); it != spawnIds_.end(); ++it)
            {
                if (it->second != event.id) continue;
                if (it->first < objects.size()) objects[it->first].active = false;
                spawnIds_.erase(it);
                ++applied_;
                return;
            }
            ++refused_;
            return;
        }
    }

    void Replicator::Snapshot(const Worlds::World& world, const std::vector<Game::MobileObjSpec>& objects)
    {
        revision_ = world.blockRevision();
        activeBefore_.resize(objects.size());
        typeBefore_.resize(objects.size());
        for (std::size_t i = 0; i < objects.size(); ++i)
        {
            activeBefore_[i] = objects[i].active;
            typeBefore_[i] = objects[i].type;
        }
    }

    std::vector<WorldEvent> Replicator::Diff(const Worlds::World& world, const std::vector<Game::MobileObjSpec>& objects)
    {
        std::vector<WorldEvent> events;
        if (world.blockRevision() != revision_)
        {
            const auto changes = world.blockChangesSince(revision_);
            if (!changes.known)
            {
                ++missed_;
            }
            else
            {
                // A cell changed twice in a step is sent as it ends.
                std::map<std::tuple<int, int, int>, std::uint16_t> last;
                for (const auto& cell : changes.cells) last[{cell.x, cell.y, cell.z}] = cell.newRaw;
                for (const auto& [where, raw] : last)
                {
                    WorldEvent e;
                    e.kind = EventKind::BlockChanged;
                    e.a = std::get<0>(where);
                    e.b = std::get<1>(where);
                    e.c = std::get<2>(where);
                    e.d = raw;
                    events.push_back(e);
                }
            }
            revision_ = world.blockRevision();
        }
        for (std::size_t i = 0; i < objects.size(); ++i)
        {
            const bool wasActive = i < activeBefore_.size() && activeBefore_[i];
            const auto wasType = i < typeBefore_.size() ? typeBefore_[i] : Def::ObjectType{};
            const bool active = objects[i].active;
            const auto type = objects[i].type;
            const bool gone = wasActive && (!active || type != wasType);
            if (const auto spawned = spawnIds_.find(i); spawned != spawnIds_.end())
            {
                if (gone)
                {
                    WorldEvent e;
                    e.kind = EventKind::SpawnRetired;
                    e.id = spawned->second;
                    events.push_back(e);
                    spawnIds_.erase(spawned);
                }
            }
            else if (gone && i < baseTypes_.size() && wasType == baseTypes_[i])
            {
                WorldEvent e;
                e.kind = EventKind::ObjectRetired;
                e.id = static_cast<std::int32_t>(i);
                events.push_back(e);
            }
            // A shot is sent when it is his own (the others' arrive as theirs, not his).
            const bool ownShot = type != Def::ObjectType::ObjectType23 || objects[i].firedByBlupi;
            if (active && IsReplicatedSpawn(static_cast<int>(type)) && ownShot && (!wasActive || type != wasType))
            {
                WorldEvent e;
                e.kind = EventKind::ObjectSpawned;
                e.id = static_cast<std::int32_t>((slot_ << 24) | static_cast<std::uint32_t>(nextSpawn_++ & 0xffffff));
                e.a = static_cast<std::int32_t>(type);
                e.x = objects[i].currentX;
                e.y = objects[i].currentY;
                e.z = objects[i].currentZ;
                if (type == Def::ObjectType::ObjectType23)
                {
                    e.b = static_cast<std::int32_t>(std::lround(objects[i].posEndX * 256.0f));
                    e.c = static_cast<std::int32_t>(std::lround(objects[i].posEndY * 256.0f));
                    e.d = static_cast<std::int32_t>(std::lround(objects[i].posEndZ * 256.0f));
                }
                events.push_back(e);
                if (IsPersistentSpawn(e.a)) spawnIds_[i] = e.id;
            }
        }
        Snapshot(world, objects);
        return events;
    }
}
