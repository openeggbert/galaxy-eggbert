#!/usr/bin/env python3
"""Mission 046, the cheese moon and world 4's golden key (worlds3d/world046.vwr): brief
plans/briefs/world046.md.

Mobile Eggbert's level 046, the last of the cheese world, under the region-9 sky of the cheese
moon: one long climb up a great slope of cheese among blue machine girders to the golden key
at its top. Blupi starts below beside a pool; the one helicopter fetches the red key from a
tower and the red door opens; up the slope, mines lie on its steps and moving mines sweep
across it; a chain of six lifts in step (80 ticks, 20 stop) carries him up over a chasm, each
handing him on to the next where they meet; a cage of toy balls holds the lollipops whose high
jump reaches a machine deck with the green key and the green door; two drinks make him
invisible to the moving mines of the gallery beyond; a crate climbs to the blue key, the blue
door opens on the last lift, and the golden key waits on the summit. Here the slope is a stair
round the south and east faces of a cheese mountain, the chain climbs its east chasm, the deck
stands on its north-east terrace, and the summit massif holds the gallery, the crate cave and
the lift to the top.

    python3 tools/worlds/world046.py                 # writes worlds3d/world046.vwr
    python3 tools/worlds/world046.py --check STAGE out
        STAGE red:    the red door open (the helicopter fetched the red key)
        STAGE timed:  also the moving mines left out (passed at the right moment, or hidden)
        STAGE power:  also a stair where the lollipop's high jump goes onto the deck
        STAGE green:  also the green door open
        STAGE solved: also the crate pushed under the blue key's ledge, the blue door open and
                      a step beside the last lift (levelcheck does not jump onto a lift)
    Check with `tools/flightcheck.py --mine-margin 0`.
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from cheese import (BLUE_DOOR, CHEESE, GREEN_DOOR, LAVA, MACHINE, MACHINE_PIPES, MECA, RED_DOOR,  # noqa: E402
                    WATER, DEEP_WATER, carve, cheese, helicopters, hole, patrol)

TOY = 215                # the toy-ball blocks of the lollipop cage
CANDY = 236              # striped candy poles (decor, no collision)
WC = 10                  # the start court's and the foot path's walking level
WP = 51                  # the plateau's (the mountain's top round the summit massif)
WD = 57                  # the machine deck's and the drinks hall's
WK = 63                  # the crate cave's
WS = 71                  # the summit's
SPAWN = (16, WC, 90)
STAGES = ("red", "timed", "power", "green", "solved")


def at_least(stage, name):
    return stage in STAGES and STAGES.index(stage) >= STAGES.index(name)


def machine(w, x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, MACHINE)


def court(w: World, rng: random.Random, stage):
    """A: the start court (walk 10), walled and roofed: the helicopter, a machine tower twelve
    high with the red key on top; the red door in its east wall."""
    cheese(w, 8, 4, 78, 24, WC - 1, 94, rng)
    for x in range(8, 25):
        for z in range(78, 95):
            if x in (8, 24) or z in (78, 94):
                for y in range(WC, 24):
                    w.set(x, y, z, rng.choice(CHEESE))
            w.set(x, 24, z, rng.choice(CHEESE))
    machine(w, 11, WC, 81, 13, WC + 11, 83)
    w.place(O.KEY1, 12, WC + 12, 82)
    helicopters(w, [(18, WC, 88)])
    w.place(O.EGG, 10, WC, 92)


def pool(w: World, rng: random.Random):
    """B: the pool room behind the red door: a ledge, a pool three deep with a curtain wall
    down into it (dive under; the helicopter is wrecked in water and stays behind), the bank
    and the way out onto the foot path."""
    cheese(w, 24, 4, 76, 34, 16, 84, rng)
    carve(w, 25, WC, 77, 33, 15, 83)
    carve(w, 26, 7, 77, 32, 9, 83)
    w.fill(26, 7, 77, 32, 8, 83, DEEP_WATER)
    w.fill(26, 9, 77, 32, 9, 83, WATER)
    cheese(w, 29, 8, 77, 29, 15, 83, rng)                 # the curtain, open at y 7 only
    carve(w, 34, WC, 78, 34, WC + 2, 80)                  # the way out east


def red_door(w: World, stage):
    """The red door between the court and the pool room (set after both are built). From the
    red stage on, the dive is modelled for levelcheck (which does not swim): the pool's floor
    raised under its top water cell and a hole in the curtain there."""
    for y in (WC, WC + 1):
        w.set(24, y, 80, 0 if at_least(stage, "red") else RED_DOOR)
    if at_least(stage, "red"):
        w.fill(26, 7, 77, 32, 8, 83, CHEESE[0])
        w.fill(29, 9, 77, 29, 10, 83, 0)


def stair(w: World, rng: random.Random, stage):
    """C: the foot path (walk 10) and the stair up the mountain's south face: twenty-one steps
    two deep and one high, three wide (z 72-74), walk 11 at x 35 to walk 31 at x 76. On steps
    4, 8, 12 and 16 mines lie on the back cell in two of the three lanes, the free lane changing
    from row to row: Blupi weaves up the slope. On steps 6 and 14 a moving mine sweeps across. Step 10 is a gap with lifts bobbing two cells (20 ticks,
    a second's wait) between steps 9 and 11; a fall from it, or off the stair, lands on the
    foot path. Machine girders stand along the foot path."""
    cheese(w, 35, 4, 75, 84, WC - 1, 80, rng)               # the foot path
    for k in range(21):
        x = 35 + 2 * k
        if k == 10:
            cheese(w, x, 4, 72, x + 1, WC - 1, 74, rng)    # the gap's floor, the foot path's level
            for xx in (x, x + 1):
                for z in (72, 73, 74):
                    lift(w, (xx, WC + 9, z), (xx, WC + 11, z), ticks=20, stop=20)
            continue
        cheese(w, x, 4, 72, x + 1, WC + k, 74, rng)
    for k, free in ((4, 73), (8, 72), (12, 74), (16, 73)):
        for z in (72, 73, 74):
            if z != free:
                mine(w, 36 + 2 * k, WC + 1 + k, z)
    if not at_least(stage, "timed"):
        for k, ticks in ((6, 30), (14, 30)):
            patrol(w, O.MOVING_MINE, (36 + 2 * k, WC + 1 + k, 72), (36 + 2 * k, WC + 1 + k, 74), ticks)
    w.place(O.EGG, 40, WC + 3, 74)
    w.place(O.EGG, 72, WC + 19, 72)
    for x in (40, 56, 72):                                  # the girders
        machine(w, x, WC, 80, x, 44, 80)


def chasm(w: World, rng: random.Random):
    """D: the south-east landing (walk 31) and the chasm north of it, its floor four lower
    (walk 27, steps back up at its east side), open to the sky in the east. Over it the chain
    of six lifts (80 ticks as the original's), two wide and two deep, climbing north in relay
    from the landing to the terrace (walk 51): each rises three or four, alternately up and
    down, so that one's top meets the next one's bottom, a rest column between each two at
    their meeting height. The original's lifts hand Blupi on side by side in mid-air, some
    moving across; in 3D each waits two seconds at its ends (the original's one second is too
    short to cross a cell) and the rests hold him back until the lift is level: holding
    forward, he is carried up. A mistimed step drops him onto the chasm floor."""
    cheese(w, 77, 4, 66, 84, 30, 80, rng)                   # the landing (the foot path's end below)
    cheese(w, 77, 4, 46, 84, 26, 65, rng)                   # the chasm's floor
    cheese(w, 83, 27, 64, 84, 28, 65, rng)                  # steps back up to the landing
    walks = (31, 35, 39, 42, 45, 48, 51)
    for k in range(6):
        z = 65 - 3 * k                                      # the lift covers z - 1 and z
        low, high = walks[k], walks[k + 1]
        a, b = (low - 1, high - 1) if k % 2 == 0 else (high - 1, low - 1)
        for x in (79, 80):
            for zz in (z - 1, z):
                w.place(O.LIFT, x, a, zz, end=(x, b, zz), advance=80, recede=80, stop_start=40, stop_end=40)
        if k < 5:                                           # the rest north of it
            cheese(w, 79, 27, z - 2, 80, high - 1, z - 2, rng)


def mountain(w: World, rng: random.Random):
    """The mountain's mass: the core (x 22-76, z 30-71) up to the plateau (walk 51), the
    north-east terrace beside the chasm (x 77-84, z 30-48), the summit massif on it (x 30-60,
    z 30-62) up to the summit (walk 71)."""
    cheese(w, 22, 4, 30, 76, WP - 1, 71, rng)
    cheese(w, 77, 27, 30, 84, WP - 1, 48, rng)
    cheese(w, 30, WP, 30, 60, WS - 1, 62, rng)


def terrace(w: World, rng: random.Random, stage):
    """E: the plateau's north-east part (walk 51): the lollipop cage of toy balls three high,
    two lollipops and candy poles inside, a third lollipop outside; the machine deck (walk 57,
    six above: only the lollipop's high jump reaches it) along the north side with the green
    key, an egg, pipes and a machine; the green door at its west end in the summit massif."""
    for x in range(66, 73):
        for z in range(38, 45):
            if x in (66, 72) or z in (38, 44):
                for y in range(WP, WP + 3):
                    w.set(x, y, z, TOY)
    for y in (WP, WP + 1):
        w.clear(72, y, 41)                                  # the cage's doorway, east
    for x, z in ((67, 39), (71, 39), (67, 43), (71, 43)):
        for y in range(WP, WP + 3):
            w.set(x, y, z, CANDY)
    w.place(O.LOLLIPOP, 68, WP, 40)
    w.place(O.LOLLIPOP, 69, WP, 42)
    w.place(O.LOLLIPOP, 82, WP, 42)
    mine(w, 75, WP, 44)
    mine(w, 64, WP, 46)
    # The deck.
    machine(w, 61, WP, 30, 84, WD - 1, 34)
    w.place(O.KEY2, 63, WD, 32)
    w.place(O.EGG, 80, WD, 31)
    for x in (66, 70, 74, 78):
        w.set(x, WD, 30, MACHINE_PIPES[(x // 4) % len(MACHINE_PIPES)])
    w.bigdecor(MECA[3], 72, WD, 31)
    if at_least(stage, "power"):                            # where the high jump goes
        for z, top in ((37, WP + 1), (36, WP + 3), (35, WP + 5)):
            cheese(w, 70, WP, z, 72, top, z, rng)
    for y in (WD, WD + 1):
        w.set(60, y, 32, 0 if at_least(stage, "green") else GREEN_DOOR)


def summit_interior(w: World, rng: random.Random, stage):
    """F: inside the summit massif. The drinks hall (walk 57) behind the green door: a strip of
    lava across it, two drinks, three eggs, two mines. The gallery west and then south, three
    wide, climbing in steps two deep to the crate cave (walk 63); four moving mines sweep across
    it (with a drink's twenty seconds Blupi walks through them). The crate cave: the blue key on
    a ledge three high, the crate to push under it, an egg, a mine; the blue door in its south
    wall on the last lift's chamber, a cell deep before the lift (48 ticks, two seconds' wait at
    the bottom: he jumps onto it) up through the summit's surface."""
    carve(w, 46, WD, 31, 59, WD + 5, 39)                    # the drinks hall
    w.fill(51, WD - 1, 31, 51, WD - 1, 39, LAVA)
    w.place(O.DRINK, 54, WD, 37)
    w.place(O.DRINK, 48, WD, 37)
    for x in (47, 48, 49):
        w.place(O.EGG, x, WD, 31)
    mine(w, 57, WD, 39)
    mine(w, 46, WD, 39)
    # The gallery: west along z 33-35, then south up the steps along x 33-35.
    carve(w, 33, WD, 33, 45, WD + 3, 35)
    for k in range(6):
        z0 = 36 + 2 * k
        top = WD + k                                        # the step's top: walk WD + 1 + k
        cheese(w, 33, WD - 1, z0, 35, top, z0 + 1, rng)
        carve(w, 33, top + 1, z0, 35, top + 4, z0 + 1)
    if not at_least(stage, "timed"):
        patrol(w, O.MOVING_MINE, (41, WD, 33), (41, WD, 35), 20)
        patrol(w, O.MOVING_MINE, (37, WD, 35), (37, WD, 33), 30)
        patrol(w, O.MOVING_MINE, (33, WD + 3, 39), (35, WD + 3, 39), 30)
        patrol(w, O.MOVING_MINE, (35, WD + 5, 43), (33, WD + 5, 43), 40)
    # The crate cave.
    carve(w, 33, WK, 48, 46, WK + 5, 56)
    cheese(w, 44, WK, 48, 46, WK + 2, 50, rng)              # the ledge (walk 66)
    w.place(O.KEY3, 45, WK + 3, 49)
    w.place(O.CRATE, *((43, WK, 49) if stage == "solved" else (38, WK, 53)))
    w.place(O.EGG, 34, WK, 56)
    mine(w, 40, WK, 51)
    # The blue door and the last lift's chamber, the lift up to the summit.
    carve(w, 39, WK, 58, 42, WK + 3, 60)                    # the chamber: a cell before the lift
    for y in (WK, WK + 1):
        w.set(40, y, 57, 0 if stage == "solved" else BLUE_DOOR)
    carve(w, 40, WK, 59, 41, WS - 1, 60)                    # the lift's shaft
    for x in (40, 41):
        for z in (59, 60):
            w.place(O.LIFT, x, WK, z, end=(x, WS - 1, z), advance=48, recede=48, stop_start=40,
                    stop_end=20)
    if stage == "solved":                                   # the jump onto it (levelcheck)
        w.set(39, WK, 59, CHEESE[0])


def summit(w: World, rng: random.Random):
    """G: the summit (walk 71), the massif's top under the cheese moon's sky: craters dented
    in it, three mines and a moving mine round the golden key in the middle."""
    for x, z, r in ((36, 36, 2.2), (54, 38, 2.6), (35, 52, 2.0), (52, 54, 2.4), (44, 33, 1.8)):
        hole(w, x, WS - 0.5, z, r, 1.2, r)
    w.place(O.SECRET_EXIT, 45, WS, 45)
    for x, z in ((42, 45), (48, 44), (45, 49)):
        mine(w, x, WS, z)
    patrol(w, O.MOVING_MINE, (40, WS, 41), (50, WS, 41), 40)


def main(argv) -> int:
    stage = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    rng = random.Random(46)
    w = World(mission=46, sky=9)
    mountain(w, rng)
    court(w, rng, stage)
    pool(w, rng)
    red_door(w, stage)
    stair(w, rng, stage)
    chasm(w, rng)
    terrace(w, rng, stage)
    summit_interior(w, rng, stage)
    summit(w, rng)
    w.spawn = SPAWN
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world046.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
