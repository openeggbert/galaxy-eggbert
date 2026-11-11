#!/usr/bin/env python3
"""Multiplayer world 216, the station's brick hall (worlds3d/world216.vwr): brief plans/briefs/world216.md.

Galaxy's own multiplayer world (Q-022), built of Mobile Eggbert's world 5, the space station
(station.py: its brick halls with water channels and lattice girders), region 1, music 2: a brick
hall with a dais in its middle (walk 13) climbed by a stair cut into its north and its south side,
ringed by a moat of water crossed by four bridges; a balcony along the north and the south wall
(walk 14) with a lift up each; four girder masts. The four starts in the corners, each team's
bombs beside its start.

    python3 tools/worlds/world216.py                 # writes worlds3d/world216.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World, lift, rect  # noqa: E402
from station import BRICK, WATER, brick, girder_mast  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 36, 64
ROOF = TOP + 15
DAIS = 12          # the dais's top: walk 13
BALCONY = 13       # the balconies' top: walk 14


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world216.vwr"
    rng = random.Random(216)
    w = World(mission=216, sky=1, music=2)

    def walls(x0, y0, z0, x1, y1, z1):
        brick(w, x0, y0, z0, x1, y1, z1, rng)

    closed_hall(w, X0, Z0, X1, Z1, TOP, ROOF, walls)

    # ---- the dais (x 46-54, z 46-54), a stair cut into its north and its south side ----
    walls(46, WALK, 46, 54, DAIS, 54)
    for x in range(49, 52):
        for y in range(WALK, DAIS + 1):
            for z in (46, 47, 53, 54):
                w.clear(x, y, z)
    w.stairs(51, WALK, 46, 0, 1, DAIS - TOP - 1, BRICK, width=3)    # z 46-47, rising south
    w.stairs(49, WALK, 54, 0, -1, DAIS - TOP - 1, BRICK, width=3)   # z 54-53, rising north
    # ---- the moat round it (two cells wide, its water a cell under the floor), four bridges ----
    bridges = {(x, z) for x in range(49, 52) for z in (44, 45, 55, 56)} | \
              {(x, z) for z in range(49, 52) for x in (44, 45, 55, 56)}
    for x, z in set(rect(44, 44, 56, 56)) - set(rect(46, 46, 54, 54)) - bridges:
        w.clear(x, TOP, z)
        w.set(x, TOP - 1, z, WATER)
        w.set(x, TOP - 2, z, BRICK)

    # ---- a balcony along the north and the south wall (x 42-58), a lift up each ----
    for z0, z1, lz in ((Z0, Z0 + 2, Z0 + 3), (Z1 - 2, Z1, Z1 - 3)):
        walls(42, WALK, z0, 58, BALCONY, z1)
        w.clear(50, TOP, lz)
        lift(w, (50, TOP, lz), (50, BALCONY, lz), ticks=30, stop=20)
    # ---- four girder masts ----
    for x, z in ((40, 42), (60, 42), (40, 58), (60, 58)):
        girder_mast(w, x, z, WALK, ROOF - 1)

    # ---- the four starts in the corners, facing along the hall; each team's bombs beside it ----
    places = ((35, WALK, 38, 2), (65, WALK, 38, 2), (35, WALK, 62, 0), (65, WALK, 62, 0))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, 1) if z < 50 else (0, -1))

    # ---- the vehicles, ammunition, shields, drinks, the charge ----
    for x, y, z in ((47, DAIS + 1, 50), (53, DAIS + 1, 50), (44, BALCONY + 1, Z0 + 1), (56, BALCONY + 1, Z1 - 1)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((38, 50), (62, 50), (42, 45), (58, 55)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((50, DAIS + 1, 50), (48, DAIS + 1, 48), (52, DAIS + 1, 52), (54, BALCONY + 1, Z0 + 1),
                    (46, BALCONY + 1, Z1 - 1), (36, WALK, 46), (64, WALK, 54), (42, WALK, 55), (58, WALK, 45)):
        w.place(O.AMMO, x, y, z)
    for x, y, z in ((52, DAIS + 1, 48), (34, WALK, 54)):
        w.place(O.SHIELD, x, y, z)
    for x, y, z in ((58, BALCONY + 1, Z0 + 1), (66, WALK, 46)):
        w.place(O.DRINK, x, y, z)
    w.place(O.CHARGE, 48, DAIS + 1, 52)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
