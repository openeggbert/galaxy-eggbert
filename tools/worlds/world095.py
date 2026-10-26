#!/usr/bin/env python3
"""Mission 095, the three doors and world 9's golden key (worlds3d/world095.vwr): brief
plans/briefs/world095.md.

Mobile Eggbert's level 095 ends world 9 deep in the clods, under a dark stone sky (region 2).
Blupi starts in a little chamber and drops down a shaft onto a block of crates (12) with a stick
of dynamite (55) at each end of the gallery round it: the blast opens the way down to teleporter
alpha (330). Its partner stands in a grassy cave with trees, the hub of a chain of keys and doors:
the green key (50) across crumbling bridges (364) over lava, behind the green door (335) the red
key (49), behind the red door (334) the dynamite store, a block of crates walling in the blue key
(51), and the blue door (336) the way on: a lift, a second plug of crates and its sticks in a
cobwebbed alcove, a pool swum under a wall past three fish (17), a lift over lava, three moving
mines (16), and the last cave with the treasure (5), the invert (40) behind it and the golden key
(ObjectType21) on its ball pedestal (217), which ends world 9 once the treasure is taken. Here the
same order in 3D: the start and the gallery in a mass of clods floating high in the north-west,
the grove and its four wings below in the south-west, the way on north and east of it.

    python3 tools/worlds/world095.py                   # writes worlds3d/world095.vwr
    python3 tools/worlds/world095.py --stage K out.vwr # lessons 1..K solved, for levelcheck (no
        dynamite, keys, swimming or timing): 1 the gallery's crates blasted (left out), 2 the
        green and red doors open, 3 the blue key's crates blasted and the blue door open, 4 the
        second plug blasted, 5 the swim (a stand-in lift), 6 the moving mines timed (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from clods import (BLUE_DOOR, BRIDGE, DEEP_WATER, GIRDER, GREEN_DOOR, LAVA, PLANTS, RED_DOOR,  # noqa: E402
                   TELEPORTER, TREES, TRESTLE, WATER, WATER_WEED, Y_FORK, carve, clay, fringe,
                   report_audit, rough)
from kit import O, World, lift, mine  # noqa: E402

GRASS = 107
BALL = 217                    # the ball on its striped block: the golden key's pedestal
COBWEBS = (401, 402)
SPAWN = (17, 46, 25)
GALLERY = 34                  # the gallery round the crate pit: walk 34
PIT = 31                      # the pit's floor and the tunnel to the teleporter: walk 31
GROVE = 10                    # the grove and its wings: walk 10
UPPER = 14                    # the way on: walk 14


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world095.vwr"
    rng = random.Random(95)
    w = World(mission=95, sky=2)

    def mass(x0, y0, z0, x1, y1, z1):
        clay(w, x0, y0, z0, x1, y1, z1, rng)

    def cut(x0, y0, z0, x1, y1, z1):
        carve(w, x0, y0, z0, x1, y1, z1)

    def lava(x0, z0, x1, z1, y):
        """Lava at y over a layer of clods (levelcheck lets a fall through lava)."""
        w.fill(x0, y, z0, x1, y, z1, LAVA)
        mass(x0, y - 1, z0, x1, y - 1, z1)

    def door(x, z, tile, opened):
        cut(x, GROVE, z, x, GROVE + 1, z)
        if not opened:
            w.set(x, GROVE, z, tile)
            w.set(x, GROVE + 1, z, tile)

    def crates(x0, y0, z0, x1, y1, z1):
        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1):
                for z in range(z0, z1 + 1):
                    w.place(O.CRATE, x, y, z)

    def perch(x, z, y0, prop):
        """A post of clods four high with a prop on it, out of reach (D41)."""
        mass(x, y0, z, x, y0 + 3, z)
        w.set(x, y0 + 4, z, prop)

    # ==== The start and the gallery: a mass of clods floating high in the north-west ===========
    mass(6, 26, 8, 36, 52, 32)
    # The start chamber (x 14-20, z 22-26, walk 46): the shaft in front of Blupi, a girder frame
    #    in the ceiling over its mouth.
    cut(14, 46, 22, 20, 50, 26)
    w.spawn = SPAWN
    for x in range(15, 20):
        w.set(x, 50, 22, GIRDER)
    w.bigdecor(PLANTS[0], 14, 46, 26)
    # The shaft (x 16-18, z 19-21) down twelve onto the crate pit in the gallery's middle.
    cut(16, GALLERY, 19, 18, 50, 21)
    # The gallery (x 10-26, z 15-25, walk 34, roof y 39); a stick at each end.
    cut(10, GALLERY, 15, 26, 38, 25)
    rough(w, 10, 15, 26, 25, GALLERY + 3, 39, rng, sag=0)
    w.place(O.DYNAMITE, 11, GALLERY, 20)
    w.place(O.DYNAMITE, 25, GALLERY, 20)
    for x, z, p in ((11, 16, 1), (25, 24, 2), (11, 24, 0)):
        w.bigdecor(PLANTS[p], x, GALLERY, z)
    # The pit (x 16-18, z 19-21, y 31-33) full of crates, their top flush with the floor: a stick
    #    on them and away along Z, and the blast takes the whole linked block.
    cut(16, PIT, 19, 18, GALLERY - 1, 21)
    if stage < 1:
        crates(16, PIT, 19, 18, GALLERY - 1, 21)
    # From the pit's floor a tunnel two high east under the gallery to the teleporter room.
    cut(19, PIT, 19, 27, PIT + 1, 21)
    cut(28, PIT, 17, 32, PIT + 4, 23)
    w.set(30, PIT + 1, 20, TELEPORTER)                  # alpha: Blupi stands under it and goes
    mass(30, PIT + 2, 20, 30, PIT + 4, 20)
    w.bigdecor(PLANTS[2], 32, PIT, 23)

    # ==== The grove and its wings (walk 10), south-west ======================================
    mass(4, 0, 38, 98, 24, 98)
    # The grove (x 16-30, z 76-90, walk 10, roof y 20): grass, trees; alpha's partner by its
    #    north side: Blupi arrives at (20, 10, 81) facing +Z.
    cut(16, GROVE, 76, 30, 19, 90)
    rough(w, 16, 76, 30, 90, GROVE + 3, 20, rng, sag=0.2)
    w.fill(16, GROVE - 1, 76, 30, GROVE - 1, 90, GRASS)
    w.set(20, GROVE + 1, 80, TELEPORTER)
    mass(20, GROVE + 2, 80, 20, 19, 80)
    for x, z, t in ((17, 77, 0), (29, 77, 1), (17, 89, 2), (28, 88, 0), (25, 79, 1)):
        w.bigdecor(TREES[t], x, GROVE, z)
    for x, z, p in ((22, 86, 0), (27, 83, 2), (18, 84, 1)):
        w.bigdecor(PLANTS[p], x, GROVE, z)
    w.place(O.EGG, 29, GROVE, 89)

    # East: the green key's corridor (x 32-50, z 82-84, walk 10) over a pit of lava two deep
    #    (x 36-43) on crumbling bridges, which hold Blupi while he walks and give way behind him.
    cut(31, GROVE, 82, 50, GROVE + 3, 84)
    cut(36, GROVE - 3, 82, 43, GROVE - 1, 84)
    lava(36, 82, 43, 84, GROVE - 3)
    w.fill(36, GROVE - 1, 82, 43, GROVE - 1, 84, BRIDGE)
    w.place(O.KEY2, 48, GROVE, 83)
    w.place(O.EGG, 50, GROVE, 84)

    # North: the green door (23, z 75) and the red key's room (x 20-26, z 69-74, walk 10).
    cut(20, GROVE, 69, 26, GROVE + 3, 74)
    door(23, 75, GREEN_DOOR, stage >= 2)
    w.place(O.KEY1, 23, GROVE, 70)
    w.bigdecor(PLANTS[1], 20, GROVE, 74)

    # West: the red door (15, z 83) and the dynamite store (x 6-14, z 79-87, walk 10): five
    #    sticks along its west wall, a mine on its floor.
    cut(6, GROVE, 79, 14, GROVE + 3, 87)
    door(15, 83, RED_DOOR, stage >= 2)
    for z in (80, 82, 84, 86):
        w.place(O.DYNAMITE, 7, GROVE, z)
    w.place(O.DYNAMITE, 9, GROVE, 87)
    mine(w, 11, GROVE, 81)

    # South: the blue key's pocket (x 20-26, z 93-96, walk 10) behind a doorway three wide and
    #    two high (x 22-24, z 91-92) filled with crates; a trench one deep across its front
    #    (z 90) stops the block being pulled out. A stick put down in the trench, a jump out and
    #    away: the blast takes the block.
    cut(20, GROVE, 93, 26, GROVE + 3, 96)
    cut(22, GROVE, 91, 24, GROVE + 1, 92)
    cut(22, GROVE - 1, 90, 24, GROVE - 1, 90)
    if stage < 3:
        crates(22, GROVE, 91, 24, GROVE + 1, 92)
    w.place(O.KEY3, 23, GROVE, 95)
    w.place(O.EGG, 21, GROVE, 95)

    # ==== The way on: behind the blue door (23, z 68) ======================================
    door(23, 68, BLUE_DOOR, stage >= 3)
    # The landing (x 21-25, z 63-67, walk 10); four lifts in step (x 22-23, z 61-62) up four to
    #    the ledge north of them (walk 14), the original's ten ticks a cell, its dwell made 40.
    cut(21, GROVE, 56, 25, 19, 67)
    mass(21, GROVE, 56, 25, UPPER - 1, 60)
    cut(22, GROVE - 1, 61, 23, GROVE - 1, 62)
    for x in (22, 23):
        for z in (61, 62):
            lift(w, (x, GROVE - 1, z), (x, UPPER - 1, z), ticks=40, stop=40)
    w.bigdecor(PLANTS[0], 25, GROVE, 67)

    # The corridor east (x 26-44, z 57-59, walk 14): an alcove north of it (x 33-35, z 54-56)
    #    with three sticks under cobwebs, then a plug of crates two deep filling it (x 36-37). A
    #    stick put down at the plug, and the alcove is the shelter: the blasts reach far along X.
    cut(26, UPPER, 57, 44, UPPER + 3, 59)
    cut(33, UPPER, 54, 35, UPPER + 4, 56)
    w.set(33, UPPER + 4, 54, COBWEBS[0])
    w.set(35, UPPER + 4, 54, COBWEBS[1])
    for x in (33, 34, 35):
        w.place(O.DYNAMITE, x, UPPER, 54)
    mass(36, UPPER + 3, 57, 37, UPPER + 3, 59)          # a lintel over the plug
    if stage < 4:
        crates(36, UPPER, 57, 37, UPPER + 2, 59)

    # The pool (x 45-62, z 54-62): banks at walk 14 (x 45-47 and 60-62), the water between them
    #    flush with the floor, seven deep; a wall across its middle (x 53-54) down to two below
    #    the surface: dive under it. Three fish patrol the depths.
    cut(45, UPPER, 54, 62, 19, 62)
    rough(w, 45, 54, 62, 62, UPPER + 3, 20, rng, sag=0)
    cut(48, 7, 54, 59, UPPER - 1, 62)
    w.fill(48, 7, 54, 59, UPPER - 2, 62, DEEP_WATER)
    w.fill(48, UPPER - 1, 54, 59, UPPER - 1, 62, WATER)
    mass(53, 11, 54, 54, 19, 62)
    for a, b, t in (((50, 9, 55), (50, 9, 61), 45), ((55, 8, 61), (55, 8, 55), 45), ((58, 10, 56), (58, 10, 60), 30)):
        w.place(O.FISH, *a, end=b, advance=t, recede=t, stop_start=47, stop_end=47)
    for x, z in ((49, 56), (52, 60), (56, 57), (58, 61), (51, 58)):
        w.bigdecor(WATER_WEED, x, 7, z)
    if stage >= 5:                     # levelcheck cannot swim: a stand-in lift bank to bank
        w.place(O.LIFT, 46, UPPER - 1, 58, end=(61, UPPER - 1, 58))

    # The lava ferry (x 63-73, z 54-62): lava three below the floor (x 63-70); four lifts in step
    #    (x 63-64, z 57-58) carry Blupi six cells east to the landing (x 71-73), six ticks a cell
    #    as the original's, their dwell made 40.
    cut(63, UPPER, 54, 73, 19, 62)
    cut(63, UPPER - 3, 54, 70, UPPER - 1, 62)
    lava(63, 54, 70, 62, UPPER - 3)
    for x in (63, 64):
        for z in (57, 58):
            lift(w, (x, UPPER - 1, z), (x + 6, UPPER - 1, z), ticks=36, stop=40)

    # The mines' passage (x 74-85, z 57-59, walk 14), one danger at a time with a pocket two
    #    cells clear of each between them: a mine sweeping across it (x 75), a gate one wide
    #    (x 79, z 58) with a mine bobbing up and down in it, a second sweeping mine (x 83).
    cut(74, UPPER, 57, 85, UPPER + 3, 59)
    mass(79, UPPER, 57, 79, UPPER + 3, 57)
    mass(79, UPPER, 59, 79, UPPER + 3, 59)
    if stage < 6:
        w.place(O.MOVING_MINE, 75, UPPER, 57, end=(75, UPPER, 59), advance=20, recede=20, stop_start=0, stop_end=0)
        w.place(O.MOVING_MINE, 79, UPPER, 58, end=(79, UPPER + 2, 58), advance=20, recede=20, stop_start=0,
                stop_end=0)
        w.place(O.MOVING_MINE, 83, UPPER, 59, end=(83, UPPER, 57), advance=20, recede=20, stop_start=0, stop_end=0)

    # The last cave (x 86-94, z 40-66, walk 14, roof y 24): forks and trestles on posts, the
    #    golden key on its ball on a rock (walk 18; a step at walk 16 beside it), the treasure in
    #    a niche of the east wall with the invert behind it.
    cut(86, UPPER, 40, 94, 23, 66)
    rough(w, 86, 40, 94, 66, UPPER + 3, 24, rng, sag=0.15)
    mass(89, UPPER, 43, 91, UPPER + 3, 45)
    mass(89, UPPER, 46, 90, UPPER + 1, 47)
    w.set(90, UPPER + 4, 44, BALL)
    w.place(O.SECRET_EXIT, 90, UPPER + 5, 44)
    cut(95, UPPER, 61, 97, UPPER + 1, 61)
    w.place(O.TREASURE, 96, UPPER, 61)
    w.place(O.INVERT, 97, UPPER, 61)
    for x, z, prop in ((87, 41, Y_FORK), (93, 41, TRESTLE), (87, 65, TRESTLE), (92, 65, Y_FORK), (93, 52, Y_FORK)):
        perch(x, z, UPPER, prop)
    w.place(O.EGG, 93, UPPER, 47)
    for x, z, p in ((87, 50, 0), (90, 57, 1), (89, 63, 2), (93, 44, 0), (88, 55, 2)):
        w.bigdecor(PLANTS[p], x, UPPER, z)

    fringe(w, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0 if stage or report_audit(out) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
