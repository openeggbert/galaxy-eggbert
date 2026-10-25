#!/usr/bin/env python3
"""Conservative reachability check for a Galaxy Eggbert world: can Blupi get from the spawn to
every treasure, key and egg and to the exit?

Searches the voxel grid with moves Blupi certainly has (BlupiController): walk to a neighbour
cell, step off an edge and fall, jump (up to two blocks up, or across a gap of up to two cells,
needing headroom), ride a platform lift between its two ends. Block classes come from
tools/data/blocks.csv (BlockDefinitionRegistry; DumpBlockDefinitions keeps it current): a cell
is free when it holds air, water, a fan or a teleporter, or a block whose collision mask is
empty; it is a floor when the block below is solid. Cells holding or standing on a hazard
(lava, spikes, crusher, saw, blitz, drip) and cells next to mines and enemies are avoided. A
world can be more reachable than this says (dynamite, vehicles, crates, springs, teleporters
are not modelled yet); when it says reachable, the route exists.

    python3 tools/levelcheck.py worlds3d/world011.vwr [...]
"""

from __future__ import annotations

import argparse
import collections
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vwr_inspect  # noqa: E402

SIZE = 100
LIFTS = {1, 47, 48}
DANGEROUS = {2, 3, 4, 16, 17, 20, 32, 33, 44, 54, 96, 97}   # mines and enemies
TARGETS = {5: "treasure", 6: "egg", 49: "key", 50: "key", 51: "key"}
EXITS = {7: "exit", 21: "secret exit"}


def load_blocks(path: Path) -> dict[int, dict]:
    with open(path) as f:
        return {int(r["icon"]): {k: int(v) for k, v in r.items()} for r in csv.DictReader(f)}


class Grid:
    def __init__(self, world: dict, blocks: dict[int, dict]):
        self.blocks = blocks
        self.cells = {p: raw >> 4 for p, raw in world["blocks"].items()}
        self.objects = world["objects"]
        self.solid_objects = set()      # crates stand like blocks
        for o in self.objects:
            if o.get("type") == 12:
                self.solid_objects.add(tuple(int(c // 1) for c in o["start"]))
        self.danger = set()
        for o in self.objects:
            if o.get("type") in DANGEROUS:
                for end in (o["start"], o["end"]):
                    cx, cy, cz = (int(c // 1) for c in end)
                    for dx in (-1, 0, 1):
                        for dz in (-1, 0, 1):
                            for dy in (0, 1):
                                self.danger.add((cx + dx, cy + dy, cz + dz))
                # a patrol sweeps the cells between its ends
                (x0, y0, z0), (x1, y1, z1) = o["start"], o["end"]
                steps = int(max(abs(x1 - x0), abs(y1 - y0), abs(z1 - z0))) + 1
                for i in range(steps + 1):
                    t = i / steps
                    self.danger.add((int((x0 + (x1 - x0) * t) // 1), int((y0 + (y1 - y0) * t) // 1),
                                     int((z0 + (z1 - z0) * t) // 1)))

    def info(self, p):
        return self.blocks.get(self.cells.get(p, 0), self.blocks[0])

    def free(self, p) -> bool:
        x, y, z = p
        if not (0 <= x < SIZE and 0 <= y < SIZE and 0 <= z < SIZE) or p in self.solid_objects:
            return False
        b = self.info(p)
        return b["icon"] == 0 or b["water"] or b["teleporter"] or b["fan"] or b["collision_mask"] == 0

    def floor(self, p) -> bool:
        x, y, z = p
        below = (x, y - 1, z)
        return y > 0 and (self.info(below)["solid"] == 1 or below in self.solid_objects)

    def safe(self, p) -> bool:
        x, y, z = p
        return (p not in self.danger and self.info(p)["hazard"] == 0 and
                self.info((x, y - 1, z))["hazard"] == 0)

    def standable(self, p) -> bool:
        return self.free(p) and self.floor(p) and self.safe(p)

    def fall(self, p):
        """Where Blupi lands stepping into the free cell p, or None (off the world / onto danger)."""
        x, y, z = p
        while y > 0:
            if self.floor((x, y, z)):
                return (x, y, z) if self.safe((x, y, z)) else None
            y -= 1
            if not self.free((x, y, z)):
                return None
        return None


def lift_links(grid: Grid):
    links = collections.defaultdict(set)
    for o in grid.objects:
        if o.get("type") in LIFTS:
            a = tuple(int(c // 1) for c in o["start"])
            b = tuple(int(c // 1) for c in o["end"])
            # Blupi stands on the lift's cube: the cell above it at either end.
            top_a, top_b = (a[0], a[1] + 1, a[2]), (b[0], b[1] + 1, b[2])
            # Everything within reach of the lift's path counts as a stop.
            stops = {top_a, top_b}
            for s in list(stops):
                for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    stops.add((s[0] + dx, s[1], s[2] + dz))
            for s in stops:
                links[s] |= stops - {s}
    return links


def reachable(grid: Grid, start):
    links = lift_links(grid)
    seen = {start}
    queue = collections.deque([start])
    axis = [(1, 0), (-1, 0), (0, 1), (0, -1)]
    while queue:
        x, y, z = queue.popleft()
        nexts = []
        for dx, dz in axis:
            n = (x + dx, y, z + dz)
            if grid.standable(n):
                nexts.append(n)
            elif grid.free(n) and grid.free((n[0], y + 1, n[2])):
                landing = grid.fall(n)
                if landing:
                    nexts.append(landing)
        # Jumps: headroom above the start, the arc's cells free, a floor at the end.
        for up in (1, 2):
            if not all(grid.free((x, y + h, z)) for h in range(1, up + 2)):
                break
            for dx, dz in axis:
                for dist in (1, 2):
                    tx, tz = x + dx * dist, z + dz * dist
                    path_ok = all(grid.free((x + dx * k, y + up + 1, z + dz * k)) for k in range(1, dist + 1))
                    if not path_ok:
                        continue
                    for ty in range(y + up, y - 6, -1):
                        t = (tx, ty, tz)
                        if grid.standable(t) and all(grid.free((tx, h, tz)) for h in range(ty, y + up + 2)):
                            nexts.append(t)
                            break
        for n in links.get((x, y, z), ()):
            if grid.free(n) and grid.safe(n):
                nexts.append(n)
        for n in nexts:
            if n not in seen:
                seen.add(n)
                queue.append(n)
    return seen


def check(path: Path, blocks) -> bool:
    world = vwr_inspect.read_world(path)
    grid = Grid(world, blocks)
    # Without an explicit spawn the game starts Blupi at render (0, 1, 0), raw (50, 1, 50).
    spawn = world["spawn"] or (50, 1, 50)
    reach = reachable(grid, tuple(spawn))
    ok = True
    report = []
    for o in world["objects"]:
        kind = TARGETS.get(o.get("type")) or EXITS.get(o.get("type"))
        if not kind:
            continue
        cell = tuple(int(c // 1) for c in o["start"])
        near = {cell} | {(cell[0] + dx, cell[1] + dy, cell[2] + dz)
                         for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1)}
        got = bool(near & reach)
        report.append(f"{kind} at {cell}: {'reachable' if got else 'NOT REACHABLE'}")
        if not got and kind != "egg":
            ok = False
    exits = [o for o in world["objects"] if o.get("type") in EXITS]
    if not exits and world["mission"] % 10 != 0 and world["mission"] not in (0, 1):
        report.append("no exit")
        ok = False
    print(f"{path.name}: {'OK' if ok else 'FAIL'} ({len(reach)} cells reachable)")
    for line in report:
        print("  " + line)
    return ok


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("worlds", nargs="+", type=Path)
    ap.add_argument("--blocks", type=Path, default=Path(__file__).resolve().parent / "data" / "blocks.csv")
    args = ap.parse_args(argv)
    blocks = load_blocks(args.blocks)
    results = [check(p, blocks) for p in args.worlds]
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
