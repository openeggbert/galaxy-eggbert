"""World 3's tiles and building pieces (Mobile Eggbert worlds 030-034: the village).

The original's world 3 is a village: grass (107), stone walls and hills (35/156 with their edge
tiles), houses whose walls are window tiles (186-192) under slate roofs with dormers (193-197),
and, in its later missions, a haunted attic, a toy room, a rock cave, brick and machine rooms.
These pieces place those tiles only, for the scripts world030.py ... world034.py.

Two 3D rules, from how the tiles render (checked live 2026-10-25):
- Window, door and blank-window tiles (186-192) show their picture on the +Z face only, the
  other faces a flat swatch of the tile (grey for the blank window, dark for a window): houses
  get their windows on their +Z facade, blank tiles at its corners.
- Tiles with transparent areas drawn as cubes show black there in 3D -- the arch 198, cobwebs
  401-403, the stone and rock corner pieces 74/75/154/155 and 285-303, the striped poles
  235/236, the lattice 201, the pillar 76: none of them is used.
"""

from __future__ import annotations

import random

from kit import World, wall

GRASS = 107                  # grass over earth; its top is drawn as a grass plate
STONE = 35                   # the stone wall, the mass of every world-3 map
STONE_FACES = (156, 36, 37, 38, 39, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153)
WINDOWS = (186, 187, 188, 189)   # shuttered windows (+Z face)
BLANK_WINDOW = 190
DOOR = 192
DORMERS = (193, 194, 195)
SLATE = (196, 197)
BRICK = 261
BRICK_FACES = (262, 263, 283)
ROCK = 284
ROCK_FACES = (301,)
TOY_FACES = (215, 216, 217, 218, 219, 231)   # the toy room's balls and stars
TREES = (20, 21, 22)         # BigDecor conifers (explo.png plante2a-c)
PLANTS = (23, 24, 25)        # BigDecor leafy plants (plante3a-c)
TREASURE_DOOR1 = 421         # opens once one treasure is collected (Decor::OpenDoorsTresor)


def stone(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, chance=0.4):
    """Solid stone wall or rock: the stone tile with its variants mixed in."""
    wall(w, x0, y0, z0, x1, y1, z1, rng, faces=STONE_FACES, face_chance=chance, block=STONE)


def grass_top(w: World, x0, z0, x1, z1, y):
    """Turn the top layer of a stone mass into grass (walking level y + 1)."""
    w.fill(x0, y, z0, x1, y, z1, GRASS)


def house(w: World, x0, z0, x1, z1, y0, height, rng: random.Random, door=None, dormers=True,
          windows=0.75):
    """A house block from y0 up `height` cells: walls of window tiles under one row of slate
    roof (its top is Blupi's walking level y0 + height). The facade is the +Z face (z1), where
    the window tiles show; `door` = x of a door on the ground row. Dormers along the facade's
    roof row."""
    xa, xb = sorted((x0, x1))
    za, zb = sorted((z0, z1))
    top = y0 + height - 1
    for x in range(xa, xb + 1):
        for z in range(za, zb + 1):
            for y in range(y0, top + 1):
                if y == top:
                    front = z == zb and xa < x < xb
                    w.set(x, y, z, rng.choice(DORMERS) if dormers and front and rng.random() < 0.5
                          else rng.choice(SLATE))
                elif z == zb and xa < x < xb:
                    if door is not None and x == door and y == y0:
                        w.set(x, y, z, DOOR)
                    else:
                        w.set(x, y, z, rng.choice(WINDOWS) if rng.random() < windows else BLANK_WINDOW)
                else:
                    w.set(x, y, z, BLANK_WINDOW)


def facade_row(w: World, x0, x1, z, y, rng: random.Random, windows=0.75):
    """Window tiles along one row of a facade, its two end cells blank: a window tile's side
    faces show a dark swatch of the window, the blank tile's a plain grey."""
    xa, xb = sorted((x0, x1))
    for x in range(xa, xb + 1):
        inner = xa < x < xb
        w.set(x, y, z, rng.choice(WINDOWS) if inner and rng.random() < windows else BLANK_WINDOW)


def roof_row(w: World, x0, x1, z0, z1, y, rng: random.Random, dormer_z=None):
    """A slate roof layer; dormers along row `dormer_z`."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for z in range(min(z0, z1), max(z0, z1) + 1):
            if z == dormer_z and min(x0, x1) < x < max(x0, x1) and rng.random() < 0.5:
                w.set(x, y, z, rng.choice(DORMERS))
            else:
                w.set(x, y, z, rng.choice(SLATE))


def bird(w: World, start, end, ticks=24, stop=8):
    """A bird (ObjectType 20) flying to and fro between start and end, the original's
    stepAdvance/stepRecede and timeStop ticks. It kills Blupi on contact."""
    from kit import O
    w.place(O.BIRD, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)


def patrol(w: World, otype: int, start, end, ticks, stop):
    """Any patrolling MoveObject (wasp, moving mine, bulldozer, fish) with the original's ticks."""
    w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)
