#!/usr/bin/env python3
"""Mission 122, the brick warren (worlds3d/world122.vwr): brief plans/briefs/world122.md.

Mobile Eggbert's level 122 is a warren of short tunnels in brick: from Blupi's start by the exit a
shaft up, little rooms off it, ten treasures in pockets of the tunnels, sixteen mines (2) along
them, six moving mines (16) bobbing in shafts, five crates (12) plugging the ways (push them on),
a bulldozer (4), the drink (30) that hides him from them, two inverts (40), twelve eggs. Here the
warren is cut in a block of brick in 3D: a central shaft climbed in steps with three galleries
off it, each a loop of tunnels with pockets of treasure, mines and crates.

    python3 tools/worlds/world122.py                 # writes worlds3d/world122.vwr
    python3 tools/worlds/world122.py --solved out    # no mines, moving mines or bulldozer, the
                                                     # crates pushed aside (for levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from station import BRICK, brick  # noqa: E402

FLOORS = (6, 14, 22)                 # the three galleries' walking levels
SHAFT = (48, 52, 48, 52)             # x0, x1, z0, z1


def main(argv) -> int:
    solved = len(argv) > 2 and argv[1] == "--solved"
    out = Path(argv[2]) if solved else Path(__file__).resolve().parents[2] / "worlds3d" / "world122.vwr"
    rng = random.Random(122)
    w = World(mission=122, sky=24)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z):
        if not solved:
            mine(w, x, y, z)

    def bob(x, y, z, height, ticks):
        if not solved:
            w.place(O.MOVING_MINE, x, y, z, end=(x, y + height, z), advance=ticks, recede=ticks, stop_start=0,
                    stop_end=0)

    brick(w, 20, 2, 20, 80, 28, 80, rng)

    # ---- the central shaft (x 46-54, z 46-54): a stair up from gallery to gallery ----
    # Steps two up each along its west and north walls, a landing at each gallery's level, and a
    # passage two wide out of it to the gallery's loop: south from the bottom, north from the
    # middle, east from the top.
    carve(46, FLOORS[0], 46, 54, FLOORS[2] + 3, 54)
    stair = [(46, 52), (46, 50), (46, 48), (46, 46), (48, 46), (50, 46), (52, 46), (54, 46)]
    for k, (x, z) in enumerate(stair):
        top = FLOORS[0] + 2 + 2 * k                                       # walk 8 ... 22
        w.fill(x, FLOORS[0], z, x + 1, top - 1, z + 1, BRICK)
    w.spawn = (50, FLOORS[0], 51)
    w.place(O.EXIT, 52, FLOORS[0], 53)
    carve(49, FLOORS[0], 55, 50, FLOORS[0] + 2, 63)                       # bottom: south
    carve(46, FLOORS[1], 37, 47, FLOORS[1] + 2, 45)                       # middle: north, at x 46-47
    carve(55, FLOORS[2], 46, 63, FLOORS[2] + 2, 47)                       # top: east

    # ---- the galleries: a loop of tunnels round the shaft at each level ----
    # Each loop runs round the shaft at 8 cells from it, a tunnel two wide and three high, joined
    # to the shaft by a short passage; pockets off it holding the treasures; mines along it (step
    # round in the other lane), a crate plugging a passage (push it into its pocket), moving mines
    # bobbing in shafts through the loop's floor and roof.
    treasures = [[(32, 50), (68, 36), (50, 66)], [(50, 32), (32, 66), (68, 64), (68, 50)],
                 [(32, 34), (50, 68), (68, 34)]]
    for g, f in enumerate(FLOORS):
        lo, hi = 36, 64
        for x, z, x2, z2 in ((lo, lo, hi, lo + 1), (lo, hi - 1, hi, hi), (lo, lo, lo + 1, hi), (hi - 1, lo, hi, hi)):
            carve(x, f, z, x2, f + 2, z2)
        for k, (tx, tz) in enumerate(treasures[g]):
            px = max(min(tx, hi + 4), lo - 4)
            pz = max(min(tz, hi + 4), lo - 4)
            ax = lo if tx < lo else hi if tx > hi else tx
            az = lo if tz < lo else hi if tz > hi else tz
            carve(min(px, ax), f, min(pz, az), max(px, ax), f + 1, max(pz, az))
            w.place(O.TREASURE, px, f, pz)
        for x, z in ((40, lo + 1), (hi, 44), (56, hi), (lo, 58)):
            hazard(x, f, z)
        for x, z in ((46, lo + 1), (lo + 1, 52)):
            bob(x, f - 1 if g else f, z, 2, 20 + 4 * g)
        if g < 2:                                                         # a crate in the north tunnel
            if solved:
                carve(58, f, lo - 1, 58, f + 1, lo - 1)                   # pushed into a pocket
            w.place(O.CRATE, 58, f, lo - 1 if solved else lo)
        for x, z in ((44, hi), (hi, 40), (lo, 46), (60, lo)):
            w.place(O.EGG, x, f, z)

    # The drink in a pocket of the top gallery, the bulldozer patrolling the middle one's south
    # side, the two inverts in pockets of the bottom one.
    carve(64, FLOORS[2], 64, 66, FLOORS[2] + 1, 66)
    w.place(O.DRINK, 66, FLOORS[2], 66)
    if not solved:
        w.place(O.BULLDOZER, 38, FLOORS[1], 63, end=(62, FLOORS[1], 63), advance=120, recede=120,
                stop_start=20, stop_end=20)
    for x, z in ((34, 36), (66, 62)):
        carve(x, FLOORS[0], z, x + 1, FLOORS[0] + 1, z)
        w.place(O.INVERT, x, FLOORS[0], z)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
