#!/usr/bin/env python3
"""Multiplayer world 208, the library (worlds3d/world208.vwr): brief plans/briefs/world208.md.

Speedy Blupi 2's eighth network mission (DATA/WORLD208.BLP, region 3, music 2): the library's
bookshelves and cupboards (139-143), the four players starting at one place on the map's west
edge, one lift, helicopters (42), shields (10), tanks (14) and ammunition (37); no personal bomb.
Here a library hall walled with shelves on a brick floor: the four starts side by side along its
west wall facing east, rows of shelves (two high) across its middle with aisles between them, and
a brick gallery along its north wall (walk 15) on the one lift and a stair at its east end.

    python3 tools/worlds/world208.py                 # writes worlds3d/world208.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts  # noqa: E402
from kit import O, World, lift  # noqa: E402
from palace import BRICK, SHELVES  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 38, 62
GALLERY = 14       # the gallery's top: walk 15


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world208.vwr"
    rng = random.Random(208)
    w = World(mission=208, sky=3, music=2)

    def shelves(x0, y0, z0, x1, y1, z1):
        for x in range(min(x0, x1), max(x0, x1) + 1):
            for y in range(min(y0, y1), max(y0, y1) + 1):
                for z in range(min(z0, z1), max(z0, z1) + 1):
                    w.set(x, y, z, rng.choice(SHELVES))

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 15, shelves)
    w.fill(X0, TOP, Z0, X1, TOP, Z1, BRICK)                          # the floor

    # ---- the gallery along the north wall (z 38-42), the lift at its middle, a stair at its east end ----
    w.fill(X0, GALLERY, Z0, X1, GALLERY, Z0 + 4, BRICK)
    for x in range(X0 + 4, X1 - 3, 6):
        w.fill(x, WALK, Z0 + 4, x, GALLERY - 1, Z0 + 4, BRICK)       # its pillars
    w.clear(50, TOP, Z0 + 5)
    lift(w, (50, TOP, Z0 + 5), (50, GALLERY, Z0 + 5), ticks=50, stop=20)
    w.stairs(X1 - 1, WALK, Z0 + 8, 0, -1, GALLERY - TOP - 1, BRICK, width=2)

    # ---- rows of shelves, two high, across the middle (east-west), aisles between and around ----
    for z in (47, 52, 57):
        for xa, xb in ((40, 46), (49, 55), (58, 63)):
            shelves(xa, WALK, z, xb, WALK + 1, z)

    # ---- the four starts side by side along the west wall, facing east ----
    places = ((X0, WALK, 47, 3), (X0, WALK, 50, 3), (X0, WALK, 53, 3), (X0, WALK, 56, 3))
    starts(w, places)

    # ---- the vehicles, the shields, the ammunition ----
    for x, y, z in ((40, GALLERY + 1, Z0 + 2), (60, GALLERY + 1, Z0 + 2), (37, WALK, 60), (64, WALK, 60),
                    (52, WALK, 44), (44, WALK, 60)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((38, 44), (66, 50), (56, 60), (47, 49)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((50, GALLERY + 1, Z0 + 1), (43, WALK, 49), (60, WALK, 54), (X1, WALK, 47)):
        w.place(O.SHIELD, x, y, z)
    for x, z in ((38, 49), (38, 55), (44, 54), (53, 54), (56, 49), (62, 44), (50, 60), (66, 58), (48, 44), (60, 60)):
        w.place(O.AMMO, x, WALK, z)
    for x in (34, 46, 56, 66):
        w.place(O.AMMO, x, GALLERY + 1, Z0 + 1)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
