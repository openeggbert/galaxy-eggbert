#!/usr/bin/env python3
"""Every egg, key, treasure and exit of the campaign: is there evidence that Blupi can reach it?

tools/levelcheck.py is conservative (no vehicles, crates, dynamite, timed passes), so it leaves
many targets "NOT REACHABLE" that the game reaches. A play pass (tools/play_pass.sh) plays every
world by its script; its traces say where Blupi actually stood. This puts the two together, per
target, in the first class that holds:

  levelcheck    levelcheck's own graph from the spawn reaches it
  playthrough   the playthrough took it (its counter rose near it) or stood within a cell of it;
                every treasure and the exit count here when the playthrough took all treasures
                and passed
  from-run      levelcheck's graph, started from every cell the playthrough stood on (in a
                patrol's lane too: he was there) and with
                its --rides moves (off a lift anywhere along its track, the helicopter's flight),
                reaches it
  earned-door   as from-run with the treasure doors (420 + n) open whose n the playthrough's
                treasures reach
  timed         as earned-door with the enemies' and mines' cells crossable: geometrically there,
                the crossing needs timing (a patrol's gap, a crouch under hanging mines)
  unknown       none of these: needs a look

Worlds whose generator has a ``--stage K`` check build (tools/worlds/worldNNN.py: doors open,
climbs as stairs) get a last chance through levelcheck on that stage (``staged``).

    python3 tools/audit/target_reach.py build-probe/play-pass-23 [--worlds 051,053] [--md out.md]
"""
from __future__ import annotations

import argparse
import collections
import math
import re
import subprocess
import sys
import tempfile
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
import levelcheck  # noqa: E402
import vwr_inspect  # noqa: E402

KINDS = {5: "treasure", 6: "egg", 49: "key", 50: "key", 51: "key", 7: "exit", 21: "secret exit"}
AT = re.compile(r"t=(\d+) at (-?[\d.]+) (-?[\d.]+) (-?[\d.]+) yaw [-\d]+( air)?$")
COUNTS = re.compile(r"t=(\d+) treasures (\d+) eggs (\d+) lives \d+ keys (\d+)")
CLASSES = ("levelcheck", "playthrough", "from-run", "earned-door", "timed", "staged", "unknown")


def near(cell):
    return {(cell[0] + dx, cell[1] + dy, cell[2] + dz) for dx in (-1, 0, 1) for dy in (-1, 0, 1) for dz in (-1, 0, 1)}


def run_reach(grid, seeds, open_doors_up_to=0, crossable=False):
    if open_doors_up_to:
        for p, icon in list(grid.cells.items()):
            if 421 <= icon <= 440 and icon - 420 <= open_doors_up_to:
                grid.cells[p] = 0
    if crossable:
        grid.danger = set()
    reach = levelcheck.graph(grid, seeds, True)[0]
    reach, flown = levelcheck.helicopter_reach(grid, reach)
    return reach | flown


def staged_reach(world_name, blocks):
    """Targets levelcheck reaches on the world's own check stages (None: no stages)."""
    gen = REPO / "tools" / "worlds" / f"{world_name}.py"
    if not gen.exists() or "--stage" not in gen.read_text():
        return None
    got = set()
    with tempfile.TemporaryDirectory() as tmp:
        for k in (1, 2, 3):
            out = Path(tmp) / f"{world_name}_{k}.vwr"
            if subprocess.run([sys.executable, str(gen), "--stage", str(k), str(out)],
                              capture_output=True).returncode != 0 or not out.exists():
                continue
            w = vwr_inspect.read_world(out)
            g = levelcheck.Grid(w, blocks)
            r = levelcheck.reachable(g, tuple(w["spawn"] or (50, 1, 50)))
            for o in w["objects"]:
                if o.get("type") in KINDS:
                    c = tuple(int(v // 1) for v in o["start"])
                    if near(c) & r:
                        got.add(c)
    return got


def audit(world_path: Path, trace: Path, blocks):
    world = vwr_inspect.read_world(world_path)
    spawn = tuple(world["spawn"] or (50, 1, 50))
    targets = [(KINDS[o["type"]], tuple(int(v // 1) for v in o["start"]))
               for o in world["objects"] if o.get("type") in KINDS]
    plain = levelcheck.reachable(levelcheck.Grid(world, blocks), spawn)
    text = trace.read_text().splitlines() if trace.exists() else []
    passed = any("PASS script complete" in line for line in text)
    stood, events, prev = [], [], None
    for line in text:
        m = AT.match(line)
        if m:
            stood.append((int(m.group(1)), float(m.group(2)), float(m.group(3)), float(m.group(4)), bool(m.group(5))))
        m = COUNTS.match(line)
        if m:
            now = (int(m.group(2)), int(m.group(3)), int(m.group(4)))
            if prev:
                for i, kind in enumerate(("treasure", "egg", "key")):
                    if now[i] > prev[i]:
                        events.append((kind, int(m.group(1))))
            prev = now
    treasures = sum(1 for k, _ in targets if k == "treasure")
    took_all = prev is not None and max(int(m.group(2)) for m in map(COUNTS.match, text) if m) >= treasures
    taken_at = collections.defaultdict(list)
    for kind, tick in events:
        if stood:
            _, x, y, z, _ = min(stood, key=lambda s: abs(s[0] - tick))
            taken_at[kind].append((x, y, z))
    grid = levelcheck.Grid(world, blocks)
    seeds = {spawn}
    for _, x, y, z, air in stood:
        c = (round(x), round(y), round(z))       # cells are centred on integers
        # Where he stood is a fact, danger or not (a patrol's lane he crossed between its passes).
        if not air and grid.free(c) and (grid.floor(c) or grid.water(c)):
            seeds.add(c)
    tiers = {}
    rows = []
    staged = None
    for kind, cell in targets:
        around = near(cell)
        if around & plain:
            rows.append((kind, cell, "levelcheck"))
            continue
        cx, cy, cz = cell            # objects and positions alike: a cell's centre is its integer
        took = any(math.dist((x, z), (cx, cz)) < 3.5 and abs(y - cy) < 3.5
                   for x, y, z in taken_at.get(kind, ()))
        visited = any(math.dist((x, z), (cx, cz)) < 1.0 and abs(y - cy) < 1.0 for _, x, y, z, _ in stood)
        if took or visited or (passed and (kind == "exit" or (kind == "treasure" and took_all))):
            rows.append((kind, cell, "playthrough"))
            continue
        cls = "unknown"
        for name, kwargs in (("from-run", {}), ("earned-door", {"open_doors_up_to": treasures}),
                             ("timed", {"open_doors_up_to": treasures, "crossable": True})):
            if name not in tiers:
                tiers[name] = run_reach(levelcheck.Grid(world, blocks), seeds, **kwargs)
            if around & tiers[name]:
                cls = name
                break
        if cls == "unknown":
            if staged is None:
                staged = staged_reach(world_path.stem, blocks) or set()
            if cell in staged:
                cls = "staged"
        rows.append((kind, cell, cls))
    return passed, rows


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("play_pass", type=Path, help="a play pass directory (<world>/autoplay_trace.txt)")
    ap.add_argument("--worlds", help="comma-separated world numbers (default: every one played)")
    ap.add_argument("--md", type=Path, help="also write the per-world table here")
    args = ap.parse_args(argv)
    blocks = levelcheck.load_blocks(REPO / "tools" / "data" / "blocks.csv")
    names = sorted(d.name for d in args.play_pass.iterdir() if (d / "autoplay_trace.txt").exists())
    if args.worlds:
        wanted = {f"world{int(n):03d}" for n in args.worlds.split(",")}
        names = [n for n in names if n in wanted]
    totals = collections.Counter()
    lines = ["| world | " + " | ".join(CLASSES) + " | not shown (kind and cell) |",
             "|---|" + "---|" * (len(CLASSES) + 1)]
    for name in names:
        passed, rows = audit(REPO / "worlds3d" / f"{name}.vwr", args.play_pass / name / "autoplay_trace.txt", blocks)
        count = collections.Counter(cls for _, _, cls in rows)
        totals.update(count)
        open_ = [f"{k} {c}" for k, c, cls in rows if cls in ("timed", "unknown")]
        lines.append(f"| {name}{'' if passed else ' (run failed)'} | " +
                     " | ".join(str(count.get(c, 0) or "") for c in CLASSES) + f" | {'; '.join(open_)} |")
        if open_:
            print(f"{name}: " + "; ".join(f"{k} {c} {cls}" for k, c, cls in rows if cls in ("timed", "unknown")))
    print("totals: " + ", ".join(f"{c} {totals.get(c, 0)}" for c in CLASSES))
    if args.md:
        args.md.write_text("\n".join(lines) + "\n\ntotals: " +
                           ", ".join(f"{c} {totals.get(c, 0)}" for c in CLASSES) + "\n")
    return 1 if totals.get("unknown") else 0


if __name__ == "__main__":
    sys.exit(main())
