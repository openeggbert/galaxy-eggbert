#!/usr/bin/env python3
"""Patrolling creatures of the shipped worlds against the original's timing, type by type.

The original's moving objects (../mobile-eggbert/worlds, read only) go posStart -> posEnd over
stepAdvance 20 Hz ticks and back over stepRecede (Decor::MoveObjectStepLine), resting
timeStopStart/End ticks at the ends. Every creature of a type has the same speed along the
longer axis of its path and the same rests (its editor's per-type values; BD-0016). This prints,
per type, the original's speeds (cells/s along the longer axis) and rests, then every Galaxy object
of that type that differs (in 3D the longer of X, Y and Z). Lifts (1/47/48) vary in the original
and have their own report, lift_envelope.py (BD-0006).

Usage: python3 tools/audit/patrol_envelope.py [--ref ../mobile-eggbert] [--types 4,16] [--world N]
"""
import argparse
import collections
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
import vwr_inspect  # noqa: E402

RECORD = re.compile(r"type=(\d+) stepAdvance=(\d+) stepRecede=(\d+) timeStopStart=(\d+) timeStopEnd=(\d+) "
                    r"posStart=(-?\d+);(-?\d+) posEnd=(-?\d+);(-?\d+)")
LIFTS = {1, 47, 48}


def speed(cells, ticks):
    return cells / (ticks / 20.0) if ticks else float("inf")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ref", default=str(REPO.parent / "mobile-eggbert"))
    ap.add_argument("--types", help="comma-separated types (default: every type that moves in the original)")
    ap.add_argument("--world", type=int)
    args = ap.parse_args()

    speeds = collections.defaultdict(collections.Counter)  # type -> Counter(speed along the longer axis)
    rests = collections.defaultdict(collections.Counter)   # type -> Counter(rest ticks)
    for path in sorted((Path(args.ref) / "worlds").glob("world*.txt")):
        for m in RECORD.finditer(path.read_text(encoding="utf-8", errors="replace")):
            t, adv, rec, s0, s1, x0, y0, x1, y1 = map(int, m.groups())
            cells = max(abs(x1 - x0), abs(y1 - y0)) / 64.0
            if cells > 0 and t not in LIFTS:
                for ticks in (adv, rec):
                    speeds[t][round(speed(cells, ticks), 4)] += 1
                for s in (s0, s1):
                    rests[t][s] += 1
    types = sorted(speeds) if not args.types else [int(t) for t in args.types.split(",")]

    outside = 0
    for t in types:
        if t not in speeds:
            continue
        ref_speed = speeds[t].most_common(1)[0][0]
        ref_rest = rests[t].most_common(1)[0][0]
        rows = []
        for path in sorted((REPO / "worlds3d").glob("world*.vwr")):
            n = int(path.stem[5:])
            if args.world and n != args.world:
                continue
            for o in vwr_inspect.read_world(path)["objects"]:
                if o["type"] != t:
                    continue
                cells = max(abs(a - b) for a, b in zip(o["start"], o["end"]))
                if cells == 0:
                    continue
                out, back = speed(cells, o["advance"]), speed(cells, o["recede"])
                bad = (abs(out - ref_speed) > 1e-3 or abs(back - ref_speed) > 1e-3 or
                       o["stop_start"] != ref_rest or o["stop_end"] != ref_rest)
                rows.append((n, out, back, o["stop_start"], o["stop_end"], cells, o["start"], o["end"], bad))
        bad_rows = [r for r in rows if r[8]]
        print("type %3d: original %4d values, all %.2f cells/s (%s), rest %d ticks (%s); galaxy %3d moving, %d differ"
              % (t, sum(speeds[t].values()), ref_speed, "uniform" if len(speeds[t]) == 1 else dict(speeds[t]),
                 ref_rest, "uniform" if len(rests[t]) == 1 else dict(rests[t]), len(rows), len(bad_rows)))
        for r in bad_rows:
            outside += 1
            print("   world%03d %5.2f/%5.2f cells/s rests %g/%g ticks, %g cells %s->%s" %
                  (r[0], r[1], r[2], r[3], r[4], r[5], tuple(round(c, 1) for c in r[6]),
                   tuple(round(c, 1) for c in r[7])))
    print("creatures off their type's original timing: %d" % outside)
    return 0


if __name__ == "__main__":
    sys.exit(main())
