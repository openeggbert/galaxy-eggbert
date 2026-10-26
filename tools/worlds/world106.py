#!/usr/bin/env python3
"""Mission 106, the treasure wells (worlds3d/world106.vwr): brief plans/briefs/world106.md.

Mobile Eggbert's level 106 has sixteen treasures, twelve of them stacked four by four in three
narrow wells over Blupi's start (the exit beside him): each well is topped by a teleporter, so he
walks out through the caves round the start to another teleporter, comes out over a well and drops
through its four treasures back to the start. The caves: a stair down with eggs and hanging mines
(3), a hall of bulldozers (4) with lifts over lava, a cave of enemy Blupis (32 fires down, 33
fires across) with mines (2), a block in the middle with two treasures between mines reached by
lifts, a high room with two more and two bulldozers, a passage walled with the flight barrier
(214) round three eggs. Here the start is an atrium of cobble with the three wells in its roof,
its four doors leading to the caves, each ending at a teleporter whose partner stands over a well.

    python3 tools/worlds/world106.py                 # writes worlds3d/world106.vwr
    python3 tools/worlds/world106.py --stage 1 out   # for levelcheck: no mines, hanging mines,
                                                     # bulldozers or enemy Blupis
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, fork_alcove, rock  # noqa: E402
from kit import O, World, lift, mine  # noqa: E402
from village import STONE  # noqa: E402

LAVA = 68
BARRIER = 214               # the flight barrier: a plain wall on foot
TP_WEST, TP_EAST, TP_SOUTH, TP_HIGH = 330, 331, 332, 333
FLOOR, WALK = 9, 10         # the atrium; the wells' rooms walk 30
TOP = 30


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world106.vwr"
    rng = random.Random(106)
    w = World(mission=106, sky=3)
    calm = stage is not None and stage >= 1

    def teleporter(icon, x, walk, z):
        """Its pillar over (x, walk, z); arriving from its partner Blupi comes out at z + 1."""
        w.set(x, walk + 1, z, icon)

    def hazard(kind, x, y, z, **patrol):
        if calm:
            return
        if kind == "mine":
            mine(w, x, y, z)
        elif kind == "hanging":
            mine(w, x, y, z, hanging=True)
        else:
            w.place(kind, x, y, z, **patrol)

    def patrol(start, end, ticks, stop=20):
        return dict(end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    rock(w, 2, 0, 2, 97, 44, 97)

    # ---- the atrium (walk 10) and its three wells ----
    # Spawn by the exit in the middle; a door in each wall. In the roof three wells (2 x 2), each up
    # to a small room (walk 30) where a teleporter's pillar stands: arriving there Blupi comes out
    # over the well's mouth and drops through its four treasures back into the atrium.
    carve(w, 40, WALK, 40, 60, WALK + 5, 60)
    w.spawn = (50, WALK, 49)
    w.place(O.EXIT, 50, WALK, 47)
    wells = ((TP_WEST, 44, 44), (TP_EAST, 55, 44), (TP_SOUTH, 50, 54))
    for icon, wx, wz in wells:
        carve(w, wx - 2, TOP, wz - 3, wx + 3, TOP + 3, wz + 2)
        carve(w, wx, WALK + 6, wz, wx + 1, TOP - 1, wz + 1)              # the well, its mouth at TOP-1
        teleporter(icon, wx, TOP, wz - 1)
        for y in (TOP - 4, TOP - 8, TOP - 12, TOP - 16):
            w.place(O.TREASURE, wx, y, wz)
    for x, z, into in ((40, 44, (-1, 0)), (60, 56, (1, 0))):
        fork_alcove(w, x, WALK, z, into)

    # ---- west: the stair down (walk 10 -> 4) ----
    # Through the west door a cave stepping down a cell at a time, eggs on the steps, two mines
    # hanging in its walking cells (step round or crouch under them); at its foot the teleporter
    # to the west well.
    for k, x in enumerate(range(39, 13, -2)):
        walk = max(WALK - k // 2, 4)
        carve(w, x - 1, walk, 47, x, walk + 3, 53)
    for x, walk in ((33, 9), (25, 7)):
        w.place(O.EGG, x, walk, 50)
    hazard("hanging", 29, 8, 50)
    hazard("hanging", 21, 6, 50)
    carve(w, 6, 4, 46, 13, 7, 54)
    teleporter(TP_WEST, 8, 4, 49)

    # ---- east: the bulldozers' hall (walk 10) ----
    # Two bulldozers pushing to and fro across it; a trench of lava crossed on two lifts meeting
    # midway; a passage walled with the flight barrier round three eggs; at the end the
    # teleporter to the east well.
    carve(w, 61, WALK, 49, 62, WALK + 2, 51)
    carve(w, 63, WALK, 40, 92, WALK + 4, 60)
    hazard(O.BULLDOZER, 66, WALK, 44, **patrol(None, (74, WALK, 44), 80))
    hazard(O.BULLDOZER, 74, WALK, 56, **patrol(None, (66, WALK, 56), 80))
    carve(w, 77, WALK - 4, 40, 83, WALK - 1, 60)
    w.fill(77, WALK - 4, 40, 83, WALK - 3, 60, LAVA)
    for x in (77, 78):
        for z in (49, 50):
            lift(w, (x, WALK - 1, z), (x + 2, WALK - 1, z), ticks=30, stop=30)   # ends at 79-80
    for x in (82, 83):
        for z in (49, 50):
            lift(w, (x, WALK - 1, z), (x - 1, WALK - 1, z), ticks=30, stop=30)   # ends at 81-82
    w.fill(85, WALK, 41, 92, WALK + 1, 43, BARRIER)
    w.fill(86, WALK, 42, 91, WALK + 1, 42, 0)
    w.fill(85, WALK, 42, 85, WALK + 1, 42, 0)
    for x in (88, 89, 90):
        w.place(O.EGG, x, WALK, 42)
    teleporter(TP_EAST, 90, WALK, 52)

    # ---- south: the enemy Blupis' cave (walk 10) ----
    # A cave where one enemy Blupi patrols high on a ledge firing down and another walks firing
    # across, two mines, a trench of lava crossed on a short lift; at its end the teleporter to
    # the south well.
    carve(w, 49, WALK, 61, 51, WALK + 2, 62)
    carve(w, 40, WALK, 63, 62, WALK + 5, 90)
    w.fill(40, WALK, 70, 62, WALK + 2, 71, STONE)                         # a ledge across it
    carve(w, 49, WALK, 70, 51, WALK + 2, 71)                              # ...with a gap below
    hazard(O.ROBOT, 42, WALK + 3, 70, **patrol(None, (60, WALK + 3, 70), 90))
    hazard(O.CREATURE, 42, WALK, 78, **patrol(None, (60, WALK, 78), 90))
    hazard("mine", 46, WALK, 74)
    hazard("mine", 55, WALK, 82)
    carve(w, 40, WALK - 4, 84, 62, WALK - 1, 86)
    w.fill(40, WALK - 4, 84, 62, WALK - 3, 86, LAVA)
    for x in (50, 51):
        lift(w, (x, WALK - 1, 84), (x, WALK - 1, 86), ticks=20, stop=20)
    teleporter(TP_SOUTH, 50, WALK, 88)

    # ---- north: the block of two treasures and the high room ----
    # Through the north door a hall with a block four high in its middle: two lifts up its sides
    # to its top, two treasures there between two mines, the teleporter to the high room. The
    # high room (walk 36): two treasures, two bulldozers, a hole in its floor down to the hall.
    carve(w, 49, WALK, 38, 51, WALK + 2, 39)
    carve(w, 38, WALK, 10, 62, WALK + 7, 37)
    rock(w, 46, WALK, 20, 54, WALK + 3, 28)
    for x0 in (45, 55):
        carve(w, x0, WALK - 1, 23, x0, WALK - 1, 24)
        for z in (23, 24):
            lift(w, (x0, WALK - 1, z), (x0, WALK + 3, z), ticks=40, stop=30)
    w.place(O.TREASURE, 48, WALK + 4, 22)
    w.place(O.TREASURE, 52, WALK + 4, 26)
    hazard("mine", 48, WALK + 4, 26)
    hazard("mine", 52, WALK + 4, 22)
    teleporter(TP_HIGH, 50, WALK + 4, 24)
    carve(w, 40, 36, 8, 60, 40, 20)
    teleporter(TP_HIGH, 42, 36, 10)
    w.place(O.TREASURE, 56, 36, 10)
    w.place(O.TREASURE, 58, 36, 18)
    hazard(O.BULLDOZER, 44, 36, 14, **patrol(None, (58, 36, 14), 70))
    hazard(O.BULLDOZER, 50, 36, 18, **patrol(None, (50, 36, 10), 50))
    carve(w, 46, 36, 21, 48, 39, 34)                                      # a passage south to...
    carve(w, 46, WALK + 8, 32, 48, 35, 34)                                # ...the hole down to the hall
    w.place(O.EGG, 40, WALK, 12)
    w.place(O.EGG, 60, WALK, 36)

    dress(w, 2, 0, 2, 97, 44, 97, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
