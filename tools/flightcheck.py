#!/usr/bin/env python3
"""Reachability with the helicopter: levelcheck.py's walking search plus flights.

levelcheck.py walks, falls, jumps, rides lifts and takes teleporters; this adds the helicopter
(ObjectType 13). Blupi takes a parked helicopter standing in or next to its cell, flies through
free cells -- two high, his feet's cell and his head's, in any of the six directions -- never
into water (the original loses the helicopter there, Decor.cpp's ByeByeHelico on IsSurfWater/
IsDeepWater) nor next to a mine or an enemy (levelcheck's danger cells, patrols swept whole),
and leaves it on any floor he can stand on; it stays parked there for him to take again.
Flights stay within two cells of the world's blocks (the open sky round a world is no route).
Treasures, keys, eggs and the exit count when reached on foot from the spawn, or with a
helicopter (in flight, or walking on from where he landed); one whose own cell is not free (an
object sunk into a wall) is reported. Like levelcheck it is conservative about what it models:
closed doors stay closed, timed enemies are always in the way. For gates it cannot model use a
staged variant of the world's script.

    python3 tools/flightcheck.py worlds3d/world041.vwr [...]
"""

from __future__ import annotations

import argparse
import collections
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import levelcheck as lc  # noqa: E402
import vwr_inspect  # noqa: E402

HELICOPTER = 13
SIX = [(1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]


def flyable(grid: lc.Grid, p) -> bool:
    x, y, z = p
    head = (x, y + 1, z)
    lo, hi = grid.bounds
    if not all(lo[i] <= p[i] <= hi[i] for i in range(3)):
        return False                       # out in the open, far from everything
    return (grid.free(p) and grid.free(head) and grid.safe(p) and head not in grid.danger and
            not grid.info(p)["water"] and not grid.info(head)["water"])


def near(cell, cells) -> bool:
    x, y, z = cell
    return any((x + dx, y + dy, z + dz) in cells for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1))


def search(grid: lc.Grid, spawn):
    """(cells reached on foot, cells reached in flight)."""
    parked = {tuple(int(c // 1) for c in o["start"]) for o in grid.objects if o.get("type") == HELICOPTER}
    foot: set = set()
    air: set = set()
    seeds = [tuple(spawn)]
    while seeds:
        for s in seeds:
            if s not in foot:
                foot |= lc.reachable(grid, s)
        # Take every helicopter he can reach on foot, fly as far as it goes, land anywhere.
        queue = collections.deque(h for h in parked if near(h, foot) and h not in air and flyable(grid, h))
        air.update(queue)
        while queue:
            x, y, z = queue.popleft()
            for dx, dy, dz in SIX:
                n = (x + dx, y + dy, z + dz)
                if n not in air and flyable(grid, n):
                    air.add(n)
                    queue.append(n)
        landings = [c for c in air if grid.standable(c) and c not in foot]
        parked |= set(landings)
        seeds = landings
    return foot, air


def check(path: Path, blocks) -> bool:
    world = vwr_inspect.read_world(path)
    grid = lc.Grid(world, blocks)
    cells = list(grid.cells)
    grid.bounds = (tuple(min(c[i] for c in cells) - 2 for i in range(3)),
                   tuple(max(c[i] for c in cells) + 2 for i in range(3)))
    spawn = tuple(world["spawn"] or (50, 1, 50))
    walk = lc.reachable(grid, spawn)
    foot, air = search(grid, spawn)
    ok = True
    report = []
    for o in world["objects"]:
        kind = lc.TARGETS.get(o.get("type")) or lc.EXITS.get(o.get("type"))
        if not kind:
            continue
        cell = tuple(int(c // 1) for c in o["start"])
        how = ("on foot" if near(cell, walk) else "with a helicopter" if near(cell, foot | air) else None)
        if not grid.free(cell):
            how = None
            report.append(f"{kind} at {cell}: INSIDE A BLOCK")
        else:
            report.append(f"{kind} at {cell}: {'reachable ' + how if how else 'NOT REACHABLE'}")
        if how is None and kind != "egg":
            ok = False
    print(f"{path.name}: {'OK' if ok else 'FAIL'} ({len(walk)} cells on foot from the spawn, "
          f"{len(foot)} with landings, {len(air)} in flight)")
    for line in report:
        print("  " + line)
    return ok


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("worlds", nargs="+", type=Path)
    ap.add_argument("--blocks", type=Path, default=Path(__file__).resolve().parent / "data" / "blocks.csv")
    args = ap.parse_args(argv)
    blocks = lc.load_blocks(args.blocks)
    results = [check(p, blocks) for p in args.worlds]
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
