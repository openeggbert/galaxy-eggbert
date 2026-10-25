#!/usr/bin/env python3
"""Mission 014, the dynamite lesson and world 1's golden key (worlds3d/world014.vwr): brief
plans/briefs/world014.md.

Mobile Eggbert's level 014: six sticks of dynamite beside the start; a wall of crates blocks the
way ("Put down the dynamite, then clear off!"); beyond it three eggs and a plug of crates in the
floor ("Put down another stick of dynamite here to move on"); down the shaft the golden key that
ends world 1. Here: a machine start room, a corridor sealed by a three-by-three crate wall, a
room with the eggs on a ledge and the crate plug over a shaft, the key room below. Blupi carries
one stick at a time, so the second blast means a second trip.

    python3 tools/worlds/world014.py                 # writes worlds3d/world014.vwr
    python3 tools/worlds/world014.py --solved out    # both blasts done (for levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MONITOR, O, World, wall  # noqa: E402

TOP = 10            # the upper floors' top: Blupi walks at 11
WALK = TOP + 1
SPAWN = (48, WALK, 62)


def room(w, x0, z0, x1, z1, rng, height=4):
    wall(w, x0, WALK, z0, x1, WALK + height - 1, z1, rng, face_chance=0.25)
    w.fill(x0 + 1, WALK, z0 + 1, x1 - 1, WALK + height - 1, z1 - 1, 0)


def main(argv) -> int:
    solved = len(argv) > 1 and argv[1] == "--solved"
    rng = random.Random(14)
    w = World(mission=14, sky=18)
    wall(w, 41, TOP - 5, 33, 55, TOP, 65, rng, face_chance=0.15)       # the upper floor, 6 thick

    # The start room and its dynamite, three sticks each side.
    room(w, 41, 56, 55, 65, rng)
    for x in (43, 53):
        for z in (58, 60, 62):
            w.place(O.DYNAMITE, x, WALK, z)
    w.hint_zone(0, 42, WALK, 57, 54, WALK + 2, 64)
    w.hint_zone(1, 43, WALK, 57, 54, WALK + 2, 64)

    # The corridor (x 47-49, roofed) sealed by a wall of nine crates at z 48.
    wall(w, 46, WALK, 46, 50, WALK + 3, 56, rng, face_chance=0.25)
    w.fill(47, WALK, 46, 49, WALK + 2, 56, 0)
    if not solved:
        for x in (47, 48, 49):
            for y in range(WALK, WALK + 3):
                w.place(O.CRATE, x, y, 48)
    w.hint_zone(2, 47, WALK, 50, 49, WALK + 2, 55)
    w.hint_zone(3, 47, WALK, 49, 49, WALK + 2, 55)

    # The room beyond: the eggs on a ledge, the crate plug in the floor over the shaft.
    room(w, 41, 33, 55, 46, rng)
    w.fill(47, WALK, 46, 49, WALK + 2, 46, 0)                         # its doorway
    wall(w, 42, WALK, 34, 44, WALK + 1, 38, rng)                      # the ledge, 2 up
    for z in (35, 36, 37):
        w.place(O.EGG, 43, WALK + 2, z)
    w.fill(47, 1, 38, 49, TOP, 40, 0)                                 # the shaft
    if not solved:
        for x in (47, 48, 49):
            for z in (38, 39, 40):
                w.place(O.CRATE, x, TOP, z)
    w.hint_zone(4, 45, WALK, 36, 51, WALK + 2, 43)
    for x, y, z in ((44, WALK + 2, 46), (52, WALK + 2, 46), (48, WALK + 2, 33)):
        w.set(x, y, z, MONITOR)

    # The key room below (walk 1): the golden key.
    wall(w, 41, 0, 33, 55, 0, 47, rng, face_chance=0.1)
    room_low = (41, 33, 55, 47)
    wall(w, room_low[0], 1, room_low[1], room_low[2], 4, room_low[3], rng, face_chance=0.3)
    w.fill(42, 1, 34, 54, 4, 46, 0)
    w.fill(47, 1, 38, 49, 4, 40, 0)                                   # the shaft comes down here
    w.place(O.SECRET_EXIT, 51, 1, 44)
    for x in range(42, 55, 4):
        w.set(x, 3, 33, rng.choice(MACHINE_FACES))

    w.spawn = SPAWN
    out = Path(argv[2]) if solved else Path(__file__).resolve().parents[2] / "worlds3d" / "world014.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
