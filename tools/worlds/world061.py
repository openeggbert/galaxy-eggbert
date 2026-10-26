#!/usr/bin/env python3
"""Mission 061, the slime chasm (worlds3d/world061.vwr): brief plans/briefs/world061.md.

Mobile Eggbert's level 061: a cave of green slime round Blupi's start, where the red arrow waits
beside him but counts only with all seven treasures. Treasure doors (421 + n - 1 opens once n
treasures are taken) gate the ways round it: a ladder of three shuttle lifts moving in step, a
chain of lifts (some moving across and up at once) over the deadly slime (347), a tall lift,
lava crossed on machine blocks, and two big creatures that kill while they rest at the ends of
their walk and are harmless while they walk. Here the cave becomes rooms round the start
chamber: west a shaft climbed on the shuttles (treasure 1), north a chasm crossed on the lift
chain between slime islands (treasures 2-3; a fall means the climb back) to a tower lift (treasure 4),
east a lava hall (treasure 5), south down a stair the creatures' halls (treasures 6-7).

    python3 tools/worlds/world061.py                 # writes worlds3d/world061.vwr
    python3 tools/worlds/world061.py --stage K out   # K treasures taken: the doors needing at
                                                     # most K open, and as floors the lifts the
                                                     # next treasures need (for levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FLOOR, O, World, lift, mine  # noqa: E402
from slime import skin, slime  # noqa: E402

LAVA = 68
WATER = 92


def door(n):
    return 420 + n               # opens once n treasures are taken (Decor::OpenDoorsTresor)


# Rooms: interiors (inclusive), shells two thick of slime round them.
ROOMS = {
    "S": (43, 15, 43, 57, 20, 57),        # the start chamber, walk 15
    "W": (37, 15, 48, 42, 17, 52),        # its corridor west
    "A": (25, 15, 42, 36, 28, 58),        # the shuttle shaft
    "P": (24, 11, 21, 76, 30, 40),        # the chasm over the deadly slime
    "T": (70, 33, 20, 76, 36, 27),        # the tower room, walk 33
    "H": (60, 15, 42, 76, 27, 58),        # the lava hall, walk 15
    "N": (47, 9, 62, 56, 13, 66),         # the creatures' landing, walk 9
    "C1": (35, 9, 64, 46, 10, 64),        # creature 1's corridor
    "V1": (30, 9, 62, 34, 13, 66),        # treasure 6's chamber
    "SW": (30, 9, 68, 53, 13, 74),        # the lava room
    "C2": (54, 9, 71, 65, 10, 71),        # creature 2's corridor
    "V2": (66, 9, 68, 70, 13, 74),        # treasure 7's chamber
}


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world061.vwr"
    rng = random.Random(61)
    w = World(mission=61, sky=31)

    for x0, y0, z0, x1, y1, z1 in ROOMS.values():
        slime(w, x0 - 2, y0 - 2, z0 - 2, x1 + 2, y1 + 2, z1 + 2)
    for x0, y0, z0, x1, y1, z1 in ROOMS.values():
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def opening(x0, y0, z0, x1, y1, z1, bar=None):
        """Carve a passage through a shell; with `bar` a treasure door fills its middle column."""
        w.fill(x0, y0, z0, x1, y1, z1, 0)
        if bar is not None and (stage is None or bar > stage):
            for x in range(x0, x1 + 1):
                for y in range(y0, y1 + 1):
                    for z in range(z0, z1 + 1):
                        w.set(x, y, z, door(bar))

    # S, the start chamber: the red arrow in view, an egg cache behind a seven-treasure door.
    w.place(O.EXIT, 50, 15, 45)
    slime(w, 54, 15, 43, 57, 17, 45)
    w.fill(55, 15, 44, 57, 16, 44, 0)
    opening(54, 15, 44, 54, 16, 44, bar=7)
    for x in (55, 56, 57):
        w.place(O.EGG, x, 15, 44)
    # A: the shaft west; three shuttles along Z moving in step, each a two-block jump above the
    # last and one column further west, then the ledge (walk 23) with treasure 1.
    opening(41, 15, 49, 42, 17, 51)                                   # S -> W
    opening(35, 15, 49, 36, 17, 51)                                   # W -> A
    slime(w, 25, 15, 42, 28, 22, 58)                                  # the ledge
    for x, y in ((33, 16), (31, 18), (29, 20)):
        if stage is not None and stage >= 0:
            w.fill(x, y, 45, x + 1, y, 55, MACHINE_FLOOR)             # the ladder as floors
        else:
            for dx in (0, 1):
                lift(w, (x + dx, y, 45), (x + dx, y, 55), ticks=24, stop=3)
    w.place(O.TREASURE, 26, 23, 45)
    for z in (50, 53, 56):
        w.place(O.EGG, 34, 15, z)
    # The way north out of the ledge, behind door "1".
    opening(26, 23, 41, 26, 24, 41, bar=1)
    w.fill(26, 23, 39, 26, 24, 40, 0)

    # P: the chasm, its floor twelve blocks below the landing G under the door (a fall means
    # climbing back up the stair beside G); islands I1 and I2, the east ledge E (walk 25); lifts
    # between them: two shuttles meeting in the middle, one moving across in X and Z, one rising
    # as it goes.
    slime(w, 24, 9, 21, 76, 10, 40)
    slime(w, 24, 10, 30, 29, 22, 40)                                  # G
    for i, x in enumerate(range(35, 29, -1)):                         # the stair back up, 2 a step
        slime(w, x, 11, 38, x, 12 + 2 * i, 40)
    slime(w, 42, 10, 30, 45, 22, 33)                                  # I1
    slime(w, 56, 10, 25, 59, 22, 28)                                  # I2
    slime(w, 68, 10, 21, 76, 24, 40)                                  # E
    if stage is not None and stage >= 1:
        w.fill(30, 22, 32, 41, 22, 32, MACHINE_FLOOR)
        w.fill(46, 22, 31, 55, 22, 31, MACHINE_FLOOR)
        w.fill(55, 22, 27, 55, 22, 30, MACHINE_FLOOR)
        w.fill(60, 22, 27, 63, 22, 27, MACHINE_FLOOR)
        w.fill(64, 23, 27, 65, 23, 27, MACHINE_FLOOR)
        w.fill(66, 24, 27, 67, 24, 27, MACHINE_FLOOR)
    else:
        # The two meet side by side: the original's 3-tick dwell is too short to step across in
        # 3D, so they wait 30 ticks there (D43).
        lift(w, (30, 22, 32), (35, 22, 32), ticks=18, stop=30)
        lift(w, (41, 22, 32), (36, 22, 32), ticks=18, stop=30)
        lift(w, (46, 22, 31), (55, 22, 26), ticks=36, stop=3)
        lift(w, (60, 22, 27), (67, 24, 27), ticks=24, stop=3)
    w.place(O.TREASURE, 43, 23, 31)
    w.place(O.TREASURE, 58, 23, 26)
    mine(w, 59, 23, 25)                                               # off the way across I2
    for x, z in ((26, 31), (28, 33), (44, 33), (56, 25)):
        w.place(O.EGG, x, 23, z)
    # T: the tower over E, up its lift (walk 25 -> 33), treasure 4.
    w.fill(74, 24, 24, 74, 32, 24, 0)
    lift(w, (74, 24, 24), (74, 32, 24), ticks=36, stop=3)
    w.place(O.TREASURE, 71, 33, 21)
    w.place(O.EGG, 75, 33, 21)

    # E -> H: a doorway out of E's south side behind door "4", a ledge, the drop into the hall.
    opening(72, 25, 41, 72, 26, 41, bar=4)
    slime(w, 70, 15, 42, 76, 24, 44)                                  # the ledge in H
    # H: two lava trenches across the hall, the second crossed on machine blocks; treasure 5.
    w.fill(60, 14, 48, 76, 14, 48, LAVA)
    w.fill(60, 14, 52, 76, 14, 53, LAVA)
    w.fill(66, 14, 52, 67, 14, 53, 10)
    w.place(O.TREASURE, 74, 15, 56)
    mine(w, 64, 15, 56)
    mine(w, 70, 15, 50)
    for x in (62, 66, 72):
        w.place(O.EGG, x, 15, 46)
    # H -> S behind door "5".
    opening(58, 15, 50, 59, 16, 50, bar=5)

    # S -> D behind door "5", a stair of two-block steps down south.
    opening(50, 15, 58, 50, 16, 58, bar=5)
    w.fill(49, 15, 59, 51, 17, 61, 0)
    slime(w, 49, 12, 60, 51, 12, 60)
    slime(w, 49, 10, 61, 51, 10, 61)
    w.fill(49, 13, 60, 51, 17, 60, 0)
    w.fill(49, 11, 61, 51, 17, 61, 0)
    w.fill(49, 9, 62, 51, 14, 62, 0)
    # D: the creatures' halls (walk 9). From the landing N a corridor one wide runs west to
    # treasure 6's chamber; creature 1 walks along it and rests at its two ends, where it kills.
    # Blupi gets past it while it walks. The chamber opens south into the lava room (machine
    # blocks to step on, a pool), whose far side leads east along creature 2's corridor to
    # treasure 7's chamber.
    w.fill(46, 9, 64, 46, 10, 64, 0)
    w.fill(34, 9, 67, 34, 10, 67, 0)                                  # V1 -> SW
    w.fill(32, 9, 67, 33, 10, 67, 0)
    w.fill(53, 9, 71, 53, 10, 71, 0)                                  # SW -> C2
    w.fill(66, 9, 71, 66, 10, 71, 0)                                  # C2 -> V2
    if stage is None or stage < 5:
        w.place(O.BIG_CREATURE, 45, 9, 64, end=(35, 9, 64), advance=80, recede=80, stop_start=200, stop_end=200)
        w.place(O.BIG_CREATURE, 54, 9, 71, end=(65, 9, 71), advance=100, recede=100, stop_start=200,
                stop_end=200)
    w.fill(36, 8, 69, 44, 8, 70, LAVA)
    for x in (38, 41):
        w.fill(x, 8, 69, x, 8, 70, 10)
    w.fill(31, 8, 72, 35, 8, 74, WATER)
    w.place(O.TREASURE, 31, 9, 63)                                     # treasure 6
    w.place(O.TREASURE, 69, 9, 73)                                     # treasure 7
    w.place(O.MOVING_MINE, 48, 9, 69, end=(48, 9, 73), advance=10, recede=10, stop_start=0, stop_end=0)
    mine(w, 51, 9, 68)
    for x, z in ((49, 64), (55, 63), (40, 72), (68, 69)):
        w.place(O.EGG, x, 9, z)

    skin(w, rng)
    w.spawn = (50, 15, 54)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
