#!/usr/bin/env python3
"""World 4's hub, the cheese wheel (worlds3d/world040.vwr): brief plans/briefs/world040.md.

Mobile Eggbert's hub of world 4 is one corridor through a block of Swiss cheese under a
background of leaves: Blupi starts at its west end beside mission 1's sign; going east, the
signs of missions 2-6 follow one another along the corridor, each behind its blue bar, so the
corridor opens one sign further each time a mission is won. Here the corridor is a round
courtyard inside a cheese wheel lying among the leaves: the six signs in niches in its wall,
in order clockwise -- 1 and 2 in the north wall, 3 and 4 in the east wall, 5 and 6 in the
south wall -- signs 2-6 behind their bars; the west quarter of the rind is cut down to a
terrace with a low parapet, open on the leaves. Blupi starts in front of sign 1.

    python3 tools/worlds/world040.py         # writes worlds3d/world040.vwr
"""

import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, sign_niche  # noqa: E402
from cheese import CHEESE, PLANTS, ball, hole  # noqa: E402

CX, CZ = 50, 50             # the wheel's axis
R_OUT, R_IN = 16.5, 10.5    # its rind and its courtyard
FLOOR = 4                   # the courtyard's floor: Blupi walks at 5
WALK = FLOOR + 1
TOP = WALK + 3              # the wall's top row: four high, out of a jump's reach
# (x, z, facing) of signs 1..6, clockwise; 1 is open, 2-6 two deep behind their bars.
SIGNS = [(48, 39, (0, 1)), (52, 39, (0, 1)), (61, 48, (-1, 0)), (61, 52, (-1, 0)),
         (52, 61, (0, -1)), (48, 61, (0, -1))]
SPAWN = (48, WALK, 45)


def inside(x, z, r):
    return (x - CX) ** 2 + (z - CZ) ** 2 <= r * r


def west(x, z):
    """The open quarter: the wheel's west side, where the rind is cut down to the floor."""
    return x - CX < -0.6 * math.hypot(x - CX, z - CZ)


def main() -> int:
    rng = random.Random(40)
    w = World(mission=40, sky=30)
    # The wheel: solid cheese to the floor inside the courtyard and in its open west quarter,
    # to the top in the rest of the rind; a parapet one high round the open quarter's edge.
    for x in range(CX - 17, CX + 18):
        for z in range(CZ - 17, CZ + 18):
            if not inside(x, z, R_OUT):
                continue
            top = FLOOR if inside(x, z, R_IN) or west(x, z) else TOP
            if top == FLOOR and not inside(x, z, R_OUT - 1):
                top = WALK
            for y in range(0, top + 1):
                w.set(x, y, z, rng.choice(CHEESE))
    # The signs: each cut into the flat run of wall on its side.
    for k, (x, z, facing) in enumerate(SIGNS):
        dx, dz = facing
        if k == 0:
            sign_niche(w, x, WALK, z, facing, MISSION_SIGN1, rng, block=CHEESE[0])
        else:
            sign_niche(w, x - dx, WALK, z - dz, facing, MISSION_SIGN1 + k, rng, door=True,
                       block=CHEESE[0])
    # Craters dented into the rind's top and outer face.
    for _ in range(26):
        a = rng.uniform(0, 2 * math.pi)
        r = rng.uniform(R_IN + 4.5, R_OUT + 0.5)          # the courtyard's wall stays whole
        if west(CX + r * math.cos(a), CZ + r * math.sin(a)):
            continue
        hole(w, CX + r * math.cos(a), TOP + 0.5, CZ + r * math.sin(a), rng.uniform(1.3, 2.0))
    for _ in range(30):
        a = rng.uniform(0, 2 * math.pi)
        if math.cos(a) < -0.45:
            continue                                   # the open quarter's floor stays whole
        hole(w, CX + (R_OUT + 0.4) * math.cos(a), rng.uniform(1.5, TOP - 1), CZ + (R_OUT + 0.4) * math.sin(a),
             rng.uniform(1.3, 2.6))
    # A lump of cheese in the middle of the courtyard; plants by the west wall.
    ball(w, CX, FLOOR + 0.5, CZ, 2.6, rng, ry=2.2)
    for icon, x, z in ((PLANTS[0], 41, 46), (PLANTS[2], 41, 54), (PLANTS[1], 42, 50)):
        w.bigdecor(icon, x, WALK, z)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world040.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
