#!/usr/bin/env python3
"""Water Blupi cannot climb out of: finds every body of water (cells connected in 3D) none of
whose surface cells has a way out.

At the surface he gets out only by the weak jump onto a bank whose top block is level with the
water's top cell, with two free cells above it (the original's -12 px/tick, BlupiController's
kJumpSpeedReduced), or by swimming on into open air at the surface's height. A bank one block
higher traps him. Lifts and teleporters are not modelled: a body reported here may still have
one of those as its way out.

    python3 tools/watercheck.py                       # every worlds3d/world*.vwr
    python3 tools/watercheck.py worlds3d/world084.vwr [...]
"""

from __future__ import annotations

import collections
import glob
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import levelcheck as lc  # noqa: E402
import vwr_inspect  # noqa: E402

AXES = ((1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1), (0, 1, 0), (0, -1, 0))


def traps(path: Path, blocks) -> list:
    grid = lc.Grid(vwr_inspect.read_world(path), blocks)
    water = {c for c, icon in grid.cells.items() if blocks.get(icon, blocks[0])["water"]}
    tops = {c for c in water if (c[0], c[1] + 1, c[2]) not in water and grid.free((c[0], c[1] + 1, c[2]))}
    seen: set = set()
    found = []
    for start in sorted(water):
        if start in seen:
            continue
        body = {start}
        queue = collections.deque([start])
        seen.add(start)
        while queue:
            x, y, z = queue.popleft()
            for dx, dy, dz in AXES:
                n = (x + dx, y + dy, z + dz)
                if n in water and n not in body:
                    body.add(n)
                    seen.add(n)
                    queue.append(n)
        surface = sorted(c for c in body if c in tops)
        if not surface:
            continue
        way_out = False
        for x, y, z in surface:
            for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (x + dx, y, z + dz)
                if n in water:
                    continue
                above, head = (n[0], y + 1, n[2]), (n[0], y + 2, n[2])
                if (not grid.free(n) and grid.free(above) and grid.free(head)) or (grid.free(n) and grid.free(above)):
                    way_out = True
                    break
            if way_out:
                break
        if not way_out:
            found.append((len(surface), surface[0]))
    return found


def main(argv=None) -> int:
    argv = sys.argv[1:] if argv is None else argv
    blocks = lc.load_blocks(Path(__file__).resolve().parent / "data" / "blocks.csv")
    paths = [Path(p) for p in argv] or [Path(p) for p in sorted(glob.glob("worlds3d/world*.vwr"))]
    bad = 0
    for path in paths:
        for size, cell in traps(path, blocks):
            print(f"{path.name}: water without a way out, {size} surface cells, first at {cell}")
            bad += 1
    if not bad:
        print(f"{len(paths)} world(s): every body of water has a way out")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
