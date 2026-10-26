"""World 5's tiles and building pieces (Mobile Eggbert worlds 050-058: the space station).

The original's world 5 is a station of grey riveted plates (78-84, 85-90 their machine parts)
in black space (regions 7, 1, 10, 13, 21, 24, 26, 29): lattice girders (201) and pillars (200),
small plate edges (63/64); in its missions also brick halls (261-263, 283) with water channels
(91/92), a green slime cave (341-363) with spike traps (373), lava (68) and lightning (304/305).
These pieces place those tiles only, for the scripts world050.py ... world058.py.

3D notes (from blocks.csv and the other worlds' live checks):
- 78-84 are full cubes (UniformCube), the same picture on every face: the station's mass.
- 201 (the girder) is a cut-out lattice cube with no collision sideways but solid underfoot
  and overhead; 200 (pillars) and 63/64 (plate edges) are walk-through props.
- 347 is a plain slime edge tile (Tables::table_adapt_decor; Galaxy since ecc5605); the spike
  trap is 373 only (Decor::IsPiege), placed as a floor cell.
- Wind (110-125) pushes Blupi as Decor.cpp:2771 (Galaxy since be3ec9c): 110 toward -X, 114 +X,
  118 up, 122 down, not on a lift or in a vehicle; wind tiles are pass icons. A fan's mouth (the
  cell it blows into) kills and blows the fan away with its wind (Decor::IsVentillo).
- Other tiles with an empty collision mask (pillars 76, cobwebs, girders) still count as solid
  underfoot and overhead in Galaxy: never a design that needs Blupi to fall through them.
"""

from __future__ import annotations

import random

from kit import World

# The riveted plates, weighted as the hub's map uses them (82 most, 84 the rare grille).
METAL = (82, 83, 81, 80, 78, 79)
METAL_WEIGHTS = (155, 136, 82, 73, 62, 54)
GRILLE = 84
PARTS = (85, 86, 88, 89, 90)        # machine parts on plates: cut-out sprites (85/86 walk-through),
                                     # only on a face with plates behind it, never a one-thick wall
BALL = 87                            # the plate with a steel ball
GIRDER = 201
PILLARS = 200
PLATE_EDGES = (63, 64)               # small cross-billboard props, walk-through
BRICK = 261
BRICK_FACES = (262, 263, 283)
WATER = 92                           # the surface
DEEP_WATER = 91
LAVA = 68
SLIME = (341, 342, 343, 344, 345, 346)
SLIME_EDGES = (348, 349, 350, 351, 352, 353, 354, 355, 356, 357, 358, 359, 360, 361, 362, 363)
SPIKE_TRAP = 373
BLITZ_EMITTER = 304
BLITZ = 305
HUB_DOOR = 182


def plate(rng: random.Random) -> int:
    return rng.choices(METAL, weights=METAL_WEIGHTS)[0]


def metal(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, grille=0.02):
    """A solid box of riveted plates, mixed as the original's maps mix them."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, GRILLE if rng.random() < grille else plate(rng))


def metal_cells(w: World, cells, y0, y1, rng: random.Random):
    """Riveted plates on the (x, z) cells from y0 to y1."""
    for x, z in cells:
        for y in range(y0, y1 + 1):
            w.set(x, y, z, plate(rng))


def sign_niche(w: World, x, y, z, facing, icon, rng: random.Random, door=False):
    """A hub sign in a small metal kiosk one cell high, opening toward `facing` (dx, dz): with
    `door` two deep with the blue bar (182) in the doorway, which the hub opens once the mission
    before is won (HubDoors.hpp); Blupi then arrives one cell beyond the bar. Walls, roof and
    floor are filled in where missing (kit.sign_niche in plates)."""
    dx, dz = facing
    cells = [(x, z)] + ([(x + dx, z + dz)] if door else [])
    for cx, cz in cells:
        w.clear(cx, y, cz)
        for sx, sz, sy in ((cx - dz, cz + dx, y), (cx + dz, cz - dx, y), (cx, cz, y + 1), (cx, cz, y - 1),
                           (cx - dz, cz + dx, y + 1), (cx + dz, cz - dx, y + 1),
                           (cx - dz, cz + dx, y - 1), (cx + dz, cz - dx, y - 1)):
            if w.get(sx, sy, sz) == 0:
                w.set(sx, sy, sz, plate(rng))
    for sy in (y - 1, y, y + 1):
        if w.get(x - dx, sy, z - dz) == 0:
            w.set(x - dx, sy, z - dz, plate(rng))
    w.set(x, y, z, icon)
    if dx != 0:
        w.rotate_plate(x, y, z)      # signboards face Z unless turned
    if door:
        w.set(x + dx, y, z + dz, HUB_DOOR)


def girder_mast(w: World, x, z, y0, y1):
    """A lattice mast of girders from y0 to y1 (walk-through sideways, standable on top)."""
    for y in range(y0, y1 + 1):
        w.set(x, y, z, GIRDER)


def lift_ticks(cells: int) -> int:
    """The original's lift pace in world 5's hub: ten ticks a cell (80 for 8, 60 for 6)."""
    return 10 * cells


# BigDecor (explo.png icons; avatars3d/objects/index.txt): leafy plants, the water weed of the
# brick cisterns, ball machines.
PLANTS = (23, 24, 25)
WATER_WEED = 203
BALL_MACHINES = (28, 29, 30, 31)
STONE_PILLAR = 76            # cut-out stone column (walk-through sideways)
COBWEBS = (401, 402, 403)
BLUE_PANEL = 10              # the blue dotted panel of the station's machine shafts
FAN_LEFT = 126               # blows -X; its mouth (the -X side) kills (Decor::IsVentillo)
WIND_LEFT = 110              # the wind in front of a FAN_LEFT: pushes Blupi toward -X
TELEPORTERS = (330, 331, 332, 333)


def treasure_door(n: int) -> int:
    """The golden bar that opens once n treasures are collected (Decor::OpenDoorsTresor)."""
    if not 1 <= n <= 20:
        raise ValueError("treasure doors need 1-20 treasures")
    return 420 + n


def brick(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random, chance=0.12):
    """A solid box of brick with its worn variants mixed in."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, rng.choice(BRICK_FACES) if rng.random() < chance else BRICK)


def water(w: World, x0, y0, z0, x1, y1, z1):
    """Deep water (91) up to y1 - 1 under a surface row (92) at y1."""
    w.fill(x0, y0, z0, x1, y1 - 1, z1, DEEP_WATER)
    w.fill(x0, y1, z0, x1, y1, z1, WATER)


def shell(w: World, x0, y0, z0, x1, y1, z1, fill, floor=True, roof=True):
    """The walls (and floor and roof) of a box drawn with `fill(x0, y0, z0, x1, y1, z1)`; its
    inside is carved to air."""
    fill(x0, y0, z0, x1, y1, z0)
    fill(x0, y0, z1, x1, y1, z1)
    fill(x0, y0, z0, x0, y1, z1)
    fill(x1, y0, z0, x1, y1, z1)
    if floor:
        fill(x0, y0, z0, x1, y0, z1)
    if roof:
        fill(x0, y1, z0, x1, y1, z1)
    w.fill(x0 + 1, y0 + (1 if floor else 0), z0 + 1, x1 - 1, y1 - (1 if roof else 0), z1 - 1, 0)
