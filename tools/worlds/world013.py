#!/usr/bin/env python3
"""Mission 013, helicopter, diving and skateboard (worlds3d/world013.vwr): brief
plans/briefs/world013.md.

Mobile Eggbert's level 013: take a helicopter in the start room and fly; leave it before the
water ("it dislikes water"), dive and swim on; take another helicopter and fly up a shaft for
the three treasures; fetch a skateboard from the top-left corner and ride it along a corridor
over mines (it runs them over) and water (jump it); leave it at the red arrow. Here the rooms are
sealed machine halls, so each vehicle is used where the lesson wants it: the hangar opens to the
pool hall only through a window up high, the treasure shaft is reached only by diving through
an underwater tunnel, and the skate tunnel only from the gallery atop the shaft's cliff.

    python3 tools/worlds/world013.py         # writes worlds3d/world013.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, MONITOR, O, WATER, World, wall  # noqa: E402

X0, X1 = 40, 56
SPAWN = (48, 1, 64)


def shell(w, x0, y0, z0, x1, y1, z1, rng):
    """Walls and roof round a hall (its floor is built separately)."""
    for x in range(x0, x1 + 1):
        for z in range(z0, z1 + 1):
            edge = x in (x0, x1) or z in (z0, z1)
            for y in range(y0, y1 + 1):
                if edge or y == y1:
                    w.set(x, y, z, rng.choice(MACHINE_FACES) if rng.random() < 0.2 else MACHINE_FLOOR)


def main() -> int:
    rng = random.Random(13)
    w = World(mission=13, sky=0)

    # A. The hangar (walk 1): two helicopters; a window up high in its north wall.
    wall(w, X0, 0, 55, X1, 0, 67, rng, face_chance=0.1)
    shell(w, X0, 1, 55, X1, 10, 67, rng)
    w.fill(43, 6, 55, 53, 8, 55, 0)                          # the window
    for x in (44, 52):
        w.place(O.HELICOPTER, x, 1, 60)
    w.hint_zone(0, X0 + 1, 1, 56, X1 - 1, 9, 66)
    w.hint_zone(1, X0 + 2, 1, 56, X1 - 1, 9, 66)

    # B. The pool hall: a ledge behind the window (walk 6), the pool below it (water 1-4).
    wall(w, X0, 0, 38, X1, 0, 54, rng, face_chance=0.1)
    shell(w, X0, 1, 38, X1, 10, 55, rng)
    wall(w, X0 + 1, 1, 50, X1 - 1, 5, 54, rng, face_chance=0.25)   # the ledge
    w.fill(X0 + 1, 1, 41, X1 - 1, 4, 49, WATER)
    w.hint_zone(2, X0 + 1, 6, 50, X1 - 1, 9, 54)
    w.hint_zone(3, X0 + 1, 1, 41, X1 - 1, 9, 54)

    # The dive: a curtain wall (z 39) reaching down into the water to y 3; the pools meet under
    #    it (y 1-2), so Blupi has to plunge to pass. One block thick, for the camera's sake.
    wall(w, X0, 3, 39, X1, 26, 39, rng, face_chance=0.2)

    # C. The treasure shaft: pool C (z 31-37), a floor at walk 5, two helicopters, three treasures
    #    high up, and the cliff of D on its north side; walls and roof to y 26.
    wall(w, X0, 0, 12, X1, 0, 37, rng, face_chance=0.1)
    shell(w, X0, 1, 12, X1, 26, 38, rng)
    w.fill(X0 + 1, 1, 31, X1 - 1, 4, 37, WATER)
    wall(w, X0 + 1, 1, 18, X1 - 1, 4, 30, rng, face_chance=0.1)       # the floor
    for x in (44, 52):
        w.place(O.HELICOPTER, x, 5, 26)
    for x, y, z in ((42, 10, 24), (54, 14, 21), (48, 18, 28)):
        w.place(O.TREASURE, x, y, z)
    wall(w, X0 + 1, 1, 13, X1 - 1, 20, 17, rng, face_chance=0.3)      # D's cliff, top y 20
    # Under the curtain the pools meet: the halls' walls at z 38-40 stop at the water.
    w.fill(X0 + 1, 1, 38, X1 - 1, 4, 38, WATER)
    w.fill(X0 + 1, 1, 39, X1 - 1, 2, 39, WATER)
    w.fill(X0 + 1, 1, 40, X1 - 1, 4, 40, WATER)
    w.hint_zone(4, X0 + 1, 5, 18, X1 - 1, 7, 30)
    w.hint_zone(5, X0 + 1, 6, 18, X1 - 1, 25, 30)   # above the floor, past zone 4
    for x, y, z in ((44, 8, 17), (52, 12, 17), (48, 16, 17), (42, 3, 38), (54, 3, 38)):
        w.set(x, y, z, MONITOR)

    # D. The gallery on the cliff (walk 21); west the skate room, east the skate tunnel.
    w.hint_zone(6, X0 + 1, 21, 13, X1 - 1, 24, 17)
    wall(w, 33, 20, 11, 39, 20, 19, rng)                               # the skate room's floor
    shell(w, 33, 21, 11, 40, 25, 19, rng)
    w.fill(40, 21, 14, 40, 22, 16, 0)                                  # its door
    for z in (13, 14, 15):
        w.place(O.SKATEBOARD, 34, 21, z)
    w.hint_zone(7, 34, 21, 12, 39, 23, 18)

    # E. The skate tunnel (walk 21) eastward: two rows of mines to run over, two water trenches
    #    to jump, the red arrow at its end.
    wall(w, 57, 19, 12, 96, 20, 16, rng, face_chance=0.1)
    shell(w, 56, 21, 12, 97, 24, 16, rng)
    w.fill(56, 21, 13, 56, 23, 15, 0)                                  # the way in
    for xs in ((64, 65), (80, 81)):
        for x in xs:
            for z in (13, 14, 15):
                w.place(O.MINE, x, 21, z)
    for x0 in (70, 86):
        w.fill(x0, 20, 13, x0 + 1, 20, 15, WATER)
    w.place(O.EXIT, 95, 21, 14)
    w.hint_zone(8, 57, 21, 13, 68, 23, 15)
    w.hint_zone(9, 69, 21, 13, 72, 23, 15)
    w.hint_zone(9, 85, 21, 13, 88, 23, 15)
    w.hint_zone(10, 90, 21, 13, 96, 23, 15)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world013.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
