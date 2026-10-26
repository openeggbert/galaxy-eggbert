#!/usr/bin/env python3
"""Mission 025, the castle's trial and world 2's golden key (worlds3d/world025.vwr): brief
plans/briefs/world025.md.

Mobile Eggbert's level 025 is world 2's last test, every lesson once more round a castle: Blupi
starts at the foot of a shaft whose twin lifts rise to the golden key, which waits until the
four treasures are found. A tower is climbed on a ladder of small lifts shuttling one cell, a
lightning column is timed, a switch stops a row of saws before a treasure; lava pits are crossed
on balustrades and small lifts past mines; skateboards run over mines; four sticks of dynamite
blast a cellar of crates, with helicopters at its bottom; a water tower is swum past fish; a
helicopter crosses lava under mines moving up and down. Here the keep's courtyard with the key
tower and its twin lifts in the middle; west the tower of the lift ladder, the bridge of the
lightning and the switch, the first treasure; the drop to the lava terrace and the second; the
skate corridor over a band of mines; the south-east hall with the dynamite, the crate plug over
the cellar, the cellar's treasure, sticks and helicopters; its pool and the water tower of fish
up to the roofed lava hall, the helicopters on its pad and the third treasure across the lava;
the bridge back over the keep. The golden key ends world 2.

    python3 tools/worlds/world025.py                   # writes worlds3d/world025.vwr
    python3 tools/worlds/world025.py --stage K out.vwr # lessons 1..K solved (levelcheck, which has
        no lifts to jump between, switches, vehicles, dynamite or swimming):
        1 the lift ladder climbed (a stand-in lift), 2 the switch pressed, 3 the skateboard over the mines,
        4 the dynamite blasts done, 5 the helicopter out of the cellar (a stand-in lift),
        6 the water tower swum (a stand-in lift), 7 the helicopter over the lava (its floor)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from palace import (BALUSTRADE, BLITZ, BLITZ_EMITTER, BRICK, COLUMN, DEEP_WATER, FOUNTAIN,  # noqa: E402
                    GRASS, LAVA, PALMS, PLANTS, SAW, STATUE, STONE, SWITCH, WATER, parapet,
                    stone, windows_row)

SPAWN = (50, 11, 56)
LH = 22                      # the lava hall's floor: Blupi walks at 23


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(25)
    w = World(mission=25, sky=30)

    def white(x0, y0, z0, x1, y1, z1, chance=0.2):
        stone(w, x0, y0, z0, x1, y1, z1, rng, chance=chance)

    def brick(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, BRICK)

    def crates(x0, y0, z0, x1, y1, z1):
        if stage < 4:
            for x in range(x0, x1 + 1):
                for y in range(y0, y1 + 1):
                    for zz in range(z0, z1 + 1):
                        w.place(O.CRATE, x, y, zz)

    # The keep: a courtyard (x 43-57, z 43-57, walk 11, walls six high) on grass; in it the key
    #    tower, its chamber at walk 19 reached only by the twin lifts on its south face (48
    #    ticks for eight cells, the original's pace), the golden key inside.
    w.fill(43, 10, 43, 57, 10, 57, GRASS)
    for x in range(46, 55):
        for zz in range(51, 58):
            w.set(x, 10, zz, STONE)
    white(42, 10, 42, 58, 16, 42)
    white(42, 10, 58, 58, 16, 58)
    white(42, 10, 42, 42, 16, 58)
    white(58, 10, 42, 58, 16, 58)
    windows_row(w, 43, 57, 13, 58, rng, chance=0.6)
    white(47, 11, 45, 53, 21, 50, chance=0.3)                        # the key tower
    windows_row(w, 47, 53, 15, 50, rng, chance=0.6)
    w.fill(48, 19, 46, 52, 20, 49, 0)                                # its chamber
    w.fill(49, 19, 50, 50, 20, 50, 0)                                # the chamber's doorway
    for x in (49, 50):
        w.clear(x, 10, 51)
        w.set(x, 9, 51, STONE)                                       # the lifts' well, one deep
        lift(w, (x, 10, 51), (x, 18, 51), ticks=48, stop=3)
    w.place(O.SECRET_EXIT, 50, 19, 47)
    w.place(O.EGG, 48, 19, 46)
    w.bigdecor(STATUE, 52, 19, 46)
    for x, zz in ((44, 44), (56, 44), (44, 56), (56, 56)):
        w.bigdecor(COLUMN, x, 11, zz)
    for x, zz in ((44, 50), (56, 50)):
        w.bigdecor(PALMS[x % 2], x, 11, zz)

    # West: the corridor to the ladder tower (x 28-36, z 46-54), walls to y 28.
    w.fill(42, 11, 49, 42, 12, 51, 0)                                # the courtyard's west door
    white(37, 10, 48, 41, 10, 52, chance=0.1)
    white(37, 11, 48, 41, 13, 48)
    white(37, 11, 52, 41, 13, 52)
    white(28, 10, 46, 36, 10, 54, chance=0.1)
    white(28, 11, 46, 36, 28, 46)                                   # three above the landing:
    white(28, 11, 54, 36, 28, 54)                                   # no stair onto them
    white(28, 11, 46, 28, 28, 54)
    white(36, 11, 46, 36, 28, 54)
    w.fill(36, 11, 49, 36, 12, 51, 0)                                # the tower's door
    # The ladder: six pairs of small lifts one above the other, each two higher than the one
    #    below, shuttling one cell along X (10 ticks, stop 10) in alternate directions as the
    #    original's, so the next one up is always one cell beside: walk 13 ... 23.
    for k in range(6):
        a, b = (33, 32) if k % 2 == 0 else (32, 33)
        for zz in (49, 50):
            lift(w, (a, 12 + 2 * k, zz), (b, 12 + 2 * k, zz), ticks=10, stop=10)
    if stage >= 1:                       # levelcheck cannot jump between lifts: a stand-in
        w.place(O.LIFT, 34, 10, 52, end=(31, 24, 52))
    white(29, 24, 47, 31, 24, 53, chance=0.1)                        # the top landing, walk 25
    for zz in (48, 49, 50):
        w.place(O.EGG, 29, 25, zz)
    # The lightning bridge: one wide along X west from the landing (walk 25), over the void.
    w.fill(28, 25, 50, 28, 26, 50, 0)                                # out through the west wall
    for x in range(12, 28):
        w.set(x, 24, 50, STONE)
    # The lightning: an emitter on a gate over the bridge, four cells of lightning under it
    #    (Decor::AdaptBorder's column), flashing half the time: walk through between flashes.
    w.set(26, 29, 50, BLITZ_EMITTER)
    for y in range(25, 29):
        w.set(26, y, 50, BLITZ)
    white(26, 22, 49, 26, 30, 49, chance=0.3)
    white(26, 22, 51, 26, 30, 51, chance=0.3)
    w.set(26, 30, 50, STONE)
    w.set(23, 24, 50, SWITCH)                                        # the switch, then ten saws
    for x in range(13, 23):
        w.set(x, 24, 50, SAW if stage < 2 else STONE)
    white(8, 24, 47, 12, 24, 58, chance=0.1)                         # the far platform: its
    w.place(O.TREASURE, 10, 25, 50)
    w.bigdecor(PLANTS[0], 9, 25, 52)                                 # south end over the terrace

    # South-west: Blupi drops off the far platform onto the lava terrace (walk 11, x 8-40,
    #    z 56-68): lava in its floor, balustrades, mines, a lift of one cell (10 ticks) and one
    #    of four (30 ticks), the second treasure on the last balustrade. The lifts travel in
    #    the floor's row, so their paths are open, not lava (standing over lava is death). The
    #    piers are stone with the balustrades along their edges: decor Blupi walks through
    #    (D41), not a floor.
    white(8, 10, 56, 40, 10, 68, chance=0.05)
    parapet(w, [(x, 68) for x in range(8, 17)] + [(x, 68) for x in range(31, 41)] +
            [(8, z) for z in range(56, 69)], 11, rng)

    def pier(x):
        for zz in range(56, 69):
            w.set(x, 10, zz, STONE)
        for zz in (56, 68):
            w.set(x, 11, zz, BALUSTRADE)

    w.fill(17, 10, 56, 18, 10, 68, LAVA)                             # lava to the terrace's edge
    w.fill(17, 10, 62, 18, 10, 62, 0)
    lift(w, (17, 10, 62), (18, 10, 62), ticks=10, stop=10)
    for x in (19, 20, 21):
        pier(x)
    mine(w, 20, 11, 58)
    mine(w, 20, 11, 66)
    w.fill(22, 10, 56, 26, 10, 68, LAVA)
    w.fill(22, 10, 62, 26, 10, 62, 0)
    lift(w, (22, 10, 62), (26, 10, 62), ticks=30, stop=10)
    for x in range(27, 31):
        pier(x)
    mine(w, 28, 11, 59)
    mine(w, 29, 11, 65)
    w.place(O.TREASURE, 30, 11, 62)
    white(8, 11, 55, 40, 13, 55)                                     # the terrace's north wall

    # The skate corridor (x 41-58, z 61-63, walls three high): five skateboards at its start, a
    #    band of nine mines three deep across it that only a skateboard runs over.
    white(41, 10, 60, 58, 10, 64, chance=0.05)
    white(41, 11, 60, 58, 13, 60)
    white(41, 11, 64, 58, 13, 64)
    for zz in (61, 62, 63):
        w.place(O.SKATEBOARD, 43, 11, zz)
    for x in (44, 45):
        w.place(O.SKATEBOARD, x, 11, 62)
    if stage < 3:
        for x in (50, 51, 52):
            for zz in (61, 62, 63):
                mine(w, x, 11, zz)
    # Over the mines the corridor is one high (a slot three long): the skateboard rides
    #    through it, a helicopter from the hall cannot pass without touching them. East of it
    #    the corridor is roofed, so the hall's helicopters stay in the hall.
    white(50, 12, 61, 52, 14, 63, chance=0.1)
    white(53, 14, 60, 58, 14, 64, chance=0.05)

    # South-east: the hall (x 59-79, z 56-78, walk 11, walls seven high).
    white(59, 10, 55, 79, 10, 78, chance=0.05)
    white(59, 11, 55, 79, 17, 55)
    white(59, 11, 78, 79, 17, 78)
    white(59, 11, 55, 59, 17, 78)
    white(79, 11, 55, 79, 17, 78)
    w.fill(59, 11, 61, 59, 13, 63, 0)                                # from the skate corridor
    windows_row(w, 60, 78, 14, 55, rng, chance=0.4)
    white(59, 18, 55, 79, 18, 78, chance=0.05)                       # its roof: the cellar's
                                                                     # helicopters stay inside
    # The dynamite: two sticks on the depot by the door, two more at the cellar's bottom.
    white(60, 11, 58, 62, 11, 59, chance=0.1)
    for x in (60, 62):
        w.place(O.DYNAMITE, x, 12, 58)
    # The cellar (x 62-74, z 60-72, walk 1, brick): under the hall's floor, its mouth (x 65-69,
    #    z 63-67) plugged by twenty-five crates at the floor's level: blast them, jump in.
    brick(61, 0, 59, 75, 0, 73)
    brick(61, 1, 59, 75, 9, 59)
    brick(61, 1, 73, 75, 9, 73)
    brick(61, 1, 59, 61, 9, 73)
    brick(75, 1, 59, 75, 9, 73)
    brick(62, 9, 60, 74, 9, 72)
    w.fill(65, 9, 63, 69, 10, 67, 0)                                 # the mouth
    crates(65, 10, 63, 69, 10, 67)
    # Its bottom: two rooms behind brick (z 70-72), the helicopters' and the treasure's, each
    #    entered through a doorway of six crates; two sticks on the floor before them.
    brick(62, 1, 69, 74, 8, 69)
    brick(69, 1, 70, 69, 8, 72)
    for x0 in (64, 71):
        w.fill(x0, 1, 69, x0 + 2, 2, 69, 0)
        crates(x0, 1, 69, x0 + 2, 2, 69)
    w.place(O.TREASURE, 73, 1, 71)
    for x in (64, 70):
        w.place(O.DYNAMITE, x, 1, 61)
    for x in (62, 63, 64, 66, 67):
        w.place(O.HELICOPTER, x, 1, 71)
    if stage >= 5:                       # levelcheck cannot fly: a stand-in lift out
        w.place(O.LIFT, 67, 0, 66, end=(67, 10, 66))
    for x, zz in ((63, 75), (64, 75), (65, 75), (64, 76)):
        w.place(O.CRATE, x, 11, zz)                                  # a stack by the wall
    w.place(O.EGG, 77, 11, 76)
    # Its pool (x 76-78, z 56-60) and a tunnel under the north wall to the water tower.
    brick(75, 6, 55, 79, 9, 61)
    # The water tower (x 73-79, z 43-54): water from y 7 up to the lava hall's floor (LH), a
    #    climb of fourteen cells, the original's shaft is thirteen deep; walls and roof shared
    #    with the lava hall, so nothing flies in from outside.
    white(72, 6, 42, 80, 6, 55, chance=0.1)
    white(72, 7, 42, 72, LH + 7, 55)
    white(80, 7, 42, 80, LH + 7, 55)
    white(72, 7, 42, 80, LH, 42)
    white(72, 7, 55, 80, LH + 7, 55)
    white(72, LH + 8, 42, 80, LH + 8, 55, chance=0.05)
    w.fill(76, 7, 56, 78, 9, 60, DEEP_WATER)
    w.fill(76, 10, 56, 78, 10, 60, WATER)
    w.fill(76, 7, 55, 78, 9, 55, DEEP_WATER)                         # the tunnel north
    w.fill(73, 7, 43, 79, LH - 1, 54, DEEP_WATER)
    w.fill(73, LH, 43, 79, LH, 54, WATER)
    # Six fish crossing it at six heights, along X and along Z.
    for a, b in (((74, 9, 46), (78, 9, 46)), ((76, 11, 44), (76, 11, 53)), ((78, 13, 50), (74, 13, 50)),
                 ((74, 15, 52), (78, 15, 52)), ((76, 17, 53), (76, 17, 45)), ((78, 19, 45), (74, 19, 45))):
        w.place(O.FISH, *a, end=b, advance=45, recede=45, stop_start=20, stop_end=20)
    if stage >= 6:                       # levelcheck cannot swim: a stand-in lift up
        w.place(O.LIFT, 74, 10, 58, end=(77, LH, 42))

    # North-east: the roofed lava hall (x 54-79, z 22-41, walk LH + 1, roof LH + 8), where the
    #    swim comes out. Across its middle (x 58-68) a channel three wide over lava, solid on
    #    both sides; two gates in it leave one cell (z 31, four high) where a mine moves up and
    #    down (30 ticks, as the original's two): fly through low when it is up, high when it is
    #    down. The helicopter pad east, saws on its edge; the third treasure on the west ledge;
    #    the door out west.
    top = LH + 1
    white(53, LH, 21, 80, LH, 42, chance=0.05)
    for x0, z0, x1, z1 in ((53, 21, 80, 21), (53, 21, 53, 42), (80, 21, 80, 42), (53, 42, 72, 42)):
        white(x0, top, z0, x1, LH + 7, z1, chance=0.15)
    white(53, LH + 8, 21, 80, LH + 8, 42, chance=0.05)
    white(58, top, 22, 68, LH + 7, 29, chance=0.2)
    white(58, top, 33, 68, LH + 7, 41, chance=0.2)
    w.fill(58, LH, 30, 68, LH, 32, STONE if stage >= 7 else LAVA)
    for x in (61, 65):
        if stage < 7:
            white(x, top, 30, x, LH + 7, 32, chance=0.3)
            w.fill(x, top + 1, 31, x, top + 4, 31, 0)
            w.place(O.MOVING_MINE, x, top + 1, 31, end=(x, top + 4, 31), advance=30, recede=30,
                    stop_start=0, stop_end=0)
    for x in (72, 74, 76, 78):
        w.place(O.HELICOPTER, x, top, 28)
    for zz in (27, 28, 29):
        w.set(69, LH, zz, SAW)
    w.place(O.TREASURE, 55, top, 31)
    w.bigdecor(FOUNTAIN, 55, top, 24)
    mine(w, 56, top, 39)
    # The bridge back: out of the west door, south over the keep's north wall, where it stops:
    #    Blupi drops into the courtyard.
    w.fill(53, top, 30, 53, top + 1, 32, 0)
    for x in range(49, 53):
        for zz in range(30, 33):
            w.set(x, LH, zz, STONE)
    for zz in range(33, 44):
        w.set(50, LH, zz, STONE)
        w.set(51, LH, zz, STONE)

    w.spawn = SPAWN
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world025.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
