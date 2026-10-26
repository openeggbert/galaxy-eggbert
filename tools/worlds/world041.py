#!/usr/bin/env python3
"""Mission 041, the helicopter caves (worlds3d/world041.vwr): brief plans/briefs/world041.md.

Mobile Eggbert's level 041, the first of the cheese world: Blupi starts in a cellar beside a
house and a hut; he stacks the two crates there to climb to a shelf where three helicopters
stand, and flies. Treasure 1 lies on a ledge above; a treasure door ("1") opens once it is
taken. Beyond it a cave whose ceilings and ledges hang with mines holds five treasures and
three eggs on its ledges, a row of helicopters on its floor; the door "6" at its top opens with
six treasures onto a corridor over water, where three moving mines rise and fall between a
ceiling of hanging mines and the water (the helicopter is lost in water); at its far end the
last treasure, two eggs and a shaft up to the red arrow. Helicopters stand at every stage, so
one is always at hand. Here the same order is built as caves in a mass of cheese: the cellar
open on the dark rocks, the chimney up to the loft, the tunnel through door 1 to the great
round cave, door 6 high in its wall, the corridor north over the water, the shaft up.

    python3 tools/worlds/world041.py                 # writes worlds3d/world041.vwr
    python3 tools/worlds/world041.py --check STAGE out
        STAGE onecrate: crate A under the shelf, crate B one push short of it (no stack)
        STAGE stacked:  the two crates stacked under the shelf (flightcheck.py from here)
        STAGE door1:    also door 1 open
        STAGE door6:    also door 6 open
        STAGE timed:    also the moving mines left out, as when passed at the right moment
"""

import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from cheese import (CHEESE, DEEP_WATER, HOUSE, HUT, PLANTS, TREASURE_DOOR1, WATER, bubble, carve,  # noqa: E402
                    cheese, hang, helicopters, hole, patrol, tube)

WA = 10                  # the cellar's walking level
WB = 28                  # the loft's
WD = 22                  # the great cave's floor
WE = 38                  # the corridor's landings (the water's top cell is WE - 1)
WF = 52                  # the exit chamber's
SHELF_TOP = WA + 3       # the helicopter shelf: Blupi stands on it at WA + 4
STACK = (22, WA, 70)     # where the two crates go, under the shelf's west end
SPAWN = (36, WA, 80)
CAVE = (60, 58, 12)      # the great cave's axis x, z and radius
DOOR1 = (37, WB, 64)
DOOR6 = (60, WE, 45)


def cellar(w: World, rng: random.Random, stage):
    """A: the cellar (walk 10), open to the south on the dark rocks; the house and the hut;
    the shelf with the helicopters four above the floor; the bench under a low overhang where
    crate B stands, the floor where crate A stands."""
    carve(w, 21, WA, 66, 43, WA + 7, 82)
    hole(w, 32, WA + 7, 74, 11, 4, 8)
    carve(w, 22, WA, 83, 42, WA + 7, 86)                  # the opening to the south
    for x in range(22, 43):
        w.set(x, WA, 86, rng.choice(CHEESE))              # its parapet
    w.bigdecor(HOUSE, 40, WA, 70)
    w.bigdecor(HUT, 40, WA, 77)
    for icon, x, z in ((PLANTS[0], 37, 68), (PLANTS[1], 43, 74), (PLANTS[2], 34, 84), (PLANTS[0], 25, 84)):
        w.bigdecor(icon, x, WA, z)
    # The shelf along the north wall, x 22-30: four high, so one crate is no stair to it.
    cheese(w, 22, WA, 66, 30, SHELF_TOP, 69, rng)
    helicopters(w, [(24, WA + 4, 67), (26, WA + 4, 68), (28, WA + 4, 67)])
    # The bench along the west wall, one high, and the overhang above it: on crate B up there
    # Blupi has no room for a full jump, so B is a stair only once pushed off onto crate A.
    cheese(w, 22, WA, 71, 22, WA, 77, rng)
    cheese(w, 22, WA + 4, 71, 23, WA + 6, 78, rng)
    a, b = (28, WA, 78), (22, WA + 1, 76)
    if stage == "onecrate":
        a, b = STACK, (22, WA + 1, 71)
    elif stage is not None:
        a, b = STACK, (STACK[0], WA + 1, STACK[2])
    w.place(O.CRATE, *a)
    w.place(O.CRATE, *b)


def loft(w: World, rng: random.Random, stage):
    """B: the chimney from the shelf up to the loft (walk 28); treasure 1 on a ledge high in
    its west wall; door 1 in its east wall."""
    for y in range(WA + 4, WB + 1):
        for x, z in ((x, z) for x in range(23, 29) for z in range(65, 71)):
            if (x - 25.5) ** 2 + (z - 67.5) ** 2 <= 2.3 ** 2:
                w.clear(x, y, z)
    bubble(w, 28, WB, 64, 8, 8, 7, rng)
    # The chimney's mouth stays a pit in the loft's floor.
    for x, z in ((x, z) for x in range(23, 29) for z in range(65, 71)):
        if (x - 25.5) ** 2 + (z - 67.5) ** 2 <= 2.3 ** 2:
            w.clear(x, WB - 1, z)
    cheese(w, 21, WB, 61, 24, WB + 2, 63, rng)            # the ledge, three above the floor
    w.place(O.TREASURE, 23, WB + 3, 62)


def tunnel(w: World, rng: random.Random):
    """C: from door 1 a tunnel winds east, a mine hanging at Blupi's height along its north
    side (he passes beside it, or crouches under it), to a mouth high in the great cave's west
    wall."""
    tube(w, (40, WB, 64), (44, WB, 64), floor=WB)
    tube(w, (44, WB, 64), (50, WB, 58), floor=WB)
    cheese(w, 42, WB + 1, 62, 42, WB + 4, 62, rng)        # a drop of cheese from the vault
    hang(w, 42, WB, 62)                                    # with a mine at Blupi's height


def great_cave(w: World, rng: random.Random, stage):
    """D: a round cave (radius 12, floor walk 22, 26 high): helicopters on its floor, five
    cheese ledges at different heights round it with the five treasures and three eggs,
    mines hanging under the ledges and from the ceiling, two lying on ledges; door 6 high in
    its north wall over a landing."""
    cx, cz, r = CAVE
    for x in range(cx - r - 1, cx + r + 2):
        for z in range(cz - r - 1, cz + r + 2):
            if (x - cx) ** 2 + (z - cz) ** 2 <= r * r:
                for y in range(WD, WD + 20):
                    w.clear(x, y, z)
    hole(w, cx, WD + 20, cz, r, 6, r)                      # the dome
    helicopters(w, [(53, WD, 56), (54, WD, 58), (53, WD, 60), (55, WD, 62), (57, WD, 64),
                    (56, WD, 54), (58, WD, 52), (60, WD, 51), (62, WD, 52)])
    ledges = [  # (x0, z0, x1, z1, top): slabs two thick
        (51, 64, 54, 67, WD + 4),       # south-west, low
        (65, 63, 68, 66, WD + 8),       # south-east
        (67, 51, 70, 54, WD + 11),      # east, high
        (55, 47, 58, 50, WD + 13),      # north-west, highest
        (59, 57, 61, 59, WD + 9),       # the middle, on a column
    ]
    for x0, z0, x1, z1, top in ledges:
        cheese(w, x0, top - 1, z0, x1, top, z1, rng)
    cheese(w, 59, WD, 57, 61, WD + 8, 59, rng)             # the middle ledge's column
    w.place(O.TREASURE, 52, WD + 5, 65)
    w.place(O.EGG, 53, WD + 5, 66)
    w.place(O.TREASURE, 67, WD + 9, 65)
    mine(w, 66, WD + 9, 64)
    w.place(O.TREASURE, 69, WD + 12, 52)
    w.place(O.EGG, 68, WD + 12, 53)
    w.place(O.TREASURE, 56, WD + 14, 48)
    mine(w, 57, WD + 14, 49)
    w.place(O.TREASURE, 60, WD + 10, 58)
    w.place(O.EGG, 61, WD + 10, 57)
    # Mines hanging under the ledges and from the dome.
    for x, y, z in ((52, WD + 2, 66), (66, WD + 6, 64), (68, WD + 9, 52), (57, WD + 11, 48),
                    (60, WD + 12, 58), (55, WD + 15, 55), (64, WD + 12, 61)):
        hang(w, x, y, z)
    # Door 6's landing high in the north wall (the door is set in `doors`).
    cheese(w, 58, WE - 3, 46, 62, WE - 1, 48, rng)


def corridor(w: World, rng: random.Random, stage):
    """E and F: the corridor north over the water (walk 38 on its landings), a row of mines
    hanging from its ceiling, three gates of moving mines rising and falling across it; at
    its far end the last treasure, two eggs and the shaft up to the exit chamber."""
    carve(w, 59, WE, 18, 61, WE + 6, 44)
    w.fill(59, WE - 2, 22, 61, WE - 2, 39, DEEP_WATER)
    w.fill(59, WE - 1, 22, 61, WE - 1, 39, WATER)
    helicopters(w, [(59, WE, 41), (60, WE, 42), (61, WE, 41), (59, WE, 43), (61, WE, 43), (60, WE, 44)])
    for x, z in ((60, 22), (60, 27), (59, 32), (61, 33), (60, 37), (60, 40)):
        hang(w, x, WE + 5, z)
    if stage != "timed":
        for k, z in enumerate((35, 30, 25)):
            for x in (59, 60, 61):
                low, high = (x, WE, z), (x, WE + 5, z)
                patrol(w, O.MOVING_MINE, low if k % 2 == 0 else high, high if k % 2 == 0 else low, ticks=50)
    # The far landing: the last treasure, three helicopters, and in a low alcove two eggs
    # either side of a mine hanging at Blupi's height.
    w.place(O.TREASURE, 59, WE, 19)
    carve(w, 62, WE, 18, 62, WE, 20)
    w.place(O.EGG, 62, WE, 18)
    w.place(O.EGG, 62, WE, 20)
    hang(w, 62, WE, 19)
    helicopters(w, [(59, WE, 21), (60, WE, 21), (61, WE, 21)])
    # The shaft up from it and the exit chamber at the top.
    shaft = [(x, z) for x in range(57, 64) for z in range(16, 24) if (x - 60) ** 2 + (z - 19.5) ** 2 <= 2.3 ** 2]
    for y in range(WE + 7, WF):
        for x, z in shaft:
            w.clear(x, y, z)
    bubble(w, 60, WF, 18, 4, 5, 4, rng)
    for x, z in shaft:
        w.clear(x, WF - 1, z)
    w.place(O.EXIT, 60, WF, 16)


def doors(w: World, stage):
    """The treasure doors, two cells high in their doorways: "1" between the loft and the
    tunnel, "6" between the great cave and the corridor. Set last, after the carving."""
    for (x, y, z), icon, open_in in ((DOOR1, TREASURE_DOOR1, ("door1", "door6", "timed")),
                                     (DOOR6, TREASURE_DOOR1 + 5, ("door6", "timed"))):
        for yy in (y, y + 1):
            w.set(x, yy, z, 0 if stage in open_in else icon)


def main(argv) -> int:
    stage = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    rng = random.Random(41)
    w = World(mission=41, sky=6)
    cheese(w, 16, 4, 54, 46, 40, 86, rng)                   # the cellar's and the loft's mass
    cheese(w, 44, 16, 40, 76, 50, 76, rng)                  # the great cave's
    cheese(w, 55, 32, 11, 65, 59, 46, rng)                  # the corridor's and the shaft's
    cellar(w, rng, stage)
    loft(w, rng, stage)
    tunnel(w, rng)
    great_cave(w, rng, stage)
    corridor(w, rng, stage)
    doors(w, stage)
    w.spawn = SPAWN
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world041.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
