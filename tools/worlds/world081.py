#!/usr/bin/env python3
"""Mission 081, the sky steps (worlds3d/world081.vwr): brief plans/briefs/world081.md.

Mobile Eggbert's level 081 is a climb through the sky from Blupi's start low in the west to the
red arrow high in the east: small machine platforms banded with pipes, and between them the
saucers of the Temp tile (324), which vanish together for a moment every four seconds (Blupi
standing on one then falls); twenty-one birds fly round the way, two still mines hover over a
causeway of saucers laid over lava, eleven eggs wait on the platforms, a fall is the end. Here
the climb winds round the sky in 3D: thirteen platforms rising from walk 20 to walk 56, joined
by straight chains of saucers two by two one gap apart, birds crossing the chains, and halfway up the
causeway east: saucers in three lanes over a ribbon of lava, rests of panel between them, the
mines over its middle lane.

    python3 tools/worlds/world081.py                 # writes worlds3d/world081.vwr
    python3 tools/worlds/world081.py --stage 1 out   # for levelcheck: no birds or mines
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World  # noqa: E402
from skyworks import BOLTS, PANEL, PIPES, TEMP  # noqa: E402

LAVA = 68

# The platforms in the order of the climb: (x0, z0, x1, z1, walk). Each shares a line along X
# or Z with the next, which a chain of saucers follows.
PLATFORMS = [
    (10, 76, 16, 82, 20),    # the start
    (10, 60, 16, 66, 23),
    (26, 60, 32, 66, 26),
    (26, 44, 32, 50, 29),
    (10, 44, 16, 50, 32),
    (10, 26, 16, 32, 35),
    (26, 24, 34, 32, 38),
    (40, 18, 48, 26, 40),    # the causeway's west end
    (72, 18, 80, 26, 40),    # its east end
    (72, 36, 80, 42, 44),
    (56, 36, 64, 42, 47),
    (56, 52, 64, 58, 50),
    (72, 52, 80, 58, 53),
    (76, 68, 84, 74, 56),    # the arrow's
]
CAUSEWAY = 7                 # PLATFORMS[7] -> PLATFORMS[8] is the causeway, not a chain
EGGS = [(11, 0, 77), (12, 0, 77), (29, 0, 63), (13, 0, 47), (30, 0, 26), (74, 0, 20), (76, 0, 20),
        (78, 0, 20), (60, 0, 38), (58, 0, 56), (78, 0, 56)]


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world081.vwr"
    rng = random.Random(81)
    w = World(mission=81, sky=26)
    solved = stage is not None and stage >= 1

    def bird(start, end, ticks):
        if not solved:
            w.place(O.BIRD, *start, end=end, advance=ticks, recede=ticks, stop_start=8, stop_end=8)

    def platform(x0, z0, x1, z1, walk):
        """A deck of panel over a band of pipe round its flank, bolts hanging under its corners."""
        w.fill(x0, walk - 2, z0, x1, walk - 1, z1, PANEL)
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                if x in (x0, x1) or z in (z0, z1):
                    w.set(x, walk - 2, z, rng.choice(PIPES))
        for x, z in ((x0, z0), (x1, z0), (x0, z1), (x1, z1)):
            w.set(x, walk - 3, z, rng.choice(BOLTS))

    for p in PLATFORMS:
        platform(*p)

    # The chains: from the edge of a platform to the next along their shared line, saucers two by
    # two with a gap of one between them (each hop a jump of two), the walking height rising
    # evenly (never more than two a hop); a bird crossing each chain at its middle.
    for k in range(len(PLATFORMS) - 1):
        if k == CAUSEWAY:
            continue
        ax0, az0, ax1, az1, aw = PLATFORMS[k]
        bx0, bz0, bx1, bz1, bw = PLATFORMS[k + 1]
        along_z = max(ax0, bx0) <= min(ax1, bx1)
        if along_z:
            line = (max(ax0, bx0) + min(ax1, bx1)) // 2
            a_edge, b_edge = (az0, bz1) if bz1 < az0 else (az1, bz0)
        else:
            line = (max(az0, bz0) + min(az1, bz1)) // 2
            a_edge, b_edge = (ax0, bx1) if bx1 < ax0 else (ax1, bx0)
        step = 1 if b_edge > a_edge else -1
        free = abs(b_edge - a_edge) - 1
        saucers, d = [], 2                                             # a gap, then saucer, gap...
        while free - (d - 1) >= 2:                                     # ... ending on a gap of one
            length = 1 if free - (d - 1) in (2, 4) else 2
            saucers.append([a_edge + step * (d + i) for i in range(length)])
            d += length + 1
        hops = len(saucers) + 1
        for i, run in enumerate(saucers):
            walk = aw + round((bw - aw) * (i + 1) / hops)
            for a in run:
                for c in (line, line + 1):
                    x, z = (c, a) if along_z else (a, c)
                    w.set(x, walk - 1, z, TEMP)
        if saucers:
            mid = saucers[len(saucers) // 2][0]
            walk = aw + round((bw - aw) * (len(saucers) // 2 + 1) / hops)
            if along_z:
                bird((line - 3, walk, mid), (line + 4, walk, mid), 24)
            else:
                bird((mid, walk, line - 3), (mid, walk, line + 4), 24)

    # The causeway east (walk 40, z 21-23): saucers in three lanes over a ribbon of lava on a
    # panel keel, rests of panel every six, the two still mines over the middle lane.
    w.fill(49, 37, 21, 71, 37, 23, PANEL)
    w.fill(49, 38, 21, 71, 38, 23, LAVA)
    w.fill(49, 39, 21, 71, 39, 23, TEMP)
    for x in (55, 61, 67):
        w.fill(x, 38, 21, x, 39, 23, PANEL)
    if not solved:
        for x in (58, 64):
            w.place(O.MOVING_MINE, x, 40, 22, end=(x, 40, 22), advance=10, recede=10, stop_start=0, stop_end=0)
    for x0, x1 in ((50, 54), (62, 66)):
        bird((x0, 42, 19), (x1, 42, 25), 32)

    # More birds round the higher platforms and the arrow's.
    for start, end, ticks in (((70, 46, 30), (82, 46, 30), 32), ((52, 49, 46), (52, 49, 34), 24),
                              ((66, 52, 60), (70, 52, 50), 24), ((86, 56, 62), (86, 56, 76), 32),
                              ((74, 58, 66), (86, 58, 66), 16), ((20, 30, 38), (20, 34, 22), 24),
                              ((36, 38, 22), (36, 40, 34), 32)):
        bird(start, end, ticks)

    for x, _, z in EGGS:
        walk = next(p[4] for p in PLATFORMS if p[0] <= x <= p[2] and p[1] <= z <= p[3])
        w.place(O.EGG, x, walk, z)
    ex0, ez0, ex1, ez1, ew = PLATFORMS[-1]
    w.place(O.EXIT, (ex0 + ex1) // 2, ew, (ez0 + ez1) // 2)

    w.spawn = (13, 20, 80)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
