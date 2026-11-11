#!/usr/bin/env python3
"""Multiplayer world 204, the metal works (worlds3d/world204.vwr): brief plans/briefs/world204.md.

Speedy Blupi 2's fourth network mission (DATA/WORLD204.BLP, region 1, music 4): the riveted metal
plates (78-84) with the grey girders (201), the four players starting in two pairs on one floor,
five lifts, helicopters (22), shields (14), tanks (6), ammunition (28), drinks (14), a charge and
each team's bombs (11). Here a metal hall: the pairs of starts at its west and east ends, a
gallery of plates across its middle (walk 15) on girder masts, reached by four lifts, and a high
platform in the north (walk 19) on the fifth.

    python3 tools/worlds/world204.py                 # writes worlds3d/world204.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World, lift  # noqa: E402
from station import girder_mast, metal  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 34, 66, 36, 64
GALLERY = 14       # the gallery's top: walk 15
HIGH = 18          # the north platform's top: walk 19


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world204.vwr"
    rng = random.Random(204)
    w = World(mission=204, sky=1, music=4)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng, grille=0.04)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 15, plates)

    # ---- the gallery across the middle (x 44-56, z 46-54) on girder masts, four lifts ----
    plates(44, GALLERY, 46, 56, GALLERY, 54)
    for x, z in ((44, 46), (56, 46), (44, 54), (56, 54), (50, 46), (50, 54)):
        girder_mast(w, x, z, WALK, GALLERY - 1)
    for x, z in ((43, 50), (57, 50), (50, 45), (50, 55)):
        w.clear(x, TOP, z)
        lift(w, (x, TOP, z), (x, GALLERY, z), ticks=35, stop=20)

    # ---- the high platform in the north (x 46-54, z 36-39), its lift ----
    plates(46, WALK, Z0, 54, HIGH, Z0 + 3)
    w.clear(50, TOP, Z0 + 4)
    lift(w, (50, TOP, Z0 + 4), (50, HIGH, Z0 + 4), ticks=60, stop=20)

    # ---- the starts in two pairs: yellow and orange west, blue and green east ----
    places = ((37, WALK, 46, 3), (37, WALK, 54, 3), (63, WALK, 46, 1), (63, WALK, 54, 1))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, -1 if z < 50 else 1))

    # ---- the vehicles, shields, ammunition, drinks, the charge ----
    for x, y, z in ((47, GALLERY + 1, 48), (53, GALLERY + 1, 52), (48, HIGH + 1, 37), (52, HIGH + 1, 37),
                    (40, WALK, 38), (60, WALK, 62)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((40, 62), (60, 38), (50, 60)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((45, GALLERY + 1, 52), (55, GALLERY + 1, 48), (40, WALK, 50), (60, WALK, 50),
                    (50, HIGH + 1, 38)):
        w.place(O.SHIELD, x, y, z)
    for x, z in ((42, 42), (58, 42), (42, 58), (58, 58), (46, 62), (54, 62), (38, 50), (62, 50)):
        w.place(O.AMMO, x, WALK, z)
    for x, z in ((49, 50), (51, 50)):
        w.place(O.AMMO, x, GALLERY + 1, z)
    for x, y, z in ((46, WALK, 40), (54, WALK, 60), (47, GALLERY + 1, 53), (53, GALLERY + 1, 47)):
        w.place(O.DRINK, x, y, z)
    w.place(O.CHARGE, 50, GALLERY + 1, 50)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
