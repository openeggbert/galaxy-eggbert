#pragma once

// The original editor's auto-tiling, in 3D (ED-0007; the owner's MR-0005, 2026-11-02: fully, right
// and left read from both X and Z).
//
// The original (Decor::AdaptBorder, Decor.cpp:10930-10972) re-tiles a placed or emptied cell and its
// neighbours with Decor::AdaptMidBorder (10721-10840): a mask of four bits (1 below, 2 above, 4 right,
// 8 left) starts full and loses each side whose neighbour is no "border" by Decor::IsRightBorder
// (10367-10675, ported below rule by rule, in its own order); the cell's icon is taken back to its
// family's base and the variant for the mask read from Tables::table_adapt_decor (Tables.cpp:1668,
// nine families of sixteen, transcribed under the standing approval for small data tables), a few
// variants swapped at random for their twins. An emitter of lightning (304) grows its bolt (305)
// into up to four empty cells under it; a cell emptied takes away the bolt under it.
//
// In 3D below and above are Y; a side counts as bordered when its neighbour on X or on Z is (right
// +X or +Z, left -X or -Z), and the neighbours re-tiled are the six round the cell.
//
// Not carried over: the edges the original lays in EMPTY cells round a mass of cheese or of cave
// rock (table_adapt_fromage, 264-282 and 285-303): in 3D they would stand in the air before every
// face of the mass, every corridor's (the edge plates a world places are their scenes' reliefs
// since Q-004/Q-012, but laid round every mass they would fill its passages), where the mass's own
// faces show its scenes' lumps;
// and the original's deletion of a moving object in a cell made solid (the editor's objects have
// their own commands).

#include "EditCommandStack.hpp"

#include <GalaxyEggbert/Worlds/World.hpp>

#include <array>
#include <cstdint>
#include <vector>

namespace GalaxyEggbert::Editor::AutoTile
{
    inline constexpr int kEmpty = -1; // the original's empty cell (Galaxy's air)

    inline constexpr std::array<std::int16_t, 144> kAdaptDecor = {
        153, 147, 148, 146, 40,  151, 150, 144, 39,  152, 149, 145, 38,  36,  37,  35,  // rock
        153, 147, 148, 146, 40,  151, 150, 144, 39,  152, 149, 145, 38,  36,  37,  156, // rock'
        75,  75,  155, 155, 75,  75,  155, 155, 74,  74,  154, 154, 75,  75,  155, 155, // girders
        157, 157, 157, 157, 109, 109, 109, 109, 108, 108, 108, 108, 107, 107, 107, 107, // grass
        202, 202, 138, 138, 202, 202, 138, 138, 202, 202, 138, 138, 202, 202, 138, 138, // bars
        283, 283, 283, 283, 262, 262, 262, 262, 263, 263, 263, 263, 261, 261, 261, 261, // brick
        361, 359, 360, 352, 357, 353, 355, 349, 358, 354, 356, 350, 351, 347, 348, 341, // slime
        387, 387, 386, 386, 394, 394, 392, 392, 395, 395, 393, 393, 397, 397, 396, 396, // palace
        251, 254, 254, 254, 251, 258, 260, 258, 251, 257, 259, 257, 251, 250, 256, 250, // pipes
    };

    // Decor::IsRightBorder: whether the neighbour (its icon, kEmpty for air) is a border of the
    // cell being re-tiled (its icon `centre`), (dx, dy) the 2D step from the neighbour to the cell
    // (dy +1: the cell is under the neighbour; dx -1: the cell is left of it).
    [[nodiscard]] inline bool IsRightBorder(int neighbour, int centre, int dx, int dy) noexcept
    {
        const auto tail = [&]() {
            if ((neighbour < 386 || neighbour > 397) && neighbour != 400)
            {
                return neighbour == 245;
            }
            return true;
        };
        if (centre >= 386 && centre <= 397)
        {
            return tail();
        }
        if (centre == 245 || centre == 400)
        {
            return tail();
        }
        if (centre >= 250 && centre <= 260)
        {
            return neighbour >= 250 && neighbour <= 260;
        }
        const int n = neighbour;
        switch (n)
        {
        case kEmpty:
        case 32:
        case 33:
        case 34:
            return false;
        case 68:
        case 92:
            return dy != -1;
        case 317:
            return dy != 1;
        case 19:
        case 21:
        case 25:
        case 26:
        case 28:
            return dy != 1;
        case 27:
            return dx == 0;
        case 45:
        case 46:
        case 47:
            return dy != 1;
        case 15:
        case 16:
        case 75:
        case 89:
        case 90:
        case 155:
            return !(dx == -1 || dy == 1);
        case 17:
        case 18:
        case 74:
        case 87:
        case 88:
        case 154:
            return !(dx == 1 || dy == 1);
        case 76:
        case 77:
        case 199:
        case 200:
            return dx == 0;
        default:
            break;
        }
        if (n >= 110 && n <= 125)
        {
            return false;
        }
        switch (n)
        {
        case 126:
            return dx == 1;
        case 129:
            return dx == -1;
        case 132:
            return dy == 1;
        case 135:
            return dy == -1;
        case 139:
        case 140:
        case 141:
        case 142:
        case 143:
            return false;
        case 138:
            return dy == -1;
        case 202:
            return false;
        case 107:
        case 108:
        case 109:
        case 157:
            return dx != 0;
        default:
            break;
        }
        // The signs (158-181, 309, 310, 410-420): a border only over the cell.
        if ((n >= 158 && n <= 181) || n == 309 || n == 310 || (n >= 410 && n <= 420))
        {
            return dy == 1;
        }
        if (n == 182 || n == 183 || (n >= 334 && n <= 336) || (n >= 421 && n <= 440))
        {
            return dy != 0;
        }
        if ((n >= 250 && n <= 260) || (n >= 264 && n <= 282) || n == 378 || n == 404)
        {
            return false;
        }
        return true;
    }

    // A cell's icon as the original reads it: kEmpty for air.
    [[nodiscard]] inline int IconAt(const Worlds::World& world, int x, int y, int z) noexcept
    {
        const auto block = world.getBlock(static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                                          static_cast<std::uint16_t>(z));
        return block.isAir() ? kEmpty : static_cast<int>(block.type());
    }

    [[nodiscard]] inline bool InWorld(const Worlds::World& world, int x, int y, int z) noexcept
    {
        const int n = static_cast<int>(world.blocksPerAxis());
        return x >= 0 && y >= 0 && z >= 0 && x < n && y < n && z < n;
    }

    // Sets a cell and keeps one change per cell in `changes` (the first `before`, the last `after`):
    // undo restores the befores in order, so a cell must not appear twice.
    inline void Record(Worlds::World& world, int x, int y, int z, int icon, std::vector<BlockChange>& changes)
    {
        const auto ux = static_cast<std::uint16_t>(x);
        const auto uy = static_cast<std::uint16_t>(y);
        const auto uz = static_cast<std::uint16_t>(z);
        const Worlds::Block before = world.getBlock(ux, uy, uz);
        const Worlds::Block after =
            icon == kEmpty ? Worlds::Block::air() : Worlds::Block::make(static_cast<std::uint16_t>(icon));
        if (before == after)
        {
            return;
        }
        world.setBlock(ux, uy, uz, after);
        for (auto& change : changes)
        {
            if (change.x == ux && change.y == uy && change.z == uz)
            {
                change.after = after;
                return;
            }
        }
        changes.push_back({ux, uy, uz, before, after});
    }

    // Decor::AdaptMidBorder's first part, in 3D. `next` returns the original's m_random->Next()
    // (any non-negative int); Next(a, b) is a + next() % (b - a), b excluded as the original's.
    template <class Next>
    void AdaptMid(Worlds::World& world, int x, int y, int z, Next&& next, std::vector<BlockChange>& changes)
    {
        if (!InWorld(world, x, y, z))
        {
            return;
        }
        const int centre = IconAt(world, x, y, z);
        const auto border = [&](int nx, int ny, int nz, int dx, int dy) {
            if (!InWorld(world, nx, ny, nz))
            {
                return true;
            }
            return IsRightBorder(IconAt(world, nx, ny, nz), centre, dx, dy);
        };
        int mask = 15;
        if (!border(x, y - 1, z, 0, -1))
        {
            mask &= ~1; // below
        }
        if (!border(x, y + 1, z, 0, 1))
        {
            mask &= ~2; // above
        }
        if (!border(x + 1, y, z, -1, 0) && !border(x, y, z + 1, -1, 0))
        {
            mask &= ~4; // right: +X or +Z
        }
        if (!border(x - 1, y, z, 1, 0) && !border(x, y, z - 1, 1, 0))
        {
            mask &= ~8; // left: -X or -Z
        }
        int base = centre;
        if (base == 156)
        {
            base = 35;
        }
        if (base == 252 || base == 253)
        {
            base = 251;
        }
        if (base == 255)
        {
            base = 254;
        }
        if (base == 362)
        {
            base = 347;
        }
        if (base == 363)
        {
            base = 348;
        }
        if (base >= 341 && base <= 346)
        {
            base = 341;
        }
        for (std::size_t i = 0; i < kAdaptDecor.size(); ++i)
        {
            if (base != kAdaptDecor[i])
            {
                continue;
            }
            int icon = kAdaptDecor[i / 16 * 16 + static_cast<std::size_t>(mask)];
            const auto coin = [&]() { return next() % 2 == 0; };
            if (icon == 35 && coin())
            {
                icon = 156;
            }
            if (icon == 251)
            {
                icon = 251 + next() % 2; // Next(251, 253)
            }
            if (icon == 254 && coin())
            {
                icon = 255;
            }
            if (icon == 347 && coin())
            {
                icon = 362;
            }
            if (icon == 348 && coin())
            {
                icon = 363;
            }
            if (icon == 341)
            {
                icon = 341 + next() % 5; // Next(341, 346)
            }
            Record(world, x, y, z, icon, changes);
            return;
        }
    }

    // Decor::AdaptBorder, in 3D: the cell and its six neighbours re-tiled; the lightning column.
    template <class Next>
    void AdaptBorder(Worlds::World& world, int x, int y, int z, Next&& next, std::vector<BlockChange>& changes)
    {
        AdaptMid(world, x, y, z, next, changes);
        AdaptMid(world, x + 1, y, z, next, changes);
        AdaptMid(world, x - 1, y, z, next, changes);
        AdaptMid(world, x, y, z + 1, next, changes);
        AdaptMid(world, x, y, z - 1, next, changes);
        AdaptMid(world, x, y - 1, z, next, changes);
        AdaptMid(world, x, y + 1, z, next, changes);
        if (!InWorld(world, x, y, z))
        {
            return;
        }
        int cy = y;
        int icon = IconAt(world, x, cy, z);
        if (icon == 304)
        {
            for (int i = 0; i < 4; ++i)
            {
                --cy; // the original's cel.Y++: one cell down
                if (cy < 0)
                {
                    break;
                }
                icon = IconAt(world, x, cy, z);
                if (icon != kEmpty)
                {
                    break;
                }
                Record(world, x, cy, z, 305, changes);
            }
        }
        if (icon != kEmpty)
        {
            return;
        }
        for (int i = 0; i < 4; ++i)
        {
            --cy;
            if (cy < 0)
            {
                break;
            }
            if (IconAt(world, x, cy, z) != 305)
            {
                break;
            }
            Record(world, x, cy, z, kEmpty, changes);
        }
    }
}
