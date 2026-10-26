#!/usr/bin/env python3
"""Mission 124, the cisterns (worlds3d/world124.vwr): brief plans/briefs/world124.md.

Mobile Eggbert's level 124 is brick cisterns full of water: from Blupi's start passages of brick
lead to tanks where eighteen fish (17) swim, the treasures on their bottoms and in pockets of their
walls; three pairs of teleporters (330-332) join the far tanks; mines (2) and hanging mines (3)
in the passages, mines bobbing (16) in shafts, five lifts, two crates (12), a fan (129) and its wind
(114), treasure doors (423 "3" to 437 "17") opening the way on as he collects. Seventeen treasures,
seven eggs. Here the cisterns are cut in a block of brick in 3D: the start passage, three tanks in
a row each a little deeper, a teleporter pair from each tank's far end to the next part, a wind
tunnel, the doors, the exit.

    python3 tools/worlds/world124.py                 # writes worlds3d/world124.vwr
    python3 tools/worlds/world124.py --stage K out   # for levelcheck: K treasures taken (the
                                                     # doors open as they count); K >= 1 no mines
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from station import brick, treasure_door, water  # noqa: E402

WALK = 20
FAN_EAST, WIND_EAST = 129, 114


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world124.vwr"
    rng = random.Random(124)
    w = World(mission=124, sky=27)
    taken = stage or 0
    calm = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    def teleporter(icon, x, walk, z):
        w.set(x, walk + 1, z, icon)

    def door(cells, n):
        for x, y, z in cells:
            carve(x, y, z, x, y, z)
            if taken < n:
                w.set(x, y, z, treasure_door(n))

    def tank(x0, z0, x1, z1, depth, fish):
        """A tank sunk `depth` into the floor, its water level with the floor; `fish` (start,
        end) patrols, left out in the calm stages."""
        carve(x0, WALK - depth, z0, x1, WALK - 1, z1)
        water(w, x0, WALK - depth, z0, x1, WALK - 1, z1)
        for k, (a, b) in enumerate(fish):
            if not calm:
                w.place(O.FISH, *a, end=b, advance=45 + 5 * k, recede=45 + 5 * k, stop_start=47, stop_end=47)

    brick(w, 2, 2, 10, 97, 30, 90, rng)

    # ---- the start passage (walk 20) ----
    carve(4, WALK, 46, 20, WALK + 3, 54)
    w.spawn = (6, WALK, 50)
    w.place(O.EXIT, 5, WALK, 53)
    hazard(12, WALK + 1, 50, hanging=True)
    hazard(16, WALK, 48)
    w.place(O.EGG, 18, WALK, 53)

    # ---- tank 1 (x 21-37, three deep): treasures 1-4 ----
    carve(21, WALK, 40, 37, WALK + 4, 60)
    tank(23, 42, 35, 58, 3, [((24, WALK - 2, 45), (34, WALK - 2, 45)), ((34, WALK - 3, 55), (24, WALK - 3, 55)),
                             ((29, WALK - 2, 43), (29, WALK - 2, 57))])
    for x, y, z in ((25, WALK - 3, 50), (33, WALK - 3, 46), (30, WALK - 3, 56), (36, WALK, 41)):
        w.place(O.TREASURE, x, y, z)
    teleporter(330, 36, WALK, 58)
    door([(38, y, z) for y in (WALK, WALK + 1) for z in (49, 50, 51)], 3)

    # ---- tank 2 (x 39-59, five deep): treasures 5-9, lifts across it ----
    carve(39, WALK, 38, 59, WALK + 5, 62)
    tank(41, 40, 57, 60, 5, [((42, WALK - 3, 44), (56, WALK - 3, 44)), ((56, WALK - 4, 50), (42, WALK - 4, 50)),
                             ((42, WALK - 2, 56), (56, WALK - 2, 56)), ((49, WALK - 5, 41), (49, WALK - 5, 59)),
                             ((44, WALK - 4, 58), (54, WALK - 4, 58))])
    for x, z in ((42, 49), (43, 49)):
        lift(w, (x, WALK - 1, z), (x + 13, WALK - 1, z), ticks=90, stop=40)
    for x, y, z in ((45, WALK - 5, 42), (53, WALK - 5, 47), (47, WALK - 5, 57), (56, WALK - 5, 52), (58, WALK, 61)):
        w.place(O.TREASURE, x, y, z)
    w.place(O.EGG, 40, WALK, 39)
    w.place(O.EGG, 58, WALK, 39)
    teleporter(331, 58, WALK, 58)
    door([(60, y, z) for y in (WALK, WALK + 1) for z in (49, 50, 51)], 9)

    # ---- the wind tunnel (walk 20) and tank 3 (x 70-92, seven deep): treasures 10-17 ----
    # A fan in the tunnel's west end blowing east, its wind pushing Blupi along over a pit with mines
    # bobbing in it; tank 3 beyond: deep, ten fish, eight treasures on its bottom and its ledges; two
    # crates; teleporter 332 back to the start passage.
    carve(61, WALK, 49, 69, WALK + 2, 51)
    w.set(61, WALK + 1, 50, FAN_EAST)
    carve(62, WALK, 49, 69, WALK + 2, 51)
    # The wind fills the tunnel's lower two rows: the engine (as the original, his middle in his
    # own tile) feels it in the cell at his feet.
    w.fill(62, WALK, 49, 69, WALK + 1, 51, WIND_EAST)
    carve(64, WALK - 4, 49, 66, WALK - 1, 51)
    if not calm:
        w.place(O.MOVING_MINE, 65, WALK - 4, 50, end=(65, WALK - 1, 50), advance=20, recede=20, stop_start=0,
                stop_end=0)
    carve(70, WALK, 36, 95, WALK + 6, 64)
    tank(72, 38, 93, 62, 7, [((73, WALK - 3, 40), (92, WALK - 3, 40)), ((92, WALK - 4, 44), (73, WALK - 4, 44)),
                             ((73, WALK - 5, 48), (92, WALK - 5, 48)), ((92, WALK - 6, 52), (73, WALK - 6, 52)),
                             ((73, WALK - 2, 56), (92, WALK - 2, 56)), ((92, WALK - 3, 60), (73, WALK - 3, 60)),
                             ((80, WALK - 5, 39), (80, WALK - 5, 61)), ((86, WALK - 4, 61), (86, WALK - 4, 39)),
                             ((76, WALK - 6, 50), (90, WALK - 6, 50)), ((90, WALK - 2, 46), (76, WALK - 2, 46))])
    for x, z in ((75, 42), (83, 41), (90, 45), (78, 53), (88, 54), (74, 60), (85, 59)):
        w.place(O.TREASURE, x, WALK - 7, z)
    w.place(O.TREASURE, 94, WALK, 37)
    for x, z in ((71, 37), (71, 63)):
        w.place(O.CRATE, x, WALK, z)
    w.place(O.EGG, 94, WALK, 63)
    w.place(O.EGG, 71, WALK, 50)
    teleporter(332, 94, WALK, 50)
    teleporter(332, 8, WALK, 47)
    teleporter(330, 22, WALK, 41)
    teleporter(331, 40, WALK, 61)
    door([(x, y, 45) for y in (WALK, WALK + 1) for x in (5, 6, 7)], 17)
    carve(4, WALK, 42, 8, WALK + 2, 44)
    w.place(O.EGG, 6, WALK, 43)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
