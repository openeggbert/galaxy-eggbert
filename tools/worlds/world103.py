#!/usr/bin/env python3
"""Mission 103, the lava labyrinth (worlds3d/world103.vwr): brief plans/briefs/world103.md.

Mobile Eggbert's level 103 winds through grey cobble lit by lava: from Blupi's start two lifts
meeting over a trench of lava, a third over another; flooded passages where fish swim (dive and
swim through); a shaft climbed on seven lifts in turn, each meeting the next at its top or its
foot; a hall at the top where six lifts each carry him across a pool of lava from pillar to
pillar to the one treasure; a drop to the exit. Nine eggs, seven fish. Here the labyrinth is cut
in one mass of cobble in 3D: the gallery, the flooded dip under a curtain of stone, the lift
shaft, the hall of pools above the gallery, the pool by the treasure, the exit's chamber below.

    python3 tools/worlds/world103.py                 # writes worlds3d/world103.vwr
    python3 tools/worlds/world103.py --stage 1 out   # for levelcheck: floors over the water (the
                                                     # swims), the curtain cut
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, rock  # noqa: E402
from kit import O, World, lift  # noqa: E402
from station import water  # noqa: E402
from village import STONE  # noqa: E402

LAVA = 68
WALK = 6                    # the gallery; the exit's chamber walk 15, the top hall walk 27
TOP = 27


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world103.vwr"
    rng = random.Random(103)
    w = World(mission=103, sky=21)
    swum = stage is not None and stage >= 1

    def lifts(x0, x1, y, z0, z1, offset, ticks, stop):
        dx, dy, dz = offset
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                lift(w, (x, y, z), (x + dx, y + dy, z + dz), ticks=ticks, stop=stop)

    def fish(start, end, ticks):
        w.place(O.FISH, *start, end=end, advance=ticks, recede=ticks, stop_start=47, stop_end=47)

    def pool(x0, y0, z0, x1, y1, z1):
        """Water from y0 to its surface row y1; in the swum stage a floor over it instead."""
        if swum:
            w.fill(x0, y1, z0, x1, y1, z1, STONE)
        else:
            water(w, x0, y0, z0, x1, y1, z1)

    rock(w, 2, 0, 66, 96, 34, 92)

    # ---- the gallery (walk 6, west to east, z 80-86) ----
    # Two lifts meeting over the first trench of lava (dwelling 30 ticks where they meet), a
    # side pit flooded with two fish and an egg at its foot, a third lift over the second trench.
    carve(w, 4, WALK, 80, 52, WALK + 4, 86)
    w.spawn = (6, WALK, 83)
    w.place(O.EGG, 14, WALK, 85)
    carve(w, 19, 2, 80, 27, 5, 86)
    w.fill(19, 2, 80, 27, 3, 86, LAVA)
    lifts(19, 20, 5, 82, 84, (3, 0, 0), 30, 30)                          # ends at x 22-23
    lifts(26, 27, 5, 82, 84, (-2, 0, 0), 30, 30)                         # ends at x 24-25
    w.place(O.EGG, 31, WALK, 81)
    carve(w, 33, 1, 81, 36, 5, 85)
    pool(33, 1, 81, 36, 5, 85)
    fish((33, 3, 82), (36, 3, 82), 30)
    fish((36, 2, 84), (33, 2, 84), 30)
    w.place(O.EGG, 35, 1, 83)
    carve(w, 41, 2, 80, 46, 5, 86)
    w.fill(41, 2, 80, 46, 3, 86, LAVA)
    lifts(41, 42, 5, 82, 84, (3, 0, 0), 60, 20)

    # ---- the flooded dip (x 53-66) ----
    # The floor falls away into water level with it; a curtain of stone hangs from the roof into
    # the water across the middle: dive under it. Three fish, two eggs on the bottom.
    carve(w, 53, 1, 80, 66, WALK + 4, 86)
    pool(53, 1, 80, 66, 5, 86)
    if swum:
        carve(w, 59, WALK, 80, 60, WALK + 2, 86)
    else:
        w.fill(59, 3, 80, 60, WALK + 4, 86, STONE)
    fish((54, 2, 81), (58, 2, 81), 30)
    fish((65, 2, 85), (61, 2, 85), 30)
    fish((61, 4, 82), (65, 4, 82), 15)
    w.place(O.EGG, 56, 1, 85)
    w.place(O.EGG, 63, 1, 81)

    # ---- the lift shaft (x 80-87, walk 6 -> 27) ----
    # Seven lifts (2 x 2) in two columns, all in step: one rises, the next comes down to meet it
    # side by side at its top and carries Blupi up as it goes back, meeting the third at its
    # foot, and so on (30-tick dwells where they meet).
    carve(w, 67, WALK, 80, 79, WALK + 4, 86)
    w.place(O.EGG, 78, WALK, 81)
    carve(w, 80, WALK, 79, 87, TOP + 4, 87)
    carve(w, 81, WALK - 1, 82, 82, WALK - 1, 83)                         # the first lift's foot
    # The top's landing (x 80, walk 27) beside the last lift, so he steps off it onto the hall.
    w.fill(80, TOP - 1, 81, 80, TOP - 1, 84, STONE)
    p, q = (81, 82), (83, 84)
    for k, (cols, start, end) in enumerate(((p, 5, 8), (q, 11, 8), (p, 11, 14), (q, 17, 14),
                                            (p, 17, 20), (q, 23, 20), (p, 23, 26))):
        for x in cols:
            for z in (82, 83):
                lift(w, (x, start, z), (x, end, z), ticks=24, stop=30)

    # ---- the hall of pools (walk 27, east to west, z 80-86) ----
    # From the shaft's top west: six pools of lava between pillars of cobble, a lift on each
    # starting by its east pillar and carrying Blupi two cells to its west pillar (30-tick
    # dwells); eggs on two pillars; a pool of water two fish swim in before the treasure.
    carve(w, 20, TOP, 80, 79, TOP + 4, 86)
    for k in range(6):
        x0 = 73 - 5 * k                                                  # the pool x0 .. x0 + 2
        carve(w, x0, TOP - 3, 80, x0 + 2, TOP - 1, 86)
        w.fill(x0, TOP - 3, 80, x0 + 2, TOP - 2, 86, LAVA)
        lifts(x0 + 2, x0 + 2, TOP - 1, 82, 84, (-2, 0, 0), 12, 30)
    w.place(O.EGG, 66, TOP, 81)
    w.place(O.EGG, 56, TOP, 85)
    carve(w, 32, TOP - 5, 80, 38, TOP - 1, 86)
    pool(32, TOP - 5, 80, 38, TOP - 1, 86)
    fish((32, TOP - 3, 82), (38, TOP - 3, 82), 30)
    fish((38, TOP - 4, 84), (32, TOP - 4, 84), 30)
    w.place(O.TREASURE, 27, TOP, 83)

    # ---- the exit's chamber (walk 15), under the hall's west end ----
    carve(w, 20, 15, 76, 28, 18, 86)
    carve(w, 23, 19, 82, 25, TOP - 1, 84)                                 # the drop
    w.place(O.EGG, 25, 15, 78)
    w.place(O.EXIT, 21, 15, 78)

    dress(w, 2, 0, 66, 96, 34, 92, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
