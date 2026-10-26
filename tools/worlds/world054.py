#!/usr/bin/env python3
"""Mission 054, the sky tower (worlds3d/world054.vwr): brief plans/briefs/world054.md.

Mobile Eggbert's level 054 is a climb through a station tower in a blue sky, no treasures: from a
hall at the bottom Blupi licks a lollipop (ObjectType26: very high jumps for fifteen seconds) and
jumps up a girder shaft from stone to stone; a relay of six lifts, each meeting the next halfway
(50 ticks, stop 10), carries him up past side rooms of mines, hanging mines and sweeping mines;
two little lifts bob a cell (6 ticks) under hanging mines; a slime pocket below holds eggs and a
wasp whose balloon lifts him out; a wasp's balloon carries him up a well past a sweeping mine
into a room of machines; a lift rising out of a gap; then down through slime past an enemy tank
(ObjectType33) and enemy helicopters (ObjectType32) dropping shots, to the red arrow. Here the
same climb as one tower in 3D: the hall at its foot, the shaft of pillars, the relay hall, the
bridge of bobbing pads, the pocket under the corridor, the balloon well, the machine room, and
the slime terraces down the tower's east side to the arrow.

    python3 tools/worlds/world054.py                   # writes worlds3d/world054.vwr
    python3 tools/worlds/world054.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which has
        no lollipop, crouching, balloon or timing): 1 the lollipop's jumps (stand-in steps in the
        shaft), 2 the sweeping mines and the enemies timed (left out), 3 the pads and G2 crossed
        crouching (their mines left out), 4 the balloon up the well (a stand-in lift)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine, rect  # noqa: E402
from station import (BALL_MACHINES, COLUMNS, GIRDER, GRILLE, PLANTS, VASE, girder_mast, metal,  # noqa: E402
                     slime)

SPAWN = (35, 5, 69)
RELAY = 25                 # the relay hall's floor: walk 25
TOP = 55                   # the landing, the bridge and the corridor: walk 55
MECA = 67                  # the machine room: walk 67


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(54)
    w = World(mission=54, sky=26)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def goo(x0, y0, z0, x1, y1, z1):
        slime(w, x0, y0, z0, x1, y1, z1, rng)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def patrol(otype, a, b, ticks, stop):
        w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def timed(otype, a, b, ticks, stop):
        if stage < 2:
            patrol(otype, a, b, ticks, stop)

    # ---- A, the hall at the foot (inside x 31-51, z 58-71, walk 5) -----------------------------
    plates(30, 3, 57, 52, 4, 72)
    plates(30, 5, 57, 52, 11, 57)
    plates(30, 5, 57, 30, 11, 72)
    plates(52, 5, 57, 52, 11, 72)
    plates(30, 5, 72, 52, 6, 72)                      # the south wall low: the camera sees in
    for x, z, k in ((33, 60, 2), (49, 61, 0), (33, 66, 0), (49, 67, 2)):
        w.set(x, 5, z, 82)
        w.bigdecor(BALL_MACHINES[k], x, 6, z)
    for x in (38, 42):
        girder_mast(w, x, 59, 5, 10)
    w.place(O.LOLLIPOP, 45, 5, 59)                    # lick it (Action): very high jumps, 15 s
    w.spawn = SPAWN

    # ---- B, the shaft of pillars (inside x 42-48, z 50-56, open above) -------------------------
    plates(41, 3, 49, 49, 4, 57)
    plates(41, 5, 49, 49, 33, 49)
    plates(41, 5, 49, 41, 33, 57)
    plates(49, 5, 49, 49, 33, 57)
    plates(41, 5, 57, 49, 33, 57)
    carve(44, 5, 57, 46, 6, 57)                       # its door from the hall
    # Four pillars spiralling round its walls, each touching the last and four higher (a jump is
    #    two; a licked jump clears four), and the ledge under the doorway out four above the last.
    plates(47, 5, 54, 48, 8, 56)                      # P1, walk 9
    plates(47, 5, 51, 48, 12, 53)                     # P2, walk 13
    plates(44, 5, 50, 46, 16, 51)                     # P3, walk 17
    plates(42, 5, 50, 43, 20, 56)                     # P4, walk 21, along the west wall
    plates(44, 21, 55, 46, 24, 56)                    # the ledge, walk 25, under the doorway
    carve(44, 25, 57, 46, 26, 57)                     # the doorway into the relay hall
    w.place(O.EGG, 44, 17, 50)
    for y in (12, 20, 28):                            # girder bands on the outer faces
        for k in range(42, 49):
            w.set(k, y, 48, GIRDER)
            w.set(40, y, k + 1, GIRDER)
    if stage >= 1:                     # levelcheck has no lollipop: stand-in lifts, step to step
        for a, b in (((46, 4, 55), (46, 8, 55)), ((46, 8, 54), (46, 12, 53)), ((46, 12, 52), (45, 16, 52)),
                     ((44, 16, 52), (44, 20, 52)), ((44, 20, 54), (44, 24, 54))):
            w.place(O.LIFT, *a, end=b)

    # ---- C, the relay hall (inside x 42-56, z 58-68, walk 25 up to the landing at 55) ----------
    plates(41, 23, 57, 57, 24, 69)
    plates(41, 25, 57, 41, 60, 69)
    plates(57, 25, 57, 57, 60, 69)
    plates(41, 34, 57, 57, 60, 57)
    plates(42, 25, 69, 56, 27, 69)                    # the south side low, open above
    for y in (33, 43):
        for x in range(42, 57, 2):
            w.set(x, y, 57, GRILLE)
    # The relay: six lifts, each a pair along Z (z 62-63), each one cell further east, all with
    #    the original's 50 ticks, so each meets the next halfway: A 24->29, B 34->29, C 34->39,
    #    D 44->39, E 44->49, F 54->49. Step across where two wait side by side: they wait 20 ticks
    #    there, not the original's 10 (half a second is a 2D step, too short to judge in 3D).
    for k, (y0, y1) in enumerate(((24, 29), (34, 29), (34, 39), (44, 39), (44, 49), (54, 49))):
        x = 44 + k
        for z in (62, 63):
            if k == 0:
                w.clear(x, 24, z)                     # A starts flush with the floor
            lift(w, (x, y0, z), (x, y1, z), ticks=50, stop=20)
    # The landing at the top (x 50-56, walk 55) beside F's top.
    plates(50, 53, 58, 56, 54, 68)
    # Galleries on the north side, each reached where two lifts meet:
    #    G1 (walk 30, beside A and B): two eggs behind a mine sweeping across it, a mine hanging
    #    over its mouth (a jump there meets it).
    plates(42, 28, 58, 45, 29, 61)
    w.place(O.EGG, 42, 30, 58)
    w.place(O.EGG, 43, 30, 58)
    timed(O.MOVING_MINE, (42, 30, 60), (45, 30, 60), 30, 0)
    mine(w, 44, 32, 61, hanging=True)
    plates(43, 33, 60, 45, 33, 61)
    #    G2 (walk 40, beside C and D): two mines hanging at Blupi's height across it (crouch under
    #    them), an egg behind.
    plates(44, 38, 58, 49, 39, 61)
    plates(44, 41, 58, 49, 41, 61)                    # its ceiling, one above Blupi
    if stage < 3:
        for x in (46, 47):
            for z in (58, 59, 60):
                mine(w, x, 40, z, hanging=True)
    w.place(O.EGG, 44, 40, 58)
    #    G3 (walk 50, beside E and F): a row of mines across it to jump, an egg behind.
    plates(46, 48, 58, 49, 49, 61)
    for z in (58, 59, 60):
        mine(w, 47, 50, z)
    w.place(O.EGG, 49, 50, 58)
    w.bigdecor(VASE, 49, 50, 60)
    # Two mines riding up and down across the landing's way to the bridge (40 ticks).
    timed(O.MOVING_MINE, (53, TOP, 62), (53, TOP + 3, 62), 40, 0)
    timed(O.MOVING_MINE, (54, TOP + 3, 63), (54, TOP, 63), 40, 0)

    # ---- D, the bridge of bobbing pads (x 57-62, z 62-63, walk 55) ----------------------------
    carve(57, TOP, 61, 57, TOP + 3, 64)               # out through the hall's east wall
    plates(57, 54, 62, 57, 54, 63)                    # a stone in the doorway
    plates(60, 54, 62, 60, 54, 63)                    # a stone in the middle
    # Two pads, each 2 x 2 lifts in step, bob a cell under the line (6 ticks, stop 3, the
    #    original's) in opposite phase; a mine hangs over each pad at Blupi's height: crossed
    #    crouching. A jump over a pad meets its mines.
    for x in (58, 59):
        for z in (62, 63):
            lift(w, (x, 54, z), (x, 53, z), ticks=6, stop=3)
    for x in (61, 62):
        for z in (62, 63):
            lift(w, (x, 53, z), (x, 54, z), ticks=6, stop=3)
    if stage < 3:
        for x in (58, 59, 61, 62):
            for z in (62, 63):
                mine(w, x, TOP, z, hanging=True)
    plates(57, TOP + 1, 60, 62, TOP + 5, 65)          # the mass they hang from, one above Blupi
    for x in range(57, 63):
        w.set(x, TOP + 1, 60, GIRDER)
        w.set(x, TOP + 1, 65, GIRDER)

    # ---- E, the slime pocket under the corridor -------------------------------------------
    # A hole in the corridor's floor (x 66-68, z 61-63) drops into the pocket (x 62-73, z 57-68, walk 47):
    #    slime, plants, three eggs and a wasp sweeping under the hole: its balloon lifts Blupi back up.
    goo(61, 44, 56, 74, 46, 69)
    goo(61, 47, 56, 61, 52, 69)
    goo(74, 47, 56, 74, 52, 69)
    goo(61, 47, 56, 74, 52, 56)
    goo(61, 47, 69, 74, 52, 69)
    goo(61, 53, 56, 74, 53, 69)                       # its roof, under the corridor
    for x, z in ((63, 58), (72, 58), (63, 67)):
        w.place(O.EGG, x, 47, z)
    for x, z in ((63, 62), (72, 66), (70, 58)):
        w.bigdecor(PLANTS[x % 3], x, 47, z)
    patrol(O.WASP, (65, 47, 62), (69, 47, 62), 12, 4)

    # The corridor (x 63-72, z 59-66, walk 55) over it, and the hole.
    plates(63, 53, 58, 72, 54, 67)
    plates(63, TOP, 58, 72, TOP + 4, 58)
    plates(63, TOP, 67, 72, TOP + 4, 67)
    plates(63, TOP + 5, 58, 72, TOP + 5, 67)
    carve(66, 53, 61, 68, 54, 63)

    # ---- F, the balloon well (inside x 73-77, z 60-64, walk 55 up to the machine room, 67) -----
    plates(72, 53, 59, 78, 54, 65)
    plates(72, TOP, 59, 78, MECA - 2, 59)
    plates(72, TOP, 65, 78, MECA - 2, 65)
    plates(78, TOP, 59, 78, MECA - 2, 65)
    plates(72, TOP + 5, 59, 72, MECA - 2, 65)
    carve(72, TOP, 61, 72, TOP + 3, 63)               # from the corridor
    # A wasp sweeps across its foot (12 ticks, stop 4): its balloon rises (faster with the jump
    #    held) for about ten seconds; a mine sweeping across halfway up pops it (time the rise).
    patrol(O.WASP, (74, TOP, 62), (77, TOP, 62), 12, 4)
    timed(O.MOVING_MINE, (73, TOP + 6, 62), (77, TOP + 6, 62), 30, 0)
    if stage >= 4:                       # levelcheck has no balloon: a stand-in lift up
        w.place(O.LIFT, 75, TOP - 1, 64, end=(75, MECA - 1, 64))

    # ---- G, the machine room (inside x 66-85, z 55-69, walk 67) -------------------------------
    plates(65, MECA - 2, 54, 86, MECA - 1, 70)
    carve(73, MECA - 2, 60, 77, MECA - 1, 64)         # the well's mouth
    plates(65, MECA, 54, 86, MECA + 7, 54)
    plates(65, MECA, 70, 86, MECA + 2, 70)
    plates(65, MECA, 54, 65, MECA + 7, 70)
    plates(86, MECA, 54, 86, MECA + 7, 70)
    for x, z, k in ((68, 57, 0), (68, 67, 1), (81, 57, 2), (81, 67, 3)):
        w.bigdecor(BALL_MACHINES[k], x, MECA, z)
    for x, z, k in ((70, 62, 0), (79, 59, 1), (79, 65, 2)):
        w.bigdecor(COLUMNS[k], x, MECA, z)
    timed(O.MOVING_MINE, (70, MECA, 58), (70, MECA, 66), 40, 0)
    timed(O.MOVING_MINE, (83, MECA, 66), (83, MECA, 58), 40, 0)
    # Its east door, and a gap across the way out (x 87-88, z 61-63): beside the door three lifts
    #    rise two cells (20 ticks, stop 10, the original's) to lie flush, a stone to step on
    #    before the jump over the last cell.
    carve(86, MECA, 61, 86, MECA + 2, 63)
    plates(87, MECA - 2, 60, 92, MECA - 1, 64)
    carve(87, MECA - 4, 61, 88, MECA - 1, 63)
    plates(87, MECA - 5, 61, 88, MECA - 5, 63)
    for z in (61, 62, 63):
        lift(w, (87, MECA - 3, z), (87, MECA - 1, z), ticks=20, stop=10)
    plates(87, MECA, 60, 92, MECA + 2, 60)
    plates(87, MECA, 64, 92, MECA + 2, 64)

    # ---- H, the slime terraces down the tower's east side to the arrow -------------------------
    # S1, the tank's ledge (x 90-98, z 58-66, walk 67): the enemy tank drives across it (z 65) and
    #    fires along its line at each turn; the way down is beyond it, the ledge's south edge.
    goo(90, MECA - 5, 57, 99, MECA - 1, 67)
    goo(99, MECA, 57, 99, MECA + 3, 67)
    goo(90, MECA, 57, 99, MECA + 2, 57)
    timed(O.CREATURE, (91, MECA, 65), (98, MECA, 65), 60, 23)
    w.bigdecor(PLANTS[0], 98, MECA, 59)
    # S2-S4, three terraces stepping south and down (walk 63, 59, 55), each a drop of four; an
    #    enemy helicopter hovers over S3 and drops a shot at each turn.
    for k, (z0, z1, walk) in enumerate(((68, 71, 63), (72, 75, 59), (76, 79, 55))):
        goo(88, walk - 3, z0, 99, walk - 1, z1)
        goo(88, walk, z0, 88, walk + 3, z1)
        goo(99, walk, z0, 99, walk + 3, z1)
    goo(88, 64, 67, 89, 70, 67)
    timed(O.ROBOT, (93, 63, 73), (95, 63, 73), 8, 25)
    for x, z in ((90, 69), (97, 74), (91, 78)):
        w.bigdecor(PLANTS[(x + z) % 3], x, {69: 63, 74: 59, 78: 55}[z], z)
    # S5, the cave of the arrow (inside x 87-98, z 80-90, walk 51), under a slime roof: two enemy
    #    helicopters patrol above the arrow and drop shots on it.
    goo(86, 46, 79, 99, 50, 91)
    goo(86, 51, 91, 99, 60, 91)
    goo(86, 51, 79, 86, 60, 91)
    goo(99, 51, 79, 99, 60, 91)
    goo(86, 61, 79, 99, 61, 91)
    carve(89, 51, 79, 97, 54, 79)                     # open from S4 above
    goo(87, 51, 87, 89, 52, 90)                       # slime mounds
    goo(95, 51, 81, 97, 51, 83)
    timed(O.ROBOT, (89, 56, 85), (94, 56, 85), 24, 25)
    timed(O.ROBOT, (90, 56, 87), (95, 56, 87), 24, 25)
    w.place(O.EXIT, 93, 51, 88)
    for x, z in ((88, 81), (97, 89)):
        w.bigdecor(PLANTS[(x + z) % 3], x, 51, z)

    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world054.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
