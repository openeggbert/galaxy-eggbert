"""World 8's pieces: the blue machine works in the sky (worlds 080-084).

The original's world 8 is built of the cyan riveted panel (10) with corner fittings (15-18),
small bolts and fittings on its faces (53-60), runs of blue pipe (250-253), pipe machinery
(254-255) and the saucers of the Temp tile (324), which shrink away and come back on a cycle
(table_decor_temp), all floating in the sky.
"""

from __future__ import annotations

import random

from kit import World

PANEL = 10
FITTINGS = (15, 16, 17, 18)
BOLTS = (53, 54, 55, 56, 59, 60)
PIPES = (250, 251, 252, 253)
PIPE_MACHINES = (254, 255)
TEMP = 324                  # the saucer that vanishes and comes back (Decor's Temp)


def deck(w: World, x0, z0, x1, z1, y, thickness=1):
    """A floor of panel whose top is at y (walking level y + 1)."""
    w.fill(x0, y - thickness + 1, z0, x1, y, z1, PANEL)


def panel(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, fitting_chance=0.1):
    """A solid block of panel with a few corner fittings mixed into it."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                tile = rng.choice(FITTINGS) if rng.random() < fitting_chance else PANEL
                w.set(x, y, z, tile)


def pipe_rail(w: World, cells, y, rng: random.Random, height=1):
    """A parapet of pipe along `cells` ((x, z) pairs), `height` high from y."""
    for x, z in cells:
        for k in range(height):
            w.set(x, y + k, z, rng.choice(PIPES))


def pipe_column(w: World, x, y0, y1, z, rng: random.Random):
    """A column of pipe machinery from y0 to y1."""
    for y in range(y0, y1 + 1):
        w.set(x, y, z, rng.choice(PIPE_MACHINES))
