#!/usr/bin/env python3
"""Multiplayer world 202, the machine towers (worlds3d/world202.vwr): brief plans/briefs/world202.md.

Speedy Blupi 2's second network mission (DATA/WORLD202.BLP, region 0, music 5): the blue machine
panel again, larger, two players starting on the floor and two above them, one lift and the air
full of helicopters (23), tanks (16), ammunition (51), drinks (12) and each team's bombs (11). Here
a machine hall with a tower along each side wall (walk 16): yellow and blue start on the floor,
orange and green on the towers above them, as the original's two levels; the towers joined by a
bridge along the north wall, reached by the one lift and a stair at each tower's south end.

    python3 tools/worlds/world202.py                 # writes worlds3d/world202.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import closed_hall, starts, team_bombs  # noqa: E402
from kit import MACHINE_FLOOR, O, World, lift, wall  # noqa: E402

TOP = 9                 # the floor's top: walk 10
WALK = TOP + 1
UP = 15                 # the towers' and the bridge's top: walk 16
X0, X1, Z0, Z1 = 32, 68, 38, 62
WEST, EAST = (X0, X0 + 6), (X1 - 6, X1)   # the towers


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world202.vwr"
    rng = random.Random(202)
    w = World(mission=202, sky=0, music=5)

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, face_chance=0.08)

    closed_hall(w, X0, Z0, X1, Z1, TOP, TOP + 17, panel)

    # ---- the towers, the bridge between them along the north wall, their stairs, the lift ----
    for xa, xb in (WEST, EAST):
        panel(xa, WALK, Z0 + 3, xb, UP, Z1)                        # each tower (z 41-62)
        panel(xa, UP, Z0, xb, UP, Z0 + 2)                          # its north end, under the bridge's line
    panel(WEST[1] + 1, UP, Z0, EAST[0] - 1, UP, Z0 + 2)            # the bridge (z 38-40)
    # A stair up each tower's inner face from the floor at its south end (z 61 down to 57).
    w.stairs(WEST[1] + 1, WALK, Z1 - 1, 0, -1, UP - WALK, MACHINE_FLOOR)
    w.stairs(EAST[0] - 1, WALK, Z1 - 1, 0, -1, UP - WALK, MACHINE_FLOOR)
    # The one lift, from the floor up beside the bridge's middle.
    w.clear(50, TOP, Z0 + 3)
    lift(w, (50, TOP, Z0 + 3), (50, UP, Z0 + 3), ticks=40, stop=20)

    # ---- the starts: yellow and blue on the floor, orange and green above them on the towers ----
    places = ((44, WALK, 50, 3), (35, UP + 1, 50, 3), (56, WALK, 50, 1), (65, UP + 1, 50, 1))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (0, -1))

    # ---- the vehicles, the ammunition, the drinks ----
    for x, z in ((44, 58), (56, 58), (50, 46), (50, 55)):
        w.place(O.HELICOPTER, x, WALK, z)
    for x in (34, 66):
        w.place(O.HELICOPTER, x, UP + 1, 58)
    for x in (44, 56):
        w.place(O.HELICOPTER, x, UP + 1, Z0 + 1)
    for x, z in ((40, 42), (60, 42), (46, 60), (54, 60), (50, 50)):
        w.place(O.TANK, x, WALK, z)
    for x, z in ((42, 46), (58, 46), (42, 54), (58, 54), (47, 50), (53, 50), (50, 42), (50, 60)):
        w.place(O.AMMO, x, WALK, z)
    for x, z in ((35, 44), (65, 44), (37, 56), (63, 56)):
        w.place(O.AMMO, x, UP + 1, z)
    for x, y, z in ((48, WALK, 44), (52, WALK, 56), (39, UP + 1, 46), (61, UP + 1, 54), (50, UP + 1, Z0)):
        w.place(O.DRINK, x, y, z)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
