#!/usr/bin/env python3
"""Which of the original world's mechanics each Galaxy world leaves out (plans/audit/lesson_inventory.md).

For every campaign world with a Mobile Eggbert counterpart: the object roles (tools/world_inventory.py
OBJECTS) and the signature mechanic tiles (world_inventory.TILES: lava, spikes, crusher, saw, drip,
water, spring, fan, blitz, teleport, switch, bridge, door, temp) the original uses, counted, beside
Galaxy's own. A role or tile the original has and Galaxy has not is listed as MISSING with the
original's cells grouped in clusters (map x, y), so the brief can be read against it: is it a lesson
on the original's route, or scenery? A role kept at under a third of the original's count is listed
as THINNED (a 3D world is laid out differently, so counts are only a hint). Roles Galaxy adds that the
original lacks are listed as ADDED (nothing may be invented).

Usage: python3 tools/audit/lesson_inventory.py [--check]
"""
import argparse
import collections
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
import vwr_inspect  # noqa: E402
import world_inventory as wi  # noqa: E402

OUT = REPO / "plans" / "audit" / "lesson_inventory.md"
CLUSTER = 9  # cells: marks closer than this on both axes belong to one place


def tile_cells(icons_at, name):
    rng = dict(wi.TILES)[name]
    return sorted(p for p, v in icons_at.items() if v in rng)


def clusters(cells):
    out = []
    for c in cells:
        for group in out:
            if any(abs(c[0] - p[0]) <= CLUSTER and abs(c[1] - p[1]) <= CLUSTER for p in group):
                group.append(c)
                break
        else:
            out.append([c])
    return ["(%d,%d)x%d" % (sum(p[0] for p in g) // len(g), sum(p[1] for p in g) // len(g), len(g)) for g in out]


def report(ref_dir):
    lines = ["# What each world leaves out of its original (generated)", "",
             "`tools/audit/lesson_inventory.py`; the audit of each finding is in `world_lessons.notes.csv`.",
             "Clusters are the original map's cells (x, y; 64 px tiles), `xN` the cells in the cluster.", "",
             "| World | Missing (original's cells) | Thinned (original -> Galaxy) | Added |", "|---|---|---|---|"]
    for path in sorted((REPO / "worlds3d").glob("world*.vwr")):
        n = int(path.stem[5:])
        me = ref_dir / ("world%03d.txt" % n)
        if not me.exists():
            continue
        ref = wi.parse_reference(me)
        world = vwr_inspect.read_world(path)
        ro = collections.Counter(wi.OBJECTS.get(t, "type %d" % t) for t in ref["objects"])
        go = collections.Counter(wi.OBJECTS.get(o["type"], "type %d" % o["type"]) for o in world["objects"])
        for c in (ro, go):
            c.pop("Blupi", None)
        gicons = collections.Counter(raw >> 4 for raw in world["blocks"].values())
        missing, thinned = [], []
        for role, count in sorted(ro.items()):
            if role not in go:
                missing.append("%s %d" % (role, count))
            elif go[role] * 3 < count:
                thinned.append("%s %d -> %d" % (role, count, go[role]))
        for name, rng in wi.TILES:
            cells = tile_cells(ref["grid"], name)
            if cells and not any(gicons[i] for i in rng):
                missing.append("%s: %s" % (name, " ".join(clusters(cells))))
        added = ["%s %d" % (r, c) for r, c in sorted(go.items()) if r not in ro]
        added += [name for name, rng in wi.TILES
                  if any(gicons[i] for i in rng) and not tile_cells(ref["grid"], name)]
        if missing or thinned or added:
            lines.append("| %03d | %s | %s | %s |" % (n, "; ".join(missing) or "-", "; ".join(thinned) or "-",
                                                      "; ".join(added) or "-"))
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ref", default=str(REPO.parent / "mobile-eggbert" / "worlds"))
    ap.add_argument("--check", action="store_true", help="fail when the report is stale")
    args = ap.parse_args()
    text = report(Path(args.ref))
    if args.check:
        if OUT.read_text(encoding="utf-8") != text:
            print("stale: %s" % OUT)
            return 1
        return 0
    OUT.write_text(text, encoding="utf-8")
    print("wrote %s (%d worlds listed)" % (OUT, text.count("\n| ") - 1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
