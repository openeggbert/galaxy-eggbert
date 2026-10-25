#!/usr/bin/env python3
"""Mission 031, birds over the rooftops (worlds3d/world031.vwr): brief plans/briefs/world031.md.

Mobile Eggbert's level 031, the first of the village world: along a street of houses whose
roofs Blupi jumps across, birds fly to and fro (they kill at a touch) and mines lie on the
roofs; a lift rises beside a tall house (an egg halfway up) to a cave in a stone hill, where
seven mines guard the first treasure and a treasure door ("1", tile 421) opens once it is
taken; behind it a shaft drops into a vault with the other two treasures and two eggs; out
of the vault the street goes on over three gaps full of mines, each crossed on a sliding lift
-- slow, faster, fast -- under a bird; a last lift climbs beside a tall house among four birds
to the red arrow. Here: a ridge of houses floating in the sky heading north (-Z), the tall
house and its lift, the hill with the cave, the door, the shaft and the vault inside it, and
the arcade of houses heading east over the three mine gaps to the exit tower.

    python3 tools/worlds/world031.py                 # writes worlds3d/world031.vwr
    python3 tools/worlds/world031.py --solved out    # the treasure door open (levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine, rect  # noqa: E402
from village import (GRASS, PLANTS, TREASURE_DOOR1, TREES, bird, grass_top, house, stone)  # noqa: E402

BASE = 9                     # the start yard's top: Blupi walks at 10
SPAWN = (22, BASE + 1, 90)
RX0, RX1 = 20, 24            # the rooftop ridge, five wide
CAVE = 21                    # the cave's floor walk level (the tall house's roof)
VAULT = 13                   # the vault's walk level
ARCADE = 15                  # the arcade roofs' walk level


def ridge_house(w, z0, z1, height, rng, door=None):
    """A house of the ridge on a stone footing down to y 4."""
    stone(w, RX0, 4, z0, RX1, BASE, z1, rng)
    house(w, RX0, z0, RX1, z1, BASE + 1, height, rng, door=door)


def rooftops(w: World, rng: random.Random):
    """A: the start yard and the ridge of houses heading north, birds and three roof mines."""
    stone(w, 17, 4, 85, 27, BASE, 93, rng)
    grass_top(w, 17, 85, 27, 93, BASE)
    w.bigdecor(TREES[0], 18, BASE + 1, 91)
    w.bigdecor(TREES[2], 26, BASE + 1, 87)
    ridge_house(w, 81, 84, 2, rng, door=22)             # walk 12: the first jump
    ridge_house(w, 77, 80, 3, rng)                      # walk 13
    mine(w, 22, BASE + 4, 78)
    # z 76: an alley, a one-cell gap to jump.
    ridge_house(w, 71, 75, 3, rng)                      # walk 13
    mine(w, 22, BASE + 4, 73)
    ridge_house(w, 67, 70, 5, rng)                      # walk 15
    mine(w, 22, BASE + 6, 68)
    ridge_house(w, 63, 66, 4, rng)                      # walk 14, under the tall house
    # Birds as the original's, short beats to and fro: over the yard (high), a head above the
    # landing row of the second house and over the alley (they meet a jump: wait for them),
    # one at Blupi's height along the east edge of the third roof, and one above each of
    # the last two roofs.
    bird(w, (19, BASE + 4, 89), (25, BASE + 4, 89), ticks=16)
    bird(w, (19, BASE + 5, 80), (25, BASE + 5, 80), ticks=24)
    bird(w, (22, BASE + 5, 78), (22, BASE + 5, 73), ticks=16)
    bird(w, (24, BASE + 4, 71), (24, BASE + 4, 75), ticks=24)
    bird(w, (19, BASE + 7, 69), (25, BASE + 7, 69), ticks=24)
    bird(w, (19, BASE + 6, 64), (25, BASE + 6, 64), ticks=16)


def tall_house(w: World, rng: random.Random):
    """B: the tall house; a lift in its facade from the last roof (walk 14) to its own roof
    (walk 21), an egg in an alcove halfway up."""
    stone(w, 18, 4, 53, 26, BASE, 62, rng)
    house(w, 18, 53, 26, 62, BASE + 1, CAVE - BASE - 1, rng)
    for y in range(BASE + 4, CAVE + 1):                 # the shaft, open to the south
        w.clear(22, y, 62)
    lift(w, (22, BASE + 4, 62), (22, CAVE - 1, 62), ticks=70, stop=10)
    w.fill(20, 18, 62, 21, 19, 62, 0)                   # the alcove beside it, walk 18
    w.place(O.EGG, 20, 18, 62)
    w.place(O.EGG, 25, CAVE, 55)                        # and one on the roof
    for x, z in ((19, 60), (25, 59)):                   # chimneys
        stone(w, x, CAVE, z, x, CAVE + 1, z, rng)
    bird(w, (18, CAVE + 1, 57), (26, CAVE + 1, 57), ticks=32)
    bird(w, (23, CAVE + 1, 54), (23, CAVE + 1, 61), ticks=24)


def hill(w: World, rng: random.Random, solved: bool):
    """C and D: the stone hill, its cave (treasure 1 among seven mines), the treasure door,
    the shaft and the vault (treasures 2 and 3, two eggs)."""
    stone(w, 10, 4, 18, 52, 31, 52, rng)
    grass_top(w, 10, 18, 52, 52, 31)
    for icon, x, z in ((TREES[1], 14, 24), (TREES[0], 30, 22), (TREES[2], 46, 26), (TREES[1], 38, 48),
                       (TREES[0], 16, 46)):
        w.bigdecor(icon, x, 32, z)
    # The cave hall behind the mouth, seven high.
    w.fill(14, CAVE, 36, 30, CAVE + 6, 51, 0)
    w.fill(20, CAVE, 52, 24, CAVE + 3, 52, 0)           # the mouth on the tall house's roof
    mine(w, 22, CAVE, 47)
    # Shelf 1 (walk 23) across the hall's back, mines along its edge; shelf 2 (walk 25) in
    # the north-west corner with the treasure, a mine beside it on the side one comes from.
    stone(w, 14, CAVE, 36, 30, CAVE + 1, 43, rng)
    for x in (17, 22, 27):
        mine(w, x, CAVE + 2, 43)
    stone(w, 14, CAVE + 2, 36, 21, CAVE + 3, 39, rng)
    w.place(O.TREASURE, 14, CAVE + 4, 36)
    mine(w, 16, CAVE + 4, 37)
    w.bigdecor(PLANTS[0], 29, CAVE, 50)
    w.bigdecor(PLANTS[2], 28, CAVE + 2, 37)
    # The passage east from shelf 1 to the treasure door, two mines to weave between.
    w.fill(31, CAVE + 2, 39, 37, CAVE + 5, 43, 0)
    mine(w, 33, CAVE + 2, 39)
    mine(w, 36, CAVE + 2, 43)
    if not solved:
        w.set(38, CAVE + 2, 41, TREASURE_DOOR1)
        w.set(38, CAVE + 3, 41, TREASURE_DOOR1)
    else:
        w.fill(38, CAVE + 2, 41, 38, CAVE + 3, 41, 0)
    # The shaft behind the door, down into the vault.
    w.fill(39, VAULT, 38, 43, CAVE + 5, 44, 0)
    w.fill(39, VAULT, 28, 50, VAULT + 4, 44, 0)
    for x in (44, 48):
        w.place(O.TREASURE, x, VAULT, 31)
    for x in (42, 50):
        w.place(O.EGG, x, VAULT, 30)
    for icon, x, z in ((PLANTS[0], 40, 29), (PLANTS[1], 46, 29), (PLANTS[2], 49, 34)):
        w.bigdecor(icon, x, VAULT, z)
    w.fill(51, VAULT, 34, 52, VAULT + 3, 38, 0)         # the way out, east


def arcade(w: World, rng: random.Random):
    """E and F: the arcade of houses heading east over three gaps of mines, each crossed on
    a sliding lift (three side by side) -- slow, faster, fast -- under a bird; the exit
    tower with its lift among four birds."""
    stone(w, 53, 4, 34, 55, VAULT - 1, 38, rng)          # the landing outside the vault
    houses = ((56, 60), (65, 69), (74, 78), (83, 87))
    for x0, x1 in houses:
        stone(w, x0, 4, 34, x1, VAULT - 2, 38, rng)
        house(w, x0, 34, x1, 38, VAULT - 1, ARCADE - VAULT + 1, rng)
    w.place(O.EGG, 58, ARCADE, 36)
    gaps = ((61, 60, 20), (70, 30, 10), (79, 18, 3))   # first x, stepAdvance, timeStop
    for x0, ticks, stop in gaps:
        for x in range(x0, x0 + 4):                     # a beam of mines down in the gap
            stone(w, x, 4, 36, x, ARCADE - 4, 36, rng)
            mine(w, x, ARCADE - 3, 36)
        for z in (35, 36, 37):
            lift(w, (x0, ARCADE - 1, z), (x0 + 3, ARCADE - 1, z), ticks=ticks, stop=stop)
        bird(w, (x0, ARCADE + 2, 36), (x0 + 3, ARCADE + 2, 36), ticks=24)
    # F: the yard, the last lift, the tower and the red arrow on its roof.
    stone(w, 88, 4, 33, 90, ARCADE - 1, 39, rng)
    mine(w, 90, ARCADE, 34)
    mine(w, 90, ARCADE, 38)
    top = ARCADE + 5
    stone(w, 91, 4, 36, 91, ARCADE - 2, 36, rng)           # the lift's footing
    lift(w, (91, ARCADE - 1, 36), (91, top - 1, 36), ticks=50, stop=10)
    stone(w, 92, 4, 32, 97, BASE, 40, rng)
    house(w, 92, 32, 97, 40, BASE + 1, top - BASE - 1, rng)
    w.place(O.EXIT, 96, top, 36)
    bird(w, (94, top, 33), (94, top, 39), ticks=24)       # across the roof, at Blupi's height
    bird(w, (88, top + 1, 36), (93, top + 1, 36), ticks=24)
    bird(w, (89, top - 2, 33), (89, top - 2, 39), ticks=32)


def main(argv) -> int:
    solved = len(argv) > 1 and argv[1] == "--solved"
    rng = random.Random(31)
    w = World(mission=31, sky=16)
    rooftops(w, rng)
    tall_house(w, rng)
    hill(w, rng, solved)
    arcade(w, rng)
    w.spawn = SPAWN
    out = Path(argv[2]) if solved else Path(__file__).resolve().parents[2] / "worlds3d" / "world031.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
