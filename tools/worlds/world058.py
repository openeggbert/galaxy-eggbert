#!/usr/bin/env python3
"""Mission 058, the lava mine and world 5's golden key (worlds3d/world058.vwr): brief
plans/briefs/world058.md.

Mobile Eggbert's level 058 is world 5's last test, in a mine lit by lava: Blupi starts in a room
whose way on is shut by the red door (334); the red key (ObjectType49) lies in a hall above it
on stepping stones over lava, over the corridor beyond the door, where mines lie and bob; a great cave of slime islands follows, enemy tanks (ObjectType33)
on them and in a tunnel to the first treasure, drinks (30) to pass them unseen, and on an island
in its middle the golden key (ObjectType21), which counts only with all ten treasures; east of
it a corridor of tanks, a room where five crates (12) plug a tunnel to seven treasures, a shaft
of little lifts climbed with a lollipop's high jumps (26), and on top a helicopter (13) flown to
the last two treasures on a slime pillar high in the cave under enemy helicopters (32), and back
to the golden key. Here the same order in 3D: the start room and the lava hall over the mine
corridor in the west, the cave in the middle, the corridor, the crate room and the shaft east of
it, the deck on top of them with the helicopter.

    python3 tools/worlds/world058.py                   # writes worlds3d/world058.vwr
    python3 tools/worlds/world058.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which
        has no keys, crates, lollipop or flight): 1 the tanks, helicopters and moving mines timed
        and the low mine crouched under (left out), 2 the red door open, 3 the crates pulled out (left out), 4 the lollipop's
        jumps (stand-in lifts), 5 the helicopter's flights (stand-in lifts)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from station import (BALL_MACHINES, GIRDER, GRILLE, LAVA, PILLARS, PLANTS, metal,  # noqa: E402
                     slime)

RED_DOOR = 334
FLIGHT_BARRIER = 214             # solid on foot, flown through (Decor::DecorDetect)
SPAWN = (14, 20, 86)
DECK = 24                        # the deck on top of the east works: walk 24


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(58)
    w = World(mission=58, sky=21)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def goo(x0, y0, z0, x1, y1, z1):
        slime(w, x0, y0, z0, x1, y1, z1, rng)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def room(x0, z0, x1, z1, walk, top):
        """Plates round the inside x0..x1, z0..z1, floor top walk - 1, roof at `top`."""
        plates(x0 - 1, walk - 2, z0 - 1, x1 + 1, top, z1 + 1)
        carve(x0, walk, z0, x1, top - 1, z1)

    def enemy(otype, a, b, ticks, stop):
        if stage < 1:
            w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    # ---- A, the start room (inside x 10-18, z 78-88, walk 20, roof at 27) ---------------------
    room(10, 78, 18, 88, 20, 27)
    w.spawn = SPAWN
    w.set(11, 20, 87, 82)
    w.bigdecor(BALL_MACHINES[0], 11, 21, 87)
    w.place(O.EGG, 11, 20, 79)
    # Two steps up in its north-east corner (walk 22, 24) and a jump into a high doorway (walk 25)
    #    into the lava hall.
    plates(16, 20, 79, 17, 21, 81)
    plates(18, 20, 79, 18, 23, 81)
    carve(19, 25, 79, 19, 26, 81)
    # The red door in the east wall at the floor (three wide, two high, under a lintel).
    for z in (82, 83, 84):
        for y in (20, 21):
            w.set(19, y, z, 0 if stage >= 2 else RED_DOOR)

    # ---- B, the mine corridor (x 20-41, z 82-84, walk 20), under the lava hall ---------------
    plates(20, 18, 81, 41, 19, 85)
    plates(20, 20, 81, 41, 22, 81)
    plates(20, 20, 85, 41, 22, 85)
    # Mines lying across its lanes (jump them), then three gates one cell wide, a mine riding up
    #    and down two cells in each (20 ticks, the original's, alternately): pass under it while
    #    it is up.
    for x, z in ((23, 82), (23, 84), (26, 83), (29, 82), (29, 84)):
        mine(w, x, 20, z)
    for k, x in enumerate((33, 35, 37)):
        for z in (82, 84):
            plates(x, 20, z, x, 22, z)
        lo, hi = (x, 20, 83), (x, 22, 83)
        if stage < 1:
            w.place(O.MOVING_MINE, *(hi if k % 2 else lo), end=(lo if k % 2 else hi),
                    advance=20, recede=20, stop_start=0, stop_end=0)
    for x in range(21, 41, 4):
        w.set(x, 22, 81, GIRDER)
        w.set(x, 22, 85, GIRDER)

    # ---- K, the lava hall (inside x 20-40, z 79-87, walk 25) over the corridor ----------------
    plates(20, 23, 79, 40, 23, 87)                    # the corridor's ceiling
    w.fill(20, 24, 79, 40, 24, 87, LAVA)              # the hall's floor
    plates(19, 25, 78, 41, 30, 78)
    plates(19, 25, 88, 41, 30, 88)
    plates(41, 25, 78, 41, 30, 88)
    plates(19, 27, 79, 19, 30, 88)
    plates(19, 25, 82, 19, 26, 88)
    plates(19, 31, 78, 41, 31, 88)
    plates(20, 24, 79, 22, 24, 81)                    # the landing inside the doorway
    # Stones across the lava, each a jump over one cell of it, to the key's pedestal.
    for x0, z0 in ((24, 80), (27, 81), (30, 82), (33, 83)):
        plates(x0, 24, z0, x0 + 1, 24, z0 + 1)
    plates(36, 24, 83, 38, 25, 86)                    # the pedestal, walk 26
    w.place(O.KEY1, 37, 26, 85)
    for x, z in ((24, 81), (31, 83)):
        w.set(x, 25, z, PILLARS)

    # ---- C, the slime cave (inside x 42-78, z 52-96, floor walk 8, open above) ----------------
    goo(41, 4, 51, 79, 7, 97)                          # its floor
    plates(41, 8, 51, 41, 22, 97)                      # its walls: west and the others to y 22,
    plates(41, 8, 51, 79, 22, 51)                      # east to the deck (y 23)
    plates(41, 8, 97, 79, 22, 97)
    plates(79, 8, 51, 79, 23, 97)
    carve(41, 20, 82, 41, 22, 84)                      # where the corridor comes in
    # Lava pools in its floor.
    for x0, z0, x1, z1 in ((45, 56, 52, 62), (66, 78, 74, 88), (47, 88, 54, 94), (70, 56, 75, 62)):
        w.fill(x0, 7, z0, x1, 7, z1, LAVA)
    # R, the rim (x 42-46, z 79-87, walk 20) along the west wall.
    plates(42, 17, 79, 46, 19, 87)
    w.place(O.EGG, 43, 20, 86)
    # The islands, floating masses of slime stepping down toward the middle, each a jump over one
    #    cell and two down: I1 (walk 18) off the rim, I2 (walk 16) south of it, then the golden
    #    key's island (walk 14).
    goo(48, 15, 79, 52, 17, 84)
    goo(49, 14, 80, 51, 14, 83)                       # a drip under it
    w.place(O.EGG, 51, 18, 83)
    w.place(O.DRINK, 49, 18, 80)
    goo(50, 13, 73, 54, 15, 77)
    goo(51, 12, 74, 53, 12, 76)
    enemy(O.CREATURE, (50, 16, 75), (54, 16, 75), 30, 23)
    w.bigdecor(PLANTS[0], 54, 16, 73)
    # GK, the golden key's island (x 56-64, z 66-74, walk 14), an arch of slime in its middle and
    #    the key under it; a tank across its north side; an enemy helicopter over it.
    goo(56, 11, 66, 64, 13, 74)
    goo(58, 9, 68, 62, 10, 72)
    for x in (58, 62):
        goo(x, 14, 69, x, 16, 71)
    goo(58, 17, 69, 62, 17, 71)
    w.place(O.SECRET_EXIT, 60, 14, 70)
    enemy(O.CREATURE, (57, 14, 67), (63, 14, 67), 30, 23)
    enemy(O.ROBOT, (59, 20, 72), (61, 20, 72), 8, 25)
    for x, z, p in ((56, 66, 1), (64, 74, 2)):
        w.bigdecor(PLANTS[p], x, 14, z)
    # On the floor: a tank sweeping before the tunnel, a mine.
    enemy(O.CREATURE, (44, 8, 64), (44, 8, 70), 50, 23)
    w.place(O.DRINK, 47, 8, 67)
    w.place(O.EGG, 70, 8, 92)

    # ---- T, the tank tunnel to treasure 1 (x 29-40, z 61, walk 8; two high) -------------------
    plates(27, 6, 58, 41, 10, 64)
    carve(29, 8, 61, 41, 9, 61)
    for x in (37, 32):                                 # alcoves to step into while a tank turns
        carve(x, 8, 62, x, 9, 62)
    carve(26, 8, 59, 28, 9, 63)                        # the treasure's cell at the end
    plates(25, 6, 58, 25, 10, 64)
    plates(26, 6, 58, 28, 7, 64)
    plates(26, 10, 58, 28, 10, 64)
    plates(26, 8, 58, 28, 9, 58)
    plates(26, 8, 64, 28, 9, 64)
    w.place(O.TREASURE, 26, 8, 61)
    w.place(O.DRINK, 42, 8, 60)                        # at the mouth, unseen for 20 s
    enemy(O.CREATURE, (35, 8, 61), (40, 8, 61), 60, 23)
    enemy(O.CREATURE, (29, 8, 61), (33, 8, 61), 50, 23)

    # ---- D, the corridor of tanks (x 80-96, z 59-63, walk 8) ----------------------------------
    plates(79, 6, 58, 97, 7, 64)
    plates(79, 8, 58, 97, 11, 58)
    plates(79, 8, 64, 97, 11, 64)
    plates(97, 8, 58, 97, 11, 64)
    plates(79, 12, 58, 97, 12, 64)
    carve(79, 8, 60, 79, 10, 62)                       # from the cave
    # Three tanks swing across it (along Z), each firing across the corridor at every turn:
    #    cross their lines between shots, or unseen; two drinks on the way.
    for x, ticks in ((83, 30), (88, 30), (93, 20)):
        enemy(O.CREATURE, (x, 8, 59), (x, 8, 63), ticks, 23)
    w.place(O.DRINK, 80, 8, 59)
    w.place(O.DRINK, 86, 8, 63)
    mine(w, 85, 8, 61)
    mine(w, 91, 8, 60)

    # ---- E, the crate room (inside x 82-92, z 48-57, walk 8) and the vault ---------------------
    room(82, 48, 92, 57, 8, 12)
    carve(89, 8, 58, 89, 9, 58)                        # the doorway from D, one wide
    if stage < 1:
        mine(w, 89, 8, 58, hanging=True)               # a mine hanging in it: crouch under
    for x in (83, 84, 85):
        w.place(O.EGG, x, 8, 49)
    w.bigdecor(BALL_MACHINES[1], 92, 8, 56)
    # The tunnel north (x 89, z 42-47, one high): five crates fill it. Pull one (crouch, facing
    #    it, Down) out into the room, walk round it, pull the next; the vault behind holds seven.
    plates(87, 6, 41, 91, 10, 47)
    carve(89, 8, 42, 89, 8, 47)
    if stage < 3:
        for z in range(43, 48):
            w.place(O.CRATE, 89, 8, z)
    room(86, 35, 92, 40, 8, 11)
    carve(89, 8, 41, 89, 8, 41)
    for x in range(86, 93):
        w.place(O.TREASURE, x, 8, 37)

    # ---- F, the lollipop shaft (inside x 94-98, z 49-53, walk 8 up to the deck, 24) ------------
    plates(93, 6, 48, 99, 7, 54)
    plates(93, 8, 48, 99, 27, 48)
    plates(93, 8, 54, 99, 27, 54)
    plates(93, 8, 48, 93, 27, 54)
    plates(99, 8, 48, 99, 27, 54)
    carve(93, 8, 50, 93, 9, 52)                        # from the crate room
    w.place(O.LOLLIPOP, 95, 8, 51)                     # lick it: very high jumps for 15 s
    # Three platforms (2 x 2) each four above the last and beside it, alternately east and west,
    #    each shuttling a cell along Z (20 ticks, stop 20: the original's); a licked jump reaches
    #    each; the top ledge four above the last, the deck beyond it.
    for k, (x0, walk) in enumerate(((96, 12), (94, 16), (96, 20))):
        a, b = (50, 51) if k % 2 == 0 else (51, 50)
        for x in (x0, x0 + 1):
            for dz in (0, 1):
                lift(w, (x, walk - 1, a + dz), (x, walk - 1, b + dz), ticks=20, stop=20)
    plates(94, DECK - 1, 49, 95, DECK - 1, 53)         # the top ledge, walk 24
    carve(94, DECK, 54, 95, DECK + 1, 54)              # out onto the deck
    mine(w, 98, 8, 53)
    if stage >= 4:                     # levelcheck has no lollipop: stand-in lifts
        for a, b in (((95, 7, 52), (95, 11, 52)), ((96, 11, 53), (96, 15, 53)), ((95, 15, 53), (95, 19, 53)),
                     ((96, 19, 53), (96, 23, 53))):
            w.place(O.LIFT, *a, end=b)

    # ---- G, the deck (x 80-98, z 55-72, walk 24), on top of the east works ---------------------
    plates(80, 21, 55, 98, 23, 72)
    w.place(O.HELICOPTER, 88, DECK, 64)
    mine(w, 84, DECK, 60)
    mine(w, 93, DECK, 68)
    w.bigdecor(BALL_MACHINES[3], 97, DECK, 71)
    for x, z in ((81, 55), (81, 72)):
        for y in range(DECK, DECK + 4):
            w.set(x, y, z, GIRDER)
        w.set(x, DECK + 4, z, GRILLE)
    # Its west edge over the cave is a flight barrier (214, Decor::DecorDetect): a wall on foot,
    #    so Blupi cannot fall back into the cave from here, which the helicopter flies through.
    for z in range(55, 73):
        for y in (DECK, DECK + 1):
            w.set(80, y, z, FLIGHT_BARRIER)

    # ---- P, the slime pillar of the last two treasures (x 60-62, z 84-86, walk 30) -------------
    goo(60, 8, 84, 62, 29, 86)
    w.place(O.TREASURE, 60, 30, 84)
    w.place(O.TREASURE, 62, 30, 86)
    if stage < 1:
        w.place(O.MOVING_MINE, 60, 30, 86, end=(62, 30, 84), advance=60, recede=60, stop_start=0, stop_end=0)
    enemy(O.ROBOT, (59, 34, 85), (63, 34, 85), 24, 25)
    w.bigdecor(PLANTS[2], 61, 30, 85)
    if stage >= 5:                     # levelcheck cannot fly: stand-in lifts
        w.place(O.LIFT, 81, DECK - 1, 70, end=(61, 29, 87))
        w.place(O.LIFT, 61, 29, 83, end=(60, 13, 73))

    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world058.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
