#!/usr/bin/env python3
"""Mission 023, the tower of keys (worlds3d/world023.vwr): brief plans/briefs/world023.md.

Mobile Eggbert's level 023 is the lesson of keys and switches. From a white room two lifts go,
one down into a rock cave where the red, green and blue keys lie (the blue one high up, reached
from the cave's crate), one up to a vault behind the blue door holding a second red and green
key; red and green doors in a row lead on, a lift carries Blupi over a row of saws, ball blocks
on pillars climb to the switches that stop the saws, two more blue keys open two blue doors, and
at the top the red arrow stands between two basins. A key opens one door and is used up. Here
the same order in a white tower: the hall and its two lifts, the cave below, the corridor of the
red, green and blue doors with the vault, the lift over the saw pit, a red door to a spiral of
ball blocks, a green door to the machine terrace, saws stopped from their switches (each reaches
its own row along X), the blue keys one after the other before their doors, the arrow's room.
No two keys of one colour are ever held at once, since a door takes the colour's key away.

    python3 tools/worlds/world023.py                   # writes worlds3d/world023.vwr
    python3 tools/worlds/world023.py --stage K out.vwr # lessons 1..K solved (levelcheck):
        1 crate at the cave ledge, 2 corridor doors open, 3 the red door to the blocks,
        4 the green door, 5 switches pressed (saws stopped), 6 the blue doors open
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift  # noqa: E402
from palace import (BLUE_DOOR, FOUNTAIN, GREEN_DOOR, MACHINE, RED_DOOR, ROCK, ROCK_FACES, SAW,  # noqa: E402
                    SHELVES, STONE, SWITCH, TOYS, gate, parapet, stone, windows_row)

PILLAR = 76
OZ = 8                      # every z of the plan below shifted by this
SPAWN = (48, 11, 56 + OZ)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else 0
    rng = random.Random(23)
    w = World(mission=23, sky=28)

    def z(v):
        return v + OZ

    def door(x, y, zz, tile, k):
        if stage < k:
            gate(w, x, y, z(zz), tile)

    def saw(x, y, zz, k):
        """A saw in the floor; once its switch is pressed (stage k) the floor stands safe."""
        w.set(x, y, z(zz), SAW if stage < k else STONE)

    def rock(x0, y0, z0, x1, y1, z1, chance=0.3):
        stone(w, x0, y0, z(z0), x1, y1, z(z1), rng, chance=chance, block=ROCK, faces=ROCK_FACES)

    def white(x0, y0, z0, x1, y1, z1, chance=0.2):
        stone(w, x0, y0, z(z0), x1, y1, z(z1), rng, chance=chance)

    # The cave (walk 4): rock under the hall, the down-lift's foot, the red and green keys on
    #    its floor, the blue key and a treasure on a ledge three high, the crate to climb it.
    rock(35, 2, 43, 61, 3, 63)
    rock(35, 4, 43, 35, 9, 63)
    rock(61, 4, 43, 61, 9, 63)
    rock(35, 4, 43, 61, 9, 43)
    rock(35, 4, 63, 61, 9, 63)
    rock(35, 10, 43, 61, 10, 63, chance=0.2)
    rock(54, 4, 56, 60, 6, 62)                                       # the ledge, walk 7
    for x, zz in ((40, 48), (48, 61), (55, 47)):
        rock(x, 4, zz, x, 9, zz, chance=0.5)                          # rock columns
    w.place(O.KEY1, 37, 4, z(61))
    w.place(O.KEY2, 59, 4, z(45))
    w.place(O.KEY3, 58, 7, z(60))
    w.place(O.TREASURE, 55, 7, z(61))
    w.place(O.EGG, 37, 4, z(45))
    w.place(O.CRATE, *((53, 4, z(58)) if stage >= 1 else (50, 4, z(58))))   # pushed +X

    # The hall (walk 11), its walls seven high; the lift down to the cave and the lift up to
    #    the landing on its north wall.
    white(40, 10, 46, 56, 10, 58, chance=0.1)
    white(39, 11, 45, 57, 17, 45)
    white(39, 11, 59, 57, 17, 59)
    white(39, 11, 45, 39, 17, 59)
    white(57, 11, 45, 57, 17, 59)
    w.fill(40, 15, z(45), 56, 17, z(45), 0)                           # the north wall stops at the landing
    windows_row(w, 40, 56, 13, z(59), rng, chance=0.5)
    w.clear(44, 10, z(52))
    w.clear(44, 3, z(52))
    lift(w, (44, 10, z(52)), (44, 3, z(52)), ticks=70, stop=10)       # 10 ticks a cell, as the original's
    w.clear(52, 10, z(46))
    lift(w, (52, 10, z(46)), (52, 14, z(46)), ticks=40, stop=10)
    w.place(O.EGG, 55, 11, z(57))

    # The landing (walk 15) and the corridor west, open on top: the red door, the green door,
    #    the blue door of the vault (a second red and green key), out north to the saw pit.
    white(47, 14, 38, 56, 14, 45, chance=0.1)
    parapet(w, [(x, z(38)) for x in range(47, 57)] + [(56, z(zz)) for zz in range(38, 45)], 15, rng)
    white(32, 14, 39, 46, 14, 43, chance=0.1)
    white(32, 15, 39, 46, 17, 39)
    white(32, 15, 43, 46, 17, 43)
    white(32, 15, 39, 32, 17, 43)
    for x in (44, 40, 36):
        white(x, 15, 40, x, 17, 42)
        w.clear(x, 15, z(41))
    door(44, 15, 41, RED_DOOR, 2)
    door(40, 15, 41, GREEN_DOOR, 2)
    door(36, 15, 41, BLUE_DOOR, 2)
    w.place(O.EGG, 42, 15, z(42))
    w.place(O.KEY1, 33, 15, z(40))
    w.place(O.KEY2, 33, 15, z(42))
    w.clear(34, 15, z(39))                                            # out of the vault, north

    # The saw pit: a walkway three wide (x 33-35, walk 15) north from the vault; across it a
    #    pit of saws five long, crossed on three lifts side by side (40 ticks).
    white(33, 14, 26, 35, 14, 38, chance=0.1)
    for x in (33, 34, 35):
        for zz in range(30, 35):
            w.clear(x, 14, z(zz))
            w.set(x, 13, z(zz), SAW)
        lift(w, (x, 14, z(34)), (x, 14, z(30)), ticks=40, stop=10)
    # The red door at the walkway's end, then the spiral of ball blocks on pillars, each two
    #    higher than the last: walk 17, 19, 21, 23, then 25 before the green door.
    white(32, 15, 27, 36, 17, 27)
    w.clear(34, 15, z(27))
    door(34, 15, 27, RED_DOOR, 3)
    blocks = [((33, 34), (24, 25), 16), ((31, 32), (24, 25), 18), ((31, 32), (22, 23), 20),
              ((33, 34), (22, 23), 22), ((33, 34), (20, 21), 24)]
    for (xa, xb), (za, zb), top in blocks:
        for x in (xa, xb):
            for zz in (za, zb):
                for y in range(14, top):
                    w.set(x, y, z(zz), PILLAR)
                w.set(x, top, z(zz), rng.choice(TOYS))
    w.place(O.EGG, 31, 19, z(25))

    # The machine terrace (walk 25) behind the green door, walls round it four high.
    for x in range(29, 46):
        for zz in range(9, 20):
            w.set(x, 24, z(zz), MACHINE if rng.random() < 0.3 else STONE)
    white(29, 25, 19, 45, 28, 19)
    w.clear(33, 25, z(19))
    door(33, 25, 19, GREEN_DOOR, 4)                                   # from the last ball block
    white(29, 25, 9, 45, 28, 9)
    white(45, 25, 9, 45, 28, 19)
    for x, zz in ((31, 11), (43, 11), (43, 17)):
        w.set(x, 25, z(zz), MACHINE)
        w.set(x, 26, z(zz), PILLAR)
        w.set(x, 27, z(zz), MACHINE)
    # West through the terrace's wall: a catwalk one wide along X with five saws, its switch
    #    at its start; the first blue key on the platform at its end.
    w.clear(29, 25, z(14))
    w.clear(29, 26, z(14))
    for x in range(21, 29):
        w.set(x, 24, z(14), STONE)
    w.set(28, 24, z(14), SWITCH)
    for x in range(22, 27):
        saw(x, 24, 14, 5)
    white(18, 24, 12, 20, 24, 16, chance=0.1)
    w.place(O.KEY3, 19, 25, z(14))
    # East through the terrace's wall: the toy room, a passage one wide between shelves, its
    #    switch in the doorway, five saws in its floor, the second treasure at its end.
    white(46, 24, 15, 53, 24, 17, chance=0.1)
    for x in range(46, 54):
        for y in range(25, 28):
            w.set(x, y, z(15), rng.choice(SHELVES))
            w.set(x, y, z(17), rng.choice(SHELVES))
    for y in range(25, 28):
        w.set(53, y, z(16), rng.choice(SHELVES))
    w.clear(45, 25, z(16))
    w.clear(45, 26, z(16))
    w.set(45, 24, z(16), SWITCH)                                     # its own row (z 16): the
    for x in range(47, 52):                                          # catwalk's is z 14
        saw(x, 24, 16, 5)
    w.place(O.TREASURE, 52, 25, z(16))
    w.place(O.EGG, 46, 25, z(16))

    # North: the first blue door onto a landing over the void; west of it a bridge one wide
    #    along X with four saws, its switch at its start, the second blue key on the island at
    #    its end; north a plain bridge to the second blue door and the arrow's room.
    w.clear(37, 25, z(9))
    door(37, 25, 9, BLUE_DOOR, 6)
    white(36, 24, 7, 38, 24, 9, chance=0.1)                           # the landing
    for x in range(29, 36):
        w.set(x, 24, z(7), STONE)
    w.set(35, 24, z(7), SWITCH)
    for x in range(31, 35):
        saw(x, 24, 7, 5)
    white(26, 24, 6, 29, 24, 8, chance=0.1)                           # the island
    w.place(O.KEY3, 27, 25, z(7))
    for zz in (4, 5, 6):
        w.set(37, 24, z(zz), STONE)                                   # the bridge north
    white(30, 24, 0, 44, 24, 3, chance=0.1)
    white(30, 25, 0, 44, 28, 3)                                       # the arrow's room
    w.clear(37, 25, z(3))
    door(37, 25, 3, BLUE_DOOR, 6)
    # The arrow's room (x 31-43, z 1-2), open on top: the arrow between two basins with eggs.
    w.fill(31, 25, z(1), 43, 28, z(2), 0)
    for x in (35, 39):
        w.set(x, 25, z(1), STONE)
        w.bigdecor(FOUNTAIN, x, 26, z(1))
        w.place(O.EGG, x, 26, z(2))
    w.place(O.EXIT, 37, 25, z(1))

    w.spawn = SPAWN
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world023.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
