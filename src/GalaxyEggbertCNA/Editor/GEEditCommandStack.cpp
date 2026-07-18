#include "GEEditCommandStack.hpp"

#include <utility>

namespace GalaxyEggbert::CNA
{
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

        undoStack_.push_back(std::move(command));
        return true;
    }
}
