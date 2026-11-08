#pragma once

#include "EditCommandStack.hpp"
#include <GalaxyEggbert/Game/ObjectDefinitionRegistry.hpp>
#include <array>
#include <cmath>
#include <utility>
#include <vector>

namespace GalaxyEggbert::Editor
{
    [[nodiscard]] inline std::array<std::uint16_t,3> EditorObjectAnchor(const MoveObjectRecord& r)
    {
        return {std::uint16_t(std::floor(r.posStartX)),std::uint16_t(std::floor(r.posStartY)),
                std::uint16_t(std::floor(r.posStartZ))};
    }

    // decdesign.cpp PlaceDecor10: placing TYPE_GOAL or TYPE_CLE replaces the old goal/key.
    // Editor-only: the world data API and existing campaign worlds may retain their contents.
    [[nodiscard]] inline std::vector<MoveObjectRecord> RemoveEditorExitConflicts(
        Worlds::World& world,const MoveObjectRecord& incoming)
    {
        std::vector<MoveObjectRecord> removed;
        if (!Game::GetObjectDefinition(incoming.type).levelExit) return removed;
        const auto destination=EditorObjectAnchor(incoming);
        for (const auto& old : CollectMoveObjects(world))
        {
            const auto anchor=EditorObjectAnchor(old);
            if (Game::GetObjectDefinition(old.type).levelExit && anchor!=destination)
            {
                RemoveMoveObject(world,anchor[0],anchor[1],anchor[2]);
                removed.push_back(old);
            }
        }
        return removed;
    }

    [[nodiscard]] inline EditCommand PlaceObjectForEditor(Worlds::World& world,const MoveObjectRecord& incoming)
    {
        EditCommand group;
        group.kind=EditCommand::Kind::Group;
        for (const auto& old : RemoveEditorExitConflicts(world,incoming))
        {
            const auto a=EditorObjectAnchor(old);
            EditCommand remove;
            remove.kind=EditCommand::Kind::MoveObjectEdit;
            remove.objectAnchorX=a[0]; remove.objectAnchorY=a[1]; remove.objectAnchorZ=a[2];
            remove.objectBefore=old;
            group.children.push_back(std::move(remove));
        }
        const auto a=EditorObjectAnchor(incoming);
        EditCommand place;
        place.kind=EditCommand::Kind::MoveObjectEdit;
        place.objectAnchorX=a[0]; place.objectAnchorY=a[1]; place.objectAnchorZ=a[2];
        for (const auto& old : CollectMoveObjects(world))
            if (EditorObjectAnchor(old)==a) { place.objectBefore=old; break; }
        place.objectAfter=incoming;
        PlaceMoveObject(world,incoming);
        group.children.push_back(std::move(place));
        if (group.children.size()==1) return std::move(group.children.front());
        return group;
    }
}
