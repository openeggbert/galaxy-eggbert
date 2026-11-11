#!/usr/bin/env python3
"""Multiplayer world 212, the clod cave (worlds3d/world212.vwr): brief plans/briefs/world212.md.

Speedy Blupi 2's twelfth network mission (DATA/WORLD212.BLP, region 27, music 7): the orange-brown
clods (284, 301) with their edge lumps (285-290, 302, 303), trestles (199) and girders (201) in a
dark red cave, the four players starting in one row, no lift, helicopters (48), shields (20),
tanks (26), ammunition (24), a charge and each team's bombs (18). Here a cave cut into clods,
lumpy along its walls: a mound in its middle (walk 13) up two stairs, two clod columns, and two
islands floating high in its corners (walk 18) that only a helicopter reaches; the four starts in
a row along its south side facing north, each team's bombs before its start; trestles about.

    python3 tools/worlds/world212.py                 # writes worlds3d/world212.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, starts, team_bombs  # noqa: E402
from clods import CLODS, TRESTLE, carve, clay, column, fringe, rough  # noqa: E402
from kit import O, World  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 38, 62
ROOF = TOP + 15
MOUND = 12         # the mound's top: walk 13
ISLAND = 17        # the islands' top: walk 18


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world212.vwr"
    rng = random.Random(212)
    w = World(mission=212, sky=27, music=7)

    clay(w, X0 - 1, TOP - 1, Z0 - 1, X1 + 1, ROOF, Z1 + 1, rng)
    carve(w, X0, WALK, Z0, X1, ROOF - 1, Z1)
    rough(w, X0, Z0, X1, Z1, WALK, ROOF, rng, sag=0.0)                # lumpy walls, the ceiling kept high

    # ---- the mound in the middle (x 45-55, z 46-54), a stair on its north and south sides ----
    clay(w, 45, WALK, 46, 55, MOUND, 54, rng)
    w.stairs(50, WALK, 44, 0, 1, MOUND - TOP - 1, CLODS[0], width=3)
    w.stairs(50, WALK, 56, 0, -1, MOUND - TOP - 1, CLODS[0], width=3)
    # ---- two clod columns, two floating islands in the corners ----
    column(w, 40, 52, WALK, ROOF - 1, rng)
    column(w, 60, 46, WALK, ROOF - 1, rng)
    for x0, z0 in ((35, 41), (60, 54)):
        clay(w, x0, ISLAND - 2, z0, x0 + 5, ISLAND, z0 + 5, rng)
    fringe(w, rng, box=(X0, Z0, X1, Z1))
    for x, z in ((36, 58), (64, 42), (44, 40), (56, 60)):
        w.set(x, WALK, z, TRESTLE)

    # ---- the four starts in a row along the south side, facing north; each team's bombs before it ----
    places = ((38, WALK, Z1 - 1, 0), (46, WALK, Z1 - 1, 0), (54, WALK, Z1 - 1, 0), (62, WALK, Z1 - 1, 0))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, -1))

    # ---- the vehicles, shields, ammunition, the charge ----
    for x, y, z in ((48, MOUND + 1, 48), (52, MOUND + 1, 52), (37, WALK, 44), (63, WALK, 50), (42, WALK, 58),
                    (58, WALK, 58), (66, WALK, 40)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((36, 50), (64, 56), (50, 41), (44, 55)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((37, ISLAND + 1, 43), (62, ISLAND + 1, 56), (50, MOUND + 1, 50), (34, WALK, 60)):
        w.place(O.SHIELD, x, y, z)
    for x, y, z in ((39, ISLAND + 1, 45), (64, ISLAND + 1, 58), (46, MOUND + 1, 50), (54, MOUND + 1, 50),
                    (42, WALK, 46), (58, WALK, 54), (50, WALK, 58), (34, WALK, 48)):
        w.place(O.AMMO, x, y, z)
    w.place(O.CHARGE, 61, ISLAND + 1, 55)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
