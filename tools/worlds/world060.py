#!/usr/bin/env python3
"""World 6's hub, the slime grotto (worlds3d/world060.vwr): brief plans/briefs/world060.md.

Mobile Eggbert's hub of world 6 is a shelf of green slime inside the slime, with six alcoves in
a row under slime vaults: mission 1's sign open beside Blupi's start, missions 2-6 each behind
its blue bar; slime ceilings and columns over the shelf. Here the shelf becomes a grotto: a
slime floor under a slime vault, its six pods cut into the walls round it in a horseshoe (two in
the west wall, three in the north, one in the east), in the original's order from the start; a
mound of slime in the middle with a column up into the vault, columns hanging from it, and the
south side open onto the green sky.

    python3 tools/worlds/world060.py         # writes worlds3d/world060.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, sign_niche  # noqa: E402
from slime import SLIME, arch, skin, slime  # noqa: E402

FLOOR = 4                   # the grotto floor's top: Blupi walks at 5
WALK = FLOOR + 1
X0, X1, Z0, Z1 = 30, 70, 30, 66
VAULT = 13                  # the vault's underside: eight blocks over the floor
SPAWN = (36, WALK, 62)
# The pods: (x, z, facing); the sign sits in the wall, its bar (2-6) in the wall's face, a slime
# arch in front of each.
PODS = [
    (32, 58, (1, 0)),       # 1, west wall, open, beside the start (in the wall's face)
    (31, 47, (1, 0)),       # 2, west wall
    (40, 31, (0, 1)),       # 3, north wall
    (50, 31, (0, 1)),       # 4
    (60, 31, (0, 1)),       # 5
    (69, 44, (-1, 0)),      # 6, east wall
]


def main() -> int:
    rng = random.Random(60)
    w = World(mission=60, sky=31)
    slime(w, X0, 0, Z0, X1, FLOOR, Z1)                                   # the floor
    # Walls three thick on the west, north and east, up into the vault.
    slime(w, X0, WALK, Z0, X0 + 2, VAULT + 1, Z1 - 4)
    slime(w, X1 - 2, WALK, Z0, X1, VAULT + 1, Z1 - 4)
    slime(w, X0, WALK, Z0, X1, VAULT + 1, Z0 + 2)
    slime(w, X0, VAULT, Z0, X1, VAULT + 1, Z1 - 4)                       # the vault
    # The south: open onto the sky over a lip too high to jump, the vault's edge hanging lower.
    slime(w, X0, WALK, Z1, X1, WALK + 2, Z1)
    slime(w, X0, WALK, Z1 - 4, X0 + 2, WALK + 2, Z1)                     # its corners
    slime(w, X1 - 2, WALK, Z1 - 4, X1, WALK + 2, Z1)
    slime(w, X0 + 3, VAULT - 1, Z1 - 6, X1 - 3, VAULT - 1, Z1 - 4)
    # The mound: two terraces (walk 6 and 7), a column from it up into the vault.
    slime(w, 44, WALK, 41, 56, WALK, 53)
    slime(w, 47, WALK + 1, 44, 53, WALK + 1, 50)
    slime(w, 49, WALK + 2, 46, 51, VAULT - 1, 48)
    # Columns hanging from the vault, and stumps rising toward them.
    for x, z, low in ((38, 40, 9), (62, 40, 10), (41, 55, 10), (60, 54, 9), (55, 36, 10)):
        slime(w, x, low, z, x + 1, VAULT - 1, z + 1)
    for x, z in ((38, 40), (62, 40)):
        slime(w, x, WALK, z, x + 1, WALK + 1, z + 1)

    for k, (x, z, facing) in enumerate(PODS):
        sign_niche(w, x, WALK, z, facing, MISSION_SIGN1 + k, rng, door=k > 0, block=SLIME, faces=())
        face = (x + facing[0], z + facing[1]) if k > 0 else (x, z)     # the wall's inner face
        arch(w, face[0], WALK, face[1], facing)

    skin(w, rng)
    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world060.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
