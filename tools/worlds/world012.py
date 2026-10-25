#!/usr/bin/env python3
"""Mission 012, the crate lesson (worlds3d/world012.vwr): brief plans/briefs/world012.md.

Mobile Eggbert's level 012 teaches crates, room by room, each ending at a ledge too high to jump
to: push a crate to the red dot under it, pull one back out of a corner to the red dot, stack two
crates on the red dot, then three, and climb them. A crate never falls in the original
(TestPushOneCaisse wants support under it), so a stack is built by pushing a crate off a shelf
onto the crate level with it. Here the rooms are five machine terraces climbing away from the
start (-Z), each higher than the last by the lesson's ledge (3, 3, 4, 5 blocks), and the crates
of a stack come to the red dot from different sides: along the floor, off a one-block shelf, off
a two-block shelf.

    python3 tools/worlds/world012.py             # writes worlds3d/world012.vwr
    python3 tools/worlds/world012.py --solved out.vwr
                                                 # crates where the lessons put them (levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, MONITOR, O, World, props, rect, wall  # noqa: E402

RED_DOT = 3                 # the red ball tile marking where a crate goes
X0, X1 = 40, 58
# Terraces: (z0, z1, top); Blupi walks at top + 1.
T0 = (62, 70, 0)
T1 = (50, 61, 3)
T2 = (38, 49, 6)
T3 = (26, 37, 10)
T4 = (14, 25, 15)
SPAWN = (48, 1, 68)


def terrace(w, z0, z1, top, rng):
    wall(w, X0, 0, z0, X1, top, z1, rng, face_chance=0.25)
    for x, z in rect(X0, z0, X1, z1):
        w.set(x, top, z, MACHINE_FLOOR)


def main(argv) -> int:
    solved = len(argv) > 1 and argv[1] == "--solved"
    rng = random.Random(12)
    w = World(mission=12, sky=0)
    for z0, z1, top in (T0, T1, T2, T3, T4):
        terrace(w, z0, z1, top, rng)
    wall(w, X0 - 1, 0, T4[0], X0 - 1, 18, T0[1], rng, face_chance=0.3)   # side walls
    wall(w, X1 + 1, 0, T4[0], X1 + 1, 18, T0[1], rng, face_chance=0.3)
    wall(w, X0 - 1, 0, T0[1] + 1, X1 + 1, 2, T0[1] + 1, rng)              # behind the start

    def crate(x, y, z, final):
        w.place(O.CRATE, *(final if solved else (x, y, z)))

    # 1. Push the crate forward (-Z) to the red dot under the 3-high ledge of T1.
    w.set(48, 1, T1[1], RED_DOT)
    crate(48, 1, 66, (48, 1, 62))
    w.hint_zone(0, X0, 1, T0[0], X1, 3, T0[1])
    w.hint_zone(1, X0, 4, 57, X1, 6, T1[1])
    # 2. Pull the crate (-X) out of the dead-end alcove to the red dot under T2. The alcove's
    #    walls stand three above the crate's top, so it is a step only at the red dot.
    y = T1[2] + 1
    wall(w, 53, y, 51, 55, y + 3, 51, rng)          # the alcove's side wall
    wall(w, 55, y, 50, 55, y + 3, 50, rng)          # its far end
    wall(w, 53, y + 3, T2[1], 56, y + 4, T2[1], rng)  # a parapet on T2 along the alcove
    w.set(52, y, T2[1], RED_DOT)
    crate(54, y, 50, (52, y, 50))
    w.hint_zone(2, 49, y, 50, 56, y + 2, 54)
    # 3. Two crates: one along the floor (-Z) to the red dot under T3, the other pushed (-X)
    #    off a one-block shelf onto it.
    y = T2[2] + 1
    w.fill(49, y, 38, 51, y, 38, MACHINE_FLOOR)     # the shelf
    # A roof two above the shelved crate's top: on it Blupi has no room for a full jump, so it
    # is no stair to T3 before it stands on the stack (open above).
    w.fill(49, y + 4, 38, 51, y + 4, 38, MACHINE_FLOOR)
    w.set(48, y, T3[1], RED_DOT)
    crate(48, y, 40, (48, y, 38))
    crate(49, y + 1, 38, (48, y + 1, 38))
    w.hint_zone(3, X0, y, 38, X1, y + 2, 45)
    # 4. Three crates: along the floor (-Z), off a one-block shelf (+X) and off a two-block shelf
    #    (-X) onto the red dot under T4.
    y = T3[2] + 1
    w.fill(49, y, 26, 51, y, 26, MACHINE_FLOOR)     # one block
    w.fill(53, y, 26, 55, y + 1, 26, MACHINE_FLOOR)  # two blocks
    w.fill(53, y + 5, 26, 55, y + 5, 26, MACHINE_FLOOR)  # its roof, as in 3
    w.set(52, y, T4[1], RED_DOT)
    crate(52, y, 28, (52, y, 26))
    crate(51, y + 1, 26, (52, y + 1, 26))
    crate(53, y + 2, 26, (52, y + 2, 26))
    w.hint_zone(4, X0, y, 26, X1, y + 3, 33)
    # The top: the red arrow.
    top = T4[2] + 1
    w.place(O.EXIT, 49, top, 18)
    for x, y_, z in ((44, 2, 61), (52, 5, 49), (44, 8, 37), (56, 12, 25)):
        w.set(x, y_, z, MONITOR)
    props(w, rect(X0, 15, X1, 20), top, rng, chance=0.08)

    w.spawn = SPAWN
    out = Path(argv[2]) if solved else Path(__file__).resolve().parents[2] / "worlds3d" / "world012.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
