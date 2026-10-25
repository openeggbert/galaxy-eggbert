#!/usr/bin/env python3
"""World 2's hub, the white palace (worlds3d/world020.vwr): brief plans/briefs/world020.md.

Mobile Eggbert's hub of world 2 is a staircase climbing through the palace: Blupi starts at the
bottom beside mission 1's sign; above it, each two rows higher and a step to the left, the
signs of missions 2, 3, 4 and 5 stand on ledges behind their blue bars, and ledges on the other
side of the shaft are the steps between them. Here the staircase is a stepped palace mound,
four white tiers in a walled courtyard, each tier a jump above the last: sign 1 in the foot of
the mound, signs 2-5 in niches in the risers of tiers 2, 3 and 4 and in a shrine on the top,
alternately on the east and west sides, so Blupi climbs round the mound from sign to sign.

    python3 tools/worlds/world020.py         # writes worlds3d/world020.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, rect, slab  # noqa: E402
from palace import (COLUMN, FOUNTAIN, GRASS, PALMS, PLANTS, STONE, sign_niche, stone,  # noqa: E402
                    windows_row)

X0, X1, Z0, Z1 = 30, 70, 30, 74          # the courtyard, walls included
# The tiers (x0, z0, x1, z1); tier k fills y 2k-1 .. 2k, so its top is walking level 2k + 1.
TIERS = [(38, 38, 62, 62), (41, 41, 59, 59), (44, 44, 56, 56), (47, 47, 53, 53)]
SPAWN = (48, 1, 66)


def main() -> int:
    rng = random.Random(20)
    w = World(mission=20, sky=25)
    slab(w, X0, Z0, X1, Z1, 0, block=STONE)                              # the courtyard, walk 1
    stone(w, X0, 1, Z0, X1, 4, Z0 + 1, rng, chance=0.25)                 # walls, 4 high
    stone(w, X0, 1, Z0, X0 + 1, 4, Z1, rng, chance=0.25)
    stone(w, X1 - 1, 1, Z0, X1, 4, Z1, rng, chance=0.25)
    stone(w, X0, 1, Z1, X1, 4, Z1, rng, chance=0.25)
    windows_row(w, X0 + 2, X1 - 2, 3, Z0 + 1, rng, chance=0.6)           # the north wall's face

    # The mound: four tiers, their south risers with windows and trims.
    for k, (x0, z0, x1, z1) in enumerate(TIERS, start=1):
        stone(w, x0, 2 * k - 1, z0, x1, 2 * k, z1, rng, chance=0.3)
        windows_row(w, x0 + 1, x1 - 1, 2 * k - 1, z1, rng, chance=0.35)

    # Mission 1's sign at the mound's foot (open); 2-5 behind their bars, alternately east and
    # west, every gated niche opening along X.
    sign_niche(w, 50, 1, TIERS[0][3], (0, 1), MISSION_SIGN1, rng)
    sign_niche(w, TIERS[1][2] - 1, 3, 50, (1, 0), MISSION_SIGN1 + 1, rng, door=True)
    sign_niche(w, TIERS[2][0] + 1, 5, 50, (-1, 0), MISSION_SIGN1 + 2, rng, door=True)
    sign_niche(w, TIERS[3][2] - 1, 7, 50, (1, 0), MISSION_SIGN1 + 3, rng, door=True)
    # The shrine on the top tier (walk 9), its niche opening west over the top.
    stone(w, 51, 9, 48, 53, 10, 52, rng, chance=0.3)
    sign_niche(w, 52, 9, 50, (-1, 0), MISSION_SIGN1 + 4, rng, door=True)

    # The courtyard: grass beds with palms and plants in the corners, fountains before the
    # mound, columns at the tiers' corners.
    for bx, bz in ((32, 32), (64, 32), (32, 68), (64, 68)):
        for x, z in rect(bx, bz, bx + 3, bz + 3):
            w.set(x, 0, z, GRASS)
        w.bigdecor(PALMS[(bx + bz) // 32 % 2], bx + 1, 1, bz + 1)
        w.bigdecor(PLANTS[bx % 3], bx + 3, 1, bz + 3)
    for x in (40, 60):
        w.set(x, 1, 68, STONE)                                           # basins' plinths
        w.bigdecor(FOUNTAIN, x, 2, 68)
    for k, (x0, z0, x1, z1) in enumerate(TIERS[:3], start=1):
        for x, z in ((x0, z0), (x1, z0), (x0, z1), (x1, z1)):
            w.bigdecor(COLUMN, x, 2 * k + 1, z)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world020.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
