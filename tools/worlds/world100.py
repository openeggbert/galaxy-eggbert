#!/usr/bin/env python3
"""World 10's hub, the cobbles (worlds3d/world100.vwr): brief plans/briefs/world100.md.

Mobile Eggbert's hub of world 10 is a heap of grey cobbled blocks (35 and its edge pieces) on
little posts in a dark cobbled sky: Blupi starts beside mission 1's sign, missions 2-7 sit on
the blocks round it and above it, each behind its blue bar, mission 7's at the top. Here the heap
is three tiers of cobbled terraces floating in that sky: the plaza with signs 1 (open), 2, 3 and
5 round it, the terrace a jump above it with sign 4, the west block above that with sign 6, and
the top, two jumps up the north-east steps, with sign 7.

    python3 tools/worlds/world100.py         # writes worlds3d/world100.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, sign_niche  # noqa: E402
from village import STONE, stone  # noqa: E402

PLAZA, TERRACE, WEST, STEP, TOP = 20, 22, 24, 24, 26   # their tops: walk 21, 23, 25, 25, 27


def main() -> int:
    rng = random.Random(100)
    w = World(mission=100, sky=2)

    # The tiers, each two above the one it steps up from (a jump).
    stone(w, 42, 17, 48, 58, PLAZA, 58, rng)                           # the plaza
    stone(w, 40, 17, 38, 60, TERRACE, 47, rng)                         # the terrace, north
    stone(w, 32, 18, 36, 39, WEST, 47, rng)                            # the west block
    stone(w, 54, 19, 36, 62, STEP, 37, rng)                            # the north-east step
    stone(w, 54, 20, 29, 62, TOP, 35, rng)                             # the top
    # Under the tiers the heap tapers into the sky.
    stone(w, 45, 13, 50, 55, 16, 56, rng)
    stone(w, 44, 14, 40, 56, 16, 46, rng)
    stone(w, 34, 15, 38, 38, 17, 45, rng)
    stone(w, 47, 9, 52, 52, 12, 54, rng)

    # The signs: 1 open on the plaza's east edge by the start; 2 and 3 in its south edge and 5 in
    # its west edge; 4 at the terrace's east end; 6 at the west block's west end; 7 on the top.
    for x, y, z, facing, n in ((59, PLAZA + 1, 53, (-1, 0), 1), (49, PLAZA + 1, 59, (0, -1), 2),
                               (54, PLAZA + 1, 59, (0, -1), 3), (61, TERRACE + 1, 42, (-1, 0), 4),
                               (41, PLAZA + 1, 53, (1, 0), 5), (31, WEST + 1, 41, (1, 0), 6),
                               (58, TOP + 1, 28, (0, 1), 7)):
        sign_niche(w, x, y, z, facing, MISSION_SIGN1 + n - 1, rng, door=n > 1, block=STONE)

    # Low cobbled walls along the open edges (a fall from the heap is the void).
    stone(w, 43, PLAZA + 1, 58, 47, PLAZA + 1, 58, rng)
    stone(w, 57, PLAZA + 1, 58, 58, PLAZA + 1, 58, rng)
    stone(w, 32, WEST + 1, 36, 39, WEST + 1, 36, rng)
    stone(w, 62, TOP + 1, 30, 62, TOP + 1, 35, rng)
    # Cobbled blocks standing on the terrace and the top, the heap's knobbly outline.
    for x0, y0, z0, x1, y1, z1 in ((44, TERRACE + 1, 38, 46, TERRACE + 2, 39),
                                   (51, TERRACE + 1, 38, 52, TERRACE + 3, 38),
                                   (55, TOP + 1, 31, 56, TOP + 2, 31),
                                   (33, WEST + 1, 46, 34, WEST + 2, 47)):
        stone(w, x0, y0, z0, x1, y1, z1, rng)

    w.spawn = (55, PLAZA + 1, 53)
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world100.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
