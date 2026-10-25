#!/usr/bin/env python3
"""World 3's hub, the village (worlds3d/world030.vwr): brief plans/briefs/world030.md.

Mobile Eggbert's hub of world 3 is a village street on grass, a stone mound with trees at each
end and in the middle a terraced house of four storeys, each set back and one step to the right
of the one below: mission 1's sign on the ground floor beside the house door, missions 2, 3 and
4 each one storey higher behind their blue bars; Blupi starts at sign 1. Here the same house
stands on a village green floating in the sky: four storeys stepping back (-Z) and to the east,
their roofs terraces Blupi climbs with a jump each, a sign in each storey's facade, signs 2-4 in
niches behind bars; stone walls and trees on either side of the green.

    python3 tools/worlds/world030.py         # writes worlds3d/world030.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, boundary, rect, sign_niche  # noqa: E402
from village import (BLANK_WINDOW, DOOR, GRASS, TREES, facade_row, grass_top, roof_row,  # noqa: E402
                     stone)

GROUND = 4                  # the green's top: Blupi walks at 5
WALK = GROUND + 1
X0, X1, Z0, Z1 = 34, 66, 28, 64
# The storeys: (x0, front z, wall row y); every storey runs back to z 30 and east to x 60,
# one wall row under one roof row, so each terrace is a two-block jump above the last.
STOREYS = [(40, 47, WALK), (43, 44, WALK + 2), (46, 41, WALK + 4), (49, 38, WALK + 6)]
SIGN_X = [44, 48, 52, 56]   # one step further east per storey, as the original's stair
HOUSE_X1, HOUSE_Z0 = 60, 30
SPAWN = (44, WALK, 52)


def main() -> int:
    rng = random.Random(30)
    w = World(mission=30, sky=16)
    stone(w, X0, 0, Z0, X1, GROUND, Z1, rng, chance=0.3)              # the green's rock
    grass_top(w, X0, Z0, X1, Z1, GROUND)

    # The terraced house: storey k is solid from its front back to z 30.
    for k, (x0, front, y) in enumerate(STOREYS):
        for x, z in rect(x0, HOUSE_Z0, HOUSE_X1, front):
            w.set(x, y, z, BLANK_WINDOW)
        facade_row(w, x0, HOUSE_X1, front, y, rng)
        roof_row(w, x0, HOUSE_X1, HOUSE_Z0, front, y + 1, rng, dormer_z=front)
    w.set(54, WALK, STOREYS[0][1], DOOR)                                # the house door
    # Signs: 1 open beside the door, 2-4 two deep behind their bars.
    for k, (x0, front, y) in enumerate(STOREYS):
        if k == 0:
            sign_niche(w, SIGN_X[k], y, front, (0, 1), MISSION_SIGN1, rng, block=BLANK_WINDOW)
        else:
            sign_niche(w, SIGN_X[k], y, front - 1, (0, 1), MISSION_SIGN1 + k, rng, door=True,
                       block=BLANK_WINDOW)
    # Chimneys on the top roof.
    for x, z in ((52, 33), (58, 35)):
        stone(w, x, WALK + 8, z, x, WALK + 9, z, rng)

    # Stone walls closing the green west and east (the original's stone mounds), a low
    # parapet along its south edge, trees beside the walls.
    stone(w, X0, WALK, Z0, X0 + 1, WALK + 2, Z1, rng)
    stone(w, X1 - 1, WALK, Z0, X1, WALK + 2, Z1, rng)
    stone(w, X0, WALK, Z0, X1, WALK + 2, HOUSE_Z0 - 1, rng)
    for x, z in boundary(rect(X0, Z0, X1, Z1)):
        if z == Z1:
            w.set(x, WALK, z, STONE_PARAPET)
    for icon, x, z in ((TREES[0], 37, 50), (TREES[2], 38, 58), (TREES[1], 63, 52), (TREES[0], 62, 59)):
        w.bigdecor(icon, x, WALK, z)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world030.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


STONE_PARAPET = 156

if __name__ == "__main__":
    sys.exit(main())
