#!/usr/bin/env python3
"""Mission 105, the helicopter cavern (worlds3d/world105.vwr): brief plans/briefs/world105.md.

Mobile Eggbert's level 105 is a great cobbled cavern flown through in helicopters (13; twenty-four
of them parked about it): its roof hung with mines (3), birds (20) crossing it, treasures and
eggs on little ledges round its walls and on blocks in its middle, the exit in a hut of cobble on
one of them; at its east side a passage under more hanging mines past columns of lightning into a
chamber with a bulldozer (4) and the last treasure; a lift, a wasp (44), two moving mines (16).
Seven treasures, eleven eggs. Here the cavern is a bottomless hall in 3D: islands of cobble at
many heights round and across it, rows of mines along its roof, the birds between the islands,
a long ferry of lifts from the start ledge to the hut, the east passage and its chamber.

    python3 tools/worlds/world105.py                 # writes worlds3d/world105.vwr
    python3 tools/worlds/world105.py --stage 1 out   # for flightcheck: no lightning, the
                                                     # bulldozer and the wasp left out
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, rock  # noqa: E402
from kit import O, World, lift, mine  # noqa: E402
from village import STONE  # noqa: E402

BLITZ, EMITTER = 305, 304
ROOF = 41                   # the cavern's roof; its mines hang at 40


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world105.vwr"
    rng = random.Random(105)
    w = World(mission=105, sky=2)
    calm = stage is not None and stage >= 1

    def heli(x, y, z):
        w.place(O.HELICOPTER, x, y, z)

    def island(x0, z0, x1, z1, walk, thick=2):
        rock(w, x0, walk - thick, z0, x1, walk - 1, z1)

    def bird(start, end, ticks):
        w.place(O.BIRD, *start, end=end, advance=ticks, recede=ticks, stop_start=8, stop_end=8)

    # The cavern: a hall of cobble open below (a fall is the end), its roof at 41.
    rock(w, 6, 4, 16, 74, ROOF + 4, 84)
    carve(w, 10, 4, 20, 70, ROOF - 1, 80)
    carve(w, 6, 0, 16, 74, 3, 84)

    # ---- the start ledge (walk 16) on the west wall ----
    island(10, 42, 17, 58, 16, thick=3)
    w.spawn = (12, 16, 50)
    for z in (44, 47, 53, 56):
        heli(15, 16, z)
    w.place(O.EGG, 11, 16, 44)
    # A ferry of lifts (2 x 2) from its east edge across the void to the hut's island (walk 18):
    # one lift each way, meeting midway and waiting 40 ticks there.
    for x in (18, 19):
        for z in (49, 50):
            lift(w, (x, 15, z), (x + 6, 17, z), ticks=80, stop=40)       # ends at x 24-25
            lift(w, (x + 11, 17, z), (x + 8, 17, z), ticks=80, stop=40)  # ends at x 26-27

    # ---- the islands ----
    # Treasures 1-6, eggs, parked helicopters, a mine; the hut of the exit on the middle island.
    island(22, 28, 26, 32, 20)
    w.place(O.TREASURE, 24, 20, 30)
    w.place(O.EGG, 23, 20, 29)
    island(38, 24, 42, 28, 14)
    w.place(O.TREASURE, 40, 14, 26)
    heli(41, 14, 27)
    island(54, 32, 58, 36, 24)
    w.place(O.TREASURE, 56, 24, 34)
    w.place(O.EGG, 57, 24, 35)
    island(60, 54, 66, 59, 12)
    w.place(O.TREASURE, 63, 12, 56)
    for z in (55, 57, 59):
        heli(65, 12, z)
    if not calm:
        mine(w, 61, 12, 58)
    island(44, 62, 48, 66, 32)
    w.place(O.TREASURE, 46, 32, 64)
    w.place(O.EGG, 45, 32, 63)
    island(26, 70, 30, 74, 26)
    w.place(O.TREASURE, 28, 26, 72)
    heli(29, 26, 73)
    w.place(O.EGG, 27, 26, 71)
    island(31, 45, 41, 54, 18)                                          # the hut's island
    rock(w, 34, 18, 47, 38, 21, 52)
    carve(w, 35, 18, 48, 37, 20, 51)
    carve(w, 34, 18, 49, 34, 19, 50)                                    # its door, west
    w.place(O.EXIT, 36, 18, 50)
    heli(40, 18, 46)
    heli(40, 18, 53)
    w.place(O.EGG, 32, 18, 46)
    # Ledges round the walls with eggs and spare helicopters.
    for x0, z0, x1, z1, walk, eggs, helis in (
            (10, 22, 13, 26, 30, [(11, 23)], [(12, 25), (11, 25)]),
            (66, 22, 69, 26, 20, [], [(67, 25), (68, 25)]),
            (10, 72, 13, 78, 10, [(11, 77)], [(12, 73), (12, 75), (11, 74)]),
            (66, 74, 69, 78, 28, [(67, 77)], [(68, 75), (67, 75)]),
            (48, 20, 52, 21, 34, [], [(50, 20)])):
        island(x0, z0, x1, z1, walk)
        for x, z in eggs:
            w.place(O.EGG, x, walk, z)
        for x, z in helis:
            heli(x, walk, z)

    # ---- the roof's mines and the birds ----
    for x in range(20, 61, 4):
        for z in (36, 64):
            mine(w, x, ROOF - 1, z, hanging=True)
    bird((22, 22, 40), (40, 22, 40), 48)
    bird((50, 18, 30), (50, 18, 50), 40)
    bird((30, 28, 60), (55, 28, 60), 56)
    bird((60, 16, 40), (60, 16, 52), 32)
    bird((18, 12, 62), (36, 12, 62), 40)
    bird((44, 26, 44), (44, 26, 30), 40)
    bird((36, 34, 50), (56, 34, 50), 48)
    bird((24, 18, 52), (24, 18, 66), 32)
    for start, end in (((57, 26, 30), (57, 30, 30)), ((47, 34, 60), (47, 37, 60))):
        if not calm:
            w.place(O.MOVING_MINE, *start, end=end, advance=30, recede=30, stop_start=10, stop_end=10)

    # ---- the east passage and chamber (walk 20) ----
    # From a ledge on the east wall a passage three wide and three high: mines hang from its roof
    # at Blupi's head (walk in, never jump), two curtains of lightning across it; the chamber: a
    # bulldozer pushing to and fro, a wasp, treasure 7, two eggs, two helicopters.
    # The ledge reaches the passage's mouth at x 70, the hall's last column (it stopped at 69, a
    # gap of one into the bottomless hall).
    island(64, 44, 70, 52, 20)
    carve(w, 70, 20, 46, 76, 22, 50)
    for x in range(70, 77):
        for z in (46, 50) if x % 2 == 0 else (48,):
            mine(w, x, 21, z, hanging=True)
    for x in (72, 75):
        for z in range(46, 51):
            w.set(x, 23, z, EMITTER)
            if not calm:
                w.fill(x, 20, z, x, 22, z, BLITZ)
    rock(w, 74, 16, 38, 90, 28, 58)
    carve(w, 77, 20, 40, 88, 25, 56)
    carve(w, 74, 20, 46, 76, 22, 50)
    w.place(O.TREASURE, 86, 20, 48)
    w.place(O.EGG, 79, 20, 41)
    w.place(O.EGG, 87, 20, 55)
    heli(80, 20, 54)
    heli(82, 20, 54)
    if not calm:
        w.place(O.BULLDOZER, 79, 20, 44, end=(86, 20, 44), advance=60, recede=60, stop_start=20,
                stop_end=20)
        w.place(O.WASP, 84, 21, 42, end=(84, 21, 54), advance=40, recede=40, stop_start=8, stop_end=8)

    dress(w, 6, 4, 16, 90, ROOF + 4, 84, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
