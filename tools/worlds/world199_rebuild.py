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
    air(25, 58, 44, 30, 61, 46)                              # the tunnel west to D
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


SECTIONS = [section_a, section_b, section_c]


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
