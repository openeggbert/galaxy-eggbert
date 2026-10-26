#!/usr/bin/env python3
"""Mission 052, the spiral of vaults round the well (worlds3d/world052.vwr): brief
plans/briefs/world052.md.

Mobile Eggbert's level 052 is one corridor cut by treasure doors that open with 2, 4, 5, 6, 7
and 10 treasures (422-430): the treasures of each stretch lie in a pocket above it, reached by a
small lift, or in a pit below it; follow stars (ObjectType96) sleep round them and wake when
Blupi comes near, then home slowly on him and burst against any wall; eight helicopters wait in a
room past the door "7" with three treasures; past the door "10" two more, a little water, and a
shaft Blupi flies up past more stars to the red arrow. Twelve treasures, 34 stars. Here the
stretches wind down round a central well in a square spiral, each side four cells below the
last, each door at a corner dropping into the next: east, north, west, south, east again; past
the door "7" the hangar of helicopters, past the door "10" a tunnel into the foot of the well,
which Blupi flies up to the arrow on a ledge at its top.

    python3 tools/worlds/world052.py                   # writes worlds3d/world052.vwr
    python3 tools/worlds/world052.py --stage K out.vwr # the first K treasure doors open (1-6), for
        levelcheck (which counts no treasures and cannot fly); from 6 on, stand-in lifts for the
        helicopter's flights: from the hangar's ledge to its floor and perches, and up the well
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, rect  # noqa: E402
from station import GRILLE, PLATE_EDGES, WATER, metal, treasure_door  # noqa: E402

SKY = 13
RING = (40, 40, 60, 60)          # the station block round the well (x0, z0, x1, z1), y 10-42
WELL = (47, 47, 53, 53)          # the well's inside, open from its floor (walk 11) to the sky
# The spiral's stretches: (cells, walk). Corners belong to the stretch that starts there.
S1 = (rect(55, 46, 57, 57), 35)  # east side, from the south-east corner north (the start)
S2 = (rect(46, 43, 57, 45), 31)  # north side with the north-east corner, heading west
S3 = (rect(43, 43, 45, 54), 27)  # west side with the north-west corner, heading south
S4 = (rect(43, 55, 54, 57), 23)  # south side with the south-west corner, heading east
S5 = (rect(55, 43, 57, 57), 19)  # east side again, from the south-east corner north
SPAWN = (56, 35, 56)


def star(w, x, y, z):
    """A follow star asleep (ObjectType96): it wakes within two cells of Blupi."""
    w.place(O.FOLLOW_STAR, x, y, z)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(52)
    w = World(mission=52, sky=SKY)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def carve(cells, y0, y1):
        for x, z in cells:
            for y in range(y0, y1 + 1):
                w.clear(x, y, z)

    def door(cells, walk, n, k):
        """Treasure door n (two high, a lintel over it) on `cells`; open from stage k."""
        for x, z in cells:
            for y in (walk, walk + 1):
                w.set(x, y, z, 0 if stage >= k else treasure_door(n))
            w.set(x, walk + 2, z, GRILLE)

    # ---- The station block and the well ------------------------------------------------------
    x0, z0, x1, z1 = RING
    plates(x0, 10, z0, x1, 42, z1)
    wx0, wz0, wx1, wz1 = WELL
    carve(rect(wx0, wz0, wx1, wz1), 11, 42)
    for y in (17, 24, 31):                       # bands of grilles round the well's walls
        for k in range(wx0, wx1 + 1, 2):         # (opaque: a cut-out tile there would be a
            w.set(k, y, wz0 - 1, GRILLE)         # hole from the corridors into the well)
            w.set(k, y, wz1 + 1, GRILLE)
            w.set(wx0 - 1, y, k, GRILLE)
            w.set(wx1 + 1, y, k, GRILLE)

    # ---- The spiral --------------------------------------------------------------------------
    for cells, walk in (S1, S2, S3, S4, S5):
        carve(cells, walk, walk + 2)             # corridors three high
    # The corners drop from the stretch above into the next one.
    carve(rect(55, 43, 57, 45), S2[1], S1[1] + 2)    # north-east: S1 -> S2
    carve(rect(43, 43, 45, 45), S3[1], S2[1] + 2)    # north-west: S2 -> S3
    carve(rect(43, 55, 45, 57), S4[1], S3[1] + 2)    # south-west: S3 -> S4
    carve(rect(55, 55, 57, 57), S5[1], S4[1] + 2)    # south-east: S4 -> S5
    # The doors, each at the far end of its stretch.
    door([(x, 46) for x in (55, 56, 57)], S1[1], 2, 1)
    door([(46, z) for z in (43, 44, 45)], S2[1], 4, 2)
    door([(x, 54) for x in (43, 44, 45)], S3[1], 5, 3)
    door([(54, z) for z in (55, 56, 57)], S4[1], 6, 4)

    # S1, the start (east side, walk 35): a lift (one cell, ten ticks a cell, the original's
    #    pace) rises through a hole in the ceiling into the pocket above with two treasures; a
    #    star sleeps under the ceiling beyond it.
    walk = S1[1]
    w.clear(56, walk - 1, 50)
    w.clear(56, walk + 3, 50)
    lift(w, (56, walk - 1, 50), (56, walk + 3, 50), ticks=40, stop=10)
    w.set(56, walk - 2, 50, 82)
    carve(rect(55, 47, 57, 53), walk + 4, walk + 5)
    w.place(O.TREASURE, 55, walk + 4, 48)
    w.place(O.TREASURE, 57, walk + 4, 52)
    star(w, 56, walk + 2, 53)
    for x, z in ((55, 57), (57, 54)):
        w.set(x, walk, z, PLATE_EDGES[(x + z) % 2])

    # S2 (north side, walk 31): four stars asleep in a row under the ceiling between the drop
    #    and the lift; the pocket above with two treasures.
    walk = S2[1]
    for x in (51, 52, 53, 54):
        star(w, x, walk + 2, 44)
    w.clear(49, walk - 1, 44)
    w.clear(49, walk + 3, 44)
    lift(w, (49, walk - 1, 44), (49, walk + 3, 44), ticks=40, stop=10)
    w.set(49, walk - 2, 44, 82)
    carve(rect(46, 43, 53, 45), walk + 4, walk + 5)
    w.place(O.TREASURE, 46, walk + 4, 43)
    w.place(O.TREASURE, 53, walk + 4, 45)

    def pit(hole, step, cells, walk, treasure, stars):
        """A pit four below a stretch: a hole of two cells in its floor, a step two below the
        floor under the hole's first cell, the treasure and the stars asleep round it."""
        bottom = walk - 4
        carve(cells, bottom, bottom + 2)
        for x, z in hole:
            carve([(x, z)], bottom + 3, walk - 1)
        sx, sz = step
        w.fill(sx, bottom, sz, sx, bottom + 1, sz, 83)
        w.place(O.TREASURE, *treasure)
        for s in stars:
            star(w, *s)

    # S3 (west side, walk 27): a pit under it, two stars at its top corners.
    pit([(44, 49), (44, 50)], (44, 49), rect(43, 47, 45, 52), S3[1], (44, 23, 52),
        [(43, 25, 47), (45, 25, 52)])
    # S4 (south side, walk 23): a pit, four stars along its top.
    pit([(49, 56), (50, 56)], (49, 56), rect(47, 55, 52, 57), S4[1], (52, 19, 56),
        [(47, 21, 55), (48, 21, 57), (51, 21, 55), (52, 21, 57)])
    # S5 (east side, walk 19): a pit, four stars, two of them a jump above the treasure.
    pit([(56, 49), (56, 50)], (56, 49), rect(55, 47, 57, 52), S5[1], (56, 15, 52),
        [(55, 17, 47), (57, 17, 47), (55, 17, 52), (57, 17, 52)])
    # The door "7" at S5's north end, in the block's north face, onto the hangar's ledge.
    walk = S5[1]
    carve(rect(55, 38, 57, 42), walk, walk + 1)
    door([(x, 42) for x in (55, 56, 57)], walk, 7, 5)

    # ---- The hangar (inside x 42-60, z 20-38, walk 11, roofed at 26) ---------------------------
    plates(41, 9, 19, 61, 10, 39)
    plates(41, 11, 19, 41, 26, 39)
    plates(61, 11, 19, 61, 26, 39)
    plates(41, 11, 19, 61, 26, 19)
    plates(41, 11, 39, 61, 26, 39)
    plates(41, 26, 19, 61, 26, 39)
    carve(rect(55, 39, 57, 39), walk, walk + 1)
    # The ledge where the door "7" opens (x 53-59, z 34-38, walk 19): four helicopters, a star.
    plates(53, walk - 2, 34, 59, walk - 1, 38)
    for x in (53, 55, 57, 59):
        w.place(O.HELICOPTER, x, walk, 35)
    star(w, 53, walk + 2, 38)
    # Four more on the floor.
    for x, z in ((44, 36), (47, 36), (44, 22), (47, 24)):
        w.place(O.HELICOPTER, x, 11, z)
    # Three treasures only a helicopter reaches, eight stars round them: on a column in the
    #    middle (walk 21), in an alcove high in the west wall (walk 17), and in a pen on the floor
    #    walled three high (out of a jump's reach) in the north-east corner.
    plates(50, 11, 27, 51, 20, 28)
    w.place(O.TREASURE, 50, 21, 27)
    for s in ((49, 22, 26), (52, 22, 29), (50, 23, 29)):
        star(w, *s)
    plates(38, 15, 25, 41, 19, 29)
    w.fill(39, 17, 26, 41, 18, 28, 0)
    w.place(O.TREASURE, 39, 17, 27)
    for s in ((42, 18, 25), (42, 18, 29), (43, 19, 27)):
        star(w, *s)
    for x in range(55, 61):
        for y in (11, 12, 13):
            w.set(x, y, 25, 81)
    for z in range(20, 25):
        for y in (11, 12, 13):
            w.set(55, y, z, 81)
    w.place(O.TREASURE, 58, 11, 22)
    for s in ((57, 13, 21), (59, 13, 23)):
        star(w, *s)
    if stage >= 6:                         # levelcheck cannot fly: stand-in lifts
        w.place(O.LIFT, 52, walk - 1, 36, end=(50, 20, 27))     # ledge -> column
        w.place(O.LIFT, 50, 20, 26, end=(42, 16, 27))           # column -> alcove
        w.place(O.LIFT, 42, 10, 27, end=(57, 10, 23))           # floor -> the pen
        w.place(O.LIFT, 56, 10, 23, end=(56, 10, 26))           # out of the pen

    # ---- The door "10", the tunnel and the foot of the well -----------------------------------
    carve(rect(49, 39, 51, 46), 11, 12)
    door([(x, 39) for x in (49, 50, 51)], 11, 10, 6)
    # A little water in the floor where the tunnel comes in: a helicopter that meets it is lost.
    plates(49, 9, 47, 51, 9, 47)
    w.fill(49, 10, 47, 51, 10, 47, WATER)
    # The last two treasures in the well's far corners, four stars over them.
    w.place(O.TREASURE, 47, 11, 53)
    w.place(O.TREASURE, 53, 11, 53)
    for s in ((47, 13, 51), (48, 13, 53), (53, 13, 51), (52, 13, 53)):
        star(w, *s)
    # Up the well: two stars in its middle; the ledge of the red arrow along its south wall
    #    (walk 38) with four stars round the arrow.
    star(w, 50, 22, 50)
    star(w, 49, 30, 49)
    plates(47, 37, 52, 53, 37, 53)
    w.place(O.EXIT, 50, 38, 53)
    for s in ((48, 38, 52), (52, 38, 52), (50, 40, 52), (50, 39, 50)):
        star(w, *s)
    for x in (47, 53):
        w.set(x, 39, 54, 3)                  # the red-dot machines either side of the arrow
    if stage >= 6:
        w.place(O.LIFT, 50, 10, 49, end=(50, 37, 51))           # up the well

    w.spawn = SPAWN
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world052.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
