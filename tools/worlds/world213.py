#!/usr/bin/env python3
"""Multiplayer world 213, the cheese cave (worlds3d/world213.vwr): brief plans/briefs/world213.md.

Galaxy's own multiplayer world (the owner's Q-022: five more of Galaxy's design, 213-217), built of
Mobile Eggbert's world 4, the cheese (cheese.py): a hall carved out of the four cheese tiles under a
red sky (region 20, music 3), a cheese mound in its middle (walk 13) tunnelled through at the floor
and climbed by a stair on its north and south sides, two cheese pillars, round holes bitten into
its walls; the four starts in the corners, each team's bombs beside its start. The cheese world's
mechanic, helicopters parked by the dozen, gives six; tanks, ammunition, shields, drinks, a charge.

    python3 tools/worlds/world213.py                 # writes worlds3d/world213.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from cheese import CHEESE, cheese, hole  # noqa: E402
from kit import O, World  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 36, 64
ROOF = TOP + 16
MOUND = 12         # the mound's top: walk 13


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world213.vwr"
    rng = random.Random(213)
    w = World(mission=213, sky=20, music=3)

    def walls(x0, y0, z0, x1, y1, z1):
        cheese(w, x0, y0, z0, x1, y1, z1, rng)

    closed_hall(w, X0, Z0, X1, Z1, TOP, ROOF, walls)
    # Round holes bitten into the long walls (thickened, so the holes stay dents, never ways out),
    # above the walking level (nothing stands in them).
    walls(X0 - 1, TOP, Z0 - 4, X1 + 1, ROOF, Z0 - 1)
    walls(X0 - 1, TOP, Z1 + 1, X1 + 1, ROOF, Z1 + 4)
    for x in (40, 50, 60):
        hole(w, x, WALK + 4, Z0 - 1, 2, 2, 1)
        hole(w, x, WALK + 4, Z1 + 1, 2, 2, 1)

    # ---- the mound (x 44-56, z 45-55), a tunnel through it east-west at the floor ----
    walls(44, WALK, 45, 56, MOUND, 55)
    for x in range(44, 57):
        for y in (WALK, WALK + 1):
            w.clear(x, y, 50)
    w.stairs(51, WALK, 43, 0, 1, MOUND - TOP - 1, CHEESE[0], width=3)    # north, rising south
    w.stairs(49, WALK, 57, 0, -1, MOUND - TOP - 1, CHEESE[0], width=3)   # south, rising north
    # ---- two pillars to the roof ----
    for x, z in ((38, 57), (62, 43)):
        walls(x, WALK, z, x + 1, ROOF - 1, z + 1)

    # ---- the four starts in the corners, facing along the hall; each team's bombs beside it ----
    places = ((35, WALK, 39, 2), (65, WALK, 39, 2), (35, WALK, 61, 0), (65, WALK, 61, 0))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (1, 0) if x < 50 else (-1, 0))

    # ---- the helicopters (the cheese world's), tanks, ammunition, shields, drinks, a charge ----
    for x, y, z in ((46, MOUND + 1, 47), (54, MOUND + 1, 53), (36, WALK, 50), (64, WALK, 50), (50, WALK, 39),
                    (50, WALK, 61)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((42, 42), (58, 58), (42, 58), (58, 42)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((48, MOUND + 1, 52), (52, MOUND + 1, 48), (40, WALK, 46), (60, WALK, 54), (45, WALK, 40),
                    (55, WALK, 60), (34, WALK, 45), (66, WALK, 55), (47, WALK, 50), (53, WALK, 50)):
        w.place(O.AMMO, x, y, z)
    for x, y, z in ((50, MOUND + 1, 50), (40, WALK, 62)):
        w.place(O.SHIELD, x, y, z)
    for x, z in ((60, 38), (40, 38), (56, 62)):
        w.place(O.DRINK, x, WALK, z)
    w.place(O.CHARGE, 54, MOUND + 1, 47)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
