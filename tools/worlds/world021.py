#!/usr/bin/env python3
"""Mission 021, the palace garden (worlds3d/world021.vwr): brief plans/briefs/world021.md.

Mobile Eggbert's level 021 is world 2's first lesson: treasure doors. On the grass before the
palace stand three stepped white towers; their treasures lie in an arch at a tower's foot, on
balconies beside saws and on a slim column reached by a lift; golden bars marked 2, 5 and 6
open only once Blupi carries that many treasures; inside the palace he climbs to the battlements
for the sixth, and the bar marked 6 shuts the room of the red arrow. Here the same garden in 3D:
three stepped towers on the grass (the first with a treasure in an arch at its foot and one on
its top over a ring of saws, the second with its treasure in a kiosk behind the bar 2, the third
with one between saws and the lift to the column), the palace gatehouse behind the bar 5, a
staircase up the palace court to the battlements and the north-east tower with the sixth
treasure, and the arrow's room behind the bar 6.

    python3 tools/worlds/world021.py                   # writes worlds3d/world021.vwr
    python3 tools/worlds/world021.py --stage N out.vwr # the treasure doors up to N open (levelcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, rect  # noqa: E402
from palace import (ARCH, COLUMN, FOUNTAIN, GRASS, PALMS, PLANTS, SAW, STATUE, STONE, gate,  # noqa: E402
                    parapet, stone, treasure_door, windows_row)

SPAWN = (44, 1, 77)


def tower(w, cx, cz, half, tiers, rng):
    """A stepped tower: tier k (from 1) fills y 2k-1 .. 2k and is `half` - 2(k-1) cells from its
    centre, so each tier is a two-block jump above the ring of the one below."""
    for k in range(1, tiers + 1):
        h = half - 2 * (k - 1)
        stone(w, cx - h, 2 * k - 1, cz - h, cx + h, 2 * k, cz + h, rng, chance=0.3)
        windows_row(w, cx - h + 1, cx + h - 1, 2 * k - 1, cz + h, rng, chance=0.4)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 2 and argv[1] == "--stage" else None
    rng = random.Random(21)
    w = World(mission=21, sky=25)

    def door(x, y, z, n):
        if stage is None or n > stage:
            gate(w, x, y, z, treasure_door(n))

    # The ground: grass, white paths, the garden walls (5 high) and the palace walls (10 high).
    w.fill(22, 0, 10, 78, 0, 82, GRASS)
    for x, z in rect(43, 41, 45, 79) + rect(30, 73, 66, 75) + rect(46, 52, 56, 54):
        w.set(x, 0, z, STONE)
    stone(w, 22, 1, 36, 23, 5, 82, rng)
    stone(w, 77, 1, 36, 78, 5, 82, rng)
    stone(w, 22, 1, 81, 78, 5, 82, rng)

    # 1. Tower 1: a treasure in the arch at its foot, one on its top over a ring of saws.
    tower(w, 34, 66, 5, 3, rng)
    for x in (31, 34, 37):
        w.set(x, 1, 71, ARCH)                       # arches in the foot, stone behind them
    w.place(O.TREASURE, 34, 1, 71)
    for x, z in ((34, 68), (36, 66), (32, 66), (34, 64)):
        w.set(x, 4, z, SAW)                         # in the balcony round the top tier
    w.place(O.TREASURE, 34, 7, 66)

    # 2. Tower 2: its treasure in a kiosk on the second tier behind the bar marked 2; saws in
    #    front of the kiosk's door.
    tower(w, 62, 60, 5, 2, rng)
    stone(w, 62, 5, 59, 64, 7, 61, rng, chance=0.2)
    w.clear(63, 5, 60)
    w.clear(63, 6, 60)
    w.clear(62, 5, 60)
    door(62, 5, 60, 2)                              # crossed along X from (61, 5, 60)
    w.place(O.TREASURE, 63, 5, 60)
    for z in (59, 60, 61):
        w.set(60, 4, z, SAW)
    w.place(O.EGG, 66, 3, 56)

    # 3. Tower 3: a treasure between four saws on its top; on its east side the lift to the
    #    slim column with the fourth treasure, a saw on the column's other cell.
    tower(w, 34, 45, 5, 2, rng)
    w.place(O.TREASURE, 34, 5, 45)
    for x, z in ((33, 45), (35, 45), (34, 44), (34, 46)):
        w.set(x, 4, z, SAW)
    stone(w, 39, 3, 45, 39, 8, 46, rng, chance=0.3)  # the column on the foot's east edge
    w.set(39, 8, 46, SAW)
    lift(w, (38, 4, 45), (38, 8, 45), ticks=40, stop=10)
    w.place(O.TREASURE, 39, 9, 45)

    # The garden's decoration: palms, plants and fountains.
    for icon, x, z in ((PALMS[0], 27, 77), (PALMS[1], 72, 77), (PALMS[0], 72, 44), (PALMS[1], 27, 56),
                       (PLANTS[0], 50, 70), (PLANTS[1], 40, 58), (PLANTS[2], 70, 68), (PLANTS[0], 55, 45)):
        w.bigdecor(icon, x, 1, z)
    for x, z in ((50, 62), (26, 40)):
        w.set(x, 1, z, STONE)
        w.bigdecor(FOUNTAIN, x, 2, z)

    # The palace: walls ten high round an open court (x 26-74, z 14-32), its facade (z 33-35)
    #    with windows; the gatehouse before it, entered along X through the bar marked 5.
    stone(w, 24, 1, 12, 76, 9, 13, rng)             # north
    stone(w, 24, 1, 12, 25, 9, 35, rng)             # west
    stone(w, 75, 1, 12, 76, 9, 35, rng)             # east
    stone(w, 24, 1, 33, 76, 9, 35, rng, chance=0.25)  # the facade
    for y in (3, 6):
        windows_row(w, 26, 74, y, 35, rng, chance=0.5)
    w.fill(26, 0, 14, 74, 0, 32, STONE)             # the court's floor
    stone(w, 46, 1, 36, 54, 4, 40, rng, chance=0.2)  # the gatehouse, open on top
    w.fill(47, 1, 37, 53, 4, 39, 0)
    w.clear(46, 1, 38)
    door(46, 1, 38, 5)                              # crossed along X from (45, 1, 38)
    w.fill(49, 1, 33, 51, 3, 35, 0)                 # through the facade into the court
    w.fill(49, 1, 36, 51, 3, 36, 0)
    w.place(O.EGG, 50, 1, 38)

    # The court: a fountain in the middle with an egg on its plinth; a staircase of
    #    two-block steps up its east side to the battlements, saws on two steps.
    w.set(55, 1, 24, STONE)
    w.bigdecor(FOUNTAIN, 55, 2, 24)
    w.set(56, 1, 24, STONE)
    w.place(O.EGG, 56, 2, 24)
    steps = [(28, 31, 2), (24, 27, 4), (20, 23, 6), (16, 19, 8)]
    for z0, z1, top in steps:
        stone(w, 70, 1, z0, 74, top, z1, rng, chance=0.3)
    w.set(71, 4, 26, SAW)
    w.set(73, 6, 21, SAW)
    # The battlements: the north wall's top (walk 10), a parapet on its outer edge; the
    #    north-east tower two higher with the sixth treasure and the statue; fountains with
    #    eggs on the battlements.
    stone(w, 37, 9, 14, 74, 9, 15, rng)             # the walk widened over the court
    parapet(w, [(x, 12) for x in range(37, 69)], 10, rng)
    stone(w, 69, 1, 12, 76, 11, 18, rng, chance=0.3)  # the north-east tower, walk 12
    w.place(O.TREASURE, 72, 12, 14)
    w.bigdecor(STATUE, 74, 12, 16)
    for x in (44, 58):
        w.set(x, 10, 14, STONE)
        w.bigdecor(FOUNTAIN, x, 11, 14)
        w.set(x + 1, 10, 14, STONE)
        w.place(O.EGG, x + 1, 11, 14)
    stone(w, 36, 1, 12, 36, 12, 32, rng)            # the battlements end at the arrow's room

    # The red arrow's room (x 26-35), roofed, entered along X through the bar marked 6.
    stone(w, 26, 5, 14, 35, 5, 32, rng, chance=0.1)
    w.clear(36, 1, 23)
    w.clear(36, 2, 23)
    door(36, 1, 23, 6)                              # crossed along X from (37, 1, 23)
    for x, z in rect(28, 20, 33, 26):
        w.set(x, 0, z, GRASS)
    w.set(30, 1, 23, STONE)
    w.place(O.EXIT, 30, 2, 23)
    w.bigdecor(PALMS[0], 29, 1, 21)
    w.bigdecor(PALMS[1], 31, 1, 25)
    for x, z in ((27, 15), (34, 15), (27, 31), (34, 31)):
        w.bigdecor(COLUMN, x, 1, z)

    w.spawn = SPAWN
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world021.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
