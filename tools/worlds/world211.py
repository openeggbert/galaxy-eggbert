#!/usr/bin/env python3
"""Multiplayer world 211, the palace hall (worlds3d/world211.vwr): brief plans/briefs/world211.md.

Speedy Blupi 2's eleventh network mission (DATA/WORLD211.BLP, region 23, music 6): the palace's
white stone (396, 397, 392-395) with its trim, the four players starting in one row far apart,
no lift, helicopters (33), shields (16), tanks (20), ammunition (40), five charges, dynamite (15)
and each team's bombs (17). Here a white palace hall on grass: two rows of columns, a dais along
its north wall (walk 13) up a wide stair, a balcony along each side wall (walk 16) up a stair;
the four starts along its south wall facing north, each team's bombs before its start. Region 23
is not among Mobile Eggbert's backgrounds: 25 (the palace's mosaic) instead.

    python3 tools/worlds/world211.py                 # writes worlds3d/world211.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World  # noqa: E402
from palace import STONE, stone, windows_row  # noqa: E402
from village import grass_top  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 36, 62
ROOF = TOP + 16
DAIS = 12          # the dais's top: walk 13
BALCONY = 15       # the balconies' top: walk 16


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world211.vwr"
    rng = random.Random(211)
    w = World(mission=211, sky=25, music=6)

    def walls(x0, y0, z0, x1, y1, z1):
        stone(w, x0, y0, z0, x1, y1, z1, rng)

    closed_hall(w, X0, Z0, X1, Z1, TOP, ROOF, walls)
    grass_top(w, X0, Z0, X1, Z1, TOP)
    windows_row(w, X0, X1, ROOF - 4, Z0 - 1, rng)                     # windows high on the north wall

    # ---- the dais along the north wall (x 42-58, z 36-40), a wide stair from the south ----
    walls(42, WALK, Z0, 58, DAIS, Z0 + 4)
    w.stairs(48, WALK, Z0 + 6, 0, -1, DAIS - TOP - 1, STONE, width=5)
    # ---- a balcony along each side wall (z 44-56), a stair up at its south end ----
    for xa, xb, sx, dx in ((X0, X0 + 2, X0 + 7, -1), (X1 - 2, X1, X1 - 7, 1)):
        walls(xa, BALCONY, 44, xb, BALCONY, 56)
        w.stairs(sx, WALK, 55, dx, 0, BALCONY - TOP - 1, STONE, width=2)
    # ---- two rows of columns ----
    for x in (40, 60):
        for z in (44, 50, 56):
            walls(x, WALK, z, x, ROOF - 1, z)

    # ---- the four starts along the south wall, facing north; each team's bombs before it ----
    places = ((38, WALK, Z1 - 1, 0), (46, WALK, Z1 - 1, 0), (54, WALK, Z1 - 1, 0), (62, WALK, Z1 - 1, 0))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, -1))

    # ---- the vehicles, shields, ammunition, charges, dynamite ----
    for x, y, z in ((44, DAIS + 1, Z0 + 2), (56, DAIS + 1, Z0 + 2), (X0 + 1, BALCONY + 1, 48), (X1 - 1, BALCONY + 1, 48),
                    (50, WALK, 47), (36, WALK, 40)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((44, 52), (56, 52), (64, 40), (50, 55)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((50, DAIS + 1, Z0 + 1), (X0 + 1, BALCONY + 1, 45), (X1 - 1, BALCONY + 1, 45), (42, WALK, 58)):
        w.place(O.SHIELD, x, y, z)
    for x, z in ((36, 50), (64, 50), (44, 44), (56, 44), (50, 51), (42, 47), (58, 47), (50, 59), (36, 56), (64, 58)):
        w.place(O.AMMO, x, WALK, z)
    w.place(O.CHARGE, 50, DAIS + 1, Z0 + 3)
    w.place(O.CHARGE, X1 - 1, BALCONY + 1, 52)
    for x, y, z in ((46, DAIS + 1, Z0 + 1), (54, DAIS + 1, Z0 + 1), (X0 + 1, BALCONY + 1, 52), (38, WALK, 44)):
        w.place(O.DYNAMITE, x, y, z)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
