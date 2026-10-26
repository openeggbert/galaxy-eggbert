#!/usr/bin/env python3
"""Mission 114, the four keys' works (worlds3d/world114.vwr): brief plans/briefs/world114.md.

Mobile Eggbert's level 114, the longest of world 11, is a great maze of blue machine panel shut by
coloured doors (334 red, 335 green, 336 blue) and their keys (49, 50, 51): fourteen treasures,
sixteen eggs; stairs swept by thirty-one moving mines (16); a row of eight skateboards (24) by
mines (2) to ride over and two inverts (40); pools with nine fish (17) and hanging mines (3) over
their banks; sleeping follow stars (96) by treasures, an enemy Blupi (32) firing down, five
helicopters (13) and fourteen lifts; the exit high up. Here the maze is a hall with four ways out
in 3D: south the terraces of moving mines and the red key; west, behind the red door, the skate
track and the green key; north, behind the green door, the pool and the blue key; east, behind
the blue door, the vault of the follow stars, the helicopters and the exit on its high platform.

    python3 tools/worlds/world114.py                 # writes worlds3d/world114.vwr
    python3 tools/worlds/world114.py --stage K out   # for levelcheck/flightcheck: K >= 1 no mines,
                                                     # moving or hanging mines, stars, fish or enemy;
                                                     # K >= 2 the red door open, 3 the
                                                     # green, 4 the blue
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402
from station import water  # noqa: E402

RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
FOLLOWER = 96
WALK, HIGH = 20, 34


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if len(argv) > 3 and argv[1] == "--stage" else \
        Path(__file__).resolve().parents[2] / "worlds3d" / "world114.vwr"
    rng = random.Random(114)
    w = World(mission=114, sky=18)
    calm = stage >= 1

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.08, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    def door(cells, icon, open_from):
        for x, y, z in cells:
            carve(x, y, z, x, y, z)
            if stage < open_from:
                w.set(x, y, z, icon)

    panel(2, 0, 2, 97, HIGH + 6, 97)

    # ---- the hall (walk 20) ----
    # Spawn mid-hall; the red door west, the green north, the blue east; south an open way down to
    # the terraces.
    carve(42, WALK, 42, 58, WALK + 5, 58)
    w.spawn = (50, WALK, 52)
    door([(41, y, z) for y in (WALK, WALK + 1) for z in (49, 50, 51)], RED_DOOR, 2)
    door([(x, y, 41) for y in (WALK, WALK + 1) for x in (49, 50, 51)], GREEN_DOOR, 3)
    door([(59, y, z) for y in (WALK, WALK + 1) for z in (49, 50, 51)], BLUE_DOOR, 4)
    carve(48, WALK, 59, 52, WALK + 2, 65)
    w.place(O.EGG, 44, WALK, 44)
    w.place(O.EGG, 56, WALK, 56)

    # ---- south: the terraces of moving mines (walk 20 -> 8) and the red key ----
    # Four terraces each three below the last, then the floor; on each, mines sweeping along it in
    # three lanes, at different paces (cross between them); treasures 1 and 2 at terrace ends, the
    # red key on the floor; a lift (2 x 2) from the floor back up to the first terrace.
    carve(34, 8, 66, 66, WALK + 5, 95)
    for k in range(5):
        z0 = 66 + 6 * k
        top = WALK - 1 - 3 * k
        panel(34, 7, z0, 66, top, min(z0 + 5, 95))
        if k == 4:
            break
        for lane, z in enumerate((z0 + 1, z0 + 3, z0 + 5)):
            pace = 30 + 8 * lane + 4 * k
            for start, end in (((35, top + 1, z), (65, top + 1, z)), ((65, top + 1, z), (35, top + 1, z))):
                if not calm and (lane + k) % 3 != 2 or start[0] == 35 and not calm and (lane + k) % 3 == 2:
                    w.place(O.MOVING_MINE, *start, end=end, advance=pace, recede=pace, stop_start=0,
                            stop_end=0)
    w.place(O.TREASURE, 35, WALK - 3, 74)
    w.place(O.TREASURE, 65, WALK - 9, 86)
    w.place(O.KEY1, 50, 8, 93)
    for x, y, z in ((60, WALK, 68), (40, WALK - 6, 80), (60, 8, 92)):
        w.place(O.EGG, x, y, z)
    for x in (64, 65):
        for z in (92, 93):
            lift(w, (x, 7, z), (x, WALK - 1, z), ticks=120, stop=30)
    carve(64, 7, 92, 65, 7, 93)
    panel(64, WALK - 2, 88, 66, WALK - 1, 91)                             # its landing, walk 20
    # The way from the landing back north to the first terrace (walk 20) over the lower terraces'
    # east ends: without its floor the red key's floor was a trap.
    panel(64, WALK - 2, 72, 66, WALK - 1, 87)
    carve(64, WALK, 70, 66, WALK + 2, 91)

    # ---- west, behind the red door: the skate track (walk 20) and the green key ----
    # Eight skateboards at the track's head; a block of nine mines three deep across it (the
    # skateboard rides through, setting them off); treasures 3-5 along it; the green key at its end;
    # two inverts in alcoves off it.
    carve(30, WALK, 49, 40, WALK + 2, 51)
    carve(4, WALK, 46, 29, WALK + 3, 54)
    for k, z in enumerate((47, 48, 49, 50, 51, 52, 53, 47)):
        w.place(O.SKATEBOARD, 28 - (k // 7), WALK, z)
    for x in (20, 21, 22):
        for z in (47, 50, 53):
            hazard(x, WALK, z)
    for x, z in ((16, 47), (12, 53), (8, 50)):
        w.place(O.TREASURE, x, WALK, z)
    w.place(O.KEY2, 5, WALK, 50)
    for x, z in ((18, 45), (10, 55)):
        carve(x, WALK, z, x, WALK + 1, z)
        w.place(O.INVERT, x, WALK, z)
    w.place(O.EGG, 25, WALK, 53)
    w.place(O.EGG, 6, WALK, 47)

    # ---- north, behind the green door: the pool (walk 20) and the blue key ----
    # A great pool level with the floor round it, nine fish in it, four treasures on its bottom
    # (dive for them); a chain of four islands across it, each reached on a ferry (2 wide, 60
    # ticks, dwelling 40) from the bank or the island before; mines hanging over the islands' edges;
    # the blue key on the last island.
    carve(49, WALK, 30, 51, WALK + 2, 40)
    carve(30, WALK, 6, 70, WALK + 5, 29)
    carve(31, 12, 7, 69, WALK - 1, 28)
    water(w, 31, 12, 7, 69, WALK - 1, 28)
    for x0, z0, x1, z1 in ((44, 20, 48, 24), (44, 10, 48, 14), (56, 10, 60, 14), (56, 20, 60, 24)):
        panel(x0, 12, z0, x1, WALK - 1, z1)
    for cells, offset in (([(47, 28), (48, 28)], (0, -3)), ([(45, 19), (46, 19)], (0, -4)),
                          ([(49, 11), (49, 12)], (6, 0)), ([(57, 15), (58, 15)], (0, 4))):
        for x, z in cells:
            lift(w, (x, WALK - 1, z), (x + offset[0], WALK - 1, z + offset[1]), ticks=60, stop=40)
    w.place(O.KEY3, 58, WALK, 23)
    for x, z in ((46, 22), (46, 12), (58, 12)):
        w.place(O.EGG, x, WALK, z)
    for x, z in ((44, 20), (48, 10), (56, 14), (60, 10), (56, 24), (44, 14)):
        hazard(x, WALK + 1, z, hanging=True)
    for x, z in ((36, 20), (40, 9), (64, 26), (66, 12)):
        w.place(O.TREASURE, x, 12, z)
    for k, (a, b) in enumerate((((32, 15, 8), (68, 15, 8)), ((68, 14, 27), (32, 14, 27)),
                                ((33, 16, 18), (42, 16, 18)), ((62, 13, 17), (68, 13, 17)),
                                ((50, 17, 16), (54, 17, 16)), ((35, 13, 26), (35, 13, 9)),
                                ((64, 15, 8), (64, 15, 27)), ((38, 17, 22), (42, 17, 22)),
                                ((66, 13, 22), (62, 13, 22)))):
        if not calm:
            w.place(O.FISH, *a, end=b, advance=60 + 6 * k, recede=60 + 6 * k, stop_start=30, stop_end=30)

    # ---- east, behind the blue door: the vault (walk 20), the helicopters and the exit (walk 34) ----
    # Five treasures on the floor, each by a sleeping follow star (wake it and it chases); an enemy
    # Blupi patrolling a ledge firing down; five helicopters; the exit on a high platform (walk 34),
    # reached by helicopter, eggs on its rim and on the ledges.
    carve(60, WALK, 49, 70, WALK + 2, 51)
    carve(71, WALK, 30, 95, HIGH + 4, 70)
    for x, z in ((76, 34), (90, 34), (76, 66), (90, 66), (83, 50)):
        w.place(O.TREASURE, x, WALK, z)
        if not calm:
            w.place(FOLLOWER, x + 2, WALK, z)
    panel(71, WALK, 30, 95, WALK + 5, 31)                                 # the ledge along the north wall
    if not calm:
        w.place(O.ROBOT, 73, WALK + 6, 31, end=(93, WALK + 6, 31), advance=120, recede=120, stop_start=20,
                stop_end=20)
    for z in (44, 46, 48, 52, 54):
        w.place(O.HELICOPTER, 73, WALK, z)
    panel(86, HIGH - 2, 56, 92, HIGH - 1, 62)
    w.place(O.EXIT, 89, HIGH, 59)
    for x, y, z in ((87, HIGH, 57), (91, HIGH, 61), (80, WALK + 6, 30), (88, WALK + 6, 30), (93, WALK, 69)):
        w.place(O.EGG, x, y, z)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
