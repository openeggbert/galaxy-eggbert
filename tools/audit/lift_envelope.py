#!/usr/bin/env python3
"""Platform lifts (ObjectType 1/47/48) of the shipped worlds against the original's envelope.

The original's lifts (../mobile-eggbert/worlds, read only) move posStart -> posEnd linearly over
stepAdvance 20 Hz ticks (Decor::MoveObjectStepLine), dwell timeStopStart/End ticks. For each
Galaxy world: every lift's path length (cells), speed (cells/s = length / (advance / 20)), dwell
(s), axis; compared with the fastest lift and the shortest dwell of the same-numbered original
world and of the whole original game. Prints the lifts outside the original's envelope.

Usage: python3 tools/audit/lift_envelope.py [--ref ../mobile-eggbert] [--world N]
"""
import argparse
import collections
import math
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO / "tools"))
import vwr_inspect  # noqa: E402

LIFT = re.compile(r"type=(1|47|48) stepAdvance=(\d+) stepRecede=(\d+) timeStopStart=(\d+) timeStopEnd=(\d+) "
                  r"posStart=(-?\d+);(-?\d+) posEnd=(-?\d+);(-?\d+)")


def speed(length, ticks):
    return length / (ticks / 20.0) if ticks else float("inf")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--ref", default=str(REPO.parent / "mobile-eggbert"))
    ap.add_argument("--world", type=int)
    args = ap.parse_args()
    original = collections.defaultdict(list)
    for path in sorted((Path(args.ref) / "worlds").glob("world*.txt")):
        for m in LIFT.finditer(path.read_text(encoding="utf-8", errors="replace")):
            t, adv, rec, s0, s1, x0, y0, x1, y1 = map(int, m.groups())
            length = math.hypot(x1 - x0, y1 - y0) / 64.0
            if length > 0:
                original[int(path.stem[5:])].append((speed(length, adv), speed(length, rec), s0 / 20.0, s1 / 20.0))
    all_orig = [l for v in original.values() for l in v]
    max_speed = max(max(a, b) for a, b, _, _ in all_orig)
    min_dwell = min(min(c, d) for _, _, c, d in all_orig)
    print("original: %d moving lifts in %d worlds; fastest %.2f cells/s; shortest dwell %.2f s" %
          (len(all_orig), len(original), max_speed, min_dwell))
    outside = 0
    for path in sorted((REPO / "worlds3d").glob("world*.vwr")):
        n = int(path.stem[5:])
        if args.world and n != args.world:
            continue
        world = vwr_inspect.read_world(path)
        lifts = [o for o in world["objects"] if o["type"] in (1, 47, 48)]
        rows = []
        for o in lifts:
            length = math.dist(o["start"], o["end"])
            if length == 0:
                continue
            axis = "Y" if abs(o["end"][1] - o["start"][1]) > 0.01 else ("X" if abs(o["end"][0] - o["start"][0]) > 0.01 else "Z")
            rows.append((speed(length, o["advance"]), speed(length, o["recede"]), o["stop_start"] / 20.0,
                         o["stop_end"] / 20.0, length, axis, o["start"], o["end"]))
        if not rows:
            continue
        ref = original.get(n, [])
        ref_max = max((max(a, b) for a, b, _, _ in ref), default=0.0)
        for r in rows:
            fast = max(r[0], r[1]) > max_speed + 1e-6
            if fast or args.world:
                outside += fast
                print("world%03d %s %5.2f/%5.2f cells/s dwell %.2f/%.2f s len %4.1f %s->%s%s (original %03d: fastest %.2f)" %
                      (n, r[5], r[0], r[1], r[2], r[3], r[4], tuple(round(c, 1) for c in r[6]),
                       tuple(round(c, 1) for c in r[7]), "  FASTER THAN ANY ORIGINAL LIFT" if fast else "", n, ref_max))
    print("lifts faster than the original's fastest: %d" % outside)
    return 0


if __name__ == "__main__":
    sys.exit(main())
