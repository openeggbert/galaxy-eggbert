#!/usr/bin/env python3
"""World 1's hub, the blue machine world (worlds3d/world010.vwr): brief plans/briefs/world010.md.

Mobile Eggbert's hub of world 1 is a small machine room on two levels: below, mission 1's
sign (open) and mission 3's behind its bar; above, missions 2 and 4 behind theirs; Blupi starts
between 1 and 3. A mission's bar slides up once the mission before it is won, and Blupi then
starts in front of the last bar opened. Here the room is a machine hall: the lower signs in
the face of a gallery along its north side, the upper ones in the north wall above the
gallery, which Blupi reaches with a jump.

    python3 tools/worlds/world010.py         # writes worlds3d/world010.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import (MACHINE_FACES, MACHINE_FLOOR, MACHINE_PROPS, MISSION_SIGN1, MONITOR, World,  # noqa: E402
                 props, rect, sign_niche, slab, wall)

X0, X1, Z0, Z1 = 40, 60, 40, 58     # the hall, walls included
GALLERY = (43, 45)                  # z range of the gallery, top y 2 (walk 3)
SPAWN = (50, 1, 53)


def main() -> int:
    rng = random.Random(10)
    w = World(mission=10, sky=0)
    slab(w, X0, Z0, X1, Z1, 0)                                         # floor, walk 1
    wall(w, X0, 1, Z0, X1, 6, Z0 + 2, rng, face_chance=0.3)            # north wall, 3 thick
    wall(w, X0, 1, Z0, X0 + 1, 4, Z1, rng, face_chance=0.3)            # west
    wall(w, X1 - 1, 1, Z0, X1, 4, Z1, rng, face_chance=0.3)            # east
    wall(w, X0, 1, Z1 - 1, X1, 2, Z1, rng, face_chance=0.3)            # south, low: the camera looks over it
    wall(w, X0 + 2, 1, GALLERY[0], X1 - 2, 2, GALLERY[1], rng, face_chance=0.25)  # the gallery
    # Mission signs as the original places them: 3 left and 1 right below, 4 and 2 above.
    sign_niche(w, 45, 1, GALLERY[1] - 1, (0, 1), MISSION_SIGN1 + 2, rng, door=True)
    sign_niche(w, 55, 1, GALLERY[1], (0, 1), MISSION_SIGN1, rng)
    sign_niche(w, 45, 3, Z0 + 1, (0, 1), MISSION_SIGN1 + 3, rng, door=True)
    sign_niche(w, 55, 3, Z0 + 1, (0, 1), MISSION_SIGN1 + 1, rng, door=True)
    for x, y, z in ((44, 2, 45), (56, 2, 45), (44, 4, 42), (56, 4, 42), (50, 4, 42), (50, 2, 45)):
        w.set(x, y, z, MONITOR)
    props(w, rect(X0 + 2, Z1 - 3, X1 - 2, Z1 - 2), 1, rng, chance=0.15, kinds=MACHINE_PROPS)
    props(w, [(x, 46) for x in range(X0 + 2, X1 - 1) if abs(x - 45) > 1 and abs(x - 55) > 1], 1, rng, chance=0.2)
    w.spawn = SPAWN
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world010.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
