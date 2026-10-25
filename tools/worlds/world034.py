#!/usr/bin/env python3
"""Mission 034, the teleporter station and world 3's golden key (worlds3d/world034.vwr): brief
plans/briefs/world034.md.

Mobile Eggbert's level 034: Blupi starts on the golden key in front of a house, between two
teleporters (330 alpha, 332 delta); the key counts only with all five treasures, which lie in
four places the teleporters join: a rock cave (the junction, with teleporters to the other two
places, a sliding lift over a gap, a temporary block, the blue key and a long lift up into the
machine hall), a machine hall (a fan, a bulldozer among mines, invert pickups, lava, a fish in
a pool before the blue door and its treasure), a toy room (a lift among mines, a lollipop for
the high jump to the treasure) and brick ruins under the sky (six birds over the roofs, a drink
to pass them unseen, a lift up a tower to the treasure). Here the five places are built apart
in 3D and joined the same way.

    python3 tools/worlds/world034.py                  # writes worlds3d/world034.vwr
    python3 tools/worlds/world034.py --check STAGE out
        STAGE timed:  the timed enemies (bulldozer, fish, birds, moving mine) left out, the
                      temporary blocks solid, as when passed at the right moment
        STAGE power:  also a stair where the lollipop's high jump goes
        STAGE solved: also the blue door open (the blue key used)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine, wall  # noqa: E402
from village import (BRICK, BRICK_FACES, GRASS, PLANTS, ROCK, ROCK_FACES, TOY_FACES, TREES,  # noqa: E402
                     bird, grass_top, house, patrol, stone)

MACHINE = (78, 79, 80, 81, 82, 83, 84)
ALPHA, BETA, DELTA, CHI = 330, 331, 332, 333       # teleporter pairs (their panels' letters)
FAN_LEFT, WIND_LEFT = 126, 110                     # the fan blows -X; its wind tiles before it
LAVA, WATER, TEMP, BLUE_DOOR = 68, 92, 324, 336
WS, WR, WL, WM, WG, WT, WB = 30, 14, 8, 28, 34, 6, 36   # walking levels of the places
SPAWN = (48, WS, 84)


def carve(w, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def mass(w, box, rng, block, faces, chance=0.35):
    wall(w, *box, rng, faces=faces, face_chance=chance, block=block)


def pillar(w, icon, x, y, z, mount=None, mount_top=None):
    """A teleporter pillar hanging one cell above the walking level y - 1 (Blupi standing under
    it goes), with an optional column of `mount` up to `mount_top`."""
    w.set(x, y, z, icon)
    if mount is not None:
        for yy in range(y + 1, (mount_top if mount_top is not None else y + 1) + 1):
            w.set(x, yy, z, mount)


def start(w: World, rng: random.Random):
    """S: the courtyard before the house, the golden key, teleporters alpha and delta."""
    stone(w, 38, 24, 66, 58, WS - 1, 90, rng)
    grass_top(w, 38, 74, 58, 90, WS - 1)
    house(w, 38, 66, 58, 73, WS, 7, rng, door=48)
    for z in range(74, 91):
        w.set(38, WS, z, 156)
        w.set(58, WS, z, 156)
    for x in range(38, 59):
        w.set(x, WS, 90, 156)
    w.place(O.SECRET_EXIT, 48, WS, 78)
    pillar(w, ALPHA, 44, WS + 1, 79, mount=35)
    pillar(w, DELTA, 52, WS + 1, 79, mount=35)
    for icon, x, z in ((TREES[0], 40, 86), (TREES[2], 56, 85), (TREES[1], 41, 76)):
        w.bigdecor(icon, x, WS, z)


def rock_cave(w: World, rng: random.Random, timed: bool):
    """R: the junction cave. The arrival hall (walk 14) with delta, beta and chi; a drop
    into the lower cave (walk 8): the temporary bridge over lava to the blue key, the lava
    pit crossed on a sliding lift to the treasure, a stair back up; the long lift up into the
    machine hall."""
    mass(w, (14, 0, 30, 46, 22, 62), rng, ROCK, ROCK_FACES)
    carve(w, 20, WR, 49, 44, WR + 5, 58)                   # the arrival hall
    for icon, x, z in ((BETA, 26, 52), (DELTA, 30, 53), (CHI, 34, 52)):
        pillar(w, icon, x, WR + 1, z, mount=ROCK, mount_top=WR + 5)
    w.bigdecor(PLANTS[1], 22, WR, 57)
    # The lower cave, x 15-38, z 32-47: west ledge, lava chasm, middle floor, lava pit, east floor.
    carve(w, 15, WL, 32, 38, WL + 5, 47)
    carve(w, 20, WL, 48, 23, WR + 5, 48)                   # the drop from the hall
    for x0, x1 in ((17, 19), (24, 29)):                   # the chasm and the pit, lava below
        carve(w, x0, 3, 32, x1, WL - 1, 47)
        w.fill(x0, 3, 32, x1, 3, 47, LAVA)
    # The temporary bridge over the chasm: solid nine tenths of the time.
    w.fill(17, WL - 1, 39, 19, WL - 1, 39, ROCK if timed else TEMP)
    w.place(O.KEY3, 15, WL, 39)
    mine(w, 16, WL, 33)
    w.place(O.EGG, 21, WL, 33)
    for z in (38, 39, 40):                                 # the sliding lift, fast
        lift(w, (24, WL - 1, z), (29, WL - 1, z), ticks=36, stop=3)
    w.place(O.TREASURE, 36, WL, 36)
    w.bigdecor(PLANTS[2], 37, WL, 46)
    # The stair back up to the hall: steps of two, through an opening in its wall.
    mass(w, (33, WL, 45, 35, WL + 1, 46), rng, ROCK, ROCK_FACES)
    mass(w, (33, WL, 47, 35, WL + 3, 47), rng, ROCK, ROCK_FACES)
    carve(w, 33, WR, 47, 35, WR + 2, 48)
    # The long lift up to the machine hall (four side by side, slow), in a shaft from the
    # hall's east end.
    carve(w, 39, WR, 52, 44, 26, 54)
    carve(w, 40, WR - 1, 53, 43, WR - 1, 53)
    for x in range(40, 44):
        lift(w, (x, WR - 1, 53), (x, WM - 1, 53), ticks=180, stop=10)


def machine_hall(w: World, rng: random.Random, timed: bool, solved: bool):
    """M: the gallery (walk 34) where alpha arrives, its fan, a treasure and two eggs; the hall
    below (walk 28): the long lift's top, a tunnel where the bulldozer drives among mines,
    lava with invert pickups on the way to an egg, the pool with the fish before the blue
    door and its treasure."""
    mass(w, (8, 23, 28, 46, 44, 62), rng, MACHINE[0], MACHINE[1:], chance=0.7)
    carve(w, 38, WM, 37, 44, WM + 12, 60)                  # the hall's east part
    carve(w, 14, WM, 37, 29, WM + 12, 60)                  # ...its west part
    carve(w, 14, WG, 30, 44, WG + 6, 35)                   # the gallery
    carve(w, 40, WG, 36, 44, WG + 6, 36)                   # its window down to the east part
    carve(w, 40, 23, 52, 43, WM - 1, 54)                   # the long lift's shaft top
    pillar(w, ALPHA, 18, WG + 1, 32, mount=MACHINE[2], mount_top=WG + 6)
    w.place(O.EGG, 16, WG, 31)
    w.place(O.EGG, 22, WG, 31)
    # The fan in the gallery's east wall, blowing -X along z 33; only its mouth kills.
    w.set(45, WG, 33, FAN_LEFT)
    for x in range(37, 45):
        w.set(x, WG, 33, WIND_LEFT)
    w.place(O.TREASURE, 44, WG, 31)
    bird(w, (42, WM + 1, 38), (42, WM + 6, 38), ticks=32)   # rising and falling under the window
    # The tunnel between the hall's two parts: the bulldozer drives its middle lane, mines in
    # the side lanes, a mine hanging a jump above.
    carve(w, 30, WM, 44, 37, WM + 2, 46)
    mine(w, 31, WM, 44)
    mine(w, 36, WM, 46)
    mine(w, 34, WM + 2, 45, hanging=True)
    if not timed:
        patrol(w, O.BULLDOZER, (30, WM, 45), (37, WM, 45), ticks=210, stop=20)
    # West part, north: lava, a walkway into it with two invert pickups and an egg at its end.
    w.fill(14, WM, 37, 29, WM, 42, LAVA)
    carve(w, 20, WM, 38, 22, WM, 42)
    for z in (41, 39):
        w.place(O.INVERT, 21, WM, z)
    w.place(O.EGG, 21, WM, 38)
    # West part, south: the pool across it, the fish sweeping before the blue door's ledge.
    w.fill(15, WM - 2, 47, 26, WM - 1, 60, WATER)
    if not timed:
        patrol(w, O.FISH, (16, WM - 1, 47), (16, WM - 1, 60), ticks=45, stop=47)
    for y in (WM, WM + 1):
        w.set(13, y, 53, 0 if solved else BLUE_DOOR)
    carve(w, 9, WM, 51, 12, WM + 3, 55)                    # the treasure room behind it
    w.place(O.TREASURE, 10, WM, 53)
    if not timed:
        patrol(w, O.MOVING_MINE, (11, WM, 53), (11, WM + 3, 53), ticks=20, stop=0)


def toy_room(w: World, rng: random.Random, timed: bool, power: bool):
    """T: beta arrives on the floor (walk 6); a lift three wide among mines up to the balcony
    (walk 12) and its lollipop; the high jump up to the shelf (walk 18) with the treasure."""
    mass(w, (58, 0, 28, 86, 26, 58), rng, TOY_FACES[0], TOY_FACES[1:], chance=0.6)
    carve(w, 60, WT, 30, 84, 22, 56)
    w.fill(60, WT - 1, 30, 84, WT - 1, 56, 231)            # a floor of stars
    pillar(w, BETA, 64, WT + 1, 32, mount=TOY_FACES[3], mount_top=22)
    mass(w, (60, WT, 48, 84, WT + 5, 56), rng, TOY_FACES[1], TOY_FACES, chance=0.5)   # the balcony
    mass(w, (60, WT + 6, 48, 66, WT + 11, 56), rng, TOY_FACES[2], TOY_FACES, chance=0.5)  # the shelf
    if power:
        mass(w, (67, WT + 6, 50, 67, WT + 7, 50), rng, TOY_FACES[2], TOY_FACES)
        mass(w, (67, WT + 6, 49, 67, WT + 9, 49), rng, TOY_FACES[2], TOY_FACES)
    carve(w, 70, WT - 1, 47, 72, WT - 1, 47)
    for x in (70, 71, 72):
        lift(w, (x, WT - 1, 47), (x, WT + 5, 47), ticks=60, stop=10)
    mine(w, 69, WT, 46)
    mine(w, 73, WT, 46)
    mine(w, 71, WT + 3, 45, hanging=True)
    w.place(O.LOLLIPOP, 80, WT + 6, 52)
    w.place(O.TREASURE, 62, WT + 12, 52)
    w.place(O.EGG, 64, WT + 12, 54)
    for x, z in ((76, 36), (80, 40), (67, 42)):            # striped poles (passable decor)
        for y in range(WT, WT + 4):
            w.set(x, y, z, 235 if (x + y) % 2 else 236)


def brick_ruins(w: World, rng: random.Random, timed: bool):
    """B: under the sky; chi arrives in a brick courtyard (walk 36) with the drink; three
    roofs stepping north with two birds sweeping each; a lift up the tower to the treasure."""
    mass(w, (18, 26, 2, 34, WB - 1, 26), rng, BRICK, BRICK_FACES)
    for z in range(18, 27):                                # the courtyard's parapet
        w.set(18, WB, z, BRICK)
        w.set(34, WB, z, BRICK)
    for x in range(18, 35):
        w.set(x, WB, 26, BRICK)
    # The gate where chi hangs.
    pillar(w, CHI, 24, WB + 1, 22)
    for x in (23, 25):
        mass(w, (x, WB, 22, x, WB + 2, 22), rng, BRICK, BRICK_FACES)
    w.fill(23, WB + 2, 22, 25, WB + 2, 22, BRICK)
    w.place(O.DRINK, 28, WB, 22)
    w.bigdecor(PLANTS[0], 32, WB, 24)
    # Three houses stepping up north, birds at Blupi's height sweeping each roof.
    for (z0, z1, h) in ((14, 17, 2), (10, 13, 4), (6, 9, 6)):
        house(w, 20, z0, 32, z1, WB, h, rng)
        if not timed:
            top = WB + h
            bird(w, (20, top, z0 + 1), (32, top, z0 + 1), ticks=32)
            bird(w, (32, top, z1 - 1), (20, top, z1 - 1), ticks=24)
    # The tower and its lift in the south face, the treasure on top.
    roof = WB + 6
    mass(w, (20, WB, 2, 32, roof + 7, 5), rng, BRICK, BRICK_FACES)
    for y in range(roof - 1, roof + 9):
        w.clear(26, y, 5)
    lift(w, (26, roof - 1, 5), (26, roof + 7, 5), ticks=48, stop=3)
    w.place(O.TREASURE, 26, roof + 8, 3)


def main(argv) -> int:
    stage = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    timed = stage in ("timed", "power", "solved")
    power = stage in ("power", "solved")
    solved = stage == "solved"
    rng = random.Random(34)
    w = World(mission=34, sky=19)
    start(w, rng)
    rock_cave(w, rng, timed)
    machine_hall(w, rng, timed, solved)
    toy_room(w, rng, timed, power)
    brick_ruins(w, rng, timed)
    w.spawn = SPAWN
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world034.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
