#!/usr/bin/env python3
"""Mission 123, the library (worlds3d/world123.vwr): brief plans/briefs/world123.md.

Mobile Eggbert's level 123 is a library in brick: Blupi starts by the exit in its south-west
corner; bookcases (139-143) stand in stacks, the fifteen treasures on top of them and between
them, climbed from case to case; treasure doors (422 "2", 424 "4", 429 "9", 432 "12") shut the
ways on as he collects; a lift (60 ticks) up to the east wing, two crates (12) to push into a step,
a pit of lava, three eggs. Here the library is a great hall of brick in 3D: rows of bookcases of
different heights to climb, the doors shutting its wings, the east wing up the lift, the gallery
round the top.

    python3 tools/worlds/world123.py                 # writes worlds3d/world123.vwr
    python3 tools/worlds/world123.py --stage K out   # for levelcheck: K treasures taken (the
                                                     # doors opening as they count), the crates
                                                     # pushed into their step
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift  # noqa: E402
from station import BRICK, brick, treasure_door  # noqa: E402

BOOKCASES = (139, 140, 141, 142, 143)
LAVA = 68
FLOOR = 6                    # the hall's floor: walk 7


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world123.vwr"
    rng = random.Random(123)
    w = World(mission=123, sky=19)
    taken = stage or 0
    walk = FLOOR + 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def bookcase(x0, z0, x1, z1, height):
        """A stack of bookcases `height` high on the floor (their tops a walk height + 1 above)."""
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                for y in range(walk, walk + height):
                    w.set(x, y, z, rng.choice(BOOKCASES))

    def door(cells, n):
        for x, y, z in cells:
            carve(x, y, z, x, y, z)
            if taken < n:
                w.set(x, y, z, treasure_door(n))

    brick(w, 4, 0, 20, 96, 30, 80, rng)

    # ---- the west hall (walk 7) ----
    # Spawn by the exit in its south-west corner. Bookcases in rows of rising height: one, two,
    # three, four and five high, each row a jump above the last; treasures on their tops and in
    # the aisles; door "2" east out of the hall.
    carve(6, walk, 24, 40, walk + 12, 76)
    w.spawn = (8, walk, 72)
    w.place(O.EXIT, 7, walk, 75)
    # The stacks: rows across the hall's width north of the start, each two higher than the one in
    # front of it and touching it, so the stack is a stair of bookcases (walk 9, 11, 13, 15, 17),
    # a treasure on each row.
    rows = ((64, 67, 2), (60, 63, 4), (56, 59, 6), (52, 55, 8), (46, 51, 10))
    for z0, z1, height in rows:
        bookcase(12, z0, 34, z1, height)
    for x, y, z in ((20, walk + 2, 66), (30, walk + 4, 61), (16, walk + 6, 57), (28, walk + 8, 53),
                    (22, walk + 10, 48), (38, walk, 63), (8, walk, 30)):
        w.place(O.TREASURE, x, y, z)
    w.place(O.EGG, 39, walk, 74)
    door([(41, y, z) for y in (walk, walk + 1) for z in (54, 55, 56)], 2)

    # ---- the middle hall (walk 7) ----
    # A pit of lava across it crossed on top of a row of bookcases two high; two crates to push
    # into a step up to the high shelf (walk 11) with three treasures; door "4" east.
    carve(42, walk, 40, 62, walk + 8, 70)
    carve(48, FLOOR - 2, 40, 54, FLOOR, 70)
    w.fill(48, FLOOR - 2, 40, 54, FLOOR - 1, 70, LAVA)
    for x in range(48, 55):
        for z in (54, 55, 56):
            w.fill(x, FLOOR - 1, z, x, FLOOR, z, BRICK)
    bookcase(56, 40, 62, 46, 4)                                           # the high shelf, walk 11
    if taken >= 2:
        w.place(O.CRATE, 58, walk, 47)
        w.place(O.CRATE, 58, walk + 1, 47)
        w.place(O.CRATE, 59, walk, 48)
    else:
        w.place(O.CRATE, 44, walk, 64)
        w.place(O.CRATE, 46, walk, 64)
        w.place(O.CRATE, 45, walk, 66)
    for x, z in ((57, 41), (60, 43), (62, 45)):
        w.place(O.TREASURE, x, walk + 4, z)
    w.place(O.EGG, 61, walk, 68)
    door([(63, y, z) for y in (walk, walk + 1) for z in (54, 55, 56)], 4)

    # ---- the east wing (walk 7 -> 17) up the lift ----
    # A lift (2 x 2, 60 ticks, dwell 30) up ten to the east wing's gallery: bookcases along it,
    # three treasures; door "9" to the last room with two more treasures and an egg, and door
    # "12" to its stair back down into the west hall's top (the gallery's end).
    carve(64, walk, 52, 70, walk + 12, 58)
    for x in (66, 67):
        for z in (54, 55):
            lift(w, (x, FLOOR, z), (x, walk + 9, z), ticks=60, stop=30)
    carve(66, FLOOR, 54, 67, FLOOR, 55)
    carve(71, walk + 10, 40, 94, walk + 14, 70)
    w.fill(68, walk + 9, 52, 70, walk + 9, 58, BRICK)
    carve(68, walk + 10, 52, 70, walk + 12, 58)
    for x0 in (74, 80, 86):
        bookcase(x0, 42, x0 + 2, 44, 0)
        for y in range(walk + 10, walk + 12):
            for x in range(x0, x0 + 3):
                w.set(x, y, 42, rng.choice(BOOKCASES))
        w.place(O.TREASURE, x0 + 1, walk + 10, 46)
    door([(x, y, 39) for y in (walk + 10, walk + 11) for x in (88, 89, 90)], 9)
    carve(84, walk + 10, 26, 94, walk + 13, 38)
    w.place(O.TREASURE, 86, walk + 10, 28)
    w.place(O.TREASURE, 92, walk + 10, 28)
    w.place(O.EGG, 89, walk + 10, 27)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
