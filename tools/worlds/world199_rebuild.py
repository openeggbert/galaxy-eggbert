#!/usr/bin/env python3
"""World 199, the finale, rebuilt as a journey (worlds3d/world199.vwr): brief plans/briefs/world199.md.

Mobile Eggbert's bonus world 199 is a slime labyrinth holding a piece of every world before it, in a
sequence of twelve regions from the gun ledges at the start to the monument and the red arrow at the
end (plans/audit/world199_comparison.md section 2). Here those twelve regions are twelve hand-built
volumes, A to L, cut in one mass of slime: the player starts at the top of the north-west corner,
descends through eleven places to the bottom of the world and climbs the monument under his start.

Heights are walking levels (a floor block at y gives walk y + 1); X and Z are raw cells. Every
section is a function `section_X()`; the table at the top of its body is the authority for its
coordinates, the brief only summarises them.

    python3 tools/worlds/world199_rebuild.py [out.vwr]        # default: worlds3d/world199.vwr
    python3 tools/worlds/world199_rebuild.py --stage K out    # for levelcheck: K >= 1 the doors open,
                                                              # no hazards, lifts and saucers as stairs
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cheese import cheese, hole  # noqa: E402,F401
from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402,F401
from palace import stone as white_stone  # noqa: E402,F401
from slime import SLIME, SLIME_TRAP, skin, slime  # noqa: E402
from station import brick, water  # noqa: E402,F401
from village import ROCK, ROCK_FACES, STONE, STONE_FACES  # noqa: E402,F401

RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
LAVA = 68
BALLS = (215, 216, 217, 218)
STAR = 231
BOOKCASES = (139, 140, 141, 142, 143)
SAW, SWITCH = 378, 384
BLITZ, EMITTER = 305, 304
BRIDGE = 364
DRIP = 404
BAR = 202
TEMP = 324
BARRIER = 214
FAN_LEFT, WIND_LEFT = 126, 110
FAN_RIGHT, WIND_RIGHT = 129, 114
FAN_UP, WIND_UP = 132, 118
CONVEYOR = 47

w: World
rng: random.Random
calm = False


# ---- small helpers: everything below is written in walking levels ------------------------------
def air(x0, y0, z0, x1, y1, z1):
    w.fill(x0, y0, z0, x1, y1, z1, 0)


def floor(x0, z0, x1, z1, walk, block=SLIME, thick=1):
    """A floor whose walking level is `walk` (blocks at walk - 1, `thick` deep)."""
    w.fill(x0, walk - thick, z0, x1, walk - 1, z1, block)


def put(otype, x, y, z, **kw):
    w.place(otype, x, y, z, **kw)


def hazard(x, y, z, hanging=False):
    if not calm:
        mine(w, x, y, z, hanging=hanging)


def mover(otype, start, end, ticks=60, stop=0):
    if not calm:
        w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)


def lift_(x, walk0, walk1, z, ticks=60, stop=40):
    """A platform lift whose top is at walking level walk0 and walk1 (its cube at walk - 1)."""
    if calm:
        w.set(x, walk0 - 1, z, SLIME)
    else:
        lift(w, (x, walk0 - 1, z), (x, walk1 - 1, z), ticks=ticks, stop=stop)


def box_of(kind, x0, y0, z0, x1, y1, z1):
    if kind == "slime":
        slime(w, x0, y0, z0, x1, y1, z1)
    elif kind == "brick":
        brick(w, x0, y0, z0, x1, y1, z1, rng)
    elif kind == "palace":
        white_stone(w, x0, y0, z0, x1, y1, z1, rng)
    elif kind == "cheese":
        cheese(w, x0, y0, z0, x1, y1, z1, rng)
    elif kind == "rock":
        for x in range(x0, x1 + 1):
            for y in range(y0, y1 + 1):
                for z in range(z0, z1 + 1):
                    w.set(x, y, z, ROCK_FACES[0] if rng.random() < 0.2 else ROCK)
    elif kind == "machine":
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE_FLOOR)
    else:
        raise ValueError(kind)


def hall(kind, x0, y0, z0, x1, y1, z1, shell=1):
    """A hall of `kind` material: a shell of `shell` cells round the inside box, carved to air."""
    box_of(kind, x0 - shell, y0 - shell, z0 - shell, x1 + shell, y1 + shell, z1 + shell)
    air(x0, y0, z0, x1, y1, z1)


def key(otype, x, y, z):
    put(otype, x, y, z)


# ==== A. the gunfire shaft ======================================================================
def section_a():
    """X 8-20, Z 8-20, walk 92 down to 72. The start pad (9, 92, 9) in the north-west corner, four
    ledges of walk 88, 84, 80, 76 round the shaft (north, east, south, west), a floor at 72 with
    the tunnel east at z 13-15. A shooting Blupi on each ledge walks its wall row; the way is the
    ledge's void row. Two lifts rise from the floor; three mines bob in the void."""
    air(8, 72, 8, 20, 95, 20)
    floor(8, 8, 10, 10, 92)                                  # the start pad
    w.spawn = (9, 92, 9)
    slime(w, 8, 84, 11, 10, 93, 11)                          # a buttress shuts the pad's south side
    floor(11, 8, 20, 11, 88)                                 # north ledge
    floor(17, 12, 20, 20, 84)                                # east ledge
    floor(8, 17, 16, 20, 80)                                 # south ledge
    floor(8, 9, 11, 16, 76)                                  # west ledge
    air(21, 72, 13, 29, 75, 15)                              # the tunnel to B
    # the gunfire: a Blupi on each ledge along its wall row
    mover(O.CREATURE, (12, 88, 8), (19, 88, 8))
    mover(O.CREATURE, (20, 84, 13), (20, 84, 19))
    mover(O.CREATURE, (15, 80, 20), (9, 80, 20))
    mover(O.CREATURE, (8, 76, 10), (8, 76, 15))
    # the invisible fence along the east ledge's void side (x 16, z 12-17): the helicopter flies
    # through it, on foot he goes on along the ledge to its south end, where the south ledge is
    for z in range(12, 18):
        for y in (84, 85):
            w.set(16, y, z, BARRIER)
    lift_(13, 72, 88, 13, ticks=80, stop=40)
    lift_(13, 72, 80, 15, ticks=60, stop=40)
    for z, y0, y1 in ((12, 74, 84), (16, 72, 82), (14, 78, 90)):
        mover(O.MOVING_MINE, (15, y0, z), (15, y1, z), 100)
    put(O.EGG, 13, 80, 18)
    put(O.EGG, 9, 76, 12)



# ==== B. the basin and fan switchyard ===========================================================
def section_b():
    """X 30-53, Z 6-26, walk 72 up to 86. West bank (x 30-33), pool 1 (x 34-40, swum), middle bank
    (41-44), pool 2 (x 45-48) crossed on two conveyor stones, east ledge (x 49-52) with a column of
    four left fans in its east wall (x 53; their mouths, x 52, kill) and B1 at its north end; a
    south strip (z 21-26, walk 72) and from its east end a stair of steps and saucers up to the upper
    gallery (walk 82, z 22-26), a gap of three saucers in it, up fans and right fans on it, R1 at
    its west end; the blue gate in the south wall (x 31-33, z 28). A lift shelf (walk 86) in the
    north with the lollipop and the invert, guarded by four sleeping follow stars."""
    air(30, 72, 6, 53, 89, 26)
    air(31, 82, 27, 33, 85, 31)                              # B's exit tunnel to C (blue door at z 28)
    if not calm:
        w.fill(31, 82, 28, 33, 83, 28, BLUE_DOOR)
    for x0, x1 in ((34, 40), (45, 48)):                      # the two pools
        air(x0, 66, 6, x1, 71, 20)
        water(w, x0, 66, 6, x1, 71, 20)
    w.bigdecor(203, 36, 66, 10)
    w.bigdecor(203, 38, 66, 17)
    w.bigdecor(203, 46, 66, 12)
    # the upper gallery: a slab over the south strip's west part, a gap of three cells (x 39-41)
    floor(30, 22, 38, 26, 82, thick=2)
    floor(42, 22, 47, 26, 82, thick=2)
    # the stair from the strip's east end (z 25): step 74, saucer 76, step 78, saucer 80, then a step
    # of the gallery's own height (x 48) beside the gallery
    for x, walk in ((52, 74), (51, 76), (50, 78), (49, 80), (48, 82)):
        if walk % 4 == 2:
            slime(w, x, 72, 25, x, walk - 1, 25)
        elif calm:
            slime(w, x, walk - 1, 25, x, walk - 1, 25)
        else:
            w.set(x, walk - 1, 25, TEMP)
    slime(w, 48, 72, 22, 48, 81, 26)
    if calm:
        floor(39, 22, 41, 26, 82, thick=2)
    else:
        for x in (39, 40, 41):
            for z in (23, 24, 25):
                w.set(x, 81, z, TEMP)
    # two conveyor platforms (47, they push east) side by side as stepping stones over pool 2, one
    # cell of water either side: jumped onto from the middle bank, run east along, jumped off
    for x in (46, 47):
        if calm:
            w.set(x, 71, 12, SLIME)
        else:
            w.place(O.LIFT_RIGHT, x, 71, 12, end=(x, 71, 12), advance=10, recede=10, stop_start=10, stop_end=10)
    # four left fans in the east wall: mouths (52, z), wind to the west across the ledge
    for z in (8, 11, 14, 17):
        w.set(53, 72, z, FAN_LEFT)
        if not calm:
            w.fill(49, 72, z, 52, 72, z, WIND_LEFT)
    # up fans on the gallery, the way between them, and right fans at its west end
    for x, z in ((35, 22), (35, 26), (38, 22), (38, 26)):
        w.set(x, 81, z, FAN_UP)
        if not calm:
            w.fill(x, 82, z, x, 87, z, WIND_UP)
    for z in (23, 25):
        w.set(30, 82, z, FAN_RIGHT)
        if not calm:
            w.fill(31, 82, z, 35, 82, z, WIND_RIGHT)
    # the lift shelf in the north (walk 86) with the lollipop, the invert and four follow stars
    floor(41, 6, 45, 8, 86, thick=2)
    lift_(43, 72, 86, 10, ticks=100, stop=40)
    put(O.LOLLIPOP, 42, 86, 7)
    put(O.INVERT, 44, 86, 7)
    for x, z in ((41, 6), (45, 6), (41, 8), (45, 8)):
        if not calm:
            put(O.FOLLOW_STAR, x, 86, z)
    lift_(33, 72, 82, 21, ticks=80, stop=40)                # lifts back up to the gallery
    lift_(44, 72, 82, 21, ticks=80, stop=40)
    # keys, eggs, mines
    key(O.KEY3, 51, 72, 7)                                   # B1, the blue key, at the east ledge's end
    key(O.KEY1, 32, 82, 24)                                  # R1, the red key, at the gallery's west end
    for x, y, z in ((42, 72, 8), (31, 72, 8), (50, 72, 19), (45, 82, 24)):
        put(O.EGG, x, y, z)
    hazard(49, 72, 8)                                        # M2 beside B1
    for x in (36,):                                          # M3 hanging over the gallery's low stretch
        slime(w, x - 1, 84, 23, x + 1, 85, 25)
        hazard(x, 83, 24, hanging=True)
    mover(O.MOVING_MINE, (37, 72, 8), (37, 72, 11), 10)
    mover(O.MOVING_MINE, (46, 72, 14), (48, 72, 14), 10)
    mover(O.MOVING_MINE, (40, 85, 22), (40, 85, 26), 10)


# ==== C. the lift comb and the brick library ====================================================
COMB = [(35, 36), (37, 36), (39, 36), (41, 36), (41, 38), (41, 40), (41, 42), (41, 44)]   # (x, z) of L1..L8


def section_c():
    """X 32-51, Z 34-48, walk 82 down to 58, brick. A hall (x 32-44) with the arrival balcony (walk 82,
    x 32-33, z 34-40) and the comb: eight single-cell lifts, each shuttling three cells (82/79 for
    the first, ... 61/58 for the eighth), two cells apart round the north-east of the void so that each is
    jumped to from the one before at the end of both their dwells (odd ones start high, even ones low).
    The library (x 46-51, walk 58) behind the red door: three aisles (z 36, 40, 44) each with a
    bulldozer, shelves between, R2 at the north end of the corridor (x 46), G1 at the south end; the
    green gate to D in the hall's west wall."""
    hall("brick", 32, 58, 34, 44, 88, 48)
    hall("brick", 46, 58, 34, 51, 66, 48)
    air(31, 82, 32, 33, 85, 33)                              # B's tunnel comes in at the balcony
    floor(32, 34, 33, 40, 82, block=261)
    for k, (x, z) in enumerate(COMB):
        top = 82 - 3 * k
        lift_(x, top if k % 2 == 0 else top - 3, top - 3 if k % 2 == 0 else top, z, ticks=45, stop=40)
    air(26, 58, 44, 30, 61, 46)                              # the tunnel west to D
    if not calm:
        w.fill(31, 58, 44, 31, 59, 46, GREEN_DOOR)
        w.fill(45, 58, 41, 45, 59, 43, RED_DOOR)
    else:
        air(45, 58, 41, 45, 59, 43)
    # the library: shelf blocks between the aisles, bulldozers in the aisles
    for z0, z1 in ((34, 35), (37, 39), (41, 43), (45, 48)):
        for x in range(47, 52):
            for z in range(z0, z1 + 1):
                for y in (58, 59, 60):
                    w.set(x, y, z, rng.choice(BOOKCASES))
    for z in (36, 40, 44):
        mover(O.BULLDOZER, (46, 58, z), (51, 58, z))
    key(O.KEY1, 46, 58, 34)                                  # R2
    key(O.KEY2, 46, 58, 48)                                  # G1
    put(O.EGG, 51, 61, 40)                                   # on the shelves' tops, by the lift below
    put(O.EGG, 51, 61, 44)
    lift_(50, 61, 64, 46, ticks=30, stop=40)
    # the hall: two crates, a lift to a high shelf, the mines
    put(O.CRATE, 36, 58, 46)
    put(O.CRATE, 39, 58, 46)
    floor(42, 34, 44, 36, 70, block=261)
    lift_(43, 58, 70, 38, ticks=60, stop=40)
    put(O.EGG, 43, 70, 35)
    for x, z in ((33, 46), (43, 47), (35, 47)):
        put(O.EGG, x, 58, z)
    mover(O.MOVING_MINE, (33, 58, 47), (40, 58, 47), 10)
    mover(O.MOVING_MINE, (36, 58, 44), (36, 64, 44), 10)
    mover(O.MOVING_MINE, (33, 60, 36), (33, 66, 36), 10)
    mover(O.MOVING_MINE, (35, 58, 42), (35, 58, 45), 10)
    mover(O.MOVING_MINE, (42, 58, 47), (44, 58, 47), 10)


# ==== D. the white palace ========================================================================
T330_D = (11, 70, 45)          # the booth over the keep's cell (walk 70), partner in E
T331_D = (20, 58, 64)          # the court's return booth, partner in E's west wing
STONEFACE = (386, 387, 388, 390, 397)


def section_d():
    """X 11-25, Z 41-65, walk 58 up to 70, white stone. C's tunnel enters the courtyard (walk 58) at
    its east wall (z 44-46). A spiral of balconies, each reached by a lift one cell off the end of the
    last: LD1 (x 15, z 45) to the west balcony (walk 62, x 11-13), LD2 (15, z 63) to the south balcony
    (walk 66, x 16-25, z 62-64; saw row 1 at z 63, x 19-23, its switch at x 17), LD3 (25, z 60) to the
    east balcony (walk 70, x 23-25), then the bridge over the north end (z 43-45, x 11-25; saw row 2 at
    z 44, x 14-18, its switch at x 21) to the keep (x 11-13): B2, the statue, and teleporter 330 down
    to E. Behind the red door in the west wall (z 50, walk 62) a gallery with saw row 3 (z 50, x 4-8,
    switch x 9) and G2. A fourth lift (x 21, z 55) to a shelf (walk 66) with an egg."""
    hall("palace", 11, 58, 41, 25, 79, 65)
    air(26, 58, 44, 26, 61, 46)
    # facades: window panels on the south wall's inside, trim along the balconies' edges
    for x in range(11, 26):
        for y in (61, 62, 66):
            if w.get(x, y, 66) and rng.random() < 0.7 and y > 60:
                w.set(x, y, 66, 391)
    floor(11, 43, 13, 63, 62, block=396)                       # the west balcony
    lift_(15, 58, 62, 45, ticks=45, stop=40)                   # LD1
    lift_(15, 62, 66, 63, ticks=45, stop=40)                   # LD2
    floor(16, 62, 25, 64, 66, block=396)                       # the south balcony
    for x in range(19, 24):
        w.set(x, 65, 63, 396 if calm else SAW)
        for z in (62, 64):
            for y in (66, 67):
                w.set(x, y, z, 396)
    w.set(17, 65, 63, SWITCH)
    lift_(25, 66, 70, 60, ticks=45, stop=40)                   # LD3
    floor(23, 43, 25, 59, 70, block=396)                       # the east balcony
    floor(11, 43, 25, 45, 70, block=396)                       # the bridge and the keep
    for x in range(14, 19):
        w.set(x, 69, 44, 396 if calm else SAW)
        for z in (43, 45):
            for y in (70, 71):
                w.set(x, y, z, 396)
    w.set(21, 69, 44, SWITCH)
    key(O.KEY3, 12, 70, 44)                                    # B2
    w.set(T330_D[0], T330_D[1] + 1, T330_D[2], 330)
    w.set(T331_D[0], T331_D[1] + 1, T331_D[2], 331)
    # the gallery behind the red door
    air(3, 62, 49, 9, 66, 51)
    w.fill(3, 61, 49, 9, 61, 51, 396)
    if not calm:
        w.fill(10, 62, 50, 10, 63, 50, RED_DOOR)
    else:
        air(10, 62, 50, 10, 63, 50)
    w.set(9, 61, 50, SWITCH)
    for x in range(4, 9):
        w.set(x, 61, 50, 396 if calm else SAW)
        for z in (49, 51):
            for y in (62, 63):
                w.set(x, y, z, 396)
    key(O.KEY2, 3, 62, 50)                                     # G2
    # the shelf with the fourth lift, the statue's recess in the north wall, the court's furniture
    floor(20, 53, 22, 55, 66, block=396)
    lift_(21, 58, 66, 57, ticks=60, stop=40)
    put(O.EGG, 21, 66, 54)
    put(O.EGG, 24, 70, 51)
    put(O.EGG, 12, 62, 60)
    put(O.EGG, 22, 58, 63)
    put(O.INVERT, 23, 70, 56)
    w.bigdecor(89, 17, 58, 42)                                 # the statue, in the north wall's niche
    w.bigdecor(88, 20, 58, 50)                                 # the fountain
    for k, (x, z) in enumerate(((14, 52), (14, 58), (22, 61), (19, 47))):
        w.bigdecor((66, 67, 68, 66)[k], x, 58, z)
    for x, z in ((16, 60), (22, 48), (17, 55)):
        w.bigdecor(69, x, 58, z)
    # the guards: two big creatures walking the court's south, two birds under the balconies
    mover(O.BIG_CREATURE, (13, 58, 60), (21, 58, 60))
    mover(O.BIG_CREATURE, (21, 58, 52), (14, 58, 52))
    mover(O.BIRD, (13, 63, 56), (22, 63, 56))
    mover(O.BIRD, (23, 68, 62), (13, 68, 62))
    # mines: five ground mines on the balconies' wide parts, four hanging over low stretches, four
    # moving and two standing moving-mine records
    for x, y, z in ((11, 62, 56), (12, 62, 48), (25, 66, 64), (23, 70, 52), (24, 58, 58)):
        hazard(x, y, z)
    for x, y, z in ((18, 67, 63), (16, 71, 44), (24, 71, 48), (11, 63, 52)):
        hazard(x, y, z, hanging=True)
    mover(O.MOVING_MINE, (16, 58, 49), (22, 58, 49), 10)
    mover(O.MOVING_MINE, (12, 58, 47), (12, 58, 57), 10)
    mover(O.MOVING_MINE, (22, 58, 57), (22, 58, 63), 10)
    mover(O.MOVING_MINE, (18, 60, 54), (18, 64, 54), 10)
    for x, y, z in ((14, 64, 52), (20, 62, 58)):
        mover(O.MOVING_MINE, (x, y, z), (x, y, z), 10)


# ==== E. the lava terraces =======================================================================
T330_E = (12, 46, 60)
T331_E = (11, 46, 44)


def section_e():
    """X 10-26, Z 43-63, walk 46 down to 34, blue machine panel. Arrival by teleporter 330 at the
    south-west terrace TA (x 10-15, z 57-63). East of it lava pit P1 (x 16-20), crossed on a clay
    bridge (z 62, five cells, crumbles behind him) or on a ferry lift (z 58), to the south-east
    terrace TB (x 21-26). North of TB lava pit P2 (z 52-56) crossed hanging from a row of seven bars
    (x 24, two over the floor, z 51-57) to the north-east terrace TC (z 43-51), where a lift (x 26,
    z 45) goes down a shaft to the exit passage (walk 34, z 45) under a column of lightning. A ferry
    lift (z 45) over lava pit P3 (x 16-19) from TC to the west wing TD (x 10-15, z 43-47: two eggs,
    teleporter 331 back to the palace court). North of TA a well (x 11-14, z 53-56, seven deep, a dead
    end): the wasp sweeping its foot, a moving mine across it halfway."""
    hall("machine", 10, 46, 43, 26, 55, 63)
    w.fill(10, 46, 48, 15, 55, 56, MACHINE_FLOOR)            # the wall between TA and TD, with the well
    w.fill(16, 46, 52, 20, 55, 56, MACHINE_FLOOR)            # the pillar west of P2
    air(11, 39, 53, 14, 45, 56)                              # the well
    for x0, z0, x1, z1 in ((16, 57, 20, 63), (21, 52, 26, 56), (16, 43, 19, 47)):
        air(x0, 41, z0, x1, 45, z1)
        w.fill(x0, 40, z0, x1, 40, z1, MACHINE_FLOOR)
        w.fill(x0, 41, z0, x1, 42, z1, LAVA)
    # P1: the clay bridge and the ferry
    for x in range(16, 21):
        w.set(x, 45, 62, MACHINE_FLOOR if calm else BRIDGE)
    if calm:
        w.fill(16, 45, 58, 20, 45, 58, MACHINE_FLOOR)
    else:
        lift(w, (16, 45, 58), (20, 45, 58), ticks=60, stop=40)
    # P2: the bars from TC's south edge to TB's north edge
    if calm:
        w.fill(21, 45, 52, 26, 45, 56, MACHINE_FLOOR)
    else:
        w.fill(24, 48, 51, 24, 48, 57, BAR)
    # P3: the ferry to the west wing
    if calm:
        w.fill(16, 45, 45, 19, 45, 45, MACHINE_FLOOR)
    else:
        lift(w, (16, 45, 45), (19, 45, 45), ticks=40, stop=40)
    # the lift down at TC's east edge, the shaft and the passage with its lightning
    air(25, 34, 44, 27, 45, 46)
    air(27, 34, 45, 31, 37, 45)
    w.fill(25, 33, 44, 31, 33, 46, MACHINE_FLOOR)
    lift_(26, 46, 34, 45, ticks=120, stop=40)
    for x in (27, 29, 31):
        w.set(x, 37, 45, EMITTER)
    w.set(29, 37, 46, EMITTER)
    if not calm:
        w.fill(29, 34, 45, 29, 36, 45, BLITZ)
    # teleporters: 330's arrival, 331's departure from the west wing
    w.set(T330_E[0], T330_E[1] + 1, T330_E[2], 330)
    w.set(T331_E[0], T331_E[1] + 1, T331_E[2], 331)
    # the wasp's well
    if not calm:
        w.place(O.WASP, 11, 39, 55, end=(14, 39, 55))
        mover(O.MOVING_MINE, (11, 43, 54), (14, 43, 54), 10)
    else:
        for k in range(3):
            slime(w, 11 + k, 39, 56, 11 + k, 40 + 2 * k, 56)
    hazard(26, 46, 59)                                       # M2 at TB's corner
    hazard(23, 48, 47, hanging=True)                         # M3 over TC
    for x, y, z in ((18, 47, 60), (22, 45, 54), (26, 45, 54)):
        mover(O.MOVING_MINE, (x, y, z), (x, y, z), 10)
    mover(O.MOVING_MINE, (16, 47, 44), (19, 47, 44), 10)
    mover(O.MOVING_MINE, (21, 46, 48), (25, 46, 48), 10)
    put(O.EGG, 12, 46, 45)
    put(O.EGG, 14, 46, 62)
    # the west wing's wall: rows of tall blue panel with the machine's props
    for z in (44, 46):
        w.set(10, 47, z, 55)
        w.set(10, 48, z, 56)


SECTIONS = [section_a, section_b, section_c, section_d, section_e]


def main(argv) -> int:
    global w, rng, calm
    args = argv[1:]
    stage = None
    if args[:1] == ["--stage"]:
        stage = int(args[1])
        args = args[2:]
    out = Path(args[0]) if args else Path(__file__).resolve().parents[2] / "worlds3d" / "world199.vwr"
    rng = random.Random(199)
    w = World(mission=199, sky=31)
    calm = stage is not None and stage >= 1
    slime(w, 2, 2, 2, 97, 97, 97)                      # the jungle: every place is cut out of it
    for build in SECTIONS:
        build()
    skin(w, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
