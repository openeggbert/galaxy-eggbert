#!/usr/bin/env python3
"""Mission 084, the golden key of the works (worlds3d/world084.vwr): brief plans/briefs/world084.md.

Mobile Eggbert's level 084, the last of world 8, is a machine of panels and pipes under a striped
sky that ends not at the red arrow but at the golden key (21), out on a chain of floating lifts.
From Blupi's start a pipe catwalk with mines under hanging mines, lifts over pits of lava, mines
bobbing by a lift down; a corridor with a mine, a curtain of lightning (305, its emitters 304
above) and a hanging mine, bars over a pit where mines sweep; a hall with a shuttle over water,
a second curtain of lightning and lifts meeting in pairs over lava; a drop to a hall where a
fan's wind blows over a shuttle across water and bars over lava; birds over lava; rows of mines
on a pipe walkway; hanging mines, a lift moving across and up, and the chain of lifts out to the
key over a pool. Fifteen eggs, no treasure. Here the works wind round the sky in 3D: the catwalk
north, the corridor east, the hall north again, the wind hall east, the birds south, the mines
west and the chain out south-east.

    python3 tools/worlds/world084.py                 # writes worlds3d/world084.vwr
    python3 tools/worlds/world084.py --stage 1 out   # for levelcheck: no mines, moving or hanging
                                                     # mines, birds or lightning; floors under the bars
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, WARNING, World, lift, mine  # noqa: E402
from skyworks import BOLTS, PANEL, PIPE_MACHINES, PIPES, panel, pipe_column, pipe_rail  # noqa: E402

LAVA, WATER = 68, 92
ROD = 202                   # a bar along X (ThinBar), seen from every side; 138 is a flat plate
BLITZ, EMITTER = 305, 304
WIND_EAST, FAN_EAST = 114, 129


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world084.vwr"
    rng = random.Random(84)
    w = World(mission=84, sky=22)
    calm = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def still(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    def drifter(start, end, ticks, stop=0):
        if not calm:
            w.place(O.MOVING_MINE, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def bird(start, end, ticks):
        if not calm:
            w.place(O.BIRD, *start, end=end, advance=ticks, recede=ticks, stop_start=8, stop_end=8)

    def lifts(x0, x1, y, z0, z1, offset, ticks, stop):
        """A platform of lifts in step, one per cell, all moving by `offset`."""
        dx, dy, dz = offset
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                lift(w, (x, y, z), (x + dx, y + dy, z + dz), ticks=ticks, stop=stop)

    def lightning(cells, ceiling):
        """A curtain of lightning (305) over `cells` (x, y0, y1, z), its emitters (304) in the
        ceiling above; in the calm stage open air."""
        for x, y0, y1, z in cells:
            w.set(x, ceiling, z, EMITTER)
            if not calm:
                w.fill(x, y0, z, x, y1, z, BLITZ)

    def bars(x0, y, z, x1):
        """Rods along X that Blupi catches walking out over the drop; in the calm stage a floor
        under them."""
        if calm:
            w.fill(x0, y - 1, z, x1, y - 1, z, PANEL)
        else:
            w.fill(x0, y, z, x1, y, z, ROD)

    # ---- the masses ----
    w.fill(8, 58, 56, 12, 59, 94, PANEL)                                # the catwalk's deck
    panel(w, 7, 46, 54, 42, 52, 58, rng, fitting_chance=0.0)            # the corridor's block
    panel(w, 27, 38, 54, 34, 45, 58, rng, fitting_chance=0.0)           # its pit's
    panel(w, 35, 31, 3, 45, 42, 61, rng, fitting_chance=0.0)            # the hall's
    panel(w, 54, 19, 2, 89, 29, 10, rng, fitting_chance=0.0)            # the wind hall's

    # ---- the catwalk (walk 60, south to north, lanes x 9-11) ----
    # Pipe round the deck's flanks and pipe rails along it. Two rows of mines, each under hanging
    # mines a cell above the free lanes (walk past, never jump). Two pits of lava, each crossed on
    # a lift; a one-wide bridge where two mines bob two cells up and down in turn (the cell between
    # them is safe); at its north end the lift down.
    for z in range(56, 95):
        for x in (8, 12):
            w.set(x, 58, z, rng.choice(PIPES))
    pipe_rail(w, [(x, z) for x in (8, 12) for z in range(56, 95)], 60, rng)
    pipe_rail(w, [(x, 94) for x in range(9, 12)], 60, rng)
    pipe_column(w, 13, 50, 68, 93, rng)
    w.spawn = (10, 60, 92)
    w.place(O.EGG, 11, 60, 90)
    still(10, 60, 87)
    still(9, 61, 87, hanging=True)
    still(11, 61, 87, hanging=True)
    still(9, 60, 84)
    still(11, 60, 84)
    still(10, 61, 84, hanging=True)
    for z0 in (75, 65):                                                 # the pits, five long
        carve(9, 57, z0, 11, 59, z0 + 4)
        w.fill(9, 56, z0, 11, 56, z0 + 4, PANEL)
        w.fill(9, 57, z0, 11, 58, z0 + 4, LAVA)
        lifts(9, 11, 59, z0 + 3, z0 + 4, (0, 0, -3), 40, 20)
    w.place(O.EGG, 9, 60, 72)
    carve(8, 58, 60, 9, 60, 62)                                         # the bridge, one wide
    carve(11, 58, 60, 12, 60, 62)
    drifter((10, 60, 62), (10, 62, 62), 20, 20)
    drifter((10, 62, 60), (10, 60, 60), 20, 20)
    carve(9, 47, 56, 10, 59, 57)                                        # the lift down to walk 48
    lifts(9, 10, 59, 56, 57, (0, -12, 0), 90, 30)

    # ---- the corridor (walk 48, west to east, z 55-57) ----
    # Two mines, warning panels, a curtain of lightning across it (on for 2.5 s of every 5), a
    # hanging mine; a pit across it, lava at its foot and two mines sweeping it, crossed hanging
    # from bars; at its east end a hole down into the hall.
    carve(8, 48, 55, 41, 50, 57)
    still(15, 48, 55)
    still(15, 48, 57)
    w.set(18, 49, 54, WARNING)
    w.set(18, 49, 58, WARNING)
    lightning([(20, 48, 50, z) for z in (55, 56, 57)], 51)
    still(24, 49, 56, hanging=True)
    w.place(O.EGG, 26, 48, 57)
    carve(28, 41, 55, 33, 47, 57)
    w.fill(28, 40, 55, 33, 40, 57, LAVA)
    bars(28, 48, 56, 33)
    drifter((28, 45, 55), (33, 45, 55), 40)
    drifter((33, 45, 57), (28, 45, 57), 40)
    panel(w, 37, 43, 54, 41, 45, 58, rng, fitting_chance=0.0)           # the hole's shaft
    carve(38, 41, 55, 40, 47, 57)                                       # the hole down

    # ---- the hall (walk 36, south to north, lanes x 36-44) ----
    # A shuttle over a pool (the water a cell under the floor: fall in and climb out); a pipe
    # floor with eggs on pedestals; a second curtain of lightning across the whole hall; a field
    # of lava crossed on two pairs of lifts meeting midway with a rest between, the last lift
    # carrying Blupi out east through the wall, under the ledge two higher.
    carve(36, 36, 4, 44, 40, 60)
    w.place(O.EGG, 42, 36, 58)
    still(37, 36, 55)
    carve(36, 32, 45, 44, 35, 52)
    w.fill(36, 32, 45, 44, 34, 52, WATER)
    lifts(39, 41, 35, 51, 52, (0, 0, -6), 70, 20)
    for z in range(30, 45):
        for x in (36, 44):
            w.set(x, 35, z, rng.choice(PIPES))
    for x, z in ((37, 38), (43, 34)):
        w.set(x, 36, z, PANEL)
        w.place(O.EGG, x, 37, z)
    w.set(35, 37, 31, WARNING)
    w.set(45, 37, 31, WARNING)
    lightning([(x, 36, 40, 29) for x in range(36, 45)], 41)
    carve(36, 33, 4, 44, 35, 25)                                        # the lava field
    w.fill(36, 33, 4, 44, 34, 25, LAVA)
    w.fill(36, 33, 12, 44, 35, 13, PANEL)                               # the rest
    w.place(O.EGG, 40, 36, 12)
    lifts(39, 41, 35, 24, 25, (0, 0, -4), 50, 30)                       # meet at z 20-21 / 18-19
    lifts(39, 41, 35, 14, 15, (0, 0, 4), 50, 30)
    lifts(39, 41, 35, 10, 11, (0, 0, -4), 50, 30)                       # ends at z 6-7, meeting
    lifts(46, 48, 35, 4, 5, (-7, 0, 0), 50, 30)                         # this one at z 4-5
    carve(45, 35, 4, 45, 39, 5)                                         # its way out east
    w.fill(49, 36, 2, 54, 37, 8, PANEL)                                 # the east ledge, walk 38
    w.place(O.EGG, 53, 38, 3)

    # ---- the wind hall (walk 24, west to east, lanes z 3-9) ----
    # A drop from the ledge's far end through the roof; a fan in the west wall blowing east over a
    # shuttle across a pool and over bars across a trench of lava; the east ledge and a stair down
    # south out of it.
    carve(55, 24, 3, 88, 28, 9)
    carve(55, 29, 3, 57, 29, 7)                                         # the drop
    w.set(54, 27, 6, FAN_EAST)
    carve(61, 21, 3, 71, 23, 9)
    w.fill(61, 21, 3, 71, 22, 9, WATER)
    lifts(61, 62, 23, 5, 7, (9, 0, 0), 90, 20)
    w.fill(61, 25, 4, 80, 25, 8, WIND_EAST)
    carve(73, 20, 3, 79, 23, 9)
    w.fill(73, 20, 3, 79, 20, 9, LAVA)
    bars(73, 24, 6, 79)
    w.place(O.EGG, 86, 24, 4)
    still(84, 24, 8)
    carve(84, 22, 9, 86, 23, 9)                                         # the stair down: 22, 20, 18
    w.fill(84, 21, 9, 86, 21, 9, PANEL)
    carve(84, 20, 10, 86, 28, 10)
    w.fill(84, 16, 11, 86, 17, 11, PANEL)

    # ---- the birds (walk 18, north to south) ----
    # A causeway of panel winding over a tray of lava in the open sky, six birds flying across it;
    # an egg on a stand a jump off it.
    w.fill(76, 15, 12, 92, 15, 40, PANEL)
    w.fill(76, 16, 12, 92, 16, 40, LAVA)
    for x0, z0, x1, z1 in ((84, 12, 86, 24), (79, 22, 86, 24), (79, 22, 81, 36), (79, 34, 88, 36),
                           (86, 34, 88, 41)):
        w.fill(x0, 16, z0, x1, 17, z1, PANEL)
    w.fill(88, 16, 18, 90, 17, 20, PANEL)                               # the egg's stand
    w.place(O.EGG, 89, 18, 19)
    bird((82, 18, 16), (88, 18, 16), 32)
    bird((88, 19, 20), (82, 19, 20), 24)
    bird((83, 18, 21), (83, 18, 26), 24)
    bird((77, 18, 29), (84, 18, 29), 32)
    bird((84, 19, 33), (77, 19, 33), 24)
    bird((84, 18, 32), (84, 18, 38), 24)
    still(79, 18, 22)
    still(88, 18, 36)

    # ---- the mines (walk 18, east to west, lanes z 43-45) ----
    # A pipe walkway: rows of mines across it, each row leaving a different lane free (weave
    # through), eggs between the rows; at its west end the platform the chain leaves from.
    w.fill(54, 16, 42, 89, 17, 46, PANEL)
    for x in range(54, 90):
        for z in (42, 46):
            w.set(x, 16, z, rng.choice(PIPES))
    pipe_rail(w, [(x, z) for x in range(61, 90) for z in (42, 46)], 18, rng)
    pipe_rail(w, [(89, z) for z in (43, 44, 45)], 18, rng)
    carve(86, 18, 42, 88, 18, 42)                                       # the way in from the birds
    for x, free in ((81, 45), (78, 43), (75, 44), (72, 45), (69, 43), (66, 44)):
        for z in (43, 44, 45):
            if z != free:
                still(x, 18, z)
    w.place(O.EGG, 79, 18, 43)
    w.place(O.EGG, 70, 18, 44)

    # ---- the chain to the golden key (walk 18 -> 26, out south-east) ----
    # The platform: two mines and a hanging mine. A lift moving out and up at once, a lift along
    # X, a lift up, a lift along Z: the four in step, each meeting the next at a dwell (two seconds
    # at each end), the last one bringing Blupi to the key on its stand. Under the chain a pool
    # catches a fall (a cell deep: he lands, swims, climbs out onto its rim): three eggs in it, a
    # stair from it back up to the platform.
    still(55, 18, 42)
    still(59, 18, 46)
    still(58, 19, 44, hanging=True)
    lifts(56, 57, 17, 47, 48, (0, 4, 6), 50, 40)                        # ends at y 21, z 53-54
    lifts(62, 63, 21, 55, 56, (-6, 0, 0), 50, 40)                       # meets it at x 56-57
    lifts(64, 65, 21, 55, 56, (0, 4, 0), 50, 40)                        # up to y 25
    lifts(64, 65, 25, 63, 64, (0, 0, -6), 50, 40)                       # meets it at z 57-58
    w.fill(63, 23, 65, 66, 25, 67, PANEL)                               # the key's stand, walk 26
    for x, z in ((63, 65), (66, 67)):
        w.set(x, 22, z, rng.choice(BOLTS))
    w.place(O.SECRET_EXIT, 64, 26, 66)
    w.fill(50, 2, 53, 72, 8, 76, PANEL)                                 # the pool, a cell deep
    carve(51, 7, 54, 71, 8, 75)
    w.fill(51, 7, 54, 71, 7, 75, WATER)
    for x, z in ((55, 62), (60, 68), (68, 58)):
        w.place(O.EGG, x, 7, z)
    for k, z in enumerate((52, 50, 48, 46)):                            # its stair: 11, 13, 15, 17
        w.fill(51, 3, z - 1, 53, 10 + 2 * k, z, PANEL)
    for y in range(4, 16):
        w.set(50, y, 52, rng.choice(PIPE_MACHINES))

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
