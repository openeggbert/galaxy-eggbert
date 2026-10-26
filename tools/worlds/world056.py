#!/usr/bin/env python3
"""Mission 056, the valley of the tanks (worlds3d/world056.vwr): brief plans/briefs/world056.md.

Mobile Eggbert's level 056 sets twelve treasures in a V under a red sky: from a slime cave where
enemy tanks (ObjectType33) patrol and fire along their lines and enemy helicopters (ObjectType32)
drop shots, a drink at hand, a lift climbs ten cells (100 ticks) up a girder column to a metal
level; there two lifts side by side slide five cells (30 ticks) over a floor of spike traps (373)
with a tank driving under them; slime islands with tanks and spikes; then down into a V of girder
steps, six treasures down one side and six up the other, tanks guarding the steps of one side,
and the red arrow at the bottom between two pillars. Here: the cave in the west, the lift up its
north-east corner, the metal deck, the spike lane and its sliding platform heading east, the
islands, and the valley south of them, its two slopes of steps along X meeting at the arrow.

    python3 tools/worlds/world056.py                   # writes worlds3d/world056.vwr
    python3 tools/worlds/world056.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which
        has no timing with enemies): 1 the tanks and helicopters passed between shots (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift  # noqa: E402
from station import COLUMNS, GIRDER, GRILLE, SPIKE_TRAP, metal, slime  # noqa: E402

SPAWN = (12, 12, 76)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(56)
    w = World(mission=56, sky=1)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def goo(x0, y0, z0, x1, y1, z1):
        slime(w, x0, y0, z0, x1, y1, z1, rng)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def enemy(otype, a, b, ticks, stop):
        if stage < 1:
            w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    # ---- S, the slime cave (inside x 10-34, z 60-78, floor walk 10, open to the sky) ---------
    goo(9, 6, 59, 35, 9, 79)
    goo(9, 10, 59, 35, 22, 59)                        # walls five above the deck: no way
    goo(9, 10, 79, 35, 22, 79)                        # along their tops
    goo(9, 10, 59, 9, 22, 79)
    goo(35, 10, 59, 35, 22, 79)
    goo(10, 10, 72, 14, 11, 78)                       # the west shelf, walk 12: the start
    goo(17, 10, 61, 19, 11, 63)                       # mounds
    goo(27, 10, 74, 30, 11, 77)
    w.spawn = SPAWN
    w.place(O.DRINK, 11, 12, 73)                      # unseen for twenty seconds
    # A pit one deep with three eggs, a tank's lane beside it.
    carve(20, 9, 74, 22, 9, 76)
    for x in (20, 21, 22):
        w.place(O.EGG, x, 9, 75)
    enemy(O.CREATURE, (16, 10, 72), (28, 10, 72), 30, 23)
    enemy(O.CREATURE, (24, 10, 66), (25, 10, 66), 10, 23)
    enemy(O.CREATURE, (29, 10, 63), (29, 10, 64), 10, 23)
    # Two enemy helicopters hover over the floor and drop a shot at each turn.
    enemy(O.ROBOT, (14, 15, 68), (17, 15, 68), 16, 25)
    enemy(O.ROBOT, (23, 15, 76), (21, 15, 76), 8, 25)
    # The lift up the north-east corner beside a girder column: ten cells in 100 ticks (the
    #    original's), waiting 30 at each end (its 10 is half a second, a 2D step off).
    w.clear(31, 9, 61)
    lift(w, (31, 9, 61), (31, 19, 61), ticks=100, stop=30)
    for y in range(10, 22):
        w.set(32, y, 62, GIRDER)

    # ---- M, the metal deck (x 26-33, z 50-58, walk 20) ----------------------------------------
    plates(26, 18, 50, 33, 19, 58)
    plates(30, 18, 59, 32, 19, 60)                    # its tongue through the cave's wall to the lift
    carve(30, 20, 59, 32, 21, 59)
    w.place(O.EGG, 27, 20, 51)
    w.place(O.EGG, 28, 20, 51)
    for x in (26, 29):                                # two grilles away from the walls (a
        w.set(x, 20, 50, GRILLE)                      # block by a wall is a step onto it)
    # The spike lane east (x 34-40, z 50-52): spike traps in its floor four below the deck, a tank
    #    driving along it under the sliding platform (its shots fly below Blupi), walls either side.
    plates(34, 14, 49, 40, 16, 53)
    for x in range(34, 41):
        for z in range(50, 53):
            w.set(x, 17, z, SPIKE_TRAP)
    plates(34, 17, 49, 40, 23, 49)
    plates(34, 17, 53, 40, 23, 53)
    enemy(O.CREATURE, (34, 18, 51), (40, 18, 51), 20, 23)
    # The sliding platform: six lifts (two along X, three across), in step, five cells east and
    #    back (30 ticks, stop 20: the original's 3 is a 2D step).
    for x in (34, 35):
        for z in (50, 51, 52):
            lift(w, (x, 19, z), (x + 5, 19, z), ticks=30, stop=20)

    # ---- The east platform and the slime islands ----------------------------------------------
    plates(41, 18, 49, 46, 19, 53)
    goo(47, 15, 49, 52, 19, 53)                       # I1, walk 20: two eggs
    w.place(O.EGG, 51, 20, 50)
    w.place(O.EGG, 51, 20, 52)
    goo(53, 15, 44, 58, 21, 51)                       # I2, walk 22, a jump up from I1
    for x, z in ((56, 46), (57, 46)):
        w.set(x, 21, z, SPIKE_TRAP)
    enemy(O.CREATURE, (53, 22, 48), (58, 22, 48), 30, 23)
    goo(60, 15, 50, 68, 19, 57)                       # I3, walk 20
    enemy(O.CREATURE, (64, 20, 51), (64, 20, 56), 30, 23)
    w.place(O.EGG, 67, 20, 51)
    w.place(O.EGG, 61, 20, 56)

    # ---- V, the valley (z 64-72): two slopes of steps along X meeting at the bottom ------------
    # The east rim (x 70-74, z 58-72, walk 20) reached from I3 over a one-cell gap.
    goo(70, 8, 57, 74, 19, 73)
    # Steps two wide, one lower each toward the bottom: east E7..E1 (walk 19..13), the bottom
    #    (x 53-55, walk 12), west W1..W7 (walk 13..19), the west rim (x 35-38, walk 20).
    steps = {}
    for k in range(1, 8):
        steps[("E", k)] = (54 + 2 * k, 55 + 2 * k, 12 + k)               # E1 x 56-57 ... E7 x 68-69
        steps[("W", k)] = (53 - 2 * k, 54 - 2 * k, 12 + k)               # W1 x 51-52 ... W7 x 39-40
    for (x0, x1, walk) in steps.values():
        goo(x0, 8, 64, x1, walk - 1, 72)
    goo(53, 8, 64, 55, 11, 72)                         # the bottom, walk 12
    goo(35, 8, 63, 38, 19, 73)                         # the west rim, walk 20
    # The valley's sides: slime walls along z 63 and z 73, up to the rims' height.
    goo(35, 8, 63, 74, 21, 63)
    goo(35, 8, 73, 74, 21, 73)
    carve(70, 20, 63, 74, 21, 63)                      # the rim's own floor stays open north
    # Treasures, one on each of E2..E7 and W2..W7.
    for side in ("E", "W"):
        for k in range(2, 8):
            x0, x1, walk = steps[(side, k)]
            w.place(O.TREASURE, x0, walk, 68)
    # Tanks guard the west slope: on W3, W5 and W7, each driving across its step (along Z) and
    #    firing along it at each turn.
    for k in (3, 5, 7):
        x0, x1, walk = steps[("W", k)]
        enemy(O.CREATURE, (x1, walk, 64), (x1, walk, 72), 40, 23)
    # Eggs on the rims and in the valley's corners.
    for x, y, z in ((72, 20, 59), (73, 20, 71), (36, 20, 64), (37, 20, 71), (57, 13, 64), (51, 13, 72),
                    (60, 15, 72)):
        w.place(O.EGG, x, y, z)
    # The red arrow at the bottom between two columns.
    w.place(O.EXIT, 54, 12, 68)
    w.bigdecor(COLUMNS[0], 54, 12, 65)
    w.bigdecor(COLUMNS[1], 54, 12, 71)

    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world056.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
