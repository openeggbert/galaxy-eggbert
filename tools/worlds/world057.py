#!/usr/bin/env python3
"""Mission 057, the slime tree (worlds3d/world057.vwr): brief plans/briefs/world057.md.

Mobile Eggbert's level 057 is a station of nineteen enemy helicopters (ObjectType32), each
dropping a shot at every turn, and three tanks (ObjectType33): from the start a gallery under
seven of them in a row; two lifts stacked in a shaft past three more, eggs on the ledge between;
a corridor with a tank; a hall where two treasures lie on a pillar of slime under two
helicopters, a lift either side; the treasure door "2"; a corridor with a tank to a ferry of six
lifts over water, each pair meeting the next (100 ticks, stop 20), a tank on the far shelf; up to
the drink (30: unseen for twenty seconds); a gallery above the last room, where the third
treasure and the red arrow stand on top of a slime tree under seven helicopters, and a lollipop
(ObjectType26) for the jump up onto it. Here: the start in the south-east, the gallery heading
north, the lift shaft, the corridor west at the top, the hall, the door, the corridor on west, the
ferry room, a stair up to the drink, the gallery east and the drop into the room of the tree.

    python3 tools/worlds/world057.py                   # writes worlds3d/world057.vwr
    python3 tools/worlds/world057.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which
        counts no treasures and has no lollipop): 1 the helicopters and tanks passed between
        shots (left out), 2 the door "2" open, 3 the lollipop's jump (a stand-in lift)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift  # noqa: E402
from station import (BALL_MACHINES, GIRDER, GRILLE, WATER, metal, slime,  # noqa: E402
                     treasure_door)

SPAWN = (77, 10, 84)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(57)
    w = World(mission=57, sky=10)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def goo(x0, y0, z0, x1, y1, z1):
        slime(w, x0, y0, z0, x1, y1, z1, rng)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def room(x0, z0, x1, z1, walk, top):
        """Plates round the inside x0..x1, z0..z1 from walk - 1 (floor) to `top` (roof)."""
        plates(x0 - 1, walk - 2, z0 - 1, x1 + 1, top, z1 + 1)
        carve(x0, walk, z0, x1, top - 1, z1)

    def enemy(otype, a, b, ticks, stop):
        if stage < 1:
            w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def heli(x, y, z, dx=1, dz=0):
        """An enemy helicopter hovering a cell to and fro (8 ticks, stop 25: the original's); it
        drops a shot straight down at each turn."""
        enemy(O.ROBOT, (x, y, z), (x + dx, y, z + dz), 8, 25)

    # ---- S, the start (inside x 73-81, z 78-86, walk 10) --------------------------------------
    room(73, 78, 81, 86, 10, 16)
    w.spawn = SPAWN
    for x, z, k in ((74, 85, 0), (80, 85, 2)):
        w.set(x, 10, z, 82)
        w.bigdecor(BALL_MACHINES[k], x, 11, z)

    # ---- G, the gallery of the seven helicopters (inside x 76-78, z 62-77, walk 10) ------------
    room(76, 62, 78, 77, 10, 17)
    carve(76, 10, 78, 78, 12, 78)
    # Seven helicopters over the middle lane at y 14, each swinging a cell across: their shots
    #    fall on every lane in turn; walk between them.
    for k, z in enumerate((75, 73, 71, 69, 67, 65, 63)):
        heli(76 if k % 2 == 0 else 78, 14, z, dx=1 if k % 2 == 0 else -1)
    for z in range(63, 77, 3):
        w.set(75, 13, z, GRILLE)
        w.set(79, 13, z, GRILLE)

    # ---- E, the lift shaft (inside x 73-83, z 50-60, walk 10 up to 20) -------------------------
    room(73, 50, 83, 60, 10, 25)
    carve(76, 10, 61, 78, 12, 61)
    # L1 rises six (60 ticks, the original's) from the floor to the ledge (walk 16); L2 four (40
    #    ticks) from the ledge to the top platform (walk 20). Both wait 20 ticks at each end.
    w.clear(78, 9, 57)
    lift(w, (78, 9, 57), (78, 15, 57), ticks=60, stop=20)
    plates(73, 14, 53, 83, 15, 55)                     # the ledge, walk 16
    w.clear(78, 15, 54)
    lift(w, (78, 15, 54), (78, 19, 54), ticks=40, stop=20)
    plates(73, 18, 50, 83, 19, 52)                     # the top platform, walk 20
    for x, z in ((73, 53), (74, 55), (82, 55), (83, 53)):
        w.place(O.EGG, x, 16, z)
    heli(74, 19, 54, dz=-1)                            # over the ledge's ends
    heli(82, 19, 54, dz=-1)
    heli(76, 14, 58)                                   # in the shaft beside L1

    # ---- C2, the corridor west (x 62-72, z 50-52, walk 20) -------------------------------------
    plates(61, 17, 49, 72, 19, 53)
    plates(61, 20, 49, 72, 23, 49)
    plates(61, 20, 53, 72, 23, 53)
    plates(61, 24, 49, 72, 24, 53)
    carve(72, 20, 50, 72, 22, 52)
    # A tank swings across it (along Z, 10 ticks, stop 23): it fires across the corridor at each
    #    turn; cross its line between shots.
    enemy(O.CREATURE, (67, 20, 50), (67, 20, 52), 10, 23)

    # ---- H, the hall of the slime pillar (inside x 46-60, z 44-56, floor walk 14) --------------
    room(46, 44, 60, 56, 14, 27)
    carve(61, 20, 50, 61, 22, 52)                      # where the corridor comes in
    plates(58, 14, 48, 60, 19, 54)                     # the east balcony, walk 20
    # The pillar of slime (x 51-56, z 48-54, top walk 20), one cell from the balcony: jump
    #    across; two treasures on it, two helicopters over it.
    goo(51, 13, 48, 56, 19, 54)
    for x, z in ((51, 48), (56, 54)):
        goo(x, 20, z, x, 20, z)                        # slime bumps on its corners
    w.place(O.TREASURE, 52, 20, 50)
    w.place(O.TREASURE, 55, 20, 52)
    heli(53, 23, 50)
    heli(55, 23, 52, dx=-1)
    # The west ledge (x 46-49, z 49-53, walk 20) and two lifts from the floor: the east one up to
    #    the balcony, the west one up to the ledge (60 ticks, stop 20): a fall from the pillar
    #    is not the end.
    plates(46, 14, 49, 49, 19, 53)
    w.clear(57, 13, 56)
    lift(w, (57, 13, 56), (57, 19, 56), ticks=60, stop=20)
    w.clear(50, 13, 46)
    lift(w, (50, 13, 46), (50, 19, 46), ticks=60, stop=20)
    plates(46, 14, 46, 49, 19, 48)                     # the west ledge reaches the lift
    plates(58, 14, 55, 60, 19, 56)                     # the balcony reaches the east lift
    w.clear(57, 19, 56)
    # The treasure door "2" in the west wall at the floor (x 45, z 54-56, two high).
    for z in (54, 55, 56):
        for y in (14, 15):
            w.set(45, y, z, 0 if stage >= 2 else treasure_door(2))
    for x, z, k in ((47, 45, 0), (59, 45, 1)):
        w.bigdecor(BALL_MACHINES[k + 1], x, 14, z)

    # ---- W, the corridor west (x 34-44, z 54-56, walk 14) --------------------------------------
    plates(34, 12, 53, 44, 13, 57)
    plates(34, 14, 53, 44, 18, 53)
    plates(34, 14, 57, 44, 18, 57)
    plates(34, 18, 53, 44, 18, 57)
    for x in range(35, 45, 3):
        w.set(x, 17, 54, GIRDER)
        w.set(x, 17, 56, GIRDER)
    enemy(O.CREATURE, (40, 14, 54), (40, 14, 56), 10, 23)

    # ---- F, the ferry room (inside x 2-33, z 48-59, walk 14) ----------------------------------
    room(2, 48, 33, 59, 14, 22)
    carve(33, 14, 54, 33, 16, 56)
    carve(34, 14, 54, 34, 16, 56)
    # The water pit (x 9-28, wall to wall) three below the shelves, its surface at y 11; a bank
    #    at its east end to climb out (one above the water, then a jump onto the shelf).
    carve(9, 11, 48, 28, 13, 59)
    w.fill(9, 10, 48, 28, 10, 59, 82)
    w.fill(9, 11, 48, 28, 11, 59, WATER)
    plates(28, 11, 48, 28, 12, 50)                     # the bank, walk 13
    # The ferry: three pairs of lifts, each 2 x 3, all 100 ticks (the original's), in step: A
    #    from the east shelf west, B east to meet it, C west to the far shelf from where B starts.
    #    Step from A onto B where they meet, from B onto C where they start. They wait 40 ticks
    #    where they meet (the original's 20 is a second to judge a step across in 3D).
    for x0, x1 in ((27, 22), (16, 20), (14, 9)):
        for dx in (0, 1):
            for z in (52, 53, 54):
                lift(w, (x0 + dx, 13, z), (x1 + dx, 13, z), ticks=100, stop=40)
    # The far shelf (x 2-8, walk 14): a tank swinging along it, firing along the ferry's lanes.
    enemy(O.CREATURE, (3, 14, 53), (7, 14, 53), 60, 23)
    w.place(O.EGG, 2, 14, 58)
    w.place(O.EGG, 3, 14, 58)

    # ---- The stair up to the drink (x 3-5, z 39-47, walk 14 up to 26) --------------------------
    plates(2, 12, 39, 6, 13, 47)
    plates(2, 14, 38, 2, 29, 47)
    plates(6, 14, 38, 6, 29, 47)
    carve(3, 14, 47, 5, 16, 47)
    for k, z in enumerate((45, 44, 43, 42, 41, 40)):
        plates(3, 14, z, 5, 15 + 2 * k, z)
    plates(3, 14, 39, 5, 25, 39)
    plates(2, 30, 38, 6, 30, 47)

    # ---- D, the drink room (inside x 2-10, z 30-38, walk 26) ----------------------------------
    room(2, 30, 10, 37, 26, 31)
    carve(3, 26, 38, 5, 28, 38)
    w.place(O.DRINK, 6, 26, 33)
    w.place(O.EGG, 2, 26, 30)

    # ---- The gallery east (x 11-47, z 32-34, walk 26), girder columns along it ----------------
    plates(11, 24, 31, 47, 25, 35)
    plates(11, 26, 31, 47, 30, 31)
    plates(11, 26, 35, 47, 30, 35)
    plates(11, 30, 31, 47, 30, 35)
    carve(11, 26, 32, 11, 28, 34)
    for x in range(14, 46, 4):
        for y in range(26, 30):
            w.set(x, y, 32, GIRDER)
            w.set(x, y, 34, GIRDER)

    # ---- X, the room of the tree (inside x 46-60, z 28-42, floor walk 14) ---------------------
    room(46, 28, 60, 42, 14, 31)
    carve(45, 26, 32, 46, 28, 34)                      # the gallery comes in at the top
    plates(46, 24, 31, 48, 25, 35)                     # a ledge, then the drop to the floor
    # The tree: a pillar of slime (x 51-55, z 34-38, top walk 18), four above the floor (a jump
    #    is two; licked, it clears four): treasure 3 and the red arrow on it between slime bumps,
    #    seven helicopters hovering over it.
    goo(51, 13, 34, 55, 17, 38)
    goo(51, 18, 36, 51, 18, 36)
    goo(55, 18, 36, 55, 18, 36)
    w.place(O.TREASURE, 52, 18, 36)
    w.place(O.EXIT, 54, 18, 36)
    for k, (x, z) in enumerate(((51, 34), (53, 34), (55, 34), (52, 36), (54, 38), (51, 38), (55, 38))):
        heli(x, 22, z, dx=1 if k % 2 == 0 else -1)
    if stage >= 3:                     # levelcheck has no lollipop: a stand-in lift up
        w.place(O.LIFT, 53, 13, 39, end=(53, 17, 39))
    # The lollipop in an alcove in the east wall with two eggs, an egg by the tree.
    carve(61, 14, 39, 62, 16, 41)
    plates(63, 13, 38, 63, 17, 42)
    plates(61, 17, 38, 62, 17, 42)
    plates(61, 13, 38, 62, 13, 42)
    plates(61, 14, 38, 62, 16, 38)
    plates(61, 14, 42, 62, 16, 42)
    w.place(O.LOLLIPOP, 62, 14, 40)
    w.place(O.EGG, 62, 14, 39)
    w.place(O.EGG, 62, 14, 41)
    w.place(O.EGG, 56, 14, 37)

    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world057.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
