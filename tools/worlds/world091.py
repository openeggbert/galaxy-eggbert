#!/usr/bin/env python3
"""Mission 091, the climb out of the clods (worlds3d/world091.vwr): brief plans/briefs/world091.md.

Mobile Eggbert's level 091 opens world 9 at the foot of a mountain of clay clods: Blupi starts
in a little grove beside a blue machine, where two sticks of dynamite (55) lie and a block of
nine crates (12) plugs the way on; the blast opens it. Then a long climb through the clods:
caves with forks and trestles, an enemy helicopter (32), water tanks in a machine house swum
through, an enemy tank (33), mines and moving mines (2, 16), a lift (90 ticks, stop 10), and at
the top the grey stone hills under the red sky, birds (20) over them, a pond with a fish (17),
a village of houses and the red arrow (7). No treasures; twelve eggs. Here the same climb in
3D: the grove deep in the south-west, the caves north of it, the tank house east, the lift
shaft, the upper cave and a crack of steps up onto the hills, the village in the north-east.

    python3 tools/worlds/world091.py                   # writes worlds3d/world091.vwr
    python3 tools/worlds/world091.py --stage K out.vwr # lessons 1..K solved, for levelcheck (no
        dynamite, swimming or timing): 1 the crate plug blasted (left out), 2 the swim under the
        tanks' wall (a stand-in lift up the east tank), 3 the moving mine, the enemies and the
        birds timed (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from clods import (DEEP_WATER, MACHINE, PLANTS, TREES, TRESTLE, WATER, WATER_WEED, Y_FORK,  # noqa: E402
                   carve, clay, column, fringe, report_audit, rough, stone)
from kit import MACHINE_FACES, O, World, lift, mine, wall  # noqa: E402
from village import house  # noqa: E402

GRASS = 107
SPAWN = (17, 10, 82)
TOP = 33                     # the hills' ground: walk 34


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if stage or len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world091.vwr"
    rng = random.Random(91)
    w = World(mission=91, sky=27)

    def mass(x0, y0, z0, x1, y1, z1):
        clay(w, x0, y0, z0, x1, y1, z1, rng)

    def cut(x0, y0, z0, x1, y1, z1):
        carve(w, x0, y0, z0, x1, y1, z1)

    def machine(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.12, block=MACHINE)

    def enemy(otype, a, b, ticks, stop):
        if stage < 3:
            w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def perch(x, z, y0, prop):
        """A thin column of clods four high with a prop on it, out of reach (D41)."""
        mass(x, y0, z, x, y0 + 3, z)
        w.set(x, y0 + 4, z, prop)

    # The mountain: clods from y 0 to the hills' ground over the caves (z 36-92), and a slab
    # under the hills north of them (z 4-35), its top two layers the village's grey stone.
    mass(6, 0, 36, 82, TOP, 92)
    mass(30, 22, 4, 82, TOP - 2, 35)
    stone(w, 30, TOP - 1, 4, 82, TOP, 35, rng)

    # ---- A, the grove (x 10-24, z 74-86, walk 10) ----------------------------------------------
    cut(10, 10, 74, 24, 19, 86)
    rough(w, 10, 74, 24, 86, 10, 20, rng)
    w.fill(10, 9, 74, 24, 9, 86, GRASS)
    w.spawn = SPAWN
    machine(10, 10, 77, 11, 12, 83)                   # the blue machine against its west wall
    w.place(O.DYNAMITE, 12, 10, 79)
    w.place(O.DYNAMITE, 12, 10, 81)
    for x, z, t in ((14, 76, 0), (22, 77, 1), (22, 85, 2), (12, 85, 0), (19, 85, 1)):
        w.bigdecor(TREES[t], x, 10, z)
    for x, z, p in ((15, 84, 0), (20, 75, 2)):
        w.bigdecor(PLANTS[p], x, 10, z)
    # The tunnel north (x 16-18, three high), plugged by a block of crates two deep: one stick
    #    of dynamite at it and a run back. A trench one deep behind the block (a jump) leaves the
    #    crates no support to be pushed onto.
    cut(16, 10, 64, 18, 12, 74)
    cut(16, 9, 69, 18, 9, 69)
    if stage < 1:
        for x in (16, 17, 18):
            for y in (10, 11, 12):
                for z in (70, 71):
                    w.place(O.CRATE, x, y, z)

    # ---- B, the cave of forks (x 10-30, z 50-63, walk 10; a shelf at walk 12 in its east) -----
    cut(10, 10, 50, 30, 17, 63)
    mass(24, 10, 50, 24, 10, 63)                      # a step (walk 11)
    mass(25, 10, 50, 30, 11, 63)                      # the shelf (walk 12)
    rough(w, 10, 50, 30, 63, 12, 18, rng)
    column(w, 14, 52, 10, 17, rng)
    enemy(O.ROBOT, (12, 13, 56), (22, 13, 56), 80, 25)   # an enemy helicopter crossing it
    mine(w, 20, 10, 52)
    w.place(O.EGG, 11, 10, 51)
    w.place(O.EGG, 29, 12, 61)
    perch(13, 60, 10, Y_FORK)
    perch(21, 50, 10, TRESTLE)
    perch(28, 52, 12, Y_FORK)
    for x, z, p in ((11, 62, 1), (26, 57, 0)):
        w.bigdecor(PLANTS[p], x, 10 if x < 24 else 12, z)
    # The passage east (z 54-56), rising a block twice to the tank house's door (walk 14).
    cut(31, 12, 54, 36, 16, 56)
    mass(34, 12, 54, 35, 12, 56)
    mass(36, 12, 54, 36, 13, 56)

    # ---- C, the tank house (machine, x 37-55, z 46-62) -----------------------------------------
    machine(37, 2, 46, 55, 20, 62)
    cut(38, 8, 47, 54, 19, 61)
    cut(37, 14, 54, 37, 16, 56)                       # its west door from the passage
    cut(55, 14, 52, 55, 16, 54)                       # its east door
    machine(38, 8, 47, 40, 13, 61)                    # the west rim (walk 14)
    machine(53, 8, 47, 54, 13, 61)                    # the east rim (walk 14)
    machine(45, 11, 47, 47, 19, 61)                   # the dividing wall, three thick
    for x0, x1 in ((41, 44), (48, 52)):               # the two tanks (surface at y 13)
        w.fill(x0, 8, 47, x1, 12, 61, DEEP_WATER)
        w.fill(x0, 13, 47, x1, 13, 61, WATER)
    machine(45, 8, 47, 47, 10, 51)                    # under the wall a channel three high and
    machine(45, 8, 57, 47, 10, 61)                    #    five wide (z 52-56)
    w.fill(45, 8, 52, 47, 10, 56, DEEP_WATER)
    for x, z in ((42, 49), (43, 59), (50, 58), (49, 48), (46, 54)):
        w.bigdecor(WATER_WEED, x, 8, z)
    w.place(O.EGG, 42, 8, 58)                         # at the west tank's bottom
    if stage >= 2:                                    # levelcheck cannot swim: a stand-in lift
        w.place(O.LIFT, 52, 7, 54, end=(52, 13, 54))

    # ---- D, the corridor and the lift shaft (walk 14 up to 23) ---------------------------------
    cut(56, 14, 51, 66, 17, 55)
    enemy(O.MOVING_MINE, (61, 14, 51), (61, 14, 55), 40, 0)   # sweeping across it
    w.place(O.EGG, 64, 14, 51)
    # Four lifts in step (x 67-68, z 52-53), their cubes in a pit flush with the floor, up nine
    #    cells (90 ticks, the original's ten a cell; its dwell of 10 made 40 to step on and off in
    #    3D) through a shaft into the floor of the upper cave.
    cut(67, 13, 52, 68, 22, 53)
    for x in (67, 68):
        for z in (52, 53):
            lift(w, (x, 13, z), (x, 22, z), ticks=90, stop=40)

    # ---- E, the upper cave (x 48-72, z 34-54, walk 23, roof at y 30) ---------------------------
    cut(48, 23, 34, 72, 29, 54)
    rough(w, 48, 34, 72, 54, 25, 30, rng)
    column(w, 57, 48, 23, 29, rng, width=2)
    column(w, 64, 38, 23, 29, rng)
    enemy(O.ROBOT, (52, 26, 40), (62, 26, 40), 80, 25)
    enemy(O.CREATURE, (49, 23, 44), (50, 23, 44), 10, 23)    # a tank on guard, firing along X
    mine(w, 58, 23, 36)
    mass(68, 23, 34, 72, 24, 37)                      # a shelf (walk 25) with three eggs
    for x in (69, 70, 71):
        w.place(O.EGG, x, 25, 35)
    perch(56, 46, 23, TRESTLE)
    perch(66, 34, 23, Y_FORK)
    # The crack north up to the hills: steps two deep, each two higher (walk 25, 27, 29, 31, 33),
    #    then the ground (34).
    cut(50, 23, 24, 54, TOP + 3, 33)
    for k in range(5):
        z0 = 32 - 2 * k
        mass(50, 23, z0, 54, 24 + 2 * k, z0 + 1)

    # ---- F, the stone hills (x 31-81, z 5-34, walk 34) under the red sky ----------------------
    # A wall of stone three high round them, out of a jump's reach.
    for x in range(30, 83):
        for z in range(4, 36):
            if x in (30, 82) or z in (4, 35):
                stone(w, x, TOP + 1, z, x, TOP + 3, z, rng)
    # The west hill, two tiers (walk 36, 38): two eggs on top, birds over it.
    stone(w, 33, TOP + 1, 8, 43, TOP + 2, 20, rng)
    stone(w, 35, TOP + 3, 10, 40, TOP + 4, 16, rng)
    w.place(O.EGG, 37, TOP + 5, 12)
    w.place(O.EGG, 38, TOP + 5, 14)
    enemy(O.BIRD, (32, TOP + 4, 22), (44, TOP + 5, 18), 48, 8)
    enemy(O.BIRD, (44, TOP + 7, 9), (34, TOP + 6, 15), 40, 8)
    # Low mounds on the way north-east, birds sweeping across it.
    stone(w, 46, TOP + 1, 14, 50, TOP + 1, 19, rng)
    stone(w, 70, TOP + 1, 20, 76, TOP + 1, 26, rng)
    enemy(O.BIRD, (48, TOP + 2, 24), (58, TOP + 2, 22), 40, 8)
    enemy(O.BIRD, (66, TOP + 2, 17), (78, TOP + 2, 17), 48, 8)
    mine(w, 66, TOP + 1, 22)
    # The pond (x 58-64, z 12-18), its surface level with the ground, a fish leaping in it.
    w.fill(58, TOP - 2, 12, 64, TOP - 1, 18, DEEP_WATER)
    w.fill(58, TOP, 12, 64, TOP, 18, WATER)
    enemy(O.FISH, (60, TOP - 1, 15), (62, TOP - 1, 15), 15, 47)
    # The village along the north wall: three houses, their fronts to the south, the red arrow
    #    in front of the middle one.
    house(w, 59, 5, 64, 9, TOP + 1, 4, rng, door=61)
    house(w, 67, 5, 72, 10, TOP + 1, 5, rng, door=70)
    house(w, 75, 5, 80, 9, TOP + 1, 4, rng, door=77)
    w.place(O.EXIT, 70, TOP + 1, 13)
    for x, z, t in ((34, 26, 0), (44, 30, 1), (57, 8, 2), (66, 30, 0), (79, 14, 1), (32, 6, 2), (56, 29, 1)):
        w.bigdecor(TREES[t], x, TOP + 1, z)
    for x, z, p in ((47, 9, 0), (73, 15, 1), (62, 25, 2)):
        w.bigdecor(PLANTS[p], x, TOP + 1, z)

    fringe(w, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0 if stage or report_audit(out) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
