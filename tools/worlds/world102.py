#!/usr/bin/env python3
"""Mission 102, the three keys (worlds3d/world102.vwr): brief plans/briefs/world102.md.

Mobile Eggbert's level 102 starts in a shaft of grey cobble beside three treasures, each behind
a coloured door (334 red, 335 green, 336 blue), and teleporters (330-333) that lead to three
other works, each holding a key: blue machine panels where Blupi rides a skateboard (24) over a
row of mines, takes a lift up and crosses on caterpillar platforms (47) carrying him along to
the green key; grey riveted plates round a pool where two fish swim over the blue key; a cave of
brown rock where seven hanging mines hang over a low passage (crouch), a wasp flies and the red
key lies on a crumbling bridge (364) over lava. The machine works' way passes two columns of lightning (304/305), one each side
of the mines; from the plate works' pool a shaft goes down in a fan's updraught (132) to a
gallery where a switch (384) stops a saw (378), and a fan's wind (126) blows Blupi along to a
fourth teleporter (333) back to the atrium. Here the atrium is a hall cut in cobble with its
three doors and four teleporters, and each work floats in the sky round it: the machine deck,
the plate hall, the rock cave.

    python3 tools/worlds/world102.py                 # writes worlds3d/world102.vwr
    python3 tools/worlds/world102.py --stage K out   # for levelcheck: K >= 1 the mines ridden over,
                                                     # the hanging mines crouched under, the
                                                     # caterpillars as floors, the lightning
                                                     # passed between flashes, the saw stopped;
                                                     # K >= 2 the doors open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cobbles import carve, dress, rock  # noqa: E402
from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402
from station import BLITZ, BLITZ_EMITTER, metal, water  # noqa: E402
from village import ROCK, ROCK_FACES  # noqa: E402

RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
TP_MACHINE, TP_PLATES, TP_ROCK, TP_BACK = 330, 331, 332, 333
LAVA = 68
BRIDGE = 364                           # crumbles under Blupi and comes back (Decor::IsBridge)
BAR = 138
SAW, SWITCH = 378, 384                 # the switch on: its saws turn (Decor::ActiveSwitch)
FAN_UP, WIND_UP = 132, 118             # blowing +Y; its wind tiles over it
FAN_WEST, WIND_WEST = 126, 110         # blowing -X; its wind tiles before it


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world102.vwr"
    rng = random.Random(102)
    w = World(mission=102, sky=7)
    ridden = stage is not None and stage >= 1
    opened = stage is not None and stage >= 2

    def teleporter(icon, x, walk, z):
        """A teleporter's pillar over (x, walk, z): standing under it Blupi leaves; arriving from
        its partner he comes out under it, as the original lands him (BD-0017)."""
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
    # The fourth (333): the plate works' way back. The first 333 in the scan (x first, as
    # Decor::SearchTeleporte), so the gallery's sends Blupi here, and this one to the gallery.
    for icon, x, z in ((TP_MACHINE, 39, 74), (TP_ROCK, 39, 82), (TP_PLATES, 61, 74), (TP_BACK, 61, 82)):
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
    # Two columns of lightning across the yard, one before the mines and one after them, as the
    # original's way has (its x 34 and 45): an emitter beam over the lane, the lightning down to
    # the floor. Both strike together (Decor::BlitzActif's one 100-tick cycle): eight cells apart,
    # a board at full speed (~4.6 cells/s) passes both in one quiet half (2.5 s) with ~0.5 s to
    # spare.
    for x in (23, 31):
        w.fill(x, 23, 11, x, 23, 13, BLITZ_EMITTER)
        if not ridden:
            w.fill(x, 20, 11, x, 22, 13, BLITZ)
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

    # ---- the gallery under the plate hall (walk 2) ----
    # East of the pool a shaft goes down from the hall's floor in the updraught of a fan at its
    # foot: it slows his fall to half (BD-0029); landing on the fan's mouth kills him and wrecks
    # it, stepping off the column at the bottom lands him beside it. In the room at its foot, east
    # of the shaft, the switch; west, a passage one wide with a saw in its floor (the switch's
    # row, z 17), then north to a channel along X where a fan's wind blows him west to the
    # teleporter (333) back to the atrium. The original's grey works: the shaft at its x 18, the
    # switch at 16, the saw at 21, the wind channel to the teleporter (333) at 33.
    metal(w, 68, 0, 10, 90, 5, 20, rng)
    metal(w, 85, 5, 16, 87, 9, 18, rng)                                 # the shaft's casing
    carve(w, 84, 2, 15, 89, 4, 19)                                       # the room at its foot
    carve(w, 86, 5, 17, 86, 11, 17)                                      # the shaft
    w.set(86, 1, 17, FAN_UP)
    w.fill(86, 2, 17, 86, 11, 17, WIND_UP)
    w.set(88, 1, 17, SWITCH)
    carve(w, 76, 2, 17, 83, 3, 17)                                       # the saw's passage
    w.set(81, 1, 17, MACHINE_FLOOR if ridden else SAW)
    carve(w, 76, 2, 13, 76, 3, 16)                                       # north to the channel
    carve(w, 70, 2, 12, 88, 3, 12)                                       # the wind channel
    w.set(89, 2, 12, FAN_WEST)
    w.fill(71, 2, 12, 88, 2, 12, WIND_WEST)
    # A bar two over its floor (the original's 138 at its x 26-30 over the wind): hanging from
    # it he is out of the wind and goes hand over hand against it.
    carve(w, 72, 4, 12, 80, 4, 12)
    w.fill(73, 4, 12, 79, 4, 12, BAR)
    teleporter(TP_BACK, 70, 2, 12)

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
    # The red key on a crumbling bridge (364) one wide across the middle pool, lava under it, as
    # the original lays it on its bridge of five over lava (its x 74-78): each cell shakes 16
    # ticks under him and gives way, so he takes the key walking and goes on.
    w.fill(17, 8, 82, 22, 8, 82, LAVA)
    w.fill(17, 9, 82, 22, 9, 82, BRIDGE)
    w.place(O.KEY1, 20, 10, 82)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
