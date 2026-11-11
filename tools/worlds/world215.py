#!/usr/bin/env python3
"""Multiplayer world 215, the sky works (worlds3d/world215.vwr): brief plans/briefs/world215.md.

Galaxy's own multiplayer world (Q-022), built of Mobile Eggbert's world 8, the blue machine works in
the sky (skyworks.py), region 26, music 8, open to the sky: a deck of cyan panel walled by a pipe
parapet; along its west and its east side a long raised deck (walk 14) up a stair, the two joined
across the south by a bridge of the Temp saucers (324) that vanish together every few seconds (world
8's own mechanic: who stands on them then falls back to the deck); in the middle a high deck (walk
18) on four pipe columns, reached by a lift on each side. The four starts in the deck's corners,
each team's bombs beside its start.

    python3 tools/worlds/world215.py                 # writes worlds3d/world215.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, starts, team_bombs  # noqa: E402
from kit import O, World, boundary, lift, rect  # noqa: E402
from skyworks import PANEL, TEMP, deck, panel, pipe_column, pipe_rail  # noqa: E402

TOP = 9            # the deck's top: walk 10
WALK = TOP + 1
X0, X1, Z0, Z1 = 34, 66, 38, 62
SIDE = 13          # the side decks' top: walk 14
HIGH = 17          # the high deck's top: walk 18


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world215.vwr"
    rng = random.Random(215)
    w = World(mission=215, sky=26, music=8)

    deck(w, X0, Z0, X1, Z1, TOP, thickness=2)
    pipe_rail(w, boundary(rect(X0, Z0, X1, Z1)), WALK, rng)

    # ---- the side decks (x 36-43 and 57-64, z 41-59), a stair up each from the middle ----
    panel(w, 36, WALK, 41, 43, SIDE, 59, rng)
    panel(w, 57, WALK, 41, 64, SIDE, 59, rng)
    w.stairs(46, WALK, 43, -1, 0, SIDE - TOP - 1, PANEL, width=2)   # x 46-44, rising west
    w.stairs(54, WALK, 42, 1, 0, SIDE - TOP - 1, PANEL, width=2)    # x 54-56, rising east
    # ... joined across the south by the vanishing saucers (z 57-58)
    w.fill(44, SIDE, 57, 56, SIDE, 58, TEMP)

    # ---- the high deck (x 46-54, z 46-54) on four pipe columns, a lift on each side ----
    for x, z in ((46, 46), (54, 46), (46, 54), (54, 54)):
        pipe_column(w, x, WALK, HIGH - 1, z, rng)
    deck(w, 46, 46, 54, 54, HIGH)
    for lx in (45, 55):
        w.clear(lx, TOP, 50)
        lift(w, (lx, TOP, 50), (lx, HIGH, 50), ticks=40, stop=20)

    # ---- the four starts in the deck's corners, facing along it; each team's bombs beside it ----
    places = ((36, WALK, 39, 3), (64, WALK, 39, 1), (36, WALK, 61, 3), (64, WALK, 61, 1))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (1, 0) if x < 50 else (-1, 0))

    # ---- the helicopters (to the high deck), tanks, ammunition, shields, drinks, the charge ----
    for x, y, z in ((48, HIGH + 1, 48), (52, HIGH + 1, 52), (38, SIDE + 1, 50), (62, SIDE + 1, 50),
                    (50, WALK, 41)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((48, 60), (52, 44), (50, 50)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((50, HIGH + 1, 50), (40, SIDE + 1, 44), (40, SIDE + 1, 56), (60, SIDE + 1, 44),
                    (60, SIDE + 1, 56), (47, WALK, 48), (53, WALK, 52), (45, WALK, 39), (55, WALK, 61)):
        w.place(O.AMMO, x, y, z)
    for x, y, z in ((48, HIGH + 1, 52), (62, SIDE + 1, 58)):
        w.place(O.SHIELD, x, y, z)
    for x, y, z in ((38, SIDE + 1, 58), (55, WALK, 39)):
        w.place(O.DRINK, x, y, z)
    w.place(O.CHARGE, 52, HIGH + 1, 48)

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
