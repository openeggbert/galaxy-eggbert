"""World 4's tiles and building pieces (Mobile Eggbert worlds 040-046: the cheese).

The original's world 4 is a world of Swiss cheese: every map is a mass of the four cheese tiles
(246-249) with their dripping edge tiles (264-282) round the holes, under backgrounds of
leaves (region 30, the hub), dark rock (6), red sky (20), lava rock (21), cobbles (2), red
curtains (3) and a cheese moon (9). Its later missions add blue machine panels (10) and pipes
(53-60), lava (68), water (91/92), crushers (317), the walk-through cheese of secret passages
(339), treasure doors (421 + n) and key doors (334-336). Its mechanics: the helicopter
(ObjectType 13, parked by the dozen so one is always at hand), hanging mines (3) on cave
ceilings, mines (2), moving mines (16), lifts, treasure doors, keys, crates, drinks,
lollipops and invert pickups. These pieces place those tiles only, for world040.py ...
world046.py.

3D rules, from how the tiles render (checked live 2026-10-26):
- The cheese tiles 246-249 are opaque cubes, the picture on every face: the mass of walls,
  floors and ceilings, mixed at random as the original's maps mix them.
- The edge tiles 264-282 are sprites with only a few cheese drips: cut out, they all but
  vanish in 3D. Not used; the holes are carved round instead (`hole`, `bubble`).
- 339 is drawn as cheese with dark holes and has no collision: the original's secret
  passages, passable cheese. 214 (the invisible wall) shows a dashed outline: not used.
- Swiss cheese is holes: caves here are round bubbles with flat floors (`bubble`), joined by
  round tunnels (`tube`); craters (`pit_surface`) dent the outer faces.
"""

from __future__ import annotations

import math
import random

from kit import O, World

CHEESE = (246, 247, 248, 249)        # the four cheese tiles, mixed
SECRET = 339                         # cheese without collision (a secret passage)
MACHINE = 10                         # the blue machine panel (046's girders and posts)
MACHINE_PIPES = (53, 54, 55, 56, 57, 58, 59, 60)   # non-colliding pipe pieces
LAVA, WATER, DEEP_WATER = 68, 92, 91
CRUSHER = 317
RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
TREASURE_DOOR1 = 421                 # 421 + n - 1 opens once n treasures are taken
PLANTS = (23, 24, 25)                # BigDecor leafy plants (explo plante3a-c)
TREES = (20, 21, 22)                 # BigDecor round trees (explo plante2a-c)
HOUSE, HUT = 26, 27                  # BigDecor the house (maison) and the hut (cabane)
MECA = (28, 29, 30, 31)              # BigDecor machines (meca1-4)


def cheese(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random):
    """A solid box of cheese, the four tiles mixed."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, rng.choice(CHEESE))


def carve(w: World, x0, y0, z0, x1, y1, z1):
    """Air in the box."""
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def _cells_in_ellipsoid(cx, cy, cz, rx, ry, rz):
    for x in range(math.floor(cx - rx), math.ceil(cx + rx) + 1):
        for y in range(math.floor(cy - ry), math.ceil(cy + ry) + 1):
            for z in range(math.floor(cz - rz), math.ceil(cz + rz) + 1):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 + ((z - cz) / rz) ** 2 <= 1.0:
                    if 0 <= x < 100 and 0 <= y < 100 and 0 <= z < 100:
                        yield x, y, z


def hole(w: World, cx, cy, cz, r, ry=None, rz=None):
    """Carve a round hole (an ellipsoid of radii r, ry, rz) out of the cheese."""
    for p in _cells_in_ellipsoid(cx, cy, cz, r, ry if ry is not None else r, rz if rz is not None else r):
        w.clear(*p)


def ball(w: World, cx, cy, cz, r, rng: random.Random, ry=None, rz=None):
    """A round lump of cheese."""
    for p in _cells_in_ellipsoid(cx, cy, cz, r, ry if ry is not None else r, rz if rz is not None else r):
        w.set(*p, rng.choice(CHEESE))


def bubble(w: World, cx, floor, cz, rx, height, rz, rng: random.Random, below=1):
    """A cave: a dome-shaped hole whose floor is flat at walking level `floor` (the cheese
    under it is filled back to `below` cells thick), `height` cells high at its middle,
    radii rx, rz across."""
    for x, y, z in list(_cells_in_ellipsoid(cx, floor, cz, rx, height, rz)):
        if y >= floor:
            w.clear(x, y, z)
    for x in range(math.floor(cx - rx), math.ceil(cx + rx) + 1):
        for z in range(math.floor(cz - rz), math.ceil(cz + rz) + 1):
            if ((x - cx) / rx) ** 2 + ((z - cz) / rz) ** 2 <= 1.0:
                for y in range(floor - below, floor):
                    if w.get(x, y, z) == 0:
                        w.set(x, y, z, rng.choice(CHEESE))


def cave(w: World, cx, floor, cz, rx, walls, dome, rng: random.Random, rz=None, below=1):
    """A cave: a round (elliptic, radii rx, rz) room with a flat floor at walking level
    `floor`, upright walls `walls` cells high and a dome `dome` cells high above them (the
    cheese under the floor filled back to `below` cells thick). Unlike `bubble` its edges are
    full height, so ledges and niches round its wall have headroom."""
    rz = rz if rz is not None else rx
    for x in range(math.floor(cx - rx), math.ceil(cx + rx) + 1):
        for z in range(math.floor(cz - rz), math.ceil(cz + rz) + 1):
            d = ((x - cx) / rx) ** 2 + ((z - cz) / rz) ** 2
            if d > 1.0:
                continue
            top = floor + walls - 1 + (dome * math.sqrt(1.0 - d) if dome else 0)
            for y in range(floor, int(top) + 1):
                w.clear(x, y, z)
            for y in range(floor - below, floor):
                if w.get(x, y, z) == 0:
                    w.set(x, y, z, rng.choice(CHEESE))


def shaft(w: World, cx, cz, y0, y1, r):
    """A round vertical shaft of radius r round (cx, cz) from y0 to y1 inclusive."""
    for x in range(math.floor(cx - r), math.ceil(cx + r) + 1):
        for z in range(math.floor(cz - r), math.ceil(cz + r) + 1):
            if (x - cx) ** 2 + (z - cz) ** 2 <= r * r:
                for y in range(min(y0, y1), max(y0, y1) + 1):
                    w.clear(x, y, z)


def tube(w: World, a, b, r=2.6, floor=None):
    """A round tunnel of radius r from cell a to cell b (any direction). With `floor` (a
    walking level) a and b are cells Blupi walks through: the axis runs r - 1.1 above them and
    the cells under the floor stay cheese, so the default tunnel walks on a floor five wide
    and is five high -- room for a jump and for the chase camera above him (in a three-high
    tunnel a jump takes the camera into the ceiling)."""
    (ax, ay, az), (bx, by, bz) = a, b
    if floor is not None:
        ay, by = ay + r - 1.1, by + r - 1.1
    n = max(abs(bx - ax), abs(by - ay), abs(bz - az), 1) * 2
    for i in range(int(n) + 1):
        t = i / n
        cx, cy, cz = ax + (bx - ax) * t, ay + (by - ay) * t, az + (bz - az) * t
        for x, y, z in _cells_in_ellipsoid(cx, cy, cz, r, r, r):
            if floor is None or y >= floor:
                w.clear(x, y, z)


def pit_surface(w: World, cells, rng: random.Random, count, rmin=1.2, rmax=2.6, keep=None):
    """Dent `count` round craters into the surface `cells` (x, y, z of surface cells), the
    look of a cut cheese. `keep(x, y, z)` returns True for cells that must stay."""
    cells = list(cells)
    for _ in range(count):
        cx, cy, cz = rng.choice(cells)
        r = rng.uniform(rmin, rmax)
        for p in _cells_in_ellipsoid(cx, cy, cz, r, r, r):
            if keep is None or not keep(*p):
                w.clear(*p)


def hang(w: World, x, y, z):
    """A mine (ObjectType 3) hanging from the first ceiling above cell (x, y, z)."""
    from kit import mine
    while w.get(x, y + 1, z) == 0:
        y += 1
        if y > 98:
            raise ValueError(f"no ceiling above ({x}, {z})")
    mine(w, x, y, z, hanging=True)


def low_mine(w: World, x, y, z, rng: random.Random):
    """A mine hanging at Blupi's height (walking cell y) from a drop of cheese that reaches
    down from the ceiling to just above it: a walker passes beside it, or crouches under it
    (a hanging mine is harmless to a crouching Blupi); walking into it explodes it."""
    from kit import mine
    top = y + 1
    while w.get(x, top, z) == 0:
        top += 1
        if top > 98:
            raise ValueError(f"no ceiling above ({x}, {z})")
    for yy in range(y + 1, top):
        w.set(x, yy, z, rng.choice(CHEESE))
    mine(w, x, y, z, hanging=True)


def helicopters(w: World, cells):
    """Parked helicopters (ObjectType 13) at walking-level cells (x, y, z): the original
    leaves them by the dozen, one at hand wherever Blupi may have lost his."""
    for x, y, z in cells:
        w.place(O.HELICOPTER, x, y, z)


def patrol(w: World, otype: int, start, end, ticks, stop=0):
    """A patrolling MoveObject with the original's stepAdvance/stepRecede and timeStop ticks."""
    w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)
