#pragma once

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace GalaxyEggbert::Editor
{
    // One block's before/after value at a raw-grid-space position, part of
    // a EditCommand::Kind::BlockEdit command (see below).
    struct BlockChange
    {
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t z = 0;
        Worlds::Block before;
        Worlds::Block after;
    };

    struct SpawnPointState
    {
        bool present = false;
        std::uint16_t x = 0;
        std::uint16_t y = 0;
        std::uint16_t z = 0;
    };

    // One undoable editor action (plan.md section 6, EDITOR-104). A single
    // tagged struct rather than a command class hierarchy, matching this
    // codebase's consistent preference for plain tagged structs
    // (MobileObjSpec, MoveObjectRecord) over polymorphism for a handful of
    // kinds. One user-visible action == one command: a single block edit
    // is one BlockEdit with one BlockChange; a whole box-fill region
    // (EDITOR-105) is still ONE BlockEdit with N BlockChanges (only the
    // cells that actually changed), so undoing a fill restores the whole
    // region in one step, not block-by-block.
    //
    // A MoveObjectEdit carries the anchor cell plus the record that was
    // stored there before and after the action -- either may be empty
    // (nullopt), which is exactly how place (no before, a record after),
    // remove (a record before, none after) and overwrite/edit (both) are
    // all expressed by the same single kind.
    //
    // A SkyRegionEdit (EDITOR-111) carries just the world-level
    // skyRegion() value before/after -- a single scalar, not a per-cell
    // vector like BlockChange, since exactly one thing changes and it
    // isn't addressed by a grid position at all.
    struct EditCommand
    {
        enum class Kind
        {
            BlockEdit,
            MoveObjectEdit,
            SkyRegionEdit,
            SpawnPointEdit,
            BigDecorEdit,
        };

        Kind kind = Kind::BlockEdit;
        std::vector<BlockChange> blockChanges;

        std::uint16_t objectAnchorX = 0;
        std::uint16_t objectAnchorY = 0;
        std::uint16_t objectAnchorZ = 0;
        std::optional<MoveObjectRecord> objectBefore;
        std::optional<MoveObjectRecord> objectAfter;

        std::uint32_t skyRegionBefore = 0;
        std::uint32_t skyRegionAfter = 0;
        SpawnPointState spawnBefore;
        SpawnPointState spawnAfter;

        std::uint16_t bigDecorAnchorX = 0;
        std::uint16_t bigDecorAnchorY = 0;
        std::uint16_t bigDecorAnchorZ = 0;
        std::optional<BigDecorRecord> bigDecorBefore;
        std::optional<BigDecorRecord> bigDecorAfter;
    };

    // Undo/redo stack of EditCommand actions against a Worlds::World.
    // Mirrors GEBlupiController's own documented preference for a single
    // reusable shape over inventing a new one per feature -- here, that
    // shape is "record before/after per command, replay on Undo/Redo."
    class EditCommandStack
    {
    public:
        // Records @p command as the most recent action -- clears the redo
        // stack (a fresh action invalidates any previously-undone redo
        // history, standard undo/redo semantics) and evicts the oldest
        // undo entry once kMaxDepth is exceeded.
        void Push(EditCommand command);

        // Reverts the most recent not-yet-undone command's changes in
        // @p world (applies each BlockChange::before) and moves it onto
        // the redo stack. Returns false (no-op) if there's nothing to undo.
        bool Undo(Worlds::World& world);

        // Re-applies the most recently undone command's changes in
        // @p world (applies each BlockChange::after) and moves it back
        // onto the undo stack. Returns false (no-op) if there's nothing to
        // redo.
        bool Redo(Worlds::World& world);

        [[nodiscard]] std::size_t UndoCount() const noexcept { return undoStack_.size(); }
        [[nodiscard]] std::size_t RedoCount() const noexcept { return redoStack_.size(); }

    private:
        static constexpr std::size_t kMaxDepth = 200;

        std::vector<EditCommand> undoStack_;
        std::vector<EditCommand> redoStack_;
    };
}
