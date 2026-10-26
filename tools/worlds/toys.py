"""World 7's toys: its tiles' roles and building pieces.

Mobile Eggbert's world 7 (worlds 070-075) builds everything of the toy tiles 214-236: balls and
marbles on coloured grounds (215-220), striped pole tiles on a ground (221-223), balls on pole
tops (224, 225), a pipe column (226), a striped ball (227), a cage (228), striped rods (229), star
badges (230, 231), cones (232), trees and toadstools (233, 234), and the striped candy poles 235
and 236, whose collision is empty (decor Blupi passes in front of). 214 is the original's
invisible wall (drawn only in the editor, solid except to the helicopter): not used in 3D.

Each mission has its own palette, as the original's: 071 the green marble ball 218 with
the orange-striped 222 and the star badges 230; 072 the yellow checkered ball 215 with orange
balls 217 as ledges; 073 and 074 the whole box mixed; 075 the green marble ball 218 and the
orange ball 217 with grey grates (200, 201). The toy BigDecor models 36-47 (bd_enfant01-13:
cones with balls, half balls, balls on poles, a cylinder, lollipops) stand about as in the
original's BigDecor layer.

Candy poles are kept out of the cells Blupi walks or jumps through: an empty-mask tile is a floor
and a ceiling to the engine's vertical test (D41), so they stand under ledges, beside paths and
in nooks, never in his way.
"""

import random

from kit import World

BALLS = (215, 216, 217, 218, 219, 220)
POLE_TILES = (221, 222, 223)          # solid: a striped pole on a coloured ground
POLE_TOPS = (224, 225)                # a ball on a pole's top
PIPE, STRIPED_BALL, CAGE, RODS = 226, 227, 228, 229
STARS = (230, 231)
CONES = 232
TREES = (233, 234)
CANDY_POLES = (235, 236)              # empty collision: decor only
# The whole box mixed, as the original's 073/074 mass (uniform cubes only).
MIXED = (215, 216, 217, 218, 219, 220, 221, 222, 230, 231, 233, 234)
# BigDecor toy models (explo icons, bd_enfant01-13).
CONE_BALLS = (36, 37, 38)
HALF_BALLS = (39, 40)
BALL_POLES = (41, 42, 43)
CYLINDERS = 44
LOLLIPOPS = (45, 46, 47)

LAVA = 68
WATER = 92
BAR = 138
WARNING = 48            # the yellow warning triangle the original sets beside a mine field
RED_DOT = 3             # the red knob tile of 074's pool frame
GRATE = 200             # grey pipe pillar, empty collision (075)
GIRDER = 201            # grey cross-braced girder, empty collision (075)
PIPES = (49, 50, 51, 52)  # the blue pipe pieces of 071's lift frame, empty collision


def toy(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, palette=BALLS, weights=None):
    """A box of toy tiles drawn from `palette` (with `weights`, as the original's counts)."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, rng.choices(palette, weights)[0] if weights else rng.choice(palette))


def carve(w: World, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def candy(w: World, x, y0, y1, z, tile=None, rng: random.Random | None = None):
    """A striped candy pole (235/236, decor) from y0 to y1."""
    tile = tile or (rng.choice(CANDY_POLES) if rng else CANDY_POLES[1])
    w.fill(x, y0, z, x, y1, z, tile)


def post(w: World, x, y0, y1, z, rng: random.Random, top=None):
    """A solid striped post (221-223) from y0 to y1 with a ball on its top (224/225)."""
    tile = rng.choice(POLE_TILES)
    w.fill(x, y0, z, x, y1 - 1, z, tile)
    w.set(x, y1, z, top or rng.choice(POLE_TOPS))
