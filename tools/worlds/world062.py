#!/usr/bin/env python3
"""Mission 062, the slime labyrinth (worlds3d/world062.vwr): brief plans/briefs/world062.md.

Mobile Eggbert's level 062 is a labyrinth of slime tunnels with six treasures, two keys and
their doors (the green 50 opens 335, the blue 51 opens 336), and the red arrow far from the
start behind a door that wants all six treasures. On the way: a column of lightning (305 under
its emitter 304), lava crossed on crumbling bridges (364), rows of saws, four big creatures,
nine moving mines, spike traps (373), lifts and a water channel. Here the labyrinth is two
levels and a flooded one: a machine start room, the lightning tunnel, the lava gallery, the saw
corridor with the green key, the exit room; below, the creatures' hub, the moving mines' cross
corridor and the spike traps with the blue key, the corridor behind the green door, the dive
behind the blue door.

    python3 tools/worlds/world062.py                 # writes worlds3d/world062.vwr
    python3 tools/worlds/world062.py --stage K out   # for levelcheck: K >= 1 no creatures or
                                                     # moving mines, the shuttle a floor; K >= 2
                                                     # the key doors open, the water floors;
                                                     # K >= 3 the exit's door open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine  # noqa: E402
from slime import skin, slime  # noqa: E402

LAVA, WATER, DEEP = 68, 92, 91
BLITZ, EMITTER = 305, 304
SAW, TRAP, DRIP, BRIDGE, GIRDER = 378, 373, 404, 364, 201
GREEN_DOOR, BLUE_DOOR = 335, 336
EXIT_DOOR = 420 + 6          # opens once all six treasures are taken


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world062.vwr"
    rng = random.Random(62)
    w = World(mission=62, sky=4)
    slime(w, 14, 0, 2, 82, 30, 63)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def bar(x0, y0, z0, x1, y1, z1, tile, open_from):
        """A door filling a doorway, gone in the stages from `open_from` on."""
        if stage is None or stage < open_from:
            w.fill(x0, y0, z0, x1, y1, z1, tile)

    # ---- the upper level (walk 21) ----
    # U1, the machine start room: lined with machine panels, a girder over it.
    carve(18, 21, 26, 28, 25, 36)
    for x in range(17, 30):
        for y in range(20, 27):
            for z in range(25, 38):
                if w.get(x, y, z) != 0:
                    wall = 21 <= y <= 25          # faces on the walls only: their tops are open
                    w.set(x, y, z, rng.choice(MACHINE_FACES) if wall and rng.random() < 0.25 else MACHINE_FLOOR)
    w.fill(19, 25, 31, 27, 25, 31, GIRDER)
    # T1, the lightning tunnel east: a curtain of lightning under its emitter, quiet half the time.
    carve(29, 21, 30, 44, 23, 32)
    w.fill(37, 21, 30, 37, 23, 32, BLITZ)
    w.fill(37, 24, 30, 37, 24, 32, EMITTER)
    for x in (31, 34, 41):
        w.place(O.EGG, x, 21, 31)
    # U2, the lava gallery: lava over the floor but a rim, an island (treasure 1) reached on
    # crumbling bridges; a pillar in the lava (treasure 6) reached on a shuttle from the south rim.
    carve(45, 21, 22, 61, 26, 40)
    w.fill(47, 20, 24, 59, 20, 38, LAVA)
    slime(w, 51, 20, 28, 55, 20, 34)                                   # the island
    w.fill(47, 20, 31, 50, 20, 31, BRIDGE)
    w.fill(56, 20, 31, 59, 20, 31, BRIDGE)
    w.place(O.TREASURE, 53, 21, 31)
    slime(w, 56, 20, 35, 56, 22, 35)                                   # the pillar, walk 23
    w.place(O.TREASURE, 56, 23, 35)
    if stage is not None and stage >= 1:
        w.fill(56, 21, 36, 56, 21, 39, MACHINE_FLOOR)                  # the shuttle as a floor
    else:
        lift(w, (56, 21, 39), (56, 21, 36), ticks=20, stop=10)         # walk 22, a step up from the rim
    mine(w, 46, 21, 23)
    for x, z in ((46, 38), (60, 23), (53, 29)):
        w.place(O.EGG, x, 21, z)
    # U3, the saw corridor north (five wide), three rows of saws across it, the green key and
    # treasure 2 in the room at its end.
    carve(52, 21, 21, 54, 23, 21)
    carve(51, 21, 10, 55, 23, 20)
    carve(46, 21, 5, 60, 24, 9)
    for z in (18, 15, 12):
        w.fill(51, 21, z, 55, 21, z, SAW)
    w.place(O.KEY2, 47, 21, 6)
    w.place(O.TREASURE, 59, 21, 6)
    w.place(O.EGG, 53, 21, 7)
    # U4, the exit room east, behind the door that wants all six treasures.
    carve(62, 21, 27, 65, 22, 29)
    bar(63, 21, 27, 63, 22, 29, EXIT_DOOR, 3)
    carve(66, 21, 22, 78, 25, 34)
    w.place(O.EXIT, 75, 21, 28)
    w.fill(67, 25, 23, 77, 25, 23, GIRDER)

    # ---- down and up ----
    carve(58, 11, 40, 60, 20, 43)                                      # the drop out of U2
    carve(46, 11, 43, 46, 20, 43)                                      # the lift back up
    carve(46, 21, 41, 46, 23, 43)
    w.set(46, 10, 43, 0)
    lift(w, (46, 10, 43), (46, 20, 43), ticks=40, stop=20)

    # ---- the lower level (walk 11) ----
    # L1, the creatures' hub. A creature rests in the doorway west and walks off it.
    carve(44, 11, 44, 62, 15, 56)
    carve(43, 11, 49, 43, 12, 49)
    if stage is None or stage < 1:
        w.place(O.BIG_CREATURE, 44, 11, 49, end=(52, 11, 49), advance=80, recede=80, stop_start=200,
                stop_end=200)
    w.set(56, 15, 52, DRIP)
    mine(w, 61, 11, 55)
    for x, z in ((50, 46), (60, 47), (48, 54)):
        w.place(O.EGG, x, 11, z)
    # L2, west: a cross corridor (three wide) under nine moving mines in three columns that rise
    # and fall in turn, then the room with the spike traps and the blue key; treasure 3.
    carve(35, 11, 48, 42, 14, 50)
    carve(24, 11, 42, 34, 15, 56)
    for k, x in enumerate((36, 38, 40)):
        low, high = (11, 14) if k % 2 == 0 else (14, 11)
        if stage is None or stage < 1:
            for z in (48, 49, 50):
                w.place(O.MOVING_MINE, x, low, z, end=(x, high, z), advance=24, recede=24, stop_start=6,
                        stop_end=6)
    w.fill(28, 10, 42, 28, 10, 56, TRAP)                               # a strip right across the room
    w.place(O.KEY3, 25, 11, 44)
    w.place(O.TREASURE, 25, 11, 54)
    for x, z in ((32, 43), (25, 51)):
        w.place(O.EGG, x, 11, z)
    # L3 behind the green door: a corridor one wide along X; a creature walks its east half and
    # rests at both ends, the east one before treasure 4.
    carve(53, 11, 57, 53, 12, 59)
    bar(53, 11, 57, 53, 12, 57, GREEN_DOOR, 2)
    carve(46, 11, 60, 60, 12, 60)
    if stage is None or stage < 1:
        w.place(O.BIG_CREATURE, 54, 11, 60, end=(59, 11, 60), advance=60, recede=60, stop_start=200,
                stop_end=200)
    w.place(O.TREASURE, 60, 11, 60)
    w.place(O.EGG, 47, 11, 60)

    # ---- the flooded level behind the blue door ----
    # From L2's south side a ledge over a pool; a channel dived east under the creatures' hub to
    # a second pool; out onto its bank: treasure 5.
    carve(30, 11, 57, 30, 12, 57)
    bar(30, 11, 57, 30, 12, 57, BLUE_DOOR, 2)
    carve(26, 3, 58, 34, 12, 61)
    w.fill(26, 3, 58, 34, 8, 61, DEEP)
    w.fill(26, 8, 58, 34, 8, 61, WATER)
    slime(w, 29, 3, 58, 31, 10, 58)                                    # the ledge, walk 11
    # The step back up, walk 9: level with the water's top cell (y 8), the only bank Blupi climbs
    #    out onto from water; a jump up from it onto the ledge (walk 11).
    slime(w, 32, 3, 58, 32, 8, 58)
    carve(30, 11, 58, 30, 12, 58)
    carve(35, 3, 59, 50, 5, 61)
    w.fill(35, 3, 59, 50, 5, 61, DEEP)
    carve(51, 3, 58, 58, 9, 61)
    w.fill(51, 3, 58, 56, 6, 61, DEEP)
    w.fill(51, 6, 58, 56, 6, 61, WATER)
    slime(w, 57, 3, 58, 58, 6, 61)                                     # the bank, walk 7
    w.place(O.TREASURE, 58, 7, 60)
    w.place(O.EGG, 57, 7, 58)
    if stage is not None and stage >= 2:
        # levelcheck does not swim: the flooded way as floors (one way in is enough to check).
        w.fill(26, 4, 58, 34, 8, 61, 0)
        w.fill(35, 4, 59, 50, 5, 61, 0)
        w.fill(51, 4, 58, 56, 6, 61, 0)
        slime(w, 26, 3, 58, 56, 3, 61)
        slime(w, 52, 4, 58, 56, 5, 61)                                 # a jump up out of the tunnel
        slime(w, 32, 4, 58, 32, 8, 58)                                 # the step kept, and the swim
        w.place(O.LIFT, 32, 3, 59, end=(32, 8, 59))                    # up to it as a stand-in lift

    skin(w, rng)
    w.spawn = (23, 21, 34)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
