#!/usr/bin/env python3
"""Mission 033, the haunted house of the eight wasps (worlds3d/world033.vwr): brief
plans/briefs/world033.md.

Mobile Eggbert's level 033 is the inside of a great house (region 15, the grey smoke behind
it), its walls the blank window panels (190), dark arches (198) and cobwebs (401-403): rooms on
several levels joined by passages, ten treasures spread through them, mines on the floors and
hanging under ceilings (41), eight wasps whose touch puts Blupi in a balloon that carries him up
to the ceilings' mines, two eggs, and a drink that makes him unseen for a while -- just before
the thickest knot of mines, behind which the last treasure lies. The red arrow stands beside
the start: it counts only with all ten treasures. Here the house is built in 3D: an entrance
hall with the arrow, a tall atrium open to the roof with a gallery round it, wings on three
floors west, east and north, the attic joined by a bridge across the atrium, stairs of
two-block steps between the floors.

    python3 tools/worlds/world033.py                 # writes worlds3d/world033.vwr
    python3 tools/worlds/world033.py --check STAGE out
        STAGE timed: the wasps left out (passed at the right moment)
        STAGE drink: also the mine knot before the last treasure (crossed unseen)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from village import BLANK_WINDOW, DOOR, STONE, WINDOWS, patrol  # noqa: E402

ARCH = 198
COBWEBS = (401, 402, 403)
F0, F1, F2 = 5, 12, 19          # the floors' walking levels
SPAWN = (50, F0, 65)


def carve(w, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def doorway(w, x0, z0, x1, z1, y):
    """An opening two high with a frieze of arches above it (passable, cut out)."""
    carve(w, x0, y, z0, x1, y + 1, z1)
    w.fill(x0, y + 2, z0, x1, y + 2, z1, ARCH)


def stair(w, cells, y, rise_x):
    """Three steps of two blocks from walking level y up through the ceiling slab above: the
    three x ranges in `cells` (lowest first) over z 39-41; the slab over them is opened."""
    xs = [x for x0, x1 in cells for x in range(x0, x1 + 1)]
    carve(w, min(xs), y + 5, 39, max(xs), y + 6, 41)
    for k, (x0, x1) in enumerate(cells):
        w.fill(x0, y, 39, x1, y + 1 + 2 * k, 41, BLANK_WINDOW)


def floors(w: World, rng: random.Random, box):
    """Stone flags wherever Blupi walks: a panel tile's flat top face shows stripes."""
    x0, y0, z0, x1, y1, z1 = box
    for x in range(x0, x1 + 1):
        for z in range(z0, z1 + 1):
            for y in range(y0, y1 + 1):
                if w.get(x, y, z) == 0 and w.get(x, y - 1, z) == BLANK_WINDOW:
                    w.set(x, y - 1, z, STONE if rng.random() < 0.6 else 156)


def decorate(w: World, rng: random.Random, box):
    """Window and door tiles on the wall faces turned to +Z (where their picture shows; never
    beside an opening, where their side faces would show), and cobwebs in the top corners of
    the rooms."""
    x0, y0, z0, x1, y1, z1 = box
    for x in range(x0, x1 + 1):
        for z in range(z0, z1 + 1):
            for y in range(y0, y1 + 1):
                if w.get(x, y, z) != BLANK_WINDOW or w.get(x, y, z + 1) != 0:
                    continue
                if w.get(x - 1, y, z) != BLANK_WINDOW or w.get(x + 1, y, z) != BLANK_WINDOW:
                    continue
                if y in (F0 + 1, F1 + 1, F2 + 1) and rng.random() < 0.18:
                    w.set(x, y, z, rng.choice(WINDOWS))
                elif y in (F0, F1, F2) and w.get(x, y - 1, z + 1) != 0 and rng.random() < 0.04:
                    w.set(x, y, z, DOOR)
    for x in range(x0, x1 + 1):
        for z in range(z0, z1 + 1):
            for y in range(y0, y1 + 1):
                if w.get(x, y, z) != 0 or w.get(x, y + 1, z) == 0:
                    continue
                walls = sum(w.get(x + dx, y, z + dz) != 0 for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                if walls >= 2 and rng.random() < 0.55:
                    w.set(x, y, z, rng.choice(COBWEBS))


def main(argv) -> int:
    stage = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    timed = stage in ("timed", "drink")
    drink = stage == "drink"
    rng = random.Random(33)
    w = World(mission=33, sky=15)
    w.fill(28, 0, 26, 72, 24, 70, BLANK_WINDOW)

    def wasp(start, end, ticks, stop=4):
        if not timed:
            patrol(w, O.WASP, start, end, ticks, stop)

    # Ground floor (walk 5): the entrance hall with the arrow, the atrium, the wings.
    carve(w, 44, F0, 58, 56, F0 + 4, 68)                    # the entrance hall
    w.place(O.EXIT, 46, F0, 59)                             # before the house door
    carve(w, 42, F0, 38, 58, F0 + 4, 56)                    # the atrium's floor
    carve(w, 45, F0, 41, 55, 22, 53)                        # ...open up to the roof
    carve(w, 30, F0, 38, 40, F0 + 4, 56)                    # the west wing (the stair)
    carve(w, 60, F0, 38, 70, F0 + 4, 56)                    # the east wing
    carve(w, 42, F0, 28, 58, F0 + 4, 36)                    # the north room
    doorway(w, 49, 57, 51, 57, F0)
    doorway(w, 41, 46, 41, 48, F0)
    doorway(w, 59, 46, 59, 48, F0)
    doorway(w, 49, 37, 51, 37, F0)
    for x in (49, 51):                                      # mines hanging in two doorways
        mine(w, x, F0 + 2, 56, hanging=True)
        mine(w, x, F0 + 2, 36, hanging=True)
    # The atrium: a treasure in the middle with a wasp sweeping over it, one more wasp
    # along the low side under the gallery.
    w.place(O.TREASURE, 50, F0, 47)
    wasp((46, F0, 47), (54, F0, 47), 24)
    wasp((43, F0, 40), (43, F0, 54), 30)
    # North room: a row of floor mines across it, the treasure beyond, an egg.
    for x in (43, 47, 51, 55):
        mine(w, x, F0, 33)
    w.place(O.TREASURE, 57, F0, 29)
    w.place(O.EGG, 43, F0, 29)
    # West wing: the treasure in its south-west corner; the stair up to the first floor in
    # its north part (steps of two, through a hole in the ceiling).
    w.place(O.TREASURE, 31, F0, 55)
    stair(w, ((31, 33), (34, 36), (37, 39)), F0, 1)
    # East wing: the treasure on a pedestal two high, mines hanging a jump above it on
    # its west and east sides: jump up from the north or the south.
    w.fill(65, F0, 47, 65, F0 + 1, 47, BLANK_WINDOW)
    w.place(O.TREASURE, 65, F0 + 2, 47)
    for x in (64, 66):
        mine(w, x, F0 + 3, 47, hanging=True)
    for z in (46, 48):
        mine(w, 61, F0 + 2, z, hanging=True)                # inside the doorway

    # First floor (walk 12): the gallery round the atrium, the wings above.
    carve(w, 42, F1, 38, 58, F1 + 4, 56)
    carve(w, 30, F1, 38, 40, F1 + 4, 56)                    # west wing 1
    carve(w, 60, F1, 38, 70, F1 + 4, 56)                    # east wing 1 (the stair up)
    carve(w, 42, F1, 28, 58, F1 + 4, 36)                    # north room 1
    carve(w, 44, F1, 58, 56, F1 + 4, 68)                    # the room over the hall
    doorway(w, 41, 46, 41, 48, F1)
    doorway(w, 59, 46, 59, 48, F1)
    doorway(w, 49, 37, 51, 37, F1)
    doorway(w, 49, 57, 51, 57, F1)
    for z in (46, 48):
        mine(w, 40, F1 + 2, z, hanging=True)
    for x, z in ((44, 44), (46, 55), (58, 51)):            # floor mines on the gallery
        mine(w, x, F1, z)
    w.place(O.TREASURE, 50, F1, 55)                         # on the gallery's south side
    wasp((57, F1, 39), (57, F1, 55), 30)
    w.place(O.TREASURE, 31, F1, 55)                         # west wing 1, under hanging mines
    for x, z in ((33, 53), (33, 55), (31, 53)):
        mine(w, x, F1 + 1, z, hanging=True)
    w.place(O.TREASURE, 43, F1, 29)                         # north room 1, floor mines, a wasp
    for x, z in ((45, 31), (45, 34), (48, 29)):
        mine(w, x, F1, z)
    wasp((54, F1, 29), (54, F1, 35), 18)
    w.place(O.EGG, 45, F1, 67)
    # The stair in east wing 1 up to the attic.
    stair(w, ((67, 69), (64, 66), (61, 63)), F1, -1)

    # The attic (walk 19): east and west, joined by the bridge across the atrium; mines
    # hang under the roof over the atrium, where a balloon ends.
    carve(w, 60, F2, 38, 70, F2 + 3, 56)
    carve(w, 41, F2, 46, 59, F2 + 2, 48)
    w.fill(45, F2 - 1, 46, 55, F2 - 1, 48, BLANK_WINDOW)    # the bridge
    w.place(O.TREASURE, 50, F2, 47)
    wasp((47, F2, 44), (47, F2, 50), 12)
    wasp((53, F2, 50), (53, F2, 44), 12)
    for x, z in ((46, 43), (50, 43), (54, 43), (46, 51), (50, 51), (54, 51)):
        mine(w, x, 22, z, hanging=True)
    w.place(O.TREASURE, 69, F2, 55)                         # east attic
    wasp((61, F2, 44), (69, F2, 44), 24)
    # West attic: the drink, then a knot of mines filling the passage to the last treasure,
    # floor mines and mines hanging a jump above them: crossed unseen, never jumped.
    carve(w, 36, F2, 40, 40, F2 + 3, 54)
    carve(w, 31, F2, 46, 35, F2 + 3, 48)
    w.place(O.DRINK, 38, F2, 47)
    wasp((37, F2, 42), (37, F2, 52), 18)
    if not drink:
        for x in (33, 35):
            for z in (46, 47, 48):
                mine(w, x, F2, z)
                mine(w, x, F2 + 2, z, hanging=True)
    w.place(O.TREASURE, 31, F2, 47)

    floors(w, rng, (29, F0, 27, 71, 22, 69))
    decorate(w, rng, (29, F0, 27, 71, 22, 69))
    w.set(46, F0, 57, DOOR)
    w.set(46, F0 + 1, 57, BLANK_WINDOW)
    w.spawn = SPAWN
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world033.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
