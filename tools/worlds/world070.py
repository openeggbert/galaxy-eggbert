#!/usr/bin/env python3
"""World 7's hub, the toy blocks (worlds3d/world070.vwr): brief plans/briefs/world070.md.

Mobile Eggbert's hub of world 7 is a small heap of toy tiles in the region-22 sky: Blupi starts
between two stacks of balls and stars; mission 1's sign stands open beside him on the left,
mission 2's on the right behind its blue bar; 3 and 4 sit one storey up, in the left and the right
stack, 5 a storey higher in the left one, each behind its bar. Here the stacks are two towers of
giant toy blocks across a court of toy balls: the west tower three blocks high (signs 1, 3, 5,
one per block), the east tower two (signs 2, 4), the signs in the faces turned toward the court;
smaller toy blocks lie at the towers' feet as steps, a rim of balls with striped posts round the
court, toy decor (lollipops, half balls) on it.

    python3 tools/worlds/world070.py         # writes worlds3d/world070.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, sign_niche  # noqa: E402
from toys import BALLS, CONE_BALLS, HALF_BALLS, LOLLIPOPS, STARS, TREES, post, toy  # noqa: E402

FLOOR = 4                   # the court's top: Blupi walks at 5
WALK = FLOOR + 1
X0, X1, Z0, Z1 = 30, 70, 36, 64
SPAWN = (49, WALK, 52)
COURT = (215, 215, 215, 216, 217, 219, 220)        # the original's hub: 215 most of all
FRAME = 230                                         # star badges round each sign's opening


def frame(w, x, y, z, facing):
    """Star badges round a niche's opening in its face cell (x, y, z): both sides and the lintel."""
    dx, dz = facing
    px, pz = -dz, dx
    for k in (-1, 1):
        w.set(x + px * k, y, z + pz * k, FRAME)
    for k in (-1, 0, 1):
        w.set(x + px * k, y + 1, z + pz * k, FRAME)


def main() -> int:
    rng = random.Random(70)
    w = World(mission=70, sky=22)
    toy(w, X0, 1, Z0, X1, FLOOR, Z1, rng, COURT)                       # the court
    # The rim, three high (out of a jump's reach), striped posts along it.
    for x in range(X0, X1 + 1):
        for z in range(Z0, Z1 + 1):
            if x in (X0, X1) or z in (Z0, Z1):
                toy(w, x, WALK, z, x, WALK + 2, z, rng, BALLS)
    for x in range(X0, X1 + 1, 8):
        for z in (Z0, Z1):
            post(w, x, WALK, WALK + 4, z, rng)
    for z in range(Z0 + 7, Z1, 7):
        for x in (X0, X1):
            post(w, x, WALK, WALK + 4, z, rng)

    # The west tower: three toy blocks stacked, each smaller, each a different toy.
    toy(w, 34, WALK, 41, 42, WALK + 3, 51, rng, (217, 216, 217))        # W1, top walk 9
    toy(w, 34, WALK + 4, 41, 39, WALK + 7, 48, rng, STARS)              # W2, top walk 13
    toy(w, 34, WALK + 8, 41, 36, WALK + 11, 45, rng, TREES)             # W3, top walk 17
    # The east tower: two blocks.
    toy(w, 58, WALK, 41, 66, WALK + 3, 51, rng, (219, 220))             # E1, top walk 9
    toy(w, 61, WALK + 4, 41, 66, WALK + 7, 48, rng, (218, 215))         # E2, top walk 13
    # Steps: small toy blocks left at the towers' feet, two high each (one jump).
    toy(w, 40, WALK, 52, 42, WALK + 1, 54, rng, (218,))                 # to W1, walk 7
    toy(w, 37, WALK + 4, 49, 39, WALK + 5, 51, rng, (215,))             # on W1 to W2, walk 11
    toy(w, 58, WALK, 52, 60, WALK + 1, 54, rng, (217,))                 # to E1, walk 7

    # The signs in the faces toward the court, in the original's order: 1 open beside the start,
    # 2-5 two deep behind their bars (the bar in the face), star badges round each opening.
    signs = [
        (42, WALK, 47, (1, 0), False),          # 1, W1's east face
        (59, WALK, 47, (-1, 0), True),          # 2, E1's west face
        (38, WALK + 4, 45, (1, 0), True),       # 3, W2's east face, over W1
        (62, WALK + 4, 45, (-1, 0), True),      # 4, E2's west face, over E1
        (35, WALK + 8, 43, (1, 0), True),       # 5, W3's east face, over W2
    ]
    for k, (x, y, z, facing, door) in enumerate(signs):
        sign_niche(w, x, y, z, facing, MISSION_SIGN1 + k, rng, door=door, block=217, faces=BALLS)
        fx, fz = (x + facing[0], z + facing[1]) if door else (x, z)
        frame(w, fx, y, fz, facing)

    # Toy decor on the court, as the original's BigDecor (lollipops, half balls, cones).
    for icon, x, z in ((LOLLIPOPS[0], 45, 42), (HALF_BALLS[0], 52, 40), (LOLLIPOPS[2], 55, 58),
                       (HALF_BALLS[1], 36, 58), (CONE_BALLS[0], 64, 57), (LOLLIPOPS[1], 68, 40)):
        w.bigdecor(icon, x, WALK, z)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world070.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
