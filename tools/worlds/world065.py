#!/usr/bin/env python3
"""Mission 065, the slime catacombs (worlds3d/world065.vwr): brief plans/briefs/world065.md.

Mobile Eggbert's level 065 is a maze of slime round Blupi's start with one treasure, and the
red arrow far below it in a funnel, reached by a drop through a hole behind the door that wants
that treasure. The treasure lies behind the red door; the red key behind the green door and a
row of saws whose switch is reached from above; the green key in a cage behind a curtain of
lightning, past a gate of saws; the blue key behind a block of nine crates that only the
dynamite, fetched from the top of a tower of saws, blows away. The blue door leads past the big
creature, over stepping stones and a lake to a cavern of balloons; a pair of teleporters, a
garage of jeeps over a row of mines and a lava bridge under a moving mine complete it. Here the
maze is built in 3D round the start hall: the west wing and the treasure pocket, the key room,
the west region over them with the tower, the north corridor to the crates and the cage, the
blue door's way east to the lake, the balloons and the north-east, and the drop.

    python3 tools/worlds/world065.py                 # writes worlds3d/world065.vwr
    python3 tools/worlds/world065.py --stage 1 out   # for levelcheck and flightcheck: the keys'
                                                     # and the treasure's doors open, the saws
                                                     # floors, the crates and the curtain gone, no
                                                     # enemies in the narrow ways, the lake, the
                                                     # balloons and the jeeps' run as floors
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from slime import POLES, SLIME_TRAP, skin, slime  # noqa: E402

LAVA, WATER, DEEP = 68, 92, 91
SAW, SWITCH, BAR, GIRDER, GRATE, BRIDGE = 378, 384, 138, 201, 200, 364
BLITZ, EMITTER = 305, 304
RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
TREASURE_DOOR = 420 + 1
BLUE_BLOCK, MACHINE_WALL, MACHINE_TOP = 4, 11, 22
PROPS = (53, 54, 58, 55, 60)
TELEPORTER = 330


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world065.vwr"
    rng = random.Random(65)
    w = World(mission=65, sky=4)
    solved = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def patrol(otype, start, end, ticks, stop):
        w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def bar(x0, y0, z0, x1, y1, z1, tile):
        """A door filling a doorway; open in the solved stage."""
        carve(x0, y0, z0, x1, y1, z1)
        if not solved:
            w.fill(x0, y0, z0, x1, y1, z1, tile)

    def saw(x, y, z, along_x=True):
        """A saw in the floor (the solved stage: a floor); its blade across the way."""
        w.set(x, y, z, 10 if solved else SAW)
        if along_x:
            w.rotate_plate(x, y, z)

    slime(w, 4, 16, 6, 96, 56, 74)
    slime(w, 60, 2, 50, 84, 16, 74)        # the arrow's funnel far below

    # ---- the start hall (walk 30) ----
    # Spawn (48, 30, 66). The lift up to the west region in the north-west corner; the red door
    # west, the green door north, and east the passage to door "1", one wide, three saws in its
    # floor (the west wing's switch reaches them).
    carve(40, 30, 58, 52, 35, 70)
    carve(41, 29, 58, 42, 29, 59)
    carve(41, 36, 58, 42, 38, 59)
    for x in (41, 42):
        for z in (58, 59):
            lift(w, (x, 29, z), (x, 35, z), ticks=30, stop=3)
    bar(39, 30, 62, 39, 31, 62, RED_DOOR)
    bar(42, 30, 57, 43, 31, 57, GREEN_DOOR)
    carve(53, 30, 62, 56, 31, 62)
    for x in (54, 55, 56):
        saw(x, 29, 62)
    bar(57, 30, 62, 57, 31, 62, TREASURE_DOOR)

    # ---- the west wing, the treasure pocket (walk 30, 22) ----
    # Behind the red door a corridor one wide along X: the switch in its floor at the door, a
    # mine to jump, three saws in its floor; at its end a shaft down to the pocket of the
    # treasure, a stair back up.
    carve(22, 30, 62, 38, 32, 62)
    w.set(36, 29, 62, SWITCH)
    if not solved:
        mine(w, 33, 30, 62)
    for x in (30, 29, 27):
        saw(x, 29, 62)
    carve(22, 22, 62, 22, 29, 62)
    carve(14, 22, 56, 24, 26, 66)
    w.place(O.TREASURE, 15, 22, 58)
    w.place(O.CRATE, 18, 22, 64)
    carve(25, 22, 60, 28, 33, 60)
    for k, x in enumerate((25, 26, 27, 28)):
        slime(w, x, 22, 60, x, 23 + 2 * k, 60)                        # walk 24, 26, 28, 30
    carve(28, 30, 61, 28, 32, 61)

    # ---- the key room (walk 30) ----
    # Behind the green door: a band of saws across the floor under a ceiling too low to jump,
    # the red key beyond. Its switch is in a closet west of the room, in the band's row, reached
    # only by a stair down from the west region.
    carve(36, 30, 48, 46, 32, 56)
    slime(w, 36, 31, 51, 46, 32, 53)
    for x in range(36, 47):
        saw(x, 29, 52)
    w.place(O.KEY1, 38, 30, 49)
    carve(29, 30, 52, 33, 35, 52)                                     # the closet's stair
    slime(w, 29, 30, 52, 29, 33, 52)                                  # walk 34
    slime(w, 30, 30, 52, 30, 31, 52)                                  # walk 32
    w.set(33, 29, 52, SWITCH)

    # ---- the west region (walk 36) ----
    # Over the key room: mines, a moving mine, an egg, teleporter 1 hanging over its floor at the
    # west end; the tower west, the north corridor north, the blue door east.
    carve(18, 36, 38, 50, 41, 57)
    for x0, z0 in ((30, 45), (40, 50), (24, 48)):
        slime(w, x0, 36, z0, x0 + 1, 41, z0 + 1)                      # columns
    if not solved:
        mine(w, 26, 36, 43)
        mine(w, 36, 36, 41)
        patrol(O.MOVING_MINE, (28, 36, 54), (36, 36, 54), 30, 0)
    w.place(O.EGG, 21, 36, 55)
    w.set(20, 37, 40, TELEPORTER)

    # ---- the tower (walk 36 -> 53) ----
    # West of the region, a tower of blue blocks round a core: a stair of steps two apart
    # climbs round inside it. Two steps are saws, each stopped by a switch in its row: the first
    # from a block in the region by the doorway, the third from a block raised on the second. At
    # the top the room of four eggs between striped poles and the shelf of four sticks of
    # dynamite.
    w.fill(6, 35, 40, 16, 55, 50, BLUE_BLOCK)
    carve(7, 36, 41, 15, 54, 49)
    carve(16, 36, 44, 17, 37, 46)
    w.fill(11, 36, 45, 11, 51, 45, BLUE_BLOCK)                        # the core
    steps = [(13, 14, 48, 48, 37), (11, 12, 47, 48, 39), (9, 10, 47, 47, 41), (7, 8, 46, 47, 43),
             (7, 8, 44, 45, 45), (7, 8, 42, 43, 47), (9, 10, 41, 42, 49), (11, 12, 41, 42, 51)]
    for x0, x1, z0, z1, top in steps:
        w.fill(x0, 36, z0, x1, top, z1, BLUE_BLOCK)
    for x0, x1, z0, z1, top in (steps[0], steps[2]):
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                saw(x, top, z)
    w.set(18, 36, 48, BLUE_BLOCK)
    w.set(18, 37, 48, SWITCH)                                         # the first saw step's row
    w.set(12, 40, 47, BLUE_BLOCK)
    w.set(12, 41, 47, SWITCH)                                         # the third's, on the second
    w.fill(7, 52, 41, 15, 52, 49, BLUE_BLOCK)                         # the top floor, walk 53
    carve(9, 52, 41, 12, 52, 42)                                      # the hole over the last steps
    for x in (8, 9, 10, 11):
        w.place(O.EGG, x, 53, 48)
    for x in (7, 12):
        w.fill(x, 53, 48, x, 54, 48, rng.choice(POLES))
    for x, z in ((14, 44), (15, 44), (14, 45), (15, 45)):
        w.place(O.DYNAMITE, x, 53, z)

    # ---- the north corridor (walk 36) ----
    # From the region's north side up a passage and east, one wide: mines to jump, two mines
    # rising and falling through it in shafts, a pit crossed on a lift or hanging from bars; then
    # three wide before a block of nine crates walling in the blue key, two mines moving up and
    # down in front of it.
    carve(44, 36, 30, 45, 38, 37)
    carve(44, 36, 27, 66, 38, 29)
    if not solved:
        for x, z in ((48, 28), (51, 27)):
            mine(w, x, 36, z)
    for x in (54, 58):
        carve(x, 39, 28, x, 42, 28)
        if not solved:
            patrol(O.MOVING_MINE, (x, 36, 28), (x, 42, 28), 30, 0)
    carve(61, 32, 27, 63, 35, 29)                                     # the pit
    w.fill(61, 36, 28, 63, 36, 28, BAR)
    if solved:
        w.fill(61, 35, 27, 63, 35, 27, 10)
    else:
        lift(w, (61, 35, 27), (63, 35, 27), ticks=30, stop=10)
    carve(64, 36, 26, 66, 38, 30)
    if not solved:
        patrol(O.MOVING_MINE, (65, 36, 26), (65, 38, 26), 20, 0)
        patrol(O.MOVING_MINE, (65, 38, 30), (65, 36, 30), 20, 0)
    carve(67, 36, 26, 69, 36, 28)                                     # one high: no climbing over
    if not solved:
        for x in (67, 68, 69):
            for z in (26, 27, 28):
                w.place(O.CRATE, x, 36, z)
    carve(70, 36, 26, 71, 37, 28)
    w.place(O.KEY3, 71, 36, 27)

    # ---- the saw gate and the lightning cage (walk 36, 40) ----
    # North of the crates' front a passage to a corridor along X: the switch in its floor at the
    # mouth, three saws in its floor; at its west end steps up into the cage: walls of machine
    # tiles, a curtain of lightning in its doorway, the green key and an egg.
    carve(65, 36, 22, 65, 38, 25)
    carve(55, 36, 22, 65, 37, 22)
    w.set(64, 35, 22, SWITCH)
    for x in (59, 60, 61):
        saw(x, 35, 22)
    carve(55, 38, 22, 56, 39, 22)                                     # room to jump
    carve(55, 36, 21, 56, 41, 21)
    slime(w, 55, 36, 21, 56, 37, 21)                                  # a step, walk 38
    carve(55, 40, 20, 56, 42, 20)                                     # the landing, walk 40
    carve(48, 40, 14, 57, 43, 19)
    w.fill(47, 40, 13, 58, 44, 13, MACHINE_WALL)
    w.fill(47, 44, 13, 58, 44, 19, MACHINE_TOP)
    for x, z in ((49, 15), (52, 17), (51, 14)):
        w.set(x, 40, z, PROPS[(x + z) % len(PROPS)])
    if not solved:
        w.fill(55, 40, 19, 56, 42, 19, BLITZ)
    w.fill(55, 43, 19, 56, 43, 19, EMITTER)
    w.place(O.KEY2, 49, 40, 17)
    w.place(O.EGG, 50, 40, 14)

    # ---- the blue door's way (walk 36) ----
    # East of the region: the big creature's corridor (it rests at both ends, harmless walking);
    # stepping stones over a lava trench, the switch on the second, three saws among the rest;
    # a passage down to the lake.
    bar(51, 36, 44, 51, 37, 45, BLUE_DOOR)
    carve(52, 36, 44, 62, 37, 44)
    if not solved:
        patrol(O.BIG_CREATURE, (53, 36, 44), (61, 36, 44), 80, 200)
    carve(63, 33, 43, 75, 38, 45)
    w.fill(63, 33, 43, 75, 33, 45, LAVA)
    for x in range(63, 76, 2):
        slime(w, x, 33, 44, x, 35, 44)
    w.set(65, 35, 44, SWITCH)
    for x in (67, 69, 71):
        saw(x, 35, 44)
    carve(76, 36, 44, 78, 38, 44)
    carve(78, 30, 42, 80, 35, 44)
    for k, z in enumerate((44, 43, 42)):
        slime(w, 79, 30, z, 80, 34 - 2 * k, z)                        # down to walk 31

    # ---- the lake and the cavern of balloons (walk 30) ----
    # A lake under a low vault, its surface at 30: from the stair's foot Blupi swims north, past
    # an air pocket with an egg and a saw on its floor, to the cavern's shore. In the cavern four
    # balloons, and a shaft in its vault up to the north-east.
    carve(76, 22, 30, 88, 32, 41)
    w.fill(76, 22, 30, 88, 29, 41, DEEP)
    w.fill(76, 30, 30, 88, 30, 41, WATER)
    slime(w, 80, 22, 34, 82, 30, 36)                                  # the pocket's floor, walk 31
    carve(80, 31, 34, 82, 32, 36)
    w.place(O.EGG, 80, 31, 34)
    saw(82, 30, 36, False)
    if solved:
        w.fill(77, 30, 30, 79, 30, 41, 10)                            # a causeway for the checks
        w.fill(77, 30, 30, 85, 30, 31, 10)
    carve(84, 30, 18, 94, 36, 29)
    for x in (86, 88, 90, 92):
        w.place(O.BALLOON, x, 30, 26)
    carve(90, 37, 20, 91, 45, 21)
    if solved:                                                        # the balloons as a stair
        carve(93, 30, 20, 94, 48, 27)
        for k in range(8):
            slime(w, 93, 30, 27 - k, 94, 31 + 2 * k, 27 - k)

    # ---- the north-east (walk 46) ----
    # Teleporter 2 hanging over the floor by two crates and a mine; bars over a gap; the garage
    # of four jeeps on grates, two lifts down from it to the corridor of the mine row (walk 42):
    # a jeep runs over the seven mines to the niche of three eggs; lava beside.
    carve(62, 46, 8, 94, 50, 24)
    w.set(70, 47, 10, TELEPORTER)
    for x, z in ((66, 12), (68, 16)):
        w.place(O.CRATE, x, 46, z)
    if not solved:
        mine(w, 75, 46, 14)
    carve(76, 42, 18, 78, 45, 22)                                     # the gap
    w.fill(76, 46, 20, 78, 46, 20, BAR)
    for x, z in ((84, 10), (86, 10), (88, 10), (90, 10)):
        w.set(x, 45, z, GRATE)
        w.place(O.JEEP, x, 46, z)
    carve(84, 42, 12, 85, 45, 13)
    for x in (84, 85):
        for z in (12, 13):
            lift(w, (x, 45, z), (x, 41, z), ticks=40, stop=10)
    carve(70, 42, 12, 83, 43, 12)                                     # one wide: only a jeep
    if not solved:
        for x in range(73, 80):
            mine(w, x, 42, 12)
    carve(68, 38, 12, 69, 43, 13)                                     # the eggs' niche, walk 38
    slime(w, 69, 38, 12, 69, 39, 13)                                  # a step back up, walk 40
    for z in (12, 13):
        w.place(O.EGG, 68, 38, z)
    w.place(O.EGG, 69, 40, 13)
    w.place(O.CRATE, 64, 46, 20)
    w.fill(86, 41, 14, 90, 41, 17, LAVA)
    carve(86, 42, 14, 90, 43, 17)

    # ---- door "1", the lava bridges, the drop (walk 30 -> 5) ----
    # Behind the treasure's door: lava over the floor but two lanes of crumbling bridges, a
    # moving mine sweeping one of them; on the far side a hole down to the funnel: the arrow on a
    # pedestal, two eggs at its foot.
    carve(58, 30, 58, 74, 34, 66)
    w.fill(59, 29, 58, 70, 29, 66, LAVA)
    w.fill(59, 29, 62, 70, 29, 63, 10 if solved else BRIDGE)
    if not solved:
        patrol(O.MOVING_MINE, (60, 30, 62), (69, 30, 62), 30, 0)
    carve(72, 5, 62, 73, 29, 63)
    carve(64, 5, 54, 80, 10, 70)
    for k in range(3):                                                # the funnel's slopes
        slime(w, 64 + k, 5, 54, 64 + k, 7 - k, 70)
        slime(w, 80 - k, 5, 54, 80 - k, 7 - k, 70)
    slime(w, 69, 5, 62, 69, 6, 62)
    w.place(O.EXIT, 69, 7, 62)
    w.place(O.EGG, 74, 5, 58)
    w.place(O.EGG, 74, 5, 66)

    skin(w, rng)
    w.spawn = (48, 30, 66)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
