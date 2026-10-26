#!/usr/bin/env python3
"""Mission 093, the pipe works (worlds3d/world093.vwr): brief plans/briefs/world093.md.

Mobile Eggbert's level 093 runs through the clods along blue pipelines (251-253 on machine joints)
under the red cave: Blupi starts in the west, jumps mines in a low passage, reaches a cistern of
blue machine under four enemy helicopters (32) and dives into it, swims down, along a water pipe
and up into the works; there he pushes crates (12) to climb to a pipeline and walks it to a
machine box with the two treasures (5), whose treasure door (422) then opens the way on; a lift
(80 ticks), a second water pipe swum through, and the red arrow (7) in a machine box guarded by
five enemy helicopters and enemy tanks (33), passed with the drink (30), which lies in a pocket
behind two mines riding up and down (16). Fifteen mines, twelve eggs. Here the same order in 3D:
the start and the passage in the south-west, the cistern room north of it, the great hall of
pipes in the middle, the lift and the second swim east, the guarded court in the south.

    python3 tools/worlds/world093.py                   # writes worlds3d/world093.vwr
    python3 tools/worlds/world093.py --stage K out.vwr # lessons 1..K solved, for levelcheck (no
        crates, swimming, treasure doors or timing): 1 the swims (stand-in lifts), 2 the crate
        stair (a crate set against the plinth), 3 the treasure door open, 4 the enemies and
        moving mines timed or passed unseen (left out)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from clods import (DEEP_WATER, MACHINE, MACHINE_BALL, PIPES, PLANTS, TRESTLE, WATER, Y_FORK,  # noqa: E402
                   carve, clay, column, fringe, report_audit, rough)
from kit import MACHINE_FACES, O, World, lift, mine, wall  # noqa: E402

TREASURE_DOOR2 = 422          # opens once two treasures are taken (Decor::OpenDoorsTresor)
TOP = 40
UP = 20                       # the start, the passage, the cistern room and the hall: walk 20
PIPE = 25                     # the pipelines' walking level (pipe tiles at y 24)
LOW = 12                      # the guarded court: walk 12
SPAWN = (13, UP, 86)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else 0
    out = Path(argv[3]) if len(argv) > 3 else Path(__file__).resolve().parents[2] / "worlds3d" / "world093.vwr"
    rng = random.Random(93)
    w = World(mission=93, sky=27)

    def mass(x0, y0, z0, x1, y1, z1):
        clay(w, x0, y0, z0, x1, y1, z1, rng)

    def cut(x0, y0, z0, x1, y1, z1):
        carve(w, x0, y0, z0, x1, y1, z1)

    def machine(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE)

    def enemy(otype, a, b, ticks, stop):
        if stage < 4:
            w.place(otype, *a, end=b, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def pipeline(cells, y):
        """A pipeline one wide along `cells`, a machine joint every sixth cell (as the original's
        runs of pipe between machine blocks)."""
        for k, (x, z) in enumerate(cells):
            w.set(x, y, z, MACHINE if k % 6 == 0 else rng.choice(PIPES))

    def water(x0, y0, z0, x1, y1, z1):
        """Deep water up to y1 - 1 under a surface row at y1."""
        w.fill(x0, y0, z0, x1, y1 - 1, z1, DEEP_WATER)
        w.fill(x0, y1, z0, x1, y1, z1, WATER)

    mass(4, 0, 4, 95, TOP, 95)

    # ---- S, the start cave (x 8-18, z 80-90, walk 20) and the mine passage north ----------------
    cut(8, UP, 80, 18, UP + 5, 90)
    rough(w, 8, 80, 18, 90, UP, UP + 6, rng)
    w.spawn = SPAWN
    w.place(O.EGG, 9, UP, 89)
    w.bigdecor(PLANTS[0], 17, UP, 89)
    # The passage (x 12-14, z 61-79, three high): a mine to walk round, then two rows of three
    #    across it, each to be jumped; a niche of three eggs off its east side.
    cut(12, UP, 61, 14, UP + 2, 79)
    mine(w, 13, UP, 76)
    for x in (12, 13, 14):
        mine(w, x, UP, 71)
        mine(w, x, UP, 65)
    cut(15, UP, 67, 17, UP + 1, 69)
    for z in (67, 68, 69):
        w.place(O.EGG, 17, UP, z)

    # ---- R, the cistern room (x 6-26, z 38-60, walk 20, roof at y 33) ---------------------------
    cut(6, UP, 38, 26, 32, 60)
    rough(w, 6, 38, 26, 60, UP, 33, rng, sag=0)
    # The cistern: machine walls (x 12-20, z 44-52) up to y 21 (its rim, walk 22: a jump up);
    #    water x 13-19, z 45-51, from y 15 to the surface at y 21. At its bottom an outlet three by
    #    three leaves east (z 47-49, y 15-17) under the room's floor and the wall to the hall,
    #    where it rises into the hall's basin (x 32-34): the only way on.
    machine(12, 14, 44, 20, 21, 52)                    # the cistern's walls
    machine(20, 14, 46, 31, 18, 50)                    # the outlet's shell
    machine(31, 14, 46, 35, UP - 1, 50)                # the rising pipe's shell and the basin's rim
    water(13, 15, 45, 19, 21, 51)
    w.fill(20, 15, 47, 31, 17, 49, DEEP_WATER)
    water(32, 15, 47, 34, UP - 1, 49)
    w.place(O.EGG, 16, 15, 48)                         # at the cistern's bottom
    # Four enemy helicopters hover over it, each to and fro a cell, firing down.
    for x, z in ((13, 45), (19, 45), (13, 51), (19, 51)):
        enemy(O.ROBOT, (x, 26, z), (x + (1 if x < 16 else -1), 26, z), 8, 25)
    perch_cells = ((8, 40), (24, 58))
    for (x, z), prop in zip(perch_cells, (Y_FORK, TRESTLE)):
        column(w, x, z, UP, UP + 3, rng)
        w.set(x, UP + 4, z, prop)
    if stage >= 1:                                     # levelcheck cannot swim: a stand-in lift
        w.place(O.LIFT, 20, UP + 1, 48, end=(33, UP - 1, 52))

    # ---- P, the hall of pipes (x 32-78, z 20-60, floor walk 20, roof at y 38) -------------------
    cut(32, UP, 20, 78, 37, 60)
    rough(w, 32, 20, 78, 60, PIPE, 38, rng, bulge=0.4)
    # The plinth (x 40-44, z 30-34): machine three high (walk 23), out of a jump's reach; five
    #    crates on the floor round it. Push one against the plinth, climb it and jump.
    machine(40, UP, 30, 44, UP + 2, 34)
    if stage < 2:
        for x, z in ((38, 38), (46, 37), (36, 28), (48, 30), (43, 40)):
            w.place(O.CRATE, x, UP, z)
    else:
        w.place(O.CRATE, 42, UP, 35)
    # From the plinth a machine pier two higher (walk 25) carries pipeline P1 east to the
    #    treasure box; a mine lies on it to jump over.
    machine(42, UP + 3, 31, 43, PIPE - 1, 32)
    pipeline([(x, 32) for x in range(44, 58)], PIPE - 1)
    for x in (49, 55):
        machine(x, UP, 32, x, PIPE - 2, 32)             # its piers
    mine(w, 51, PIPE, 32)
    # The treasure box (machine, x 58-64, z 29-35, interior walk 25, roof at y 28) on four piers:
    #    its doorway west from P1, the two treasures inside, its east doorway onto P2.
    machine(58, PIPE - 1, 29, 64, PIPE + 3, 35)
    cut(59, PIPE, 30, 63, PIPE + 2, 34)
    cut(58, PIPE, 32, 58, PIPE + 1, 32)
    cut(64, PIPE, 32, 64, PIPE + 1, 32)
    for x, z in ((58, 29), (64, 29), (58, 35), (64, 35)):
        machine(x, UP, z, x, PIPE - 2, z)
    for z in (30, 34):
        w.set(61, PIPE + 2, z, MACHINE_BALL)
    w.place(O.TREASURE, 60, PIPE, 31)
    w.place(O.TREASURE, 62, PIPE, 33)
    # P2: east from the box to the hall's east wall, where door "2" shuts a doorway; an enemy
    #    helicopter patrols over it, a mine lies on it.
    pipeline([(x, 32) for x in range(65, 79)], PIPE - 1)
    for x in (70, 76):
        machine(x, UP, 32, x, PIPE - 2, 32)
    mine(w, 73, PIPE, 32)
    enemy(O.ROBOT, (66, PIPE + 4, 33), (77, PIPE + 4, 33), 48, 25)
    cut(79, PIPE, 31, 79, PIPE + 1, 33)
    if stage < 3:
        w.fill(79, PIPE, 31, 79, PIPE + 1, 33, TREASURE_DOOR2)
    # The hall's floor: an enemy tank on guard by the east wall, firing along Z; mines; eggs on
    #    a shelf two high (walk 22) in the north-west corner; plants.
    enemy(O.CREATURE, (74, UP, 50), (74, UP, 51), 10, 23)
    for x, z in ((56, 44), (66, 26), (52, 54)):
        mine(w, x, UP, z)
    mass(33, UP, 21, 37, UP + 1, 25)
    for x in (34, 35, 36):
        w.place(O.EGG, x, UP + 2, 22)
    for x, z, p in ((38, 56, 0), (60, 52, 1), (70, 40, 2), (47, 22, 1), (76, 24, 0)):
        w.bigdecor(PLANTS[p], x, UP, z)

    # ---- L, the corridor, the lift and the landing (behind door "2", walk 25 down to 17) -------
    cut(80, PIPE, 31, 88, PIPE + 2, 33)
    # Four lifts in step (x 87-88, z 34-35) south of it, down eight cells (80 ticks, the
    #    original's; its dwell of 10 made 40 to step on and off in 3D) to walk 17.
    cut(87, 16, 34, 88, PIPE + 2, 35)
    for x in (87, 88):
        for z in (34, 35):
            lift(w, (x, PIPE - 1, z), (x, 16, z), ticks=80, stop=40)
    # The landing at its foot, a corridor south (x 85-89, z 36-64, walk 17): a mine sweeping
    #    across it, a mine lying in it.
    cut(85, 17, 36, 89, 20, 64)
    enemy(O.MOVING_MINE, (85, 17, 48), (89, 17, 48), 20, 0)
    mine(w, 86, 17, 56)
    # W2, the second swim: a well at its end (x 86-88, z 61-63), its surface level with the floor;
    #    down six, a pipe three by three south to z 70-72 and west to the court's east basin
    #    (x 80-82), up into it.
    machine(85, 7, 60, 89, 16, 64)
    machine(85, 7, 64, 89, 11, 73)
    machine(79, 7, 69, 89, 11, 73)
    machine(79, 7, 69, 83, LOW - 1, 73)
    water(86, 8, 61, 88, 16, 63)
    w.fill(86, 8, 64, 88, 10, 72, DEEP_WATER)
    w.fill(83, 8, 70, 88, 10, 72, DEEP_WATER)
    water(80, 8, 70, 82, LOW - 1, 72)
    if stage >= 1:
        w.place(O.LIFT, 87, 16, 65, end=(81, LOW - 1, 74))

    # ---- G, the guarded court (x 40-78, z 66-90, walk 12, roof at y 24) -----------------------
    cut(40, LOW, 66, 78, 23, 90)
    cut(79, LOW, 68, 84, 20, 78)                        # the east alcove over the basin
    rough(w, 40, 66, 78, 90, LOW, 24, rng, sag=0)
    # The court is cut in two by a bank of clods three high (x 51-62, out of a jump's reach), the
    #    lane one wide through it (z 78) the only way west. Two tanks drive in the lane, each
    #    firing along it at both ends of its run (their bodies are harmless, their shots kill);
    #    enemy helicopters hover over it and round the exit box, dropping shots. In plain sight
    #    it is a gauntlet; unseen (the drink) nothing touches Blupi.
    mass(51, LOW, 66, 62, LOW + 2, 77)
    mass(51, LOW, 79, 62, LOW + 2, 90)
    enemy(O.CREATURE, (53, LOW, 78), (55, LOW, 78), 10, 23)
    enemy(O.CREATURE, (60, LOW, 78), (58, LOW, 78), 10, 23)
    # The exit box west of it (machine, x 41-47, z 80-88, roof at y 15), out of the lane's line
    #    of fire, its doorway north (44, z 80); the arrow inside.
    machine(41, LOW - 1, 80, 47, LOW + 3, 88)
    cut(42, LOW, 81, 46, LOW + 2, 87)
    cut(44, LOW, 80, 44, LOW + 1, 80)
    w.place(O.EXIT, 44, LOW, 85)
    for a, b, t in (((51, 16, 78), (56, 16, 78), 32), ((57, 17, 78), (62, 17, 78), 32),
                    ((53, 15, 78), (54, 15, 78), 8), ((41, 16, 79), (47, 16, 79), 32),
                    ((49, 16, 80), (49, 16, 86), 32)):
        enemy(O.ROBOT, a, b, t, 25)
    for x, z in ((66, 74), (66, 84), (42, 68)):
        mine(w, x, LOW, z)
    w.place(O.EGG, 83, LOW, 76)
    w.place(O.EGG, 41, LOW, 89)
    # The drink's pocket in the court's south-east corner (x 74-78, z 86-90): its mouth two
    #    wide (x 75-76, z 85), a mine riding up and down three cells in each (30 ticks): step
    #    through under one while it is up.
    machine(73, LOW, 85, 79, LOW + 4, 85)
    cut(75, LOW, 85, 76, LOW + 3, 85)
    machine(73, LOW, 86, 73, LOW + 4, 90)
    w.place(O.DRINK, 76, LOW, 88)
    if stage < 4:
        w.place(O.MOVING_MINE, 75, LOW, 85, end=(75, LOW + 3, 85), advance=30, recede=30, stop_start=0,
                stop_end=0)
        w.place(O.MOVING_MINE, 76, LOW + 3, 85, end=(76, LOW, 85), advance=30, recede=30, stop_start=0,
                stop_end=0)
    for x, z, p in ((70, 89, 2), (68, 67, 0), (49, 70, 1)):
        w.bigdecor(PLANTS[p], x, LOW, z)

    fringe(w, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0 if stage or report_audit(out) else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
