#!/usr/bin/env python3
"""Multiplayer world 207, the stone islands (worlds3d/world207.vwr): brief plans/briefs/world207.md.

Speedy Blupi 2's seventh network mission (DATA/WORLD207.BLP, region 16, music 1): a sparse map of
the village's stone (38, 144-146, 76) in the clouds, the four players starting in two pairs back
to back, no lift, helicopters (28), tanks (23), ammunition (46), drinks (20), two charges and each
team's bombs (5, 5, 5, 4). Here a stone hall with a grass floor: four stone islands in its corners
(walk 13) and a tower in the middle (walk 16), each with stairs; the two pairs back to back in the
south, each team's bombs beside its start.

    python3 tools/worlds/world207.py                 # writes worlds3d/world207.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World  # noqa: E402
from village import STONE, grass_top, stone  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 34, 66, 36, 64
ISLAND = 12        # the islands' top: walk 13
TOWER = 15         # the tower's top: walk 16


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world207.vwr"
    rng = random.Random(207)
    w = World(mission=207, sky=16, music=1)

    def walls(x0, y0, z0, x1, y1, z1):
        stone(w, x0, y0, z0, x1, y1, z1, rng)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 16, walls)
    grass_top(w, X0, Z0, X1, Z1, TOP)

    # ---- four islands in the corners (6 x 6), stairs from the hall's middle ----
    islands = ((X0 + 1, Z0 + 1, 1, 1), (X1 - 6, Z0 + 1, -1, 1), (X0 + 1, Z1 - 6, 1, -1), (X1 - 6, Z1 - 6, -1, -1))
    for x0, z0, sx, sz in islands:
        walls(x0, WALK, z0, x0 + 5, ISLAND, z0 + 5)
        grass_top(w, x0, z0, x0 + 5, z0 + 5, ISLAND)
        # Stairs off the island's inner side (towards the middle in x), rising towards it.
        edge_x = x0 + 6 if sx > 0 else x0 - 1
        w.stairs(edge_x + sx * (ISLAND - TOP - 2), WALK, z0 + 2, -sx, 0, ISLAND - TOP - 1, STONE, width=2)

    # ---- the tower in the middle (x 48-52, z 46-50), stairs on its south side ----
    walls(48, WALK, 46, 52, TOWER, 50)
    grass_top(w, 48, 46, 52, 50, TOWER)
    w.stairs(50, WALK, 55, 0, -1, TOWER - TOP - 1, STONE, width=2)

    # ---- the two pairs back to back south of the tower: yellow west, orange east; blue west, green east ----
    places = ((43, WALK, 54, 1), (44, WALK, 54, 3), (56, WALK, 54, 1), (57, WALK, 54, 3))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (-1, 0) if x in (43, 56) else (1, 0))

    # ---- the vehicles, ammunition, drinks, the two charges ----
    for x, y, z in ((X0 + 3, ISLAND + 1, Z0 + 3), (X1 - 3, ISLAND + 1, Z0 + 3), (X0 + 3, ISLAND + 1, Z1 - 3),
                    (X1 - 3, ISLAND + 1, Z1 - 3), (40, WALK, 50), (60, WALK, 50)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((44, 44), (56, 44), (50, 40), (38, 56), (62, 56)):
        w.place(O.TANK, x, WALK, z)
    for x, z in ((46, 52), (54, 52), (42, 48), (58, 48), (50, 62), (47, 38), (53, 38), (38, 46), (62, 46), (50, 44)):
        w.place(O.AMMO, x, WALK, z)
    for x, y, z in ((X0 + 5, ISLAND + 1, Z0 + 5), (X1 - 5, ISLAND + 1, Z1 - 5), (45, WALK, 56), (55, WALK, 56),
                    (50, WALK, 42)):
        w.place(O.DRINK, x, y, z)
    w.place(O.CHARGE, 50, TOWER + 1, 48)
    w.place(O.CHARGE, X1 - 5, ISLAND + 1, Z0 + 5)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
