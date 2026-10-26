#!/usr/bin/env python3
"""Mission 051, the station and the cistern (worlds3d/world051.vwr): brief plans/briefs/world051.md.

Mobile Eggbert's level 051 opens world 5 in two places joined only by teleporters. A metal
station: Blupi starts beside a shaft of four lifts shuttling in alternate directions, the red
arrow at its top (it counts only with both treasures); west of the start a creature's tank
(ObjectType33) patrols and fires, two eggs past it; a climb on blue panels between moving mines
and a column of fans (deadly at the mouth) to a lift shuttling across a shaft, and teleporter
330 at its end. A brick cistern: Blupi dives into a water tank past fish, swims a channel into
a gallery with the two treasures behind a moving mine; the treasure door "2" (422) opens onto a
drop to a corridor of mines, hanging mines, moving mines, saws and a second tank, a drink (30:
unseen, untouchable for twenty seconds) at its start and teleporter 331 at its end, which
lands Blupi at the top of a shaft that falls into the start room. Here the same two places
apart in the grid: the station's atrium with the lift ladder and the exit block, the west wing
with the tank's lane and the fan shaft, the alcove of teleporter 330; the cistern's arrival
room over the water tank, the channel, the gallery, the door, the drop and the corridor to
teleporter 331, whose partner hangs over a ledge high in the atrium.

    python3 tools/worlds/world051.py                   # writes worlds3d/world051.vwr
    python3 tools/worlds/world051.py --stage K out.vwr # lessons 1..K solved, for levelcheck (which has
        no lifts to jump between, timing, swimming or the drink):
        1 the lift ladder climbed (a stand-in lift), 2 the moving mines and tanks timed (left out),
        3 the tank swum (a stand-in lift), 4 the treasure door open, 5 the corridor crossed unseen
        (its mines, saws and tank left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine, rect  # noqa: E402
from station import (BLUE_PANEL, COBWEBS, FAN_LEFT, GIRDER, GRILLE, PLANTS, STONE_PILLAR,  # noqa: E402
                     WATER_WEED, WIND_LEFT, brick, metal, shell, treasure_door, water)
from station import DEEP_WATER  # noqa: E402

TP_STATION = 330          # alpha: the station's fan shaft <-> the cistern's arrival room
TP_RETURN = 331           # beta: the cistern's corridor <-> the ledge over the atrium
SPAWN = (27, 20, 50)


def patrol(w, otype, start, end, ticks, stop):
    w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(51)
    w = World(mission=51, sky=24)

    def plates(x0, y0, z0, x1, y1, z1):
        metal(w, x0, y0, z0, x1, y1, z1, rng)

    def bricks(x0, y0, z0, x1, y1, z1):
        brick(w, x0, y0, z0, x1, y1, z1, rng)

    # ---- The station ------------------------------------------------------------------------
    # The atrium (inside x 24-37, z 37-52, walk 20), open to the sky above its walls; its south
    # wall low, so the camera looks in over it.
    plates(23, 18, 36, 38, 19, 53)
    plates(23, 20, 36, 23, 40, 53)
    plates(38, 20, 36, 38, 40, 53)
    plates(23, 20, 36, 38, 40, 36)
    plates(23, 20, 53, 38, 23, 53)
    for y in (31, 35):                                   # bands of grilles on its walls
        for z in range(38, 52, 3):
            w.set(23, y, z, GRILLE)
            w.set(38, y, z, GRILLE)
    for x in range(25, 37, 2):
        w.set(x, 33, 36, GIRDER)
    # The exit block against the east wall (x 34-37, z 37-48), its top walk 29: the red arrow
    #    and an egg on it, plants.
    plates(34, 20, 37, 37, 28, 48)
    w.place(O.EXIT, 36, 29, 39)
    w.place(O.EGG, 36, 29, 46)
    w.bigdecor(PLANTS[0], 37, 29, 42)
    # The lift ladder: four levels two cells apart, each a 2 x 2 platform shuttling five cells
    #    along Z (50 ticks, stop 10, the original's) in alternate directions; levels 1 and 3 in
    #    lane x 30-31, 2 and 4 in lane x 32-33, so each next level passes beside the last and
    #    Blupi jumps across and up as they pass; from level 4 a jump east onto the block.
    for k, y in enumerate((20, 22, 24, 26)):
        xs = (30, 31) if k % 2 == 0 else (32, 33)
        a, b = (41, 46) if k % 2 == 0 else (46, 41)
        for x in xs:
            for dz in (0, 1):
                lift(w, (x, y, a + dz), (x, y, b + dz), ticks=50, stop=10)
    if stage >= 1:                           # levelcheck cannot jump between lifts: a stand-in
        w.place(O.LIFT, 29, 19, 50, end=(33, 28, 40))
    # The ledge of teleporter 331's partner high in the north-west corner (walk 38): Blupi
    #    arrives one cell +Z of the pillar and walks off it into the atrium.
    plates(24, 37, 37, 28, 37, 41)
    w.set(26, 39, 38, TP_RETURN)
    # A mine that never moves, in the corner by the west door (the original's still one).
    w.place(O.MOVING_MINE, 25, 20, 40)

    # The west wing. W1, a corridor west from the atrium's door (x 13-22, z 44-46, walk 20).
    w.fill(23, 20, 44, 23, 22, 46, 0)
    plates(13, 18, 43, 22, 19, 47)
    plates(13, 20, 43, 22, 23, 43)
    plates(13, 20, 47, 22, 23, 47)
    plates(13, 24, 43, 22, 24, 47)
    for x in range(14, 23, 2):
        w.set(x, 19, 45, GRILLE)
    # The tank's lane crosses it at x 17 (z 37-54, a bay at its south end with the two eggs):
    #    the tank drives its length and fires along it at each turn; cross between its shots.
    plates(16, 18, 36, 18, 24, 55)
    w.fill(17, 20, 37, 17, 23, 54, 0)
    w.fill(16, 20, 44, 18, 23, 46, 0)
    plates(15, 18, 53, 19, 24, 56)
    w.fill(16, 20, 53, 18, 22, 55, 0)
    w.place(O.EGG, 16, 20, 55)
    w.place(O.EGG, 18, 20, 55)
    if stage < 2:
        patrol(w, O.CREATURE, (17, 20, 38), (17, 20, 52), ticks=140, stop=23)

    # W2, the fan shaft (inside x 4-12, z 38-50, walk 20, roofed at 35), entered from W1.
    shell(w, 3, 19, 37, 13, 35, 51, plates)
    w.fill(13, 20, 44, 13, 22, 46, 0)
    for x in range(4, 13, 3):
        w.set(x, 19, 39, GRILLE)
    # A climb on blue panels, each a jump (two up) beside the last: P1 by the south wall
    #    (walk 22), P2 north of it (24), then onto the wind deck's east end (26).
    w.fill(9, 21, 49, 10, 21, 50, BLUE_PANEL)
    w.fill(9, 23, 46, 10, 23, 48, BLUE_PANEL)
    # The wind deck (x 7-11, z 44-45, walk 26): four fans at its east end (x 12, two high)
    #    blow -X along it; the wind carries Blupi west onto the landing (x 4-6, z 43-46). The
    #    fans' mouths are the deck's east cells (x 11): a landing there kills.
    w.fill(4, 25, 43, 6, 25, 46, BLUE_PANEL)
    w.fill(7, 25, 44, 11, 25, 45, BLUE_PANEL)
    for y in (26, 27):
        for z in (44, 45):
            w.set(12, y, z, FAN_LEFT)
            w.fill(7, y, z, 11, y, z, WIND_LEFT)
    # Moving mines in the wind (30 ticks, no stop: the original's): one sweeping across the
    #    deck's two lanes at x 8, which the wind carries Blupi past (jump when it is in the
    #    other lane); one sweeping P1.
    if stage < 2:
        patrol(w, O.MOVING_MINE, (8, 26, 44), (8, 26, 45), ticks=30, stop=0)
        patrol(w, O.MOVING_MINE, (9, 22, 49), (10, 22, 50), ticks=30, stop=0)
    # The ferry: two lifts in step shuttling from the landing's south edge to the alcove in the
    #    shaft's south wall (30 ticks for three cells, stop 10).
    for x in (5, 6):
        lift(w, (x, 25, 47), (x, 25, 50), ticks=30, stop=10)
    # The alcove (x 4-7, z 51-54, walk 26) with teleporter 330 hanging over its floor.
    plates(3, 25, 51, 8, 28, 55)
    w.fill(4, 26, 51, 7, 27, 54, 0)
    w.set(5, 27, 52, TP_STATION)

    # ---- The cistern -------------------------------------------------------------------------
    # The arrival room (inside x 69-77, z 34-40, walk 40) over the water tank's north bank:
    #    Blupi arrives one cell +Z of teleporter 330's pillar and walks south into the water.
    shell(w, 68, 39, 33, 78, 44, 40, bricks)
    w.set(72, 41, 35, TP_STATION)
    for x, y, z in ((69, 43, 34), (77, 43, 34)):
        w.set(x, y, z, COBWEBS[(x + z) % 3])
    # The tank (inside x 69-77, z 41-50): water from y 29 to its surface at 37, two below the
    #    arrival room's floor: Blupi drops in but cannot climb back out (the original's arrival
    #    is one way too: no floor under its teleporter); six fish at six depths.
    shell(w, 68, 28, 40, 78, 44, 51, bricks, roof=False)
    w.fill(69, 40, 40, 77, 43, 40, 0)                     # the room opens onto the tank
    water(w, 69, 29, 41, 77, 37, 50)
    for a, b in (((70, 36, 44), (76, 36, 44)), ((73, 35, 42), (73, 35, 49)), ((76, 33, 47), (70, 33, 47)),
                 ((71, 32, 49), (71, 32, 42)), ((70, 30, 43), (76, 30, 43)), ((75, 29, 49), (75, 29, 47))):
        patrol(w, O.FISH, a, b, ticks=45, stop=47)
    for x, z in ((70, 50), (74, 41), (77, 46)):
        w.bigdecor(WATER_WEED, x, 29, z)
    # The channel west from the tank's foot (x 63-68, z 44-46, y 29-31) into the gallery's pool:
    #    about six cells down, six across and two up, well within the air (the swim is slow).
    bricks(62, 28, 43, 68, 32, 47)
    w.fill(63, 29, 44, 68, 31, 46, DEEP_WATER)
    patrol(w, O.FISH, (64, 30, 45), (67, 30, 45), ticks=15, stop=47)
    patrol(w, O.FISH, (66, 29, 44), (64, 29, 46), ticks=30, stop=47)
    if stage >= 3:                           # levelcheck cannot swim: a stand-in lift through
        w.place(O.LIFT, 73, 39, 39, end=(60, 33, 42))
    # The gallery (inside x 50-62, z 36-54, walk 34): its pool (x 59-62, z 43-47) rises from the
    #    channel to the floor's level; stone pillars, plants, cobwebs; the two treasures at its
    #    west end behind a mine sweeping along it; two eggs in its corners.
    shell(w, 49, 33, 35, 63, 41, 55, bricks)
    bricks(50, 28, 36, 62, 32, 54)
    water(w, 59, 29, 43, 62, 33, 47)
    patrol(w, O.FISH, (60, 32, 44), (60, 32, 46), ticks=30, stop=47)
    patrol(w, O.FISH, (61, 30, 46), (59, 30, 44), ticks=45, stop=47)
    for x in (53, 56):
        for z in (39, 51):
            w.fill(x, 34, z, x, 40, z, STONE_PILLAR)
    for x, z, p in ((50, 36, 0), (57, 36, 1), (50, 54, 2), (62, 37, 0), (62, 54, 1)):
        w.bigdecor(PLANTS[p], x, 34, z)
    for x, z in ((50, 36), (62, 36), (50, 54), (62, 54)):
        w.set(x, 40, z, COBWEBS[(x + z) % 3])
    w.place(O.TREASURE, 51, 34, 43)
    w.place(O.TREASURE, 51, 34, 47)
    w.place(O.EGG, 50, 34, 38)
    w.place(O.EGG, 50, 34, 52)
    if stage < 2:
        patrol(w, O.MOVING_MINE, (53, 34, 41), (53, 34, 49), ticks=80, stop=0)
    # The treasure door "2" in the gallery's south wall (x 55-56, two high), opening onto the
    #    drop: a shaft (x 54-57, z 56-59) fourteen cells down to the corridor.
    shell(w, 53, 18, 55, 58, 41, 60, bricks, floor=False)
    door = treasure_door(2)
    for x in (55, 56):
        for y in (34, 35):
            w.set(x, y, 55, 0 if stage >= 4 else door)
    # The corridor (x 54-77, z 57-59, walk 20, roofed at 24): the drink at its foot, then a mine
    #    sweeping across, a band of mines with mines hanging over it, saws, a second sweeping mine,
    #    a second band under two bobbing mines, the second tank; teleporter 331 in the room at its
    #    end. Only unseen can Blupi cross: twenty seconds, enough for the corridor and the jump.
    bricks(53, 18, 56, 78, 19, 60)
    bricks(53, 20, 56, 78, 24, 56)
    bricks(53, 20, 60, 78, 24, 60)
    bricks(58, 24, 56, 78, 24, 60)
    w.fill(54, 20, 56, 57, 24, 56, 0)                    # the drop reaches the floor
    w.fill(54, 20, 57, 58, 23, 59, 0)                    # the shaft's foot opens east
    w.place(O.DRINK, 56, 20, 58)
    if stage < 5:
        patrol(w, O.MOVING_MINE, (60, 20, 57), (60, 20, 59), ticks=20, stop=0)
        for x in (62, 63):
            for z in (57, 58, 59):
                mine(w, x, 20, z)
        for x, z in ((62, 57), (63, 58), (62, 59)):
            mine(w, x, 21, z, hanging=True)
        for z in (57, 58, 59):
            w.set(66, 19, z, 378)
        patrol(w, O.MOVING_MINE, (68, 20, 59), (68, 20, 57), ticks=20, stop=0)
        for x in (70, 71):
            for z in (57, 58, 59):
                mine(w, x, 20, z)
        patrol(w, O.MOVING_MINE, (70, 21, 57), (70, 22, 57), ticks=10, stop=0)
        patrol(w, O.MOVING_MINE, (71, 21, 59), (71, 22, 59), ticks=10, stop=0)
        patrol(w, O.CREATURE, (73, 20, 58), (76, 20, 58), ticks=40, stop=23)
    # The room of teleporter 331 (inside x 78-84, z 55-61, walk 20) and an egg.
    shell(w, 77, 19, 54, 85, 25, 62, bricks)
    w.fill(77, 20, 57, 77, 23, 59, 0)
    w.set(81, 21, 56, TP_RETURN)
    w.place(O.EGG, 83, 20, 61)
    w.bigdecor(PLANTS[1], 79, 20, 61)
    w.set(78, 24, 55, COBWEBS[0])
    w.set(84, 24, 61, COBWEBS[2])

    w.spawn = SPAWN
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world051.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
