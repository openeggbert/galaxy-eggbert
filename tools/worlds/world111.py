#!/usr/bin/env python3
"""Mission 111, the hangar shaft (worlds3d/world111.vwr): brief plans/briefs/world111.md.

Mobile Eggbert's level 111 climbs through the blue machine panel (10): from Blupi's start a
passage over pits crossed on lifts under hanging mines (3: walk, never jump), mines on posts
between bobbing lifts, a bulldozer (4) pushing to and fro; a hangar of thirteen helicopters (13)
at the foot of a tall shaft, eggs by its pool; four treasures on ledges up the shaft (fly up to
them), the invert (40) on one; at the top a corridor of mines (2) and the room of the exit with
four eggs. Here the climb is the same in 3D: the start cave, the passage of pits, the bulldozer's
corridor, the hangar and its pool, the shaft and its ledges, the corridor of mines, the exit.

    python3 tools/worlds/world111.py                 # writes worlds3d/world111.vwr
    python3 tools/worlds/world111.py --stage 1 out   # for flightcheck: no hanging mines, mines
                                                     # or bulldozer
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402
from station import water  # noqa: E402

WALK, TOPWALK = 6, 34


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world111.vwr"
    rng = random.Random(111)
    w = World(mission=111, sky=0)
    calm = stage is not None and stage >= 1

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    panel(4, 0, 40, 84, TOPWALK + 6, 60)

    # ---- the start cave and the passage of pits (walk 6) ----
    # Two pits crossed on lifts (3 wide) under four mines hanging a cell above Blupi's head (walk
    # on, never jump); then three small pits, a lift bobbing in each, mines on the posts between
    # them (hop from lift to post to lift).
    carve(6, WALK, 44, 14, WALK + 4, 56)
    w.spawn = (8, WALK, 50)
    carve(15, WALK, 48, 37, WALK + 3, 52)
    for x0 in (17, 23):
        carve(x0, 1, 48, x0 + 4, WALK - 1, 52)
        for x in (x0, x0 + 1):
            for z in (49, 50, 51):
                lift(w, (x, WALK - 1, z), (x + 3, WALK - 1, z), ticks=30, stop=30)
        for x in range(x0, x0 + 5, 2):
            hazard(x, WALK + 1, 50, hanging=True)
        hazard(x0 + 1, WALK + 1, 49, hanging=True)
    for x in (29, 32, 35):
        carve(x, 1, 48, x + 1, WALK - 1, 52)
        for z in (49, 50, 51):
            lift(w, (x, WALK - 2, z), (x, WALK - 1, z), ticks=10, stop=10)
            lift(w, (x + 1, WALK - 2, z), (x + 1, WALK - 1, z), ticks=10, stop=10)
        hazard(x + 2, WALK, 50)

    # ---- the bulldozer's corridor (walk 6) ----
    # A bulldozer pushing along it (90 ticks, dwell 20); alcoves in its walls to step into.
    carve(38, WALK, 49, 45, WALK + 2, 51)
    for x in (40, 43):
        carve(x, WALK, 48, x, WALK + 1, 48)
        carve(x, WALK, 52, x, WALK + 1, 52)
    if not calm:
        w.place(O.BULLDOZER, 39, WALK, 50, end=(44, WALK, 50), advance=90, recede=90, stop_start=20,
                stop_end=20)

    # ---- the hangar and the shaft (walk 6 -> 34) ----
    # Nine helicopters in a row, four more by a pool with two eggs; the shaft open above it to the
    # top, treasures on four ledges up its walls (walk 12, 18, 24, 30), the invert on a fifth.
    carve(46, WALK, 42, 64, TOPWALK + 4, 58)
    for x in range(47, 56):
        w.place(O.HELICOPTER, x, WALK, 44)
    carve(56, 2, 50, 63, WALK - 1, 57)
    water(w, 56, 2, 50, 63, WALK - 1, 57)
    for x in (52, 53, 54, 55):
        w.place(O.HELICOPTER, x, WALK, 56)
    w.place(O.EGG, 50, WALK, 52)
    w.place(O.EGG, 50, WALK, 56)
    for walk, x0, z0, x1, z1, tx, tz in ((12, 46, 42, 49, 45, 47, 43), (18, 61, 42, 64, 45, 63, 43),
                                        (24, 61, 55, 64, 58, 63, 57), (30, 46, 55, 49, 58, 47, 57)):
        panel(x0, walk - 2, z0, x1, walk - 1, z1)
        w.place(O.TREASURE, tx, walk, tz)
    panel(54, 20, 42, 57, 21, 43)
    w.place(O.INVERT, 55, 22, 42)

    # ---- the corridor of mines and the exit's room (walk 34) ----
    carve(65, TOPWALK, 48, 76, TOPWALK + 2, 52)
    for x, z in ((67, 49), (70, 51), (73, 49), (75, 51)):
        hazard(x, TOPWALK, z)
    carve(77, TOPWALK, 44, 83, TOPWALK + 3, 56)
    for x in (78, 79, 80, 81):
        w.place(O.EGG, x, TOPWALK, 45)
    w.place(O.EXIT, 82, TOPWALK, 54)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
