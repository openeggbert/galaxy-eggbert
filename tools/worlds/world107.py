#!/usr/bin/env python3
"""Mission 107, the tower and world 10's golden key (worlds3d/world107.vwr): brief
plans/briefs/world107.md.

Mobile Eggbert's level 107, the last of world 10, is one tall shaft of cobble: Blupi starts at its
foot beside the golden key (21) and the one treasure behind the red door (334); the red key (49)
is at the very top. Up the shaft: a lift moving across and up at once, a lollipop (26) for the
high jump past a wasp (44), mines bobbing (16), a row of mines hanging across the way (crouch), a
lift of six cells round a pillar carrying him far up (108 ticks), ledges with mines (2), hanging
mines (3) and eggs, the red key and two eggs at the top; then down again, the red door, the
treasure, the golden key. Here the tower rises north of the foot's chamber in 3D: the chamber,
the lifts and the lollipop's wall, the low passage, the great lift round its column, the stair
of ledges round the shaft, the top room and a chute straight down to the foot.

    python3 tools/worlds/world107.py                 # writes worlds3d/world107.vwr
    python3 tools/worlds/world107.py --stage K out   # for levelcheck: K >= 1 a stair beside the
                                                     # lollipop's wall, no mines or hanging mines;
                                                     # K >= 2 the red door open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, fork_alcove, rock  # noqa: E402
from kit import O, World, lift, mine  # noqa: E402
from village import STONE  # noqa: E402

RED_DOOR = 334
FOOT, LEDGE, SHAFT, HIGH, TOPWALK = 4, 10, 14, 32, 58


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world107.vwr"
    rng = random.Random(107)
    w = World(mission=107, sky=2)
    helped = stage is not None and stage >= 1
    opened = stage is not None and stage >= 2

    def hazard(x, y, z, hanging=False):
        if not helped:
            mine(w, x, y, z, hanging=hanging)

    rock(w, 30, 0, 26, 70, 64, 64)

    # ---- the foot's chamber (walk 4) ----
    # Spawn by the golden key in its west end; the red door in its east wall, the treasure in the
    # niche behind it. In its north-west corner the first ledge (walk 10), a lift moving across and
    # up at once from the floor to it.
    carve(w, 38, FOOT, 42, 62, FOOT + 10, 58)
    w.spawn = (44, FOOT, 52)
    w.place(O.SECRET_EXIT, 40, FOOT, 52)
    carve(w, 64, FOOT, 49, 66, FOOT + 2, 51)
    if not opened:
        w.fill(63, FOOT, 49, 63, FOOT + 1, 51, RED_DOOR)
    else:
        carve(w, 63, FOOT, 49, 63, FOOT + 1, 51)
    w.place(O.TREASURE, 65, FOOT, 50)
    rock(w, 38, FOOT, 42, 52, LEDGE - 1, 44)                              # the first ledge
    for x in (48, 49):
        for z in (47, 48):
            lift(w, (x, FOOT - 1, z), (x, LEDGE - 1, z - 2), ticks=50, stop=30)
    carve(w, 48, FOOT - 1, 47, 49, FOOT - 1, 48)
    fork_alcove(w, 62, FOOT, 56, (1, 0))

    # ---- the ledge, the lollipop and its wall (walk 10 -> 14) ----
    # North of the ledge the tower's floor stands four higher: a jump only the lollipop's power
    # makes (taken at the ledge's west end). A wasp crosses the ledge at Blupi's height.
    carve(w, 38, LEDGE, 30, 62, 61, 41)                                    # the tower
    rock(w, 38, LEDGE, 30, 62, SHAFT - 1, 41)                              # its floor, walk 14
    carve(w, 38, LEDGE, 42, 52, LEDGE + 8, 44)                            # head room for the jump
    w.place(O.LOLLIPOP, 39, LEDGE, 43)
    w.place(O.WASP, 41, LEDGE, 44, end=(51, LEDGE, 44), advance=40, recede=40, stop_start=8, stop_end=8)
    if helped:
        rock(w, 44, LEDGE, 42, 46, LEDGE + 1, 42)                          # levelcheck's stair

    # ---- the tower's floor (walk 14): bobbing mines, the low passage, the great lift ----
    # Four mines bobbing between walk 14 and 16 across the floor; east a passage two high where six
    # mines hang in the walking cells (crouch under them); beyond it the great lift: six cells round
    # a column, carrying him up to walk 32 (108 ticks, dwelling 30).
    if not helped:
        for x, z, t in ((42, 33, 10), (46, 38, 14), (44, 35, 12), (48, 32, 10)):
            w.place(O.MOVING_MINE, x, SHAFT, z, end=(x, SHAFT + 2, z), advance=t, recede=t, stop_start=0,
                    stop_end=0)
    rock(w, 50, SHAFT, 30, 54, SHAFT + 30, 41)                            # a wall up the tower...
    carve(w, 50, SHAFT, 34, 54, SHAFT + 1, 36)                            # ...the low passage through it
    for x in (51, 53):
        for z in (34, 35, 36):
            hazard(x, SHAFT, z, hanging=True)
    rock(w, 58, SHAFT, 34, 58, 60, 36)                                     # the column
    for x in (57, 59):
        for z in (34, 35, 36):
            lift(w, (x, SHAFT - 1, z), (x, HIGH - 1, z), ticks=108, stop=30)
    carve(w, 57, SHAFT - 1, 34, 57, SHAFT - 1, 36)
    carve(w, 59, SHAFT - 1, 34, 59, SHAFT - 1, 36)

    # ---- the stair of ledges (walk 32 -> 56) ----
    # From the great lift's top a landing, then ledges two up each, three wide, winding round the
    # column twice (a gap of one between each and the next); mines on two of them (step round),
    # mines hanging over the gaps between them, eggs.
    rock(w, 55, HIGH - 2, 30, 62, HIGH - 1, 33)                            # the landing, walk 32
    ring = [(60, 34, 62, 36), (60, 38, 62, 41), (55, 38, 58, 41), (55, 34, 56, 36), (55, 30, 58, 32),
            (60, 30, 62, 32)]
    for k in range(12):
        x0, z0, x1, z1 = ring[k % 6]
        top = HIGH + 1 + 2 * k                                            # walk 34 + 2k
        rock(w, x0, top - 1, z0, x1, top, z1)
    for walk, (x, z) in ((36, (61, 40)), (42, (56, 31)), (48, (61, 39)), (54, (57, 31))):
        w.place(O.EGG, x, walk, z)
    hazard(56, 38, 39)
    hazard(61, 44, 31)
    for x, y, z in ((59, 41, 37), (59, 49, 37), (57, 53, 33)):
        hazard(x, y, z, hanging=True)

    # ---- the top room (walk 58) and the chute ----
    # The red key and two eggs; a chute straight down to the foot's chamber.
    carve(w, 54, TOPWALK, 30, 62, TOPWALK + 3, 48)
    w.fill(54, TOPWALK - 1, 33, 62, TOPWALK - 1, 48, STONE)
    w.place(O.KEY1, 58, TOPWALK, 46)
    w.place(O.EGG, 55, TOPWALK, 47)
    w.place(O.EGG, 61, TOPWALK, 47)
    carve(w, 60, FOOT + 11, 44, 61, TOPWALK - 1, 45)

    dress(w, 30, 0, 26, 70, 64, 64, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
