#!/usr/bin/env python3
"""Mission 075, the golden key of the toys (worlds3d/world075.vwr): brief plans/briefs/world075.md.

Mobile Eggbert's level 075, the last of world 7, under the region-15 sky, ends at the golden key
(ObjectType 21), which counts only once all seven treasures are taken. Two masses of toys: green
marble balls (218) above, orange balls (217) below. Blupi starts at the foot of a hill with mines
on its steps and a hanging mine; two shuttles meet over a gap; five skateboards by a row of mines
with three eggs beyond; a raft of three lifts over lava; a lava strip crossed on three small
shuttles with a mine on each cell between; a long fall down a shaft; four skateboards at its foot,
humps covered in mines; seven treasures in a funnel with two moving mines sliding over it, an egg;
a relay of five lifts climbing a column, each meeting the next; bars over a gap; four skateboards;
a corridor of mines in groups between pillars, ridden to the golden key. Here the upper part is a
walled walkway on a plateau of marble balls, the shaft drops into a hall of orange balls whose
course runs south over the humps to the funnel room, the relay climbs a column in the south-west
to a tube of orange balls that runs west over the bars to the skate shed and north to the key.

    python3 tools/worlds/world075.py                 # writes worlds3d/world075.vwr
    python3 tools/worlds/world075.py --stage K out   # for flightcheck --mine-margin 0:
        1: the skateboards' mine fields gone (run over), the bars taken away (levelcheck would
           walk on them): the route stops at the bars
        2: the mine fields gone, the bars in place (levelcheck walks on them where Blupi hangs)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from toys import (CONE_BALLS, GRATE, GIRDER, HALF_BALLS, LAVA, LOLLIPOPS, POLE_TILES, POLE_TOPS,  # noqa: E402
                  WATER, carve, toy)

MARBLE = 218            # the upper mass
ORANGE = 217            # the lower mass
STRIPES = 222
ROD_X = 202             # a rod along X
WP = 40                 # the plateau's walking level
WH = 34                 # the hill's foot
WB = 10                 # the basin's
WW = 25                 # the west level's
SPAWN = (13, WH, 44)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world075.vwr"
    rng = random.Random(75)
    w = World(mission=75, sky=15)
    fields_on = stage == 0
    bars_on = stage != 1

    def marble(x0, y0, z0, x1, y1, z1):
        toy(w, x0, y0, z0, x1, y1, z1, rng, (MARBLE, MARBLE, MARBLE, STRIPES))

    def orange(x0, y0, z0, x1, y1, z1):
        toy(w, x0, y0, z0, x1, y1, z1, rng, (ORANGE, ORANGE, ORANGE, 221))

    def field(x0, z0, x1, z1, y):
        """A skateboard's mine field (two deep: no jump clears it)."""
        if fields_on:
            for x in range(x0, x1 + 1):
                for z in range(z0, z1 + 1):
                    mine(w, x, y, z)

    # ================= the plateau (walk 40), open to the sky =================
    # Walkways: a floor slab and walls three high round each; every shell first, then every
    # inside, so walkways that touch open into each other.
    walkways = [
        (10, 26, 16, 46, WH - 1, 28),       # A, the hill (its foot walk 34; steps added below)
        (10, 6, 16, 25, WP - 1, 28),        # B, the canal (the deck and the north bank)
        (17, 6, 26, 12, WP - 1, 34),        # C, the yard
        (27, 9, 44, 11, WP - 1, 34),        #    the mine corridor
        (45, 9, 56, 11, WP - 1, 34),        # D, the lava pond
        (57, 9, 76, 11, WP - 1, 34),        # E, the lava alley
    ]
    for x0, z0, x1, z1, top, bottom in walkways:
        marble(x0 - 1, bottom, z0 - 1, x1 + 1, WP + 2, z1 + 1)
    for x0, z0, x1, z1, top, bottom in walkways:
        carve(w, x0, top + 1, z0, x1, WP + 2, z1)

    # ---- A, the hill (x 10-16): the foot (walk 34), two steps, the top (walk 40) ----
    marble(10, WH, 26, 16, WP - 1, 37)                              # the steps' mass
    carve(w, 10, 36, 35, 16, WP - 1, 37)                            # step 1, walk 36
    carve(w, 10, 38, 32, 16, WP - 1, 34)                            # step 2, walk 38
    for x in (10, 11, 14, 15):                                      # step 1's landing row
        mine(w, x, 36, 37)
    for x in (12, 13, 16):                                          # step 2's
        mine(w, x, 38, 34)
    for x in (12, 13, 14, 15):                                      # over step 2's takeoff row,
        mine(w, x, 39, 32, hanging=True)                            # under a beam: walk, don't jump
        w.set(x, 41, 32, STRIPES)
    for icon, x, z in ((HALF_BALLS[0], 11, 45), (CONE_BALLS[0], 15, 45)):
        w.bigdecor(icon, x, WH, z)

    # ---- B, the canal: a gap of twelve over water, two shuttles meeting in its middle ----
    carve(w, 10, 33, 14, 16, WP - 1, 25)
    w.fill(10, 33, 14, 16, 37, 24, WATER)                           # water to y 37
    marble(10, 33, 25, 16, 37, 25)                                  # its bank (top y 37, walk 38)
    for x in (12, 13, 14):
        lift(w, (x, WP - 1, 25), (x, WP - 1, 20), ticks=60, stop=40)   # S1 from the south deck
        lift(w, (x, WP - 1, 14), (x, WP - 1, 19), ticks=60, stop=40)   # S2 from the north bank

    # ---- C, the yard (x 10-26, z 6-12) and the mine corridor east (z 9-11) ----
    for x in (19, 21, 23, 25, 26):
        w.place(O.SKATEBOARD, x, WP, 7)
    field(31, 9, 32, 11, WP)
    # The eggs' ledge (walk 42) in an alcove past the mines, the walls raised round it so its
    # top is no step onto the walls.
    marble(33, WP + 3, 8, 34, WP + 5, 8)
    marble(38, WP + 3, 8, 39, WP + 5, 8)
    marble(33, WP + 3, 12, 39, WP + 5, 12)
    marble(34, WP, 7, 38, WP + 5, 7)
    carve(w, 35, WP, 8, 37, WP + 2, 8)
    marble(35, WP, 8, 37, WP + 1, 8)
    for x in (35, 36, 37):
        w.place(O.EGG, x, WP + 2, 8)

    # ---- D, the lava pond: a raft of three lifts in step, eight cells over lava ----
    carve(w, 46, WP - 1, 9, 54, WP - 1, 11)
    w.fill(46, WP - 2, 9, 54, WP - 2, 11, LAVA)
    for z in (9, 10, 11):
        lift(w, (46, WP - 1, z), (54, WP - 1, z), ticks=80, stop=40)   # orig. 80/20

    # ---- E, the lava alley: three pits, a small shuttle over each, a mine between ----
    for x0, lane in ((59, 9), (64, 11), (69, 9)):
        carve(w, x0, WP - 1, 9, x0 + 2, WP - 1, 11)
        w.fill(x0, WP - 2, 9, x0 + 2, WP - 2, 11, LAVA)
        lift(w, (x0, WP - 1, lane), (x0 + 2, WP - 1, lane), ticks=40, stop=40)   # orig. 40/20
        mine(w, x0 + 3, WP, 10)

    # ---- F, the shaft: thirty down into the basin, walled up to the alley's walls ----
    marble(76, 16, 8, 80, WP + 2, 12)
    carve(w, 77, 10, 9, 79, WP + 2, 11)
    carve(w, 76, WP, 9, 76, WP + 2, 11)
    for x, z in ((76, 8), (80, 8), (76, 12), (80, 12)):
        w.set(x, WP + 3, z, rng.choice(POLE_TOPS))

    # ================= the basin (walk 10), a hall of orange balls =================
    orange(71, 4, 8, 89, 16, 63)
    carve(w, 72, WB, 9, 88, 15, 62)
    carve(w, 77, 16, 9, 79, 16, 11)                                 # the shaft through the roof
    # The garage at the shaft's foot (z 9-16): four skateboards.
    for x in (73, 75, 85, 87):
        w.place(O.SKATEBOARD, x, WB, 14)
    # G, the course south (x 78-82, z 17-44): mine fields, humps one high to hop.
    orange(72, WB, 17, 77, 15, 44)
    orange(83, WB, 17, 88, 15, 44)
    field(78, 20, 82, 21, WB)
    orange(78, WB, 24, 82, WB, 25)                                  # hump, walk 11
    field(78, 28, 82, 29, WB)
    orange(78, WB, 33, 82, WB, 34)
    # H, the funnel room (z 45-62): a square funnel four deep, seven treasures on its rings.
    for k, (x0, z0, x1, z1) in enumerate(((75, 48, 85, 58), (76, 49, 84, 57), (77, 50, 83, 56),
                                          (78, 51, 82, 55))):
        carve(w, x0, WB - 1 - k, z0, x1, WB - 1, z1)
    for c in ((80, 9, 48), (80, 9, 58), (76, 8, 53), (84, 8, 53), (80, 7, 50), (80, 7, 56), (80, 6, 53)):
        w.place(O.TREASURE, *c)
    w.place(O.MOVING_MINE, 77, 9, 48, end=(83, 9, 48), advance=40, recede=40, stop_start=0, stop_end=0)
    w.place(O.MOVING_MINE, 83, 9, 58, end=(77, 9, 58), advance=40, recede=40, stop_start=0, stop_end=0)
    orange(86, WB, 60, 88, WB + 1, 62)                              # the egg's ledge, walk 12
    w.place(O.EGG, 87, WB + 2, 61)
    for x, z in ((72, 45), (88, 45)):                               # grates on high plinths
        orange(x, WB, z, x, WB + 3, z)
        w.fill(x, WB + 4, z, x, 15, z, GRATE if x < 80 else GIRDER)

    # ---- the way west (walk 10) to the relay ----
    orange(47, 4, 59, 71, 13, 63)
    carve(w, 48, WB, 60, 71, 12, 62)

    # ---- I, the relay: five lifts in two columns, each meeting the next (30 ticks) ----
    # A (x 46) rises 9-12, 15-18, 21-24; B (x 47) comes down 15-12 and 21-18: A1 meets B1 at
    # the top, B1 goes back up to A2, A2 meets B2, B2 back up to A3, A3 up to the west level.
    orange(44, 4, 59, 48, 29, 63)
    carve(w, 46, WB, 60, 47, 27, 62)
    carve(w, 48, WB, 60, 48, 12, 62)
    carve(w, 46, WB - 1, 60, 46, WB - 1, 61)
    for z in (60, 61):
        lift(w, (46, 9, z), (46, 12, z), ticks=30, stop=40)          # A1 (orig. 30/10)
        lift(w, (47, 15, z), (47, 12, z), ticks=30, stop=40)         # B1
        lift(w, (46, 15, z), (46, 18, z), ticks=30, stop=40)         # A2
        lift(w, (47, 21, z), (47, 18, z), ticks=30, stop=40)         # B2
        lift(w, (46, 21, z), (46, 24, z), ticks=30, stop=40)         # A3

    # ================= the west level (walk 25), a tube of orange balls =================
    orange(29, 19, 59, 45, 29, 63)
    carve(w, 30, WW, 60, 45, 27, 62)                                # the landing and the bars' way
    carve(w, 34, 21, 60, 38, WW - 1, 62)                            # the gap, lava four below
    w.fill(34, 20, 60, 38, 20, 62, LAVA)
    if bars_on:
        w.fill(33, WW, 60, 39, WW, 62, ROD_X)                       # rods along X over it
    # The skate shed (x 22-29, z 57-64): four skateboards.
    orange(21, 19, 56, 30, 29, 65)
    carve(w, 22, WW, 57, 29, 28, 64)
    carve(w, 30, WW, 60, 30, 27, 62)
    for x in (23, 25, 27, 29):
        w.place(O.SKATEBOARD, x, WW, 64)
    w.bigdecor(LOLLIPOPS[1], 22, WW, 57)
    # The key corridor north (x 24-26, z 21-56): four mine fields between striped pilasters.
    orange(23, 19, 16, 27, 28, 56)
    carve(w, 24, WW, 21, 26, 27, 57)
    for z0 in (50, 43, 36, 29):
        field(24, z0, 26, z0 + 1, WW)
    for z in (47, 40, 33):
        for x in (23, 27):
            w.fill(x, WW, z, x, 27, z, rng.choice(POLE_TILES))
    # The key's room (x 22-28, z 17-20) under striped pillars.
    orange(21, 19, 15, 29, 29, 21)
    carve(w, 22, WW, 17, 28, 28, 20)
    carve(w, 24, WW, 21, 26, 27, 21)
    for x, z in ((22, 17), (28, 17), (22, 20), (28, 20)):
        w.fill(x, WW, z, x, 28, z, rng.choice(POLE_TILES))
    w.place(O.SECRET_EXIT, 25, WW, 18)

    w.spawn = SPAWN
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
