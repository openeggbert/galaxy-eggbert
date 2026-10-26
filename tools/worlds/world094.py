#!/usr/bin/env python3
"""Mission 094, the red door and the teleporter of crates (worlds3d/world094.vwr): brief
plans/briefs/world094.md.

Mobile Eggbert's level 094 winds up through caves of clods with plants to a ridge where yellow
forks and trestles (77, 199) stand by pools of lava (68), crosses crumbling bridges (364) under
two hanging mines (3), and comes to the red door (334) between a trestle and a fork; its key
(ObjectType49) lies below it, reached and brought back. Behind the door a teleporter (330)
hangs too high to stand under: two crates (12), one pushed along the floor, one pushed off a
ledge onto it, make the step. The teleporter leads down to caverns with the four treasures (5),
mines (2), a bulldozer (4) and the red arrow (7). Here the same order in 3D: the start cave in
the south-west, the forks terrace and the bridges along the west, the door ledge in the middle
over the pools cave of the key, the teleporter room east of the door, the treasure caverns in
the north.

    python3 tools/worlds/world094.py                   # writes worlds3d/world094.vwr
    python3 tools/worlds/world094.py --stage K out.vwr # lessons 1..K solved, for levelcheck (no
        keys, crates or timing): 1 the red door open, 2 the crate stack built under the
        teleporter, 3 the bulldozer timed (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from clods import (BRIDGE, LAVA, PLANTS, RED_DOOR, TELEPORTER, TRESTLE, Y_FORK, carve,  # noqa: E402
                   clay, column, fringe, report_audit, rough)
from kit import O, World, mine  # noqa: E402

TOP = 40
START = 14                    # the start cave: walk 14
RIDGE = 20                    # the terrace, the bridges, the door ledge, the teleporter room: walk 20
POOLS = 10                    # the pools cave of the key: walk 10
DEEP = 10                     # the treasure caverns: walk 10 (and higher shelves)
SPAWN = (13, START, 84)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world094.vwr"
    rng = random.Random(94)
    w = World(mission=94, sky=27)

    def mass(x0, y0, z0, x1, y1, z1):
        clay(w, x0, y0, z0, x1, y1, z1, rng)

    def cut(x0, y0, z0, x1, y1, z1):
        carve(w, x0, y0, z0, x1, y1, z1)

    def lava(x0, z0, x1, z1, y):
        """Lava at y over a layer of clods (levelcheck lets a fall through lava)."""
        w.fill(x0, y, z0, x1, y, z1, LAVA)
        mass(x0, y - 1, z0, x1, y - 1, z1)

    def prop_post(x, z, y0, prop):
        """A post of clods three high with a prop on it: out of a jump's reach (D41)."""
        mass(x, y0, z, x, y0 + 2, z)
        w.set(x, y0 + 3, z, prop)

    mass(4, 0, 4, 95, TOP, 95)

    # ---- A, the start cave (x 8-20, z 76-88, walk 14) and the winding way up north -----------
    cut(8, START, 76, 20, START + 6, 88)
    rough(w, 8, 76, 20, 88, START, START + 7, rng)
    w.spawn = SPAWN
    for x, z, p in ((9, 87, 0), (19, 86, 1), (18, 77, 2), (10, 79, 1)):
        w.bigdecor(PLANTS[p], x, START, z)
    # The way up (x 13-15): three steps two high, each three deep, north to the terrace (walk 20).
    for k, z0 in enumerate((73, 70, 67)):
        walk = START + 2 * (k + 1)
        cut(13, START, z0, 15, walk + 4, z0 + 2)
        mass(13, START, z0, 15, walk - 1, z0 + 2)
    cut(13, RIDGE, 63, 15, RIDGE + 4, 66)

    # ---- F, the forks terrace (x 10-40, z 46-62, walk 20; roof at y 30) ------------------------
    cut(10, RIDGE - 1, 46, 40, 29, 62)
    rough(w, 10, 46, 40, 62, RIDGE + 2, 30, rng, sag=0)
    lava(10, 46, 40, 62, RIDGE - 1)                     # the hall's floor is lava ...
    mass(10, RIDGE - 2, 53, 40, RIDGE - 1, 55)          # ... but for a causeway three wide (z 53-55)
    mass(13, RIDGE - 2, 56, 15, RIDGE - 1, 62)          #     and the way in from the south
    for x in (22, 31):                                  # two gaps in it of lava: jump them
        lava(x, 53, x, 55, RIDGE - 1)
    # Shelves beyond three cells of lava on both sides (out of a jump's reach) with the forks and
    #    the trestles on them, plants among them.
    mass(10, RIDGE - 2, 46, 40, RIDGE - 1, 49)
    mass(19, RIDGE - 2, 59, 40, RIDGE - 1, 62)
    for x, prop in ((12, TRESTLE), (16, Y_FORK), (20, TRESTLE), (26, Y_FORK), (33, TRESTLE), (38, Y_FORK)):
        w.set(x, RIDGE, 47, prop)
    for x, prop in ((20, Y_FORK), (27, TRESTLE), (34, Y_FORK)):
        w.set(x, RIDGE, 61, prop)
    for x, z, p in ((14, 48, 0), (23, 48, 2), (30, 47, 1), (24, 60, 0), (38, 61, 2)):
        w.bigdecor(PLANTS[p], x, RIDGE, z)
    w.place(O.EGG, 36, RIDGE, 54)

    # ---- B, the bridges over a lava chasm (x 41-58, z 46-58) -------------------------------
    cut(41, 14, 46, 58, 29, 58)
    lava(41, 46, 58, 58, 14)
    # Two bridges two wide (z 53-54): from the causeway to a rest pillar (x 48-49), then on to the
    #    door ledge (x 50-58) under two mines hanging a cell over it (walk under them, never
    #    jump). The bridges hold Blupi while he walks on and give way behind him.
    w.fill(41, RIDGE - 1, 53, 47, RIDGE - 1, 54, BRIDGE)
    mass(48, 15, 52, 49, RIDGE - 1, 55)
    w.fill(50, RIDGE - 1, 53, 58, RIDGE - 1, 54, BRIDGE)
    w.place(O.EGG, 48, RIDGE, 55)
    if stage < 3:
        w.place(O.HANGING_MINE, 52, RIDGE + 1, 53)
        w.place(O.HANGING_MINE, 55, RIDGE + 1, 54)

    # ---- D, the door ledge (x 59-68, z 44-56, walk 20, open above) ------------------------------
    cut(59, RIDGE, 44, 68, 32, 56)
    rough(w, 59, 44, 68, 56, RIDGE + 3, 33, rng, sag=0)
    # The red door in its east wall (x 69, z 50, two high), a trestle and a fork on posts beside
    #    it; the key opens it.
    cut(69, RIDGE, 50, 69, RIDGE + 1, 50)
    if stage < 1:
        w.set(69, RIDGE, 50, RED_DOOR)
        w.set(69, RIDGE + 1, 50, RED_DOOR)
    prop_post(68, 48, RIDGE, TRESTLE)
    prop_post(68, 52, RIDGE, Y_FORK)
    w.bigdecor(PLANTS[1], 60, RIDGE, 45)

    # ---- K, the pools cave of the key (x 56-74, z 57-72, walk 10), under the ledge's south edge -
    # The ledge ends at z 56 in a drop of ten into it; the key on an island in a ring of lava, an
    #    egg, pools to jump; the way back up a stair of two-high steps along its south side, west
    #    then north to the ledge.
    cut(56, POOLS, 57, 74, 32, 72)
    rough(w, 56, 57, 74, 72, POOLS + 3, 33, rng, sag=0)
    for x0, z0, x1, z1 in ((60, 60, 60, 66), (66, 58, 66, 63), (70, 61, 73, 61)):
        lava(x0, z0, x1, z1, POOLS - 1)
    lava(63, 66, 67, 70, POOLS - 1)                     # the ring of lava ...
    mass(64, POOLS - 2, 67, 66, POOLS - 1, 69)          # ... round the key's island
    w.place(O.KEY1, 65, POOLS, 68)
    w.place(O.EGG, 72, POOLS, 59)
    # The stair: steps two wide (z 71-72), each two higher and two further west (x 72 ... 63),
    #    walk 12 ... 20, then a shelf on at walk 20 (x 56-62, z 71-72), north along x 56-57 and
    #    onto the ledge's south-west corner.
    for k in range(5):
        x1 = 72 - 2 * k
        mass(x1 - 1, POOLS, 71, x1, POOLS + 1 + 2 * k, 72)
    mass(56, POOLS, 71, 62, RIDGE - 1, 72)
    mass(56, POOLS, 57, 57, RIDGE - 1, 72)
    mass(58, POOLS, 57, 59, RIDGE - 1, 58)              # its landing under the ledge's corner
    for x, z, p in ((58, 62, 0), (69, 66, 2), (73, 68, 1)):
        w.bigdecor(PLANTS[p], x, POOLS, z)

    # ---- T, the teleporter room behind the door (x 70-86, z 44-58, walk 20, roof y 30) ---------
    cut(70, RIDGE, 44, 86, 29, 58)
    rough(w, 70, 44, 86, 58, RIDGE + 4, 30, rng, sag=0)
    mass(82, RIDGE, 44, 86, RIDGE, 58)                  # a ledge one high along its east side
    # The teleporter hangs three over the floor cell beside the ledge (81, 51): out of reach to
    #    stand under. A crate on the floor (pushed east to it) and one on the ledge (pushed off
    #    onto the first) make a stack whose top is level with a lump of clods beside it (81, 52);
    #    Blupi climbs the lump (a jump up from the ledge) and walks onto the stack, under the
    #    teleporter (a jump into the cell under a pillar meets the pillar), and waits.
    w.set(81, RIDGE + 3, 51, TELEPORTER)
    mass(81, RIDGE + 4, 51, 81, 29, 51)                 # its mount up to the roof
    mass(81, RIDGE, 52, 81, RIDGE + 1, 52)              # the lump (walk 22)
    if stage < 2:
        w.place(O.CRATE, 76, RIDGE, 51)
        w.place(O.CRATE, 82, RIDGE + 1, 51)
    else:
        w.place(O.CRATE, 81, RIDGE, 51)
        w.place(O.CRATE, 81, RIDGE + 1, 51)
    mine(w, 74, RIDGE, 46)
    w.bigdecor(PLANTS[0], 85, RIDGE + 1, 57)

    # ---- C, the treasure caverns (x 30-90, z 8-40, walk 10; roof y 22) --------------------------
    cut(30, DEEP, 8, 90, 21, 40)
    rough(w, 30, 8, 90, 40, DEEP + 4, 22, rng, bulge=0.4, sag=0)
    # The teleporter's partner hangs one over the floor by the caverns' middle: Blupi arrives one
    #    cell +Z of the cell under it (60, 10, 21), facing on.
    w.set(60, DEEP + 1, 20, TELEPORTER)
    mass(60, DEEP + 2, 20, 60, 21, 20)
    # Banks of clods dividing the caverns into rooms, a passage between each.
    mass(44, DEEP, 8, 46, 21, 30)                        # west bank, the way through at z 31-40
    mass(74, DEEP, 18, 76, 21, 40)                       # east bank, the way through at z 8-17
    # T1, the west room: a shelf of steps (walk 12, 14, 16) up to the treasure.
    for k, x0 in enumerate((40, 37, 34)):
        mass(x0, DEEP, 10, x0 + 2, DEEP + 1 + 2 * k, 16)
    w.place(O.TREASURE, 35, DEEP + 6, 12)
    w.place(O.EGG, 32, DEEP, 38)
    # T2, the middle room: two rows of three mines across the way (jump each), the treasure beyond.
    for z in (28, 33):
        for x in (52, 53, 54):
            mine(w, x, DEEP, z)
    w.place(O.TREASURE, 53, DEEP, 38)
    # T3, the east room: the bulldozer drives up and down a trench one wide and three deep (x 84,
    #    z 18-38, open above for the camera, out of a jump's reach), the treasure at its end;
    #    alcoves to step into while it passes.
    mass(77, DEEP, 18, 90, DEEP + 2, 40)
    cut(84, DEEP, 18, 84, DEEP + 2, 38)                 # its mouth open to the room's south part
    for z in (26, 32):
        cut(85, DEEP, z, 85, DEEP + 2, z)
    if stage < 3:
        w.place(O.BULLDOZER, 84, DEEP, 22, end=(84, DEEP, 36), advance=120, recede=120, stop_start=20,
                stop_end=20)
    w.place(O.TREASURE, 84, DEEP, 38)
    # T4, high on a pillar in the middle room, reached by jumps from block to block.
    mass(66, DEEP, 31, 68, DEEP + 7, 34)
    for x0, top in ((63, DEEP + 1), (65, DEEP + 3), (67, DEEP + 5)):
        mass(x0, DEEP, 35, x0 + 1, top, 36)
    w.place(O.TREASURE, 67, DEEP + 8, 32)
    mine(w, 70, DEEP, 26)
    w.place(O.EGG, 87, DEEP, 10)
    # The red arrow in an alcove of the caverns' north side.
    cut(58, DEEP, 5, 62, DEEP + 3, 7)
    w.place(O.EXIT, 60, DEEP, 6)
    for x, z, p in ((33, 20, 0), (50, 12, 1), (57, 36, 2), (72, 12, 0), (80, 14, 1), (64, 16, 2), (40, 34, 1)):
        w.bigdecor(PLANTS[p], x, DEEP, z)

    fringe(w, rng, clear=7)
    w.save(out)
    print(f"wrote {out}")
    return 0 if stage or report_audit(out) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
