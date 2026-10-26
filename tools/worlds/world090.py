#!/usr/bin/env python3
"""World 9's hub, the sinkhole of clods (worlds3d/world090.vwr): brief plans/briefs/world090.md.

Mobile Eggbert's hub of world 9 is a sponge of clay clods (284/301) in a dark red cave
(region 27): Blupi starts at the top left beside mission 1's sign; the pockets of missions 2-5
follow in a chain, each a step lower and further on, every one behind its blue bar (182). Here
the chain winds down a sinkhole in a floating mass of clods: ledges round its four sides, each
three lower than the last (north 26, east 23, south 20, west 17) and joined at the corners by
one-block steps, with a pocket in each ledge's outer wall; from the west ledge a stair of clods
leads down into the pit, where mission 5's pocket is cut under the start. A fall into the pit
is a short cut down; the stair and the corner steps lead back up.

    python3 tools/worlds/world090.py         # writes worlds3d/world090.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from clods import (BUMPS, CLODS, PLANTS, carve, clay, fringe, report_audit,  # noqa: E402
                   STRING)
from kit import MISSION_SIGN1, World, sign_niche  # noqa: E402

N, E, S, W = 26, 23, 20, 17      # the ledges' walking levels
PIT = 12                          # the pit's floor: walk 12
RIM = 31                          # above every wall top
SPAWN = (47, N, 43)


def main() -> int:
    rng = random.Random(90)
    w = World(mission=90, sky=27)

    def mass(x0, y0, z0, x1, y1, z1):
        clay(w, x0, y0, z0, x1, y1, z1, rng)

    def niche(x, y, z, facing, n):
        sign_niche(w, x, y, z, facing, MISSION_SIGN1 + n - 1, rng, door=n > 1, block=CLODS[0],
                   faces=(CLODS[1],))

    # The floating mass (x 37-63, z 37-63): solid up to the pit's floor, its walls up to the
    # rim; the hollow inside carved out level by level.
    mass(38, 6, 38, 62, RIM - 1, 62)
    mass(40, 3, 40, 60, 5, 60)                       # its underside narrowing
    mass(44, 1, 44, 56, 2, 56)
    # The pit (x 45-55, z 45-55), open from its floor (walk 12) to the sky.
    carve(w, 45, PIT, 45, 55, RIM - 1, 55)
    # The north ledge (z 40-44, x 40-60): walk 26, open above.
    carve(w, 40, N, 40, 60, RIM - 1, 44)
    # The east ledge (x 56-60, z 45-60): steps down from the north ledge at z 45 (25) and 46
    #    (24), then walk 23.
    carve(w, 56, E, 45, 60, RIM - 1, 60)
    mass(56, E, 45, 60, N - 2, 45)
    mass(56, E, 46, 60, N - 3, 46)
    # The south ledge (x 40-55, z 56-60): steps from the east ledge at x 55 (22) and 54 (21),
    #    then walk 20.
    carve(w, 40, S, 56, 55, RIM - 1, 60)
    mass(55, S, 56, 55, E - 2, 60)
    mass(54, S, 56, 54, E - 3, 60)
    # The west ledge (x 40-44, z 40-55): steps from the south ledge at z 55 (19) and 54 (18),
    #    then walk 17; the north ledge is above it there (walk 26): its corner x 40-44, z 40-44
    #    is a solid pillar of the north ledge, the west ledge running z 45-55.
    carve(w, 40, W, 45, 44, RIM - 1, 55)
    mass(40, W, 55, 44, S - 2, 55)
    mass(40, W, 54, 44, S - 3, 54)
    # The stair into the pit from the west ledge's north end: clods along the pit's west side,
    #    each one lower (walk 16, 15, 14, 13), then the floor (12).
    for k, z in enumerate((49, 47, 45)):
        mass(45, PIT, z, 46, W - 2 - k, z + 1)
    mass(47, PIT, 45, 48, PIT, 46)                    # walk 13, the last step
    # The north ledge reaches over the pit's north edge as an overhang at z 45 (its underside
    #    at y 22), lumps under it (fringe below).
    mass(47, PIT + 10, 45, 53, N - 1, 45)

    # The signs: 1 open in the north ledge's north wall before the start; 2 in the east wall,
    #    3 in the south wall, 4 in the west wall, each two deep behind its bar; 5 in the pit's
    #    north wall under the start ledge.
    niche(47, N, 39, (0, 1), 1)
    niche(62, E, 53, (-1, 0), 2)
    niche(47, S, 62, (0, -1), 3)
    niche(38, W, 50, (1, 0), 4)
    niche(50, PIT, 43, (0, 1), 5)

    # The crater's walls (two thick round the ledges) stand only three above the ledge beside
    #    them, out of a jump's reach, a lump or two higher here and there, so the red cave shows
    #    over them as through the original's sponge of clods.
    def walk(x, z):
        if 45 <= x <= 55 and 45 <= z <= 55:
            return PIT
        if z <= 44:
            return N
        if x >= 56:
            return {45: N - 1, 46: N - 2}.get(z, E)
        if z >= 56:
            return {55: E - 1, 54: E - 2}.get(x, S)
        return {55: S - 1, 54: S - 2}.get(z, W)

    for x in range(38, 63):
        for z in range(38, 63):
            if 40 <= x <= 60 and 40 <= z <= 60:
                continue
            near = [walk(ix, iz) for ix in range(x - 2, x + 3) for iz in range(z - 2, z + 3)
                    if 40 <= ix <= 60 and 40 <= iz <= 60]
            top = max(near) + 2 + rng.choice((0, 0, 0, 1, 1, 2))
            carve(w, x, top + 1, z, x, RIM, z)
            solid = all(w.get(x, y, z) in CLODS for y in range(top - 3, top + 1))
            if solid and rng.random() < 0.35:          # not over a sign's niche
                w.set(x, top + 1, z, rng.choice(BUMPS))
    # Clods floating in the cave round the hub, four or more cells off its walls (no jump
    #    reaches them), lumps hanging under them; one over the pit, strings of lumps under it.
    for x0, y0, z0, x1, y1, z1 in ((30, 22, 44, 33, 24, 48), (67, 18, 35, 70, 20, 38), (66, 27, 60, 69, 28, 64),
                                   (44, 14, 67, 49, 16, 70), (31, 31, 60, 34, 33, 63), (55, 33, 30, 59, 34, 33),
                                   (48, 33, 48, 52, 34, 52)):
        mass(x0, y0, z0, x1, y1, z1)
        mass(x0 + 1, y1 + 1, z0 + 1, x1 - 1, y1 + 1, z1 - 1)
    for x, z in ((49, 49), (51, 51), (50, 48)):
        w.set(x, 32, z, STRING)
    for x, z, p in ((53, 53, 0), (49, 54, 1), (54, 48, 2), (51, 51, 0)):
        w.bigdecor(PLANTS[p], x, PIT, z)
    for x, z, p in ((59, 57, 1), (42, 59, 2), (58, 41, 0), (41, 52, 1)):
        y = N if z <= 44 else E if x >= 56 else S if z >= 56 else W
        w.bigdecor(PLANTS[p], x, y, z)

    fringe(w, rng, box=(40, 40, 60, 60))
    fringe(w, rng, box=(25, 25, 75, 39))
    fringe(w, rng, box=(25, 63, 75, 75))
    fringe(w, rng, box=(25, 40, 37, 62))
    fringe(w, rng, box=(63, 40, 75, 62))
    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world090.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0 if report_audit(out) else 1


if __name__ == "__main__":
    sys.exit(main())
