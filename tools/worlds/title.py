#!/usr/bin/env python3
"""Galaxy Eggbert's title scene: worlds3d/title.vwr, drawn live behind the main menu.

A floating grass island in the starfield (background region 7) with Blupi, his jeep and
helicopter, a treasure chest and an egg; colourful toy balls float around like planets; behind
them "GALAXY" / "EGGBERT" stands in yellow cheese blocks. Everything is Mobile Eggbert's own
tiles and objects, arranged in 3D: no copied artwork. The game frames it from GalaxyEggbertGame::
UpdateTitleScene(): eye 5 blocks in front of Blupi at his eye height, looking 3.5 degrees up, so
Blupi fills the lower middle and the title (rows 45-59 at z 96) the band above the menu panels.

    python3 tools/worlds/title.py            # writes worlds3d/title.vwr
"""

import math
import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from worldkit import ObjectType as O, World, letter_cells  # noqa: E402

GRASS = (107, 108, 109)     # grass-topped earth
GRASS_EDGE = 157            # grass on a rock
EARTH = (284, 301, 300)     # brown earth and rock
TUFTS = (203, 204, 205, 206, 207, 208)
LETTER = 246                # yellow cheese block
PLANETS = (215, 216, 217, 218, 219)

BLUPI = (50, 31, 40)        # spawn cell: stands on the island's top at y 30


def island(w: World, cx: int, top: int, cz: int, rx: float, rz: float, rng: random.Random):
    for x in range(cx - int(rx) - 1, cx + int(rx) + 2):
        for z in range(cz - int(rz) - 1, cz + int(rz) + 2):
            d = ((x - cx) / rx) ** 2 + ((z - cz) / rz) ** 2
            if d > 1.0:
                continue
            w.set(x, top, z, GRASS_EDGE if d > 0.82 else rng.choice(GRASS))
            # The underside narrows into a rocky point, deeper in the middle.
            depth = int(round(1 + 5 * (1.0 - d) ** 1.5))
            for y in range(top - depth, top):
                w.set(x, y, z, rng.choice(EARTH))
            if 0.15 < d < 0.75 and rng.random() < 0.18 and (x, z) != (BLUPI[0], BLUPI[2]):
                w.set(x, top + 1, z, rng.choice(TUFTS))


def title_letters(w: World, text: str, bottom: int, z: int, centre_x: int = 50, depth: int = 2):
    cells, width = letter_cells(text)
    # The camera looks along +Z, so the screen's right is -X: columns run toward -X.
    left = centre_x + width // 2
    for col, row in cells:
        for d in range(depth):
            w.set(left - col, bottom + 6 - row, z + d, LETTER)


def main() -> int:
    rng = random.Random(1998)
    w = World(mission=0, sky=7)
    island(w, 50, 30, 42, 9.5, 6.5, rng)
    # Two small islands drifting nearby.
    island(w, 36, 27, 52, 3.2, 2.6, rng)
    island(w, 65, 33, 56, 3.6, 2.8, rng)
    for x, y, z in ((50, 31, 40),):
        w.clear(x, y, z)
    w.spawn = BLUPI

    w.place(O.JEEP, 47, 31, 47)
    w.place(O.HELICOPTER, 53, 31, 47)
    w.place(O.TREASURE, 48, 31, 43)
    w.place(O.EGG, 52, 31, 43)
    w.place(O.TREASURE, 36, 28, 52)

    title_letters(w, "GALAXY", 53, 96, depth=2)
    title_letters(w, "EGGBERT", 45, 96)

    # Toy-ball "planets" scattered through the space around the scene.
    for _ in range(22):
        x, y, z = rng.randint(14, 86), rng.randint(14, 64), rng.randint(58, 97)
        if 42 <= y <= 62 and 26 <= x <= 74 and z >= 88:
            continue  # keep the title clear
        w.set(x, y, z, rng.choice(PLANETS))
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "title.vwr"
    w.save(out)
    print(f"wrote {out}: {len(w.blocks)} blocks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
