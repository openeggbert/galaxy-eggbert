#!/usr/bin/env python3
"""Multiplayer world 209, the cobbled cave (worlds3d/world209.vwr): brief plans/briefs/world209.md.

Speedy Blupi 2's ninth network mission (DATA/WORLD209.BLP, region 11, music 10): the grey cobbles
(156, 35-38, 144-152), the four players starting in the middle two facing two, no lift,
helicopters (22), tanks (18), ammunition (47) and a charge; no personal bomb. Here a cave cut into
cobble: a terrace along its north and its south wall (walk 12) a step up from the floor, and a
ledge along its west and its east wall (walk 15) up a stair; the four starts in the middle, yellow
and orange facing blue and green.

    python3 tools/worlds/world209.py                 # writes worlds3d/world209.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, starts  # noqa: E402
from cobbles import carve, dress, rock  # noqa: E402
from kit import O, World  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 34, 66, 38, 62
ROOF = TOP + 14
TERRACE = 11       # the terraces' top: walk 12
LEDGE = 14         # the ledges' top: walk 15


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world209.vwr"
    rng = random.Random(209)
    w = World(mission=209, sky=11, music=10)

    rock(w, X0 - 1, TOP - 1, Z0 - 1, X1 + 1, ROOF, Z1 + 1)
    carve(w, X0, WALK, Z0, X1, ROOF - 1, Z1)

    # ---- the terraces along the north and south walls, a step up along their whole edge ----
    for za, zb, step in ((Z0, Z0 + 4, Z0 + 5), (Z1 - 4, Z1, Z1 - 5)):
        rock(w, X0, WALK, za, X1, TERRACE, zb)
        rock(w, X0 + 6, WALK, step, X1 - 6, WALK, step)              # the step (walk 11)
    # ---- the ledges along the west and east walls, between the terraces, a stair up each ----
    for xa, xb, sx, dx in ((X0, X0 + 3, X0 + 7, -1), (X1 - 3, X1, X1 - 7, 1)):
        rock(w, xa, WALK, Z0 + 5, xb, LEDGE, Z1 - 5)
        w.stairs(sx, WALK, 50, dx, 0, LEDGE - TOP - 1, 35, width=2)

    dress(w, X0 - 1, TOP - 1, Z0 - 1, X1 + 1, ROOF, Z1 + 1, rng)

    # ---- the four starts in the middle: yellow and orange facing east, blue and green west ----
    places = ((46, WALK, 49, 3), (46, WALK, 51, 3), (54, WALK, 51, 1), (54, WALK, 49, 1))
    starts(w, places)

    # ---- the vehicles, the ammunition, the charge ----
    for x, y, z in ((40, TERRACE + 1, Z0 + 2), (60, TERRACE + 1, Z1 - 2), (X0 + 1, LEDGE + 1, 46),
                    (X1 - 1, LEDGE + 1, 54), (50, WALK, 45), (50, WALK, 55)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((42, 48), (58, 52), (44, 54), (56, 46), (50, 50)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((47, WALK, 46), (53, WALK, 54), (42, WALK, 52), (58, WALK, 48), (50, TERRACE + 1, Z0 + 1),
                    (50, TERRACE + 1, Z1 - 1), (36, TERRACE + 1, Z0 + 3), (64, TERRACE + 1, Z1 - 3),
                    (X0 + 2, LEDGE + 1, 52), (X1 - 2, LEDGE + 1, 48), (45, WALK, 50), (55, WALK, 50)):
        w.place(O.AMMO, x, y, z)
    w.place(O.CHARGE, X0 + 2, LEDGE + 1, Z1 - 7)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
