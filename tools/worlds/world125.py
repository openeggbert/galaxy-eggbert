#!/usr/bin/env python3
"""Mission 125, the golden key's cave and world 12's end (worlds3d/world125.vwr): brief
plans/briefs/world125.md.

Mobile Eggbert's level 125, the last of world 12, has no treasure: Blupi takes a helicopter (13;
twenty-two of them) through a dark cave of brick walls and pillars to the golden key (21), which
hangs in a ring of moving mines (16) bobbing round it; on the way rows of mines (2) on the ledges,
columns of lightning (305) over a corridor of hanging mines (3), a bird (20), a pool with a fish
(17), a bulldozer (4), crates (12), four eggs. Here the cave is a dark hall with brick in 3D: the
start ledge and its helicopters, the corridor of lightning, the pool, the bird's hall and, in its
far corner, the key on a pillar in its ring of mines.

    python3 tools/worlds/world125.py                 # writes worlds3d/world125.vwr
    python3 tools/worlds/world125.py --stage 1 out   # for flightcheck: no mines, moving or hanging
                                                     # mines, lightning, bird or bulldozer
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from station import BRICK, brick, water  # noqa: E402

BLITZ, EMITTER = 305, 304
WALK = 20


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world125.vwr"
    rng = random.Random(125)
    w = World(mission=125, sky=2)
    calm = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    brick(w, 4, 4, 10, 96, 44, 90, rng)
    carve(8, 8, 14, 92, 40, 86)                                            # the cave, its floor walk 8
    for x, z in ((30, 30), (60, 70)):                                    # a fall: helicopters on the floor
        w.place(O.HELICOPTER, x, 8, z)

    # ---- the start ledge (walk 20) and its helicopters ----
    brick(w, 8, 17, 44, 20, WALK - 1, 56, rng)
    w.spawn = (12, WALK, 50)
    for z in (45, 47, 49, 51, 53, 55):
        w.place(O.HELICOPTER, 18, WALK, z)
    w.place(O.EGG, 9, WALK, 55)
    for x, z in ((14, 45), (14, 55)):
        hazard(x, WALK, z)

    # ---- the corridor of lightning (walk 20, east, z 48-52) ----
    # A tube of brick across the cave: mines hang in its roof at Blupi's head height (fly low, or
    # walk), three curtains of lightning across it, a mine on its floor.
    brick(w, 21, 17, 46, 50, 25, 54, rng)
    carve(21, WALK, 48, 50, WALK + 3, 52)
    for x in range(23, 49, 3):
        hazard(x, WALK + 3, 50, hanging=True)
    for x in (28, 36, 44):
        for z in range(48, 53):
            w.set(x, WALK + 4, z, EMITTER)
            if not calm:
                w.fill(x, WALK, z, x, WALK + 2, z, BLITZ)
    hazard(40, WALK, 49)

    # ---- the pool (walk 20) ----
    brick(w, 51, 12, 44, 62, WALK - 1, 56, rng)
    carve(53, 14, 46, 60, WALK - 1, 54)
    water(w, 53, 14, 46, 60, WALK - 1, 54)
    w.place(O.FISH, 54, 16, 50, end=(59, 16, 50), advance=45, recede=45, stop_start=47, stop_end=47)
    w.place(O.EGG, 52, WALK, 55)
    for x, z in ((61, 45), (61, 55)):
        w.place(O.HELICOPTER, x, WALK, z)

    # ---- the bird's hall: ledges of mines and crates ----
    # Ledges at several heights round the cave's east end with rows of mines along them, crates,
    # a bulldozer patrolling one, a bird crossing the hall; spare helicopters on them.
    for x0, z0, x1, z1, walk, row in ((64, 20, 74, 26, 26, True), (78, 20, 90, 26, 30, True),
                                       (64, 74, 74, 80, 24, True), (78, 74, 90, 80, 32, False),
                                       (82, 44, 90, 56, 22, False)):
        brick(w, x0, walk - 2, z0, x1, walk - 1, z1, rng)
        if row:
            for x in range(x0 + 1, x1, 2):
                hazard(x, walk, z0 + 3)
        w.place(O.HELICOPTER, x1, walk, z1)
    for x, walk, z in ((66, 26, 22), (68, 26, 22), (80, 24, 76)):
        w.place(O.CRATE, x, walk, z)
    w.place(O.EGG, 89, 32, 79)
    if not calm:
        w.place(O.BULLDOZER, 79, 30, 21, end=(89, 30, 21), advance=60, recede=60, stop_start=20,
                stop_end=20)
        w.place(O.BIRD, 66, 28, 40, end=(88, 28, 60), advance=80, recede=80, stop_start=8, stop_end=8)

    # ---- the golden key in its ring of mines ----
    # A pillar in the hall's far east, the key on its top (walk 30); eight mines bobbing round it,
    # two up and down, at different paces; a lift beside it from the floor of the east ledge.
    brick(w, 85, 8, 49, 87, 29, 51, rng)
    w.place(O.SECRET_EXIT, 86, 30, 50)
    if not calm:
        for k, (x, z) in enumerate(((84, 48), (86, 47), (88, 48), (89, 50), (88, 52), (86, 53), (84, 52), (83, 50))):
            w.place(O.MOVING_MINE, x, 29 + (k % 2), z, end=(x, 31 + (k % 2), z), advance=14 + 2 * k,
                    recede=14 + 2 * k, stop_start=0, stop_end=0)
    for z in (44, 45):
        lift(w, (84, WALK + 1, z), (84, 29, z), ticks=60, stop=30)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
