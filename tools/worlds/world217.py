#!/usr/bin/env python3
"""Multiplayer world 217, the floating islands (worlds3d/world217.vwr): brief plans/briefs/world217.md.

Galaxy's own multiplayer world (Q-022), in the spirit of Galaxy's title scene (title.py: a grass
island floating among the stars, region 7, music 4): a large grass island in the middle and four
smaller ones at the compass points, each team's start and bombs on its own island, straight grass
causeways to the middle one (walk 12); under them all, far below, a meadow ringed by a low stone
rim that catches whoever falls (walk 5), with a lift back up beside each outer island.

    python3 tools/worlds/world217.py                 # writes worlds3d/world217.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from arena import check, starts, team_bombs  # noqa: E402
from kit import O, World, boundary, lift, rect  # noqa: E402
from title import EARTH, GRASS, GRASS_EDGE  # noqa: E402
from village import STONE  # noqa: E402

ISLE = 11          # the islands' and causeways' top: walk 12
WALK = ISLE + 1
MEADOW = 4         # the meadow's top: walk 5
CENTRE = (50, 50)
OUTER = {"N": (50, 30), "E": (70, 50), "S": (50, 70), "W": (30, 50)}


def island(w: World, cx, cz, r, rng: random.Random):
    """A round grass island whose top is at ISLE, its underside narrowing into a rocky point."""
    for x in range(cx - r - 1, cx + r + 2):
        for z in range(cz - r - 1, cz + r + 2):
            d = ((x - cx) / r) ** 2 + ((z - cz) / r) ** 2
            if d > 1.0:
                continue
            w.set(x, ISLE, z, GRASS_EDGE if d > 0.82 else rng.choice(GRASS))
            depth = int(round(1 + 5 * (1.0 - d) ** 1.5))
            for y in range(ISLE - depth, ISLE):
                w.set(x, y, z, rng.choice(EARTH))


def causeway(w: World, cells, rng: random.Random):
    """A grass path two cells deep."""
    for x, z in cells:
        if w.get(x, ISLE, z) == 0:
            w.set(x, ISLE, z, rng.choice(GRASS))
            w.set(x, ISLE - 1, z, rng.choice(EARTH))


def main(argv) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "worlds3d" / "world217.vwr"
    rng = random.Random(217)
    w = World(mission=217, sky=7, music=4)

    # ---- the meadow far below, its low rim ----
    w.fill(22, MEADOW - 1, 22, 78, MEADOW - 1, 78, STONE)
    for x, z in rect(22, 22, 78, 78):
        w.set(x, MEADOW, z, rng.choice(GRASS))
    for x, z in boundary(rect(22, 22, 78, 78)):
        w.set(x, MEADOW + 1, z, STONE)

    # ---- the islands and the causeways to the middle one ----
    island(w, *CENTRE, 9, rng)
    for cx, cz in OUTER.values():
        island(w, cx, cz, 5, rng)
    causeway(w, rect(49, 34, 51, 42), rng)   # north
    causeway(w, rect(49, 58, 51, 66), rng)   # south
    causeway(w, rect(34, 49, 42, 51), rng)   # west
    causeway(w, rect(58, 49, 66, 51), rng)   # east
    # ---- a lift from the meadow up beside each outer island, on its outer side ----
    for lx, lz in ((50, 24), (76, 50), (50, 76), (24, 50)):
        w.clear(lx, MEADOW, lz)
        lift(w, (lx, MEADOW, lz), (lx, ISLE, lz), ticks=40, stop=20)

    # ---- each team's start and bombs on its island, facing the middle ----
    places = ((50, WALK, 28, 2), (72, WALK, 50, 1), (50, WALK, 72, 0), (28, WALK, 50, 3))
    starts(w, places)
    team_bombs(w, places, 3, lambda x, z: (1, 0) if x == 50 else (0, 1))

    # ---- the helicopters, tanks, ammunition, shields, drinks, the charge ----
    for x, y, z in ((45, WALK, 50), (55, WALK, 50), (30, MEADOW + 1, 30), (70, MEADOW + 1, 70)):
        w.place(O.HELICOPTER, x, y, z)
    for x, z in ((50, 45), (50, 55)):
        w.place(O.TANK, x, WALK, z)
    for x, y, z in ((47, WALK, 47), (53, WALK, 53), (47, WALK, 53), (53, WALK, 47), (48, WALK, 31), (69, WALK, 48),
                    (52, WALK, 69), (31, WALK, 52), (70, MEADOW + 1, 30), (30, MEADOW + 1, 70)):
        w.place(O.AMMO, x, y, z)
    for x, y, z in ((44, WALK, 46), (56, WALK, 54)):
        w.place(O.SHIELD, x, y, z)
    for x, y, z in ((44, WALK, 54), (35, MEADOW + 1, 65)):
        w.place(O.DRINK, x, y, z)
    w.place(O.CHARGE, CENTRE[0], WALK, CENTRE[1])

    check(w)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
