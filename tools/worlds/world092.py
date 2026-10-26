#!/usr/bin/env python3
"""Mission 092, the lollipop dell and the saw works (worlds3d/world092.vwr): brief
plans/briefs/world092.md.

Mobile Eggbert's level 092 starts in the grey stone village (houses, trees, a pond with a fish)
walled in at the top of the clods; a lollipop (26) in a pocket gives the very high jumps that
climb out of it. Then crumbling bridges (364) over lava (68), hanging mines (3) over one of them,
a great cave where ten birds (20) fly among the plants, mines and a moving mine; below, blue
machines with saws (378) in their floors stopped by two switches (384), a box of two sticks of
dynamite (55) guarded by an enemy helicopter (32) and an enemy tank (33), and the one treasure
(5) in a machine cage behind a block of crates (12), the red arrow (7) beside it. Here the same
order in 3D: the village in a deep dell in the south-west, the lava canyon and the birds' crater
on the clods' top, a stair down into the machine hall under them, the cage south of it.

    python3 tools/worlds/world092.py                   # writes worlds3d/world092.vwr
    python3 tools/worlds/world092.py --stage K out.vwr # lessons 1..K solved, for levelcheck (no
        lollipop, switches, dynamite or timing): 1 the licked jump (a stand-in lift), 2 the saws
        stopped (379), 3 the cage's plug blasted (left out), 4 the enemies, birds and moving mine
        timed (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from clods import (BRIDGE, DEEP_WATER, LAVA, MACHINE, MACHINE_BALL, PLANTS, SAW, SWITCH, TREES,  # noqa: E402
                   TRESTLE, WATER, Y_FORK, carve, clay, column, fringe, report_audit, rough, stone)
from kit import MACHINE_FACES, O, World, mine, wall  # noqa: E402
from village import house  # noqa: E402

SAW_STOPPED = 379
TOP = 40                      # the clods' top: walls nine above the canyon and the crater, out of
                              # reach even of a licked jump (7.6 cells)
DELL = 20                     # the village's floor: walk 20
HIGH = 32                     # the canyon and the crater: walk 32
WORKS = 12                    # the machine hall: walk 12
SPAWN = (14, DELL, 86)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world092.vwr"
    rng = random.Random(92)
    w = World(mission=92, sky=27)

    def mass(x0, y0, z0, x1, y1, z1):
        clay(w, x0, y0, z0, x1, y1, z1, rng)

    def cut(x0, y0, z0, x1, y1, z1):
        carve(w, x0, y0, z0, x1, y1, z1)

    def machine(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.12, block=MACHINE)

    def enemy(otype, a, b, ticks, stop):
        if stage < 4:
            w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def perch(x, z, y0, prop, high=4):
        mass(x, y0, z, x, y0 + high - 1, z)
        w.set(x, y0 + high, z, prop)

    mass(4, 0, 26, 94, TOP, 97)

    # ---- A, the village in the dell (x 8-32, z 66-92, walk 20), open to the sky ---------------
    cut(8, DELL, 66, 32, TOP, 92)
    stone(w, 8, DELL - 2, 66, 32, DELL - 1, 92, rng)
    w.spawn = SPAWN
    house(w, 9, 67, 14, 71, DELL, 4, rng, door=11)
    house(w, 16, 67, 22, 72, DELL, 5, rng, door=19)
    w.fill(19, DELL - 3, 81, 26, DELL - 2, 88, DEEP_WATER)        # the pond, its fish leaping
    w.fill(19, DELL - 1, 81, 26, DELL - 1, 88, WATER)
    enemy(O.FISH, (21, DELL - 2, 84), (24, DELL - 2, 85), 15, 47)
    w.place(O.EGG, 10, DELL, 90)
    w.place(O.EGG, 31, DELL, 90)
    for x, z, t in ((9, 76, 0), (12, 80, 1), (30, 78, 2), (28, 86, 0), (15, 75, 2), (24, 74, 1)):
        w.bigdecor(TREES[t], x, DELL, z)
    for x, z, p in ((10, 84, 0), (17, 90, 1), (29, 73, 2)):
        w.bigdecor(PLANTS[p], x, DELL, z)
    # The way out: in the dell's north-east corner a step four high (walk 24), out of reach
    #    but for a licked jump: the lollipop lies at its foot. Then steps two high, each a plain
    #    jump, cut into the cliff north up to the canyon (walk 32).
    w.place(O.LOLLIPOP, 30, DELL, 70)
    stone(w, 28, DELL, 66, 32, DELL + 3, 68, rng)
    for k, z0 in enumerate((63, 60, 57)):
        cut(28, DELL + 4, z0, 32, TOP, z0 + 2)
        mass(28, DELL, z0, 32, DELL + 5 + 2 * k, z0 + 2)
    cut(28, HIGH, 54, 32, TOP, 56)
    if stage >= 1:                     # levelcheck has no lollipop: a stand-in lift
        w.place(O.LIFT, 26, DELL - 1, 68, end=(28, DELL + 3, 68))

    # ---- B, the lava canyon (x 26-58, z 46-56), open to the sky ------------------------------
    cut(26, HIGH, 46, 58, TOP, 56)
    # Its floor: the west landing (x 26-33) and the east landing (x 50-58) at walk 32; between
    #    them a pit of lava five below, crossed on crumbling bridges two wide (z 50-51) that give
    #    way under Blupi and come back: an island of clods halfway (x 41-42) with two eggs.
    cut(34, 25, 46, 49, HIGH - 1, 56)
    w.fill(34, 25, 46, 49, 25, 56, LAVA)
    mass(34, 24, 46, 49, 24, 56)                                   # under the lava (levelcheck)
    mass(41, 26, 49, 42, HIGH - 1, 52)
    for x0, x1 in ((34, 40), (43, 49)):
        w.fill(x0, HIGH - 1, 50, x1, HIGH - 1, 51, BRIDGE)
    w.place(O.EGG, 41, HIGH, 49)
    w.place(O.EGG, 42, HIGH, 52)
    # Two mines hang over the second bridge a cell above Blupi: walk under them, never jump.
    if stage < 4:
        w.place(O.HANGING_MINE, 45, HIGH + 1, 50)
        w.place(O.HANGING_MINE, 47, HIGH + 1, 51)
    mine(w, 54, HIGH, 47)
    perch(29, 47, HIGH, TRESTLE, high=4)
    perch(55, 55, HIGH, Y_FORK, high=4)

    # ---- C, the birds' crater (x 58-90, z 28-60, walk 32), open to the sky --------------------
    cut(58, HIGH, 28, 90, TOP, 60)
    rough(w, 58, 28, 90, 60, HIGH, TOP + 1, rng, sag=0)
    for x0, z0, x1, z1, h in ((63, 34, 68, 39, 1), (74, 44, 79, 48, 2), (82, 32, 86, 36, 1), (66, 50, 70, 55, 1)):
        mass(x0, HIGH, z0, x1, HIGH + h - 1, z1)
    w.place(O.EGG, 76, HIGH + 2, 46)
    w.place(O.EGG, 84, HIGH + 1, 34)
    w.place(O.EGG, 61, HIGH, 30)
    for x, z, p in ((60, 40, 0), (71, 31, 1), (80, 52, 2), (87, 44, 0), (64, 58, 1), (73, 40, 2), (69, 46, 0)):
        w.bigdecor(PLANTS[p], x, HIGH, z)
    # Ten birds crossing it at Blupi's height and above, in every direction.
    for a, b, t in (((60, 33, 36), (72, 33, 36), 48), ((72, 34, 30), (72, 34, 42), 48),
                    ((78, 33, 38), (88, 33, 38), 40), ((62, 34, 46), (62, 34, 56), 40),
                    ((66, 35, 44), (80, 35, 44), 56), ((84, 33, 42), (84, 33, 54), 48),
                    ((70, 33, 58), (82, 33, 58), 48), ((76, 34, 50), (88, 35, 56), 56),
                    ((60, 35, 32), (66, 34, 40), 32), ((86, 34, 30), (78, 35, 34), 32)):
        enemy(O.BIRD, a, b, t, 8)
    enemy(O.MOVING_MINE, (80, HIGH, 40), (80, HIGH, 46), 20, 0)
    mine(w, 70, HIGH, 33)
    mine(w, 86, HIGH, 49)

    # ---- D, the stair down (x 66-88, z 62-64): two-block steps, two cells each, west ---------
    cut(86, HIGH, 61, 88, TOP, 61)
    for k in range(11):
        x1 = 88 - 2 * k
        walk = HIGH - 2 * k
        cut(x1 - 1, walk, 62, x1, walk + 4, 64)
        mass(x1 - 1, walk - 3, 62, x1, walk - 1, 64)

    # ---- E, the machine hall (x 40-66, z 66-84, walk 12; machine roof at y 18) -----------------
    machine(39, WORKS - 2, 65, 67, WORKS + 6, 85)
    cut(40, WORKS, 66, 66, WORKS + 5, 84)
    cut(66, WORKS, 62, 67, WORKS + 3, 65)                         # from the stair's foot
    mass(64, WORKS - 1, 62, 67, WORKS - 1, 65)
    # Two saw catwalks one wide along X over a lava pit (x 51-60): each with three saws in its
    #    floor and its switch at its start (a switch stops the saws of its own row); a landing
    #    between them (x 55-56). Neither row can be jumped.
    cut(51, WORKS - 6, 66, 60, WORKS - 1, 84)
    w.fill(51, WORKS - 5, 66, 60, WORKS - 5, 84, LAVA)
    machine(51, WORKS - 6, 66, 60, WORKS - 6, 84)
    for x in range(57, 61):                                         # catwalk 1 (z 71), east
        w.set(x, WORKS - 1, 71, MACHINE)
    w.set(60, WORKS - 1, 71, SWITCH)
    for x in (57, 58, 59):
        w.set(x, WORKS - 1, 71, SAW if stage < 2 else SAW_STOPPED)
    machine(55, WORKS - 6, 70, 56, WORKS - 1, 79)                  # the landing (x 55-56, z 70-79)
    for x in range(51, 55):                                         # catwalk 2 (z 78), west
        w.set(x, WORKS - 1, 78, MACHINE)
    w.set(55, WORKS - 1, 78, SWITCH)                                # on the landing's edge
    for x in (51, 52, 53):
        w.set(x, WORKS - 1, 78, SAW if stage < 2 else SAW_STOPPED)
    w.place(O.EGG, 62, WORKS, 82)
    mine(w, 63, WORKS, 74)
    # Machines along the walls (decor), out of the way.
    for x0, z0, x1, z1 in ((62, 66, 65, 67), (40, 66, 43, 67), (62, 83, 65, 84)):
        machine(x0, WORKS, z0, x1, WORKS + 1, z1)
        w.set(x0 + 1, WORKS + 2, z0, MACHINE_BALL)

    # ---- F, the dynamite box (west of the pit, x 40-50) ---------------------------------------
    machine(43, WORKS, 69, 47, WORKS + 2, 73)
    cut(44, WORKS, 70, 46, WORKS + 1, 73)                           # open to the south
    w.place(O.DYNAMITE, 44, WORKS, 71)
    w.place(O.DYNAMITE, 46, WORKS, 71)
    enemy(O.ROBOT, (41, WORKS + 4, 75), (49, WORKS + 4, 75), 32, 25)   # over its mouth
    enemy(O.CREATURE, (42, WORKS, 77), (48, WORKS, 77), 40, 23)        # driving before it
    mine(w, 49, WORKS, 68)

    # ---- G, the treasure's cage (machine, x 42-50, z 87-93) and the arrow's alcove -------------
    machine(41, WORKS - 1, 85, 51, WORKS + 4, 94)
    cut(42, WORKS, 87, 50, WORKS + 2, 93)
    cut(45, WORKS, 85, 47, WORKS + 2, 86)                           # its door from the hall
    cut(45, WORKS - 1, 87, 47, WORKS - 1, 87)                       # a trench inside: the block
    machine(45, WORKS - 2, 87, 47, WORKS - 2, 87)                   #    cannot be pushed in
    if stage < 3:
        for x in (45, 46, 47):
            for y in (WORKS, WORKS + 1, WORKS + 2):
                w.place(O.CRATE, x, y, 86)
        w.place(O.CRATE, 46, WORKS, 85)                             # and one before it
    w.place(O.TREASURE, 46, WORKS, 91)
    w.place(O.EGG, 49, WORKS, 92)
    cut(34, WORKS, 88, 41, WORKS + 2, 92)                           # the arrow's alcove, west
    machine(33, WORKS - 1, 87, 33, WORKS + 3, 93)
    w.place(O.EXIT, 36, WORKS, 90)

    fringe(w, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0 if stage or report_audit(out) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
