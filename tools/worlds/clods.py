"""World 9's tiles and building pieces (Mobile Eggbert worlds 090-095: the clay clods).

The original's world 9 is built of orange-brown clods of earth (284 and 301, its mass, mixed
about 5 : 4) floating in a dark red cave (region 27; 095 in region 2), their rims drawn with
edge sprites (285-300, 302, 303: lumps hanging from an underside, lumps on a top, sides,
corners, frames round small holes). Its missions add the grey stone village (35-37, 144-156
with the house tiles 186-197), blue machines (10, 11, 22, pipes 251-253) with water tanks
(91/92), girders (201), crumbling bridges (364) over lava (68), and props on the cave floors:
the yellow forks (77) and trestles (199).

3D notes (blocks.csv and the other worlds' live checks):
- 284/301 are full cubes, the same picture on every face.
- The edge sprites are flat plates in the middle of their cell (InnerFlatPlate) with an empty
  collision mask. Since D48 a plate reads unmirrored from both sides, so an edge on one side of
  its picture (287, 288, 291-296) would sit on the far side of its cell seen from behind: only
  the symmetric ones are used -- 285/302 hanging under an underside, 286/303 on a top, 289
  (top and bottom), 290 (both sides), 297/298/299 (frames), 300 (a string of lumps).
- Plates, props (77, 199), girders and pillars are floors and ceilings to the vertical test
  (D41, open): never in a cell Blupi walks, stands over or jumps through. `audit()` lists any
  such cell levelcheck can reach; every world-9 script checks itself with it.
"""

from __future__ import annotations

import random
import sys
from pathlib import Path

from kit import O, World  # noqa: F401

CLODS = (284, 301)
CLOD_WEIGHTS = (5, 4)                 # 284 : 301 as the original's maps mix them
HANG = (285, 302)                     # lumps hanging from a top edge: under an underside
BUMPS = (286, 303)                    # lumps along a bottom edge: on a top
TOP_AND_BOTTOM = 289
BOTH_SIDES = 290
FRAMES = (297, 298, 299)              # round a small hole (open below, open above, all round)
STRING = 300                          # lumps on a thin neck, top to bottom
EDGES = frozenset((285, 286, 289, 290, 297, 298, 299, 300, 302, 303))
Y_FORK = 77                           # the yellow fork prop
TRESTLE = 199                         # the yellow trestle prop
BRIDGE = 364                          # crumbles under Blupi and comes back (Decor::IsBridge)
LAVA = 68
WATER = 92
DEEP_WATER = 91
MACHINE = 10                          # the blue riveted panel
MACHINE_BALL = 11
MACHINE_GRILLE = 22
PIPES = (251, 252, 253)
GIRDER = 201
PILLARS = 200
STONE = (35, 36, 37, 156, 144, 145, 151, 152)   # the grey stone of the village
STONE_WEIGHTS = (40, 6, 6, 30, 4, 3, 3, 3)
HUB_DOOR = 182
RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
TELEPORTER = 330
SAW, SWITCH = 378, 384

# BigDecor (explo.png icons): trees, green plants, water weed.
TREES = (20, 21, 22)
PLANTS = (23, 24, 25)
WATER_WEED = 203


def clod(rng: random.Random) -> int:
    return rng.choices(CLODS, weights=CLOD_WEIGHTS)[0]


def clay(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random):
    """A solid box of clods."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, clod(rng))


def clay_cells(w: World, cells, y0, y1, rng: random.Random):
    for x, z in cells:
        for y in range(y0, y1 + 1):
            w.set(x, y, z, clod(rng))


def stone(w: World, x0, y0, z0, x1, y1, z1, rng: random.Random):
    """A solid box of the village's grey stone."""
    for x in range(min(x0, x1), max(x0, x1) + 1):
        for y in range(min(y0, y1), max(y0, y1) + 1):
            for z in range(min(z0, z1), max(z0, z1) + 1):
                w.set(x, y, z, rng.choices(STONE, weights=STONE_WEIGHTS)[0])


def carve(w: World, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def is_clod(w: World, x, y, z) -> bool:
    return w.get(x, y, z) in CLODS


def fringe(w: World, rng: random.Random, box=None, clear=5, chance=0.55):
    """Lumps hanging under the clods' undersides, as the original draws every underside: a plate
    (285/302) in the air cell under a clod when the `clear` cells under that are air too, so no
    floor Blupi can reach is near it (D41). `box` (x0, z0, x1, z1) limits it to a region."""
    cells = sorted(p for p, raw in w.blocks.items() if (raw >> 4) in CLODS)
    for x, y, z in cells:
        if box and not (box[0] <= x <= box[2] and box[1] <= z <= box[3]):
            continue
        if y < clear + 1 or w.get(x, y - 1, z) != 0:
            continue
        if any(w.get(x, y - 1 - k, z) != 0 for k in range(1, clear + 1)):
            continue
        if rng.random() < chance:
            w.set(x, y - 1, z, rng.choice(HANG))


def bumps(w: World, cells, y, rng: random.Random, chance=0.6):
    """Lumps on the tops of clods at y - 1 (plates 286/303 at y), for tops out of reach."""
    for x, z in cells:
        if w.get(x, y, z) == 0 and is_clod(w, x, y - 1, z) and rng.random() < chance:
            w.set(x, y, z, rng.choice(BUMPS))


def audit(path):
    """Cells holding a decor tile with an empty collision mask (plates, props, girders, pillars)
    where D41 would make it a floor or a ceiling to Blupi: in the column of a cell levelcheck says
    he reaches, from under his feet to the top of his jump (ry - 1 .. ry + 3), or in a column a
    walk or a jump along X or Z passes (one or two cells on, ry .. ry + 3, with nothing solid
    between his feet's height and it). Returns [(decor cell, icon, reached cell)]."""
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
    import levelcheck as lc
    import vwr_inspect
    blocks = lc.load_blocks(Path(__file__).resolve().parents[1] / "data" / "blocks.csv")
    world = vwr_inspect.read_world(Path(path))
    grid = lc.Grid(world, blocks)
    reach = lc.reachable(grid, tuple(world["spawn"]))

    def decor(p):
        icon = grid.cells.get(p, 0)
        if icon == 0:
            return False
        b = blocks.get(icon, blocks[0])
        return not (b["collision_mask"] != 0 or b["hazard"] or b["water"] or b["teleporter"] or b["fan"] or
                    b["world_select"] > 0 or b["hub_door"] or b["door_key"] >= 0 or icon == SWITCH)

    found = []
    for r in sorted(reach):
        x, ry, z = r
        for y in range(ry - 1, ry + 4):
            if decor((x, y, z)):
                found.append(((x, y, z), grid.cells[(x, y, z)], r))
        for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            for k in (1, 2):
                cx, cz = x + dx * k, z + dz * k
                for y in range(ry, ry + 4):
                    if not grid.free((cx, y, cz)) and not decor((cx, y, cz)):
                        break
                    if decor((cx, y, cz)):
                        found.append(((cx, y, cz), grid.cells[(cx, y, cz)], r))
    return found


def report_audit(path) -> bool:
    found = audit(path)
    seen = set()
    for p, icon, r in found:
        if p in seen:
            continue
        seen.add(p)
        print(f"  D41: decor {icon} at {p} by reachable {r}")
    return not found
