#!/usr/bin/env python3
"""Multiplayer world 214, the slime pit (worlds3d/world214.vwr): brief plans/briefs/world214.md.

Galaxy's own multiplayer world (Q-022), built of Mobile Eggbert's world 6, the green slime
(slime.py: one block set, each cube skinned by what lies above and below it), region 31, music 5:
a slime hall with a pit sunk in its middle (walk 7) entered by a stair on its north and south
sides, two spike traps on the pit's floor (the slime world's own hazard, Decor::IsPiege); a ledge
along the west and the east wall (walk 14), each reached by a lift beside the original's striped
poles. The four starts in the corners, each team's bombs beside its start.

    python3 tools/worlds/world214.py                 # writes worlds3d/world214.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, closed_hall, starts, team_bombs  # noqa: E402
from kit import O, World, lift  # noqa: E402
from slime import SLIME, SLIME_TRAP, pole, skin, slime  # noqa: E402

TOP = 9            # the floor's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 32, 68, 36, 64
ROOF = TOP + 15
PIT = TOP - 3      # the pit's floor top: walk 7
LEDGE = 13         # the ledges' top: walk 14


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world214.vwr"
    rng = random.Random(214)
    w = World(mission=214, sky=31, music=5)

    def walls(x0, y0, z0, x1, y1, z1):
        slime(w, x0, y0, z0, x1, y1, z1)

    closed_hall(w, X0, Z0, X1, Z1, TOP, ROOF, walls)

    # ---- the pit (x 44-56, z 45-55, walk 7): a slime box under the floor, carved ----
    walls(43, PIT - 1, 44, 57, TOP, 56)
    for x in range(44, 57):
        for y in range(PIT + 1, TOP + 1):
            for z in range(45, 56):
                w.clear(x, y, z)
    w.stairs(49, PIT + 1, 47, 0, -1, TOP - PIT, SLIME, width=3)   # north side, rising to the floor
    w.stairs(51, PIT + 1, 53, 0, 1, TOP - PIT, SLIME, width=3)    # south side
    w.set(44, PIT, 55, SLIME_TRAP)
    w.set(56, PIT, 45, SLIME_TRAP)

    # ---- a ledge along each side wall (z 42-58), a lift up beside two poles ----
    for xa, xb, lx in ((X0, X0 + 3, X0 + 4), (X1 - 3, X1, X1 - 4)):
        walls(xa, WALK, 42, xb, LEDGE, 58)
        w.clear(lx, TOP, 50)
        lift(w, (lx, TOP, 50), (lx, LEDGE, 50), ticks=30, stop=20)
        for z in (48, 52):
            pole(w, lx, WALK, LEDGE + 2, z, rng)

    # ---- the four starts in the corners, facing along the hall; each team's bombs beside it ----
    places = ((35, WALK, 38, 2), (65, WALK, 38, 2), (35, WALK, 62, 0), (65, WALK, 62, 0))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (1, 0) if x < 50 else (-1, 0))

    # ---- the vehicles, ammunition, shields, a drink, the charge ----
    for x, y, z in ((X0 + 1, LEDGE + 1, 44), (X1 - 1, LEDGE + 1, 56), (50, WALK, 40), (50, WALK, 60)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((41, 44), (59, 56), (41, 56), (59, 44)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((46, PIT + 1, 50), (54, PIT + 1, 50), (X0 + 2, LEDGE + 1, 54), (X1 - 2, LEDGE + 1, 46),
                    (45, WALK, 41), (55, WALK, 59), (40, WALK, 50), (60, WALK, 50)):
        w.place(O.AMMO, x, y, z)
    for x, y, z in ((50, PIT + 1, 50), (X0 + 2, LEDGE + 1, 58)):
        w.place(O.SHIELD, x, y, z)
    w.place(O.DRINK, X1 - 2, LEDGE + 1, 42)
    w.place(O.CHARGE, X0 + 1, LEDGE + 1, 56)

    skin(w, rng)
    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
