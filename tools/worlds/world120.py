#!/usr/bin/env python3
"""World 12's hub, the brick tower (worlds3d/world120.vwr): brief plans/briefs/world120.md.

Mobile Eggbert's hub of world 12 is a little tower of brick (261-263, 283) on stone posts (76):
Blupi starts on its lowest floor between mission 1's sign (open) and mission 4's; a floor up,
missions 2 and 5; at the top, mission 3, each later sign behind its blue bar. Here the tower's
floors step back as they rise, each a jump above the last, standing in the sky on piers of brick:
the ground floor with signs 1 (east) and 4 (west), the first floor with 2 (east) and 5 (west),
the top floor with 3.

    python3 tools/worlds/world120.py         # writes worlds3d/world120.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, sign_niche  # noqa: E402
from station import BRICK, brick  # noqa: E402

GROUND, FIRST, TOP = 20, 22, 24            # their tops: walk 21, 23, 25


def main() -> int:
    rng = random.Random(120)
    w = World(mission=120, sky=24)

    # The floors: the ground floor (x 42-58, z 44-58), the first (z 44-51) and the top (z 44-47)
    # stepping back north, each two higher; piers of brick under the ground floor's corners.
    brick(w, 42, GROUND - 1, 44, 58, GROUND, 58, rng)
    brick(w, 42, GROUND + 1, 44, 58, FIRST, 51, rng)
    brick(w, 44, FIRST + 1, 44, 56, TOP, 47, rng)
    for x, z in ((43, 45), (57, 45), (43, 57), (57, 57)):
        brick(w, x, 8, z, x, GROUND - 2, z, rng)
    # Signs: 1 open at the ground floor's east end, 4 at its west; 2 and 5 at the first floor's
    # ends; 3 on the top.
    for x, y, z, facing, n in ((59, GROUND + 1, 54, (-1, 0), 1), (41, GROUND + 1, 54, (1, 0), 4),
                               (59, FIRST + 1, 49, (-1, 0), 2), (41, FIRST + 1, 49, (1, 0), 5),
                               (50, TOP + 1, 43, (0, 1), 3)):
        sign_niche(w, x, y, z, facing, MISSION_SIGN1 + n - 1, rng, door=n > 1, block=BRICK)
    # A low parapet along the ground floor's south edge.
    brick(w, 42, GROUND + 1, 58, 58, GROUND + 1, 58, rng)
    w.spawn = (55, GROUND + 1, 54)
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world120.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
