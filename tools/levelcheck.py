#!/usr/bin/env python3
"""Conservative reachability check for a Galaxy Eggbert world: can Blupi get from the spawn to
every treasure, key and egg and to the exit?

Also finds traps: the exit must stay reachable from where each target is taken (a one-way drop
into a pit with no way out is a softlock), and of any two treasures or keys one must be reachable
from the other, so that some order takes them all (linear levels with one-way drops are fine).
A lift travelling level must not have an end inside a block.

Searches the voxel grid with moves Blupi certainly has (BlupiController): walk to a neighbour
cell, step off an edge and fall, jump (up to two blocks up, or across a gap of up to two cells,
needing headroom, the arc's columns free at the apex's feet and head), ride a platform lift
between its two ends, swim (through water in every direction, out at the surface onto a bank level
with the water's top cell), take a teleporter (standing under one of its pillars he arrives in the cell
under the other pillar of the same icon, as WorldRuntime::FindTeleportDestination places him
since BD-0017). Hub doors (the bars 182/183) count as open: they open as the player progresses. In
a hub every sign must be reachable too. Block classes come from tools/data/blocks.csv
(BlockDefinitionRegistry; DumpBlockDefinitions keeps it current): a cell is free when it holds
air, water or a teleporter (a fan is a whole block, M3D-0004), or a block whose collision mask is empty; it is a floor when
the block below is solid (a crusher is no floor: it hangs in a walk cell, BD-0032). Cells holding
or standing on a hazard (lava, spikes, saw, blitz, drip; a crusher's own cell is a way) and cells
next to mines and enemies are avoided. A world can be more reachable than
this says (dynamite, vehicles, crates, springs are not modelled yet); when it says reachable,
the route exists.

With ``--rides`` it also counts two moves that are real but harder to bound, for auditing what
the plain check leaves unreachable (optional eggs, secret exits): stepping or jumping off a lift
anywhere along its path (a rider is carried through every cell above the lift's track), and the
helicopter (13): from where it stands he flies through free, safe cells two high (no water, no
wind), takes what he flies through, and lands on any floor he can stand on, leaving it there to
come back to.

    python3 tools/levelcheck.py worlds3d/world011.vwr [...]
"""

from __future__ import annotations

import argparse
import math
import collections
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import vwr_inspect  # noqa: E402

SIZE = 100
CRUSHER_HAZARD = 3  # BlockHazardKind::Crusher (tools/data/blocks.csv)
DRIP_HAZARD = 6     # BlockHazardKind::Drip: hangs in an open cell (BD-0033), nothing stands on it
LIFTS = {1, 47, 48}
SOLID_BIGDECOR = set(range(16, 32)) | set(range(36, 48)) | {87, 88, 89}
SOLID_BIGDECOR_HEIGHT = 2
OBJECTS = Path(__file__).resolve().parents[1] / "avatars3d" / "objects"
VERTEX = None  # numpy dtype, made on first use (the models' 52-byte vertices)


def bigdecor_shapes() -> dict:
    """icon -> [(minX, maxX, minY, maxY, minZ, maxZ)], the boxes the game measures from the BigDecor
    models (GalaxyEggbertGame.cpp ModelFileShape: half-block slices of the model as drawn)."""
    import math
    import numpy as np
    global VERTEX
    if VERTEX is None:
        VERTEX = np.dtype([("pos", "<f4", 3), ("rest", "V40")])
    shapes = {}
    if not OBJECTS.exists():
        return shapes
    for d in sorted(OBJECTS.glob("bd_*")):
        lines = [l.split() for l in (d / "icons.txt").read_text().splitlines() if l.startswith("icon explo")]
        pts = np.concatenate([np.fromfile(f, dtype=VERTEX)["pos"] for f in d.glob("*.verts.bin")]).astype(float)
        for f in lines:
            icon, mirror, yaw, px, dx, dy = int(f[2]), int(f[4]), float(f[5]), int(f[7]), int(f[8]), int(f[9])
            if icon not in SOLID_BIGDECOR:
                continue
            c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
            x = pts[:, 0] * c + pts[:, 2] * s
            y = pts[:, 1].copy()
            z = -pts[:, 0] * s + pts[:, 2] * c
            if mirror == 1:
                x = -x - 1 / 64
            elif mirror == 2:
                y = -y + 1 / 64
            x = x + dx / 64
            y = y - dy / 64 + (px or 128) / 128
            top = max(0.0, float(y.max()))
            n = max(1, math.ceil(top / 0.5))
            k = np.clip(np.floor(y / 0.5).astype(int), 0, n - 1)
            boxes = [None] * n
            for i in range(n):
                sel = k == i
                if sel.any():
                    boxes[i] = (x[sel].min(), x[sel].max(), z[sel].min(), z[sel].max())
            area = lambda b: (b[1] - b[0]) * (b[3] - b[2])
            for i in range(n):
                if boxes[i] is None:
                    near = [boxes[j] for d_ in range(1, n) for j in (i - d_, i + d_) if 0 <= j < n and boxes[j] is not None]
                    if near:
                        boxes[i] = min(near[:2], key=area)
            shapes[icon] = [(b[0], b[1], 0.5 * i, 0.5 * (i + 1), b[2], b[3]) for i, b in enumerate(boxes)
                            if b is not None and b[1] - b[0] >= 0.05 and b[3] - b[2] >= 0.05]
    return shapes


_SHAPES = None
HELICOPTER = 13
WIND_FIRST, WIND_LAST = 110, 125   # BlockTypes::isWind
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
        # The solid BigDecor (BlockDefinitionRegistry's kSolidBigDecorIcons, Q-014) as the game's
        # boxes: a cell counts as solid where a box reaches within a quarter block of its middle
        # (Blupi is half a block wide), over the heights it covers; the cell's quarter turns as the game.
        global _SHAPES
        if _SHAPES is None:
            _SHAPES = bigdecor_shapes()
        turns_at = world.get("object_turns", {})
        for (x, y, z), icon in world.get("bigdecor", ()):
            if icon not in SOLID_BIGDECOR:
                continue
            shape = _SHAPES.get(icon) or [(-0.5, 0.5, 0.0, float(SOLID_BIGDECOR_HEIGHT), -0.5, 0.5)]
            for x0, x1, y0, y1, z0, z1 in shape:
                for _ in range(turns_at.get((x, y, z), 0)):
                    x0, x1, z0, z1 = z0, z1, -x1, -x0
                for cy in range(int(y + y0 // 1), int(math.ceil(y + y1))):
                    if not (cy + 1 > y + y0 and cy < y + y1):
                        continue
                    for cx in range(math.floor(x + x0 - 0.25), math.ceil(x + x1 + 0.25) + 1):
                        for cz in range(math.floor(z + z0 - 0.25), math.ceil(z + z1 + 0.25) + 1):
                            if x + x0 < cx + 0.25 and x + x1 > cx - 0.25 and z + z0 < cz + 0.25 and z + z1 > cz - 0.25:
                                self.solid_objects.add((cx, cy, cz))
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
        # A fan is a whole block (M3D-0004, BlupiController::IsPointSolid), not free.
        return (b["icon"] == 0 or b["water"] or b["teleporter"] or
                (b["collision_mask"] == 0 and not b["fan"]) or b["hub_door"] != 0)

    def floor(self, p) -> bool:
        x, y, z = p
        below = (x, y - 1, z)
        b = self.info(below)
        # A crusher hangs in a walk cell (BD-0032): nothing stands on it.
        return y > 0 and ((b["solid"] == 1 and b["hazard"] not in (CRUSHER_HAZARD, DRIP_HAZARD)) or
                          below in self.solid_objects)

    def safe(self, p) -> bool:
        x, y, z = p
        # A crusher only squashes him for a while (BD-0032): its cell is a way, not a danger.
        return (p not in self.danger and self.info(p)["hazard"] in (0, CRUSHER_HAZARD) and
                self.info((x, y - 1, z))["hazard"] == 0)

    def water(self, p) -> bool:
        return bool(self.info(p)["water"])

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
    # Where a rider stands at either end of every lift: another lift's end is a stop too (relays).
    ends = set()
    for o in grid.objects:
        if o.get("type") in LIFTS:
            for end in (o["start"], o["end"]):
                c = tuple(int(v // 1) for v in end)
                ends.add((c[0], c[1] + 1, c[2]))
    for o in grid.objects:
        if o.get("type") in LIFTS:
            a = tuple(int(c // 1) for c in o["start"])
            b = tuple(int(c // 1) for c in o["end"])
            # Blupi stands on the lift's cube: the cell above it at either end.
            top_a, top_b = (a[0], a[1] + 1, a[2]), (b[0], b[1] + 1, b[2])
            # He steps on and off at either end from a neighbouring cell with a floor under it, or
            # from another lift's end (a hole beside the lift's end is a gap to jump, not a stop).
            stops = {top_a, top_b}
            for s in list(stops):
                for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    n = (s[0] + dx, s[1], s[2] + dz)
                    if grid.standable(n) or n in ends:
                        stops.add(n)
            for s in stops:
                links[s] |= stops - {s}
    return links


def teleport_links(grid: Grid):
    """Under a teleporter pillar -> the cell under its partner (same icon, BD-0017)."""
    pillars = collections.defaultdict(list)
    for p, icon in grid.cells.items():
        if grid.blocks.get(icon, grid.blocks[0])["teleporter"]:
            pillars[icon].append(p)
    links = collections.defaultdict(set)
    for cells in pillars.values():
        for a in cells:
            for b in cells:
                if a != b:
                    links[(a[0], a[1] - 1, a[2])].add((b[0], b[1] - 1, b[2]))
    return links


def lift_ride_links(grid: Grid):
    """--rides: every cell a rider passes through on a lift's track, linked both ways with the
    cells where he gets on and off at its ends (lift_links): off it anywhere he can step or jump."""
    links = collections.defaultdict(set)
    ends = lift_links(grid)
    for o in grid.objects:
        if o.get("type") not in LIFTS:
            continue
        (x0, y0, z0), (x1, y1, z1) = o["start"], o["end"]
        a = (int(x0 // 1), int(y0 // 1) + 1, int(z0 // 1))
        b = (int(x1 // 1), int(y1 // 1) + 1, int(z1 // 1))
        stops = ({a, b} | ends.get(a, set()) | ends.get(b, set()))
        steps = int(max(abs(x1 - x0), abs(y1 - y0), abs(z1 - z0))) + 1
        track = set()
        for i in range(steps + 1):
            f = i / steps
            c = (int((x0 + (x1 - x0) * f) // 1), int((y0 + (y1 - y0) * f) // 1) + 1,
                 int((z0 + (z1 - z0) * f) // 1))
            if grid.free(c) and grid.safe(c):
                track.add(c)
        for s in stops:
            links[s] |= track
        for c in track:
            links[c] |= stops
    return links


def flight(grid: Grid, start):
    """--rides: the cells the helicopter flies through from start (free, safe, the cell above free
    too; no water, no wind)."""
    def open_air(c):
        x, y, z = c
        return (grid.free(c) and grid.safe(c) and not grid.water(c) and grid.free((x, y + 1, z)) and
                grid.info(c)["fan"] == 0 and not (WIND_FIRST <= grid.cells.get(c, 0) <= WIND_LAST))
    if not open_air(start):
        return set()
    seen = {start}
    queue = collections.deque([start])
    while queue:
        x, y, z = queue.popleft()
        for dx, dy, dz in ((1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1), (0, 1, 0), (0, -1, 0)):
            n = (x + dx, y + dy, z + dz)
            if n not in seen and open_air(n):
                seen.add(n)
                queue.append(n)
    return seen


def reachable(grid: Grid, start, rides: bool = False):
    return graph(grid, start, rides)[0]


def graph(grid: Grid, start, rides: bool = False):
    """The cells reachable from start (a cell, or a set of cells) and the moves between them
    (cell -> cells)."""
    links = lift_links(grid)
    for cell, targets in teleport_links(grid).items():
        links[cell] |= targets
    if rides:
        for cell, targets in lift_ride_links(grid).items():
            links[cell] |= targets
    starts = {start} if isinstance(start, tuple) else set(start)
    seen = set(starts)
    edges = collections.defaultdict(set)
    queue = collections.deque(starts)
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
                    # At the apex his feet are at y + up: the arc's columns need room there and
                    # a cell above (a wall three high between two floors is no jump).
                    path_ok = all(grid.free((x + dx * k, y + h, z + dz * k))
                                  for k in range(1, dist + 1) for h in (up, up + 1))
                    if not path_ok:
                        continue
                    for ty in range(y + up, y - 6, -1):
                        t = (tx, ty, tz)
                        if grid.standable(t) and all(grid.free((tx, h, tz)) for h in range(ty, y + up + 2)):
                            nexts.append(t)
                            break
        # Swimming: on through the water in every direction (he floats up, dives with LeftShift);
        # at the surface out onto a bank whose top block is level with the water's top cell (the
        # weak jump, as tools/watercheck.py), or on into open air at the surface's height.
        if grid.water((x, y, z)):
            for dx, dy, dz in ((1, 0, 0), (-1, 0, 0), (0, 0, 1), (0, 0, -1), (0, 1, 0), (0, -1, 0)):
                n = (x + dx, y + dy, z + dz)
                if grid.water(n) and grid.safe(n):
                    nexts.append(n)
            if not grid.water((x, y + 1, z)) and grid.free((x, y + 1, z)):
                for dx, dz in axis:
                    n = (x + dx, y, z + dz)
                    above, head = (n[0], y + 1, n[2]), (n[0], y + 2, n[2])
                    if grid.water(n):
                        continue
                    if not grid.free(n) and grid.free(above) and grid.free(head) and grid.safe(above):
                        nexts.append(above)
                    elif grid.free(n) and grid.free(above):
                        landing = grid.fall(n)
                        if landing:
                            nexts.append(landing)
        for n in links.get((x, y, z), ()):
            if grid.free(n) and grid.safe(n):
                nexts.append(n)
        for n in nexts:
            edges[(x, y, z)].add(n)
            if n not in seen:
                seen.add(n)
                queue.append(n)
    return seen, edges


def helicopter_reach(grid: Grid, reach: set):
    """--rides: (cells, flown) once every helicopter he can get to is used: the cells it flies
    through (flown: he takes what is there) and the walk on from wherever it lands (cells)."""
    total, flown, used = set(reach), set(), set()
    regions = []                    # flight regions found so far (a helicopter inside one reuses it)
    helis = [tuple(int(c // 1) for c in o["start"]) for o in grid.objects if o.get("type") == HELICOPTER]
    changed = True
    while changed:
        changed = False
        for i, c in enumerate(helis):
            near = {(c[0] + dx, c[1] + dy, c[2] + dz) for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1)}
            if i in used or not near & total:
                continue
            used.add(i)
            region = next((r for r in regions if c in r), None)
            if region is None:
                region = flight(grid, c)
                regions.append(region)
            flown |= region
            landings = {p for p in region if p not in total and grid.standable(p)}
            if landings:
                total |= graph(grid, landings, True)[0]
            changed = True
    return total, flown


def onward_sets(cells, edges):
    """For every cell, the bit set of the strongly connected components it can reach (one bit
    per component), and each cell's component: Kosaraju's algorithm, iteratively."""
    order, done = [], set()
    for root in cells:
        if root in done:
            continue
        done.add(root)
        stack = [(root, iter(edges.get(root, ())))]
        while stack:
            cell, it = stack[-1]
            nxt = next((n for n in it if n not in done), None)
            if nxt is None:
                order.append(cell)
                stack.pop()
            else:
                done.add(nxt)
                stack.append((nxt, iter(edges.get(nxt, ()))))
    reverse = collections.defaultdict(list)
    for a, targets in edges.items():
        for b in targets:
            reverse[b].append(a)
    component = {}
    count = 0
    for root in reversed(order):
        if root in component:
            continue
        component[root] = count
        stack = [root]
        while stack:
            cell = stack.pop()
            for prev in reverse.get(cell, ()):
                if prev not in component:
                    component[prev] = count
                    stack.append(prev)
        count += 1
    # Components come out sources first: fill each one's reach from the sinks back.
    succ = collections.defaultdict(set)
    for a, targets in edges.items():
        for b in targets:
            if component[a] != component[b]:
                succ[component[a]].add(component[b])
    reach = [0] * count
    for c in range(count - 1, -1, -1):
        bits = 1 << c
        for d in succ[c]:
            bits |= reach[d]
        reach[c] = bits
    return component, reach


def check(path: Path, blocks, rides: bool = False) -> bool:
    world = vwr_inspect.read_world(path)
    grid = Grid(world, blocks)
    # Without an explicit spawn the game starts Blupi at render (0, 1, 0), raw (50, 1, 50).
    spawn = world["spawn"] or (50, 1, 50)
    reach, edges = graph(grid, tuple(spawn), rides)
    flown = set()
    if rides:
        reach, flown = helicopter_reach(grid, reach)
    ok = True
    report = []
    targets = []
    for o in world["objects"]:
        kind = TARGETS.get(o.get("type")) or EXITS.get(o.get("type"))
        if not kind:
            continue
        cell = tuple(int(c // 1) for c in o["start"])
        near = {cell} | {(cell[0] + dx, cell[1] + dy, cell[2] + dz)
                         for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1)}
        targets.append((kind, cell, near))
        got = bool(near & (reach | flown))
        report.append(f"{kind} at {cell}: {'reachable' if got else 'NOT REACHABLE'}")
        if not got and kind != "egg":
            ok = False
    for cell, raw in sorted(world["blocks"].items()):
        if blocks.get(raw >> 4, blocks[0])["world_select"] > 0:
            near = {(cell[0] + dx, cell[1], cell[2] + dz) for dx in (-1, 0, 1) for dz in (-1, 0, 1)}
            got = bool(near & reach)
            report.append(f"sign {raw >> 4} at {cell}: {'reachable' if got else 'NOT REACHABLE'}")
            ok = ok and got
    # A lift travelling level must not have an end inside a block: standing on that block Blupi is
    # on ground, and the lift slides away from under him (082's arrow room). A rising or sinking
    # lift may start in the floor, which it lifts him off.
    for o in world["objects"]:
        if o.get("type") in LIFTS:
            a = tuple(int(c // 1) for c in o["start"])
            b = tuple(int(c // 1) for c in o["end"])
            if a[1] == b[1] and any(grid.info(e)["solid"] == 1 for e in (a, b)):
                report.append(f"lift {a} -> {b}: an end inside a block")
                ok = False
    exits = [o for o in world["objects"] if o.get("type") in EXITS]
    if not exits and world["mission"] % 10 != 0 and world["mission"] not in (0, 1):
        report.append("no exit")
        ok = False
    if rides:
        # The helicopter's moves are not in the graph's edges: no trap analysis here.
        print(f"{path.name}: {'OK' if ok else 'FAIL'} ({len(reach)} cells reachable, --rides)")
        for line in report:
            print("  " + line)
        return ok
    # No traps: the exit stays reachable from where each target is taken (no one-way drop into a
    # pit with no way out), and of any two treasures or keys one is reachable from the other, so
    # there is an order that takes them all before the exit.
    component, onward = onward_sets(reach, edges)
    exit_bits = 0
    for kind, _, near in targets:
        if kind in EXITS.values():
            for c in near & reach:
                exit_bits |= 1 << component[c]
    taken = {}
    for i, (kind, cell, near) in enumerate(targets):
        if kind in EXITS.values() or not near & reach:
            continue
        starts = {component[c] for c in near & reach}
        leading = [c for c in starts if onward[c] & exit_bits]
        if exit_bits and not leading:
            report.append(f"{kind} at {cell}: TRAP, the exit is unreachable from there")
            ok = False
        taken[i] = (leading or list(starts), sum(1 << c for c in starts))
    goals = [i for i in taken if targets[i][0] != "egg"]
    for a in goals:
        for b in goals:
            if a < b:
                a_to_b = any(onward[c] & taken[b][1] for c in taken[a][0])
                b_to_a = any(onward[c] & taken[a][1] for c in taken[b][0])
                if not a_to_b and not b_to_a:
                    report.append(f"{targets[a][0]} at {targets[a][1]} and {targets[b][0]} at {targets[b][1]}: "
                                  f"TRAP, neither reachable from the other")
                    ok = False
    print(f"{path.name}: {'OK' if ok else 'FAIL'} ({len(reach)} cells reachable)")
    for line in report:
        print("  " + line)
    return ok


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("worlds", nargs="+", type=Path)
    ap.add_argument("--blocks", type=Path, default=Path(__file__).resolve().parent / "data" / "blocks.csv")
    ap.add_argument("--rides", action="store_true",
                    help="also off a lift anywhere along its track, and the helicopter's flight")
    args = ap.parse_args(argv)
    blocks = load_blocks(args.blocks)
    results = [check(p, blocks, args.rides) for p in args.worlds]
    return 0 if all(results) else 1


if __name__ == "__main__":
    sys.exit(main())
