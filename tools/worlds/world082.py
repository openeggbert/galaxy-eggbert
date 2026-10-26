#!/usr/bin/env python3
"""Mission 082, the mine shaft works (worlds3d/world082.vwr): brief plans/briefs/world082.md.

Mobile Eggbert's level 082 climbs a machine of panels and pipes under a dark sky: from Blupi's
start in a room walled with pipe, a lift up and a corridor of small shuttles to the first
treasure; a shaft where two lifts meet midway through a swarm of moving mines (16) to the
second; lifts meeting in pairs across the sky to door "2" (422); a gallery of six treasures, each
in a niche under a hanging mine (3), shuttles over lava and mines bobbing out of pits in its
floor; a tower of three lifts flanked by hanging mines up to the red arrow. Here the climb runs
south to north in 3D: the start room, the shuttle corridor, the shaft and its swarm, the relay in
the sky, the gallery along Z, the tower and the arrow's room.

    python3 tools/worlds/world082.py                 # writes worlds3d/world082.vwr
    python3 tools/worlds/world082.py --stage K out   # for levelcheck: K treasures taken, door "2"
                                                     # open from K >= 2; K >= 1 no moving or
                                                     # hanging mines, the shuttles as floors
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from skyworks import BOLTS, PANEL, PIPE_MACHINES, PIPES, panel  # noqa: E402

LAVA = 68
DOOR_2 = 420 + 2


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world082.vwr"
    rng = random.Random(82)
    w = World(mission=82, sky=11)
    calm = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def drifter(start, end, ticks):
        """A moving mine on a short beat."""
        if not calm:
            w.place(O.MOVING_MINE, *start, end=end, advance=ticks, recede=ticks, stop_start=0, stop_end=0)

    def hanger(x, y, z):
        if not calm:
            mine(w, x, y, z, hanging=True)

    def shuttle(cells, end_offset, ticks, stop, floor_y):
        """Lifts along X (each cell of `cells` one lift, all in step), or in the calm stages a floor
        over their whole run."""
        for x, y, z in cells:
            if calm:
                w.fill(min(x, x + end_offset), y, z, max(x, x + end_offset), y, z, PANEL)
            else:
                lift(w, (x, y, z), (x + end_offset, y, z), ticks=ticks, stop=stop)

    # The whole machine stands in a shell of panel, pipe on its outer faces.
    panel(w, 8, 6, 60, 22, 18, 84, rng)                               # the start room's block
    panel(w, 14, 16, 64, 58, 24, 76, rng)                              # the corridor's
    panel(w, 46, 16, 62, 58, 48, 78, rng)                              # the shaft's
    panel(w, 78, 36, 12, 92, 64, 70, rng)                              # the gallery's and tower's

    # ---- the start room (walk 10) and the lift up ----
    carve(10, 10, 70, 20, 14, 80)
    for x in range(10, 21):
        w.set(x, 10, 80, rng.choice(PIPES))                            # pipe along its south wall
    for z in range(70, 80):
        w.set(10, 10, z, rng.choice(PIPES))
    carve(18, 9, 70, 19, 22, 71)
    for x in (18, 19):
        for z in (70, 71):
            lift(w, (x, 9, z), (x, 19, z), ticks=60, stop=3)

    # ---- the shuttle corridor (walk 20) ----
    # East, two wide, four gaps across it bridged by small shuttles (20 ticks, 10 dwell); a gap
    # drops back into the start room's block. Treasure 1 at its end.
    carve(16, 20, 70, 46, 23, 71)
    for x0 in (24, 30, 36, 42):
        carve(x0, 14, 70, x0 + 2, 19, 71)
        shuttle([(x0, 19, 70), (x0, 19, 71)], 2, 20, 10, 19)
    w.place(O.TREASURE, 46, 20, 71)

    # ---- the shaft (walk 20 -> 40) ----
    # Nine by nine and twenty-one high: a lift rises from its floor, another sinks from the top
    # ledge, they meet level beside each other midway (walk 31); twenty moving mines drift
    # round their way on short beats. Treasure 2 on the top ledge, the way out east.
    carve(48, 20, 66, 56, 46, 74)
    carve(47, 20, 70, 47, 23, 71)
    for x in (50, 51):
        for z in (69, 70):
            lift(w, (x, 19, z), (x, 30, z), ticks=70, stop=20)
    for x in (52, 53):
        for z in (69, 70):
            lift(w, (x, 39, z), (x, 30, z), ticks=70, stop=20)
    w.fill(52, 39, 66, 56, 39, 68, PANEL)                              # the top ledge, walk 40
    w.place(O.TREASURE, 55, 40, 66)
    swarm = [((49, 22, 72), (49, 25, 72), 20), ((53, 24, 72), (53, 24, 74), 10), ((48, 27, 68), (50, 27, 68), 16),
             ((54, 26, 69), (54, 29, 69), 20), ((50, 31, 72), (52, 31, 72), 12), ((49, 30, 69), (49, 33, 69), 24),
             ((55, 31, 71), (55, 34, 71), 18), ((51, 34, 67), (53, 34, 67), 14), ((48, 35, 71), (48, 38, 71), 20),
             ((54, 36, 72), (52, 36, 72), 12), ((50, 37, 69), (50, 40, 69), 22), ((55, 38, 69), (55, 41, 69), 20),
             ((52, 42, 71), (54, 42, 71), 14), ((49, 41, 67), (49, 44, 67), 24), ((51, 25, 67), (51, 28, 67), 20),
             ((53, 28, 73), (53, 31, 73), 16), ((48, 24, 74), (51, 24, 74), 18), ((56, 23, 70), (56, 26, 70), 12),
             ((50, 43, 73), (53, 43, 73), 20), ((54, 33, 66), (54, 36, 66), 16)]
    for start, end, ticks in swarm:
        drifter(start, end, ticks)

    # ---- the relay in the sky (walk 40) ----
    # From the ledge east: two pairs of lifts, each pair meeting midway (50 ticks, 10 dwell), a
    # rest of panel between the pairs; door "2" at the far end.
    carve(57, 39, 66, 58, 42, 67)                                     # out through the wall
    w.fill(57, 39, 66, 57, 39, 67, PANEL)
    shuttle([(58, 39, 66), (58, 39, 67)], 5, 50, 10, 39)              # meets the next at 63/64
    shuttle([(68, 39, 66), (68, 39, 67)], -4, 50, 10, 39)
    w.fill(69, 37, 65, 70, 39, 68, PANEL)
    carve(77, 39, 66, 84, 43, 67)                                     # into the gallery's block
    shuttle([(71, 39, 66), (71, 39, 67)], 5, 50, 10, 39)              # meets the next at 76/77
    shuttle([(81, 39, 66), (81, 39, 67)], -4, 50, 10, 39)
    w.fill(82, 37, 66, 84, 39, 67, PANEL)                               # the landing at the door
    for x in range(69, 71):
        w.set(x, 36, 66, rng.choice(PIPES))

    # ---- the gallery (walk 40, along -Z) ----
    # Door "2" in its south end, an egg by it; a lava pit across the gallery crossed on two
    # shuttles along Z (30 ticks) with a rest between them; six niches in its west wall, a
    # treasure in each under a hanging mine (walk in, never jump); three pits across the floor, two
    # mines bobbing up out of each; a last gap to jump, a mine by it, into the tower.
    carve(82, 40, 65, 83, 41, 65)
    if stage is None or stage < 2:
        w.fill(82, 40, 65, 83, 41, 65, DOOR_2)
    carve(80, 40, 29, 88, 43, 64)
    w.place(O.EGG, 87, 40, 63)
    carve(80, 36, 54, 88, 39, 62)
    w.fill(80, 36, 54, 88, 36, 62, LAVA)
    w.fill(83, 36, 58, 84, 39, 58, PANEL)                               # the rest
    if calm:
        w.fill(83, 39, 54, 84, 39, 62, PANEL)
    else:
        for x in (83, 84):
            lift(w, (x, 39, 62), (x, 39, 59), ticks=30, stop=10)
            lift(w, (x, 39, 57), (x, 39, 54), ticks=30, stop=10)
    for z in (52, 47, 42, 38, 34, 31):
        carve(79, 40, z, 79, 41, z)
        w.place(O.TREASURE, 79, 40, z)
        hanger(79, 41, z)
    for z in (50, 44, 36):
        carve(80, 37, z, 88, 39, z)
        for x in (82, 86):
            drifter((x, 37, z), (x, 41, z), 20)
        if calm:
            w.fill(80, 39, z, 88, 39, z, PANEL)
    carve(80, 37, 29, 88, 39, 29)
    if calm:
        w.fill(80, 39, 29, 88, 39, 29, PANEL)
    else:
        mine(w, 81, 40, 30)

    # ---- the tower (walk 40 -> 58) and the arrow's room ----
    # North of the gallery a shaft: three lifts stacked in one column (2 x 2, 40 ticks, 10
    # dwell), each rising four beside a shelf of the north wall two above its top; hanging mines
    # over the shelves' ends, a mine on two of them. At the top the arrow's room: a gap two wide
    # across it bridged by a shuttle, a mine beyond.
    carve(82, 40, 21, 88, 60, 28)
    carve(84, 39, 23, 85, 39, 24)
    for bottom in (39, 45, 51):
        for x in (84, 85):
            for z in (23, 24):
                lift(w, (x, bottom, z), (x, bottom + 4, z), ticks=40, stop=10)
        shelf = bottom + 6                                               # walk bottom + 7
        w.fill(82, 40 if bottom == 39 else shelf, 21, 88, shelf, 22, PANEL)  # the upper two float
        hanger(82, shelf + 2, 22)
        hanger(88, shelf + 2, 22)
    mine(w, 87, 46, 22)
    mine(w, 83, 52, 22)
    carve(80, 58, 12, 88, 61, 20)
    carve(80, 54, 16, 88, 57, 17)                                       # the gap
    if calm:
        w.fill(83, 57, 16, 84, 57, 17, PANEL)
    else:
        for x in (83, 84):
            lift(w, (x, 57, 18), (x, 57, 15), ticks=20, stop=10)
        mine(w, 86, 58, 14)
    w.place(O.EXIT, 84, 58, 13)
    for x, z in ((81, 13), (87, 19)):
        w.set(x, 58, z, rng.choice(BOLTS))
    for y in range(58, 61):
        w.set(80, y, 20, rng.choice(PIPE_MACHINES))

    w.spawn = (12, 10, 78)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
