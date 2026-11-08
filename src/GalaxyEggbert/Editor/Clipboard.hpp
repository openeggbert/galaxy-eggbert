#pragma once

#include "BoxRegion.hpp"
#include "EditCommandStack.hpp"

#include <GalaxyEggbert/BigDecorRecord.hpp>
#include <GalaxyEggbert/MoveObjectRecord.hpp>
#include <GalaxyEggbert/Worlds/Block.hpp>
#include <GalaxyEggbert/Worlds/World.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace GalaxyEggbert::Editor
{
    // The editor's copy and paste (ED-0010; the owner chose it from the editor proposals on
    // 2026-11-06): a box of the world -- its blocks, their turns (the plate turns, metadata type 2;
    // an object's, a model's or a BigDecor's, type 5), the MoveObjects and the BigDecor anchored in
    // it -- stamped elsewhere, as it was, turned a quarter about the vertical or mirrored. The
    // original editor has no copy (decdesign.cpp places one cell at a time): an editor addition
    // only, which places nothing a click could not.
    //
    // Not carried: the hint zones (type 4, the campaign's) and the spawn point (one per world).
    struct Clipboard
    {
        struct Cell
        {
            Worlds::Block block;
            int plateTurns = -1;  // the cell's type-2 byte (0-3), -1 when it has none
            int objectTurns = -1; // its type-5 byte, -1 when it has none
            int legacySwitchRow = -1; // type6 compatibility link, -1 when absent
        };
        struct Decor
        {
            std::uint16_t icon = 0;
            int x = 0; // from the box's min corner
            int y = 0;
            int z = 0;
        };

        int sizeX = 0;
        int sizeY = 0;
        int sizeZ = 0;
        std::vector<Cell> cells;               // [Index(x, y, z)]
        std::vector<MoveObjectRecord> objects; // posStart/posEnd from the box's min corner
        std::vector<Decor> bigDecor;
        int quarterTurns = 0; // turned since copied, 0-3 (the status line; the re-tiling)
        bool mirrored = false;

        [[nodiscard]] bool Empty() const noexcept { return cells.empty(); }
        [[nodiscard]] std::size_t Index(int x, int y, int z) const noexcept
        {
            return (static_cast<std::size_t>(x) * static_cast<std::size_t>(sizeY) + static_cast<std::size_t>(y)) *
                       static_cast<std::size_t>(sizeZ) +
                   static_cast<std::size_t>(z);
        }
        // Turned or mirrored from the world it was copied from: its edge pieces need re-tiling.
        [[nodiscard]] bool Transformed() const noexcept { return quarterTurns % 4 != 0 || mirrored; }
    };

    // The box @p box of @p world (inclusive, in the world).
    [[nodiscard]] Clipboard CopyBox(const Worlds::World& world, const BoxRegion& box);

    // A quarter turn about the vertical, the way O turns a model ((x, z) -> (z, -x), as
    // WorldRuntime::SolidBigDecorBoxes turns a footprint): the cells and the objects' paths move;
    // a plate's, fan's or wind tile's axis swaps (a side fan or its wind blowing along X becomes
    // its twin of the other way, so that it blows where the turn takes it); an object or a BigDecor
    // turns a quarter with it. A model with a front (a sign, a teleporter) keeps its turns: it faces
    // its cell's open side by itself (TerrainRenderer's openSide), which turns with the copy.
    [[nodiscard]] Clipboard TurnedQuarter(const Clipboard& clipboard);

    // Mirrored left to right (X): the turns are reversed (a quarter one way becomes a quarter the
    // other), a side fan or its wind blowing along X becomes its twin. The models themselves are
    // not mirrored (a sign's painting stays readable).
    [[nodiscard]] Clipboard MirroredX(const Clipboard& clipboard);

    // Stamps @p clipboard with its min corner at (x, y, z), the cells outside the world left out,
    // and returns the undo command (a Group: the blocks, the turns, the objects, the BigDecor; no
    // children when nothing changed). The box is replaced as a whole -- its empty cells empty the
    // world, the objects and BigDecor anchored in it before go -- unless @p keepUnderAir, which
    // leaves what is under the clipboard's empty cells. The edge pieces round the stamp are re-tiled
    // as the editor re-tiles a placed cell (ED-0007); a turned or mirrored stamp re-tiles all its
    // cells, its edges having changed sides. @p next is the re-tiling's dice (AutoTile.hpp).
    [[nodiscard]] EditCommand PasteBox(Worlds::World& world, const Clipboard& clipboard, int x, int y, int z,
                                       bool keepUnderAir, const std::function<int()>& next);
}
