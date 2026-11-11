#!/usr/bin/env python3
"""Multiplayer world 210, the village street (worlds3d/world210.vwr): brief plans/briefs/world210.md.

Speedy Blupi 2's tenth network mission (DATA/WORLD210.BLP, region 17, music 8): a wide map of the
village's stone (156, 35-38, 144-145, 152-155) and its houses (185, 186), the four players starting
far apart along it, two lifts, helicopters (40), shields (8), tanks (20), ammunition (29), drinks
(15), three charges and each team's bombs (14). Here a long walled street of stone: a row of
houses along its north side (their slate roofs walk 14) with alleys between them, two lifts up to
the roofs, a well in the middle; the four starts spread along the street, each team's bombs south
of its start. Region 17 is not among Mobile Eggbert's backgrounds: 6 (dark rock) instead.

    python3 tools/worlds/world210.py                 # writes worlds3d/world210.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World, lift  # noqa: E402
from village import house, stone  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 22, 78, 42, 58
HOUSE = 4          # the houses' height: their roofs' top at walk - 1 + 4 = 13, walk 14
ROOF = WALK + HOUSE - 1
HOUSES = ((24, 31), (35, 43), (47, 54), (58, 65), (69, 76))


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world210.vwr"
    rng = random.Random(210)
    w = World(mission=210, sky=6, music=8)

    def walls(x0, y0, z0, x1, y1, z1):
        stone(w, x0, y0, z0, x1, y1, z1, rng)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 14, walls)

    # ---- the houses along the north side, their facades (+Z) on the street ----
    for xa, xb in HOUSES:
        house(w, xa, Z0 + 1, xb, Z0 + 5, WALK, HOUSE, rng, door=(xa + xb) // 2)
    # ---- two lifts up to the roofs, before the second and the fourth house ----
    for lx in (39, 61):
        w.clear(lx, TOP, Z0 + 6)
        lift(w, (lx, TOP, Z0 + 6), (lx, ROOF, Z0 + 6), ticks=40, stop=20)
    # ---- the well in the middle of the street ----
    walls(49, WALK, 52, 51, WALK, 54)

    # ---- the four starts along the street: yellow west facing west, the others facing east ----
    places = ((26, WALK, 54, 1), (56, WALK, 54, 3), (70, WALK, 54, 3), (36, WALK, 54, 3))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, 1))

    # ---- the vehicles, shields, ammunition, drinks, the charges ----
    for x, y, z in ((28, ROOF + 1, Z0 + 3), (72, ROOF + 1, Z0 + 3), (31, WALK, 50), (45, WALK, 55),
                    (65, WALK, 50), (75, WALK, 52)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((40, 52), (60, 55), (50, 49), (24, 50)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((50, ROOF + 1, Z0 + 3), (33, WALK, 56), (67, WALK, 56)):
        w.place(O.SHIELD, x, y, z)
    for x, z in ((29, 53), (42, 49), (46, 52), (54, 52), (58, 49), (64, 53), (73, 49), (77, 56), (23, 57), (50, 57)):
        w.place(O.AMMO, x, WALK, z)
    for x, y, z in ((39, ROOF + 1, Z0 + 2), (61, ROOF + 1, Z0 + 2), (52, WALK, 50), (34, WALK, 49)):
        w.place(O.DRINK, x, y, z)
    w.place(O.CHARGE, 63, ROOF + 1, Z0 + 3)
    w.place(O.CHARGE, 26, ROOF + 1, Z0 + 2)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
