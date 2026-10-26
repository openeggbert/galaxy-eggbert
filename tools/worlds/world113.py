#!/usr/bin/env python3
"""Mission 113, the lightning loop (worlds3d/world113.vwr): brief plans/briefs/world113.md.

Mobile Eggbert's level 113 runs under a pale blue sky through the blue machine panel and back:
Blupi starts beside the exit; east a gallery of small quick lifts over lava under hanging mines
(3), columns of lightning (305) one after another, a long lift over a pit of lava, a pool with a
fish (17), lifts down; at the far end the one treasure in a cage of four tiny lifts opening and
closing round it, mines hanging above and lying below; then the long way back to the exit. Five
eggs, twelve mines (2), twelve lifts, a crate. Here the loop is laid out in 3D: the start ledge,
the gallery of lifts, the lightning, the ferry over the pit, the pool, the descent, the cage (four
little bridges sliding in to the treasure's pillar, as lifts here do not block his way sideways),
the lift up and the high way back, dropping onto the start ledge.

    python3 tools/worlds/world113.py                 # writes worlds3d/world113.vwr
    python3 tools/worlds/world113.py --stage 1 out   # for levelcheck: no mines, hanging mines or
                                                     # lightning; floors over the water
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402
from station import water  # noqa: E402

LAVA = 68
BLITZ, EMITTER = 305, 304
WALK, LOW, HIGHWAY = 20, 10, 28


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world113.vwr"
    rng = random.Random(113)
    w = World(mission=113, sky=26)
    calm = stage is not None and stage >= 1

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    def lightning(x, y0, y1, z0, z1):
        """A curtain of lightning across a passage at x, its emitters above."""
        for z in range(z0, z1 + 1):
            carve(x, y0, z, x, y1, z)
            w.set(x, y1 + 1, z, EMITTER)
            if not calm:
                w.fill(x, y0, z, x, y1, z, BLITZ)

    def lifts(x0, x1, y, z0, z1, offset, ticks, stop):
        dx, dy, dz = offset
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                lift(w, (x, y, z), (x + dx, y + dy, z + dz), ticks=ticks, stop=stop)

    panel(4, 2, 40, 92, HIGHWAY + 4, 64)

    # ---- the start ledge (walk 20) and the gallery of lifts ----
    # The exit beside the spawn (it needs the treasure). East a gallery three wide over four pits of
    # lava, a small lift in each (quick: 10-12 ticks, 20 dwelling); mines hang a cell above Blupi's
    # head over the last two (walk on, never jump).
    carve(6, WALK, 47, 12, WALK + 4, 53)
    w.spawn = (9, WALK, 50)
    w.place(O.EXIT, 7, WALK, 48)
    carve(13, WALK, 49, 40, WALK + 2, 51)
    for k, x0 in enumerate((15, 21, 27, 33)):
        carve(x0, WALK - 4, 49, x0 + 3, WALK - 1, 51)
        w.fill(x0, WALK - 4, 49, x0 + 3, WALK - 3, 51, LAVA)
        lifts(x0, x0, WALK - 1, 49, 51, (3, 0, 0), 10 + 2 * (k % 2), 20)
        if k >= 2:
            hazard(x0 + 1, WALK + 1, 50, hanging=True)
    w.place(O.EGG, 25, WALK, 49)

    # ---- the lightning and the ferry over the pit (walk 20) ----
    lightning(41, WALK, WALK + 2, 49, 51)
    carve(42, WALK, 46, 60, WALK + 4, 54)
    w.place(O.CRATE, 43, WALK, 47)
    carve(44, WALK - 5, 46, 53, WALK - 1, 54)
    w.fill(44, WALK - 5, 46, 53, WALK - 4, 54, LAVA)
    lifts(44, 45, WALK - 1, 49, 51, (8, 0, 0), 80, 30)
    hazard(52, WALK + 1, 47, hanging=True)
    w.place(O.EGG, 59, WALK, 53)

    # ---- the pool and the descent (walk 20 -> 10) ----
    # A pool across the way with a fish (swim across, or wait); a lift down the shaft beyond it to
    # the low passage (walk 10); a second curtain of lightning across it.
    carve(61, WALK, 48, 70, WALK + 3, 52)
    carve(62, WALK - 4, 48, 67, WALK - 1, 52)
    if calm:
        w.fill(62, WALK - 1, 48, 67, WALK - 1, 52, MACHINE_FLOOR)
    else:
        water(w, 62, WALK - 4, 48, 67, WALK - 1, 52)
        w.place(O.FISH, 62, WALK - 3, 50, end=(67, WALK - 3, 50), advance=45, recede=45, stop_start=47,
                stop_end=47)
    carve(71, LOW, 47, 76, WALK + 3, 53)
    # The lift stands against the pool room's floor (x 70), so Blupi walks straight onto it.
    lifts(71, 72, WALK - 1, 49, 50, (0, LOW - WALK, 0), 40, 30)
    carve(71, LOW - 1, 49, 72, LOW - 1, 50)
    w.place(O.EGG, 75, LOW, 52)
    carve(77, LOW, 49, 90, LOW + 2, 51)
    lightning(79, LOW, LOW + 2, 49, 51)
    hazard(82, LOW, 49)
    hazard(84, LOW, 51)

    # ---- the cage (walk 10) ----
    # A pit round a pillar with the treasure on it; four little bridges, one from each side,
    # sliding in to the pillar and out again (20 ticks, dwell 20); mines hang a cell above them
    # and lie in the pit below.
    carve(80, LOW, 52, 90, LOW + 4, 60)
    carve(81, LOW - 4, 53, 89, LOW - 1, 59)
    panel(84, LOW - 4, 55, 86, LOW - 1, 57)                              # the pillar
    w.place(O.TREASURE, 85, LOW, 56)
    for start, offset in (((81, LOW - 1, 56), (2, 0, 0)), ((89, LOW - 1, 56), (-2, 0, 0)),
                          ((85, LOW - 1, 53), (0, 0, 1)), ((85, LOW - 1, 59), (0, 0, -1))):
        lift(w, start, tuple(a + b for a, b in zip(start, offset)), ticks=20, stop=20)
    for x, z in ((82, 56), (88, 56), (85, 54)):
        hazard(x, LOW + 1, z, hanging=True)
    for x, z in ((82, 54), (88, 58), (83, 58)):
        hazard(x, LOW - 4, z)
    carve(82, LOW, 52, 88, LOW + 1, 52)                                   # its way in from the passage

    # ---- the way back (walk 10 -> 28 -> 20) ----
    # From the cage's west side a lift up to the high way (walk 28), a corridor back west over the
    # whole loop, a third curtain of lightning across it, and a hole down onto the start ledge.
    carve(77, LOW, 55, 79, HIGHWAY + 2, 57)
    # Against the high way's floor (x 76) at the top; reached along the shaft's floor below.
    lifts(77, 77, LOW - 1, 55, 57, (0, HIGHWAY - LOW, 0), 90, 30)
    carve(77, LOW - 1, 55, 77, LOW - 1, 57)
    carve(8, HIGHWAY, 55, 79, HIGHWAY + 2, 57)
    lightning(40, HIGHWAY, HIGHWAY + 2, 55, 57)
    for x, z in ((60, 55), (30, 57), (20, 56)):
        hazard(x, HIGHWAY, z)
    w.place(O.EGG, 50, HIGHWAY, 56)
    carve(8, WALK + 5, 52, 10, HIGHWAY - 1, 56)                          # the hole down
    carve(8, HIGHWAY - 1, 55, 10, HIGHWAY - 1, 57)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
