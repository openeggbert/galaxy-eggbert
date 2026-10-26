#!/usr/bin/env python3
"""Mission 024, the halls of mines (worlds3d/world024.vwr): brief plans/briefs/world024.md.

Mobile Eggbert's level 024: Blupi starts beside the red arrow, shut in behind a red, a green and
a blue door, with a crate in the room. A staircase climbs to rooms of mines hanging overhead and
the blue key; a cage of blue bars holds lifts one above the other, the red key at its top; a
long hall over lava has balustrades and small lifts to cross it, mines on its way and the green
key at its end; a flooded part of the palace is swum through past a fish to a tower of lifts
with the five treasures, one above the other. Here a start room with the arrow in a shrine
behind the three doors in a row; north a stairwell of two-block steps to the blue key past
hanging mines; east a cage of bars and five lifts (the original's timings) to the red key;
west the lava hall of balustrades and bobbing lifts to the green key; south the flooded
gallery, dived under a curtain past the fish, to the treasure tower of shelves and two lifts.

    python3 tools/worlds/world024.py                   # writes worlds3d/world024.vwr
    python3 tools/worlds/world024.py --stage K out.vwr # lessons 1..K solved (levelcheck):
        1 crates pushed (start room, tower foot), 2 the shrine's doors open,
        3 the flooded gallery swum (its surface made a floor, the curtain gone)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from palace import (BALUSTRADE, BLUE_DOOR, DEEP_WATER, FOUNTAIN, GREEN_DOOR, LAVA, RED_DOOR,  # noqa: E402
                    SAW, STONE, WATER, WINDOWS, gate, parapet, stone, windows_row)

BARS = 51
SPAWN = (50, 11, 59)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(24)
    w = World(mission=24, sky=28)

    def white(x0, y0, z0, x1, y1, z1, chance=0.2):
        stone(w, x0, y0, z0, x1, y1, z1, rng, chance=chance)

    def crate(start, solved):
        w.place(O.CRATE, *(solved if stage >= 1 else start))

    # The start room (walk 11), walls five high, a way out on each side.
    white(41, 10, 39, 59, 10, 61, chance=0.1)                       # floor, doorways included
    white(41, 11, 39, 59, 16, 39)                                   # walls six high: the
    white(41, 11, 61, 59, 16, 61)                                   # corner ledge is no
    white(41, 11, 39, 41, 16, 61)                                   # stair onto them
    white(59, 11, 39, 59, 16, 61)
    windows_row(w, 42, 58, 13, 61, rng, chance=0.6)
    w.fill(49, 11, 39, 51, 16, 39, 0)                               # north: the stairwell, open
    w.fill(59, 11, 49, 59, 13, 51, 0)                               # east: the cage
    w.fill(41, 11, 49, 41, 13, 51, 0)                               # west: the lava hall
    w.fill(44, 11, 61, 46, 13, 61, 0)                               # south: the flooded gallery
    # The shrine: the red, green and blue doors in a row from the south, the red arrow inside
    #    on its open court with a fountain.
    white(47, 11, 49, 53, 13, 56, chance=0.3)
    w.fill(48, 11, 50, 52, 13, 53, 0)
    for zz in (54, 55, 56):
        w.clear(50, 11, zz)
    if stage < 2:
        gate(w, 50, 11, 56, RED_DOOR)
        gate(w, 50, 11, 55, GREEN_DOOR)
        gate(w, 50, 11, 54, BLUE_DOOR)
    w.place(O.EXIT, 50, 11, 52)
    w.bigdecor(FOUNTAIN, 50, 11, 50)
    # An egg on a ledge three high in the north-west corner, climbed from the room's crate.
    white(42, 11, 40, 44, 13, 42, chance=0.3)
    w.place(O.EGG, 43, 14, 41)
    crate((45, 11, 46), (45, 11, 42))                                # pushed -Z to the ledge

    # North: the stairwell (x 49-51), eight two-block steps up to walk 27, saws on two of them;
    #    the top room with a corridor under hanging mines (walk on, do not jump), the blue key
    #    beside a mine at its end.
    white(48, 11, 14, 48, 30, 38)
    white(52, 11, 14, 52, 30, 38)
    for k in range(1, 9):
        z0 = 39 - 3 * k
        white(49, 10, z0, 51, 10 + 2 * k, z0 + 2, chance=0.1)
    w.set(51, 16, 30, SAW)                                          # step 3's front row
    w.set(49, 22, 21, SAW)                                          # step 6's front row
    mine(w, 51, 25, 18)                                             # on step 7's back row
    white(49, 26, 14, 51, 26, 14, chance=0.1)                       # the top step to the room
    w.place(O.EGG, 49, 19, 27)
    white(44, 26, 3, 56, 26, 13, chance=0.1)
    white(44, 27, 3, 44, 30, 13)
    white(56, 27, 3, 56, 30, 13)
    white(44, 27, 3, 56, 30, 3)
    white(44, 27, 13, 47, 30, 13)
    white(53, 27, 13, 56, 30, 13)
    white(48, 27, 5, 48, 30, 12)                                    # the corridor's walls
    white(52, 27, 5, 52, 30, 12)
    for zz in (11, 8):
        for x in (49, 50, 51):
            w.set(x, 29, zz, STONE)                                 # beams overhead
            mine(w, x, 28, zz, hanging=True)
    w.place(O.KEY3, 50, 27, 4)
    mine(w, 45, 27, 5)
    w.place(O.MOVING_MINE, 54, 27, 5, end=(54, 27, 11), advance=40, recede=40, stop_start=0, stop_end=0)
    w.place(O.EGG, 46, 27, 11)

    # East: the yard (walk 11) and the cage of bars; inside it ledges on the west and east sides
    #    one above the other and five lifts between them, each carrying Blupi across to the
    #    next ledge two higher (the original's timings); the red key on the top ledge.
    white(60, 10, 42, 74, 10, 58, chance=0.1)
    parapet(w, [(74, z) for z in range(42, 59)] + [(x, 42) for x in range(60, 75)] +
            [(x, 58) for x in range(60, 75)], 11, rng)
    for y in (13, 17, 21):
        for x in range(61, 72):
            w.set(x, y, 46, BARS)
            w.set(x, y, 54, BARS)
    for x, zz in ((61, 46), (71, 46), (61, 54), (71, 54)):
        white(x, 11, zz, x, 24, zz, chance=0.4)                     # the cage's corner posts
    for y in (12, 16, 20):
        white(62, y, 48, 63, y, 52, chance=0.1)                     # west ledges, walk 13/17/21
    for y in (14, 18, 22):
        white(69, y, 48, 70, y, 52, chance=0.1)                     # east ledges, walk 15/19/23
    lift(w, (64, 12, 49), (68, 12, 49), ticks=30, stop=10)
    lift(w, (68, 14, 51), (64, 14, 51), ticks=30, stop=10)
    w.place(O.LIFT, 64, 16, 49, end=(68, 16, 49), advance=15, recede=66, stop_start=2, stop_end=2)
    lift(w, (68, 18, 51), (64, 18, 51), ticks=60, stop=20)
    lift(w, (64, 20, 49), (68, 20, 49), ticks=18, stop=3)
    w.set(62, 16, 48, SAW)
    mine(w, 66, 11, 44)
    w.place(O.KEY1, 69, 23, 50)
    w.place(O.EGG, 69, 23, 52)
    w.place(O.EGG, 62, 17, 52)

    # West: the lava hall (x 12-33, lava four below), open on top; balustrades in the lava and
    #    small lifts rising and sinking between them; hanging mines over its entry, a mine on a
    #    balustrade, a mine moving up and down beside a jump; the green key at its end.
    white(34, 10, 46, 40, 10, 54, chance=0.1)                       # the entry ledge
    for zz in range(46, 55):
        w.set(37, 13, zz, STONE)                                    # a beam across the entry
    for zz in (49, 50, 51):
        mine(w, 37, 12, zz, hanging=True)
    w.set(35, 10, 53, SAW)
    w.fill(12, 6, 46, 33, 6, 54, LAVA)
    white(11, 6, 45, 40, 14, 45)
    white(11, 6, 55, 40, 14, 55)
    white(11, 6, 45, 11, 14, 55)
    # Piers two wide, between them a gap of two with a lift rising into its first cell:
    #    step on it when it is up, jump the second cell (the original's 20-tick lifts). The
    #    balustrades run along the piers' edges: decor Blupi walks through (D41), not a floor.
    for x0 in (31, 27, 23, 19):
        for x in (x0, x0 + 1):
            for zz in range(48, 53):
                w.set(x, 10, zz, STONE)
            for zz in (48, 52):
                w.set(x, 11, zz, BALUSTRADE)
    for x in (30, 26, 22):
        lift(w, (x, 8, 50), (x, 10, 50), ticks=20, stop=10)
    mine(w, 24, 11, 52)
    w.place(O.MOVING_MINE, 25, 11, 52, end=(25, 13, 52), advance=20, recede=20, stop_start=0, stop_end=0)
    white(12, 10, 47, 17, 10, 53, chance=0.1)                       # the far platform
    w.place(O.KEY2, 13, 11, 50)
    w.place(O.EGG, 13, 11, 48)
    w.place(O.EGG, 28, 11, 49)

    # South: the flooded gallery (x 43-47, z 62-76): water up to the room's floor, a curtain
    #    wall across it to dive under, the fish patrolling beneath; then the treasure tower.
    white(42, 3, 62, 48, 3, 76, chance=0.1)
    white(42, 4, 62, 42, 14, 76)
    white(48, 4, 62, 48, 14, 76)
    w.fill(43, 4, 62, 47, 9, 76, DEEP_WATER)
    w.fill(43, 10, 62, 47, 10, 76, WATER)
    white(42, 4, 77, 48, 9, 77, chance=0.1)                         # under the landing's edge
    white(42, 9, 65, 48, 14, 65, chance=0.3)                        # the curtain, open below y 9
    w.fill(43, 4, 65, 47, 8, 65, DEEP_WATER)
    w.place(O.FISH, 44, 7, 67, end=(44, 7, 75), advance=60, recede=60, stop_start=20, stop_end=20)
    if stage >= 3:                      # levelcheck cannot swim: the surface stands in for it
        w.fill(43, 10, 62, 47, 10, 76, STONE)
        w.fill(42, 11, 65, 48, 14, 65, 0)
    # The treasure tower: a white core with shelves round it, climbed from the crate, by jumps
    #    and two fast lifts (24 and 12 ticks as the original's), a treasure on every shelf.
    white(38, 10, 77, 54, 10, 90, chance=0.1)                       # the landing, walk 11
    parapet(w, [(38, z) for z in range(77, 91)] + [(54, z) for z in range(77, 91)] +
            [(x, 90) for x in range(38, 55)], 11, rng)
    white(44, 11, 78, 48, 25, 83, chance=0.3)                       # the core
    for y in (14, 18, 22):
        for x in range(44, 49):
            if rng.random() < 0.6:
                w.set(x, y, 83, WINDOWS)
    white(44, 11, 84, 48, 13, 85, chance=0.2)                       # shelf 1 (south), walk 14
    crate((46, 11, 88), (46, 11, 86))                               # pushed -Z under shelf 1
    w.place(O.TREASURE, 45, 14, 85)
    white(49, 15, 79, 50, 15, 85, chance=0.1)                       # shelf 2 (east), walk 16
    w.place(O.TREASURE, 50, 16, 80)
    w.place(O.EGG, 50, 16, 84)
    lift(w, (50, 15, 78), (50, 19, 78), ticks=24, stop=3)           # up to shelf 3
    white(43, 19, 76, 50, 19, 77, chance=0.1)                       # shelf 3 (north), walk 20
    w.place(O.TREASURE, 45, 20, 76)
    w.set(47, 19, 77, SAW)
    lift(w, (42, 19, 77), (42, 21, 77), ticks=12, stop=3)           # up to shelf 4
    white(42, 21, 78, 43, 21, 83, chance=0.1)                       # shelf 4 (west), walk 22
    w.place(O.TREASURE, 42, 22, 81)
    w.place(O.EGG, 43, 22, 83)
    white(42, 23, 84, 46, 23, 86, chance=0.1)                       # shelf 5 (top), walk 24
    w.place(O.TREASURE, 45, 24, 85)
    w.place(O.EGG, 43, 24, 86)
    w.place(O.MOVING_MINE, 40, 11, 79, end=(40, 11, 88), advance=50, recede=50, stop_start=0, stop_end=0)
    w.place(O.EGG, 52, 11, 88)
    mine(w, 52, 11, 79)

    w.spawn = SPAWN
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world024.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
