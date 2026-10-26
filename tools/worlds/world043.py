#!/usr/bin/env python3
"""Mission 043, the cheese warren (worlds3d/world043.vwr): brief plans/briefs/world043.md.

Mobile Eggbert's level 043, under the region-21 background of lava rock: a warren of narrow
passages through cheese. Blupi starts beside the red arrow, takes two treasures near the
start, then walks a long way down through winding passages -- three mines lying in narrow
places to jump, three more treasures -- to a cave where eight helicopters stand. He flies up
through two mines hanging over the way, and through the winding upper passages for the other
seven treasures, finds a second row of eight helicopters, and flies back down to the arrow
beside the start. Here the warren is a block of cheese carved in 3D: the start cave, the
walking passages spiralling down and east with the mines in slots of the cross walls, the
helicopter grotto at the bottom, the flight passages climbing through chambers, a chimney and
a dome round the block's south side, and the way back over the start into its roof.

    python3 tools/worlds/world043.py         # writes worlds3d/world043.vwr
    Check with `tools/flightcheck.py --mine-margin 0` (mines can be jumped, as in the game).
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from cheese import PLANTS, carve, cave, cheese, hang, helicopters, hole, shaft, tube  # noqa: E402

WS = 40          # the start cave's floor
SPAWN = (30, WS, 33)
WG = 14          # the helicopter grotto's floor


def start_cave(w: World, rng: random.Random):
    """S: the start cave (walk 40), the red arrow beside the spawn (it counts only with all
    twelve treasures), treasures 1 and 2 on ledges a jump up, round windows out onto the lava
    rock high in its west and north walls."""
    cave(w, 30, WS, 30, 6.5, 5, 3, rng, rz=5.5)
    w.place(O.EXIT, 28, WS, 34)
    cheese(w, 23, WS, 28, 25, WS + 1, 32, rng)            # the west ledge
    w.place(O.TREASURE, 24, WS + 2, 30)
    cheese(w, 28, WS, 24, 32, WS + 1, 26, rng)            # the north ledge
    w.place(O.TREASURE, 30, WS + 2, 25)
    w.bigdecor(PLANTS[0], 33, WS, 34)
    for a, b in (((18, WS + 5, 30), (24, WS + 5, 30)), ((30, WS + 5, 18), (30, WS + 5, 25))):
        tube(w, a, b, 1.3)


def walk(w: World, rng: random.Random):
    """W: the passages down from the start cave to the grotto, on foot: east, a drop, south,
    a drop, east, a drop, north, and a shaft down into the grotto's roof. A mine lies in each
    of three slots one wide in the cross walls, jumped through; treasures 3 and 4 in pockets
    off the way."""
    r = 2.1
    tube(w, (36, WS, 31), (46, WS, 31), r, floor=WS)                       # east
    tube(w, (47, WS - 3, 31), (47, WS - 3, 50), r, floor=WS - 3)           # a drop, south
    tube(w, (47, WS - 7, 51), (62, WS - 7, 51), r, floor=WS - 7)           # a drop, east
    tube(w, (63, WS - 11, 51), (63, WS - 11, 40), r, floor=WS - 11)        # a drop, north
    # The three slots: a cross wall with a slot one wide, five high, a mine lying in it.
    for (x, y, z), along_x in (((42, WS, 31), True), ((47, WS - 3, 44), False), ((56, WS - 7, 51), True)):
        if along_x:
            cheese(w, x, y, z - 3, x, y + 5, z + 3, rng)
            carve(w, x, y, z, x, y + 4, z)
        else:
            cheese(w, x - 3, y, z, x + 3, y + 5, z, rng)
            carve(w, x, y, z, x, y + 4, z)
        mine(w, x, y, z)
    # Pockets: treasure 3 east of the first drop, a jump up; treasure 4 north of the east
    # passage, an egg beside it.
    cave(w, 53, WS - 3, 37, 2.8, 4, 1, rng)
    tube(w, (48, WS - 3, 37), (51, WS - 3, 37), 1.6, floor=WS - 3)
    cheese(w, 53, WS - 3, 36, 54, WS - 2, 38, rng)
    w.place(O.TREASURE, 54, WS - 1, 37)
    cave(w, 54, WS - 7, 46, 2.8, 4, 1, rng)
    tube(w, (54, WS - 7, 49), (54, WS - 7, 47), 1.6, floor=WS - 7)
    w.place(O.TREASURE, 54, WS - 7, 45)
    # The shaft from the north passage's end down into the grotto's roof.
    shaft(w, 63, 39, WG + 6, WS - 11, 1.8)


def grotto(w: World, rng: random.Random):
    """G: the helicopter grotto at the bottom (walk 14): eight helicopters, treasure 5, an
    egg; in its roof the shaft up, narrowed by a ring with two mines hanging under it at its
    edge."""
    cave(w, 66, WG, 35, 8, 7, 2, rng, rz=7)
    helicopters(w, [(61, WG, 31), (62, WG, 33), (61, WG, 35), (62, WG, 37), (70, WG, 30), (71, WG, 32),
                    (70, WG, 37), (71, WG, 39)])
    w.place(O.TREASURE, 66, WG, 29)
    w.place(O.EGG, 67, WG, 30)
    # The shaft up, radius 2.5, from the grotto's roof to the first chamber (y 34); a ring at
    # y 27 leaves a hole three wide; the mines hang under it either side.
    shaft(w, 71, 39, WG + 6, 34, 2.5)
    for x, z in ((x, z) for x in range(68, 75) for z in range(36, 43)):
        if (x - 71) ** 2 + (z - 39) ** 2 <= 2.5 ** 2 and not (70 <= x <= 72 and 38 <= z <= 40):
            w.set(x, 27, z, rng.choice((246, 247, 248, 249)))
    hang(w, 71, 26, 37)
    hang(w, 71, 26, 41)


def flight(w: World, rng: random.Random):
    """F: the flight passages round the block's south side. Chamber 1 (walk 34) at the
    shaft's top: treasure 6 on a shelf. A passage descending south-west to chamber 2 (walk
    24): treasure 7 on its floor, treasure 8 in a niche high in its wall. A chimney from it up
    to the dome, treasure 9 in a side niche halfway. The dome (walk 46): the second row of
    helicopters, treasures 10 and 11 on ledges, an egg. The passage north from the dome over
    the start cave, treasure 12 in a pocket of its roof, and the shaft down into the start
    cave's roof."""
    cave(w, 73, 34, 43, 5, 5, 2, rng)                                   # chamber 1
    shaft(w, 71, 39, 30, 35, 2.5)                                        # the shaft opens in its floor
    cheese(w, 75, 34, 45, 77, 36, 47, rng)                              # its shelf, three high
    w.place(O.TREASURE, 76, 37, 46)
    tube(w, (72, 36, 47), (61, 27, 61), 1.6)                             # down south-west
    cave(w, 57, 24, 63, 5, 6, 2, rng, rz=4.5)                           # chamber 2
    w.place(O.TREASURE, 58, 24, 63)
    tube(w, (60, 28, 66), (62, 28, 69), 1.3)                             # the high niche
    w.set(62, 27, 69, 248)
    w.place(O.TREASURE, 62, 28, 69)
    # The chimney up from chamber 2's west side to the dome, a niche halfway with treasure 9.
    shaft(w, 50, 62, 24, 46, 2.2)
    tube(w, (53, 26, 63), (50, 26, 62), 1.8)                             # its foot from chamber 2
    tube(w, (50, 36, 63), (50, 36, 68), 1.3)                             # the niche
    w.set(50, 35, 68, 247)
    w.place(O.TREASURE, 50, 36, 68)
    # The dome at the chimney's top.
    cave(w, 42, 46, 60, 7, 7, 3, rng, rz=6)
    helicopters(w, [(41, 46, 58), (39, 46, 59), (38, 46, 61), (39, 46, 63), (45, 46, 57), (46, 46, 59),
                    (36, 46, 60), (41, 46, 64)])
    cheese(w, 38, 46, 56, 40, 49, 57, rng)                              # a ledge four high
    w.place(O.TREASURE, 39, 50, 56)
    cheese(w, 43, 46, 64, 46, 51, 65, rng)                              # a ledge six high
    w.place(O.TREASURE, 44, 52, 64)
    w.place(O.EGG, 42, 46, 56)
    # The passage north over the start cave and the shaft down into its roof.
    tube(w, (41, 51, 55), (32, 51, 41), 1.6)
    hole(w, 37, 54, 48, 1.5)                                             # a pocket in its roof
    w.set(37, 51, 48, 249)
    w.place(O.TREASURE, 37, 52, 48)
    shaft(w, 32, 39, WS + 5, 52, 1.8)
    tube(w, (32, WS + 6, 39), (31, WS + 5, 34), 1.8)


def main() -> int:
    rng = random.Random(43)
    w = World(mission=43, sky=21)
    cheese(w, 18, 8, 20, 82, 58, 72, rng)
    start_cave(w, rng)
    walk(w, rng)
    grotto(w, rng)
    flight(w, rng)
    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world043.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
