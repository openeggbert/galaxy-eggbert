"""World 2's tiles and building pieces (Mobile Eggbert worlds 020-025: the white palace).

The original's world 2 is a white palace before a mosaic (regions 25, 28, 30): white stone
(396) with its edge and trim tiles (386-397), window panels (391), stepped towers with
balconies, arches and columns, fountains, palms on grass (107); inside, brick halls (261) with
crates, libraries of shelves and cupboards (139-143), a rock cave (284/301), a toy room of
ball blocks (215-220) and blue machine panels (10). Its mechanics: saws (378) and their
switches (384/385), treasure doors (421 + n: open with n treasures), key doors (334-336),
lava (68), water (91 deep, 92 surface) and, in 025, blitz (304/305). These pieces place those
tiles only, for the scripts world020.py ... world025.py.

3D rules, from how the tiles render and collide (checked live 2026-10-25):
- The white tiles show their picture on the +Z face only, the other faces plain white: trims
  and windows read on south facades.
- Tiles with transparent areas drawn as cubes show black there: the balustrade 398, pillars
  399/76/375, the girder 201, cobwebs 401-403, the bars 51/138, the window panel 245 -- none is
  used. The arch 400 (a dark archway on its Z faces) has no collision: only as a blind arch
  with stone behind it. Columns, fountains, palms and the statue are BigDecor models.
- A door bar (182, 334-336, 421-440) closes its whole cell (since 66a1330; before, only the
  middle of the cell along X).
- A saw kills Blupi in its cell or standing on it (1bb048e). Drawn as a stone block with the
  blade above, it reads best as a floor cell, the blade standing up out of the floor. A switch
  is the floor cell Blupi stands on when he presses Action, and it reaches the saws of its own
  row along X (same height and Z, 20 cells either way: Decor::ActiveSwitch's 2D window).
"""

from __future__ import annotations

import random

from kit import World, wall

STONE = 396                  # the white stone, the mass of every world-2 map
STONE_FACES = (386, 387, 388, 390, 397, 392, 393)
WINDOWS = 391                # a pair of arched windows (+Z face)
CORNERS = (394, 395)
GRASS = 107                  # grass over earth; its top is drawn as a grass plate
BRICK = 261
SHELVES = (139, 140, 141, 142, 143)   # the library's bookshelves and cupboards
ROCK = 284
ROCK_FACES = (301,)
TOYS = (215, 216, 217, 218, 219, 220)  # the toy room's ball blocks
MACHINE = 10                 # the blue riveted panel
ARCH = 400                   # walk-through: a blind arch only, stone behind it

SAW = 378
SWITCH = 384                 # on: its saws turn
SWITCH_OFF = 385
LAVA = 68
DEEP_WATER = 91
WATER = 92                   # the surface
RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336   # keys 49, 50, 51
BLITZ_EMITTER = 304
BLITZ = 305


def treasure_door(n: int) -> int:
    """The golden bar that opens once n treasures are collected (Decor::OpenDoorsTresor)."""
    if not 1 <= n <= 20:
        raise ValueError("treasure doors need 1-20 treasures")
    return 420 + n


# BigDecor models (explo.png icons with a 3D model, avatars3d/objects/index.txt).
PALMS = (16, 18)
PLANTS = (23, 24, 25)
BALL_MACHINES = (28, 29, 30)
COLUMN = 87
FOUNTAIN = 88
STATUE = 89


def stone(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, chance=0.15, block=STONE,
          faces=STONE_FACES):
    """Solid white stone with its trim tiles mixed into the faces."""
    wall(w, x0, y0, z0, x1, y1, z1, rng, faces=faces, face_chance=chance, block=block)


def windows_row(w: World, x0, x1, y, z, rng: random.Random, chance=0.5):
    """Window panels along a south (+Z) facade row, where their picture shows."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        if w.get(x, y, z) and rng.random() < chance:
            w.set(x, y, z, WINDOWS)


def parapet(w: World, cells, y, rng: random.Random, gaps=()):
    """A one-block stone parapet on cells (x, z) at height y, except at `gaps`."""
    gaps = set(gaps)
    for x, z in cells:
        if (x, z) not in gaps:
            w.set(x, y, z, rng.choice(STONE_FACES) if rng.random() < 0.2 else STONE)


def sign_niche(w: World, x, y, z, facing, icon, rng: random.Random, door=False, block=STONE):
    """kit.sign_niche in palace stone: a hub sign in a niche one cell high opening toward
    `facing` (dx, dz); with `door` two deep with the blue bar in the doorway."""
    dx, dz = facing
    cells = [(x, z)] + ([(x + dx, z + dz)] if door else [])
    for cx, cz in cells:
        w.clear(cx, y, cz)
        for sx, sz, sy in ((cx - dz, cz + dx, y), (cx + dz, cz - dx, y), (cx, cz, y + 1), (cx, cz, y - 1),
                           (cx - dz, cz + dx, y + 1), (cx + dz, cz - dx, y + 1)):
            if w.get(sx, sy, sz) == 0:
                w.set(sx, sy, sz, rng.choice(STONE_FACES) if rng.random() < 0.2 else block)
    if w.get(x - dx, y, z - dz) == 0:
        w.set(x - dx, y, z - dz, block)
    w.set(x, y, z, icon)
    if dx != 0:
        w.rotate_plate(x, y, z)      # signboards face Z unless turned
    if door:
        w.set(x + dx, y, z + dz, 182)


def gate(w: World, x, y, z, door: int, height=1):
    """A door tile in a doorway, `height` cells high (a door stops Blupi in his walking cell;
    above it goes the lintel, so he cannot jump over it)."""
    for k in range(height):
        w.set(x, y + k, z, door)
