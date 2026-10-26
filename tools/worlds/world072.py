#!/usr/bin/env python3
"""Mission 072, the lollipop well (worlds3d/world072.vwr): brief plans/briefs/world072.md.

Mobile Eggbert's level 072 teaches the lollipop (ObjectType 26): licked with Action it gives very
high jumps for 15 s, and it is there again at once for another lick (Decor.cpp ObjectStart after
the Sucette action). Everything is of yellow checkered balls (215) with single orange balls (217)
as ledges and candy poles (235) as decor; six treasures, the red arrow at the top, which counts
only once all six are taken. Blupi starts in a little room over a pit with the lollipop: down,
lick, and the high jump back up; then a tall shaft of small ledges four rows apart, a treasure on
the first, three side by side in a wider chamber halfway up, two beside the arrow at the top. The
power ends on the way: he drops back down for another lick (the video shows two). Here the shaft
is a well round a toy tower: a stair of orange-ball blocks climbs its west face, each five higher
than the last and touching it, a jump only the lollipop gives; at the chamber's level two columns
stand off west and east, two higher across two cells: no ordinary jump clears that.

    python3 tools/worlds/world072.py                 # writes worlds3d/world072.vwr
    python3 tools/worlds/world072.py --stage 1 out   # for levelcheck: stand-ins where each
                                                     # licked jump goes (the lollipop's power)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World  # noqa: E402
from toys import CANDY_POLES, CONE_BALLS, HALF_BALLS, LOLLIPOPS, toy  # noqa: E402

BALLS = 215            # the yellow checkered ball: the whole mass
LEDGE = 217            # the orange ball: every ledge
FLOOR = 4              # the well's floor top: walk 5
WALK = FLOOR + 1
X0, X1, Z0, Z1 = 37, 63, 39, 63          # the well's inside
RIM = 40               # the walls' top block: their rim at walk 41, out of every jump's reach
SPAWN = (44, 9, 52)

# (x0, x1, z0, z1, walk): the orange-ball blocks, from the floor up. The climb is a stair up the
# tower's west face, every jump toward -Z (the original's shaft is one straight climb), B3 and the
# tower against the well's north wall (a jump too many there meets the wall); one turn east onto
# the top.
PIER = (42, 46, 51, 52, 9)               # the start, four over the floor
B1 = (42, 46, 48, 50, 14)
B2 = (42, 46, 44, 47, 19)                # the chamber's level
COL_W = (38, 39, 44, 46, 21)             # two higher, across two cells west of B2
COL_E = (49, 51, 46, 47, 21)             # two higher, across two cells east, against the tower
B3 = (42, 46, 39, 43, 24)
TOWER = (47, 53, 39, 45, 29)             # the tower's top: the arrow


def block(w, rng, spec, tile):
    x0, x1, z0, z1, walk = spec
    toy(w, x0, WALK, z0, x1, walk - 1, z1, rng, (tile,))


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world072.vwr"
    rng = random.Random(72)
    w = World(mission=72, sky=11)
    # The well: a floor, walls three thick up to the rim, candy poles standing on the rim.
    toy(w, X0 - 3, 0, Z0 - 3, X1 + 3, FLOOR, Z1 + 3, rng, (BALLS,))
    for x in range(X0 - 3, X1 + 4):
        for z in range(Z0 - 3, Z1 + 4):
            if not (X0 <= x <= X1 and Z0 <= z <= Z1):
                w.fill(x, WALK, z, x, RIM, z, BALLS)
    for x in range(X0 - 2, X1 + 3, 4):
        for z in (Z0 - 2, Z1 + 2):
            w.fill(x, RIM + 1, z, x, RIM + 4, z, rng.choice(CANDY_POLES))
    for z in range(Z0 + 2, Z1, 4):
        for x in (X0 - 2, X1 + 2):
            w.fill(x, RIM + 1, z, x, RIM + 4, z, rng.choice(CANDY_POLES))

    block(w, rng, TOWER, BALLS)
    for spec in (PIER, B1, B2, COL_W, COL_E, B3):
        block(w, rng, spec, LEDGE)
    # A crown of orange balls on the tower's top corners, the arrow between two treasures.
    for x, z in ((53, 39), (47, 45), (53, 45)):
        w.set(x, TOWER[4], z, LEDGE)
    w.place(O.EXIT, 50, TOWER[4], 40)
    for x, z in ((48, 40), (52, 40)):
        w.place(O.TREASURE, x, TOWER[4], z)
    w.place(O.TREASURE, 44, B1[4], 49)
    w.place(O.TREASURE, 44, B2[4], 45)
    w.place(O.TREASURE, 38, COL_W[4], 45)
    w.place(O.TREASURE, 50, COL_E[4], 46)

    # The lollipop on the floor behind the pier (south): stepping off its back edge he lands on
    # it, and after the lick every jump but the last is toward -Z. Big toy lollipops round the
    # floor (the original's BigDecor 46, 47), toy decor.
    w.place(O.LOLLIPOP, 44, WALK, 54)
    for icon, x, z in ((LOLLIPOPS[1], 39, 56), (LOLLIPOPS[2], 49, 57), (LOLLIPOPS[0], 44, 61),
                       (HALF_BALLS[0], 58, 56), (CONE_BALLS[1], 59, 43), (HALF_BALLS[1], 56, 61)):
        w.bigdecor(icon, x, WALK, z)

    if stage >= 1:
        # Stand-ins for the licked jumps (levelcheck jumps two): a lift in the lower block's edge
        # cell rising to the next block's level beside it; bridges over the gaps to the columns.
        def standin(x, z, walk_a, walk_b):
            w.place(O.LIFT, x, walk_a - 1, z, end=(x, walk_b - 1, z))
        standin(44, 53, WALK, PIER[4])           # floor -> pier
        standin(44, 51, PIER[4], B1[4])          # pier -> B1
        standin(44, 48, B1[4], B2[4])            # B1 -> B2
        standin(44, 44, B2[4], B3[4])            # B2 -> B3
        standin(46, 41, B3[4], TOWER[4])         # B3 -> the top
        w.fill(40, COL_W[4] - 1, 45, 41, COL_W[4] - 1, 45, BALLS)
        w.fill(47, COL_E[4] - 1, 46, 48, COL_E[4] - 1, 46, BALLS)
    w.spawn = SPAWN
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
