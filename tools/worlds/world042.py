#!/usr/bin/env python3
"""Mission 042, mines and lifts under the red sky (worlds3d/world042.vwr): brief
plans/briefs/world042.md.

Mobile Eggbert's level 042, on foot under the region-20 red sky: Blupi starts in a pocket of
cheese where mines lie on the floor and hang above the jumps over them; treasure 1 up a step
opens door "1"; a stair of six little lifts, each bobbing one cell, climbs over a drop, with
treasure 2 down below it; door "2" opens onto a hill with a house and trees; a row of bumps
with a mine in every gap between them; down at its end treasure 3, and a shaft climbed on
five lifts in relay, seven cells each, through cheese with walk-through secret passages; an
avenue of trees with fourteen mines to jump one by one; a pyramid of twelve treasures on its
steps; and once all fifteen are taken the doors "15" at its top open the way down to the red
arrow. Here: the pocket is a round crater, the lift stair crosses a basin that catches a
fall, door 2 is in a ridge across the hill's island, the bumps are a causeway east to a
tower with rows of mines across it, the relay climbs the tower's hall (rests between the
lifts), the avenue runs north from its top, and the pyramid is a real stepped pyramid with
a shaft down its core.

    python3 tools/worlds/world042.py                 # writes worlds3d/world042.vwr
    python3 tools/worlds/world042.py --check STAGE out
        STAGE door1:  door 1 open
        STAGE stones: also the lift stair's stones fixed at their low ends, as when each is
                      jumped onto at the right moment (levelcheck rides a lift between its
                      ends but does not jump onto a moving one)
        STAGE door2:  also door 2 open
        STAGE solved: also door 15 open
    Check with `tools/flightcheck.py --mine-margin 0` (mines can be jumped, as in the game).
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from cheese import (HOUSE, SECRET, TREASURE_DOOR1, TREES, carve, cheese, hole)  # noqa: E402

WA = 12          # the crater's floor
WS = 6           # the basin under the lift stair
WC = 18          # the house island, the causeway and the tower hall's floor
WT = 53          # the relay's top, the avenue and the pyramid island
WX = 43          # the exit hall under the pyramid
SPAWN = (15, WA, 90)
DOOR1 = (23, WA, 85)
DOOR2 = (58, WC, 85)
PYR = (78, 21)                    # the pyramid's axis
DOOR15 = (78, WT + 6, 23)


def crater(w: World, rng: random.Random):
    """A: the crater (walk 12, open to the sky, rim seven above the floor). In a notch in its
    north wall two steps up to treasure 1 and an egg; across the notch's mouth three mines to
    jump, and over two of the three lanes a beam with mines hanging where a jump peaks."""
    cheese(w, 5, 4, 75, 25, WA + 6, 95, rng)
    for x in range(5, 26):
        for z in range(75, 96):
            if (x - 15) ** 2 + (z - 85) ** 2 <= 7.5 ** 2:
                carve(w, x, WA, z, x, WA + 6, z)
    carve(w, 14, WA, 74, 16, WA + 6, 78)                  # the notch
    cheese(w, 14, WA, 76, 16, WA, 78, rng)                # step 1 (walk 13)
    cheese(w, 14, WA, 74, 16, WA + 2, 75, rng)            # step 2 (walk 15)
    w.place(O.TREASURE, 16, WA + 3, 74)
    w.place(O.EGG, 14, WA + 3, 75)
    for x in (14, 15, 16):
        mine(w, x, WA, 79)
    cheese(w, 14, WA + 4, 79, 16, WA + 4, 79, rng)        # the beam over the mouth
    for x in (14, 15):
        mine(w, x, WA + 3, 79, hanging=True)
    mine(w, 20, WA, 88)
    # Door 1 in the east wall and the short tunnel out to the landing over the basin.
    carve(w, 24, WA, 84, 25, WA + 2, 86)
    mine(w, 24, WA + 2, 85, hanging=True)


def lift_stair(w: World, rng: random.Random, fixed: bool):
    """B: the landing outside door 1 (walk 12), six stones of four lifts each (two by two)
    bobbing one cell (10 ticks, 10 stop, in counter-step), a cell apart and a cell from the
    landing -- a lift does not stop Blupi sideways, so he must never walk into one at its top:
    every stone is jumped onto -- rising one each to the house island (walk 18); the basin
    under them (walk 6) catches a fall and holds treasure 2, steps back up to the landing at
    its west end."""
    cheese(w, 26, WA - 2, 83, 27, WA - 1, 88, rng)        # the landing
    cheese(w, 26, WS - 3, 80, 45, WS - 1, 93, rng)        # the basin
    cheese(w, 26, WS, 91, 27, WS + 1, 92, rng)            # steps up: walk 8, 10, then 12
    cheese(w, 26, WS, 89, 27, WS + 3, 90, rng)
    w.place(O.TREASURE, 36, WS, 88)
    for k in range(6):
        low, high = WA - 1 + k, WA + k                    # the cube: Blupi stands at +1
        a, b = (low, high) if k % 2 == 0 else (high, low)
        for x in (29 + 3 * k, 30 + 3 * k):
            for z in (84, 85):
                if fixed:
                    cheese(w, x, low, z, x, low, z, rng)
                else:
                    lift(w, (x, a, z), (x, b, z), ticks=10, stop=10)


def house_island(w: World, rng: random.Random):
    """C: the island of the house and the trees (walk 18); a ridge four high along its east
    edge with door 2 in it."""
    cheese(w, 46, 10, 76, 58, WC - 1, 94, rng)
    cheese(w, 58, WC, 76, 58, WC + 3, 94, rng)            # the ridge
    w.bigdecor(HOUSE, 51, WC, 80)
    for icon, x, z in ((TREES[0], 55, 79), (TREES[1], 55, 91), (TREES[2], 48, 91), (TREES[0], 51, 92),
                       (TREES[2], 48, 78)):
        w.bigdecor(icon, x, WC, z)
    w.place(O.EGG, 55, WC, 88)


def causeway(w: World, rng: random.Random):
    """D: the causeway east (walk 18), three wide over the sky, five rows of three mines lying
    across it four apart: jumped one row at a time (a mine kills within 0.7 of Blupi, a jump
    carries him 2.4 cells, so rows closer than four leave almost no room to land between)."""
    cheese(w, 59, WC - 4, 84, 80, WC - 1, 86, rng)
    for x in (62, 66, 70, 74, 78):
        for z in (84, 85, 86):
            mine(w, x, WC, z)


def tower(w: World, rng: random.Random):
    """E: the tower and its hall (floor walk 18, 38 high): treasure 3 and two eggs on the
    floor, an egg in a niche behind walk-through cheese; along the north wall five lifts in
    relay (42 ticks, 3 stop), each two cells deep, each rising seven above the last, the first
    from a step at its foot. In 3D the lifts stand a column apart, a rest between each two
    whose top is the meeting height: Blupi rides one up, steps across onto the rest and boards
    the next at its bottom, or jumps onto it as it leaves (he waits on the rest instead of
    crossing from lift to lift in the moment they meet). A mistimed step drops him back to the
    hall floor. From the last lift's top the way out north, a step down onto the avenue."""
    cheese(w, 81, 10, 66, 97, 56, 92, rng)
    carve(w, 84, WC, 70, 94, 55, 88)
    carve(w, 81, WC, 84, 83, WC + 3, 86)                  # the entrance from the causeway
    w.place(O.TREASURE, 90, WC, 86)
    w.place(O.EGG, 93, WC, 87)
    w.place(O.EGG, 93, WC, 85)
    carve(w, 88, WC, 89, 90, WC + 2, 90)                  # the secret niche
    w.fill(88, WC, 89, 90, WC + 1, 89, SECRET)
    w.place(O.EGG, 89, WC, 90)
    for k in range(5):
        x = 85 + 2 * k
        low, high = WC + 7 * k, WC + 7 + 7 * k            # the first stands on the floor
        a, b = (low, high) if k % 2 == 0 else (high, low)
        for z in (70, 71):
            lift(w, (x, a, z), (x, b, z), ticks=42, stop=3)
        if k < 4:                                         # the rest east of it
            cheese(w, x + 1, WC, 70, x + 1, high, 71, rng)
    cheese(w, 84, WC, 72, 86, WC, 73, rng)                # a step at the first lift's foot
    carve(w, 92, WT + 1, 66, 94, WT + 3, 69)              # the way out north, down onto the avenue


def avenue(w: World, rng: random.Random):
    """G: the avenue north from the tower (walk 53): bays five wide and three long with trees,
    between them five necks three wide where three mines lie across the way, jumped one row at
    a time (fifteen mines for the original's fourteen)."""
    z = 65
    for k in range(6):
        cheese(w, 90, WT - 4, z - 2, 94, WT - 1, z, rng)               # a bay
        w.bigdecor(TREES[k % 3], 90 if k % 2 else 94, WT, z - 1)
        if k == 3:
            w.place(O.EGG, 94, WT, z - 2)
        z -= 3
        if k < 5:
            cheese(w, 91, WT - 4, z, 93, WT - 1, z, rng)               # a neck
            for x in (91, 92, 93):
                mine(w, x, WT, z)
            z -= 1
    cheese(w, 90, WT - 4, 38, 94, WT - 1, z, rng)                      # on to the island


def pyramid(w: World, rng: random.Random, solved: bool):
    """H: the island of the pyramid (walk 53); the pyramid six steps high, twelve treasures on
    its steps 1, 3 and 5, three on each face; on its top a cap with door 15 in its south wall
    over a shaft down its core into the exit hall (walk 43)."""
    cx, cz = PYR
    cheese(w, 60, WX - 1, 6, 96, WT - 1, 38, rng)
    for k in range(1, 7):
        h = 8 - k
        cheese(w, cx - h, WT - 1 + k, cz - h, cx + h, WT - 1 + k, cz + h, rng)
    for k in (1, 3, 5):
        h, y = 8 - k, WT + k
        for x, z in ((cx, cz + h), (cx + h, cz), (cx, cz - h), (cx - h, cz)):
            w.place(O.TREASURE, x, y, z)
    # The cap round the top step, door 15 in its south wall, the shaft inside it.
    top = WT + 6
    cheese(w, cx - 2, top, cz - 2, cx + 2, top + 2, cz + 2, rng)
    carve(w, cx - 1, top, cz - 1, cx + 1, top + 2, cz + 1)
    carve(w, cx - 1, WX, cz - 1, cx + 1, top - 1, cz + 1)
    x, y, z = DOOR15
    for yy in (y, y + 1):
        w.set(x, yy, z, 0 if solved else TREASURE_DOOR1 + 14)
    # The exit hall under the island's top.
    carve(w, cx - 6, WX, cz - 6, cx + 6, WX + 5, cz + 6)
    w.place(O.EXIT, cx, WX, cz - 5)


def doors(w: World, stage):
    for (x, y, z), icon, open_in in ((DOOR1, TREASURE_DOOR1, ("door1", "stones", "door2", "solved")),
                                     (DOOR2, TREASURE_DOOR1 + 1, ("door2", "solved"))):
        for yy in (y, y + 1):
            w.set(x, yy, z, 0 if stage in open_in else icon)


def main(argv) -> int:
    stage = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    rng = random.Random(42)
    w = World(mission=42, sky=20)
    crater(w, rng)
    lift_stair(w, rng, fixed=stage in ("stones", "door2", "solved"))
    house_island(w, rng)
    causeway(w, rng)
    tower(w, rng)
    avenue(w, rng)
    pyramid(w, rng, solved=stage == "solved")
    doors(w, stage)
    w.spawn = SPAWN
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world042.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
