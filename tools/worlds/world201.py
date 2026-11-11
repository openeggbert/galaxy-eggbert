#!/usr/bin/env python3
"""Multiplayer world 201, the machine arena (worlds3d/world201.vwr): brief plans/briefs/world201.md.

Speedy Blupi 2's first network mission (DATA/WORLD201.BLP, region 0 and music 2): a small closed
arena of the blue machine panel (icon 10), four players starting on one floor, three lifts, and a
hall full of vehicles to fight with -- helicopters (13) and tanks (28) with their ammunition (29)
-- drinks (30), and each team's personal bombs (200-203) to take and lay. Here the arena is a
closed machine hall in 3D: the four starts in its corners, a raised centre reached by stairs and a
lift, two balconies on lifts along the side walls, the vehicles spread over the three levels.
No crate, goal or enemy Blupi: a network game would take them out anyway (decblock.cpp:1430).

    python3 tools/worlds/world201.py                 # writes worlds3d/world201.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check  # noqa: E402
from kit import MACHINE_FLOOR, O, World, lift, slab, wall  # noqa: E402

TOP = 9            # the hall's floor top: Blupi walks at y 10
WALK = TOP + 1
X0, X1 = 36, 64    # the hall inside its walls
Z0, Z1 = 36, 64
ROOF = TOP + 13    # the roof: twelve cells of air over the floor, room for the helicopters
UPPER = 13         # the centre's and the balconies' top: walk 14
BOMB_TEAMS = (200, 201, 202, 203)


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world201.vwr"
    rng = random.Random(201)
    w = World(mission=201, sky=0, music=2)

    # ---- the hall: floor, walls and roof of machine panel ----
    slab(w, X0 - 1, Z0 - 1, X1 + 1, Z1 + 1, TOP, thickness=2)
    slab(w, X0 - 1, Z0 - 1, X1 + 1, Z1 + 1, ROOF)
    for x0, z0, x1, z1 in ((X0 - 1, Z0 - 1, X1 + 1, Z0 - 1), (X0 - 1, Z1 + 1, X1 + 1, Z1 + 1),
                           (X0 - 1, Z0 - 1, X0 - 1, Z1 + 1), (X1 + 1, Z0 - 1, X1 + 1, Z1 + 1)):
        wall(w, x0, WALK, z0, x1, ROOF - 1, z1, rng)

    # ---- the raised centre (x, z 46-54): stairs north and south, a lift on its east ----
    wall(w, 46, WALK, 46, 54, UPPER, 54, rng, face_chance=0.1)
    w.stairs(52, WALK, 43, 0, 1, UPPER - WALK, MACHINE_FLOOR, width=5)   # z 43-45, rising south
    w.stairs(48, WALK, 57, 0, -1, UPPER - WALK, MACHINE_FLOOR, width=5)  # z 57-55, rising north
    w.clear(55, TOP, 50)
    lift(w, (55, TOP, 50), (55, UPPER, 50), ticks=30, stop=20)

    # ---- the balconies along the west and east walls (z 42-58), each with its lift ----
    for xa, xb, lx in ((X0, X0 + 3, X0 + 4), (X1 - 3, X1, X1 - 4)):
        wall(w, xa, WALK, 42, xb, UPPER, 58, rng, face_chance=0.1)
        w.clear(lx, TOP, 50)
        lift(w, (lx, TOP, 50), (lx, UPPER, 50), ticks=30, stop=20)

    # ---- the four starts in the corners, facing along the hall (yellow, orange, blue, green) ----
    starts = ((39, 38, 2), (61, 38, 2), (39, 62, 0), (61, 62, 0))
    for color, (x, z, turns) in enumerate(starts):
        w.start(color, x, WALK, z, turns)

    # ---- each team's personal bombs: a row of four beside its start ----
    for team, (x, z, _) in zip(BOMB_TEAMS, starts):
        dx = 1 if x < 50 else -1
        for k in range(4):
            w.place(team, x + dx * (2 + k), WALK, z)

    # ---- the vehicles, their ammunition, the drinks ----
    for x, z in ((48, 48), (52, 52)):                           # helicopters on the centre
        w.place(O.HELICOPTER, x, UPPER + 1, z)
    for x in (X0 + 1, X1 - 1):                                  # and on the balconies
        w.place(O.HELICOPTER, x, UPPER + 1, 44)
        w.place(O.HELICOPTER, x, UPPER + 1, 56)
    for x, z in ((50, 40), (50, 60), (43, 50), (57, 46)):       # tanks on the floor
        w.place(O.TANK, x, WALK, z)
    for x, z in ((44, 44), (56, 44), (44, 56), (56, 56), (50, 38), (50, 62), (45, 40), (55, 60)):
        w.place(O.AMMO, x, WALK, z)
    for x, z in ((50, 50), (47, 53)):
        w.place(O.AMMO, x, UPPER + 1, z)
    for x in (X0 + 2, X1 - 2):
        w.place(O.AMMO, x, UPPER + 1, 50)
    for x, y, z in ((45, WALK, 60), (55, WALK, 40), (53, UPPER + 1, 47)):
        w.place(O.DRINK, x, y, z)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
