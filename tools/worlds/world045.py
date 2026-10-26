#!/usr/bin/env python3
"""Mission 045, the cheese mountain under the red curtains (worlds3d/world045.vwr): brief
plans/briefs/world045.md.

Mobile Eggbert's level 045, the longest of the cheese world, under the region-3 red curtains.
On foot first: beside a house and a tree, a lift up to treasure 1; a lift sliding over lava;
three little lifts that pop up under hanging mines, then the treasure door "1"; four lifts at
four speeds to hop across over a pit of mines; three lifts carrying Blupi over a gap to where
the bulldozer drives; seven helicopters beside a treasure between two mines. Then flying: a
machine cage with a treasure behind bars, crushers before it and before the red arrow; a
cellar of lava pools with six treasures and nine helicopters; a pool with eggs at its bottom;
secret passages of walk-through cheese to an attic with the crate and two treasures and to a
pocket of eggs; seven invert pickups on the way, seventeen eggs. Here the on-foot half runs
east from a plateau under the curtains into the mountain, and the flying half is the
mountain's core, a cavern whose walls hold the other places.

    python3 tools/worlds/world045.py                 # writes worlds3d/world045.vwr
    python3 tools/worlds/world045.py --check STAGE out
        STAGE door1:  door 1 open
        STAGE timed:  also the bulldozer left out (passed at the right moment), the pads'
                      mines too (passed crouching), and a step beside the plateau lift's low
                      end (levelcheck rides a lift between the cells beside its ends; it does
                      not jump onto one)
        STAGE solved: also the crate pushed under its ledge
    Check with `tools/flightcheck.py --mine-margin 0`.
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from cheese import (CHEESE, CRUSHER, DEEP_WATER, HOUSE, LAVA, MACHINE, MECA, PLANTS, SECRET,  # noqa: E402
                    TREASURE_DOOR1, TREES, WATER, carve, cave, cheese, hang, helicopters, low_mine,
                    patrol, shaft)

W = 20           # the on-foot half's walking level
SPAWN = (22, W, 80)
BARS = 22        # the machine cage's grating (decor, no collision)
PILLAR = 76
PROPS = (55, 56)
STAGE = None


def moving_lift(w, start, end, ticks, stop=10, recede=None, stops=None):
    """A lift (ObjectType 1) with the original's tick timings (dwell `stops` at start, end)."""
    s0, s1 = stops if stops else (stop, stop)
    w.place(O.LIFT, *start, end=end, advance=ticks, recede=recede if recede else ticks,
            stop_start=s0, stop_end=s1)


def plateau(w: World, rng: random.Random):
    """S and I1: the start plateau (walk 20) under the curtains with the house and the trees;
    a ledge six high in its west corner with treasure 1 and three eggs, a lift up to it; the
    lava moat (z 63-69) crossed on three sliding lifts in step to the island I1."""
    cheese(w, 8, 4, 50, 32, W - 1, 92, rng)
    carve(w, 8, 14, 63, 32, W - 1, 69)                    # the moat
    w.fill(8, 13, 63, 32, 13, 69, LAVA)
    w.bigdecor(HOUSE, 18, W, 87)
    for icon, x, z in ((TREES[0], 27, 88), (TREES[2], 12, 76), (TREES[1], 29, 74), (TREES[0], 25, 56)):
        w.bigdecor(icon, x, W, z)
    w.bigdecor(PLANTS[1], 15, W, 58)
    # The ledge in the west corner (walk 26) and its lift.
    cheese(w, 8, W, 82, 12, W + 5, 91, rng)
    w.place(O.TREASURE, 10, W + 6, 87)
    for x in (9, 10, 11):
        w.place(O.EGG, x, W + 6, 84)
    moving_lift(w, (13, W, 87), (13, W + 5, 87), ticks=50)
    if STAGE in ("timed", "solved"):                      # the jump onto it (levelcheck)
        w.set(14, W, 87, rng.choice(CHEESE))
    w.place(O.INVERT, 24, W, 72)
    # The sliding lifts over the moat: a raft three wide and two long of lifts in step, at the
    # original's speed (60 ticks for six cells), waiting a second and a half at each bank so
    # that Blupi can step on and off (the original's half second, in 3D a blind step).
    for x in (19, 20, 21):
        for z0 in (69, 68):
            moving_lift(w, (x, W - 1, z0), (x, W - 1, z0 - 5), ticks=50, stops=(30, 30))
    w.place(O.EGG, 14, W, 54)
    w.place(O.EGG, 22, W, 52)


def gallery(w: World, rng: random.Random):
    """P: the gallery east from the island (walk 20, three wide, four high). Across it a strip
    of lava five long; over it a line of stones one wide with, between them, three pads: lifts
    sinking a cell below the line and popping back up flush with it (5 ticks up, 22 down; the
    middle one 6 and 6). A mine hangs in each pad's column at Blupi's height from a drop of
    cheese: he crosses crouching (a hanging mine is harmless to a crouching Blupi) and steps
    off a pad while it is up. In 3D a lift does not stop him sideways, so a pad never rises
    above the line (he would walk into it and fall through), and it rests a second at the top
    (the original's 2-3 ticks) to leave him time to step off. Door 1 at the end."""
    cheese(w, 33, 14, 52, 45, 26, 60, rng)
    carve(w, 33, W, 55, 44, W + 3, 57)
    carve(w, 35, W - 2, 55, 39, W - 1, 57)                 # the lava strip
    w.fill(35, W - 3, 55, 39, W - 3, 57, LAVA)
    for x in (36, 38):                                     # the stones
        cheese(w, x, W - 2, 56, x, W - 1, 56, rng)
    for k, x in enumerate((35, 37, 39)):
        if k == 1:
            moving_lift(w, (x, W - 1, 56), (x, W - 2, 56), ticks=6, stops=(20, 3))
        else:
            moving_lift(w, (x, W - 2, 56), (x, W - 1, 56), ticks=5, recede=22, stops=(2, 20))
        low_mine(w, x, W, 56, rng)
        if STAGE in ("timed", "solved"):                   # crouching, they are harmless
            w.records.pop((x, W, 56), None)
    w.place(O.INVERT, 34, W, 55)


def hop_hall(w: World, rng: random.Random):
    """H: behind door 1 a hall over a pit (floor walk 10, mines on it) between the entry ledge
    and the exit ledge (both walk 20). The original's four lifts at four speeds -- 50, 70, 40
    and 20 ticks, spans of 5, 7, 4 and 2 -- become a chain of four lift columns, each two deep,
    with a rest column between each two at their meeting height: L1 down from the entry ledge
    to rest 1 (walk 15), L2 up to rest 2 (22), L3 down to rest 3 (18), L4 up to the exit ledge.
    Blupi rides one, steps onto the rest, waits for the next (the rest beyond each lift blocks
    him until the lift is level with it). A fall lands on the pit floor; a stair of two-steps
    along its south side leads back up to the entry ledge."""
    cheese(w, 46, 6, 50, 64, 30, 62, rng)
    carve(w, 46, 10, 52, 62, 29, 60)
    cheese(w, 46, 10, 52, 48, W - 1, 60, rng)              # the entry ledge
    cheese(w, 56, 10, 52, 62, W - 1, 60, rng)              # the exit ledge
    carve(w, 45, W, 56, 45, W + 1, 56)                     # the doorway from the gallery
    # (x, top at the start, top at the end, ticks): the lifts, in counter-step pairs.
    chain = ((49, 20, 15, 50), (51, 15, 22, 70), (53, 22, 18, 40), (55, 18, 20, 20))
    for x, a, b, ticks in chain:
        for z in (56, 57):
            moving_lift(w, (x, a - 1, z), (x, b - 1, z), ticks=ticks, stops=(20, 20))
    for x, top in ((50, 15), (52, 22), (54, 18)):          # the rests
        cheese(w, x, 10, 56, x, top - 1, 57, rng)
    for x, z in ((51, 58), (53, 55), (50, 54)):
        mine(w, x, 10, z)
    w.place(O.EGG, 60, W, 56)
    for k, x in enumerate(range(55, 50, -1)):              # the stair, walk 12 .. 20
        cheese(w, x, 10, 52, x, 11 + 2 * k, 53, rng)
    cheese(w, 49, 10, 52, 50, W - 1, 53, rng)


def road(w: World, rng: random.Random):
    """B: from the exit ledge three lifts in step (40 ticks, waiting two seconds at the ledge)
    carry Blupi over a gap seven long and four deep (a stair in it back up to the ledge after a
    fall) to the road (walk 20, three wide), where the bulldozer drives to and fro in the
    middle lane (180 ticks); at two pinch points mines lie in both side lanes, so Blupi passes
    there in the bulldozer's lane while it is away; a mine hangs under the roof over the gap;
    an invert on the road."""
    cheese(w, 63, 10, 52, 87, 26, 60, rng)
    carve(w, 63, W, 55, 87, W + 4, 57)
    carve(w, 63, W - 4, 55, 69, W - 1, 57)                 # the gap, four deep
    for k in range(3):                                     # a stair back up to the ledge
        cheese(w, 63 + k, W - 4, 55, 63 + k, W - 2 - k, 55, rng)
    for z in (55, 56, 57):                                 # it waits two seconds at the ledge
        moving_lift(w, (63, W - 1, z), (69, W - 1, z), ticks=40, stops=(40, 10))
    hang(w, 66, W + 2, 56)
    if STAGE not in ("timed", "solved"):
        patrol(w, O.BULLDOZER, (72, W, 56), (84, W, 56), ticks=180, stop=20)
    for x in (75, 81):                                     # pinch points: only the bulldozer's lane
        mine(w, x, W, 55)
        mine(w, x, W, 57)
    w.place(O.INVERT, 78, W, 55)


def hangar(w: World, rng: random.Random):
    """G: the hangar at the road's end (walk 20): seven helicopters in a row, and along its
    north side a mine, an invert, treasure 2 and a mine; a machine (BigDecor) in a corner; its
    north wall open onto a balcony over the mountain's core."""
    cheese(w, 84, 12, 42, 98, 30, 66, rng)
    carve(w, 86, W, 46, 96, W + 7, 64)
    carve(w, 84, W, 55, 86, W + 4, 57)
    helicopters(w, [(88 + k, W, 61 if k % 2 else 62) for k in range(7)])
    mine(w, 88, W, 48)
    w.place(O.INVERT, 89, W, 48)
    w.place(O.TREASURE, 90, W, 48)
    mine(w, 91, W, 48)
    w.bigdecor(MECA[1], 95, W, 63)
    carve(w, 87, W, 42, 95, W + 5, 45)                     # the opening north


def core(w: World, rng: random.Random):
    """K: the mountain's core, a cavern (x 56-92, z 10-40, floor walk 8 up to y 56) whose
    walls hold the flying half's places: the balcony from the hangar (walk 20), the exit
    landing (walk 36, west), the cage on its column (walk 44, middle), the pool grotto (walk
    26, north), the attic behind secret cheese (walk 52, west) and the cellar on its floor."""
    cheese(w, 46, 4, 2, 98, 60, 44, rng)
    carve(w, 56, 8, 10, 92, 56, 40)
    cheese(w, 84, 8, 36, 92, W - 1, 41, rng)                # the balcony under the opening
    carve(w, 84, W, 36, 92, W + 5, 41)


def cellar(w: World, rng: random.Random):
    """R: the core's floor (walk 8): two lava pools, six treasures, nine helicopters, two eggs,
    a crusher before one treasure, a mine, an invert; in its east wall a wall of secret cheese
    in front of a pocket with three eggs."""
    for x0, z0 in ((64, 18), (78, 28)):
        carve(w, x0, 6, z0, x0 + 3, 7, z0 + 3)
        w.fill(x0, 6, z0, x0 + 3, 6, z0 + 3, LAVA)
    for x, z in ((64, 13), (70, 32), (86, 14), (88, 30), (66, 22), (75, 12)):
        w.place(O.TREASURE, x, 8, z)
    w.set(66, 7, 23, CRUSHER)                              # before the treasure at (66, 22)
    w.set(65, 7, 23, CRUSHER)
    w.set(67, 7, 23, CRUSHER)
    helicopters(w, [(58, 8, 30), (60, 8, 33), (62, 8, 30), (72, 8, 20), (74, 8, 36), (82, 8, 16),
                    (84, 8, 22), (88, 8, 34), (78, 8, 20)])
    w.place(O.EGG, 62, 8, 37)
    w.place(O.EGG, 64, 8, 37)
    mine(w, 87, 8, 31)
    w.place(O.INVERT, 70, 8, 29)
    for icon, x, z in ((MECA[0], 58, 36), (MECA[2], 90, 12)):
        w.bigdecor(icon, x, 8, z)
    # The secret pocket east: a wall of walk-through cheese, the eggs behind it.
    carve(w, 93, 8, 20, 96, 10, 24)
    w.fill(93, 8, 21, 93, 9, 23, SECRET)
    for z in (21, 22, 23):
        w.place(O.EGG, 95, 8, z)


def exit_and_cage(w: World, rng: random.Random):
    """X: the exit landing in the core's west wall (walk 36) with a helicopter and an invert;
    a corridor west with a crusher across it, the red arrow at its end. C: the cage on a
    column in the middle (walk 44): machine panels round it, gratings in its west and east
    walls, a crusher across each landing before them, treasure 3 inside beside a pillar."""
    cheese(w, 56, 8, 20, 62, 35, 26, rng)                   # the landing's buttress
    carve(w, 48, 36, 22, 56, 38, 24)                        # the corridor
    for z in (22, 23, 24):
        w.set(53, 35, z, CRUSHER)
    w.place(O.EXIT, 50, 36, 23)
    helicopters(w, [(60, 36, 23)])
    w.place(O.INVERT, 58, 36, 21)
    # The cage's column, its two landings, the cage (five by five inside, walls three high and
    # open above, so the camera looks in).
    cheese(w, 71, 8, 21, 77, 43, 27, rng)
    cheese(w, 65, 41, 23, 70, 43, 25, rng)
    cheese(w, 78, 41, 23, 83, 43, 25, rng)
    for z in (23, 24, 25):                                 # a cell short of the gratings
        w.set(69, 43, z, CRUSHER)
        w.set(79, 43, z, CRUSHER)
    w.box(71, 44, 21, 77, 46, 27, MACHINE, hollow=True, floor=False, roof=False)
    for x in (71, 77):
        for z in (23, 24, 25):
            w.set(x, 44, z, BARS)
            w.set(x, 45, z, BARS)
    w.place(O.TREASURE, 74, 44, 24)
    w.set(74, 44, 25, PILLAR)
    w.set(73, 44, 22, PROPS[0])
    w.set(75, 44, 26, PROPS[1])


def pool(w: World, rng: random.Random):
    """W: the pool grotto in the core's north wall (walk 26): a pool two deep, three eggs at
    its bottom (a dive: the helicopter is wrecked in water), four helicopters on its banks."""
    cave(w, 70, 26, 6, 6, 5, 2, rng, rz=4)
    carve(w, 64, 26, 8, 76, 30, 10)
    cheese(w, 64, 8, 8, 76, 25, 11, rng)                    # the landing in the mouth
    carve(w, 67, 23, 4, 73, 25, 7)
    w.fill(67, 23, 4, 73, 24, 7, DEEP_WATER)
    w.fill(67, 25, 4, 73, 25, 7, WATER)                      # level with the banks: climb out
    for x in (69, 70, 71):
        w.place(O.EGG, x, 23, 5)
    helicopters(w, [(65, 26, 6), (66, 26, 9), (75, 26, 6), (74, 26, 9)])


def attic(w: World, rng: random.Random, solved: bool):
    """A: the attic behind a wall of secret cheese high in the core's west wall (walk 52):
    treasures 4 and 5 (one on a ledge three high, the crate to push under it), three eggs."""
    cheese(w, 56, 8, 12, 62, 51, 18, rng)                   # the landing's buttress
    carve(w, 49, 52, 11, 54, 56, 19)
    w.fill(55, 52, 14, 55, 53, 16, SECRET)
    cheese(w, 49, 52, 11, 50, 54, 13, rng)                  # the ledge, three high
    w.place(O.TREASURE, 49, 55, 12)
    w.place(O.TREASURE, 53, 52, 18)
    for x in (51, 52, 53):
        w.place(O.EGG, x, 52, 11)
    w.place(O.CRATE, *((51, 52, 12) if solved else (53, 52, 15)))


def doors(w: World):
    for yy in (W, W + 1):
        w.set(45, yy, 56, 0 if STAGE else TREASURE_DOOR1)


def main(argv) -> int:
    global STAGE
    STAGE = argv[2] if len(argv) > 3 and argv[1] == "--check" else None
    rng = random.Random(45)
    w = World(mission=45, sky=3)
    core(w, rng)
    plateau(w, rng)
    gallery(w, rng)
    hop_hall(w, rng)
    road(w, rng)
    hangar(w, rng)
    cellar(w, rng)
    exit_and_cage(w, rng)
    pool(w, rng)
    attic(w, rng, solved=STAGE == "solved")
    doors(w)
    w.spawn = SPAWN
    out = Path(argv[3]) if STAGE else Path(__file__).resolve().parents[2] / "worlds3d" / "world045.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
