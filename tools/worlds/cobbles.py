"""World 10's pieces: the grey cobbles (worlds 100-107).

The original's world 10 is cut into grey cobbled rock (35, with its edge pieces 36-40 and
144-152 and the dressed stone 156) under a dark cobbled sky; golden forks (77) stand about its
caves. These pieces carve caves into solid cobble and dress the cave walls with the variants.
"""

from __future__ import annotations

import random

from kit import World
from village import STONE, STONE_FACES

FORK = 77                   # the golden fork prop (an InnerFlatPlate, no collision)


def rock(w: World, x0, y0, z0, x1, y1, z1):
    """Solid cobble, plain (the variants go only where a face shows: dress())."""
    w.fill(x0, y0, z0, x1, y1, z1, STONE)


def carve(w: World, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def dress(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, chance=0.3):
    """Mix the cobble's variants into the cells of the box that face open air."""
    for x in range(x0, x1 + 1):
        for y in range(y0, y1 + 1):
            for z in range(z0, z1 + 1):
                if w.get(x, y, z) != STONE:
                    continue
                if any(w.get(x + dx, y + dy, z + dz) == 0
                       for dx, dy, dz in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1))):
                    if rng.random() < chance:
                        w.set(x, y, z, rng.choice(STONE_FACES))


def fork_alcove(w: World, x, y, z, into):
    """A golden fork in a one-cell alcove cut into the wall at (x, y, z), `into` (dx, dz) the
    way into the wall: off the walking cells (an empty-mask tile is a floor to the vertical
    test, D41)."""
    dx, dz = into
    w.set(x + dx, y, z + dz, FORK)
