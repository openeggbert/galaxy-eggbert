#!/usr/bin/env python3
"""Multiplayer world 203, the toy box (worlds3d/world203.vwr): brief plans/briefs/world203.md.

Speedy Blupi 2's third network mission (DATA/WORLD203.BLP, region 22, music 8): the toy tiles
(balls 215-220, striped poles 221-223, stars 230/231, trees 233/234), the four players starting
side by side, six lifts, helicopters (15), shields (9), tanks (22), ammunition (42) and one
charge (31); no personal bomb. Here a hall of toy balls: the four starts in a row along its west
wall, three toy platforms (the centre one low, two in the east corners higher) each reached by
two lifts, striped posts and candy poles about.

    python3 tools/worlds/world203.py                 # writes worlds3d/world203.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import closed_hall, starts  # noqa: E402
from kit import O, World, lift  # noqa: E402
from toys import BALLS, MIXED, candy, post, toy  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 34, 66, 36, 64
CENTRE = 13        # the centre platform's top: walk 14
CORNER = 16        # the corner platforms' top: walk 17


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world203.vwr"
    rng = random.Random(203)
    w = World(mission=203, sky=22, music=8)

    def balls(x0, y0, z0, x1, y1, z1):
        toy(w, x0, y0, z0, x1, y1, z1, rng, palette=BALLS)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 16, balls)

    # ---- the platforms and their lifts ----
    toy(w, 46, WALK, 46, 53, CENTRE, 53, rng, palette=MIXED)                  # the centre
    for x, z in ((45, 49), (54, 50)):
        w.clear(x, TOP, z)
        lift(w, (x, TOP, z), (x, CENTRE, z), ticks=30, stop=20)
    for za, zb, lz in ((Z0, Z0 + 6, Z0 + 7), (Z1 - 6, Z1, Z1 - 7)):          # the east corners
        toy(w, X1 - 6, WALK, za, X1, CORNER, zb, rng, palette=MIXED)
        for lx in (X1 - 5, X1 - 2):
            w.clear(lx, TOP, lz)
            lift(w, (lx, TOP, lz), (lx, CORNER, lz), ticks=45, stop=20)

    # ---- posts and candy poles along the walls (decor and cover) ----
    for x, z in ((40, 40), (40, 60), (58, 50), (47, 40), (47, 60)):
        post(w, x, WALK, WALK + 2, z, rng)
    for x, z in ((X0, 42), (X0, 58), (50, Z0), (50, Z1)):
        candy(w, x, WALK, WALK + 3, z, rng=rng)

    # ---- the four starts side by side along the west wall, facing east ----
    places = ((36, WALK, 44, 3), (36, WALK, 48, 3), (36, WALK, 52, 3), (36, WALK, 56, 3))
    starts(w, places)

    # ---- the vehicles, the shields, the ammunition, the charge ----
    for x, y, z in ((48, CENTRE + 1, 48), (51, CENTRE + 1, 51), (X1 - 3, CORNER + 1, Z0 + 2),
                    (X1 - 3, CORNER + 1, Z1 - 2), (42, WALK, 38), (42, WALK, 62)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((44, 44), (44, 56), (56, 42), (56, 58), (60, 50)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((40, WALK, 50), (52, WALK, 44), (52, WALK, 56), (X1 - 1, CORNER + 1, Z0 + 4),
                    (X1 - 1, CORNER + 1, Z1 - 4)):
        w.place(O.SHIELD, x, y, z)
    ammo = [(38, 40), (38, 60), (44, 50), (48, 42), (48, 58), (56, 46), (56, 54), (62, 46), (62, 54)]
    for x, z in ammo:
        w.place(O.AMMO, x, WALK, z)
    for x, z in ((47, 51), (52, 47)):
        w.place(O.AMMO, x, CENTRE + 1, z)
    w.place(O.CHARGE, 50, CENTRE + 1, 50)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
