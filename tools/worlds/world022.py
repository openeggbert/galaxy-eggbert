#!/usr/bin/env python3
"""Mission 022, the brick cellars and the green key (worlds3d/world022.vwr): brief
plans/briefs/world022.md.

Mobile Eggbert's level 022: from a balcony over a lava pit Blupi collects three treasures past a
water channel, one high up by pushing a crate under it; he goes down into the brick halls of
crates, lifts and a pool with a treasure at its bottom; a shaft of four lifts, one above the
other, has three treasures on its side ledges; the bars marked 4 and 7 open the way up to the
library, whose eighth treasure lies between saws; the bars marked 8 keep the green key, and
the key opens the green doors of the red arrow's shrine. Here: a white roof terrace (walk 11)
with a lava trench, a water channel and a plinth climbed from a pushed crate; the brick court
below with crates to push to its gallery, a lift across a lava trench, the pool and a lift up
to the bar 4; the white shaft of four lifts; the upper hall with the library of bookcases and
the two shrines.

    python3 tools/worlds/world022.py                   # writes worlds3d/world022.vwr
    python3 tools/worlds/world022.py --stage K out.vwr # lessons 1..K solved (levelcheck):
        1 terrace crate under the plinth, 2 court crate at the gallery, 3 bar 4 open,
        4 crate moved from the bar 7 and the bar open, 5 bar 8 open, 6 green door open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, rect  # noqa: E402
from palace import (BALL_MACHINES, BRICK, COLUMN, DEEP_WATER, GREEN_DOOR, LAVA, SAW, SHELVES,  # noqa: E402
                    STATUE, STONE, WATER, gate, parapet, stone, treasure_door, windows_row)

TERRACE = 10                 # the roof terrace's top: Blupi walks at 11
UPPER = 25                   # the upper hall's floor: walk 26
SPAWN = (44, 11, 76)


def brick(w, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, BRICK)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(22)
    w = World(mission=22, sky=25)

    def crate(start, solved, k):
        w.place(O.CRATE, *(solved if stage >= k else start))

    def door(x, y, z, tile, k):
        if stage < k:
            gate(w, x, y, z, tile)

    # A. The roof terrace (x 37-51, z 56-78), heading -Z from the spawn; a fall off it is a
    #    fall to death, as the original's pits.
    stone(w, 37, 0, 56, 51, TERRACE, 78, rng, chance=0.25)
    windows_row(w, 38, 50, 5, 78, rng, chance=0.5)
    parapet(w, [(37, z) for z in range(56, 79)] + [(51, z) for z in range(56, 79)] +
            [(x, 78) for x in range(37, 52)], TERRACE + 1, rng)
    w.place(O.EGG, 38, 11, 75)
    w.fill(38, TERRACE, 73, 50, TERRACE, 73, LAVA)                  # the lava trench: jump it
    w.place(O.TREASURE, 39, 11, 70)
    w.fill(38, TERRACE, 67, 50, TERRACE, 68, WATER)                 # the water channel, one deep
    w.place(O.TREASURE, 49, 11, 65)
    stone(w, 43, 11, 58, 45, 13, 60, rng, chance=0.3)               # the plinth, 3 high
    w.place(O.TREASURE, 44, 14, 59)
    crate((44, 11, 63), (44, 11, 61), 1)                            # pushed -Z to its foot

    # B. The brick court below (x 29-61, z 23-55, walk 1), walls 14 high; Blupi drops into it
    #    from the terrace's north end.
    brick(w, 28, 0, 22, 62, 0, 56)
    brick(w, 28, 1, 22, 28, 14, 56)
    brick(w, 62, 1, 22, 62, 14, 56)
    brick(w, 28, 1, 22, 62, 14, 22)
    brick(w, 28, 1, 56, 36, 14, 56)
    brick(w, 52, 1, 56, 62, 14, 56)
    w.place(O.EGG, 58, 1, 53)
    # The galleries (walk 4): the south one at z 39-46 climbed from a crate; beyond it a lava
    #    trench (z 34-38) crossed on a lift three wide; the north one (z 23-33) with the pool
    #    and the lift up to the ledge of the bar marked 4.
    brick(w, 29, 1, 39, 61, 3, 46)
    w.fill(29, 0, 34, 61, 0, 38, LAVA)
    brick(w, 29, 1, 23, 61, 3, 33)
    crate((40, 1, 51), (40, 1, 47), 2)                              # pushed -Z to the gallery
    for pos in ((35, 1, 53), (50, 1, 52), (56, 1, 50), (31, 1, 51)):
        w.place(O.CRATE, *pos)
    for x in (44, 45, 46):
        lift(w, (x, 3, 38), (x, 3, 34), ticks=30, stop=10)
    w.fill(30, 2, 24, 38, 2, 31, DEEP_WATER)                        # the pool, two deep as the
    w.fill(30, 3, 24, 38, 3, 31, WATER)                             # original's
    w.place(O.TREASURE, 34, 2, 27)
    w.place(O.EGG, 40, 4, 25)
    for z in (25, 29):
        w.set(49, 3, z, SAW)
    brick(w, 53, 4, 23, 61, 6, 31)                                  # the ledge, walk 7
    w.clear(52, 3, 27)
    lift(w, (52, 3, 27), (52, 6, 27), ticks=30, stop=10)
    w.place(O.EGG, 58, 7, 24)
    w.clear(62, 7, 27)
    door(62, 7, 27, treasure_door(4), 3)                            # into the shaft

    # C. The lift tower: a white yard (x 63-77, z 18-33, walk 7) behind the bar marked 4, and
    #    on it the four lifts one above the other (30, 40, 40 and 80 ticks as the original's)
    #    between white piers, treasures on the piers' side ledges. Open all round, so the
    #    camera keeps its view; a fall lands in the yard.
    stone(w, 63, 0, 18, 77, 6, 33, rng, chance=0.1)
    parapet(w, [(77, z) for z in range(18, 34)] + [(x, 33) for x in range(63, 78)], 7, rng)
    windows_row(w, 63, 77, 4, 33, rng, chance=0.5)
    w.place(O.EGG, 76, 7, 30)
    w.clear(66, 6, 23)
    lift(w, (66, 6, 23), (66, 9, 23), ticks=30, stop=10)            # L1, walk 7 -> 10
    stone(w, 67, 7, 22, 69, 9, 24, rng)                             # P1
    stone(w, 64, 7, 21, 65, 9, 25, rng)                             # the ledge of treasure 5
    w.place(O.TREASURE, 64, 10, 23)
    lift(w, (70, 9, 23), (70, 13, 23), ticks=40, stop=10)           # L2, walk 10 -> 14
    stone(w, 71, 7, 22, 73, 13, 24, rng)                            # P2
    stone(w, 71, 7, 26, 73, 13, 27, rng)                            # the ledge of treasure 6
    w.place(O.TREASURE, 72, 14, 27)
    lift(w, (72, 13, 21), (72, 17, 21), ticks=40, stop=10)          # L3, walk 14 -> 18
    stone(w, 71, 7, 18, 73, 17, 20, rng)                            # P3
    lift(w, (70, 17, 19), (70, UPPER, 19), ticks=80, stop=10)       # L4, walk 18 -> 26
    stone(w, 63, UPPER, 18, 69, UPPER, 20, rng)                     # the top landing
    w.place(O.TREASURE, 63, 26, 19)
    crate((66, 26, 18), (67, 26, 18), 4)                            # before the bar marked 7

    # D. The upper hall (x 30-74, z 4-16, walk 26), walls to y 33.
    stone(w, 29, UPPER, 3, 75, UPPER, 17, rng, chance=0.1)
    for x0, z0, x1, z1 in ((29, 3, 29, 17), (75, 3, 75, 17), (29, 3, 75, 3), (29, 17, 75, 17)):
        stone(w, x0, 26, z0, x1, 33, z1, rng, chance=0.25)
    w.clear(66, 26, 17)
    door(66, 26, 17, treasure_door(7), 4)                           # from the top landing
    # Its east part: columns, the ball machines and the statue; a passage three wide with a
    #    saw across its floor.
    for x in (60, 64, 68, 72):
        for z in (5, 15):
            w.bigdecor(COLUMN, x, 26, z)
    for icon, x, z in zip(BALL_MACHINES, (73, 73, 61), (8, 12, 5)):
        w.bigdecor(icon, x, 26, z)
    w.bigdecor(STATUE, 74, 26, 16)
    w.place(O.EGG, 73, 26, 5)
    stone(w, 55, 26, 4, 57, 33, 8, rng)
    stone(w, 55, 26, 12, 57, 33, 16, rng)
    for z in (9, 10, 11):
        w.set(56, UPPER, z, SAW)
    # The library (x 35-54): a wall of bookcases along z 10 parts its north and south aisles,
    #    open at its east end; treasure 8 beyond a row of saws in the north aisle.
    for x in range(35, 52):
        for y in range(26, 31):
            w.set(x, y, 10, rng.choice(SHELVES))
    for x, z in rect(38, 4, 54, 4) + rect(38, 16, 54, 16):
        for y in range(26, 29):
            w.set(x, y, z, rng.choice(SHELVES))
    for z in range(5, 10):
        w.set(44, UPPER, z, SAW)
    w.place(O.TREASURE, 40, 26, 7)
    w.place(O.EGG, 40, 26, 5)
    for x, z in ((47, 14), (48, 14), (48, 13)):
        w.set(x, 26, z, rng.choice(SHELVES))
    w.place(O.EGG, 48, 27, 14)
    # The shrines at its west end: the green key behind the bars marked 8 (north aisle), the
    #    red arrow behind the green door (south aisle); both entered from the east.
    for z0, z1, dz, tile, k in ((5, 9, 7, treasure_door(8), 5), (11, 15, 13, GREEN_DOOR, 6)):
        stone(w, 30, 26, z0, 34, 29, z1, rng, chance=0.3)
        w.fill(31, 26, z0 + 1, 33, 27, z1 - 1, 0)
        w.clear(34, 26, dz)
        door(34, 26, dz, tile, k)
    w.place(O.KEY2, 32, 26, 7)
    w.place(O.EXIT, 32, 26, 13)

    w.spawn = SPAWN
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world022.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
