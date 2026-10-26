#!/usr/bin/env python3
"""Mission 073, the toy works (worlds3d/world073.vwr): brief plans/briefs/world073.md.

Mobile Eggbert's level 073, all of the toy box mixed, under the region-22 sky: four treasures and
three eggs, the red arrow at the top. Blupi starts by a maze of lifts in narrow shafts whose
bottoms are lava (60/20, 40/10, 60/10, 40/10 and a quick 18/3); two treasures in it, and the
treasure door "2" (422) that lets him out once both are taken; a strip of lava crossed on small
shuttles (20 ticks); the lollipop; two licked jumps up to a shelf with four skateboards; a
corridor with two rows of five mines and two cells of lava in its floor, ridden on the
skateboard (it runs mines over, and jumps); at its east end lifts up to an upper corridor back
west with two bulldozers and two shuttles meeting over a gap (10 ticks), two eggs; then lifts
on striped poles in the open sky up to the arrow, a treasure on the way. Here the maze is a hall
of toy pillars over lava, each lift rising beside one pillar to the next; the strip of lava a
river of islands stepping down, a raft between each two; the corridors stacked; the last climb a
stair of floating platforms. Every lift is boarded by a step down onto it and left level with the
ledge it serves; the dwells are 40 ticks where the original's are shorter (a cell to walk in 3D).

    python3 tools/worlds/world073.py                 # writes worlds3d/world073.vwr
    python3 tools/worlds/world073.py --stage K out   # for levelcheck:
        1: every lift's low end raised level with its ledge (in the game it lies one below:
           he steps down onto it; levelcheck boards a lift only level with it)
        2: also the treasure door open (both treasures taken)
        3: also stand-in lifts where the lollipop's jumps go
        4: also the corridor's mines gone (the skateboard ran them over)
        5: also the bulldozers left out (passed at the right moment)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from toys import (BALL_POLES, CANDY_POLES, CONE_BALLS, HALF_BALLS, LAVA, MIXED, POLE_TILES,  # noqa: E402
                  carve, toy)

TREASURE_DOOR2 = 420 + 2      # opens once two treasures are taken (Decor::OpenDoorsTresor)
SKY_BALLS = (215, 216, 217, 218, 219, 220)
WS = 8                        # the start shore's walking level
WN = 18                       # the hall's north ledge, the door, the river's first landing
WI = 15                       # the lollipop's island (the river steps down one per island)
WL = WI + 5                   # the landing over the river and the skate corridor
WSH = WL + 5                  # the skateboard shelf
WF = WL + 8                   # the upper corridor (one lower west of its gap)
SPAWN = (50, WS, 87)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if stage else Path(__file__).resolve().parents[2] / "worlds3d" / "world073.vwr"
    rng = random.Random(73)
    w = World(mission=73, sky=22)

    def mass(x0, y0, z0, x1, y1, z1):
        toy(w, x0, y0, z0, x1, y1, z1, rng, MIXED)

    def pillar(x0, z0, x1, z1, walk):
        mass(x0, 5, z0, x1, walk - 1, z1)

    def step_lift(start, end, ticks, stop, raised=True):
        """A lift whose low end lies one below the ledge it is boarded from (for a raft or a
        shuttle, both ends: one below the ledge it leaves, level with the one it reaches): from
        stage 1 on raised by one, as levelcheck boards a lift only level with it. Blupi cannot
        jump off a lift in this engine (reported), so every lift's far end is level with its
        ledge."""
        if stage >= 1 and raised:
            if start[1] == end[1]:
                start, end = (start[0], start[1] + 1, start[2]), (end[0], end[1] + 1, end[2])
            elif start[1] < end[1]:
                start = (start[0], start[1] + 1, start[2])
            else:
                end = (end[0], end[1] + 1, end[2])
        lift(w, start, end, ticks=ticks, stop=stop)

    # ---- B, the lift hall (lava at y 5, walls to y 23, open above) ----
    mass(40, 0, 63, 60, 23, 92)
    carve(w, 43, 6, 66, 57, 23, 89)
    w.fill(43, 5, 66, 57, 5, 83, LAVA)
    mass(43, 5, 84, 57, WS - 1, 89)                                   # S, the start shore
    pillar(48, 76, 52, 82, 11)                                       # P1
    pillar(54, 76, 56, 78, 8)                                        # P2, the low pocket
    pillar(48, 70, 52, 74, 15)                                       # P3
    pillar(44, 70, 46, 72, 10)                                       # P4, the egg's pocket
    pillar(43, 66, 57, 68, WN)                                       # N, the north ledge
    # Each lift's low end one below the ledge it is boarded from (a step down onto it, never onto
    # a moving edge), its high end level with the ledge it serves; the pockets' lifts (L2, L4)
    # level with both their ledges (ridden there and back).
    step_lift((50, WS - 2, 83), (50, 10, 83), ticks=60, stop=40)        # L1: S -> P1 (orig. stop 20)
    step_lift((53, 10, 77), (53, 7, 77), ticks=40, stop=40, raised=False)  # L2: P1 <-> P2 (orig. stop 10)
    step_lift((50, 9, 75), (50, 14, 75), ticks=40, stop=40)             # L3: P1 -> P3 (orig. stop 10)
    step_lift((47, 14, 71), (47, 9, 71), ticks=60, stop=40, raised=False)  # L4: P3 <-> P4 (orig. stop 10)
    step_lift((50, 13, 69), (50, WN - 1, 69), ticks=18, stop=40)        # L5: P3 -> N (orig. stop 3)
    w.place(O.TREASURE, 55, 8, 77)                                   # T1
    w.place(O.TREASURE, 44, WN, 67)                                  # T2
    w.place(O.EGG, 45, 10, 71)
    # The door "2" through the north wall, three wide, two high.
    carve(w, 49, WN, 63, 51, WN + 1, 65)
    if stage < 2:
        w.fill(49, WN, 65, 51, WN + 1, 65, TREASURE_DOOR2)

    # ---- C, the lava river north to the lollipop ----
    # Islands stepping down one each (walk 18, 17, 16, 15), a strip of lava between each two, a
    # raft (two lifts in step) one below the island it leaves and level with the one it reaches.
    mass(43, 0, 42, 57, 23, 62)
    carve(w, 46, 10, 42, 54, 23, 62)
    sections = [(59, 62, WN), (53, 55, WN - 1), (47, 49, WN - 2), (42, 43, WI)]   # (z0, z1, walk)
    for z0, z1, walk in sections:
        mass(46, 0, z0, 54, walk - 1, z1)
    strips = [((56, 58), (49, 50)), ((50, 52), (51, 52)), ((44, 46), (48, 49))]
    for k, ((z0, z1), xs) in enumerate(strips):
        top = WN - 1 - k                                               # the raft's top: walk - 1
        mass(46, 0, z0, 54, top - 4, z1)
        w.fill(46, top - 3, z0, 54, top - 3, z1, LAVA)                # lava three below the raft
        for x in xs:
            step_lift((x, top - 1, z1), (x, top - 1, z0), ticks=20, stop=40)   # orig. stop 20
    w.place(O.LOLLIPOP, 50, WI, 42)
    for icon, x, walk, z in ((HALF_BALLS[0], 47, WN, 61), (CONE_BALLS[2], 53, WN - 1, 54),
                             (BALL_POLES[1], 47, WN - 2, 48)):
        w.bigdecor(icon, x, walk, z)

    # ---- D, the landing (walk 20) and the skateboard shelf (walk 25) ----
    mass(46, 8, 36, 54, WL - 1, 41)                                  # L, five over the island
    mass(46, 15, 30, 54, WSH - 1, 35)                                # SH, five over L
    w.place(O.TREASURE, 47, WL, 37)                                  # T3
    for x in (47, 49, 51, 53):
        w.place(O.SKATEBOARD, x, WSH, 34)
    if stage >= 3:
        # Stand-ins for the licked jumps: lifts from the island to L, from L to SH.
        w.clear(50, WI - 1, 43)
        lift(w, (50, WI - 1, 43), (50, WL - 1, 43))
        w.clear(50, WL - 1, 36)
        lift(w, (50, WL - 1, 36), (50, WSH - 1, 36))

    # ---- K, the skate corridor (walk 20), E, the lift tower, F, the upper corridor (walk 28) ----
    mass(55, 12, 33, 93, WF + 3, 43)
    carve(w, 55, WL, 37, 88, WL + 2, 39)                             # K
    for x0, x1 in ((59, 63), (78, 82)):
        for x in range(x0, x1 + 1):
            for z in (37, 38, 39):
                if stage < 4:
                    mine(w, x, WL, z)
    for x in (68, 73):                                               # lava in its floor
        w.fill(x, WL - 1, 37, x, WL - 1, 39, LAVA)
    carve(w, 89, WL, 34, 92, WF + 2, 41)                             # E
    mass(90, WL, 35, 92, WL + 3, 38)                                 # EL, walk 24
    w.fill(89, WL - 2, 38, 89, WL - 1, 38, 0)
    step_lift((89, WL - 2, 38), (89, WL + 3, 38), ticks=50, stop=40)   # E1: floor -> EL (orig. 20/20)
    step_lift((89, WL + 2, 35), (89, WF - 1, 35), ticks=50, stop=40)   # E2: EL -> FL (orig. 20/20)
    carve(w, 57, WF, 37, 88, WF + 2, 39)                             # F
    carve(w, 84, WF, 34, 88, WF + 2, 36)                             # FL, by E2's top
    carve(w, 57, WF - 1, 37, 69, WF - 1, 39)                         # the west part one lower
    # The bulldozers' stretches one wide (jump it, or pass at the right moment).
    for x0, x1, a, b, ticks, walk in ((76, 81, 80, 78, 60, WF), (61, 66, 65, 62, 90, WF - 1)):
        mass(x0, walk, 37, x1, walk + 2, 37)
        mass(x0, walk, 39, x1, walk + 2, 39)
        if stage < 5:
            w.place(O.BULLDOZER, a, walk, 38, end=(b, walk, 38), advance=ticks, recede=ticks,
                    stop_start=20, stop_end=20)
    # The gap x 70-73 over lava, two shuttles meeting in its middle (orig. stop 10), one below
    # the east bank and level with the west bank, which is one lower: F goes on west at walk 27.
    carve(w, 70, WF - 4, 37, 73, WF - 1, 39)
    w.fill(70, WF - 5, 37, 73, WF - 5, 39, LAVA)
    step_lift((73, WF - 2, 38), (72, WF - 2, 38), ticks=10, stop=40)
    step_lift((70, WF - 2, 38), (71, WF - 2, 38), ticks=10, stop=40)
    w.place(O.EGG, 58, WF - 1, 38)
    w.place(O.EGG, 59, WF - 1, 38)
    # F2: north from F's west end, out to the open sky (walk 27).
    WG = WF - 1
    mass(56, WG - 5, 22, 60, WF + 2, 32)
    carve(w, 57, WG, 22, 59, WF + 2, 36)

    # ---- G, the climb in the open sky to the arrow ----
    def platform(x0, z0, x1, z1, walk):
        toy(w, x0, walk - 2, z0, x1, walk - 1, z1, rng, SKY_BALLS)
        cx, cz = (x0 + x1) // 2, (z0 + z1) // 2
        w.fill(cx, walk - 7, cz, cx, walk - 3, cz, rng.choice(POLE_TILES))
        w.fill(cx, walk - 11, cz, cx, walk - 8, cz, rng.choice(CANDY_POLES))

    platform(57, 19, 59, 21, WG)                                     # G1, F2's balcony, walk 27
    step_lift((58, WG - 2, 18), (58, WG + 2, 18), ticks=40, stop=40)    # Ga: up, G1 -> G2
    platform(57, 14, 59, 17, WG + 3)                                 # G2, walk 30
    step_lift((60, WG + 2, 15), (60, WG - 1, 15), ticks=40, stop=40, raised=False)  # Gb: G2 <-> G2b
    platform(61, 14, 63, 16, WG)                                     # G2b, walk 27
    w.place(O.TREASURE, 62, WG, 15)                                  # T4
    step_lift((56, WG + 1, 15), (56, WG + 5, 15), ticks=40, stop=40)    # Gc: up, G2 -> G3
    platform(53, 13, 55, 16, WG + 6)                                 # G3, walk 33
    step_lift((54, WG + 4, 12), (54, WG + 8, 12), ticks=40, stop=40)    # Gd: up, G3 -> G4
    platform(52, 8, 56, 11, WG + 9)                                  # G4, walk 36
    w.place(O.EXIT, 54, WG + 9, 9)
    w.fill(54, WG + 12, 9, 54, WG + 15, 9, rng.choice(CANDY_POLES))  # a pole over the arrow

    w.spawn = SPAWN
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
