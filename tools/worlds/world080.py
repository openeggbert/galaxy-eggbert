#!/usr/bin/env python3
"""World 8's hub, the sky works (worlds3d/world080.vwr): brief plans/briefs/world080.md.

Mobile Eggbert's hub of world 8 is a small machine of cyan panels and blue pipes floating in the
sky: below, mission 1's sign (open) right of Blupi's start and mission 3's left of it behind its
bar; above, missions 2 and 4 behind theirs. Here it is a deck on two levels in the open sky: the
lower deck with the pods of 3 and 1 at its west and east ends, the upper deck two higher north
of it with the pods of 4 and 2, pipe parapets round the edges, pipe columns and a tank of pipe
machinery on the upper deck, a pipe under the decks.

    python3 tools/worlds/world080.py         # writes worlds3d/world080.vwr
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MISSION_SIGN1, World, rect, sign_niche  # noqa: E402
from skyworks import BOLTS, PANEL, PIPES, deck, panel, pipe_column, pipe_rail  # noqa: E402

LOW, HIGH = 20, 22          # the decks' tops: walk 21 and walk 23


def main() -> int:
    rng = random.Random(80)
    w = World(mission=80, sky=26)

    # The decks: the lower (x 40-60, z 49-57), the upper two higher north of it (x 42-58, z 41-48),
    # each two thick, the step between them a jump.
    deck(w, 40, 49, 60, 57, LOW, thickness=2)
    deck(w, 42, 41, 58, 48, HIGH, thickness=4)
    # The pods: 3 west and 1 east on the lower deck, 4 west and 2 east on the upper.
    for x, z, y, facing, n in ((41, 53, LOW + 1, (1, 0), 3), (59, 53, LOW + 1, (-1, 0), 1),
                               (43, 44, HIGH + 1, (1, 0), 4), (57, 44, HIGH + 1, (-1, 0), 2)):
        sign_niche(w, x, y, z, facing, MISSION_SIGN1 + n - 1, rng, door=n > 1, block=PANEL, faces=())
    # Pipe parapets round the outer edges (two high on the south, where the drop faces the camera's
    # back), gaps where the pods stand.
    pipe_rail(w, [(x, 57) for x in range(40, 61)], LOW + 1, rng, height=2)
    for z in range(49, 57):
        if not 51 <= z <= 55:
            pipe_rail(w, [(40, z), (60, z)], LOW + 1, rng)
    pipe_rail(w, [(x, 41) for x in range(42, 59)], HIGH + 1, rng)
    for z in range(41, 48):
        if not 42 <= z <= 46:
            pipe_rail(w, [(42, z), (58, z)], HIGH + 1, rng)
    # On the upper deck: a tank of pipe machinery between two pipe columns, a pipe across their tops.
    panel(w, 48, HIGH + 1, 42, 52, HIGH + 3, 43, rng)
    for x in (47, 53):
        pipe_column(w, x, HIGH + 1, HIGH + 5, 42, rng)
    for x in range(47, 54):
        w.set(x, HIGH + 6, 42, rng.choice(PIPES))
    # Bolts and fittings on the decks' edges (small parts, no collision).
    for x in range(41, 60, 3):
        w.set(x, LOW + 1, 56, rng.choice(BOLTS))
    # Under the decks a pipe running east-west and stubs of panel hanging from them.
    for x in range(38, 63):
        w.set(x, LOW - 3, 50, rng.choice(PIPES))
    for x, z in rect(44, 51, 56, 55):
        if (x + z) % 5 == 0:
            w.set(x, LOW - 2, z, PANEL)

    w.spawn = (50, LOW + 1, 54)
    out = Path(__file__).resolve().parents[2] / "worlds3d" / "world080.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
