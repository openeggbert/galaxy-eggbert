#!/usr/bin/env python3
"""World 5's hub, the space station's lift tower (worlds3d/world050.vwr): brief
plans/briefs/world050.md.

Mobile Eggbert's hub of world 5 is a lift shaft in grey riveted plates against black space:
Blupi starts at its foot beside mission 1's sign; a lift rises eight cells past the signs of
missions 2, 3 and 4 (each two rows higher, behind its blue bar, on a ledge beside the lift) to
mission 5's sign; from there a second lift rises six cells past 6 and 7 to 8. Blupi steps off a
lift onto the ledge he wants. Here a tower of plates stands on a station deck in space: lift 1
rides up its west face from the deck to its roof, lift 2 up its east side from the roof to a
platform above; round each lift balconies on different sides at different heights, three cells
apart (out of a jump's reach), each with a sign in a kiosk at its outer end. Blupi rides up and
jumps off at the balcony he wants; a balcony he misses is a fall to the deck.

    python3 tools/worlds/world050.py         # writes worlds3d/world050.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, boundary, lift, rect  # noqa: E402
from station import (BALL, GIRDER, GRILLE, PARTS, PILLARS, PLATE_EDGES, girder_mast, lift_ticks,  # noqa: E402
                     metal, metal_cells, plate, sign_niche)

DECK = 10                 # the deck's top: Blupi walks at 11
WALK = DECK + 1
STEP = 3                  # three cells between sign levels: out of a jump's reach (two)
LEVEL = [WALK + STEP * k for k in range(8)]          # signs 1-8: walk 11, 14, ... 32
ROOF = LEVEL[4]           # walk 23: the tower's roof, lift 1's top, lift 2's foot, sign 5
CORE = (46, 47, 53, 52)   # the tower (x0, z0, x1, z1), from the deck to the roof
LIFT1 = rect(44, 49, 45, 50)   # 2 x 2 lifts in step, beside the tower's west face
LIFT2 = rect(54, 49, 55, 50)   # ... and its east face, from the roof up
SPAWN = (49, WALK, 58)


def balcony(w, cells, walk, rng):
    """A one-thick floor of plates whose top is walk - 1."""
    metal_cells(w, cells, walk - 1, walk - 1, rng)


def main() -> int:
    rng = random.Random(50)
    w = World(mission=50, sky=7)

    # The deck: a station plate floating in space, two thick, a parapet round its edge; under
    # it the hull narrows in two steps.
    deck = rect(37, 40, 62, 62)
    metal_cells(w, deck, DECK - 1, DECK, rng)
    metal(w, 40, DECK - 4, 43, 59, DECK - 2, 59, rng)
    metal(w, 44, DECK - 7, 47, 55, DECK - 5, 55, rng)
    for x, z in boundary(deck):
        w.set(x, WALK, z, BALL if (x + z) % 5 == 0 else plate(rng))
    # A strip of grilles across the deck from the start to sign 1, masts at the deck's corners,
    # pillars along its south edge, plate edges as clutter.
    for z in range(53, 61):
        for x in (48, 49, 50):
            w.set(x, DECK, z, GRILLE)
    for x, z in ((39, 42), (60, 42), (39, 60), (60, 60)):
        girder_mast(w, x, z, WALK, WALK + 9)
        w.set(x, WALK + 10, z, GRILLE)
    for x in (42, 45, 54, 57):
        w.set(x, WALK, 61, PILLARS)
    for x, z in ((40, 47), (41, 55), (58, 45), (59, 54), (43, 59), (56, 58), (52, 44)):
        w.set(x, WALK, z, PLATE_EDGES[(x + z) % 2])

    # The tower: plates with machine parts and grilles on its faces, from the deck to its roof.
    x0, z0, x1, z1 = CORE
    metal(w, x0, WALK, z0, x1, ROOF - 1, z1, rng, grille=0.06)
    for y in range(WALK + 1, ROOF - 1, 2):
        for x, z in ((x0 + 2, z1), (x1 - 1, z1), (x1, z0 + 2), (x0 + 3, z0)):
            if rng.random() < 0.6:
                w.set(x, y, z, rng.choice(PARTS + (BALL,)))
    # Sign 1 in the tower's south face, before the start.
    sign_niche(w, 49, WALK, z1, (0, 1), MISSION_SIGN1, rng)

    # Lift 1 rises from the deck (its cube in a pit one deep) twelve cells to the roof; lift 2
    # from the roof nine cells to the top, at the original's pace of ten ticks a cell.
    for x, z in LIFT1:
        w.clear(x, DECK, z)
        lift(w, (x, DECK, z), (x, ROOF - 1, z), ticks=lift_ticks(ROOF - WALK), stop=10)
    for x, z in LIFT2:
        lift(w, (x, ROOF - 1, z), (x, LEVEL[7] - 1, z), ticks=lift_ticks(LEVEL[7] - ROOF), stop=10)

    # Lift 1's balconies: 2 west, 3 north, 4 south, each sign at the outer end behind its bar.
    balcony(w, rect(40, 49, 43, 50), LEVEL[1], rng)
    sign_niche(w, 38, LEVEL[1], 49, (1, 0), MISSION_SIGN1 + 1, rng, door=True)
    balcony(w, rect(44, 45, 45, 48), LEVEL[2], rng)
    sign_niche(w, 44, LEVEL[2], 43, (0, 1), MISSION_SIGN1 + 2, rng, door=True)
    balcony(w, rect(44, 51, 45, 54), LEVEL[3], rng)
    sign_niche(w, 44, LEVEL[3], 56, (0, -1), MISSION_SIGN1 + 3, rng, door=True)

    # The roof: sign 5 in a kiosk on its north edge, a parapet of plates round the rest except
    # beside the lifts.
    roof = rect(x0, z0, x1, z1)
    sign_niche(w, 50, ROOF, z0 - 1, (0, 1), MISSION_SIGN1 + 4, rng, door=True)
    for x, z in boundary(roof):
        if x in (x0, x1) and 49 <= z <= 50:
            continue                                   # where the lifts meet the roof
        if z == z0 and 49 <= x <= 51:
            continue                                   # before sign 5's kiosk
        w.set(x, ROOF, z, BALL if (x + z) % 4 == 0 else plate(rng))

    # Lift 2's balconies: 6 north, 7 east, 8 on the top platform south of it.
    balcony(w, rect(54, 45, 55, 48), LEVEL[5], rng)
    sign_niche(w, 55, LEVEL[5], 43, (0, 1), MISSION_SIGN1 + 5, rng, door=True)
    balcony(w, rect(56, 49, 59, 50), LEVEL[6], rng)
    sign_niche(w, 61, LEVEL[6], 49, (-1, 0), MISSION_SIGN1 + 6, rng, door=True)
    balcony(w, rect(53, 51, 57, 54), LEVEL[7], rng)
    sign_niche(w, 55, LEVEL[7], 56, (0, -1), MISSION_SIGN1 + 7, rng, door=True)
    # A beacon on the top platform's corner: a girder mast under a grille.
    girder_mast(w, 57, 54, LEVEL[7], LEVEL[7] + 3)
    w.set(57, LEVEL[7] + 4, 54, GRILLE)

    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world050.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
