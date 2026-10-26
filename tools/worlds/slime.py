"""World 6's slime, the green world: its tiles' roles and building pieces.

Mobile Eggbert's world 6 builds everything of one autotiled set, icons 341-363. Measured over
the original's worlds 060-066 (which tile sits where air is above, below, left or right of it):
341-346 inside the mass, 362 on tops, 363 and 348 on undersides, 351 for a slab one block thick,
349/350 on left and right sides, 352-361 for thin pieces and corners. 347, also on tops, is the
world's hazard: an animated slime surface that kills like spikes (blocks.csv hazard 2). A cube
shows its tile on every side, so only the vertical roles carry into 3D: `skin()` gives each
slime cube its tile from what lies above and below it, once a world is built. The undersides and
the slab (363, 348, 351) have small clear bits along their edges, under the renderer's cutout
threshold, which a cube draws black: undersides take inside tiles, slabs the opaque top 362.
"""

import random

from kit import World

SLIME_INSIDE = (341, 342, 343, 344, 345, 346)
SLIME_TOP = 362
SLIME_SPIKES = 347          # the deadly slime surface (hazard: spikes); never skinned onto slime
SLIME_UNDER = (363, 348)
SLIME_SLAB = 351
SLIME = 341                 # what slime is built of until skin() gives each cube its tile
SLIME_ALL = frozenset(range(341, 364)) - {SLIME_SPIKES}
POLES = (235, 236)          # the striped poles the original's lifts run along


def slime(w: World, x0, y0, z0, x1, y1, z1):
    """A box of slime (skinned later)."""
    w.fill(x0, y0, z0, x1, y1, z1, SLIME)


def skin(w: World, rng: random.Random):
    """Every slime cube its tile by the original's roles: top, underside, slab or inside."""
    cells = sorted(p for p, raw in w.blocks.items() if (raw >> 4) in SLIME_ALL)
    for x, y, z in cells:
        open_above = y + 1 >= 100 or w.get(x, y + 1, z) == 0
        open_below = y == 0 or w.get(x, y - 1, z) == 0
        if open_above:
            tile = SLIME_TOP
        else:
            tile = rng.choice(SLIME_INSIDE)
        w.set(x, y, z, tile)


def pole(w: World, x, y0, y1, z, rng: random.Random):
    """A striped pole from y0 to y1 (a lift's track, as the original's)."""
    tile = rng.choice(POLES)
    for y in range(y0, y1 + 1):
        w.set(x, y, z, tile)


def arch(w: World, x, y, z, facing, height=3, half=2):
    """A slime arch standing in front of a wall cell (x, y, z) that opens toward `facing`
    (dx, dz): pillars `half` cells to each side, a lintel over them, the opening 2*half-1 wide
    and `height` high. Marks the hub pods."""
    dx, dz = facing
    px, pz = -dz, dx                    # along the wall
    rx, rz = x + dx, z + dz             # the room cell in front of the wall cell
    for side in (-half, half):
        slime(w, rx + px * side, y, rz + pz * side, rx + px * side, y + height, rz + pz * side)
    for k in range(-half, half + 1):
        w.set(rx + px * k, y + height, rz + pz * k, SLIME)

