#!/usr/bin/env python3
"""Mission 011, the first lesson (worlds3d/world011.vwr): brief plans/briefs/world011.md.

Mobile Eggbert's level 011 is a corridor teaching, left to right: move, jump for a treasure on a
pillar, jump mine pits, water, two elevators, hanging mines, three moving platforms, the upper
path with two eggs, a pair of lifts that meet at the top, the second treasure and the red arrow.
Here the same lessons, in the same order and with the same hints (Tables::table_training1, as
hint zones), run as a square spiral of machine walkways round a central machine tower: east
walkway heading away from the camera, north, west, south, then over the moat on the paired
lifts to the tower's terrace and up its elevator to the red arrow on top. Falling off a
walkway is a fall to death, as the original's mine pits were.

    python3 tools/worlds/world011.py         # writes worlds3d/world011.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import (MACHINE_FACES, MACHINE_FLOOR, MACHINE_PROPS, MONITOR, O, World, boundary,  # noqa: E402
                 lift, mine, props, rect, wall)

TOP = 9            # walkway top: Blupi walks at y 10
WALK = TOP + 1
TERRACE = 11       # the tower's terrace top: Blupi walks at y 12
TOWER = 17         # the tower's roof: the red arrow stands at y 18

# Walkways, 5 cells wide with a parapet on each side (raw x, z ranges, inclusive).
START = rect(61, 57, 67, 62)
EAST = rect(62, 38, 66, 56)          # A, heading -Z from the spawn
CORNER_NE = rect(61, 31, 67, 37)
NORTH = rect(38, 32, 60, 36)         # B, heading -X
CORNER_NW = rect(31, 31, 37, 37)
WEST = rect(32, 38, 36, 62)          # C, heading +Z
CORNER_SW = rect(31, 63, 37, 69)
SOUTH = rect(38, 64, 52, 68)         # D, heading +X; its lower lane is the trap
RING = START + EAST + CORNER_NE + NORTH + CORNER_NW + WEST + CORNER_SW + SOUTH

SPAWN = (64, WALK, 60)


def carve(w, cells, y0, y1):
    for x, z in cells:
        for y in range(y0, y1 + 1):
            w.clear(x, y, z)


def build_ring(w: World, rng: random.Random, no_rail: set):
    for x, z in RING:
        for y in range(TOP - 2, TOP + 1):
            w.set(x, y, z, rng.choice(MACHINE_FACES) if y < TOP and rng.random() < 0.3 else MACHINE_FLOOR)
    for x, z in boundary(RING):
        if (x, z) not in no_rail:
            w.set(x, WALK, z, rng.choice(MACHINE_FACES) if rng.random() < 0.25 else MACHINE_FLOOR)
    # Machine columns carry the walkways down into the dark.
    for cx, cz in ((64, 34), (34, 34), (34, 66), (64, 60), (64, 46), (49, 34), (34, 50), (45, 66)):
        wall(w, cx - 1, 0, cz - 1, cx + 1, TOP - 3, cz + 1, rng, face_chance=0.35)


def east(w: World, rng: random.Random):
    """A: move; the treasure on its pillar (jump); two mine pits; the water."""
    lane = lambda z: [(x, z) for x in (63, 64, 65)]  # noqa: E731
    w.fill(65, WALK, 54, 65, WALK + 1, 54, MACHINE_FACES[1])          # the pillar, 2 high
    w.place(O.TREASURE, 65, WALK + 2, 54)
    w.set(66, WALK, 53, MONITOR)                                       # the machine beside it
    w.set(66, WALK + 1, 53, MONITOR)
    for z in (51, 47):                                                 # mine pits, one deep
        carve(w, lane(z), TOP, TOP)
        for x, _ in lane(z):
            mine(w, x, TOP, z)
    carve(w, lane(43) + lane(42), TOP, TOP)                            # the water channel
    for x, z in lane(43) + lane(42):
        w.set(x, TOP, z, 92)
    w.hint_zone(0, 61, WALK - 1, 57, 67, WALK + 3, 62)
    w.hint_zone(1, 62, WALK - 1, 53, 66, WALK + 3, 56)
    w.hint_zone(2, 62, WALK - 1, 50, 66, WALK + 3, 52)
    w.hint_zone(3, 62, WALK - 1, 46, 66, WALK + 3, 49)
    w.hint_zone(4, 62, WALK - 2, 41, 66, WALK + 3, 45)


def north(w: World, rng: random.Random):
    """B: the elevator taken quietly; the elevator jumped onto; under the hanging mines."""
    # Elevator 1 rises from the walkway (x 58) to the top of wall 1 (x 56-57, 3 high).
    carve(w, [(58, 34)], TOP, TOP)
    lift(w, (58, TOP, 34), (58, TOP + 3, 34), ticks=30, stop=10)
    wall(w, 56, WALK, 32, 57, WALK + 2, 36, rng)
    # Behind it a trough one lower; elevator 2 starts a step above its floor and rises to
    # one below the top of wall 2 (x 51-52): jump on, ride, jump off.
    carve(w, rect(54, 32, 55, 36), TOP, TOP)
    carve(w, [(53, 34)], TOP, TOP)
    lift(w, (53, TOP, 34), (53, TOP + 2, 34), ticks=20, stop=10)
    wall(w, 51, WALK, 32, 52, WALK + 2, 36, rng)
    # The gantry: a beam three above the walkway on four corner posts, and mines hanging
    # under it over all three lanes -- a jump meets them. Open at the sides, so the camera
    # keeps its view; the parapets under it are gone, so none of them is a way round.
    for x in (44, 48):
        for z in (32, 36):
            wall(w, x, WALK, z, x, WALK + 2, z, rng, face_chance=0.3)
    for x in range(44, 49):
        for z in range(32, 37):
            w.set(x, WALK + 3, z, rng.choice(MACHINE_FACES) if rng.random() < 0.4 else MACHINE_FLOOR)
        for z in (32, 36):
            if x not in (44, 48):
                w.clear(x, WALK, z)
    for x in (45, 46, 47):
        for z in (33, 34, 35):
            mine(w, x, WALK + 2, z, hanging=True)
    carve(w, [(41, z) for z in (33, 34, 35)], TOP, TOP)                # one more mine pit
    for z in (33, 34, 35):
        mine(w, 41, TOP, z)
    w.hint_zone(6, 58, WALK - 1, 32, 61, WALK + 4, 36)
    w.hint_zone(7, 53, WALK - 2, 32, 56, WALK + 4, 36)
    w.hint_zone(9, 43, WALK - 1, 32, 49, WALK + 2, 36)


def west(w: World, rng: random.Random):
    """C: three chasms crossed on moving platforms (three lifts side by side, in step): slow,
    faster, and a long one that stops a jump short of the far side."""
    def chasm(z0, z1, travel_end, ticks, stop):
        carve(w, rect(32, z0, 36, z1), TOP - 2, WALK)
        for x in (33, 34, 35):
            lift(w, (x, TOP, z0), (x, TOP, travel_end), ticks=ticks, stop=stop)
    chasm(41, 45, 45, 40, 10)
    chasm(49, 53, 53, 24, 3)
    chasm(56, 62, 61, 60, 10)          # stops at z 61: jump the last cell to the corner
    w.hint_zone(12, 32, WALK - 1, 38, 36, WALK + 3, 40)
    w.hint_zone(13, 32, WALK - 1, 45, 36, WALK + 3, 46)
    w.hint_zone(14, 32, WALK - 1, 47, 36, WALK + 3, 48)
    w.hint_zone(15, 32, WALK - 1, 54, 36, WALK + 3, 55)
    w.hint_zone(16, 32, WALK - 1, 56, 36, WALK + 4, 62)


def south(w: World, rng: random.Random):
    """D: the lower lane runs into a low tunnel of mines; the upper path over it holds the
    two eggs and leads to the paired lifts over the moat."""
    wall(w, 41, TOP - 2, 61, 53, TERRACE, 63, rng)                     # the upper path, 2 up
    wall(w, 45, TERRACE, 64, 49, TERRACE, 68, rng)                     # ...and the tunnel roof
    for x in (46, 48):
        for z in (65, 66, 67):
            mine(w, x, WALK, z)
    w.place(O.EGG, 46, TERRACE + 1, 66)
    w.place(O.EGG, 48, TERRACE + 1, 66)
    # Lifts A (from the path) and B (from the terrace) rise together and meet at the top, where
    # they wait 30 ticks, long enough to step across in 3D (D43; the original's 10 are not).
    for x in (50, 51):
        lift(w, (x, TERRACE, 60), (x, TERRACE + 2, 58), ticks=30, stop=30)
        lift(w, (x, TERRACE, 55), (x, TERRACE + 2, 57), ticks=30, stop=30)
    w.hint_zone(17, 38, WALK - 1, 64, 44, WALK + 1, 68)
    w.hint_zone(18, 44, TERRACE, 61, 50, TERRACE + 3, 68)
    w.hint_zone(19, 47, TERRACE, 55, 53, TERRACE + 5, 63)


def tower(w: World, rng: random.Random):
    """The central machine tower: a terrace round its foot, the second treasure behind it, an
    elevator up its east face, the red arrow on its roof."""
    terrace = rect(43, 40, 57, 54)
    for x, z in terrace:
        for y in range(TERRACE - 2, TERRACE + 1):
            w.set(x, y, z, MACHINE_FLOOR)
    for x, z in boundary(terrace):
        if not (49 <= x <= 52 and z == 54):          # where lift B lands
            w.set(x, TERRACE + 1, z, rng.choice(MACHINE_FACES) if rng.random() < 0.25 else MACHINE_FLOOR)
    wall(w, 46, 0, 43, 54, TOWER, 51, rng, face_chance=0.3)
    for x, y, z in ((46, 14, 47), (54, 14, 46), (50, 15, 43), (50, 6, 51), (48, 3, 43), (54, 13, 50)):
        w.set(x, y, z, MONITOR)
    roof = rect(46, 43, 54, 51)
    for x, z in boundary(roof):
        if not (x == 54 and 48 <= z <= 50):          # where the elevator arrives
            w.set(x, TOWER + 1, z, rng.choice(MACHINE_FACES) if rng.random() < 0.3 else MACHINE_FLOOR)
    carve(w, [(55, 49)], TERRACE, TERRACE)
    lift(w, (55, TERRACE, 49), (55, TOWER, 49), ticks=60, stop=20)
    w.place(O.TREASURE, 50, TERRACE + 1, 41)
    w.place(O.EXIT, 50, TOWER + 1, 47)
    props(w, rect(47, 44, 53, 50), TOWER + 1, rng, chance=0.12, kinds=MACHINE_PROPS)
    w.clear(50, TOWER + 1, 47)
    w.hint_zone(20, 43, TERRACE, 40, 57, TERRACE + 3, 55)
    w.hint_zone(21, 42, TERRACE, 39, 58, TOWER + 3, 56)


def main() -> int:
    rng = random.Random(11)
    w = World(mission=11, sky=0)
    no_rail = set()
    # No parapet where it would let Blupi step round a lesson: beside the elevator walls and
    # along the upper path's face. (The chasms cut theirs away.)
    no_rail |= {(x, z) for x in range(51, 60) for z in (32, 36)}
    no_rail |= {(x, 64) for x in range(40, 53)}
    build_ring(w, rng, no_rail)
    east(w, rng)
    north(w, rng)
    west(w, rng)
    south(w, rng)
    tower(w, rng)
    # Machine parts on the corners' parapets.
    corner_cells = set(CORNER_NE + CORNER_NW + CORNER_SW + START)
    corners = [c for c in boundary(RING) if c in corner_cells and c not in no_rail]
    props(w, corners, WALK + 1, rng, chance=0.3)
    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world011.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
