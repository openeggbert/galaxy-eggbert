#!/usr/bin/env python3
"""Mission 115, the works' last round and world 11's golden key (worlds3d/world115.vwr): brief
plans/briefs/world115.md.

Mobile Eggbert's level 115, the last of world 11, starts beside the golden key (21), which waits
until the eleven treasures are found: about the blue machine panel eighteen helicopters (13) to
fly up to treasures on high ledges, eleven skateboards (24) to ride over rows of mines (2), a row
of crates (12) to push into steps, columns of lightning (305), pits of lava, a fish (17) in a pool,
five eggs. Here the round is a hall with four wings in 3D: north the helicopters' gallery and its
ledges, east the skate run over two blocks of mines, south the crates' wall, west the lightning,
the pool and a second skate run; the golden key in its alcove off the hall.

    python3 tools/worlds/world115.py                 # writes worlds3d/world115.vwr
    python3 tools/worlds/world115.py --stage 1 out   # for flightcheck: no mines or lightning, the
                                                     # crates pushed into their steps
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, mine, wall  # noqa: E402
from station import water  # noqa: E402

LAVA = 68
BLITZ, EMITTER = 305, 304
WALK = 20


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world115.vwr"
    rng = random.Random(115)
    w = World(mission=115, sky=22)
    calm = stage is not None and stage >= 1

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.08, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def mines(cells):
        if not calm:
            for x, y, z in cells:
                mine(w, x, y, z)

    def lightning(x, z0, z1):
        for z in range(z0, z1 + 1):
            carve(x, WALK, z, x, WALK + 2, z)
            w.set(x, WALK + 3, z, EMITTER)
            if not calm:
                w.fill(x, WALK, z, x, WALK + 2, z, BLITZ)

    panel(2, 4, 2, 97, 44, 97)

    # ---- the hall (walk 20) and the golden key ----
    carve(40, WALK, 40, 60, WALK + 5, 60)
    w.spawn = (50, WALK, 52)
    carve(46, WALK, 61, 48, WALK + 2, 62)
    w.place(O.SECRET_EXIT, 47, WALK, 62)
    w.place(O.EGG, 58, WALK, 42)

    # ---- north: the helicopters' gallery (walk 20, open up to 42) ----
    # Eight helicopters on its floor, five on a ledge; treasures 1-4 on ledges round its walls at
    # walk 26, 30, 34 and 38, two pits of lava in its floor.
    carve(49, WALK, 30, 51, WALK + 2, 39)
    carve(34, WALK, 6, 66, 42, 29)
    for x in range(40, 48):
        w.place(O.HELICOPTER, x, WALK, 26)
    for x0, z0, x1, z1 in ((36, 8, 42, 14), (56, 8, 62, 14)):
        carve(x0, WALK - 3, z0, x1, WALK - 1, z1)
        w.fill(x0, WALK - 3, z0, x1, WALK - 2, z1, LAVA)
    for walk, x0, z0, x1, z1, tx, tz in ((26, 34, 6, 37, 9, 35, 7), (30, 63, 6, 66, 9, 65, 7),
                                        (34, 63, 25, 66, 29, 65, 27), (38, 34, 25, 37, 29, 35, 27)):
        panel(x0, walk - 2, z0, x1, walk - 1, z1)
        w.place(O.TREASURE, tx, walk, tz)
    panel(48, 30, 6, 54, 31, 9)
    for x in range(49, 54):
        w.place(O.HELICOPTER, x, 32, 7)
    w.place(O.EGG, 51, 32, 9)

    # ---- east: the skate run (walk 20) ----
    # Five skateboards at its head; two blocks of mines two deep across it (the skateboard rides
    # through); treasures 5-7 at its end.
    carve(61, WALK, 49, 70, WALK + 2, 51)
    carve(71, WALK, 46, 95, WALK + 3, 54)
    for z in (47, 48, 50, 52, 53):
        w.place(O.SKATEBOARD, 72, WALK, z)
    mines([(x, WALK, z) for x in (78, 79, 86, 87) for z in (47, 49, 51, 53)])
    for x, z in ((93, 47), (93, 50), (93, 53)):
        w.place(O.TREASURE, x, WALK, z)
    w.place(O.EGG, 90, WALK, 46)

    # ---- south: the crates' wall (walk 20 -> 23 -> 26) ----
    # A row of six crates by a wall three high, and over it a second wall three higher: push
    # crates to the walls' feet to make steps (one, then two stacked); treasures 8 and 9 on the
    # high shelf.
    carve(49, WALK, 61, 51, WALK + 2, 69)
    carve(36, WALK, 70, 64, WALK + 9, 94)
    panel(36, WALK, 86, 64, WALK + 2, 94)                                 # the first wall, walk 23
    panel(36, WALK + 3, 91, 64, WALK + 5, 94)                             # the second, walk 26
    for k, x in enumerate(range(42, 54, 2)):
        w.place(O.CRATE, x, WALK, 78 if not calm else 85)
    if calm:
        w.place(O.CRATE, 44, WALK + 3, 90)
        w.place(O.CRATE, 44, WALK + 4, 90)
    w.place(O.TREASURE, 40, WALK + 6, 93)
    w.place(O.TREASURE, 60, WALK + 6, 93)
    w.place(O.EGG, 50, WALK + 3, 88)

    # ---- west: the lightning, the pool and the second skate run (walk 20) ----
    # A curtain of lightning across the way; a pool with a fish (swim, or edge round on its rim);
    # five skateboards; a run over a block of mines and past a second curtain to treasures 10 and 11.
    carve(30, WALK, 49, 39, WALK + 2, 51)
    lightning(35, 49, 51)
    carve(4, WALK, 40, 29, WALK + 4, 60)
    carve(18, 14, 42, 28, WALK - 1, 58)
    water(w, 18, 14, 42, 28, WALK - 1, 58)
    w.place(O.FISH, 19, 16, 50, end=(27, 16, 50), advance=45, recede=45, stop_start=47, stop_end=47)
    for z in (41, 42, 43, 44, 45):
        w.place(O.SKATEBOARD, 16, WALK, z)
    mines([(x, WALK, z) for x in (10, 11) for z in (41, 43, 45)])
    lightning(7, 40, 46)
    w.place(O.TREASURE, 5, WALK, 42)
    w.place(O.TREASURE, 5, WALK, 58)
    w.place(O.EGG, 12, WALK, 57)
    w.place(O.SKATEBOARD, 14, WALK, 58)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
