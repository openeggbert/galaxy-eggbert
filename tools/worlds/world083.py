#!/usr/bin/env python3
"""Mission 083, the grey works (worlds3d/world083.vwr): brief plans/briefs/world083.md.

Mobile Eggbert's level 083 is a machine of panels under a grey sky with three treasures: from
Blupi's start a shuttle over a pool and a climb to treasure 1, door "1" (421), a cavern below of
platforms joined by Temp saucers (324) over pools of water and lava, a long lift up, a block of
rooms with moving mines (16), mines, bars and treasure 2, a lift moving across and up beside
mines rising and falling in shafts, door "3" (423) by the pocket of treasure 3, and at the end
a bridge of saucers over lava and a stair of saucers up to the red arrow. Here the machine is
built round a cavern in 3D: the start hall with its pool, the climb, the cavern, the lift column,
the block of rooms, the east shafts, door "3", the arrow's hall.

    python3 tools/worlds/world083.py                 # writes worlds3d/world083.vwr
    python3 tools/worlds/world083.py --stage K out   # for levelcheck: K treasures taken, door "1"
                                                     # open from K >= 1, door "3" from K >= 3;
                                                     # K >= 1 no mines or moving mines, the
                                                     # shuttles as floors
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, WARNING, World, lift, mine  # noqa: E402
from skyworks import BOLTS, PANEL, PIPE_MACHINES, PIPES, TEMP, panel  # noqa: E402

LAVA, WATER, DEEP = 68, 92, 91
BAR = 138


def door(n):
    return 420 + n


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world083.vwr"
    rng = random.Random(83)
    w = World(mission=83, sky=15)
    calm = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def gate(x0, y0, z0, x1, y1, z1, n):
        carve(x0, y0, z0, x1, y1, z1)
        if stage is None or stage < n:
            w.fill(x0, y0, z0, x1, y1, z1, door(n))

    def drifter(start, end, ticks):
        if not calm:
            w.place(O.MOVING_MINE, *start, end=end, advance=ticks, recede=ticks, stop_start=0, stop_end=0)

    def ferry(cells, end, ticks, stop):
        """Lifts in step from `cells` by the offset `end`; in the calm stages floors over the run."""
        dx, dy, dz = end
        for x, y, z in cells:
            if calm:
                w.fill(min(x, x + dx), min(y, y + dy), min(z, z + dz), max(x, x + dx), max(y, y + dy),
                       max(z, z + dz), PANEL)
            else:
                lift(w, (x, y, z), (x + dx, y + dy, z + dz), ticks=ticks, stop=stop)

    panel(w, 6, 8, 16, 96, 54, 76, rng, fitting_chance=0.0)     # fittings would make every face show

    # ---- the start hall (walk 30) and the climb to treasure 1 ----
    # A pool flush with the floor across the hall, a shuttle (2 x 2, 50 ticks) over it from the
    # west ledge to the east one; north of the west ledge a stair of steps two apart up to the
    # ledge of treasure 1 (walk 40), a warning panel at its foot. Door "1" in the east wall.
    carve(10, 30, 60, 30, 36, 72)
    carve(14, 26, 61, 26, 29, 71)
    w.fill(14, 26, 61, 26, 28, 71, DEEP)
    w.fill(14, 29, 61, 26, 29, 71, WATER)
    ferry([(15, 29, 65), (15, 29, 66), (16, 29, 65), (16, 29, 66)], (10, 0, 0), 50, 10)
    for x in range(10, 31):
        w.set(x, 36, 72, rng.choice(PIPES))
    carve(10, 30, 50, 13, 42, 59)
    for k, z in enumerate((58, 56, 54, 52)):
        w.fill(10, 30, z, 13, 31 + 2 * k, z + 1, PANEL)                 # walk 32, 34, 36, 38
    w.fill(10, 30, 50, 13, 39, 51, PANEL)                               # the ledge, walk 40
    w.place(O.TREASURE, 11, 40, 50)
    w.set(13, 30, 59, WARNING)
    w.spawn = (11, 30, 67)
    gate(31, 30, 66, 31, 31, 67, 1)

    # ---- the cavern (walk 14) ----
    # Behind door "1" a ledge over the cavern; platforms of panel at walk 22, 20, 18, 16 east
    # across it, joined by chains of saucers (two by two, a gap of one between); its floor panel
    # with pools of lava and water (a fall on the floor means the stair back up to the ledge).
    carve(32, 14, 56, 66, 34, 80)
    w.fill(32, 13, 56, 33, 29, 80, PANEL)                               # the west wall under the ledge
    w.fill(32, 29, 64, 35, 29, 69, PANEL)                               # the ledge, walk 30
    for x0, z0, x1, z1 in ((38, 60, 43, 66), (48, 70, 53, 76), (56, 58, 61, 64)):
        w.fill(x0, 13, z0, x1, 13, z1, LAVA)
    for x0, z0, x1, z1 in ((44, 72, 47, 78), (58, 70, 63, 76)):
        w.fill(x0, 12, z0, x1, 13, z1, WATER)
    for k, x in enumerate((34, 36, 38, 40)):
        w.fill(x, 14, 77, x + 1, 15 + 2 * k, 79, PANEL)                 # the stair back: 16 ... 22
    w.fill(32, 14, 77, 33, 29, 80, PANEL)
    carve(34, 22, 77, 41, 28, 79)
    w.fill(34, 14, 64, 35, 29, 69, PANEL)                               # under the ledge
    platforms = [(37, 64, 41, 68, 22), (47, 64, 51, 68, 20), (47, 54 + 2, 51, 60, 18),
                 (57, 66, 61, 70, 16), (63, 66, 66, 70, 16)]
    for x0, z0, x1, z1, walk in platforms:
        w.fill(x0, walk - 2, z0, x1, walk - 1, z1, PANEL)
        for x in range(x0, x1 + 1):
            w.set(x, walk - 2, z0, rng.choice(PIPES))
    chains = [[(36, 23, 66)], [(42, 22, 66), (43, 22, 66), (45, 21, 66)], [(49, 19, 62), (49, 19, 63)],
              [(52, 18, 59), (53, 18, 59), (55, 17, 59), (56, 17, 62), (56, 17, 64)], [(62, 15, 68)]]
    for chain in chains:
        for x, y, z in chain:
            for dz in (0, 1):
                w.set(x, y - 1, z + dz, TEMP)

    # ---- the lift column (walk 16 -> 31) and the block of rooms (walk 31) ----
    # At the cavern's east end a lift (2 x 2, 90 ticks) up the column to the block of rooms: a
    # mine rising and falling in a shaft across the way, another sweeping a room, treasure 2 in
    # it, two mines; bars over a pit of lava to hang across.
    carve(66, 16, 66, 67, 33, 67)
    for x in (66, 67):
        for z in (66, 67):
            lift(w, (x, 15, z), (x, 30, z), ticks=90, stop=10)
    carve(68, 31, 56, 80, 34, 70)
    carve(68, 28, 62, 70, 30, 62)
    drifter((69, 28, 62), (69, 34, 62), 40)
    drifter((72, 31, 58), (78, 31, 58), 40)
    w.place(O.TREASURE, 76, 31, 57)
    if not calm:
        mine(w, 74, 31, 60)
        mine(w, 79, 31, 64)
    carve(72, 27, 65, 77, 30, 67)
    w.fill(72, 27, 65, 77, 27, 67, LAVA)
    w.fill(72, 31, 66, 77, 31, 66, BAR)
    if calm:
        w.fill(72, 30, 65, 77, 30, 67, PANEL)
    for x in (68, 80):
        for y in (31, 33):
            w.set(x, y, 69, rng.choice(PIPE_MACHINES))

    # ---- the east shafts, door "3" and the pocket (walk 31 -> 37) ----
    # East of the block a lift moving across and up at once (50 ticks) beside two shafts where
    # mines rise and fall; it lands on a gallery where door "3" closes the way north; below the
    # gallery the pocket of treasure 3, reached by a drop and a stair back.
    carve(81, 31, 60, 92, 37, 64)
    ferry([(81, 30, 62), (82, 30, 62)], (5, 5, -2), 50, 10)
    for x in (84, 86):
        carve(x, 28, 63, x, 38, 63)
        drifter((x, 29, 63), (x, 37, 63), 30)
    w.fill(87, 30, 58, 92, 35, 62, PANEL)                               # the gallery, walk 36
    carve(87, 36, 58, 92, 38, 62)
    carve(89, 31, 63, 92, 35, 66)                                       # the pocket, walk 31
    w.place(O.TREASURE, 91, 31, 65)
    for k, x in enumerate((89, 90)):
        w.fill(x, 31, 63, x, 32 + 2 * k, 63, PANEL)                    # a stair back: 33, 35
    gate(89, 36, 57, 90, 37, 57, 3)

    # ---- the arrow's hall (walk 36 -> 50) ----
    # North of door "3" a hall over a deep pit: a bridge on a keel across it, two lanes of saucers
    # over lava with lanes of lava beside them and rests of panel, an egg at its north end; then a
    # stair of saucers rising two at a time to the arrow's ledge (walk 50). A fall into the pit
    # means the stair back up its east wall.
    carve(84, 20, 20, 96, 54, 56)
    w.fill(88, 33, 26, 91, 33, 56, PANEL)                               # the keel
    w.fill(88, 34, 26, 91, 35, 56, LAVA)
    w.fill(89, 35, 30, 90, 35, 56, 10 if calm else TEMP)
    w.fill(89, 34, 30, 90, 34, 56, LAVA)
    for z in (50, 44, 38):
        w.fill(89, 34, z, 90, 35, z, PANEL)
    w.fill(87, 33, 26, 92, 35, 29, PANEL)                               # the far end, walk 36
    w.place(O.EGG, 91, 36, 27)
    stair = [(88, 37, 24), (88, 39, 22), (90, 41, 21), (92, 43, 22), (92, 45, 24), (90, 47, 25)]
    for x, y, z in stair:
        w.fill(x, y, z, x + 1, y, z + 1, 10 if calm else TEMP)
    w.fill(86, 48, 25, 89, 49, 28, PANEL)                               # the arrow's ledge, walk 50
    w.place(O.EXIT, 87, 50, 26)
    w.fill(84, 19, 20, 96, 19, 56, PANEL)                               # the pit's floor, walk 20
    for k in range(7):                                                  # its stair back, east wall
        w.fill(95, 20, 42 + 2 * k, 96, 21 + 2 * k, 43 + 2 * k, PANEL)   # walk 22 ... 34
    w.fill(92, 20, 56, 96, 35, 58, PANEL)                               # a landing onto the gallery
    for x, z in ((84, 20), (96, 20), (84, 56)):
        w.set(x, 37, z, rng.choice(BOLTS))

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
