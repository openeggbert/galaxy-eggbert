#!/usr/bin/env python3
"""Mission 063, the slime works (worlds3d/world063.vwr): brief plans/briefs/world063.md.

Mobile Eggbert's level 063 is a building site in the slime, under the red machine wall of
region 13: girders, riveted metal blocks, machines. From Blupi's start a shuttle carries him
over a pit past a floating mine, a girder elevator lifts him to a bowl crossed on a long
shuttle, a metal tree in the middle of it holds six eggs; the skateboard rides over a row of
six mines; a tower of arms with saws and mines is climbed after a switch stops the saws of its
lowest arm; a shaft lift with spikes under it, a gallery of pumping pistons, and a last tower,
saws and mines on its arms, with the red arrow in a cage at its top. Here in 3D: the pit, the
elevator, a basin with a pool and a terraced side under a girder gantry, the tree of metal
plates beside the shuttle's way, a machine yard, the mine corridor, the saw tower over a pit,
the shaft, the piston gallery, the exit tower.

    python3 tools/worlds/world063.py                 # writes worlds3d/world063.vwr
    python3 tools/worlds/world063.py --stage 1 out   # for levelcheck: no mines in the corridor
                                                     # (the skateboard), the beam's saws stopped
                                                     # (the switch), the basin shuttle and the
                                                     # pistons as floors
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from slime import SLIME_TRAP, skin, slime  # noqa: E402

METAL = (78, 79, 80, 81, 82, 83, 84)  # the riveted blocks
MACHINES = (89, 84, 87)                # the original's "9M7": an arm end, a block, an arm end
GEAR = 90
GIRDER, GRATE = 201, 200
SAW, SWITCH = 378, 384
POLE, KNOB = 235, 410
WATER, DEEP = 92, 91


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world063.vwr"
    rng = random.Random(63)
    w = World(mission=63, sky=13)
    stand_ins = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def metal(x0, y0, z0, x1, y1, z1):
        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1):
                for z in range(z0, z1 + 1):
                    w.set(x, y, z, rng.choice(METAL))

    def girder(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, GIRDER)

    # The slime mass of the four parts.
    slime(w, 38, 5, 60, 56, 27, 97)        # the start, the pit, the elevator
    slime(w, 26, 7, 22, 66, 35, 70)        # the basin, the machine yard
    slime(w, 56, 10, 6, 99, 37, 42)        # the mine corridor, the saw yard, the cliff ledge
    slime(w, 50, 26, 0, 99, 68, 20)        # the shaft, the gallery, the exit tower

    # ---- the start, the pit (walk 11) ----
    # A, the start pocket; B, the pit north of it, crossed on a shuttle two wide that a floating
    # mine waits over: Blupi rides it on the lane the mine is not over. A fall into the pit: a
    # step and a jump back up into the pocket.
    carve(42, 11, 86, 50, 14, 95)
    carve(44, 8, 77, 49, 14, 85)
    slime(w, 44, 8, 84, 44, 8, 85)                                    # the step back up, walk 9
    for x in (46, 47):
        for z in (77, 78, 79):
            lift(w, (x, 10, z), (x, 10, z + 6), ticks=60, stop=10)
    w.place(O.MOVING_MINE, 46, 11, 81, end=(46, 11, 81), advance=10, recede=10, stop_start=0, stop_end=0)
    # C, the landing; the elevator (2 x 2, twelve up) in a metal shaft to the ledge E (walk 23).
    carve(42, 11, 68, 50, 14, 76)
    metal(44, 11, 68, 47, 26, 71)
    carve(45, 10, 69, 46, 26, 70)
    carve(45, 11, 71, 46, 13, 71)                                     # its door from C
    for x in (45, 46):
        for z in (69, 70):
            lift(w, (x, 10, z), (x, 22, z), ticks=72, stop=3)
    carve(40, 23, 62, 52, 26, 68)

    # ---- the basin (rims walk 20) ----
    # A hall over the basin, the ledge E above its south rim. The basin: a pool (its surface at
    # 13), the west side terraced in metal from the bank (walk 14) up to the west rim, the north
    # and east sides sheer. The shuttle (2 x 3) crosses it along Z from the south rim to the
    # north rim; beside its way the tree, metal plates stacked two apart on a girder trunk, six
    # eggs on them, reached only by a jump off the passing shuttle.
    carve(30, 20, 38, 62, 32, 61)
    carve(32, 11, 43, 62, 19, 58)
    w.fill(38, 11, 43, 62, 12, 58, DEEP)
    w.fill(38, 13, 43, 62, 13, 58, WATER)
    for k, x in enumerate(range(37, 31, -1)):                         # walk 14, 15 ... 19
        metal(x, 11, 43, x, 13 + k, 58)
    if stand_ins:
        w.fill(47, 19, 43, 48, 19, 58, 10)                            # the shuttle as a bridge
    else:
        for x in (47, 48):
            for z in (56, 57, 58):
                lift(w, (x, 19, z), (x, 19, z - 13), ticks=130, stop=10)
    girder(52, 11, 50, 52, 20, 50)                                    # the trunk
    metal(49, 21, 47, 55, 21, 53)                                     # walk 22
    metal(50, 23, 48, 54, 23, 52)                                     # walk 24
    metal(51, 25, 49, 53, 25, 51)                                     # walk 26
    metal(52, 27, 50, 52, 27, 50)                                     # walk 28
    girder(52, 22, 50, 52, 22, 50)
    girder(52, 24, 50, 52, 24, 50)
    w.place(O.EGG, 52, 28, 50)
    for x, z in ((51, 49), (53, 51)):
        w.place(O.EGG, x, 26, z)
    for x, z in ((50, 48), (54, 48), (52, 52)):
        w.place(O.EGG, x, 24, z)
    # The gantry over the shuttle's way, hung from the roof.
    for x in (46, 49):
        girder(x, 26, 40, x, 26, 61)
    for z in (44, 50, 56):
        girder(46, 26, z, 49, 26, z)
    for x, z in ((46, 40), (49, 40), (46, 61), (49, 61), (46, 50), (49, 50)):
        girder(x, 27, z, x, 32, z)

    # ---- the machine yard (walk 20) and the mine corridor ----
    # North of the basin: machines along its walls, grates under its ceiling, the skateboard.
    carve(40, 20, 26, 58, 25, 37)
    for x, z0 in ((40, 27), (40, 33), (58, 27), (58, 34)):
        for k, icon in enumerate(MACHINES):
            w.set(x, 20, z0 + k, icon)
        w.set(x, 21, z0 + 1, GEAR)
    for x in range(44, 56, 3):
        w.fill(x, 25, 29, x + 1, 25, 34, GRATE)
    w.place(O.SKATEBOARD, 49, 20, 31)
    # East, a corridor two wide; six mines across it, three deep: only the skateboard passes.
    carve(59, 20, 30, 74, 22, 31)
    if not stand_ins:
        for x in (65, 66, 67):
            for z in (30, 31):
                mine(w, x, 20, z)

    # ---- the saw yard (walk 20) and the saw tower ----
    # The yard; in its north-east a pit (walk 14, steps back up) with the tower in it: a metal
    # core, and arms two wide round it, each two above the last, from the beam (walk 20) to the
    # ledge L (walk 32). The beam, one wide along X from the yard's edge, has three saws in its
    # floor; the switch at its start, in its row, stops them.
    carve(75, 20, 18, 81, 35, 38)
    carve(82, 20, 27, 94, 35, 38)
    carve(82, 14, 19, 94, 35, 26)
    slime(w, 83, 14, 26, 84, 15, 26)                                  # walk 16
    slime(w, 85, 14, 26, 86, 17, 26)                                  # walk 18
    metal(89, 14, 22, 89, 31, 22)                                     # the core
    metal(82, 19, 22, 88, 19, 22)                                     # the beam, walk 20
    w.set(81, 19, 22, SWITCH)
    for x in (84, 85, 86):                                            # blades across the way
        w.set(x, 19, 22, 10 if stand_ins else SAW)                   # the stage: stopped
        w.rotate_plate(x, 19, 22)
    metal(88, 21, 19, 89, 21, 21)                                     # 1 north, walk 22
    metal(90, 23, 21, 92, 23, 22)                                     # 2 east, walk 24
    metal(89, 25, 23, 90, 25, 25)                                     # 3 south, walk 26
    metal(86, 27, 22, 88, 27, 23)                                     # 4 west, walk 28
    metal(87, 29, 19, 88, 29, 21)                                     # 5 north, walk 30
    for x, z in ((92, 21), (92, 22)):
        w.set(x, 23, z, SAW)
        w.rotate_plate(x, 23, z)
    for x, z in ((89, 25), (90, 25)):
        w.set(x, 25, z, SAW)
    mine(w, 88, 22, 19)
    mine(w, 86, 28, 22)
    mine(w, 78, 20, 19)
    girder(89, 32, 22, 89, 35, 22)                                    # the crane's mast

    # ---- the ledge L (walk 32), the shaft, the gallery (walk 48) ----
    carve(84, 32, 12, 94, 35, 18)
    carve(95, 32, 15, 97, 34, 16)                                     # a nook: a trap, an egg
    w.fill(95, 31, 15, 95, 31, 16, SLIME_TRAP)
    w.place(O.EGG, 97, 32, 15)
    # The shaft north of L: three lifts in a row (sixteen up), spikes two under their low stop.
    carve(88, 29, 11, 90, 50, 11)
    w.fill(88, 28, 11, 90, 28, 11, SLIME_TRAP)
    for x in (88, 89, 90):
        lift(w, (x, 31, 11), (x, 47, 11), ticks=96, stop=3)
    # The gallery west, two wide: striped poles and knobs in niches of its north wall, a trench
    # two deep with four pistons pumping in it (a fall: a jump back out at its ends), a row of
    # spike traps before the exit tower.
    carve(62, 48, 9, 92, 50, 10)
    for x in (66, 72, 87, 91):
        w.fill(x, 48, 8, x, 49, 8, POLE)
        w.set(x, 50, 8, KNOB)
    carve(76, 46, 9, 84, 47, 10)
    for k, x in enumerate((83, 81, 79, 77)):
        for z in (9, 10):
            if stand_ins:
                w.set(x, 47, z, 10)
            else:
                ys = (47, 46) if k % 2 == 0 else (46, 47)
                lift(w, (x, ys[0], z), (x, ys[1], z), ticks=6, stop=3)
    w.fill(68, 47, 9, 68, 47, 10, SLIME_TRAP)

    # ---- the exit tower ----
    # A tall room; a lift (four up) beside a metal core, arms round the core each two above the
    # last, a saw and a mine on the far ends of the first two; the red arrow in a cage of grates
    # under a girder roof at the top (walk 62).
    carve(52, 48, 4, 61, 66, 15)
    metal(56, 48, 9, 56, 60, 9)
    w.set(57, 47, 9, 0)
    lift(w, (57, 47, 9), (57, 51, 9), ticks=24, stop=3)
    metal(56, 53, 6, 57, 53, 8)                                       # 1 north, walk 54
    metal(53, 55, 8, 55, 55, 9)                                       # 2 west, walk 56
    metal(55, 57, 10, 56, 57, 12)                                     # 3 south, walk 58
    metal(57, 59, 9, 59, 59, 10)                                      # 4 east, walk 60
    w.set(57, 53, 6, SAW)
    mine(w, 56, 54, 6)
    w.set(53, 55, 8, SAW)
    w.rotate_plate(53, 55, 8)
    mine(w, 53, 56, 9)
    metal(60, 61, 8, 61, 61, 10)                                      # the cage's floor
    for z in (7, 11):
        w.fill(60, 62, z, 61, 63, z, GRATE)
    girder(59, 64, 7, 61, 64, 11)
    girder(56, 61, 9, 56, 65, 9)
    w.place(O.EXIT, 61, 62, 9)

    skin(w, rng)
    w.spawn = (46, 11, 90)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
