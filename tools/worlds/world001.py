#!/usr/bin/env python3
"""The main hub (worlds3d/world001.vwr): brief plans/briefs/world001.md.

Mobile Eggbert's main hub is a machine shaft Blupi climbs level by level, a world sign on each
side of every level, and at its top two rows of gold bars (183) before the exit to the bonus
world 199: one bar opens per completed world. Above them the blocks spell DADA. Here the shaft
is a stepped machine ziggurat: seven tiers two blocks apart (jumpable), the twelve world signs
in niches on its south face, paired per tier as the original pairs them per level (bottom:
world 12 left, world 1 right, ...), the twelve gold bars in a tunnel on the summit leading to
the exit, DADA above it. All worlds are open from the start, as in the original.

    python3 tools/worlds/world001.py         # writes worlds3d/world001.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import (GOLD_DOOR, MACHINE_FACES, MACHINE_FLOOR, MONITOR, O, SHOWCASE_SIGN, WORLD_SIGNS,  # noqa: E402
                 World, boundary, props, rect, sign_niche, wall)
from worldkit import letter_cells  # noqa: E402

TIERS = 7                         # tier k: square LO + 3k .. HI - 3k, top at y 2k
LO, HI = 24, 76
# Per tier, bottom to top, the original's (left, right) world signs (worlds/world001.txt).
PAIRS = [(12, 1), (11, 3), (9, 4), (2, 7), (5, 10), (6, 8)]
SPAWN = (50, 1, 75)


def tier_bounds(k):
    return LO + 3 * k, HI - 3 * k


def main() -> int:
    rng = random.Random(1)
    w = World(mission=1, sky=0)
    for k in range(TIERS):
        lo, hi = tier_bounds(k)
        top = 2 * k
        for y in range(max(0, top - 1), top + 1):
            wall(w, lo, y, lo, hi, y, hi, rng, face_chance=0.12 if y == top else 0.35)
        for x, z in rect(lo, lo, hi, hi):
            w.set(x, top, z, MACHINE_FLOOR)
    # A parapet round the bottom tier, the only fall that would leave the world.
    for x, z in boundary(rect(LO, LO, HI, HI)):
        w.set(x, 1, z, rng.choice(MACHINE_FACES) if rng.random() < 0.3 else MACHINE_FLOOR)

    # World signs: tier k's pair in the south face of tier k + 1, at tier k's walking height.
    for k, (left, right) in enumerate(PAIRS):
        _, hi = tier_bounds(k + 1)
        for world, x in ((left, 45), (right, 55)):
            sign_niche(w, x, 2 * k + 1, hi, (0, 1), WORLD_SIGNS[world], rng)
            w.set(x - 1, 2 * k + 2, hi, MONITOR)   # a lit panel over each world's niche
    # Galaxy's own showcase portal, on the bottom tier's east side.
    _, hi1 = tier_bounds(1)
    sign_niche(w, hi1, 1, 50, (1, 0), SHOWCASE_SIGN, rng)

    # The summit (walk 13): the gold-bar tunnel along z 44 from its entrance at x 57 to the
    # exit at x 43; bars at x 45-56 open from the entrance inwards (HubDoors.hpp's order).
    top = 2 * (TIERS - 1) + 1
    for x in range(42, 58):
        for z in (43, 45):
            w.set(x, top, z, rng.choice(MACHINE_FACES) if rng.random() < 0.3 else MACHINE_FLOOR)
        for z in (43, 44, 45):
            w.set(x, top + 1, z, MACHINE_FLOOR)
    w.set(42, top, 44, MACHINE_FLOOR)
    for x in range(45, 57):
        w.set(x, top, 44, GOLD_DOOR)
    w.place(O.EXIT, 43, top, 44)
    # DADA, the original's signature, standing above the tunnel and read from the south.
    cells, width = letter_cells("DADA")
    left = 50 - width // 2
    for col, row in cells:
        w.set(left + col, top + 3 + 6 - row, 43, MACHINE_FLOOR)
    lo6, hi6 = tier_bounds(TIERS - 1)
    props(w, [c for c in rect(lo6 + 1, 48, hi6 - 1, hi6 - 1) if c != (50, 50)], top, rng, chance=0.06)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world001.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
