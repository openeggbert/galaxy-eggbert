#!/usr/bin/env python3
"""Mission 115, the works' last round and world 11's golden key (worlds3d/world115.vwr): brief
plans/briefs/world115.md.

Mobile Eggbert's level 115, the last of world 11, starts beside the golden key (21), which waits
until the eleven treasures are found: about the blue machine panel eighteen helicopters (13) to
fly up to treasures on high ledges, eleven skateboards (24) to ride over rows of mines (2), a row
of crates (12) to push into steps, columns of lightning (305), pits of lava, a fish (17) in a pool,
five eggs. Here the round is a hall with four wings in 3D: north the helicopters' gallery and its
ledges, east the skate run over two blocks of mines, south the crates' wall, west the lightning,
the pool and a second skate run; the golden key in its alcove off the hall.

    python3 tools/worlds/world115.py                 # writes worlds3d/world115.vwr
    python3 tools/worlds/world115.py --stage 1 out   # for flightcheck: no mines or lightning, the
                                                     # crates pushed into their steps
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, mine, wall  # noqa: E402
from station import water  # noqa: E402

LAVA = 68
BLITZ, EMITTER = 305, 304
WALK = 20
FAN_UP, WIND_UP = 132, 118             # in a floor, blowing +Y
FAN_DOWN, WIND_DOWN = 135, 122         # in a ceiling, blowing -Y
FAN_LEFT, WIND_LEFT = 126, 110         # in a wall, blowing -X
FAN_RIGHT, WIND_RIGHT = 129, 114       # in a wall, blowing +X


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world115.vwr"
    rng = random.Random(115)
    w = World(mission=115, sky=22)
    calm = stage is not None and stage >= 1

    def panel(x0, y0, z0, x1, y1, z1):
        wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.08, block=MACHINE_FLOOR)

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def mines(cells):
        if not calm:
            for x, y, z in cells:
                mine(w, x, y, z)

    def lightning(x, z0, z1):
        for z in range(z0, z1 + 1):
            carve(x, WALK, z, x, WALK + 2, z)
            w.set(x, WALK + 3, z, EMITTER)
            if not calm:
                w.fill(x, WALK, z, x, WALK + 2, z, BLITZ)

    panel(2, 4, 2, 97, 44, 97)

    # ---- the hall (walk 20) and the golden key ----
    carve(40, WALK, 40, 60, WALK + 5, 60)
    w.spawn = (50, WALK, 52)
    carve(46, WALK, 61, 48, WALK + 2, 62)
    w.place(O.SECRET_EXIT, 47, WALK, 62)
    w.place(O.EGG, 58, WALK, 42)

    # ---- north: the helicopters' gallery (walk 20, open up to 42) ----
    # Eight helicopters on its floor, five on a ledge; an egg on a ledge of its walls at walk 26,
    # two pits of lava in its floor. (Its treasures were on ledges at walk 30, 34 and 38 until
    # 2026-11-04; they are in the winds now, as the original's: the wind wing and the stream east of
    # the gallery.)
    carve(49, WALK, 30, 51, WALK + 2, 39)
    carve(34, WALK, 6, 66, 42, 29)
    for x in range(40, 48):
        w.place(O.HELICOPTER, x, WALK, 26)
    for x0, z0, x1, z1 in ((36, 8, 42, 14), (56, 8, 62, 14)):
        carve(x0, WALK - 3, z0, x1, WALK - 1, z1)
        w.fill(x0, WALK - 3, z0, x1, WALK - 2, z1, LAVA)
    for walk, x0, z0, x1, z1, tx, tz in ((26, 34, 6, 37, 9, 35, 7),):
        panel(x0, walk - 2, z0, x1, walk - 1, z1)
        if (tx, tz) != (35, 7):
            w.place(O.TREASURE, tx, walk, tz)
    w.place(O.EGG, 35, 26, 7)
    # Treasure 1 at the end of the draught tunnel, west out of the gallery's wall five high
    # (walk 28) and three wide: across it four curtains of wind, updraughts (fans in the floor) and
    # downdraughts (fans in the ceiling) by turns, as the original's cave of drafts (its x 14-20,
    # y 23-27) flown through in a helicopter: an updraught throws him up a cell or more, into the
    # ceiling's crash band if he enters it high (OR-0019); a downdraught throws him down.
    carve(13, 28, 16, 33, 32, 18)
    carve(6, 28, 14, 12, 32, 20)
    for x, up in ((30, True), (26, False), (22, True), (18, False)):
        if up:
            w.fill(x, 27, 16, x, 27, 18, FAN_UP)
            w.fill(x, 28, 16, x, 32, 18, WIND_UP)
        else:
            w.fill(x, 33, 16, x, 33, 18, FAN_DOWN)
            w.fill(x, 28, 16, x, 32, 18, WIND_DOWN)
    w.place(O.TREASURE, 8, 28, 17)
    panel(48, 30, 6, 54, 31, 9)
    for x in range(49, 54):
        w.place(O.HELICOPTER, x, 32, 7)
    w.place(O.EGG, 51, 32, 9)

    # ---- east of the gallery: the stream, the original's row 17 (x 20-29) ----
    # A fan in the west end of a corridor blowing east along it, two treasures in the stream (24, 17)
    # and (26, 17), an updraught shaft at its east end (x 29, from its fan at the foot) to the level
    # above: flown in by helicopter from the room below. Here: a chamber open to the gallery (x 67-71,
    # z 15-19, walk 20 up to 28); up through a hole (x 69-70) into the stream (x 68-81, z 16-18, walk
    # 30, three high), its fans in its west end (x 67), treasures 2 and 3 on its floor; the updraught
    # shaft (x 82-83, its fans at y 25, under the stream's floor) up to a landing (x 80-85, walk 40)
    # and a corridor west
    # back to the gallery's top.
    carve(67, WALK, 15, 71, WALK + 8, 19)                                 # the chamber
    carve(69, WALK + 9, 16, 70, WALK + 9, 18)                             # the hole up
    carve(68, 30, 16, 83, 32, 18)                                         # the stream
    w.fill(67, 30, 16, 67, 32, 18, FAN_RIGHT)
    w.fill(68, 30, 16, 81, 32, 18, WIND_RIGHT)
    w.place(O.TREASURE, 74, 30, 17)
    w.place(O.TREASURE, 77, 30, 17)
    # The shaft's fans four under the stream's floor, as the original's column rises from its fan
    # (29, 21) four rows under the corridor: the stream drops a helicopter into the updraught, not
    # onto a fan's mouth.
    carve(82, 26, 16, 83, 39, 18)
    w.fill(82, 25, 16, 83, 25, 18, FAN_UP)
    w.fill(82, 26, 16, 83, 39, 18, WIND_UP)
    carve(80, 40, 15, 85, 42, 19)                                         # the landing, walk 40
    carve(67, 40, 16, 79, 41, 18)                                         # back west to the gallery

    # ---- south-east: the wind wing (walk 20 / 22), the original's sideways wind ----
    # Its row 11 (x 7-24): two fans back to back in a corridor one high, each blowing out along it
    # onto a mine in its stream (9, 11) and (19, 11); over the corridor single blocks with gaps
    # between, a treasure on the one over the far mine (19, 9). Fallen into a gap, the wind (9 px a
    # tick, his walk 8) carries him onto the mine; the gaps nearest the ends blow him to a wall
    # instead. Its row 11 (x 34-42) too: a stream blowing away from its fan with a treasure beside
    # the fan (39, 11): no walking up to it against the wind, only dropping onto it from above.
    # Here, east out of the hall (z 57-58): the corridor along X (z 66), its floor at walk 20; over
    # it blocks at y 21 with one-cell gaps (the original's are 2-4 wide between single blocks, met
    # from above; a jump spans ~2.3 cells, so the course is walked and jumped here): the start (x
    # 69-70), a gap blowing back to it (71), a block over the west mine (72-73), a gap blowing onto
    # that mine (74), blocks over the fans (75-80), a gap blowing onto the east mine (81), the block
    # over that mine with treasure 3 (82-83), a gap blowing to the end (84), the end (85-87). South
    # from the end to a hole over the second stream (z 74, its fan at x 87 blowing west, treasure 4
    # under the hole a cell from the fan, as the original's (39, 11) from its fan (41, 11): its blades
    # kill), which blows him west into the crates' room (x 64, z 73-75).
    carve(61, WALK, 57, 67, WALK + 2, 58)
    carve(66, WALK, 59, 70, WALK + 6, 65)
    panel(68, WALK, 63, 70, WALK + 1, 65)                                 # the step up, walk 22
    carve(71, WALK, 66, 84, WALK, 66)                                     # the corridor
    carve(69, WALK + 1, 66, 87, WALK + 6, 66)                             # over it
    for x0, x1 in ((69, 70), (79, 79), (85, 87)):                         # the ends, the fans' wall
        panel(x0, WALK, 66, x1, WALK + 1, 66)
    for x0, x1 in ((72, 73), (75, 80), (82, 83)):                         # the blocks over it
        panel(x0, WALK + 1, 66, x1, WALK + 1, 66)
    w.set(78, WALK, 66, FAN_LEFT)
    w.fill(71, WALK, 66, 77, WALK, 66, WIND_LEFT)
    w.set(80, WALK, 66, FAN_RIGHT)
    w.fill(81, WALK, 66, 84, WALK, 66, WIND_RIGHT)
    mines([(73, WALK, 66), (83, WALK, 66)])                               # in the streams
    w.place(O.TREASURE, 83, WALK + 2, 66)
    carve(85, WALK + 2, 67, 86, WALK + 4, 74)                             # south from the end
    carve(85, WALK + 1, 74, 85, WALK + 1, 74)                             # the hole
    carve(65, WALK, 74, 86, WALK, 74)                                     # the second stream
    w.set(87, WALK, 74, FAN_LEFT)
    w.fill(71, WALK, 74, 86, WALK, 74, WIND_LEFT)
    carve(65, WALK, 73, 70, WALK + 2, 75)                                 # into the crates' room
    w.place(O.TREASURE, 85, WALK, 74)

    # ---- east: the skate run (walk 20) ----
    # Five skateboards at its head; two blocks of mines two deep across it (the skateboard rides
    # through); treasures 4 and 5 at its end.
    carve(61, WALK, 49, 70, WALK + 2, 51)
    carve(71, WALK, 46, 95, WALK + 3, 54)
    for z in (47, 48, 50, 52, 53):
        w.place(O.SKATEBOARD, 72, WALK, z)
    mines([(x, WALK, z) for x in (78, 79, 86, 87) for z in (47, 49, 51, 53)])
    for x, z in ((93, 47), (93, 53)):
        w.place(O.TREASURE, x, WALK, z)
    w.place(O.EGG, 90, WALK, 46)

    # ---- south: the crates' wall (walk 20 -> 23 -> 26) ----
    # A row of six crates by a wall three high, and over it a second wall three higher: push
    # crates to the walls' feet to make steps (one, then two stacked); treasures 8 and 9 on the
    # high shelf.
    carve(49, WALK, 61, 51, WALK + 2, 69)
    carve(36, WALK, 70, 64, WALK + 9, 94)
    panel(36, WALK, 86, 64, WALK + 2, 94)                                 # the first wall, walk 23
    panel(36, WALK + 3, 91, 64, WALK + 5, 94)                             # the second, walk 26
    for k, x in enumerate(range(42, 54, 2)):
        w.place(O.CRATE, x, WALK, 78 if not calm else 85)
    if calm:
        w.place(O.CRATE, 44, WALK + 3, 90)
        w.place(O.CRATE, 44, WALK + 4, 90)
    w.place(O.TREASURE, 40, WALK + 6, 93)
    w.place(O.TREASURE, 60, WALK + 6, 93)
    w.place(O.EGG, 50, WALK + 3, 88)

    # ---- west: the lightning, the pool and the second skate run (walk 20) ----
    # A curtain of lightning across the way; a pool with a fish (swim, or edge round on its rim);
    # five skateboards; a run over a block of mines and past a second curtain to treasures 10 and 11.
    carve(30, WALK, 49, 39, WALK + 2, 51)
    lightning(35, 49, 51)
    carve(4, WALK, 40, 29, WALK + 4, 60)
    carve(18, 14, 42, 28, WALK - 1, 58)
    water(w, 18, 14, 42, 28, WALK - 1, 58)
    w.place(O.FISH, 19, 16, 50, end=(27, 16, 50), advance=45, recede=45, stop_start=47, stop_end=47)
    for z in (41, 42, 43, 44, 45):
        w.place(O.SKATEBOARD, 16, WALK, z)
    mines([(x, WALK, z) for x in (10, 11) for z in (41, 43, 45)])
    lightning(7, 40, 46)
    w.place(O.TREASURE, 5, WALK, 42)
    w.place(O.TREASURE, 5, WALK, 58)
    w.place(O.EGG, 12, WALK, 57)
    w.place(O.SKATEBOARD, 14, WALK, 58)

    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
