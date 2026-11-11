#!/usr/bin/env python3
"""Multiplayer world 206, the metal factory (worlds3d/world206.vwr): brief plans/briefs/world206.md.

Speedy Blupi 2's sixth network mission (DATA/WORLD206.BLP, region 14, music 9): the riveted metal
plates (78-84, 87-90), the four players starting in one row all facing the same way, no lift,
helicopters (12), tanks (12), ammunition (32) and thirteen bombs of each team. Here a metal hall:
the four starts along its south wall facing north, each team's bombs before its start, a stage in
the middle (walk 13) and a gallery along each side wall (walk 15) on girder masts, all reached by
stairs. Region 14 is not among Mobile Eggbert's backgrounds: 29 (machine parts) instead.

    python3 tools/worlds/world206.py                 # writes worlds3d/world206.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World  # noqa: E402
from station import girder_mast, metal  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 38, 62
STAGE = 12         # the stage's top: walk 13
GALLERY = 14       # the galleries' top: walk 15


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world206.vwr"
    rng = random.Random(206)
    w = World(mission=206, sky=29, music=9)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng, grille=0.04)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 15, plates)

    # ---- the stage in the middle (x 44-56, z 46-54), stairs on its north and south sides ----
    plates(44, WALK, 46, 56, STAGE, 54)
    w.stairs(50, WALK, 44, 0, 1, STAGE - TOP - 1, 78, width=3)      # from the north, rising south
    w.stairs(50, WALK, 56, 0, -1, STAGE - TOP - 1, 78, width=3)     # from the south, rising north

    # ---- a gallery along each side wall on girder masts, stairs up at its north end ----
    for gx0, gx1, mast_x, sx, dx in ((X0, X0 + 3, X0 + 3, X0 + 7, -1), (X1 - 3, X1, X1 - 3, X1 - 7, 1)):
        plates(gx0, GALLERY, Z0 + 2, gx1, GALLERY, Z1 - 4)
        for z in range(Z0 + 6, Z1 - 4, 6):
            girder_mast(w, mast_x, z, WALK, GALLERY - 1)
        w.stairs(sx, WALK, Z0 + 3, dx, 0, GALLERY - TOP - 1, 78, width=2)

    # ---- the four starts in a row along the south wall, facing north; each team's bombs before it ----
    places = ((38, WALK, Z1 - 1, 0), (46, WALK, Z1 - 1, 0), (54, WALK, Z1 - 1, 0), (62, WALK, Z1 - 1, 0))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, -1))

    # ---- the vehicles, the ammunition ----
    for x, y, z in ((X0 + 1, GALLERY + 1, 50), (X1 - 1, GALLERY + 1, 50), (47, STAGE + 1, 48), (53, STAGE + 1, 52),
                    (40, WALK, 44), (60, WALK, 44)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((40, 52), (60, 52), (50, 41)):
        w.place(O.TANK, x, WALK, z)
    for x, z in ((42, 46), (58, 46), (42, 56), (58, 56), (50, 59), (36, 46), (64, 46), (50, 43)):
        w.place(O.AMMO, x, WALK, z)
    for x, y, z in ((49, STAGE + 1, 50), (51, STAGE + 1, 50), (X0 + 1, GALLERY + 1, 44), (X1 - 1, GALLERY + 1, 56)):
        w.place(O.AMMO, x, y, z)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
