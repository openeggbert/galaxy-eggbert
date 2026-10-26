#!/usr/bin/env python3
"""Mission 102, the three keys (worlds3d/world102.vwr): brief plans/briefs/world102.md.

Mobile Eggbert's level 102 starts in a shaft of grey cobble beside three treasures, each behind
a coloured door (334 red, 335 green, 336 blue), and teleporters (330-333) that lead to three
other works, each holding a key: blue machine panels where Blupi rides a skateboard (24) over a
row of mines, takes a lift up and crosses on caterpillar platforms (47) carrying him along to
the green key; grey riveted plates round a pool where two fish swim over the blue key; a cave of
brown rock where seven hanging mines hang over a low passage (crouch), a wasp flies and the red
key lies by lava. Here the atrium is a hall cut in cobble with its three doors and three
teleporters, and each work floats in the sky round it: the machine deck, the plate hall, the
rock cave.

    python3 tools/worlds/world102.py                 # writes worlds3d/world102.vwr
    python3 tools/worlds/world102.py --stage K out   # for levelcheck: K >= 1 the mines ridden over,
                                                     # the hanging mines crouched under, the
                                                     # caterpillars as floors; K >= 2 the doors open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, rock  # noqa: E402
from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402
from station import metal, water  # noqa: E402
from village import ROCK, ROCK_FACES  # noqa: E402

RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
TP_MACHINE, TP_PLATES, TP_ROCK = 330, 331, 332
LAVA = 68


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world102.vwr"
    rng = random.Random(102)
    w = World(mission=102, sky=7)
    ridden = stage is not None and stage >= 1
    opened = stage is not None and stage >= 2

    def teleporter(icon, x, walk, z):
        """A teleporter's pillar over (x, walk, z): standing under it Blupi leaves; arriving from
        its partner he comes out at (x, walk, z + 1)."""
        w.set(x, walk + 1, z, icon)

    # ---- the atrium (walk 6), cut in cobble ----
    # Spawn by the exit at its south end; along its north wall three niches, each with a
    # treasure behind its coloured door; in its walls the alcoves of the three teleporters.
    rock(w, 36, 0, 62, 64, 14, 94)
    carve(w, 40, 6, 68, 60, 10, 90)
    for x, door in ((44, RED_DOOR), (50, GREEN_DOOR), (56, BLUE_DOOR)):
        carve(w, x, 6, 64, x, 7, 67)
        if not opened:
            w.fill(x, 6, 67, x, 7, 67, door)
        w.place(O.TREASURE, x, 6, 65)
    for icon, x, z in ((TP_MACHINE, 39, 74), (TP_ROCK, 39, 82), (TP_PLATES, 61, 74)):
        carve(w, x, 6, z, x, 7, z + 1)
        teleporter(icon, x, 6, z)
    w.place(O.EXIT, 50, 6, 89)
    w.spawn = (50, 6, 85)
    dress(w, 36, 0, 62, 64, 14, 94, rng)

    # ---- the machine deck (walk 20), north-west in the sky ----
    # The arrival deck and five skateboards; a walled yard east of it, a block of nine mines across
    # it (ride over them: the skateboard sets them off); a lift up out of a pit of lava at its end
    # (walk 28); five caterpillar platforms carrying Blupi west, a gap of one between them, to the
    # platform of the green key; from it a drop back onto the deck.
    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE_FLOOR)

    panel(4, 17, 4, 20, 19, 20)
    teleporter(TP_MACHINE, 8, 20, 8)
    for x in range(12, 17):
        w.place(O.SKATEBOARD, x, 20, 15)
    panel(21, 17, 10, 42, 19, 14)
    panel(21, 20, 10, 42, 23, 10)
    panel(21, 20, 14, 42, 23, 14)
    panel(42, 20, 11, 42, 23, 13)
    if not ridden:
        for x in (26, 27, 28):
            for z in (11, 12, 13):
                mine(w, x, 20, z)
    carve(w, 39, 18, 11, 41, 19, 13)
    w.fill(39, 18, 11, 41, 18, 13, LAVA)
    for x in (39, 40):
        for z in (11, 12):
            lift(w, (x, 19, z), (x, 27, z), ticks=80, stop=30)
    for k in range(5):
        x0 = 37 - 3 * k
        for x in (x0, x0 + 1):
            for z in (11, 12):
                if ridden:
                    w.set(x, 27, z, MACHINE_FLOOR)
                else:
                    w.place(O.LIFT_LEFT, x, 27, z, end=(x, 27, z), advance=10, recede=10, stop_start=10,
                            stop_end=10)
    if ridden:
        w.fill(24, 27, 11, 36, 27, 12, MACHINE_FLOOR)
    panel(16, 25, 9, 23, 27, 14)
    w.place(O.KEY2, 19, 28, 12)

    # ---- the plate hall (walk 12), north-east in the sky ----
    # A hall of riveted plates round a pool level with its floor (climb out anywhere), two fish
    # swimming across it, the blue key on its bottom; a mine by the pool.
    metal(w, 55, 10, 3, 90, 17, 31, rng)
    metal(w, 64, 5, 8, 82, 9, 26, rng)                                  # the pool's basin
    carve(w, 56, 12, 4, 89, 16, 30)
    teleporter(TP_PLATES, 60, 12, 7)
    carve(w, 66, 7, 10, 80, 11, 24)
    water(w, 66, 7, 10, 80, 11, 24)
    w.place(O.FISH, 68, 9, 14, end=(78, 9, 14), advance=60, recede=60, stop_start=30, stop_end=30)
    w.place(O.FISH, 78, 9, 20, end=(68, 9, 20), advance=60, recede=60, stop_start=30, stop_end=30)
    w.place(O.KEY3, 73, 7, 17)
    if not ridden:
        mine(w, 63, 12, 17)

    # ---- the rock cave (walk 10), south-west in the sky ----
    # The arrival cave; south of it a low passage two wide where seven mines hang in Blupi's way
    # (crouch under them); the cave beyond, pools of lava, a wasp, the red key in a pocket
    # between two pools.
    def brown(x0, y0, z0, x1, y1, z1):
        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1):
                for z in range(z0, z1 + 1):
                    w.set(x, y, z, ROCK_FACES[0] if rng.random() < 0.2 else ROCK)

    brown(4, 6, 46, 26, 16, 94)
    carve(w, 6, 10, 48, 16, 13, 58)
    teleporter(TP_ROCK, 10, 10, 50)
    carve(w, 10, 10, 59, 11, 11, 70)
    if not ridden:
        for k, z in enumerate((61, 63, 65, 67)):
            for x in ((10, 11) if k < 3 else (10,)):
                mine(w, x, 10, z, hanging=True)
    carve(w, 6, 10, 71, 24, 14, 92)
    for x0, z0, x1, z1 in ((8, 78, 13, 83), (17, 80, 22, 85), (14, 86, 19, 90)):
        w.fill(x0, 9, z0, x1, 9, z1, LAVA)
    w.place(O.WASP, 10, 10, 75, end=(20, 10, 75), advance=24, recede=24, stop_start=8, stop_end=8)
    w.place(O.KEY1, 15, 10, 84)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
