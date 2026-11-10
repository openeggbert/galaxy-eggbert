#!/usr/bin/env python3
"""Multiplayer world 205, the stone courtyard (worlds3d/world205.vwr): brief plans/briefs/world205.md.

Speedy Blupi 2's fifth network mission (DATA/WORLD205.BLP, region 2, music 4): the village's stone
(35-38, 144-146, 156), the four players starting side by side on one row, six lifts, helicopters
(20), shields (11), tanks (20), ammunition (31) and two charges (31); no personal bomb. Here a
closed courtyard of stone with a grass floor: the four starts in a row in its south, four stone
bastions in its corners (walk 15) with grass on top, each reached by a lift, and a higher tower in
the north (walk 19) on two lifts.

    python3 tools/worlds/world205.py                 # writes worlds3d/world205.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import closed_hall, starts  # noqa: E402
from kit import O, World, lift  # noqa: E402
from village import grass_top, stone  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 34, 66, 36, 64
BASTION = 14       # the bastions' top: walk 15
TOWER = 18         # the tower's top: walk 19


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world205.vwr"
    rng = random.Random(205)
    w = World(mission=205, sky=2, music=4)

    def walls(x0, y0, z0, x1, y1, z1):
        stone(w, x0, y0, z0, x1, y1, z1, rng)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 16, walls)
    grass_top(w, X0, Z0, X1, Z1, TOP)                                   # the courtyard's grass

    # ---- four bastions in the corners, a lift beside each ----
    corners = ((X0, Z0, X0 + 5, Z0 + 5, X0 + 6, Z0 + 2), (X1 - 5, Z0, X1, Z0 + 5, X1 - 6, Z0 + 2),
               (X0, Z1 - 5, X0 + 5, Z1, X0 + 6, Z1 - 2), (X1 - 5, Z1 - 5, X1, Z1, X1 - 6, Z1 - 2))
    for x0, z0, x1, z1, lx, lz in corners:
        stone(w, x0, WALK, z0, x1, BASTION, z1, rng)
        grass_top(w, x0, z0, x1, z1, BASTION)
        w.clear(lx, TOP, lz)
        lift(w, (lx, TOP, lz), (lx, BASTION, lz), ticks=35, stop=20)

    # ---- the tower in the north (x 46-54, z 36-41), two lifts ----
    stone(w, 46, WALK, Z0, 54, TOWER, Z0 + 5, rng)
    grass_top(w, 46, Z0, 54, Z0 + 5, TOWER)
    for lx in (45, 55):
        w.clear(lx, TOP, Z0 + 3)
        lift(w, (lx, TOP, Z0 + 3), (lx, TOWER, Z0 + 3), ticks=60, stop=20)

    # ---- the four starts side by side in the south, facing north ----
    places = ((47, WALK, 60, 0), (49, WALK, 60, 0), (51, WALK, 60, 0), (53, WALK, 60, 0))
    starts(w, places)

    # ---- the vehicles, shields, ammunition, charges ----
    for x, y, z in ((X0 + 2, BASTION + 1, Z0 + 2), (X1 - 2, BASTION + 1, Z0 + 2), (X0 + 2, BASTION + 1, Z1 - 2),
                    (X1 - 2, BASTION + 1, Z1 - 2), (48, TOWER + 1, Z0 + 2), (52, TOWER + 1, Z0 + 2),
                    (42, WALK, 50), (58, WALK, 50)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((44, 46), (56, 46), (44, 54), (56, 54), (50, 50), (50, 44)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((X0 + 4, BASTION + 1, Z0 + 4), (X1 - 4, BASTION + 1, Z1 - 4), (50, TOWER + 1, Z0 + 4),
                    (38, WALK, 50), (62, WALK, 50)):
        w.place(O.SHIELD, x, y, z)
    for x, z in ((40, 44), (60, 44), (40, 56), (60, 56), (47, 50), (53, 50), (50, 56), (45, 60), (55, 60)):
        w.place(O.AMMO, x, WALK, z)
    for x, y, z in ((X1 - 4, BASTION + 1, Z0 + 4), (X0 + 4, BASTION + 1, Z1 - 4)):
        w.place(O.AMMO, x, y, z)
    w.place(O.CHARGE, 50, TOWER + 1, Z0 + 1)
    w.place(O.CHARGE, 50, WALK, 47)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
