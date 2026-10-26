#!/usr/bin/env python3
"""Mission 121, the brick house (worlds3d/world121.vwr): brief plans/briefs/world121.md.

Mobile Eggbert's level 121, the first of world 12, is short: Blupi starts beside the exit in a
little room of brick under two hanging mines (3); round it rooms with four treasures, three crates
(12) to push, mines (2), four eggs; far off a cave of brown rock with the invert (40) and mines.
Here the house is a block of brick in 3D: the start room in its middle, the west room with a
treasure on a ledge (push the crate), the east corridor of mines and its ledge, the cellar below,
and a tunnel out to the rock cave.

    python3 tools/worlds/world121.py                 # writes worlds3d/world121.vwr
    python3 tools/worlds/world121.py --solved out    # the crates pushed to the ledges' feet, no
                                                     # mines (for levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from station import BRICK, brick  # noqa: E402
from village import ROCK, ROCK_FACES  # noqa: E402

WALK = 10


def main(argv) -> int:
    solved = len(argv) > 2 and argv[1] == "--solved"
    out = Path(argv[2]) if solved else Path(__file__).resolve().parents[2] / "worlds3d" / "world121.vwr"
    rng = random.Random(121)
    w = World(mission=121, sky=24)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not solved:
            mine(w, x, y, z, hanging=hanging)

    def crate(at, pushed):
        w.place(O.CRATE, *(pushed if solved else at))

    brick(w, 20, 2, 32, 80, 20, 64, rng)

    # ---- the start room (walk 10) ----
    # The exit beside the spawn (it opens with the four treasures); two mines hang a cell over the
    # doorway north (walk under, never jump). Doors west, east and north.
    carve(44, WALK, 46, 56, WALK + 4, 56)
    w.spawn = (50, WALK, 52)
    w.place(O.EXIT, 52, WALK, 54)
    carve(49, WALK, 40, 51, WALK + 2, 45)
    hazard(49, WALK + 1, 44, hanging=True)
    hazard(51, WALK + 1, 44, hanging=True)

    # ---- west: the room with the ledge (walk 10) ----
    # Treasure 1 on its floor, treasure 2 on a ledge three up: push the crate to its foot.
    carve(40, WALK, 49, 43, WALK + 2, 51)
    carve(26, WALK, 42, 39, WALK + 5, 58)
    brick(w, 26, WALK, 42, 31, WALK + 2, 46, rng)
    w.place(O.TREASURE, 28, WALK + 3, 44)
    w.place(O.TREASURE, 34, WALK, 56)
    crate((35, WALK, 50), (32, WALK, 45))
    w.place(O.EGG, 37, WALK, 43)

    # ---- east: the corridor of mines and its ledge (walk 10) ----
    # Two mines in the corridor (step round), treasure 3 on a ledge three up in the room beyond:
    # push its crate.
    carve(57, WALK, 49, 64, WALK + 2, 51)
    hazard(60, WALK, 49)
    hazard(62, WALK, 51)
    carve(65, WALK, 42, 76, WALK + 5, 58)
    brick(w, 72, WALK, 52, 76, WALK + 2, 58, rng)
    w.place(O.TREASURE, 74, WALK + 3, 56)
    crate((68, WALK, 48), (71, WALK, 55))
    w.place(O.EGG, 66, WALK, 57)

    # ---- north: the cellar (walk 5) ----
    # Through the doorway under the hanging mines, a drop into the cellar: treasure 4, two eggs, a
    # crate, a stair back up (two a step).
    carve(44, 5, 37, 56, WALK - 1, 39)
    carve(49, WALK - 4, 40, 51, WALK + 2, 40)                           # the drop from the doorway
    w.fill(44, 4, 37, 56, 4, 39, BRICK)
    w.place(O.TREASURE, 46, 5, 38)
    w.place(O.EGG, 54, 5, 38)
    w.place(O.EGG, 48, 5, 38)
    w.place(O.CRATE, 50, 5, 38)
    for k, x in enumerate((56, 55)):
        w.fill(x, 5, 40, x, 6 + 2 * k, 40, BRICK)
    carve(55, 7, 41, 56, WALK + 1, 45)
    w.fill(55, 9, 41, 56, 9, 45, BRICK)

    # ---- the rock cave (walk 10), down a long tunnel west ----
    # The invert (it turns the controls round for a while) and three mines.
    carve(8, WALK, 50, 25, WALK + 2, 51)
    for x in range(2, 19):
        for y in range(4, 20):
            for z in range(40, 62):
                if not (8 <= x and 50 <= z <= 51 and WALK <= y <= WALK + 2):
                    w.set(x, y, z, ROCK_FACES[0] if rng.random() < 0.2 else ROCK)
    carve(4, WALK, 44, 16, WALK + 3, 58)
    w.place(O.INVERT, 6, WALK, 46)
    w.place(O.EGG, 14, WALK, 56)
    for x, z in ((8, 50), (11, 46), (12, 54)):
        hazard(x, WALK, z)
    carve(16, WALK, 50, 25, WALK + 2, 51)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
