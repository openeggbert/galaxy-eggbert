#!/usr/bin/env python3
"""Mission 104, the jeeps (worlds3d/world104.vwr): brief plans/briefs/world104.md.

Mobile Eggbert's level 104 is a cobbled cave crossed in jeeps (19): six of them wait on a ledge
over a long corridor of mines (2), which a jeep sets off one after another as it drives on; a
heap of crates by the start, a treasure on a ledge, hanging mines (3) over the way down to
another; a lift bobbing over lava; a shaft of lifts sliding to and fro one above the other up to
a treasure; a seventh jeep over a second row of mines to a treasure; a long winding way down
with mines on it to the exit. Six treasures, three eggs, forty-four mines. Here the cave is cut
in cobble in 3D: the start cave, the garage, the corridor of mines, the hall with its crates and
its pit of lava, the sliding ladder, the lower gallery and the spiral down to the exit.

    python3 tools/worlds/world104.py                 # writes worlds3d/world104.vwr
    python3 tools/worlds/world104.py --stage 1 out   # for levelcheck: the mines driven over, the
                                                     # hanging mines crouched under, the crates
                                                     # pushed into their steps, the sliding
                                                     # ladder as a stair
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, fork_alcove, rock  # noqa: E402
from kit import O, World, lift, mine  # noqa: E402
from village import STONE  # noqa: E402

LAVA = 68
TOP, HALL, LOW = 30, 28, 18         # the walks: start cave and corridor, hall, lower gallery


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world104.vwr"
    rng = random.Random(104)
    w = World(mission=104, sky=2)
    driven = stage is not None and stage >= 1

    def mines(cells):
        if not driven:
            for x, y, z in cells:
                mine(w, x, y, z)

    def hanging(x, y, z):
        if not driven:
            mine(w, x, y, z, hanging=True)

    def crate(at, solved_at):
        w.place(O.CRATE, *(solved_at if driven else at))

    rock(w, 2, 0, 36, 97, 44, 66)

    # ---- the start cave (walk 30) ----
    # Spawn mid-cave. A ledge three up in its north-west corner holds treasure 1: push a crate of
    # the heap to its foot. South, a passage down in steps, two mines hanging in its walking
    # cells (crouch under them), treasure 2 at its end.
    carve(w, 4, TOP, 42, 16, TOP + 4, 56)
    w.spawn = (10, TOP, 52)
    w.fill(4, TOP, 42, 6, TOP + 2, 44, STONE)
    w.place(O.TREASURE, 5, TOP + 3, 43)
    crate((10, TOP, 46), (7, TOP, 44))
    crate((11, TOP, 46), (11, TOP, 46))
    crate((10, TOP, 47), (10, TOP, 47))
    for k, z in enumerate(range(57, 64)):
        floor = TOP - 1 - (k // 2)
        x0, x1 = (7, 7) if z in (59, 61) else (6, 8)                  # one wide where they hang
        carve(w, x0, floor + 1, z, x1, floor + 3, z)
    hanging(7, TOP - 1, 59)
    hanging(7, TOP - 2, 61)
    w.place(O.TREASURE, 7, TOP - 3, 63)
    fork_alcove(w, 16, TOP, 44, (1, 0))

    # ---- the garage and the corridor of mines (walk 30) ----
    # Six jeeps in a bay east of the cave, two eggs by them; the corridor east of the bay, three
    # wide, four blocks of mines two deep across it (no jump clears one: the jeep drives through,
    # setting them off). Then a step down into a passage two wide, a block of four more.
    carve(w, 17, TOP, 44, 20, TOP + 3, 56)
    for z in (45, 47, 49, 51, 53, 55):
        w.place(O.JEEP, 18, TOP, z)
    w.place(O.EGG, 20, TOP, 45)
    w.place(O.EGG, 20, TOP, 55)
    carve(w, 21, TOP, 49, 46, TOP + 2, 51)
    mines([(x, TOP, z) for x in (24, 25, 30, 31, 36, 37, 42, 43) for z in (49, 50, 51)])
    carve(w, 47, HALL, 49, 52, TOP + 2, 50)
    mines([(x, HALL, z) for x in (49, 50) for z in (49, 50)])

    # ---- the hall (walk 28) ----
    # Treasure 3 on a ledge three up along its north wall: push one of its crates there. In its
    # south-east corner a pit of lava, a lift bobbing in it between two gaps of one and a pillar
    # beyond with treasure 4 (jump when the lift is up); a mine hangs over the lava. An egg.
    carve(w, 53, HALL, 42, 71, HALL + 6, 58)
    w.fill(58, HALL, 42, 66, HALL + 2, 43, STONE)
    w.place(O.TREASURE, 62, HALL + 3, 42)
    crate((56, HALL, 46), (60, HALL, 44))
    crate((57, HALL, 46), (57, HALL, 46))
    crate((56, HALL, 47), (56, HALL, 47))
    carve(w, 60, HALL - 4, 52, 66, HALL - 1, 57)
    w.fill(60, HALL - 4, 52, 66, HALL - 3, 57, LAVA)
    for x in (61, 62):
        for z in (54, 55):
            lift(w, (x, HALL - 2, z), (x, HALL - 1, z), ticks=20, stop=20)
    w.fill(64, HALL - 4, 54, 65, HALL - 1, 55, STONE)
    w.place(O.TREASURE, 65, HALL, 55)
    hanging(63, HALL + 1, 52)
    w.place(O.EGG, 68, HALL, 46)

    # ---- the sliding ladder (walk 28 -> 38) ----
    # East of the hall a shaft: five lifts two apart one above the other, each sliding a cell to
    # and fro (20 ticks, dwell 20), the even ones west of the odd ones and sliding the other way:
    # at the end of their slide two neighbours stand side by side, a jump up from one to the next
    # with free head room. At the top a landing north of the last, treasure 5 on it.
    carve(w, 72, HALL, 47, 80, 44, 52)
    for k in range(5):
        y = HALL - 1 + 2 * (k + 1)
        x0, dx = (74, 1) if k % 2 == 0 else (78, -1)
        if driven:
            # levelcheck's stand-in: it wants three free cells over a jump of two (the game
            # needs two), so a plain stair climbing north to the landing instead.
            w.fill(74, y, 52 - k, 76, y, 52 - k, STONE)
            continue
        for x in (x0, x0 + 1):
            for z in (49, 50):
                lift(w, (x, y, z), (x + dx, y, z), ticks=20, stop=20)
    w.fill(74, HALL + 8, 47, 76, HALL + 9, 48, STONE)                    # the landing, walk 38
    w.place(O.TREASURE, 75, HALL + 10, 47)

    # ---- the lower gallery (walk 18) ----
    # A hole in the hall's floor drops into it. The seventh jeep; two blocks of mines two deep
    # across it; treasure 6 at its east end; the door north into the spiral.
    carve(w, 53, LOW, 58, 90, LOW + 3, 62)
    carve(w, 54, LOW + 4, 58, 56, HALL - 1, 58)                          # the hole
    w.place(O.JEEP, 58, LOW, 60)
    mines([(x, LOW, z) for x in (64, 65, 72, 73) for z in (59, 60, 61)])
    w.place(O.TREASURE, 88, LOW, 61)

    # ---- the spiral down to the exit (walk 18 -> 3) ----
    # A shaft with a ramp of steps one high, two wide, winding down round a central pillar; four
    # mines on it (the jeep sets them off, a walker steps round them); the exit at its foot.
    carve(w, 84, 1, 40, 94, LOW + 3, 53)
    ring = ([(85, z) for z in range(53, 40, -1)] + [(x, 41) for x in range(86, 94)] +
            [(93, z) for z in range(42, 54)] + [(x, 53) for x in range(92, 85, -1)])
    path = ring + ring
    for i, (x, z) in enumerate(path):
        walk = LOW - i // 3
        if walk < 3:
            break
        inner = (x + (1 if x == 85 else -1 if x == 93 else 0), z + (1 if z == 41 else -1 if z == 53 else 0))
        for cx, cz in ((x, z), inner):
            w.set(cx, walk - 1, cz, STONE)
    w.fill(87, 1, 43, 91, LOW + 3, 51, STONE)                            # the central pillar
    mines([(85, 16, 47), (90, 13, 41), (93, 9, 48), (89, 6, 53)])
    w.place(O.EXIT, 86, 3, 46)
    carve(w, 85, LOW, 54, 86, LOW + 2, 57)                               # the door from the gallery

    dress(w, 2, 0, 36, 97, 44, 66, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
