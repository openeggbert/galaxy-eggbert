#!/usr/bin/env python3
"""Mission 055, the dark works (worlds3d/world055.vwr): brief plans/briefs/world055.md.

Mobile Eggbert's level 055 runs through a dark machine works, seven treasures behind treasure
doors 2, 4 and 7: Blupi starts in a shaft where a wasp's balloon (ObjectType44) carries him up to
two treasures in its walls and, before he meets the mines hanging under its roof, into the
corridor to the door "2"; two more balloon shafts, past hanging mines, a block with mines under
it and eggs on top, and mines sweeping across; a corridor of mines with two invert pickups
(ObjectType40: the controls turn round for a while) and two treasures, the door "4"; a shaft
climbed down ledge to ledge past mines, a treasure; a corridor of three lightning curtains
(304/305); a hall of lava (68) crossed on overcrafts (ObjectType46), a lift up to a treasure,
the last treasure beyond the lava, the door "7"; a pool the overcraft dislikes, and the red
arrow. Here the same chain in 3D: the start shaft, two balloon shafts stepping up north, the
corridor of inverts heading east, the ledge shaft down, the lightning corridor heading north,
the lava hall heading west under a low roof, and the pool hall below it.

    python3 tools/worlds/world055.py                   # writes worlds3d/world055.vwr
    python3 tools/worlds/world055.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which
        counts no treasures and has no balloon, overcraft or timing): 1 the balloons ridden
        (stand-in lifts), 2 the mines on the floors jumped or passed (left out), 3 the door "2"
        open, 4 the door "4" open, 5 the lightning timed (left out), 6 the lava crossed on an
        overcraft (a floor), 7 the door "7" open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from station import BLITZ, BLITZ_EMITTER, GRILLE, LAVA, WATER, treasure_door  # noqa: E402

DARK = 78                          # the dark machine plate, the mass of the original's map
DARK_MIX = (81, 82, 84)
FACES = (87, 89, 90, 88)           # machine parts set into wall faces
SPAWN = (23, 10, 74)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(55)
    w = World(mission=55, sky=29)

    def dark(x0, y0, z0, x1, y1, z1, mix=0.04):
        for x in range(min(x0, x1), max(x0, x1) + 1):
            for y in range(min(y0, y1), max(y0, y1) + 1):
                for z in range(min(z0, z1), max(z0, z1) + 1):
                    w.set(x, y, z, rng.choice(DARK_MIX) if rng.random() < mix else DARK)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def room(x0, z0, x1, z1, floor, top):
        """A closed room: walls and floor and roof of dark plates round the inside x0..x1,
        z0..z1, from the floor block's top at `floor` (walk floor + 1) to the roof at `top`."""
        dark(x0 - 1, floor - 1, z0 - 1, x1 + 1, top, z1 + 1)
        carve(x0, floor + 1, z0, x1, top - 1, z1)

    def faces(cells):
        for x, y, z in cells:
            w.set(x, y, z, rng.choice(FACES))

    def patrol(otype, a, b, ticks, stop):
        w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def door(cells, walk, n, k):
        for x, z in cells:
            for y in (walk, walk + 1):
                w.set(x, y, z, 0 if stage >= k else treasure_door(n))

    # ---- A, the start shaft (inside x 20-26, z 68-74, walk 10, roof at 24) -------------------
    room(20, 68, 26, 74, 9, 24)
    w.spawn = SPAWN
    patrol(O.WASP, (21, 10, 71), (25, 10, 71), 12, 4)      # the original's 12 ticks, stop 4
    # Mines hang under the roof (y 23): a balloon that rises past the corridor meets them.
    for x in (20, 22, 24, 26):
        for z in (68, 70, 72, 74):
            if (x + z) % 4 == 0:
                mine(w, x, 23, z, hanging=True)
    # The two treasures in niches in the west and east walls, four above the floor (no jump
    #    reaches them): drift in on the balloon.
    #    The niches are three high and five wide: the balloon is steered by walking, and held at
    #    a height with crouch (it stops rising), not to the cell.
    for x0, x1, c0, c1, tx in ((16, 19, 17, 19, 18), (27, 30, 27, 29, 28)):
        dark(x0, 12, 68, x1, 18, 74)
        carve(c0, 14, 69, c1, 16, 73)
        w.place(O.TREASURE, tx, 14, 71)
    faces([(20, 12, 68), (26, 12, 74), (23, 16, 68), (20, 19, 74), (26, 20, 71)])
    # The corridor north out of its wall at walk 18 (eight up), three wide and three high: three
    #    mines across it to jump, the door "2" at its end.
    dark(21, 17, 58, 25, 21, 67)
    carve(22, 18, 59, 24, 20, 67)
    if stage < 2:
        for x in (22, 23, 24):
            mine(w, x, 18, 64)
    door([(22, 61), (23, 61), (24, 61)], 18, 2, 3)
    if stage >= 1:                     # levelcheck has no balloon: stand-in lifts
        w.place(O.LIFT, 20, 9, 74, end=(20, 13, 72))
        w.place(O.LIFT, 26, 9, 74, end=(26, 13, 72))
        w.place(O.LIFT, 23, 9, 74, end=(23, 17, 68))

    # ---- The drop and the low corridor to the balloon shafts ----------------------------------
    # D0, a shaft down from the corridor's end (x 22-24, z 55-58) to walk 4.
    dark(21, 2, 54, 25, 21, 59)
    carve(22, 4, 55, 24, 20, 58)
    carve(22, 18, 59, 24, 20, 59)
    # L, the low corridor south to B1 (x 22-24, z 45-54, walk 4).
    dark(21, 2, 45, 25, 7, 54)
    carve(22, 4, 45, 24, 6, 55)

    # ---- B1, the first balloon shaft (inside x 20-26, z 38-44, walk 4 up to 14) ---------------
    room(20, 38, 26, 44, 3, 18)
    carve(22, 4, 45, 24, 6, 45)
    patrol(O.WASP, (21, 4, 42), (25, 4, 42), 18, 4)
    # Low, four corner ledges with a mine hanging under each: rise in the middle.
    for x0, z0 in ((20, 38), (25, 38), (20, 43), (25, 43)):
        dark(x0, 8, z0, x0 + 1, 8, z0 + 1)
        mine(w, x0, 7, z0, hanging=True)
        mine(w, x0 + 1, 7, z0 + 1, hanging=True)
    # Higher, a block in the middle with three mines hanging under it and two eggs on top: drift
    #    to a side to pass it.
    dark(22, 10, 40, 24, 11, 42)
    for x in (22, 23, 24):
        mine(w, x, 9, 41, hanging=True)
    w.place(O.EGG, 22, 12, 40)
    w.place(O.EGG, 24, 12, 42)
    # At the top the mouth north (walk 14), mines hanging under the roof elsewhere.
    for x, z in ((20, 40), (22, 42), (24, 44), (26, 40), (20, 44), (26, 42)):
        mine(w, x, 17, z, hanging=True)
    faces([(20, 6, 38), (26, 13, 44), (23, 15, 44)])
    if stage >= 1:
        w.place(O.LIFT, 20, 3, 44, end=(23, 13, 38))

    # ---- P, the passage north (x 22-24, z 32-37, walk 14), and B2 -----------------------------
    dark(21, 12, 32, 25, 17, 37)
    carve(22, 14, 32, 24, 16, 37)
    # B2 (inside x 20-26, z 25-31, walk 14 up to 24): two mines sweep across it at two heights
    #    (30 ticks, no stop: the original's); mines hang under its roof except over the mouth east.
    room(20, 25, 26, 31, 13, 28)
    carve(22, 14, 32, 24, 16, 32)
    patrol(O.WASP, (21, 14, 28), (25, 14, 28), 18, 4)
    patrol(O.MOVING_MINE, (20, 18, 27), (26, 18, 27), 30, 0)
    patrol(O.MOVING_MINE, (26, 21, 29), (20, 21, 29), 30, 0)
    for x, z in ((20, 25), (22, 27), (24, 29), (20, 31), (22, 25), (24, 31)):
        mine(w, x, 27, z, hanging=True)
    faces([(20, 16, 25), (26, 19, 31), (23, 22, 25)])
    if stage >= 1:
        w.place(O.LIFT, 25, 13, 26, end=(26, 23, 27))

    # ---- C, the corridor of inverts (inside x 28-55, z 27-29, walk 24) ------------------------
    dark(27, 22, 26, 56, 27, 30)
    carve(27, 24, 27, 55, 26, 29)
    # Narrowed to one cell at x 30 and x 52, an invert pickup lying in each narrows: the controls
    #    turn round for a while, Up walks him back.
    for x in (30, 52):
        for z in (27, 29):
            w.fill(x, 24, z, x, 26, z, DARK)
        w.place(O.INVERT, x, 24, 28)
    if stage < 2:
        for x, z in ((34, 28), (36, 27), (36, 29), (39, 28), (42, 27), (42, 29), (45, 28),
                     (48, 27), (48, 29)):
            mine(w, x, 24, z)
    w.place(O.TREASURE, 39, 24, 27)
    w.place(O.TREASURE, 48, 24, 28)
    faces([(33, 26, 26), (41, 25, 30), (47, 26, 26), (50, 25, 30)])

    # ---- D, the ledge shaft down (inside x 57-63, z 25-31, from walk 24 down to 12) -----------
    room(57, 25, 63, 31, 11, 28)
    carve(56, 24, 27, 56, 25, 29)
    door([(56, 27), (56, 28), (56, 29)], 24, 4, 4)
    dark(57, 22, 26, 59, 23, 30)                            # the landing inside the door, walk 24
    # A spiral of ledges round the walls, each two below the last, meeting at the corners: step
    #    off one onto the next (north walk 22, east 20, south 18, west 16, north 14), the middle a
    #    drop to the floor (walk 12). Mines lie where a careless step lands; treasure 5 on the
    #    south ledge.
    dark(60, 21, 25, 63, 21, 27)
    dark(61, 19, 28, 63, 19, 31)
    dark(57, 17, 29, 60, 17, 31)
    dark(57, 15, 25, 59, 15, 28)
    dark(60, 13, 25, 63, 13, 27)
    if stage < 2:
        for x, y, z in ((62, 20, 28), (60, 18, 30), (58, 16, 28), (61, 14, 26)):
            mine(w, x, y, z)
    w.place(O.TREASURE, 57, 18, 31)
    faces([(63, 15, 25), (57, 20, 31), (60, 25, 25)])

    # ---- E, the lightning corridor (inside x 58-60, z 9-24, walk 12) --------------------------
    dark(57, 10, 8, 61, 16, 24)
    carve(58, 12, 9, 60, 14, 25)
    # Three curtains of lightning across it: emitters in the roof, lightning down to the floor,
    #    on half the time (2.5 s on, 2.5 s off): pass while they are off.
    for z0 in (21, 16, 11):
        for z in (z0, z0 + 1):                              # two deep: no jump clears one
            for x in (58, 59, 60):
                w.set(x, 15, z, BLITZ_EMITTER)
                if stage < 5:
                    for y in (12, 13, 14):
                        w.set(x, y, z, BLITZ)
    if stage < 2:
        for x, z in ((59, 19), (58, 14), (60, 14)):
            mine(w, x, 12, z)

    # ---- F, the lava hall (inside x 30-56, z 6-14, walk 12, roof at 17) ------------------------
    dark(29, 9, 5, 57, 11, 15)
    dark(29, 12, 5, 57, 17, 5)
    dark(29, 12, 15, 57, 17, 15)
    dark(29, 12, 5, 29, 17, 15)
    dark(57, 12, 5, 57, 17, 15)
    dark(29, 17, 5, 57, 17, 15)
    carve(57, 12, 9, 57, 14, 11)                            # from the lightning corridor
    carve(58, 12, 9, 60, 14, 9)
    # Five overcrafts parked by the door; the lava across the hall (x 38-48): on foot it burns
    #    him; the overcraft crosses it while Up holds it hovering (let go, it sinks into it).
    for z in (6, 8, 10, 12, 14):
        w.place(O.OVERCRAFT, 54, 12, z)
    w.fill(38, 11, 6, 48, 11, 14, 82 if stage >= 6 else LAVA)
    # Treasure 6 on a shelf in the south-east corner three above the floor (no jump): a lift
    #    rises to it from the floor (the original's 20 ticks a cell, stop 20).
    dark(50, 12, 13, 53, 14, 14)
    w.clear(49, 11, 14)
    lift(w, (49, 11, 14), (49, 14, 14), ticks=60, stop=20)
    w.place(O.TREASURE, 51, 15, 14)
    # Treasure 7 beyond the lava, the door "7" in the west wall.
    w.place(O.TREASURE, 32, 12, 7)
    faces([(40, 15, 5), (46, 15, 15), (33, 15, 15), (52, 15, 5)])
    door([(29, 9), (29, 10), (29, 11)], 12, 7, 7)

    # ---- G, the pool hall (inside x 8-24, z 4-16, walk 6) -------------------------------------
    room(8, 4, 24, 16, 5, 16)
    dark(25, 10, 8, 28, 15, 12)
    carve(24, 12, 9, 28, 14, 11)                            # the passage and its drop
    # A pool across the hall (x 14-17), one deep, its surface level with the floor: the
    #    overcrafts parked beside it are lost if they meet it; Blupi swims across.
    w.fill(14, 5, 4, 17, 5, 16, WATER)
    for z in (6, 8, 10, 12, 14):
        w.place(O.OVERCRAFT, 21, 6, z)
    w.place(O.EGG, 19, 6, 4)
    w.place(O.EXIT, 10, 6, 10)
    faces([(8, 9, 4), (24, 8, 16), (12, 10, 16), (20, 11, 4)])

    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world055.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
