#!/usr/bin/env python3
"""Mission 074, the toy depot (worlds3d/world074.vwr): brief plans/briefs/world074.md.

Mobile Eggbert's level 074, all of the toy box mixed, under the region-22 sky: nine treasures,
four eggs, the red arrow at the top. Blupi starts between two skateboards on posts (here low
toy tables); rows of mines only a skateboard crosses; a stair down with lava in each step's gap; mines under warning signs;
bars over lava (leave the skateboard to hang); a stair of five treasures to the treasure door "5"
(425); two pits under bars; a relay of three lift pairs in step (40 ticks): the first rises, the
second comes down beside it, the third waits above where the second goes back up; an upper
corridor back with mines, a floor of lava under bars and rods with a treasure on a cell between
its pools; a shelf of skateboards with eggs above; the treasure door "7" (427); mines; a pool with
two treasures on pillars, to swim to; a second relay up to the arrow. Here the depot is a U of
toy halls round a pool court open to the sky: the skate street north, the lava stair east, the
treasure tower, the pits and the east relay up to the upper storey, which runs south and back
west to a balcony over the court; the pool; the west relay up to a pavilion with the arrow.

    python3 tools/worlds/world074.py                 # writes worlds3d/world074.vwr
    python3 tools/worlds/world074.py --stage K out   # for levelcheck:
        1: the mines gone (the skateboard ran them over), the bars taken away (levelcheck
           would walk on them): the route stops at the first bars
        2: the mines gone, the bars in place (levelcheck walks on them where Blupi hangs)
        3: also the door "5" open (five treasures taken)
        4: also the door "7" open (seven taken)
        5: also bridges over the pool to the pillars and the west bank (swimming)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from toys import (BALL_POLES, CANDY_POLES, CONE_BALLS, HALF_BALLS, LAVA, LOLLIPOPS, MIXED,  # noqa: E402
                  POLE_TILES, POLE_TOPS, WARNING, WATER, carve, toy)

BAR_Z = 138                   # a bar plate facing Z: for bars along Z
ROD_X = 202                   # a rod along X: for bars along X
DOOR5, DOOR7 = 420 + 5, 420 + 7   # treasure doors (Decor::OpenDoorsTresor)
W1 = 10                       # the start court and the skate street
WF = 7                        # the foot of the lava stair
WP = 12                       # the door "5" and the pits
W2 = 24                       # the upper storey
WB = 15                       # the pool court's banks
SPAWN = (20, W1, 78)
SKY = (215, 216, 217, 218, 219, 220)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world074.vwr"
    rng = random.Random(74)
    w = World(mission=74, sky=22)
    mines_on = stage == 0
    bars_on = stage != 1

    def mass(x0, y0, z0, x1, y1, z1):
        toy(w, x0, y0, z0, x1, y1, z1, rng, MIXED)

    def hall(x0, y0, z0, x1, y1, z1):
        """A toy hall: walls, floor and roof one thick round the inside x0..x1, y0..y1, z0..z1
        (y0 the walking level)."""
        mass(x0 - 1, y0 - 2, z0 - 1, x1 + 1, y1 + 1, z1 + 1)
        carve(w, x0, y0, z0, x1, y1, z1)

    def danger(x, y, z):
        if mines_on:
            mine(w, x, y, z)

    def bars(x0, y, z0, x1, z1, tile):
        if bars_on:
            w.fill(x0, y, z0, x1, y, z1, tile)

    def table(x0, z0, x1, z1, y, at):
        """A toy table one block high with a skateboard on it at `at` (x, z): the original's
        skateboards wait on posts a jump up; in 3D a post's one-cell top was a fiddly landing."""
        toy(w, x0, y, z0, x1, y, z1, rng, SKY)
        w.place(O.SKATEBOARD, at[0], y + 1, at[1])

    def stem(x, z, y0, y1):
        w.fill(x, y0, z, x, y1, z, rng.choice(POLE_TILES))

    # ---- A, the start court (walk 10), open to the sky ----
    mass(13, 6, 69, 27, W1 - 1, 83)
    for x in range(13, 28):
        for z in range(69, 84):
            if x in (13, 27) or z in (69, 83):
                mass(x, W1, z, x, W1 + 2, z)
    for x, z in ((13, 83), (27, 83), (13, 69), (27, 69)):
        stem(x, z, W1 + 3, W1 + 5)
        w.set(x, W1 + 6, z, rng.choice(POLE_TOPS))

    # ---- B, the skate street (x 19-21, walk 10) north, three high ----
    hall(19, W1, 32, 21, W1 + 2, 68)
    carve(w, 19, W1, 69, 21, W1 + 2, 69)                             # in from the court
    # Alcoves between the mine rows, a spare skateboard on a post in each.
    mass(15, W1 - 2, 52, 18, W1 + 5, 56)
    carve(w, 17, W1, 53, 18, W1 + 4, 55)
    mass(22, W1 - 2, 52, 25, W1 + 5, 56)
    carve(w, 22, W1, 53, 23, W1 + 4, 55)

    # ---- C, the lava stair east (z 32-34) and the foot corridor (walk 7) ----
    mass(22, 0, 31, 49, 13, 35)
    carve(w, 22, 3, 32, 48, 12, 34)
    # Steps three long, a gap of one between, the gap's lava level with the next step.
    mass(22, 0, 32, 24, W1 - 1, 34)                                 # step 0, walk 10
    mass(25, 0, 32, 25, W1 - 3, 34)
    w.fill(25, W1 - 2, 32, 25, W1 - 2, 34, LAVA)
    mass(26, 0, 32, 28, W1 - 2, 34)                                 # step 1, walk 9
    mass(29, 0, 32, 29, W1 - 4, 34)
    w.fill(29, W1 - 3, 32, 29, W1 - 3, 34, LAVA)
    mass(30, 0, 32, 32, W1 - 3, 34)                                 # step 2, walk 8
    mass(33, 0, 32, 33, WF - 2, 34)
    w.fill(33, WF - 1, 32, 33, WF - 1, 34, LAVA)
    mass(34, 0, 32, 48, WF - 1, 34)                                 # the foot corridor, walk 7
    carve(w, 43, 3, 32, 46, WF - 1, 34)                             # the pit, lava four down
    w.fill(43, 2, 32, 46, 2, 34, LAVA)

    # ---- D, the treasure tower (inside x 50-57, z 29-37): a stair of five round a newel ----
    mass(49, 0, 28, 58, 16, 38)
    carve(w, 50, WF, 29, 57, 15, 37)
    carve(w, 49, WF, 32, 49, WF + 2, 34)                            # in from the foot corridor
    mass(52, WF, 32, 54, 15, 34)                                    # the newel
    steps = [((50, 51, 29, 31), 8), ((52, 54, 29, 31), 9), ((55, 57, 29, 31), 10),
             ((55, 57, 32, 34), 11), ((55, 57, 35, 37), 12)]
    for (x0, x1, z0, z1), walk in steps:
        mass(x0, WF, z0, x1, walk - 1, z1)

    # ---- E, the pits (walk 12): two pits two wide, a cell of floor between ----
    mass(58, 2, 34, 71, 9, 38)
    hall(59, WP, 35, 71, WP + 3, 37)
    for x0 in (61, 64):
        carve(w, x0, 8, 35, x0 + 1, WP - 1, 37)
        w.fill(x0, 7, 35, x0 + 1, 7, 37, LAVA)

    # ---- F, the east relay shaft (inside x 72-75, z 36-37) ----
    mass(71, 0, 35, 76, 28, 38)

    # ---- G, the upper storey (walk 24): landing, corridor south, corner room, west corridor ----
    hall(71, W2, 38, 75, W2 + 3, 41)                                # L2
    hall(72, W2, 42, 74, W2 + 2, 61)
    carve(w, 72, W2, 41, 74, W2 + 2, 42)
    w.fill(72, W2 - 1, 49, 74, W2 - 1, 52, LAVA)                    # the lava floor
    w.fill(72, W2 - 1, 55, 74, W2 - 1, 58, LAVA)
    hall(71, W2, 62, 76, W2 + 6, 66)                                # the corner room, to y 30
    carve(w, 72, W2, 61, 74, W2 + 2, 61)
    mass(74, W2, 65, 76, W2 + 1, 66)                                # the shelf, walk 26
    mass(74, W2 + 2, 66, 76, W2 + 3, 66)                            # the eggs' ledge, walk 28
    hall(59, W2, 62, 69, W2 + 2, 64)                                # the west corridor
    for x, z in ((71, 45), (75, 45), (71, 55), (75, 55), (71, 66), (76, 62), (60, 61), (66, 65)):
        stem(x, z, 1, W2 - 3)                                       # posts under the storey

    # ---- H, the pool court (inside x 28-55, z 44-66), open to the sky ----
    mass(27, 2, 43, 56, WB - 1, 67)
    for x in range(27, 57):
        for z in range(43, 68):
            if x in (27, 56) or z in (43, 67):
                mass(x, WB, z, x, W2 + 3, z)
    mass(56, W2 - 2, 59, 59, W2 - 1, 67)                            # the balcony, walk 24
    carve(w, 56, W2, 60, 58, W2 + 3, 66)
    carve(w, 56, WB + 6, 60, 56, W2 - 3, 66)                        # the court's wall open below it
    carve(w, 59, W2, 62, 59, W2 + 2, 64)
    mass(52, WB, 60, 55, W2 - 4, 66)                                # terrace, walk 21
    mass(48, WB, 60, 51, W2 - 7, 66)                                # terrace, walk 18
    # The pool (x 34-45): water three deep, its top cell level with the banks' top (from the
    # surface Blupi jumps or swims out onto a block level with it, not one higher); two pillars
    # in it, their tops level with the water too.
    carve(w, 34, 12, 44, 45, WB - 1, 66)
    w.fill(34, 12, 44, 45, WB - 1, 66, WATER)
    for x, z in ((37, 50), (42, 58)):
        mass(x, 12, z, x, WB - 1, z)

    # ---- I, the west relay shaft (inside x 29-31, z 52-53) ----
    carve(w, 29, WB, 52, 31, 30, 53)
    mass(28, WB + 3, 52, 28, 30, 54)
    mass(32, WB + 3, 52, 32, 30, 54)
    mass(29, WB, 54, 31, 30, 54)
    w.fill(30, WB - 1, 52, 30, WB - 1, 53, 0)                       # D's cells in the bank

    # ---- J, the pavilion: a landing by F's top, two jumps west, the arrow under a dome ----
    def platform(x0, z0, x1, z1, walk):
        toy(w, x0, walk - 2, z0, x1, walk - 1, z1, rng, SKY)

    platform(29, 48, 31, 51, W2 + 3)                                # DL, walk 27
    stem(30, 48, WB, W2)
    platform(26, 48, 28, 51, W2 + 5)                                # walk 29
    platform(23, 48, 25, 51, W2 + 7)                                # walk 31
    platform(18, 47, 22, 52, W2 + 9)                                # walk 33
    stem(20, 50, 14, W2 + 6)
    toy(w, 17, W2 + 14, 46, 23, W2 + 14, 53, rng, SKY)              # the dome
    for x, z in ((18, 47), (22, 47), (18, 52), (22, 52)):
        w.fill(x, W2 + 12, z, x, W2 + 13, z, rng.choice(CANDY_POLES))

    # ---- openings, doors, bars and objects (after every mass) ----
    carve(w, 72, WP - 1, 36, 73, 27, 37)                            # the east shaft: A/C column
    carve(w, 74, WP, 36, 75, 27, 37)                                # B column, its floor walk 12
    carve(w, 71, WP, 36, 71, WP + 3, 37)                            # the pits' mouth into it
    carve(w, 72, W2, 37, 75, W2 + 3, 38)                            # C's top onto L2
    carve(w, 58, WP, 35, 58, WP + 1, 37)
    if stage < 3:
        w.fill(58, WP, 35, 58, WP + 1, 37, DOOR5)
    carve(w, 70, W2, 62, 70, W2 + 1, 64)
    if stage < 4:
        w.fill(70, W2, 62, 70, W2 + 1, 64, DOOR7)
    if stage >= 2:
        w.fill(72, W2 - 1, 49, 74, W2 - 1, 58, rng.choice(MIXED))   # levelcheck: a floor under the bars
    if stage >= 5:
        for x in range(34, 46):
            w.set(x, WB - 1, 50, rng.choice(MIXED))                 # levelcheck: bridges for swimming
        for z in range(51, 59):
            w.set(42, WB - 1, z, rng.choice(MIXED))
    bars(43, WF, 32, 47, 34, ROD_X)                                 # over the foot corridor's pit
    bars(61, WP, 35, 66, 37, ROD_X)                                 # over the two pits
    # Over the lava floor one above the walking level (jump up to catch them); none over the
    # island's first cell, where he drops for the treasure.
    bars(72, W2 + 1, 48, 74, 52, BAR_Z)
    bars(72, W2 + 1, 54, 74, 59, BAR_Z)
    # Warning signs (48 shows its picture on +Z only): a lintel of them across the street before
    # each row of mines, and in the north walls of the corridors along X.
    for z in (64, 47):
        w.fill(19, W1 + 2, z, 21, W1 + 2, z, WARNING)
    for x in (38, 39, 40):
        w.set(x, WF + 2, 31, WARNING)
    for x in (64, 65, 66):
        w.set(x, W2 + 1, 61, WARNING)

    table(17, 75, 18, 76, W1, (18, 76))                            # either side of the start
    table(22, 75, 23, 76, W1, (22, 76))
    table(17, 53, 17, 55, W1, (17, 54))                            # in the street's alcoves
    table(23, 53, 23, 55, W1, (23, 54))
    table(71, 40, 72, 41, W2, (71, 40))                            # on the landing L2
    table(74, 40, 75, 41, W2, (75, 40))
    w.place(O.SKATEBOARD, 74, W2 + 2, 65)
    w.place(O.SKATEBOARD, 76, W2 + 2, 65)
    for z in range(60, 64):                                         # the street: four deep
        for x in (19, 20, 21):
            danger(x, W1, z)
    for z in range(44, 47):                                         # three deep
        for x in (19, 20, 21):
            danger(x, W1, z)
    for x in (38, 39, 40):
        for z in (32, 33, 34):
            danger(x, WF, z)
    for z in (44, 45, 46):
        for x in (72, 73, 74):
            danger(x, W2, z)
    for x in (64, 65, 66):
        for z in (62, 63, 64):
            danger(x, W2, z)
    for (x0, x1, z0, z1), walk in steps:
        w.place(O.TREASURE, (x0 + x1) // 2, walk, (z0 + z1) // 2)
    w.place(O.TREASURE, 73, W2, 53)                                 # on the island
    w.place(O.TREASURE, 73, W2, 61)
    for x, z in ((37, 50), (42, 58)):
        w.place(O.TREASURE, x, WB, z)                               # on the pillars
    for x in (74, 75, 76):
        w.place(O.EGG, x, W2 + 4, 66)
    w.place(O.EGG, 29, WB, 45)
    for x in (72, 73, 74, 75):                                      # the east relay
        for z in (36, 37):
            if x < 74:
                lift(w, (x, WP - 1, z), (x, WP + 3, z), ticks=40, stop=40)      # A
                lift(w, (x, WP + 7, z), (x, WP + 11, z), ticks=40, stop=40)     # C
            else:
                lift(w, (x, WP + 7, z), (x, WP + 3, z), ticks=40, stop=40)      # B
    for z in (52, 53):                                              # the west relay
        lift(w, (30, WB - 1, z), (30, WB + 3, z), ticks=40, stop=40)            # D
        lift(w, (30, WB + 7, z), (30, WB + 11, z), ticks=40, stop=40)           # F
        for x in (29, 31):
            lift(w, (x, WB + 7, z), (x, WB + 3, z), ticks=40, stop=40)          # E
    w.place(O.EXIT, 19, W2 + 9, 49)
    for icon, x, y, z in ((LOLLIPOPS[0], 15, W1, 81), (HALF_BALLS[0], 25, W1, 81), (CONE_BALLS[1], 15, W1, 72),
                          (BALL_POLES[0], 50, WB, 46), (CONE_BALLS[0], 53, WB, 50), (HALF_BALLS[1], 30, WB, 63),
                          (LOLLIPOPS[2], 48, WB, 56)):
        w.bigdecor(icon, x, y, z)

    w.spawn = SPAWN
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
