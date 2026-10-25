"""Building blocks for authoring Galaxy Eggbert's campaign worlds with tools/worldkit.py.

Small, deliberate pieces -- floors, walls, rails, towers, lifts and patrols -- so each world's
script reads as its design. Everything places the game's existing tiles and objects only.
Coordinates are raw grid cells, Y up; a floor block at y makes y + 1 Blupi's walking level.
"""

from __future__ import annotations

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from worldkit import ObjectType as O, World  # noqa: E402,F401

# World 1 (the blue machine world, region 0): its own tiles (mobile-eggbert worlds 010-014).
MACHINE_FLOOR = 10                       # the blue dotted panel, the mass of every world-1 map
# Machine cubes for walls and towers: only fully opaque tiles. 15-31 have transparent areas
# that show the dark backdrop in 2D but would be holes in a 3D cube.
MACHINE_FACES = (2, 4, 5, 6, 8, 9, 11, 12, 13, 14)
MONITOR = 1
WARNING = 48
MACHINE_PROPS = (53, 54, 55, 56, 57, 58, 59, 60)  # small non-colliding machine parts
MACHINE_PILLARS = (50, 52)                       # non-colliding pillars
WATER = 92


def slab(w: World, x0, z0, x1, z1, y, block=MACHINE_FLOOR, thickness=1):
    """A floor of `thickness` blocks whose top is at y (walking level y + 1)."""
    w.fill(x0, y - thickness + 1, z0, x1, y, z1, block)


def wall(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, faces=MACHINE_FACES, face_chance=0.15,
         block=MACHINE_FLOOR):
    """A solid wall or block of machine panel with some machine cubes mixed into its faces."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, rng.choice(faces) if rng.random() < face_chance else block)


def rail(w: World, cells, y, rng: random.Random, block=MACHINE_FLOOR):
    """A one-block-high parapet along `cells` (x, z) at height y."""
    for x, z in cells:
        w.set(x, y, z, rng.choice(MACHINE_FACES) if rng.random() < 0.2 else block)


def props(w: World, cells, y, rng: random.Random, chance=0.25, kinds=MACHINE_PROPS):
    """Scatter non-colliding machine parts on floor cells (x, z) at walking level y."""
    for x, z in cells:
        if rng.random() < chance and w.get(x, y, z) == 0:
            w.set(x, y, z, rng.choice(kinds))


def lift(w: World, start, end, ticks=30, stop=10):
    """A platform lift (ObjectType 1): a cube Blupi stands on, travelling between start and end
    in `ticks` (20 Hz, the reference's stepAdvance/stepRecede) and dwelling `stop` ticks at
    each end (timeStopStart/End). Lifts with the same timing move in step."""
    w.place(O.LIFT, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)


def mine(w: World, x, y, z, hanging=False):
    """A still mine (ObjectType 2), or with `hanging` one hanging from above (ObjectType 3,
    harmless to a crouching Blupi)."""
    w.place(O.HANGING_MINE if hanging else O.MINE, x, y, z)


def boundary(cells):
    """The cells of a 2D region that touch its outside: where its parapet goes."""
    region = set(cells)
    return sorted(c for c in region
                  if any((c[0] + dx, c[1] + dz) not in region for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1))))


def line(x0, z0, x1, z1):
    """Cells of an axis-aligned line (inclusive)."""
    if x0 == x1:
        return [(x0, z) for z in range(min(z0, z1), max(z0, z1) + 1)]
    return [(x, z0) for x in range(min(x0, x1), max(x0, x1) + 1)]


def rect(x0, z0, x1, z1):
    return [(x, z) for x in range(min(x0, x1), max(x0, x1) + 1) for z in range(min(z0, z1), max(z0, z1) + 1)]


# Hub tiles (include/GalaxyEggbert/BlockTypes.hpp).
HUB_DOOR = 182
GOLD_DOOR = 183
MISSION_SIGN1 = 174      # ...181
WORLD_SIGNS = {**{n: 157 + n for n in range(1, 9)}, 9: 309, 10: 411, 11: 412, 12: 413}
SHOWCASE_SIGN = 414      # Galaxy's own portal to world999


def sign_niche(w: World, x, y, z, facing, icon, rng: random.Random, door=False, block=MACHINE_FLOOR):
    """A hub sign in a niche one cell high, opening toward `facing` (dx, dz). With `door` the
    niche is two deep and its doorway holds a blue bar (HubDoor), which the hub opens once the
    previous mission is won (HubDoors.hpp); Blupi then arrives just outside it. Walls, roof
    and floor are filled in where missing, so it can be cut into a wall or stand alone."""
    dx, dz = facing
    cells = [(x, z)] + ([(x + dx, z + dz)] if door else [])
    for cx, cz in cells:
        w.clear(cx, y, cz)
        for sx, sz, sy in ((cx - dz, cz + dx, y), (cx + dz, cz - dx, y), (cx, cz, y + 1), (cx, cz, y - 1),
                           (cx - dz, cz + dx, y + 1), (cx + dz, cz - dx, y + 1)):
            if w.get(sx, sy, sz) == 0:
                w.set(sx, sy, sz, rng.choice(MACHINE_FACES) if rng.random() < 0.3 else block)
    if w.get(x - dx, y, z - dz) == 0:
        w.set(x - dx, y, z - dz, block)
    w.set(x, y, z, icon)
    if dx != 0:
        w.rotate_plate(x, y, z)      # signboards face Z unless turned
    if door:
        w.set(x + dx, y, z + dz, HUB_DOOR)

