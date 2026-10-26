#!/usr/bin/env python3
"""Mission 066, the slime winds (worlds3d/world066.vwr): brief plans/briefs/world066.md.

Mobile Eggbert's level 066, the last of world 6, ends not at the red arrow but at the golden key
(ObjectType 21, the secret exit). A strip of slime passages: Blupi starts between two mines by
an egg, passes a curtain of lightning and three big creatures; above, a blupit, a bulldozer, a
lift over a gap, a trench of mines crossed hanging from bars, an upper corridor with moving
mines and traps to teleporter 1; east two wind tunnels, one blowing east and one west, with
mines and a bulldozer in the wind; far east, over a lava pit, a line of thin rods Blupi hangs
from past a rising mine and a bird, to the golden key; teleporter 2 beside them. Here the strip
is built in 3D as two storeys over each other, the tunnels side by side over each other, and the
lava pit across the whole east cave.

    python3 tools/worlds/world066.py                 # writes worlds3d/world066.vwr
    python3 tools/worlds/world066.py --stage 1 out   # for levelcheck: no mines, enemies or
                                                     # lightning in the narrow ways, floors under
                                                     # the bars and the rods
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from slime import POLES, SLIME_TRAP, skin, slime  # noqa: E402

LAVA = 68
SAW, BAR, ROD, DRIP, KNOB = 378, 138, 202, 404, 410
BLITZ, EMITTER = 305, 304
FAN_LEFT, FAN_RIGHT = 126, 129        # blowing toward -X, toward +X
WIND_WEST, WIND_EAST = 110, 114
TELEPORTER = 331


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world066.vwr"
    rng = random.Random(66)
    w = World(mission=66, sky=31)
    solved = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def patrol(otype, start, end, ticks, stop):
        if not solved:
            w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def danger(x, y, z, hanging=False):
        if not solved:
            mine(w, x, y, z, hanging)

    slime(w, 6, 14, 40, 98, 40, 70)

    # ---- the start corridor (walk 20) ----
    # One wide, three high, along X: an egg behind the mine west of Blupi's start, a mine east of
    # it; a curtain of lightning; three big creatures (they rest at the ends of their walks,
    # deadly then, harmless walking), the middle one taking a single step; a trap in the floor.
    carve(16, 20, 60, 51, 22, 60)
    w.place(O.EGG, 17, 20, 60)
    danger(18, 20, 60)
    danger(20, 20, 60)
    if not solved:
        w.fill(24, 20, 60, 24, 21, 60, BLITZ)
    w.set(24, 22, 60, EMITTER)
    patrol(O.BIG_CREATURE, (27, 20, 60), (32, 20, 60), 100, 200)
    patrol(O.BIG_CREATURE, (35, 20, 60), (36, 20, 60), 40, 200)
    patrol(O.BIG_CREATURE, (43, 20, 60), (39, 20, 60), 80, 200)
    w.set(30, 19, 60, SLIME_TRAP)

    # ---- the middle storey (walk 26) ----
    # Up a stair at the corridor's east end: B1, a blupit shooting along the room; west the
    # bulldozer's corridor; a gap crossed on a lift; the trench of five mines (and a crate at its
    # end), the only way over it hanging from the bars above.
    carve(48, 20, 57, 49, 28, 59)                                     # its top step the room's floor
    slime(w, 48, 20, 58, 49, 21, 58)                                  # walk 22
    slime(w, 48, 20, 57, 49, 23, 57)                                  # walk 24
    carve(40, 26, 50, 49, 29, 56)
    patrol(O.CREATURE, (42, 26, 52), (46, 26, 52), 50, 23)
    carve(30, 26, 53, 39, 28, 53)
    patrol(O.BULLDOZER, (38, 26, 53), (33, 26, 53), 90, 20)
    carve(26, 22, 53, 29, 28, 53)                                     # the gap, floor 21
    slime(w, 29, 22, 53, 29, 23, 53)                                  # a step out, walk 24
    if solved:
        w.fill(26, 25, 53, 29, 25, 53, 10)
    else:
        lift(w, (29, 25, 53), (26, 25, 53), ticks=18, stop=3)
    carve(12, 26, 53, 25, 28, 53)
    carve(15, 25, 53, 20, 25, 53)                                     # the trench, walk 25
    for x in range(15, 20):
        danger(x, 25, 53)
    if solved:
        w.fill(15, 24, 53, 20, 25, 53, 10)                            # filled for the checks
    else:
        w.place(O.CRATE, 20, 25, 53)
    w.fill(14, 27, 53, 21, 27, 53, BAR)

    # ---- the upper corridor (walk 32) ----
    # A stair up from the trench room's west end; the corridor east, one wide: traps to jump, a
    # still moving mine and one sweeping it, an egg at its east end under two drips; teleporter 1
    # hanging over its west end.
    carve(10, 26, 47, 11, 34, 53)
    slime(w, 10, 26, 52, 11, 27, 52)                                  # walk 28
    slime(w, 10, 26, 51, 11, 29, 51)                                  # walk 30
    slime(w, 10, 26, 47, 11, 31, 50)                                  # walk 32
    carve(10, 32, 46, 46, 34, 46)
    w.set(12, 33, 46, TELEPORTER)
    for x in (16, 32, 34):
        w.set(x, 31, 46, SLIME_TRAP)
    patrol(O.MOVING_MINE, (21, 32, 46), (21, 32, 46), 10, 0)
    patrol(O.MOVING_MINE, (26, 32, 46), (30, 32, 46), 40, 0)
    w.place(O.EGG, 46, 32, 46)
    for x in (44, 45):
        w.set(x, 34, 46, DRIP)

    # ---- the wind tunnels (walk 20, 26) ----
    # The lower, along X: a fan at its west end blowing east, the wind in its whole bore, three
    # mines to jump in it; entered from the start corridor beside the fan; an egg at its east end.
    # The upper, over it: a fan at its east end blowing west, three mines and the bulldozer in the
    # wind; entered from the east beside the fan, it comes out by the middle storey.
    carve(51, 20, 61, 51, 21, 61)
    carve(50, 20, 62, 68, 21, 62)
    w.fill(49, 20, 62, 49, 21, 62, FAN_RIGHT)
    w.fill(51, 20, 62, 67, 21, 62, WIND_EAST)
    for x in (57, 61, 65):                                            # room to react first
        danger(x, 20, 62)
    w.place(O.EGG, 68, 20, 62)
    carve(50, 26, 62, 68, 27, 62)
    w.fill(69, 26, 62, 69, 27, 62, FAN_LEFT)
    w.fill(51, 26, 62, 67, 27, 62, WIND_WEST)
    for x in (52, 60, 63):
        danger(x, 26, 62)
    patrol(O.BULLDOZER, (56, 26, 62), (60, 26, 62), 120, 20)
    carve(50, 26, 56, 50, 27, 61)                                     # out to the middle storey

    # ---- the east (walk 20 -> 32) ----
    # The lower tunnel goes on east: two saws in its floor, a striped pole; a stair up to a
    # corridor (walk 26) whose floor is lava on every other cell under knobs, a side way to the
    # upper tunnel; a stair up to the east cave (walk 32): teleporter 2 on its west ledge, the
    # lava pit across the whole cave, thin rods over it to hang from, a mine rising and falling
    # through them and a bird flying over; the far ledge: a trap, a pole, the golden key.
    carve(69, 20, 62, 78, 22, 62)
    for x in (71, 73):
        w.set(x, 19, 62, 10 if solved else SAW)
        w.rotate_plate(x, 19, 62)
    w.fill(75, 20, 63, 75, 21, 63, rng.choice(POLES))                 # in a niche
    carve(76, 20, 62, 78, 27, 64)
    slime(w, 77, 20, 63, 78, 21, 63)                                  # walk 22
    slime(w, 77, 20, 64, 78, 23, 64)                                  # walk 24
    carve(66, 26, 65, 79, 29, 65)
    for x in (70, 72, 74):
        w.set(x, 25, 65, LAVA)
        w.set(x, 29, 65, KNOB)
    carve(67, 26, 63, 67, 27, 64)                                     # beside the upper fan
    carve(79, 26, 63, 81, 34, 66)
    slime(w, 80, 26, 65, 81, 27, 65)                                  # walk 28
    slime(w, 80, 26, 64, 81, 29, 64)                                  # walk 30
    slime(w, 80, 26, 63, 81, 31, 63)                                  # walk 32
    carve(80, 32, 56, 98, 36, 66)
    w.set(81, 33, 57, TELEPORTER)
    carve(84, 28, 56, 92, 31, 66)
    w.fill(84, 27, 56, 92, 27, 66, LAVA)
    w.fill(84, 32, 61, 92, 32, 61, ROD)
    if solved:
        w.fill(84, 31, 61, 92, 31, 61, 10)
    patrol(O.MOVING_MINE, (86, 28, 61), (86, 35, 61), 60, 0)
    patrol(O.BIRD, (91, 33, 58), (87, 32, 64), 32, 8)
    w.set(94, 31, 61, SLIME_TRAP)
    w.fill(96, 32, 59, 96, 33, 59, rng.choice(POLES))
    w.set(97, 35, 59, KNOB)
    w.place(O.SECRET_EXIT, 98, 32, 61)

    skin(w, rng)
    w.spawn = (19, 20, 60)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
