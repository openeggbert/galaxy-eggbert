#!/usr/bin/env python3
"""World 199, the finale, rebuilt as a journey (worlds3d/world199.vwr): brief plans/briefs/world199.md.

Mobile Eggbert's bonus world 199 is a slime labyrinth holding a piece of every world before it, in a
sequence of twelve regions from the gun ledges at the start to the monument and the red arrow at the
end (plans/audit/world199_comparison.md section 2). Here those twelve regions are twelve hand-built
volumes, A to L, cut in one mass of slime: the player starts at the top of the north-west corner,
descends through eleven places to the bottom of the world and climbs the monument under his start.

Heights are walking levels (a floor block at y gives walk y + 1); X and Z are raw cells. Every
section is a function `section_X()`; its docstring is the authority for its
coordinates, the brief only summarises them.

    python3 tools/worlds/world199.py [out.vwr]        # default: worlds3d/world199.vwr
    python3 tools/worlds/world199.py --stage K out    # for levelcheck: K >= 1 the doors open, no mines,
                                                      # creatures, saws, fans' winds or lightning; the
                                                      # saucers, ferries over lava and bars as floor
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cheese import cave, cheese, hole, low_mine, tube  # noqa: E402,F401
from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402,F401
from palace import stone as white_stone  # noqa: E402,F401
from slime import SLIME, SLIME_TRAP, skin, slime  # noqa: E402
from station import brick, girder_mast, metal, water  # noqa: E402,F401
from village import ROCK, ROCK_FACES, STONE, STONE_FACES, house  # noqa: E402,F401

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
    elif kind == "metal":
        metal(w, x0, y0, z0, x1, y1, z1, rng)
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
    for x, y, z, icon in ((31, 72, 21, 66), (43, 72, 19, 67), (43, 72, 7, 68), (32, 72, 18, 29)):
        w.bigdecor(icon, x, y, z)
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
            w.set(x, 81, 24, TEMP)
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
    floor(39, 6, 47, 8, 86, thick=2)
    lift_(43, 72, 86, 10, ticks=100, stop=40)
    put(O.LOLLIPOP, 43, 72, 14)                              # by the pillar on the middle bank
    put(O.INVERT, 43, 86, 8)
    for x, z in ((39, 6), (47, 6), (39, 8), (47, 8)):
        if not calm:
            put(O.FOLLOW_STAR, x, 86, z)
    lift_(33, 72, 82, 21, ticks=80, stop=40)                # lifts back up to the gallery
    lift_(44, 72, 82, 21, ticks=80, stop=40)
    # keys, eggs, mines
    key(O.KEY3, 51, 72, 7)                                   # B1, the blue key, at the east ledge's end
    key(O.KEY1, 32, 82, 24)                                  # R1, the red key, at the gallery's west end
    slime(w, 42, 72, 12, 42, 75, 12)                        # a pillar four high: the lollipop's power jump reaches its egg
    for x, y, z in ((42, 76, 12), (31, 72, 8), (50, 72, 19), (45, 82, 24)):
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
        air(31, 58, 44, 31, 59, 46)
    # the library: shelf blocks between the aisles, bulldozers in the aisles
    for z0, z1 in ((34, 35), (37, 39), (41, 43), (45, 48)):
        for x in range(47, 52):
            for z in range(z0, z1 + 1):
                for y in (58, 59, 60):
                    w.set(x, y, z, rng.choice(BOOKCASES))
    for x in range(34, 45):                                  # bookcase stacks along the hall's north wall
        for y in (58, 59, 60):
            w.set(x, y, 34, rng.choice(BOOKCASES))
    for z in list(range(34, 44)) + [47, 48]:                # and its west wall
        for y in (58, 59, 60, 61):
            if w.get(32, y, z) == 0 and not (z in (38, 40)):
                w.set(32, y, z, rng.choice(BOOKCASES))
    for z in (36, 40, 44):
        mover(O.BULLDOZER, (46, 58, z), (51, 58, z))
    key(O.KEY1, 46, 58, 34)                                  # R2
    key(O.KEY2, 46, 58, 48)                                  # G1
    put(O.EGG, 46, 58, 38)                                   # in the corridor between two aisles
    # the hall: two crates, a lift to a high shelf, the mines
    for x, z, icon in ((33, 40, 66), (33, 38, 67), (44, 38, 68)):
        w.bigdecor(icon, x, 58, z)
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


def trimmed_floor(x0, z0, x1, z1, walk):
    """A white stone floor with its trim tiles mixed in (the court's mosaic)."""
    for x in range(x0, x1 + 1):
        for z in range(z0, z1 + 1):
            w.set(x, walk - 1, z, rng.choice(STONEFACE) if (x + z) % 3 == 0 else 396)


# ==== D. the white palace ========================================================================
T330_D = (11, 70, 45)          # the booth over the keep's cell (walk 70), partner in E
T331_D = (17, 58, 47)          # the court's return booth, partner in E's west wing
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
    white_stone(w, 10, 57, 40, 26, 80, 66, rng, chance=0.4)
    air(11, 58, 41, 25, 79, 65)
    trimmed_floor(11, 41, 25, 65, 58)
    air(26, 58, 44, 26, 61, 46)
    # facades: window panels on the north wall's inside (they show on +Z faces), two rows, trim round
    for x in range(11, 26):
        for y in (60, 61, 64, 65, 68, 69, 72, 73):
            if x % 2 == 1 and not (y in (68, 69) and x in (16, 17, 18)):
                w.set(x, y, 40, 391)
    for x in range(11, 26):                                   # a cornice along the top of the walls
        w.set(x, 77, 40, 394)
        w.set(x, 77, 66, 395)
    trimmed_floor(11, 43, 13, 63, 62)                         # the west balcony
    lift_(15, 58, 62, 45, ticks=45, stop=40)                   # LD1
    lift_(15, 62, 66, 63, ticks=45, stop=40)                   # LD2
    trimmed_floor(16, 62, 25, 64, 66)                         # the south balcony
    for x in range(19, 24):
        w.set(x, 65, 63, 396 if calm else SAW)
        for z in (62, 64):
            for y in (66, 67):
                w.set(x, y, z, 396)
    w.set(17, 65, 63, SWITCH)
    lift_(25, 66, 70, 60, ticks=45, stop=40)                   # LD3
    trimmed_floor(23, 43, 25, 59, 70)                         # the east balcony
    trimmed_floor(11, 43, 25, 45, 70)                         # the bridge and the keep
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
    for x, z, icon in ((14, 52, 66), (14, 58, 67), (22, 61, 68), (19, 47, 66), (18, 58, 68), (12, 50, 67),
                       (24, 54, 68), (16, 63, 68)):
        w.bigdecor(icon, x, 58, z)
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
    air(11, 39, 53, 14, 55, 56)                              # the well
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


# ==== F. the village roofs =======================================================================
HOUSES = ((33, 37, 6), (38, 42, 8), (45, 50, 8), (51, 55, 10), (56, 61, 8))   # x0, x1, height


def section_f():
    """X 33-61, Z 31-45, walk 34, stone and cobble. E's passage (z 45) opens on the west end of a
    street (z 41-45) with five houses along its north side, their windows on their south faces, roofs
    at walk 40, 42, 42, 44, 42; an alley (x 43-44) between the second and third, closed by a green
    door (z 38, G2) on a lift (x 43) up to the roofs, where three birds cross the way and four eggs
    lie; a stone wall with plants along the south side. Four birds cross the street at x 39, 44, 50,
    56. The east end (x 61) leaves by a tunnel (z 43) to G."""
    box_of_stone(32, 33, 30, 62, 48, 49)
    air(33, 34, 31, 61, 47, 45)
    air(32, 34, 45, 32, 37, 45)                              # E's passage joins the street's west end
    w.fill(33, 33, 31, 61, 33, 45, STONE)                    # cobble
    for x0, x1, h in HOUSES:
        house(w, x0, 31, x1, 40, 34, h, rng, door=(x0 + x1) // 2)
    # the alley between the second and third house: its walls are the houses' sides; door and lift
    air(43, 34, 31, 44, 47, 40)
    w.fill(43, 33, 31, 44, 33, 40, STONE)
    if not calm:
        w.fill(43, 34, 38, 44, 35, 38, GREEN_DOOR)
    lift_(43, 34, 42, 34, ticks=80, stop=40)
    # the south wall with its plants
    for x in range(33, 62):
        for y in (34, 35, 36):
            w.set(x, y, 46, rng.choice(STONE_FACES) if rng.random() < 0.3 else STONE)
    for x, icon in ((35, 23), (39, 66), (44, 24), (49, 67), (54, 68)):      # niches in the wall: plants and columns
        air(x, 34, 46, x + 1, 36, 47)
        w.bigdecor(icon, x, 34, 46)
    air(62, 34, 42, 66, 37, 44)                              # the tunnel east to G
    # the birds: four across the street, three across the roofs
    for x in (39, 44, 50, 56):
        mover(O.BIRD, (x, 34, 41), (x, 34, 45))
    for x, y in ((40, 42), (48, 42), (53, 44)):
        mover(O.BIRD, (x, y, 32), (x, y, 38))
    for x, y, z in ((35, 40, 36), (41, 42, 34), (49, 42, 34), (54, 44, 34), (60, 34, 43), (34, 34, 41)):
        put(O.EGG, x, y, z)


def box_of_stone(x0, y0, z0, x1, y1, z1):
    wall(w, x0, y0, z0, x1, y1, z1, rng, faces=STONE_FACES, face_chance=0.3, block=STONE)


# ==== G. the riveted station =====================================================================
def section_g():
    """X 67-91, Z 34-54, grey riveted plate. F's tunnel meets a blue gate (B2) in the west wall (z 42-44);
    hall 1 (x 67-77, walk 34) with sleeping follow stars and up fans at its sides; a lava service
    trench (x 78-80) crossed on two ferry lifts side by side (z 43 east, z 45 west); hall 2 (x 81-91,
    walk 30, four lower) with four left fans in its east wall (z 40, 43, 46, 49: their winds cross the
    way along x 90), B3 in its north-east corner, and the exit tunnel south (x 88-90, z 55)."""
    hall("metal", 67, 34, 34, 77, 42, 54)
    hall("metal", 81, 30, 34, 91, 42, 54)
    air(78, 34, 34, 80, 42, 54)
    air(78, 28, 34, 80, 33, 54)
    w.fill(78, 27, 34, 80, 27, 54, MACHINE_FLOOR)
    w.fill(78, 28, 34, 80, 29, 54, LAVA)
    air(62, 34, 42, 66, 37, 44)                              # F's tunnel arrives; the door closes its end
    if not calm:
        w.fill(66, 34, 42, 66, 35, 44, BLUE_DOOR)
    # the ferries over the trench (cubes at y 33, top walk 34)
    for z, a, b in ((43, 78, 80), (45, 80, 78)):
        if calm:
            w.fill(78, 33, z, 80, 33, z, MACHINE_FLOOR)
        else:
            lift(w, (a, 33, z), (b, 33, z), ticks=30, stop=40)
    # the exit tunnel south
    air(88, 30, 55, 90, 33, 59)
    # girder masts and grilles for the hall's look
    for x, z in ((67, 34), (77, 34), (67, 54), (77, 54), (81, 34), (91, 54)):
        girder_mast(w, x, z, 34 if x < 80 else 30, 41)
    for x in range(69, 76, 2):
        for z in (36, 40, 48, 52):
            w.set(x, 33, z, 84)
    for x, z in ((70, 38), (74, 50), (72, 53)):               # up fans in hall 1's floor, away from the way
        w.set(x, 33, z, FAN_UP)
        if not calm:
            w.fill(x, 34, z, x, 38, z, WIND_UP)
    for z in (40, 43, 46, 49):                               # left fans in hall 2's east wall
        w.set(92, 30, z, FAN_LEFT)
        if not calm:
            w.fill(87, 30, z, 91, 30, z, WIND_LEFT)
    key(O.KEY3, 90, 30, 35)                                  # B3
    w.bigdecor(29, 75, 34, 36)                               # a ball machine, one of the station's landmarks
    # ten sleeping follow stars: hall 1's sides, hall 2's corners
    for x, y, z in ((69, 34, 36), (69, 34, 52), (73, 34, 36), (73, 34, 52), (76, 34, 38), (83, 30, 36),
                    (83, 30, 52), (84, 30, 53), (84, 30, 33), (82, 30, 47)):
        if not calm:
            put(O.FOLLOW_STAR, x, y, z)
    mover(O.MOVING_MINE, (72, 34, 38), (72, 34, 50), 10)
    mover(O.MOVING_MINE, (75, 34, 50), (75, 34, 38), 10)
    mover(O.MOVING_MINE, (85, 30, 38), (85, 30, 50), 10)
    mover(O.MOVING_MINE, (82, 30, 49), (82, 30, 52), 10)


# ==== H. the cheese passages =====================================================================
def section_h():
    """X 62-92, Z 60-84, walk 26 down to 18, cheese. G's tunnel ends high on the north wall of the
    first cave (walk 26, centre 89, 66); a tube down to the second (walk 22, centre 79, 75) and a
    tube to the third (walk 18, centre 72, 74), whose west tube leaves (z 74) for I. Bulldozers
    patrol across the caves' floors (the way crosses their lanes), three low mines hang over the
    tubes, four standing mines hover in the caves, two helicopters wait, three eggs lie in nooks."""
    cheese(w, 62, 14, 60, 92, 34, 84, rng)
    cave(w, 89, 26, 66, 6, 7, 3, rng, rz=6)
    cave(w, 79, 22, 75, 6, 7, 3, rng, rz=5)
    cave(w, 72, 18, 74, 5, 7, 3, rng, rz=5)
    tube(w, (86, 26, 69), (81, 22, 72), r=2.6, floor=22)
    tube(w, (75, 22, 75), (73, 18, 74), r=2.6, floor=18)
    tube(w, (70, 18, 74), (60, 18, 74), r=2.6, floor=18)
    air(88, 30, 56, 90, 33, 61)                              # G's tunnel reaches the cave's wall
    # bulldozers across the first and second caves' floors, the third's
    mover(O.BULLDOZER, (84, 26, 64), (90, 26, 64), 30)
    mover(O.BULLDOZER, (77, 22, 74), (83, 22, 74), 30)
    mover(O.BULLDOZER, (69, 18, 72), (69, 18, 77), 30)
    for x, y, z in ((84, 26, 70), (78, 22, 77), (65, 18, 74)):
        low_mine(w, x, y, z, rng)
    for x, y, z in ((89, 29, 64), (80, 25, 74), (72, 21, 76), (73, 21, 71)):
        mover(O.MOVING_MINE, (x, y, z), (x, y, z), 10)
    put(O.HELICOPTER, 76, 22, 78)
    put(O.HELICOPTER, 70, 18, 77)
    for x, y, z in ((93 - 5, 26, 70), (75, 22, 78), (68, 18, 71)):
        put(O.EGG, x, y, z)


# ==== I. the helicopter jungle ===================================================================
def tower(x0, z0, x1, z1, top, block=SLIME, cap=STAR, floor_y=10):
    """A slime column from the cavern floor to walking level `top`, capped with `cap`."""
    w.fill(x0, floor_y - 1, z0, x1, top - 2, z1, block)
    w.fill(x0, top - 1, z0, x1, top - 1, z1, cap)


def section_i():
    """X 31-61, Z 53-87, walk 10 up to 30, slime with star-block perches. H's tube arrives at the east
    shelf (x 54-60, z 66-80, walk 18) where eight helicopters stand in a row; the west shelf (x 31-35,
    z 54-62, walk 12) leaves by a tunnel west to J. Between them a cavern floor at walk 10 with four
    big creatures, two spike traps and two drips, and three perches (towers capped with stars, walk 26,
    22, 30) with a helicopter pair each: the way is a flight from the east shelf over the perches and
    down to the west shelf; on foot a stair from the east shelf down to the floor and the floor's way
    west. One moving mine sweeps the middle."""
    air(31, 10, 53, 61, 37, 87)
    w.fill(54, 9, 66, 61, 17, 80, SLIME)                     # the east shelf, walk 18
    w.fill(31, 9, 54, 35, 11, 62, SLIME)                     # the west shelf, walk 12
    air(60, 18, 73, 62, 21, 75)                              # H's tube reaches the shelf
    air(27, 12, 56, 30, 15, 58)                              # the tunnel west to J
    # the stair down from the east shelf (z 82): steps at walk 16, 14, 12, 10 along x 53 to 50
    for k, x in enumerate((53, 52, 51, 50)):
        w.fill(x, 9, 78, x, 15 - 2 * k, 82, SLIME)
    # perches: towers capped with stars
    tower(44, 58, 48, 62, 26)
    tower(38, 76, 42, 80, 22)
    tower(52, 56, 56, 60, 30)
    # the overhangs and hanging slabs that break the cavern's roof into passages
    for x0, z0, x1, z1, y in ((40, 56, 50, 64, 33), (34, 70, 46, 84, 31), (50, 70, 53, 86, 29)):
        w.fill(x0, y, z0, x1, y + 4, z1, SLIME)
    # drips under small slabs on the floor's way
    for x, z in ((46, 70), (40, 66)):
        w.fill(x, 13, z, x, 13, z, SLIME)
        w.set(x, 12, z, DRIP)
    for x, z in ((44, 72), (37, 63)):                        # the two spike traps in the floor
        w.set(x, 9, z, SLIME_TRAP)
    # four big creatures walking the floor
    mover(O.BIG_CREATURE, (36, 10, 68), (46, 10, 68))
    mover(O.BIG_CREATURE, (50, 10, 76), (40, 10, 74))
    mover(O.BIG_CREATURE, (36, 10, 82), (36, 10, 72))
    mover(O.BIG_CREATURE, (48, 10, 56), (48, 10, 66))
    mover(O.MOVING_MINE, (47, 20, 72), (47, 28, 72), 10)
    # the helicopters: eight on the east shelf, two on each perch, two on the floor, two on the west shelf
    for k in range(8):
        put(O.HELICOPTER, 55 + (k % 4), 18, 68 + 3 * (k // 4) + 2 * (k // 4))
    for x, y, z in ((45, 26, 59), (47, 26, 61), (39, 22, 77), (41, 22, 79), (53, 30, 57), (55, 30, 59),
                    (33, 10, 66), (33, 10, 78), (33, 12, 55), (34, 12, 61)):
        put(O.HELICOPTER, x, y, z)
    for x, y, z in ((46, 26, 60), (40, 22, 78), (54, 30, 58), (34, 10, 70), (42, 10, 60), (33, 12, 58)):
        put(O.EGG, x, y, z)
    w.bigdecor(66, 59, 18, 67)                               # columns on the east shelf
    w.bigdecor(68, 59, 18, 79)


# ==== J. the rock gauntlet =======================================================================
def section_j():
    """X 6-27, Z 36-59, walk 12 down to 6, brown rock. I's tunnel ends at a blue gate (B3, x 29). The
    entrance ledge (x 25-27, walk 12), three steps down west (walk 10, 8, 6; x 21-24, 17-20, 8-16),
    the trench (lava x 8-16, z 48-51) across the way north: a clay bridge (x 14, four cells) or seven
    bars (x 10, z 46-52, two over the floor); the north bank (x 8-15, z 36-47, walk 6) with two left
    fans in its east wall, two ground mines and three moving mines across the way; then the gallery
    east (z 36-38, x 16-28) with two up fans at one-wide pinches and a mine bobbing between them,
    and the passage north-east to K."""
    box_of("rock", 5, 3, 35, 28, 21, 60)
    air(7, 6, 53, 16, 13, 59)                                # the south bank and the step down's hall
    air(17, 8, 55, 20, 14, 59)
    air(21, 10, 55, 24, 15, 59)
    air(25, 12, 55, 27, 17, 59)
    air(27, 12, 56, 30, 15, 58)                              # (I's tunnel's end)
    if not calm:
        w.fill(29, 12, 56, 29, 13, 58, BLUE_DOOR)
    # the floors at walk 12, 10, 8, 6 come from the rock mass below; fill what the carve took
    w.fill(25, 3, 55, 27, 11, 59, ROCK)
    w.fill(21, 3, 55, 24, 9, 59, ROCK)
    w.fill(17, 3, 55, 20, 7, 59, ROCK)
    w.fill(7, 3, 53, 16, 5, 59, ROCK)
    air(8, 6, 36, 15, 13, 52)                                # the trench's hall and the north bank
    air(8, 3, 48, 16, 5, 51)
    w.fill(8, 2, 48, 16, 2, 51, ROCK)
    w.fill(8, 3, 48, 16, 4, 51, LAVA)
    w.fill(8, 3, 52, 16, 5, 52, ROCK)                        # south bank's edge
    w.fill(8, 3, 36, 15, 5, 47, ROCK)
    for z in (48, 49, 50, 51):                               # the bridge
        w.set(14, 5, z, ROCK if calm else BRIDGE)
    if calm:
        w.fill(10, 5, 48, 10, 5, 51, ROCK)
    else:
        w.fill(10, 8, 46, 10, 8, 52, BAR)                    # the bars
    # the fans and mines on the north bank
    for z in (42, 46):
        w.set(16, 6, z, FAN_LEFT)
        if not calm:
            w.fill(8, 6, z, 15, 6, z, WIND_LEFT)
    hazard(9, 6, 43)
    hazard(15, 6, 39)
    mover(O.MOVING_MINE, (8, 6, 44), (15, 6, 44), 10)
    mover(O.MOVING_MINE, (15, 6, 40), (8, 6, 40), 10)
    mover(O.MOVING_MINE, (12, 6, 37), (15, 6, 37), 10)
    # the gallery east and the passage north-east
    air(16, 6, 36, 28, 13, 38)
    w.fill(16, 5, 36, 28, 5, 38, ROCK)
    for x in (19, 25):                                       # pinches one wide, an up fan in each
        w.fill(x, 6, 36, x, 13, 36, ROCK)
        w.fill(x, 6, 38, x, 13, 38, ROCK)
        w.set(x, 5, 37, FAN_UP)
        if not calm:
            w.fill(x, 6, 37, x, 11, 37, WIND_UP)
    mover(O.MOVING_MINE, (22, 10, 37), (22, 12, 37), 10)
    air(28, 6, 28, 30, 9, 38)                                # the passage north to K's south end
    put(O.EGG, 9, 6, 47 - 2)
    put(O.EGG, 27, 12, 57)
    for x in (9, 12, 15):                                    # rock outcrops for the walls' look
        w.set(x, 7, 36, 301)


# ==== K. the toy chamber =========================================================================
PADS = ((34, 24, 8), (37, 23, 10), (40, 24, 12), (43, 23, 14), (44, 20, 16), (44, 17, 18), (41, 17, 20), (38, 17, 22))


def section_k():
    """X 32-50, Z 11-26, walk 6 up to 22, ball blocks. J's passage enters at the south wall's west end
    (x 32-34, z 27); eight two-by-two ball-block columns (walk 8, 10, ... 22) spiral up round the
    chamber, a cell or two between them; the last reaches the exit ledge (x 32-36, z 14-18, walk 22)
    whose south and east edges are fenced by ten flight-barrier cells (the entrance, x 37, z 15-17, is open); the tunnel west to L leaves it. Two eggs on the
    columns, a few star blocks among the balls."""
    hall("slime", 32, 6, 11, 50, 28, 26)
    w.fill(31, 5, 10, 51, 5, 27, SLIME)
    for x in range(32, 51):                                  # the ball-block floor and walls' skin
        for z in range(11, 27):
            w.set(x, 5, z, rng.choice(BALLS))
    for x0, z0, top in PADS:
        for x in (x0, x0 + 1):
            for z in (z0, z0 + 1):
                for y in range(6, top):
                    w.set(x, y, z, STAR if (y == top - 1 and (x + z) % 2 == 0) else rng.choice(BALLS))
    floor(32, 14, 36, 18, 22, block=216, thick=3)
    for y in (22, 23):                                       # the ten barrier cells fence the ledge's open sides
        for x in (34, 35, 36):
            w.set(x, y, 19, BARRIER)
        for z in (14, 18):
            w.set(37, y, z, BARRIER)
    air(28, 22, 14, 33, 26, 16)                              # the tunnel west to L
    air(28, 6, 27, 34, 9, 30)                                # J's passage reaches the chamber's south end
    put(O.EGG, 43, 14, 23)
    put(O.EGG, 41, 20, 17)
    for x, z in ((32, 11), (50, 11), (50, 26)):
        w.set(x, 6, z, STAR)


# ==== L. the basin, the column ascent and the monument ===========================================
L_PADS = ((13, 24, 12), (13, 21, 14), (13, 18, 16), (13, 15, 18), (12, 12, 20), (11, 9, 22))   # x, z, top walk


def star_lift(x, walk0, walk1, z, ticks=60, stop=40):
    """A short lift drawn as a star block (MoveObjectRecord's visual icon, BD-0082)."""
    w.place(O.LIFT, x, walk0 - 1, z, end=(x, walk1 - 1, z), advance=ticks, recede=ticks, stop_start=stop,
            stop_end=stop, icon=STAR)


def section_l():
    """X 9-26, Z 9-34, walk 10 up to 34, water, slime, columns and (only here and in K) star blocks.
    K's tunnel (z 14-16, walk 22) opens high on the east wall over the east bank (x 19-25, z 10-17,
    walk 10); south of it the basin (x 10-25, z 16-33, five deep) with an island (x 16-20, z 23-27:
    a palm and G3), six two-by-two columns standing in the water along x 13 and 10 (tops walk 12, 14,
    ... 22) climbing north, then two star lifts (x 13, z 9: 22 to 28; x 17, z 11: 28 to 34) up to the
    monument terrace (x 18-25, z 10-17, walk 34): the Blupi statue with its column, two fountains and vases,
    and in the north wall the green gate (z 9) to the frame of star blocks (x 19-23, z 4-8) with the
    red arrow. Two helicopters wait on the banks."""
    air(9, 4, 9, 26, 47, 34)
    w.fill(9, 3, 9, 26, 3, 34, SLIME)
    air(26, 22, 14, 28, 26, 16)                              # K's tunnel opens on the cave
    # the banks: the east bank, the north strip, the island
    w.fill(19, 4, 10, 25, 9, 17, SLIME)
    w.fill(9, 4, 9, 18, 9, 15, SLIME)
    w.fill(16, 4, 23, 20, 9, 27, SLIME)
    # the water
    for x0, z0, x1, z1 in ((10, 16, 25, 22), (10, 23, 15, 33), (21, 23, 25, 33), (16, 28, 20, 33)):
        water(w, x0, 4, z0, x1, 9, z1)
    # the columns standing in the water or on the strip, capped with stars
    for x0, z0, top in L_PADS:
        w.fill(x0, 4, z0, x0 + 1, top - 2, z0 + 1, SLIME)
        w.fill(x0, top - 1, z0, x0 + 1, top - 1, z0 + 1, STAR)
    star_lift(13, 22, 28, 9)
    floor(14, 9, 16, 11, 28, block=STAR, thick=2)            # the landing between the lifts
    star_lift(17, 28, 34, 11)
    # the monument terrace
    floor(18, 10, 25, 17, 34, block=396, thick=2)
    w.bigdecor(87, 21, 34, 15)                               # the monument column
    w.bigdecor(89, 21, 34, 13)                               # the statue
    w.bigdecor(88, 19, 34, 14)                               # the two fountains
    w.bigdecor(88, 23, 34, 14)
    w.bigdecor(69, 18, 34, 16)
    w.bigdecor(69, 24, 34, 16)
    # the green gate and the frame of star blocks
    box_of("slime", 17, 33, 2, 25, 40, 9)
    air(19, 34, 4, 23, 37, 8)
    air(20, 34, 9, 22, 37, 9)
    w.fill(19, 33, 4, 23, 33, 8, STAR)                       # the compact frame of star blocks: the floor under the arrow,
    w.fill(20, 34, 3, 22, 36, 3, STAR)                       # the wall behind it,
    w.fill(19, 33, 9, 19, 37, 9, STAR)                       # and the gateway's posts and lintel
    w.fill(23, 33, 9, 23, 37, 9, STAR)
    w.fill(20, 36, 9, 22, 37, 9, STAR)
    if not calm:
        w.fill(20, 34, 9, 22, 35, 9, GREEN_DOOR)
    put(O.EXIT, 21, 34, 5)
    key(O.KEY2, 18, 10, 25)                                  # G3 on the island
    w.bigdecor(18, 19, 10, 26)                               # the palm
    w.bigdecor(23, 23, 10, 12)
    w.bigdecor(24, 11, 10, 11)
    for x, y, z, icon in ((20, 10, 11, 67), (24, 10, 10, 68), (25, 34, 10, 68), (18, 34, 17, 67), (25, 34, 17, 68)):
        w.bigdecor(icon, x, y, z)
    w.bigdecor(203, 13, 4, 28)
    w.bigdecor(203, 23, 4, 28)
    put(O.EGG, 19, 10, 25)
    put(O.EGG, 11, 22, 10)
    put(O.HELICOPTER, 12, 10, 11)
    put(O.HELICOPTER, 24, 10, 11)


SECTIONS = [section_a, section_b, section_c, section_d, section_e, section_f, section_g, section_h, section_i, section_j, section_k, section_l]


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
