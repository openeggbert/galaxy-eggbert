"""The multiplayer worlds' shared pieces (Speedy Blupi 2's network missions, DATA/WORLD201-212):
a closed hall, the four coloured starts and each team's personal bombs. Everything places the
game's existing tiles and objects only; the halls are original 3D designs carrying each mission's
identity (its region's tiles, its music, its vehicles), as every Galaxy world.

The four starts are the editor's player starts (PlayerStart.hpp): yellow, orange, blue, green, the
original's teams 0-3 (m_blupiStartPos[m_team], decblock.cpp:1349-1353). A start's `turns` are
quarter turns: 0 faces -Z, 1 -X, 2 +Z, 3 +X.
"""

from __future__ import annotations

from typing import Callable

from kit import World

TEAM_BOMBS = (200, 201, 202, 203)  # TYPE_BOMBEPERSO1-4: each team's own bomb


def closed_hall(w: World, x0: int, z0: int, x1: int, z1: int, top: int, roof: int,
                fill: Callable[[int, int, int, int, int, int], None]):
    """A hall whose inside is x0-x1, z0-z1 (floor top `top`, so Blupi walks at top + 1), walled
    and roofed at `roof` with `fill(x0, y0, z0, x1, y1, z1)`: closed, so no helicopter leaves it
    (the flight barrier lets them through)."""
    fill(x0 - 1, top - 1, z0 - 1, x1 + 1, top, z1 + 1)
    fill(x0 - 1, roof, z0 - 1, x1 + 1, roof, z1 + 1)
    for wx0, wz0, wx1, wz1 in ((x0 - 1, z0 - 1, x1 + 1, z0 - 1), (x0 - 1, z1 + 1, x1 + 1, z1 + 1),
                               (x0 - 1, z0 - 1, x0 - 1, z1 + 1), (x1 + 1, z0 - 1, x1 + 1, z1 + 1)):
        fill(wx0, top + 1, wz0, wx1, roof - 1, wz1)


def starts(w: World, places):
    """The four coloured starts: (x, y, z, turns) for yellow, orange, blue, green."""
    for color, (x, y, z, turns) in enumerate(places):
        w.start(color, x, y, z, turns)


def team_bombs(w: World, places, count: int, step):
    """Each team's `count` bombs in a row from beside its start, `step` (dx, dz) apart."""
    for team, (x, y, z, _) in zip(TEAM_BOMBS, places):
        dx, dz = step(x, z)
        for k in range(count):
            w.place(team, x + dx * (2 + k), y, z + dz * (2 + k))
