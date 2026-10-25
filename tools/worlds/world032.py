#!/usr/bin/env python3
"""Mission 032, the mountain and its two keys (worlds3d/world032.vwr): brief plans/briefs/world032.md.

Mobile Eggbert's level 032: a street of houses where a bulldozer drives to and fro; a lollipop
at its end (lick it with Action: a while of very high jumps) to jump up the first cliff of a
stone mountain; a wasp on the climb (its touch puts Blupi in a balloon); a floating island with
palms and a second lollipop for the next cliff; a fish in a small pool; three eggs in a nook; on
the top a village with birds, the red key in a house (and an egg); the red door (334) opens the
way down into a machine room with lightning (blitz, on and off) and mines riding up and down,
the blue key in it; the blue door (336) closes the room of star tiles with the red arrow, two
mines sliding across it. Here the same order on a 3D mountain: the street heading east, ledges
round the mountain's south, east and north faces, the village on its top, the machine gallery
and the exit room inside it.

    python3 tools/worlds/world032.py                  # writes worlds3d/world032.vwr
    python3 tools/worlds/world032.py --check STAGE out
        STAGE timed:  the timed enemies (bulldozer, wasp, fish, birds, moving mines) and the
                      lightning left out, as when passed at the right moment
        STAGE power:  also a stair where a lollipop's high jump goes
        STAGE solved: also both doors open (the keys used)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine, rect  # noqa: E402
from village import GRASS, TREES, bird, grass_top, house, patrol, stone  # noqa: E402

MACHINE = (78, 79, 80, 81, 82, 83, 84)       # the grey machine panels of the original's room
STARS = 231
RED_DOT = 3
BLITZ_EMITTER, BLITZ = 304, 305
RED_DOOR, BLUE_DOOR = 334, 336
PALMS = (16, 17, 18, 19)                     # BigDecor plante1a-d
STREET = 4                                   # the street's top: Blupi walks at 5
L1, L2A, L2B, L3, SUMMIT, HALL = 10, 12, 14, 20, 24, 8   # walking levels
SPAWN = (10, STREET + 1, 92)


def skin(w: World, box, tiles, rng: random.Random):
    """Line the solid cells round the air of `box` (x0, y0, z0, x1, y1, z1) with `tiles`."""
    x0, y0, z0, x1, y1, z1 = box
    for x in range(x0, x1 + 1):
        for y in range(y0, y1 + 1):
            for z in range(z0, z1 + 1):
                if w.get(x, y, z) != 0:
                    continue
                for dx, dy, dz in ((1, 0, 0), (-1, 0, 0), (0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)):
                    n = (x + dx, y + dy, z + dz)
                    if w.get(*n) in (35,) + tuple(range(36, 40)) + tuple(range(144, 154)) + (156,):
                        w.set(*n, rng.choice(tiles))


def street(w: World, rng: random.Random, timed: bool):
    """A: the street of houses heading east; the bulldozer drives through the narrow gate
    between two houses; the lollipop at its end."""
    stone(w, 6, 0, 74, 44, STREET, 86, rng)
    grass_top(w, 6, 74, 44, 86, STREET)
    y = STREET + 1
    for x0, x1, h in ((8, 12, 4), (13, 17, 3), (18, 22, 5), (23, 25, 3), (26, 28, 4), (29, 34, 3)):
        house(w, x0, 75, x1, 80, y, h, rng, door=(x0 + x1) // 2 if h == 3 else None)
    house(w, 26, 81, 28, 81, y, 4, rng, dormers=False)          # the gate: the street one wide
    house(w, 26, 83, 28, 87, y, 4, rng)
    stone(w, 6, 0, 87, 28, STREET, 87, rng)
    for x in [8] + list(range(12, 26)) + list(range(29, 43)):
        w.set(x, y, 84, 156)                                      # the parapet
    stone(w, 6, y, 74, 7, y + 2, 84, rng)                          # the west end
    # The lane from the south into the street's west end, where Blupi starts.
    stone(w, 8, 0, 85, 12, STREET, 94, rng)
    grass_top(w, 9, 85, 11, 94, STREET)
    for z in range(85, 95):
        w.set(8, y, z, 156)
        w.set(12, y, z, 156)
    for x in range(8, 13):
        w.set(x, y, 94, 156)
    w.bigdecor(TREES[0], 14, y, 88)
    w.bigdecor(TREES[1], 36, y, 76)
    if not timed:
        patrol(w, O.BULLDOZER, (24, y, 82), (30, y, 82), ticks=90, stop=20)
    w.place(O.LOLLIPOP, 40, y, 82)


def mountain(w: World, rng: random.Random, timed: bool, power: bool):
    """B-D: the ledges round the mountain, the island, the fish pool, the egg nook, the
    village on top with the red key."""
    stone(w, 44, 0, 28, 72, SUMMIT - 1, 73, rng)                  # the core
    grass_top(w, 44, 28, 72, 73, SUMMIT - 1)
    stone(w, 38, 0, 74, 78, L1 - 1, 80, rng)                       # L1, the south ledge
    stone(w, 73, 0, 66, 78, L1 - 1, 73, rng)                       # L2, up the east face
    stone(w, 73, 0, 58, 78, L2A - 1, 65, rng)
    stone(w, 73, 0, 36, 78, L2B - 1, 57, rng)
    stone(w, 73, 0, 22, 78, L3 - 1, 35, rng)                       # L3's north-east corner
    stone(w, 40, 0, 22, 72, L3 - 1, 27, rng)                       # L3, the north ledge
    if power:                                                      # stairs for the high jumps
        stone(w, 41, STREET + 1, 82, 41, STREET + 2, 82, rng)
        stone(w, 41, STREET + 1, 81, 41, STREET + 3, 81, rng)
        stone(w, 75, L2B, 37, 75, L2B + 1, 37, rng)
        stone(w, 75, L2B, 36, 75, L2B + 3, 36, rng)
    if not timed:
        patrol(w, O.WASP, (58, L1, 74), (58, L1, 80), ticks=12, stop=4)
    # The floating island east of L2, one cell across, a rock cone under it.
    for i, (x0, z0, x1, z1) in enumerate(((80, 42, 86, 50), (81, 43, 85, 49), (82, 44, 84, 48), (83, 45, 83, 47))):
        stone(w, x0, L2B - 1 - i, z0, x1, L2B - 1 - i, z1, rng)
    grass_top(w, 80, 42, 86, 50, L2B - 1)
    w.place(O.LOLLIPOP, 84, L2B, 46)
    w.bigdecor(PALMS[0], 81, L2B, 43)
    w.bigdecor(PALMS[2], 85, L2B, 49)
    # The fish pool across L3, two cells long and one deep as the original's: wade or jump
    # it while the fish is away.
    w.fill(60, L3 - 1, 22, 61, L3 - 1, 27, 92)
    if not timed:
        patrol(w, O.FISH, (60, L3 - 1, 22), (60, L3 - 1, 27), ticks=15, stop=47)
    # The egg nook in the core's north face.
    w.fill(51, L3, 28, 53, L3 + 2, 29, 0)
    for x in (51, 52, 53):
        w.place(O.EGG, x, L3, 29)
    # Up from L3's west end to the summit: a step at walk 22.
    stone(w, 44, L3, 22, 47, SUMMIT - 3, 27, rng)
    # The village on top: the key house (a room behind its open doorway), two more houses,
    # trees, three birds.
    house(w, 56, 46, 62, 52, SUMMIT, 5, rng)
    w.fill(57, SUMMIT, 47, 61, SUMMIT + 2, 51, 0)
    w.fill(59, SUMMIT, 52, 59, SUMMIT + 1, 52, 0)
    w.place(O.KEY1, 59, SUMMIT, 48)
    w.place(O.EGG, 57, SUMMIT, 48)
    house(w, 46, 60, 52, 66, SUMMIT, 3, rng, door=49)
    house(w, 64, 36, 70, 42, SUMMIT, 6, rng, door=67)
    for icon, x, z in ((TREES[0], 66, 58), (TREES[2], 50, 46), (TREES[1], 68, 68), (TREES[0], 55, 38)):
        w.bigdecor(icon, x, SUMMIT, z)
    bird(w, (54, SUMMIT + 1, 44), (54, SUMMIT + 1, 56), ticks=24)
    if not timed:
        bird(w, (56, SUMMIT, 54), (62, SUMMIT, 54), ticks=24)    # before the key house's door
    bird(w, (64, SUMMIT + 2, 58), (70, SUMMIT + 2, 64), ticks=24)


def inside(w: World, rng: random.Random, timed: bool, solved: bool):
    """E-G: the gatehouse with the red door over a shaft, the machine gallery (lightning,
    mines riding up and down, the blue key), the blue door, the exit room."""
    # The gatehouse on the summit's north-west corner; its door faces the village (+Z).
    stone(w, 46, SUMMIT, 30, 50, SUMMIT + 3, 34, rng)
    w.fill(47, HALL, 31, 49, SUMMIT + 2, 33, 0)                    # the shaft down
    for y in (SUMMIT, SUMMIT + 1):
        w.set(48, y, 34, 0 if solved else RED_DOOR)
    # The gallery: x 46-50, z 31-59, walk 8, six high, lined with machine panels.
    w.fill(46, HALL, 31, 50, HALL + 5, 59, 0)
    for z in (38, 50):                                             # two curtains of lightning
        for x in range(46, 51):
            w.set(x, HALL - 1, z, RED_DOT)
            w.set(x, HALL + 6, z, BLITZ_EMITTER)
            if not timed:
                for y in range(HALL, HALL + 6):
                    w.set(x, y, z, BLITZ)
    for x in (46, 50):                                             # the mine gate, three wide
        stone(w, x, HALL, 43, x, HALL + 5, 45, rng)
    if not timed:
        for x in (47, 48, 49):
            patrol(w, O.MOVING_MINE, (x, HALL, 44), (x, HALL + 4, 44), ticks=40, stop=0)
    w.fill(51, HALL, 54, 53, HALL + 2, 56, 0)                      # the key's alcove
    w.place(O.KEY3, 53, HALL, 55)
    # The blue door and the exit room of star tiles (x 46-54, z 61-70).
    for y in (HALL, HALL + 1):
        w.set(48, y, 60, 0 if solved else BLUE_DOOR)
    w.fill(46, HALL, 61, 54, HALL + 4, 70, 0)
    skin(w, (46, HALL - 1, 60, 54, HALL + 5, 71), (STARS,), rng)
    skin(w, (45, HALL - 1, 30, 54, HALL + 7, 59), MACHINE, rng)
    if not timed:
        patrol(w, O.MOVING_MINE, (46, HALL, 64), (54, HALL, 64), ticks=30, stop=0)
        patrol(w, O.MOVING_MINE, (54, HALL, 67), (46, HALL, 67), ticks=30, stop=0)
    w.place(O.EXIT, 50, HALL, 69)


def main(argv) -> int:
    stage = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    timed = stage in ("timed", "power", "solved")
    power = stage in ("power", "solved")
    solved = stage == "solved"
    rng = random.Random(32)
    w = World(mission=32, sky=8)
    street(w, rng, timed)
    mountain(w, rng, timed, power)
    inside(w, rng, timed, solved)
    w.spawn = SPAWN
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world032.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
