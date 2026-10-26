#!/usr/bin/env python3
"""Mission 112, the machine cavern (worlds3d/world112.vwr): brief plans/briefs/world112.md.

Mobile Eggbert's level 112 is a cavern in the blue machine panel: from Blupi's start west past a
bulldozer (4) to a row of six helicopters (13); up on the left a gallery with three small lifts
bobbing between mines (2) on posts and the first treasure with two eggs; in the middle, high up,
the second treasure between two mines; east past a column of lightning (305) and a mine a tall
lift (54 ticks) up to the exit, eggs by its foot. Here the cavern is cut in a block of panel in
3D: the start hall, the bulldozer's passage, the hangar under the open west, the gallery along
its north wall, the high chamber over the start, the lightning passage, the lift to the exit.

    python3 tools/worlds/world112.py                 # writes worlds3d/world112.vwr
    python3 tools/worlds/world112.py --stage 1 out   # for flightcheck: no mines, bulldozer or
                                                     # lightning
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402

WALK, HIGH = 6, 18
BLITZ, EMITTER = 305, 304


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world112.vwr"
    rng = random.Random(112)
    w = World(mission=112, sky=0)
    calm = stage is not None and stage >= 1

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z):
        if not calm:
            mine(w, x, y, z)

    panel(2, 0, 38, 60, 30, 62)

    # ---- the start hall (walk 6) and the high chamber over it (walk 18) ----
    carve(24, WALK, 44, 36, WALK + 4, 56)
    w.spawn = (30, WALK, 52)
    carve(20, HIGH, 44, 32, HIGH + 4, 52)
    w.place(O.TREASURE, 26, HIGH, 48)
    hazard(24, HIGH, 48)
    hazard(28, HIGH, 48)

    # ---- the bulldozer's passage and the hangar (walk 6) ----
    carve(14, WALK, 48, 23, WALK + 2, 52)
    for x in (16, 20):
        carve(x, WALK, 47, x, WALK + 1, 47)
    if not calm:
        w.place(O.BULLDOZER, 21, WALK, 50, end=(15, WALK, 50), advance=60, recede=60, stop_start=20,
                stop_end=20)
    carve(4, WALK, 42, 13, HIGH + 6, 58)                                  # the hangar, open above
    for z in (44, 46, 48, 50, 52, 54):
        w.place(O.HELICOPTER, 10, WALK, z)
    w.place(O.EGG, 5, WALK, 56)
    carve(14, HIGH, 44, 19, HIGH + 4, 52)                                 # the way into the chamber

    # ---- the gallery (walk 18) along the hangar's north wall ----
    # A ledge two wide: the first treasure and two eggs at its west end; east of them three small
    # pits, a lift bobbing in each, mines on the posts between.
    panel(4, HIGH - 2, 42, 13, HIGH - 1, 43)
    w.place(O.TREASURE, 4, HIGH, 42)
    w.place(O.EGG, 5, HIGH, 43)
    w.place(O.EGG, 6, HIGH, 43)
    for x in (8, 10, 12):
        carve(x, HIGH - 2, 42, x, HIGH - 1, 43)
        for z in (42, 43):
            lift(w, (x, HIGH - 2, z), (x, HIGH - 1, z), ticks=10, stop=10)
    for x in (9, 11):
        hazard(x, HIGH, 42)

    # ---- the lightning passage and the lift to the exit ----
    # East from the hall a passage, a curtain of lightning across it under its emitters, a mine
    # beyond; the lift (2 x 2, 54 ticks) up nine to the exit's room; three eggs by its foot.
    carve(37, WALK, 48, 48, WALK + 2, 52)
    for z in range(48, 53):
        w.set(41, WALK + 3, z, EMITTER)
        if not calm:
            w.fill(41, WALK, z, 41, WALK + 2, z, BLITZ)
    hazard(44, WALK, 50)
    carve(49, WALK, 46, 54, WALK + 10, 54)
    for x in (50, 51):
        for z in (49, 50):
            lift(w, (x, WALK - 1, z), (x, WALK + 8, z), ticks=54, stop=30)
    carve(50, WALK - 1, 49, 51, WALK - 1, 50)
    for x in (52, 53, 54):
        w.place(O.EGG, x, WALK, 47)
    panel(52, WALK, 50, 54, WALK + 8, 54)                                  # the exit's ledge, walk 15
    carve(49, WALK + 9, 44, 56, WALK + 12, 56)
    w.place(O.EXIT, 54, WALK + 9, 52)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
