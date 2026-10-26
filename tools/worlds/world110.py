#!/usr/bin/env python3
"""World 11's hub, the machine pit (worlds3d/world110.vwr): brief plans/briefs/world110.md.

Mobile Eggbert's hub of world 11 is a cave in a block of the blue machine panel (10), widening
upward in steps: Blupi starts on its narrow floor beside mission 1's sign; a step up, missions 2
and 4 face each other across it behind their blue bars; another step up, missions 3 and 5. Here
the cave is a pit in a block of panel open to the machine sky, its sides stepping out as they
rise: the floor with sign 1, the ring a jump above with signs 2 (east) and 4 (west), the next
ring with signs 3 (east) and 5 (west).

    python3 tools/worlds/world110.py         # writes worlds3d/world110.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, MISSION_SIGN1, World, sign_niche, wall  # noqa: E402

FLOOR, RING1, RING2 = 20, 22, 24        # tops: walk 21, 23, 25


def main() -> int:
    rng = random.Random(110)
    w = World(mission=110, sky=0)

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.15, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    # The block and its pit: the floor (x 47-53, z 47-53), each ring one cell wider all round and
    # two higher, open above.
    panel(36, 14, 36, 64, RING2 + 4, 64)
    carve(47, FLOOR + 1, 47, 53, RING2 + 4, 53)
    carve(45, RING1 + 1, 45, 55, RING2 + 4, 55)
    carve(43, RING2 + 1, 43, 57, RING2 + 4, 57)
    # Signs: 1 open on the floor's east side; 2 and 4 on the first ring, east and west; 3 and 5 on
    # the second, east and west.
    for x, y, z, facing, n in ((54, FLOOR + 1, 50, (-1, 0), 1), (56, RING1 + 1, 50, (-1, 0), 2),
                               (58, RING2 + 1, 50, (-1, 0), 3), (44, RING1 + 1, 50, (1, 0), 4),
                               (42, RING2 + 1, 50, (1, 0), 5)):
        sign_niche(w, x, y, z, facing, MISSION_SIGN1 + n - 1, rng, door=n > 1)
    # Machine parts round the top's rim, a low wall round its open edge.
    for x in range(43, 58):
        for z in (42, 58):
            w.set(x, RING2 + 1, z, rng.choice(MACHINE_FACES))
    w.spawn = (49, FLOOR + 1, 50)
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world110.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
