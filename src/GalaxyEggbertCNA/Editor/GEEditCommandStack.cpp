#include "GEEditCommandStack.hpp"

#include <utility>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // Restores whichever MoveObjectRecord (or none) a MoveObjectEdit
        // recorded for its anchor cell. RemoveMoveObject() first in both
        // directions, so an overwrite (both before and after present) can't
        // leave a stale record behind if the new one happens to anchor
        // elsewhere -- PlaceMoveObject() anchors at floor(posStart), which
        // need not equal the command's own anchor cell after an EDITOR-110
        // position edit.
        void ApplyMoveObjectState(Worlds::World& world, const GEEditCommand& command,
                                  const std::optional<MoveObjectRecord>& state)
        {
            RemoveMoveObject(world, command.objectAnchorX, command.objectAnchorY, command.objectAnchorZ);
            if (state.has_value())
            {
                PlaceMoveObject(world, *state);
            }
        }

        void ApplySpawnPointState(
            Worlds::World& world, const SpawnPointState& state)
        {
            if (state.present)
            {
                world.setSpawnPoint(state.x, state.y, state.z);
            }
            else
            {
                world.clearSpawnPoint();
            }
        }

        void ApplyBigDecorState(
            Worlds::World& world, const GEEditCommand& command,
            const std::optional<BigDecorRecord>& state)
        {
            RemoveBigDecor(
                world, command.bigDecorAnchorX, command.bigDecorAnchorY,
                command.bigDecorAnchorZ);
            if (state)
            {
                PlaceBigDecor(world, *state);
            }
        }
    }

    void GEEditCommandStack::Push(GEEditCommand command)
    {
        redoStack_.clear();
        undoStack_.push_back(std::move(command));
        if (undoStack_.size() > kMaxDepth)
        {
            undoStack_.erase(undoStack_.begin());
        }
    }

    bool GEEditCommandStack::Undo(Worlds::World& world)
    {
        if (undoStack_.empty())
        {
            return false;
        }
        GEEditCommand command = std::move(undoStack_.back());
        undoStack_.pop_back();

        if (command.kind == GEEditCommand::Kind::BlockEdit)
        {
            for (const auto& change : command.blockChanges)
            {
                world.setBlock(change.x, change.y, change.z, change.before);
            }
        }
        else if (command.kind == GEEditCommand::Kind::MoveObjectEdit)
        {
            ApplyMoveObjectState(world, command, command.objectBefore);
        }
        else if (command.kind == GEEditCommand::Kind::SkyRegionEdit)
        {
            world.setSkyRegion(command.skyRegionBefore);
        }
        else if (command.kind == GEEditCommand::Kind::SpawnPointEdit)
        {
            ApplySpawnPointState(world, command.spawnBefore);
        }
        else if (command.kind == GEEditCommand::Kind::BigDecorEdit)
        {
            ApplyBigDecorState(world, command, command.bigDecorBefore);
        }

        redoStack_.push_back(std::move(command));
        return true;
    }

    bool GEEditCommandStack::Redo(Worlds::World& world)
    {
        if (redoStack_.empty())
        {
            return false;
        }
        GEEditCommand command = std::move(redoStack_.back());
        redoStack_.pop_back();

        if (command.kind == GEEditCommand::Kind::BlockEdit)
        {
            for (const auto& change : command.blockChanges)
            {
                world.setBlock(change.x, change.y, change.z, change.after);
            }
        }
        else if (command.kind == GEEditCommand::Kind::MoveObjectEdit)
        {
            ApplyMoveObjectState(world, command, command.objectAfter);
        }
        else if (command.kind == GEEditCommand::Kind::SkyRegionEdit)
        {
            world.setSkyRegion(command.skyRegionAfter);
        }
        else if (command.kind == GEEditCommand::Kind::SpawnPointEdit)
        {
            ApplySpawnPointState(world, command.spawnAfter);
        }
        else if (command.kind == GEEditCommand::Kind::BigDecorEdit)
        {
            ApplyBigDecorState(world, command, command.bigDecorAfter);
        }

        undoStack_.push_back(std::move(command));
        return true;
    }
}
