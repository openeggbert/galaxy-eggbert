#!/usr/bin/env python3
"""Mission 064, the slime well (worlds3d/world064.vwr): brief plans/briefs/world064.md.

Mobile Eggbert's level 064 is one tall well of slime climbed from the bottom to the red arrow at
its top: a zigzag of small shuttles under the fire of two blupits, a treasure door "1", a long
lift, a pool with two fish and a treasure under water, a moving mine by two treasures, the
door "4"; a stack of corridors with moving mines and a bird, the potion (Hide) before a stair of
mines and a slope of saws; two shuttles crossing, a bulldozer under bars, the lollipop (Power)
for a high jump, hanging mines, bars over moving mines; a garage of helicopters at the foot of a
hall of four treasures on pillars, a second garage above it; at the top a switch stopping the
saws before a treasure, a flooded bend before another, and the arrow in a box with a grate roof.
Here the well is built as storeys in 3D: a spiral of lifts pumping up round a core, the pool
under the gallery storey, a spiral of corridors round another core, the stair of mines and saws,
the shuttle room, the bulldozer's corridor, the corridors of hanging mines and of bars, the hall
of the helicopters, its upper garage and the top cave.

    python3 tools/worlds/world064.py                 # writes worlds3d/world064.vwr
    python3 tools/worlds/world064.py --stage K out   # K treasures taken: doors "1" (K >= 1) and
                                                     # "4" (K >= 4) open; K >= 4 also the potion's,
                                                     # the lollipop's, the bars' and the switch's
                                                     # ways as plain floors and steps, no enemies
                                                     # on them, the flooded bend a doorway (for
                                                     # levelcheck and flightcheck)
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, lift, mine  # noqa: E402
from slime import POLES, skin, slime  # noqa: E402

WATER, DEEP = 92, 91
SAW, SWITCH, BAR, GIRDER = 378, 384, 138, 201
FRAME, GRATE_ROOF = 4, 73            # the arrow's box: blue blocks, a passable top
PROPS = (50, 53, 54)


def door(n):
    return 420 + n                   # opens once n treasures are taken (Decor::OpenDoorsTresor)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world064.vwr"
    rng = random.Random(64)
    w = World(mission=64, sky=31)
    solved = stage is not None and stage >= 4

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def patrol(otype, start, end, ticks, stop):
        w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def gate(x0, y0, z0, x1, y1, z1, n):
        """Treasure door n filling a doorway, open in the stages from n treasures on."""
        carve(x0, y0, z0, x1, y1, z1)
        if stage is None or stage < n:
            w.fill(x0, y0, z0, x1, y1, z1, door(n))

    slime(w, 34, 2, 32, 62, 38, 62)        # the bottom, the long lift, the pool storey
    slime(w, 56, 28, 32, 76, 49, 52)       # the corridors' spiral
    slime(w, 70, 40, 40, 97, 66, 54)       # the stair, the shuttles, the bulldozer
    slime(w, 52, 60, 10, 78, 97, 46)       # hanging mines, bars, the hall, the top

    # ---- the bottom (walk 5) ----
    # A0, Blupi's start: treasure 1 and an egg; a blupit walks one cell and shoots along the
    # room's north edge. North-west the spiral: six lifts (2 x 2) round a core, each pumping two
    # up and down (20 ticks, 20 dwell), odd ones rising while even ones sink, so that each is
    # level with the next in turn: Blupi steps across and is pumped up to the landing (walk 17).
    # A second blupit shoots across the spiral's north side from a niche in its wall.
    carve(40, 5, 44, 58, 9, 58)
    carve(40, 5, 38, 45, 21, 43)
    slime(w, 42, 5, 40, 43, 21, 41)                                   # the core
    w.place(O.TREASURE, 55, 5, 47)
    w.place(O.EGG, 57, 5, 56)
    patrol(O.CREATURE, (47, 5, 45), (48, 5, 45), 10, 23)
    carve(37, 11, 38, 39, 12, 38)
    patrol(O.CREATURE, (37, 11, 38), (38, 11, 38), 10, 23)
    spiral = [((40, 42), 4), ((40, 40), 6), ((40, 38), 8), ((42, 38), 10), ((44, 38), 12), ((44, 40), 14)]
    carve(40, 4, 42, 41, 4, 43)                                       # the first one's low stop
    for k, ((x0, z0), low) in enumerate(spiral):
        ys = (low, low + 2) if k % 2 == 0 else (low + 2, low)
        for x in (x0, x0 + 1):
            for z in (z0, z0 + 1):
                if solved:
                    w.set(x, low, z, 10)                              # each at its low stop
                else:
                    lift(w, (x, ys[0], z), (x, ys[1], z), ticks=20, stop=20)
    slime(w, 44, 16, 42, 45, 16, 43)                                  # the landing, walk 17

    # ---- door "1", the long lift (walk 17 -> 32) ----
    gate(46, 17, 42, 46, 18, 43, 1)
    carve(47, 17, 36, 58, 21, 46)
    carve(56, 16, 38, 57, 34, 39)
    for x in (56, 57):
        for z in (38, 39):
            lift(w, (x, 16, z), (x, 31, z), ticks=90, stop=3)

    # ---- the pool storey (walk 32) ----
    # R2: treasures 3 and 4 on ledges two up, a moving mine sweeping the floor to an egg. South
    # of it the pool room: the pool flush with its floor, seven deep, two fish crossing it, the
    # second treasure and an egg on its bottom. Door "4" in R2's east wall.
    carve(40, 32, 36, 58, 35, 46)
    slime(w, 41, 32, 36, 43, 33, 37)
    w.place(O.TREASURE, 42, 34, 36)
    slime(w, 47, 32, 36, 49, 33, 37)
    w.place(O.TREASURE, 48, 34, 36)
    w.place(O.EGG, 40, 32, 41)
    patrol(O.MOVING_MINE, (42, 32, 41), (53, 32, 41), 120, 0)
    carve(41, 32, 47, 42, 33, 47)
    carve(38, 32, 48, 46, 35, 56)
    carve(39, 25, 49, 45, 31, 55)
    w.fill(39, 25, 49, 45, 30, 55, DEEP)
    w.fill(39, 31, 49, 45, 31, 55, WATER)
    w.place(O.TREASURE, 44, 25, 54)
    w.place(O.EGG, 40, 25, 50)
    patrol(O.FISH, (40, 27, 51), (44, 27, 51), 60, 47)
    patrol(O.FISH, (44, 29, 53), (40, 29, 53), 60, 47)
    gate(59, 32, 45, 59, 33, 46, 4)
    carve(60, 32, 45, 61, 33, 46)

    # ---- the corridors' spiral (walk 32, 35, 38, 41, 44) ----
    # Five corridors two wide and three high round a core, each three above the last (they meet
    # only at the corners): at each corner a step in
    # the lower one's end and a hole in the upper one's floor. A moving mine in the first and the
    # last, a bird in the third, each on the outer lane; eggs in the second and the fourth;
    # striped poles in niches; the potion at the end of the fifth.
    carve(60, 32, 47, 72, 34, 48)                                     # C1, south
    slime(w, 71, 32, 47, 72, 32, 48)
    carve(71, 35, 36, 72, 37, 48)                                     # C2, east
    carve(71, 34, 47, 72, 34, 48)
    slime(w, 71, 35, 36, 72, 35, 37)
    carve(60, 38, 36, 72, 40, 37)                                     # C3, north
    carve(71, 37, 36, 72, 37, 37)
    slime(w, 60, 38, 36, 61, 38, 37)
    carve(60, 41, 36, 61, 43, 48)                                     # C4, west
    carve(60, 40, 36, 61, 40, 37)
    slime(w, 60, 41, 47, 61, 41, 48)
    carve(60, 44, 47, 72, 46, 48)                                     # C5, south again
    carve(60, 43, 47, 61, 43, 48)
    if not solved:
        patrol(O.MOVING_MINE, (62, 32, 48), (70, 32, 48), 60, 0)
        patrol(O.BIRD, (70, 38, 36), (62, 38, 36), 40, 8)
        patrol(O.MOVING_MINE, (62, 44, 48), (69, 44, 48), 60, 0)
    w.place(O.EGG, 72, 35, 39)
    w.place(O.EGG, 60, 41, 45)
    for x in (64, 68):
        for y in (32, 44):
            w.fill(x, y, 49, x, y + 1, 49, rng.choice(POLES))
    w.place(O.DRINK, 72, 44, 48)

    # ---- the potion's stair (walk 45 -> 56) ----
    # East from the fifth corridor a stair one wide: a mine on each of its first four steps, then
    # seven steps that are saws. Under the potion's Hide Blupi climbs it unharmed.
    for k in range(11):
        x, top = 73 + k, 44 + k
        carve(x, top + 1, 48, x, top + 6, 48)                         # room for the camera
        if k >= 4:
            w.set(x, top, 48, 10 if solved else SAW)
            w.rotate_plate(x, top, 48)
        elif not solved:
            mine(w, x, top + 1, 48)

    # ---- the shuttle room (walk 56 -> 60) ----
    # From the stair's landing two shuttles (2 x 2, 30 ticks, 3 dwell) cross the room in opposite
    # directions, the second two above the first and beside it: Blupi rides the first out, jumps
    # onto the second as they pass, rides it back and jumps up to the ledge of the next corridor.
    # Under them a pit (walk 52) with steps back up to the landing.
    carve(84, 52, 44, 93, 62, 49)
    slime(w, 84, 52, 47, 84, 55, 49)                                  # the landing, walk 56
    slime(w, 85, 52, 49, 86, 53, 49)                                  # a step, walk 54
    slime(w, 84, 52, 45, 84, 59, 46)                                  # the ledge, walk 60
    for x in (0, 1):
        for z in (47, 48):
            if solved:
                w.fill(85, 55, z, 92, 55, z, 10)
            else:
                lift(w, (91 + x, 55, z), (85 + x, 55, z), ticks=30, stop=3)
        for z in (45, 46):
            if solved:
                w.fill(85, 57, z, 86, 57, z, 10)
            else:
                lift(w, (85 + x, 57, z), (91 + x, 57, z), ticks=30, stop=3)

    # ---- the bulldozer's corridor (walk 60) ----
    # West from the ledge, one wide: the bulldozer patrols it; bars under its ceiling to hang
    # from while it passes under; the lollipop at its west end, where a column rises four to the
    # next corridor: a jump only Power gives.
    carve(73, 60, 45, 83, 62, 45)
    w.fill(75, 61, 45, 82, 61, 45, BAR)
    if not solved:
        patrol(O.BULLDOZER, (82, 60, 45), (77, 60, 45), 90, 20)
    w.place(O.LOLLIPOP, 74, 60, 45)
    carve(73, 63, 45, 74, 66, 45)
    if solved:
        slime(w, 73, 60, 45, 73, 61, 45)                              # a step, walk 62

    # ---- hanging mines, then bars over moving mines (walk 64, 67) ----
    # North from the column a corridor one wide under three hanging mines (walk under them, never
    # jump), then west, two wide, a moving mine in one lane and a dip with a mine in the other.
    # Then up into the bars' corridor: two moving mines sweep it, Blupi hangs from the bars above
    # them.
    carve(73, 64, 36, 73, 65, 44)
    carve(66, 64, 35, 73, 66, 35)
    carve(67, 64, 34, 73, 66, 34)                                     # a second lane
    carve(70, 63, 35, 70, 63, 35)                                     # the dip
    slime(w, 66, 64, 35, 66, 64, 35)                                  # a step, walk 65
    carve(66, 67, 35, 66, 68, 35)
    if not solved:
        for z in (41, 40, 39):
            mine(w, 73, 65, z, hanging=True)
        mine(w, 70, 63, 35)
        patrol(O.MOVING_MINE, (72, 64, 34), (67, 64, 34), 40, 0)
    carve(66, 67, 28, 66, 69, 34)
    w.fill(66, 68, 29, 66, 68, 34, BAR)                              # the first over the mouth
    if not solved:
        patrol(O.MOVING_MINE, (66, 67, 29), (66, 67, 33), 50, 0)
        patrol(O.MOVING_MINE, (66, 67, 33), (66, 67, 29), 50, 0)
    slime(w, 66, 67, 27, 66, 67, 27)                                  # the step, walk 68
    carve(66, 68, 27, 66, 69, 27)

    # ---- the hall (walk 70) and its upper garage (walk 82) ----
    # Five helicopters on the hall's floor; two columns standing, two hanging from the vault,
    # a treasure on each standing one and under each hanging one. A hole in the vault framed in
    # girders up to the upper garage: four more helicopters.
    carve(56, 70, 18, 68, 80, 27)
    for x in (58, 60, 62, 64, 66):
        w.place(O.HELICOPTER, x, 70, 19)
    slime(w, 59, 70, 23, 59, 75, 23)
    w.place(O.TREASURE, 59, 76, 23)
    slime(w, 64, 70, 25, 64, 77, 25)
    w.place(O.TREASURE, 64, 78, 25)
    slime(w, 61, 78, 25, 61, 80, 25)
    w.place(O.TREASURE, 61, 77, 25)
    slime(w, 66, 77, 21, 66, 80, 21)
    w.place(O.TREASURE, 66, 76, 21)
    carve(56, 82, 18, 68, 85, 27)
    carve(62, 81, 22, 63, 81, 23)
    for x, z in ((61, 21), (61, 22), (61, 23), (61, 24), (64, 21), (64, 22), (64, 23), (64, 24),
                 (62, 21), (63, 21), (62, 24), (63, 24)):
        w.set(x, 81, z, GIRDER)
    for x in (57, 59, 65, 67):
        w.place(O.HELICOPTER, x, 82, 25)

    # ---- the top (walk 88) ----
    # A hole up from the garage. In the cave: a corridor one wide in its south-west corner, the
    # switch in its floor at the mouth, two saws in its floor, two mines hanging over them,
    # treasure 10 at its end; a bend of water under the north wall to a niche with treasure 9 (no helicopter in
    # water); an egg on a ledge; the arrow's box, walls of blue blocks, a grate roof with a gap
    # to drop through, two eggs and machine parts inside.
    carve(57, 86, 19, 58, 87, 20)
    carve(54, 88, 16, 68, 94, 28)
    slime(w, 54, 88, 25, 61, 90, 27)
    carve(55, 88, 26, 61, 89, 26)                                     # the switch's corridor
    w.set(61, 87, 26, SWITCH)
    for x in (58, 59):
        w.set(x, 87, 26, 10 if solved else SAW)
        w.rotate_plate(x, 87, 26)
        if not solved:
            mine(w, x, 89, 26, hanging=True)
    w.place(O.TREASURE, 55, 88, 26)
    for z in (16, 13):                                                # the bend: down, along,
        w.fill(56, 85, z, 56, 86, z, DEEP)                            # up into the niche
        w.set(56, 87, z, WATER)
    w.fill(56, 84, 13, 56, 84, 16, DEEP)
    carve(56, 88, 12, 57, 89, 13)
    w.place(O.TREASURE, 57, 88, 12)
    if solved:
        carve(57, 88, 14, 57, 89, 15)                                 # a doorway for the checks
    slime(w, 54, 88, 19, 55, 89, 21)
    w.place(O.EGG, 55, 90, 20)
    w.fill(61, 88, 21, 65, 90, 25, FRAME)
    carve(62, 88, 22, 64, 90, 24)
    w.fill(62, 91, 22, 64, 91, 24, GRATE_ROOF)
    w.set(63, 91, 23, 0)
    w.place(O.EXIT, 63, 88, 23)
    w.place(O.EGG, 62, 88, 22)
    w.place(O.EGG, 64, 88, 24)
    for (x, z), icon in zip(((64, 22), (62, 24)), PROPS):
        w.set(x, 88, z, icon)

    skin(w, rng)
    w.spawn = (50, 5, 54)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
