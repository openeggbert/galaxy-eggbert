#include "EditCommandStack.hpp"

#include <GalaxyEggbert/Game/PlateRotationMetadata.hpp>

#include <utility>

namespace GalaxyEggbert::Editor
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
        void ApplyMoveObjectState(Worlds::World& world, const EditCommand& command,
                                  const std::optional<MoveObjectRecord>& state)
        {
            RemoveMoveObject(world, command.objectAnchorX, command.objectAnchorY, command.objectAnchorZ);
            if (state.has_value())
            {
                PlaceMoveObject(world, *state);
            }
        }

        void ApplySpawnPointState(
            Worlds::World& world, const SpawnPointState& state, PlayerStartColor color)
        {
            if (state.present)
            {
                SetPlayerStart(world,{color,state.x,state.y,state.z,state.quarterTurns});
            }
            else
            {
                ClearPlayerStart(world,color);
            }
        }

        void ApplyBigDecorState(
            Worlds::World& world, const EditCommand& command,
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

    void EditCommandStack::Push(EditCommand command)
    {
        redoStack_.clear();
        undoStack_.push_back(std::move(command));
        if (undoStack_.size() > kMaxDepth)
        {
            undoStack_.erase(undoStack_.begin());
        }
    }

    namespace
    {
        // Applies @p command's before (forward false) or after (forward true) state.
        void Apply(Worlds::World& world, const EditCommand& command, bool forward)
        {
            switch (command.kind)
            {
            case EditCommand::Kind::BlockEdit:
                for (const auto& change : command.blockChanges)
                {
                    world.setBlock(change.x, change.y, change.z, forward ? change.after : change.before);
                }
                break;
            case EditCommand::Kind::MoveObjectEdit:
                ApplyMoveObjectState(world, command, forward ? command.objectAfter : command.objectBefore);
                break;
            case EditCommand::Kind::SkyRegionEdit:
                world.setSkyRegion(forward ? command.skyRegionAfter : command.skyRegionBefore);
                break;
            case EditCommand::Kind::MusicEdit:
                world.setMusicTrack(forward ? command.musicAfter : command.musicBefore);
                break;
            case EditCommand::Kind::SpawnPointEdit:
                ApplySpawnPointState(world, forward ? command.spawnAfter : command.spawnBefore, command.spawnColor);
                break;
            case EditCommand::Kind::BigDecorEdit:
                ApplyBigDecorState(world, command, forward ? command.bigDecorAfter : command.bigDecorBefore);
                break;
            case EditCommand::Kind::TurnEdit:
                (command.turnObject ? Game::SetObjectQuarterTurns : Game::SetQuarterTurns)(
                    world, command.turnX, command.turnY, command.turnZ, forward ? command.turnAfter : command.turnBefore);
                break;
            case EditCommand::Kind::Group:
                if (forward)
                {
                    for (const auto& child : command.children)
                    {
                        Apply(world, child, true);
                    }
                }
                else
                {
                    for (auto child = command.children.rbegin(); child != command.children.rend(); ++child)
                    {
                        Apply(world, *child, false);
                    }
                }
                break;
            case EditCommand::Kind::MetadataEdit:
            {
                const auto& value=forward ? command.metadataAfter : command.metadataBefore;
                if (value) world.setBlockExtraMetadata(command.metadataX,command.metadataY,command.metadataZ,
                    command.metadataType,*value);
                else world.removeBlockExtraMetadata(command.metadataX,command.metadataY,command.metadataZ,
                    command.metadataType);
                break;
            }
            }
        }
    }

    bool EditCommandStack::Undo(Worlds::World& world)
    {
        if (undoStack_.empty())
        {
            return false;
        }
        EditCommand command = std::move(undoStack_.back());
        undoStack_.pop_back();
        Apply(world, command, false);
        redoStack_.push_back(std::move(command));
        return true;
    }

    bool EditCommandStack::Redo(Worlds::World& world)
    {
        if (redoStack_.empty())
        {
            return false;
        }
        EditCommand command = std::move(redoStack_.back());
        redoStack_.pop_back();
        Apply(world, command, true);
        undoStack_.push_back(std::move(command));
        return true;
    }
}
