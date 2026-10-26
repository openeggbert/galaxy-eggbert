#!/usr/bin/env python3
"""Mission 053, the foundry of stars (worlds3d/world053.vwr): brief plans/briefs/world053.md.

Mobile Eggbert's level 053 is world 5's long one, 21 treasures through a foundry over lava: a
hall of girder towers where follow stars sleep, a lift step; two mines bobbing in a corridor; a
lava pool crossed on two lifts that meet in the middle under sleeping stars; a lift over a pit
of wasps under hanging mines; a shaft climbed in the balloon a wasp gives, past hanging mines;
a vault of thirteen treasures, each with its stars; the treasure door "13" onto a corridor of
eleven stars (the drink beside it: unseen, they do not wake) to a treasure; the door "14" onto
a crusher, a lava slope, gates of mines bobbing in pairs, a row of seven treasures under two
columns of stars, a lift up to the last corridor and the red arrow in a pit of stars. Here the
same order in 3D: the hall in the south, the corridor, the lava crossing and the wasp pit
heading north; the balloon shaft up into the vault above them; the door "13" to the north
corridor, the door "14" east to the slope down to the treasure hall, the lift to the
corridor and the cage of the arrow.

    python3 tools/worlds/world053.py                   # writes worlds3d/world053.vwr
    python3 tools/worlds/world053.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which
        counts no treasures and has no balloon, crusher or star luring):
        1 the bobbing mines timed (left out), 2 the balloon ridden (a stand-in lift), 3 the door "13"
        open, 4 the corridor crossed unseen (its stars left out), 5 the door "14" open, 6 the
        crushers waited out (plates), 7 every star lured away or burst (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine, rect  # noqa: E402
from station import (BALL_MACHINES, GIRDER, GRILLE, LAVA, girder_mast, metal,  # noqa: E402
                     treasure_door)

CRUSHER = 317
RED_DOT = 3                  # the plate with the red light (Mobile Eggbert's machine tile 3)
SPAWN = (26, 11, 91)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(53)
    w = World(mission=53, sky=1)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def star(x, y, z):
        if stage < 7:                    # stage 7: every star lured away
            w.place(O.FOLLOW_STAR, x, y, z)

    def patrol(otype, a, b, ticks, stop):
        w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def walls(x0, z0, x1, z1, y0, y1, south=None):
        """Plate walls round x0..x1, z0..z1 from y0 to y1 (the south one to `south` if given)."""
        plates(x0, y0, z0, x1, y1, z0)
        plates(x0, y0, z1, x1, south if south is not None else y1, z1)
        plates(x0, y0, z0, x0, y1, z1)
        plates(x1, y0, z0, x1, y1, z1)

    # ---- A, the start hall (inside x 21-31, z 82-93, walk 11) ---------------------------------
    plates(20, 8, 81, 32, 10, 94)
    walls(20, 81, 32, 94, 11, 17, south=12)
    w.spawn = SPAWN
    # Four ball machines on plinths and four girder masts; a star asleep beside each mast, two
    #    above the floor: it wakes when Blupi comes within two cells and homes on him, slowly,
    #    until it meets something solid (a mast).
    for k, (x, z) in enumerate(((22, 84), (30, 84), (22, 91), (30, 91))):
        w.set(x, 11, z, 82)
        w.bigdecor(BALL_MACHINES[2 if k < 3 else 0], x, 12, z)
    for x, z in ((24, 86), (28, 86), (24, 89), (28, 89)):
        girder_mast(w, x, z, 11, 16)
    for x, z in ((23, 86), (29, 86), (23, 89), (29, 89)):
        star(x, 11, z)
    w.place(O.EGG, 21, 11, 93)
    w.place(O.EGG, 31, 11, 82)
    # The ledge by the north wall (x 25-27, z 82-83, walk 14) and the lift up to it: three cells
    #    at ten ticks a cell, the original's pace; the doorway north (two high).
    plates(25, 11, 82, 27, 13, 83)
    w.clear(26, 10, 84)
    lift(w, (26, 10, 84), (26, 13, 84), ticks=30, stop=10)
    carve(25, 14, 81, 27, 15, 81)

    # ---- B, the corridor of the bobbing mines (x 25-27, z 67-80, walk 14) ----------------------
    plates(24, 11, 67, 28, 13, 80)
    plates(24, 14, 67, 24, 17, 80)
    plates(28, 14, 67, 28, 17, 80)
    for z, up in ((76, False), (71, True)):
        for x in (25, 27):                          # a gate one cell wide
            w.fill(x, 14, z, x, 17, z, RED_DOT if x == 25 else 82)
        w.set(26, 18, z, GRILLE)
        if stage < 1:                               # a mine riding up and down in the gap
            lo, hi = (26, 14, z), (26, 16, z)
            patrol(O.MOVING_MINE, hi if up else lo, lo if up else hi, 20, 0)

    # ---- C, the lava crossing (x 18-34, z 46-66) ----------------------------------------------
    plates(17, 10, 46, 35, 12, 66)
    plates(17, 13, 62, 35, 13, 66)                   # the south bank, walk 14
    plates(17, 13, 46, 35, 13, 50)                   # the north bank
    w.fill(18, 13, 51, 34, 13, 61, LAVA)             # the lava, level with the banks
    carve(25, 13, 51, 26, 13, 61)                    # the lifts' lane, lava one lower
    w.fill(25, 12, 51, 26, 12, 61, LAVA)
    for x in (25, 26):
        # Lift A slides from the south bank four cells north, lift B from the north bank five
        #    cells south, in the same time: they meet side by side. The original's 24 ticks and
        #    3-tick stop ask for a step across in 0.15 s, a 2D jump; in 3D they take 36 ticks and
        #    stop 12, long enough to walk across while they touch.
        lift(w, (x, 13, 61), (x, 13, 57), ticks=36, stop=12)
        lift(w, (x, 13, 51), (x, 13, 56), ticks=36, stop=12)
    for z in (52, 54, 56, 58, 60):                   # stars asleep two above the crossing
        star(25 if z % 4 else 26, 16, z)
    plates(17, 14, 46, 17, 16, 66)                   # low walls on the pool's sides
    plates(35, 14, 46, 35, 16, 66)
    w.place(O.EGG, 18, 14, 47)
    w.place(O.EGG, 34, 14, 47)
    carve(25, 14, 66, 27, 15, 66)                    # from the corridor

    # ---- D, the wasp pit and its ferry (pit x 18-34, z 40-45) ---------------------------------
    plates(17, 4, 39, 35, 6, 45)                     # the pit's floor, walk 8 (top y 7 below)
    w.fill(18, 7, 40, 34, 12, 45, 0)
    plates(17, 7, 40, 17, 13, 45)
    plates(35, 7, 40, 35, 13, 45)
    plates(17, 7, 39, 35, 13, 39)                    # under the north bank
    plates(18, 7, 40, 34, 7, 45)
    for x, z in ((19, 41), (21, 44), (23, 42), (28, 43), (30, 41), (32, 44), (26, 41)):
        patrol(O.WASP, (x, 8, z), (x + 1, 8, z), 6, 4)
    # Steps out of the pit at its west end, back onto the south bank (walk 8 -> 10 -> 12 -> 14).
    plates(18, 7, 44, 19, 9, 44)
    plates(18, 7, 45, 19, 11, 45)
    # The ferry: two lifts in step from the south bank's edge across to the north bank (30 ticks
    #    for five cells, stop 3); six mines hang two above its path from a girder gantry, so a
    #    jump on it meets them.
    for x in (25, 26):
        lift(w, (x, 13, 45), (x, 13, 40), ticks=30, stop=3)
    for x in (24, 27):
        for z in (46, 39):
            girder_mast(w, x, z, 14, 16)
    for x in range(24, 28):
        for z in range(39, 47):
            w.set(x, 17, z, GIRDER)
    for x, z in ((25, 41), (26, 42), (25, 43), (26, 44), (25, 45), (26, 40)):
        mine(w, x, 16, z, hanging=True)
    # The north bank (x 18-34, z 31-38, walk 14), an egg in its corner.
    plates(17, 10, 31, 35, 13, 38)
    plates(17, 14, 31, 17, 16, 38)
    plates(35, 14, 31, 35, 16, 38)
    w.place(O.EGG, 34, 14, 32)

    # ---- E, the balloon shaft (inside x 23-27, z 25-29, walk 14 -> the vault's floor, 26) ------
    VAULT = 25                                       # the vault floor's top: walk 26
    plates(22, 10, 24, 28, 13, 30)
    walls(22, 24, 28, 30, 14, VAULT)
    carve(24, 14, 30, 26, 15, 30)                    # its doorway from the north bank
    # Two wasps sweep across its foot (12 ticks, stop 4): their touch puts Blupi in a balloon
    #    that rises (faster with the jump held) and drifts where he walks, for about ten seconds.
    patrol(O.WASP, (23, 14, 26), (27, 14, 26), 12, 4)
    patrol(O.WASP, (27, 14, 28), (23, 14, 28), 12, 4)
    patrol(O.WASP, (25, 21, 25), (25, 21, 29), 12, 4)
    # Hanging mines pop the balloon: two in corners halfway up, and in the middle near the top a
    #    block with a mine on it and one hanging under it.
    mine(w, 23, 19, 25, hanging=True)
    mine(w, 27, 19, 29, hanging=True)
    w.set(25, 22, 27, 84)
    mine(w, 25, 23, 27)
    mine(w, 25, 21, 27, hanging=True)
    if stage >= 2:                       # levelcheck has no balloon: a stand-in lift up
        w.place(O.LIFT, 25, 13, 31, end=(25, VAULT, 23))

    # ---- F, the vault (inside x 15-57, z 5-33, walk 26), round the shaft's top ----------------
    plates(14, VAULT - 1, 4, 58, VAULT, 34)
    carve(23, VAULT - 1, 25, 27, VAULT, 29)          # the shaft comes up through its floor
    walls(14, 4, 58, 34, VAULT + 1, VAULT + 11)
    # The dais in the middle: three tiers, a jump each (walk 28, 30, 32).
    plates(33, VAULT + 1, 11, 47, VAULT + 2, 25)
    plates(36, VAULT + 3, 14, 44, VAULT + 4, 22)
    plates(39, VAULT + 5, 17, 41, VAULT + 6, 19)
    for x, y, z in ((33, 28, 11), (47, 28, 25), (33, 28, 25), (47, 28, 11),
                    (36, 30, 14), (44, 30, 22), (40, 32, 18)):
        w.place(O.TREASURE, x, y, z)
    for x, y, z in ((34, 28, 12), (46, 28, 24), (32, 28, 26), (48, 28, 10), (37, 30, 15),
                    (43, 30, 21), (40, 32, 16), (40, 32, 20), (35, 28, 25), (45, 28, 11)):
        star(x, y, z)
    # Four alcoves in the walls, two west, two south, a treasure in each and a star asleep at
    #    its mouth.
    for x, z, dz, egg in ((34, 34, 1, False), (50, 34, 1, True)):
        zz = z + dz
        plates(x - 2, VAULT - 1, min(z, zz + dz), x + 2, VAULT + 4, max(z, zz + dz))
        carve(x - 1, VAULT + 1, min(z, zz), x + 1, VAULT + 3, max(z, zz))
        w.place(O.TREASURE, x, VAULT + 1, zz)
        if egg:
            w.place(O.EGG, x + 1, VAULT + 1, zz)
        star(x, VAULT + 1, z - dz)
    for z, egg in ((10, False), (19, True)):
        plates(12, VAULT - 1, z - 2, 14, VAULT + 4, z + 2)
        carve(13, VAULT + 1, z - 1, 14, VAULT + 3, z + 1)
        w.place(O.TREASURE, 13, VAULT + 1, z)
        if egg:
            w.place(O.EGG, 13, VAULT + 1, z + 1)
        star(15, VAULT + 1, z)
    # The gallery along the east wall (x 52-57, z 5-33, walk 30), a step (walk 28) up to it;
    #    two treasures and four eggs on it, two stars.
    plates(52, VAULT + 1, 5, 57, VAULT + 4, 33)
    plates(50, VAULT + 1, 28, 51, VAULT + 2, 31)
    w.place(O.TREASURE, 56, 30, 8)
    w.place(O.TREASURE, 56, 30, 30)
    for z in (13, 15, 23, 25):
        w.place(O.EGG, 57, 30, z)
    star(54, 30, 9)
    star(54, 30, 29)
    w.bigdecor(BALL_MACHINES[1], 57, 30, 19)

    # ---- G, the door "13", the drink and the corridor of stars (x 15-57, z 1-3, walk 30) --------
    carve(53, 30, 4, 55, 31, 4)
    for x in (53, 54, 55):
        for y in (30, 31):
            w.set(x, y, 4, 0 if stage >= 3 else treasure_door(13))
        w.set(x, 32, 4, GRILLE)
    plates(14, 26, 0, 58, 29, 4)
    plates(14, 30, 0, 58, 33, 0)
    plates(14, 30, 0, 14, 33, 4)
    plates(58, 30, 0, 58, 33, 4)
    plates(14, 34, 0, 58, 34, 4)
    for x in range(15, 58):
        for z in (1, 2, 3):
            for y in (30, 31, 32, 33):
                if w.get(x, y, z) and z < 4:
                    w.clear(x, y, z)
    # The drink at the corridor's east end, beside the door.
    w.place(O.DRINK, 56, 30, 2)
    # Four curtains of stars asleep across all three lanes at Blupi's height (the last one two),
    #    the fourteenth treasure at the corridor's west end: unseen (the drink: twenty seconds)
    #    Blupi walks through them and they sleep on; seen, they wake as he comes near.
    if stage < 4:
        for x, lanes in ((48, (1, 2, 3)), (39, (1, 2, 3)), (30, (1, 2, 3)), (21, (1, 3))):
            for z in lanes:
                star(x, 30, z)
    w.place(O.TREASURE, 16, 30, 2)
    w.set(15, 33, 1, RED_DOT)
    w.set(15, 33, 3, RED_DOT)

    # ---- H, the door "14" and the way down to the treasure hall --------------------------------
    for z in (18, 19, 20):
        for y in (30, 31):
            w.set(58, y, z, 0 if stage >= 5 else treasure_door(14))
        w.set(58, 32, z, GRILLE)
    # The crusher passage (x 59-69, z 18-20, walk 30), narrowed to one cell at x 64 where a
    #    crusher waits in the floor: squashed, Blupi cannot jump for a while.
    plates(59, 26, 17, 69, 29, 21)
    plates(59, 30, 17, 69, 33, 17)
    plates(59, 30, 21, 69, 33, 21)
    for z in (18, 20):
        w.fill(64, 30, z, 64, 33, z, 82)
    w.set(64, 29, 19, 82 if stage >= 6 else CRUSHER)
    # The lava slope (x 70-74, z 18-44): terraces heading south, each two below the last, a strip
    #    of lava across each one to jump.
    walk = 30
    for k, z0 in enumerate(range(18, 43, 5)):
        top = walk - 2 * k - 1
        plates(70, top - 3, z0, 74, top, z0 + 4)
        if k > 0:                                   # two cells to land, then the strip
            w.fill(70, top, z0 + 3, 74, top, z0 + 3, LAVA)
        plates(69, top + 1, z0, 69, top + 4, z0 + 4)
        plates(75, top + 1, z0, 75, top + 4, z0 + 4)
    carve(69, 30, 18, 69, 31, 20)
    # Its foot (walk 20) runs into three gates of two mines riding up and down in opposite
    #    phase (20 ticks, the original's): pass on the side whose mine is up.
    plates(70, 15, 43, 74, 19, 57)
    plates(69, 20, 43, 69, 24, 57)
    plates(75, 20, 43, 75, 24, 57)
    for z in (47, 51, 55):
        for x in (70, 73, 74):
            w.fill(x, 20, z, x, 24, z, 82)
        w.fill(71, 24, z, 72, 24, z, GRILLE)
        if stage < 1:
            patrol(O.MOVING_MINE, (71, 20, z), (71, 22, z), 20, 0)
            patrol(O.MOVING_MINE, (72, 22, z), (72, 20, z), 20, 0)
    # The treasure hall (inside x 60-82, z 58-66, walk 20): crushers in its doorway, seven
    #    treasures in a row, two columns of four stars stacked over the row.
    plates(59, 15, 57, 83, 19, 67)
    walls(59, 57, 83, 67, 20, 28)
    carve(71, 20, 57, 73, 22, 57)
    for x in (71, 72, 73):                          # two deep: no jump clears them
        for z in (58, 59):
            w.set(x, 19, z, 82 if stage >= 6 else CRUSHER)
    for x in (64, 66, 68, 70, 72, 74, 76):
        w.place(O.TREASURE, x, 20, 62)
    for x in (67, 73):
        for y in (20, 21, 22, 23):
            star(x, y, 62)
    w.bigdecor(BALL_MACHINES[2], 61, 20, 59)
    w.bigdecor(BALL_MACHINES[2], 61, 20, 65)
    # The lift at its east end, seven cells up (42 ticks, the original's) to the ledge (x 84-88,
    #    walk 27) with two eggs.
    w.clear(82, 19, 62)
    lift(w, (82, 19, 62), (82, 26, 62), ticks=42, stop=3)
    plates(83, 15, 57, 89, 26, 67)
    carve(83, 20, 61, 83, 22, 63)
    carve(83, 27, 58, 88, 29, 66)
    carve(82, 27, 61, 82, 28, 63)
    plates(83, 27, 57, 89, 30, 57)
    plates(89, 27, 57, 89, 30, 67)
    plates(83, 27, 67, 89, 30, 67)
    plates(83, 30, 58, 88, 30, 66)
    w.place(O.EGG, 88, 27, 58)
    w.place(O.EGG, 88, 27, 66)

    # ---- The last corridor and the cage of the red arrow ---------------------------------------
    # South from the ledge (x 85-87, z 68-80, walk 27): two mines and three stars.
    carve(85, 27, 67, 87, 28, 67)
    plates(84, 22, 68, 88, 26, 80)
    plates(84, 27, 68, 84, 30, 80)
    plates(88, 27, 68, 88, 30, 80)
    mine(w, 85, 27, 70)
    mine(w, 87, 27, 77)
    for z in (70, 76, 79):
        star(86, 27, z)
    # The antechamber (x 79-87, z 81-83, walk 27) at the corridor's end, and the cage of the
    #    arrow south of it (inside x 78-86, z 85-91), entered by a doorway one cell wide at x 82:
    #    twenty stars sleep round the arrow. From the doorway Blupi wakes the nearest; they home on
    #    him, he steps aside, and they burst against the doorway's jambs, a few at a time.
    plates(77, 22, 80, 88, 26, 92)
    carve(79, 27, 81, 87, 29, 83)
    plates(77, 27, 80, 88, 30, 80)
    plates(78, 27, 80, 78, 30, 84)
    plates(88, 27, 80, 88, 30, 84)
    plates(78, 27, 84, 88, 30, 84)
    carve(85, 27, 80, 87, 28, 80)
    walls(77, 84, 87, 92, 27, 31)
    carve(82, 27, 84, 82, 28, 84)
    w.place(O.EXIT, 82, 27, 88)
    if stage < 7:
        for x, z in ((80, 86), (81, 86), (82, 86), (83, 86), (84, 86), (80, 87), (84, 87),
                     (80, 88), (84, 88), (80, 89), (84, 89), (80, 90), (81, 90), (82, 90),
                     (83, 90), (84, 90)):
            star(x, 27, z)
        for x, z in ((81, 87), (83, 87), (81, 89), (83, 89)):
            star(x, 29, z)
    for x, z in ((78, 85), (86, 85), (78, 91), (86, 91)):
        w.place(O.EGG, x, 27, z)
    w.set(79, 31, 92, RED_DOT)
    w.set(85, 31, 92, RED_DOT)

    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world053.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
