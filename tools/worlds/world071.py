#!/usr/bin/env python3
"""Mission 071, the jeep and the toy tower (worlds3d/world071.vwr): brief plans/briefs/world071.md.

Mobile Eggbert's level 071, the first of the toy world, under the region-12 sky, no treasure, eight
eggs. Blupi starts by a row of six mines between two warning signs; the jeep waits at the top of a
zigzag shaft of green marble balls, its ledges strewn with mines and hung with hanging mines: he
climbs to it on foot, drives it down over the ledges' mines and over the row (the jeep runs mines
over), and leaves it where a lift in a frame of blue pipes crosses a gap. On foot: a ramp up
among mines floating in the air, an egg; pole tops to jump from one to the next under balls hung
with mines; bars over lava to hang from; a garden of every toy with two mines and an egg; a long
fall down a shaft between striped poles to the floor of a forest of poles; lifts carrying him
sideways over gaps, then up a lattice of star platforms on striped poles with lifts, bars over
lava, eggs, to the red arrow at its top. Here the shaft is a tower of toy balls with a stair of
ledges round its faces, the row of mines fills a cleft through a toy cliff, and the forest of
poles is a lower hall under the garden, the lattice rising in its west.

    python3 tools/worlds/world071.py                 # writes worlds3d/world071.vwr
    python3 tools/worlds/world071.py --stage K out   # for levelcheck:
        1: the cleft's mines gone (the jeep ran them over), the bars taken away (levelcheck
           would walk on them): the route stops at the lava trench
        2: the cleft's mines gone, floors under the bars (where Blupi hangs from them)
        3: the cleft's mines gone, the trench floored, the basin's bars taken away: the route
           stops at the basin
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from toys import (BAR, BALLS, BALL_POLES, CANDY_POLES, CONE_BALLS, HALF_BALLS, LAVA, MIXED,  # noqa: E402
                  PIPES, POLE_TILES, POLE_TOPS, STARS, WARNING, carve, toy)

# 071's own toys, as the original's counts: the green marble ball 218 most, the orange-striped
# pole tile 222, the star badge 230, the green-striped 221.
MASS = (218, 222, 230, 221)
MASS_W = (880, 300, 120, 80)
FLOOR = 218
STAR = 230
STRIPES = 222
WC = 30                  # the court's and the cleft's walking level
WT = 38                  # the upper storey's (the ramp's top, the pole tops, the bars, the garden)
WH = 6                   # the lower hall's
SPAWN = (16, WC, 71)

# The pole tops (x 24 + 2i, z 27 + 2j): 'o' a cap to stand on, 'M' a cap under a ball hung with a
# mine (a jump from or onto it meets the mine).
CAPS = [
    "ooMooooMoo",
    "oMooMoMooo",
    "oooMooooMo",
    "MoooMoMooo",
    "ooMooMoooM",
]


def mass(w, rng, x0, y0, z0, x1, y1, z1):
    toy(w, x0, y0, z0, x1, y1, z1, rng, MASS, MASS_W)


def court_and_tower(w, rng, stage):
    """A: the island court (walk 30) and the tower of balls, its stair of ledges round three
    faces up to the jeep on its top (walk 44); mines on the ledges, mines hanging under beams."""
    toy(w, 4, 26, 66, 30, WC - 1, 96, rng, (FLOOR,))
    mass(w, rng, 7, 23, 69, 27, 25, 93)                                # the underside, tapering
    mass(w, rng, 11, 21, 73, 23, 22, 89)
    for x in range(4, 31):
        for z in range(66, 97):
            if (x in (4, 30) or z == 96) and not (14 <= x <= 18 and z == 66):
                w.set(x, WC, z, rng.choice((STRIPES, FLOOR)))           # a kerb round the island
    w.bigdecor(HALF_BALLS[0], 25, WC, 90)
    w.bigdecor(HALF_BALLS[1], 7, WC, 70)

    mass(w, rng, 10, WC, 78, 16, 43, 84)                               # the core, top walk 44
    # The ledges, each three by three (L4 four wide), two higher than the last, round the east,
    # south and west faces: (x0, x1, z0, z1, walk).
    ledges = [(17, 19, 82, 84, 32), (17, 19, 85, 87, 34), (14, 16, 85, 87, 36), (10, 13, 85, 87, 38),
              (7, 9, 85, 87, 40), (7, 9, 81, 84, 42)]
    for x0, x1, z0, z1, walk in ledges:
        bottom = WC if walk == 32 else walk - 2
        toy(w, x0, bottom, z0, x1, walk - 1, z1, rng, (STRIPES, FLOOR, STAR) if walk % 4 else (FLOOR,))
    w.place(O.JEEP, 13, 44, 81)
    # Mines on the ledges' outer cells; hanging mines under beams from the core over the inner
    # cells where a jump would start, so the climber walks under them and jumps elsewhere.
    for x, y, z in ((19, 34, 87), (15, 36, 87), (11, 38, 87), (7, 40, 87), (7, 42, 82)):
        mine(w, x, y, z)
    for x, walk, z in ((16, 36, 85), (12, 38, 85), (9, 42, 84)):
        w.set(x, walk + 2, z, STRIPES)                                 # the beam
        mine(w, x, walk + 1, z, hanging=True)
    # Striped posts with balls at the court's corners (decor in the island's kerb).
    for x, z in ((4, 96), (30, 96), (4, 66), (30, 66)):
        w.fill(x, WC, z, x, WC + 3, z, rng.choice(POLE_TILES))
        w.set(x, WC + 4, z, rng.choice(POLE_TOPS))


def cleft(w, rng, stage):
    """B: the toy cliff north of the court, a cleft three wide through it: its floor full of mines
    five deep between warning signs. Only the jeep crosses it. Then C: the chasm, crossed on a
    lift three wide in a gantry of blue pipes; an egg on the far rim."""
    mass(w, rng, 4, 26, 52, 30, 44, 65)
    carve(w, 15, WC, 52, 17, 44, 65)
    for x in (14, 18):
        for z in (65, 63, 56):
            w.set(x, WC, z, WARNING)
    if stage < 1:
        for x in range(15, 18):
            for z in range(57, 62):
                mine(w, x, WC, z)
    # The chasm z 47-51, open to the sky; three lifts in step carry Blupi across it.
    for x in range(15, 18):
        lift(w, (x, WC - 1, 51), (x, WC - 1, 47), ticks=40, stop=40)
    # The gantry of blue pipes over it, six above the lift: out of reach (an empty-mask tile is a
    # floor to the engine's vertical test, D41).
    for x in (14, 18):
        w.fill(x, 36, 46, x, 36, 52, PIPES[2])
    w.fill(15, 36, 49, 17, 36, 49, PIPES[0])


def ramp(w, rng, stage):
    """D: the ramp hall (x 13-19), four steps of two up to walk 38, mines floating over the steps'
    edges in the lanes where a jump would start, a different two or three lanes each step."""
    mass(w, rng, 9, 26, 24, 21, 29, 46)                                # the floor, walk 30
    mass(w, rng, 9, WC, 24, 13, 46, 46)                                # the west wall
    mass(w, rng, 19, WC, 24, 21, 46, 46)                               # the east wall
    mass(w, rng, 9, WC, 24, 21, 46, 25)                                # the north wall
    for k, (z0, z1) in enumerate(((39, 41), (36, 38), (33, 35), (26, 32))):
        toy(w, 14, WC, z0, 18, WC + 1 + 2 * k, z1, rng, (FLOOR, FLOOR, STAR))
    w.place(O.EGG, 14, WC, 45)                                         # the chasm's egg, on the landing
    # Floating mines (16, start = end), two over the takeoff cell of a jump: (lanes, takeoff z, walk).
    for lanes, z, walk in (((14, 15, 16), 42, 30), ((16, 17, 18), 39, 32), ((14, 15), 36, 34),
                           ((14, 17, 18), 33, 36)):
        for x in lanes:
            w.place(O.MOVING_MINE, x, walk + 2, z, end=(x, walk + 2, z), advance=10, recede=10,
                    stop_start=0, stop_end=0)
    w.place(O.EGG, 16, WT, 27)
    carve(w, 19, WT, 29, 21, WT + 3, 31)                               # out east to the pole tops


def pole_tops(w, rng, stage):
    """E: the pole tops: caps on striped stems over a pit, a cell apart, in a hall roofed with
    balls; balls hung on candy poles from the roof over some caps, a mine under each."""
    mass(w, rng, 19, 26, 24, 46, 31, 38)                               # the pit floor, walk 32
    mass(w, rng, 19, 32, 26, 22, WT - 1, 36)                           # the entry ledge, walk 38
    mass(w, rng, 19, 32, 24, 46, 46, 25)                               # north wall
    mass(w, rng, 19, 32, 37, 46, 46, 38)                               # south wall
    mass(w, rng, 19, 45, 24, 46, 46, 38)                               # the roof
    mass(w, rng, 44, 32, 26, 46, WT - 1, 36)                           # the far ledge, walk 38
    mass(w, rng, 47, 26, 24, 48, 46, 38)                               # the east wall
    carve(w, 47, WT, 30, 48, WT + 5, 32)                               # but the corridor's mouth
    for j, row in enumerate(CAPS):
        for i, kind in enumerate(row):
            x, z = 24 + 2 * i, 27 + 2 * j
            w.fill(x, 32, z, x, WT - 2, z, rng.choice(POLE_TILES))
            w.set(x, WT - 1, z, rng.choice(POLE_TOPS))
            if kind == "M":
                w.fill(x, 41, z, x, 44, z, rng.choice(CANDY_POLES))    # out of reach, 3 above a cap
                w.set(x, 40, z, rng.choice(BALLS))
                mine(w, x, WT + 1, z, hanging=True)
    # A way back up from the pit: two blocks against the entry ledge's face.
    w.fill(23, 32, 26, 23, 33, 26, FLOOR)                              # walk 34
    w.fill(23, 32, 27, 23, 35, 27, FLOOR)                              # walk 36


def bars(w, rng, stage):
    """F: the lava trench: a corridor three wide, its floor gone over seven cells, lava three
    below; bars across it at the walking level: walking off the edge Blupi hangs from them."""
    mass(w, rng, 44, 32, 29, 56, WT - 1, 33)                           # the floor, walk 38
    mass(w, rng, 44, WT, 29, 55, 44, 29)                               # walls
    mass(w, rng, 44, WT, 33, 55, 44, 33)
    mass(w, rng, 44, 44, 29, 55, 44, 33)                               # the roof
    carve(w, 47, 35, 30, 53, WT - 1, 32)
    w.fill(47, 34, 30, 53, 34, 32, LAVA)
    if stage in (2, 3):
        w.fill(47, 34, 30, 53, WT - 1, 32, FLOOR)
    if stage != 1:
        w.fill(47, WT, 30, 54, WT, 32, BAR)


def garden(w, rng, stage):
    """G: the garden of every toy (walk 38) over the lower hall: mounds, an egg on the higher one,
    toy decor; the shaft down through its floor, lined with striped poles, a mine each side."""
    toy(w, 56, 34, 18, 78, WT - 1, 42, rng, MIXED)
    for x in range(56, 79):
        for z in range(18, 43):
            if x in (56, 78) or z in (18, 42):
                if not (x == 56 and 30 <= z <= 32):
                    toy(w, x, WT, z, x, WT + 2, z, rng, MIXED)
    carve(w, 55, WT, 30, 56, WT + 3, 32)                               # in from the trench
    # The mounds three cells clear of the kerb (from a mound a jump would reach its top).
    toy(w, 60, WT, 22, 64, WT + 1, 26, rng, MIXED)                     # mound, walk 40
    toy(w, 66, WT, 22, 70, WT + 3, 25, rng, MIXED)                     # higher, walk 42
    w.place(O.EGG, 68, WT + 4, 23)
    for icon, x, z in ((CONE_BALLS[0], 59, 38), (CONE_BALLS[1], 75, 22), (CONE_BALLS[2], 64, 34),
                       (BALL_POLES[0], 75, 30)):
        w.bigdecor(icon, x, WT, z)
    # The shaft x 70-72, z 36-38 through the floor; striped poles line it.
    for x in range(69, 74):
        for z in range(35, 40):
            w.fill(x, 34, z, x, WT - 1, z, rng.choice(POLE_TILES))
    carve(w, 70, 34, 36, 72, WT - 1, 38)
    for x, z in ((69, 35), (73, 35), (69, 39), (73, 39)):
        w.fill(x, WT, z, x, WT + 2, z, rng.choice(POLE_TILES))
        w.set(x, WT + 3, z, rng.choice(POLE_TOPS))
    mine(w, 71, WT, 34)
    mine(w, 71, WT, 40)


def lower_hall(w, rng, stage):
    """H: the lower hall (walk 6) under the garden, its floor of orange stripes: the trench of
    lava cut across it, crossed on two pairs of lifts with an island between; in its west the
    lattice of star platforms on striped posts: lifts up and across, bars over a lava basin, a
    lift over another, eggs, the red arrow at the top (walk 24)."""
    toy(w, 40, 2, 10, 82, 5, 46, rng, (STRIPES, STRIPES, 221))
    for x in range(40, 83):
        for z in range(10, 47):
            if x in (40, 82) or z in (10, 46):
                toy(w, x, WH, z, x, WH + 4, z, rng, (STRIPES, FLOOR))
    # The trench x 58-67 across the hall, lava four below; the island x 62-63 between its halves.
    carve(w, 58, 2, 11, 67, 5, 45)
    w.fill(58, 1, 11, 67, 1, 45, LAVA)
    toy(w, 62, 1, 11, 63, 5, 45, rng, (STRIPES,))
    for z in (36, 37):
        lift(w, (67, 5, z), (64, 5, z), ticks=30, stop=40)             # east half: east -> west
    for z in (24, 25):
        lift(w, (61, 5, z), (58, 5, z), ticks=30, stop=40)             # west half

    # The lattice (x 41-57): star platforms (slabs of 230 on striped posts), walking levels.
    def platform(x0, z0, x1, z1, walk, post=True):
        w.fill(x0, walk - 1, z0, x1, walk - 1, z1, STAR)
        if post:
            cx, cz = (x0 + x1) // 2, (z0 + z1) // 2
            w.fill(cx, WH, cz, cx, walk - 2, cz, rng.choice(POLE_TILES))

    def stem(x, z, y1):
        w.fill(x, WH, z, x, y1, z, rng.choice(POLE_TILES))

    # V1: two lifts in step rising four from the floor (flush in it) to P1 (walk 10).
    for x in (52, 53):
        w.clear(x, 5, 30)
        lift(w, (x, 5, 30), (x, 9, 30), ticks=40, stop=40)
    platform(51, 26, 54, 29, 10)                                       # P1
    platform(47, 26, 50, 29, 12)                                       # P2, a jump up west
    platform(47, 22, 50, 25, 14)                                       # P3, a jump up north
    # The lava basin under bars (walk 14), from P3 north to P4: lava over z 16-21, two below, no
    # rim to walk round the bars on.
    w.fill(47, 10, 16, 50, 10, 21, STAR)                               # its floor
    w.fill(47, 11, 16, 50, 11, 21, LAVA)
    stem(48, 18, 9)
    if stage == 2:
        w.fill(47, 11, 16, 50, 13, 21, STAR)
    if stage in (0, 2):
        w.fill(47, 14, 15, 50, 14, 21, BAR)
    platform(47, 12, 50, 15, 14)                                       # P4, the far side
    w.place(O.EGG, 47, 14, 24)
    w.place(O.EGG, 50, 14, 12)
    # H1: a shuttle pair east from P4 over the gap to P5 (walk 14).
    for z in (12, 13):
        lift(w, (51, 13, z), (54, 13, z), ticks=30, stop=40)
    platform(55, 11, 57, 14, 14)                                       # P5
    # V2: a lift rising three south of P5 to P6 (walk 17); three eggs on P6.
    lift(w, (56, 13, 15), (56, 16, 15), ticks=30, stop=40)
    platform(53, 16, 57, 19, 17)                                       # P6
    for x in (54, 55, 57):
        w.place(O.EGG, x, 17, 18)
    # V3: rising three west of P6 to P7 (walk 20), over the basin's corner.
    lift(w, (52, 16, 17), (52, 19, 17), ticks=30, stop=40)
    platform(49, 15, 51, 18, 20, post=False)                           # P7
    w.fill(51, WH, 18, 51, 18, 18, rng.choice(POLE_TILES))             # beside the bars' lanes
    # H2: a lift across a strip of lava from P7 west to P8 (walk 20), the lava one below it.
    w.fill(44, 17, 15, 48, 17, 18, STAR)
    w.fill(44, 18, 15, 48, 18, 18, LAVA)
    stem(46, 16, 16)
    lift(w, (48, 19, 16), (44, 19, 16), ticks=40, stop=40)
    platform(41, 15, 43, 18, 20)                                       # P8
    # Up two jumps to the arrow (walk 24).
    platform(41, 11, 43, 14, 22)                                       # P9, north of P8
    platform(44, 11, 46, 14, 24)                                       # the arrow's, east of P9
    w.place(O.EXIT, 45, 24, 12)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world071.vwr"
    rng = random.Random(71)
    w = World(mission=71, sky=12)
    court_and_tower(w, rng, stage)
    cleft(w, rng, stage)
    ramp(w, rng, stage)
    pole_tops(w, rng, stage)
    bars(w, rng, stage)
    garden(w, rng, stage)
    lower_hall(w, rng, stage)
    w.spawn = SPAWN
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
