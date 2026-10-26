#!/usr/bin/env python3
"""World 199, the grand tour (worlds3d/world199.vwr): brief plans/briefs/world199.md.

Mobile Eggbert's bonus world 199, the finale reached through the main hub's twelve gold bars, is a
jungle of slime (341-363) holding a piece of every world before it: blue machine panel and grey
plates, white palace stone with its saws, brick with its bookcases, a village street, cheese,
toy balls and star blocks, brown rock; no treasure, thirty-eight eggs; red, green and blue doors
(334-336) and their keys (49-51), duplicates of each lying in other parts; twenty-two helicopters,
twenty-six lifts, thirty-six moving mines (16), follow stars (96), birds (20), bulldozers (4), big
creatures (54), enemy Blupis (33), the lollipop (26), inverts (40); at the end the red arrow in a
frame of star blocks, then the win screen. Here the jungle is a great plaza cut in slime, a ring
gallery round it at the start's height and eight enclaves off it, four at the plaza's floor and
four at the ring's, each built of one world's material and holding a short lesson of that world.

    python3 tools/worlds/world199.py                 # writes worlds3d/world199.vwr
    python3 tools/worlds/world199.py --stage K out   # for levelcheck: K >= 1 the doors open, no
                                                     # mines, creatures or saws, the lifts' and the
                                                     # lollipop's climbs as stairs
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from cheese import cheese  # noqa: E402
from kit import MACHINE_FACES, MACHINE_FLOOR, O, World, lift, mine, wall  # noqa: E402
from palace import stone as white_stone  # noqa: E402
from slime import SLIME, skin, slime  # noqa: E402
from station import brick, water  # noqa: E402
from village import ROCK, ROCK_FACES, STONE, STONE_FACES  # noqa: E402

RED_DOOR, GREEN_DOOR, BLUE_DOOR = 334, 335, 336
LAVA = 68
BALLS = (215, 216, 217, 218)
STAR = 231
BOOKCASES = (139, 140, 141, 142, 143)
WINDOWS = (186, 187, 188, 189)
SLATE = (196, 197)
SAW, SWITCH, CRUSHER = 378, 384, 317
LOW, HIGH = 20, 30                    # the plaza's floor and the ring gallery: walking levels
P0, P1 = 34, 66                       # the plaza's interior, x and z


class Room:
    """An enclave off the plaza: `side` (E, N, W or S), centred `c` along the plaza's wall, at walk
    `walk`. Local coordinates: u across (0-12), v in from its doorway (0-12)."""

    def __init__(self, side, c, walk, height=6):
        self.side, self.c, self.walk, self.height = side, c, walk, height

    def at(self, u, v):
        c, s = self.c, self.side
        if s == "E":
            return 70 + v, c - 6 + u
        if s == "W":
            return 30 - v, c - 6 + u
        if s == "N":
            return c - 6 + u, 30 - v
        return c - 6 + u, 70 + v

    def box(self, u0, v0, u1, v1):
        (xa, za), (xb, zb) = self.at(u0, v0), self.at(u1, v1)
        return min(xa, xb), min(za, zb), max(xa, xb), max(za, zb)


def main(argv) -> int:
    stage = int(argv[2]) if len(argv) > 3 and argv[1] == "--stage" else None
    out = Path(argv[3]) if stage is not None else Path(__file__).resolve().parents[2] / "worlds3d" / "world199.vwr"
    rng = random.Random(199)
    w = World(mission=199, sky=31)
    calm = stage is not None and stage >= 1

    def carve(x0, y0, z0, x1, y1, z1):
        w.fill(x0, y0, z0, x1, y1, z1, 0)

    def hazard(x, y, z, hanging=False):
        if not calm:
            mine(w, x, y, z, hanging=hanging)

    def mover(otype, start, end, ticks, stop=8):
        if not calm:
            w.place(otype, *start, end=end, advance=ticks, recede=ticks, stop_start=stop, stop_end=stop)

    def bob(x, y, z, height=2, ticks=20):
        mover(O.MOVING_MINE, (x, y, z), (x, y + height, z), ticks, stop=0)

    def material_box(kind, x0, y0, z0, x1, y1, z1):
        if kind == "machine":
            wall(w, x0, y0, z0, x1, y1, z1, rng, faces=MACHINE_FACES, face_chance=0.1, block=MACHINE_FLOOR)
        elif kind == "brick":
            brick(w, x0, y0, z0, x1, y1, z1, rng)
        elif kind == "village":
            wall(w, x0, y0, z0, x1, y1, z1, rng, faces=STONE_FACES, face_chance=0.3, block=STONE)
        elif kind == "cheese":
            cheese(w, x0, y0, z0, x1, y1, z1, rng)
        elif kind == "toys":
            for x in range(x0, x1 + 1):
                for y in range(y0, y1 + 1):
                    for z in range(z0, z1 + 1):
                        w.set(x, y, z, STAR if (x + y + z) % 3 == 0 else rng.choice(BALLS))
        elif kind == "palace":
            white_stone(w, x0, y0, z0, x1, y1, z1, rng)
        elif kind == "rock":
            for x in range(x0, x1 + 1):
                for y in range(y0, y1 + 1):
                    for z in range(z0, z1 + 1):
                        w.set(x, y, z, ROCK_FACES[0] if rng.random() < 0.2 else ROCK)
        elif kind == "stars":
            w.fill(x0, y0, z0, x1, y1, z1, STAR)

    def enclave(room, kind, door=None):
        """Builds the room's shell of `kind`, carves it and its passage from the plaza, and sets
        its door (`door` icon) in the passage."""
        x0, z0, x1, z1 = room.box(-1, -1, 13, 13)
        material_box(kind, x0, room.walk - 1, z0, x1, room.walk + room.height + 1, z1)
        x0, z0, x1, z1 = room.box(0, 0, 12, 12)
        carve(x0, room.walk, z0, x1, room.walk + room.height, z1)
        px0, pz0, px1, pz1 = room.box(5, -3, 7, -1)
        carve(px0, room.walk, pz0, px1, room.walk + 2, pz1)
        if door is not None and not calm:
            dx0, dz0, dx1, dz1 = room.box(5, -2, 7, -2)
            w.fill(dx0, room.walk, dz0, dx1, room.walk + 1, dz1, door)

    def put(room, otype, u, v, dy=0, **kw):
        x, z = room.at(u, v)
        w.place(otype, x, room.walk + dy, z, **kw)

    # ---- the jungle: the plaza, the ring gallery, the stair ----
    slime(w, 2, 2, 2, 97, 44, 97)
    carve(P0, LOW, P0, P1, HIGH + 7, P1)
    for x0, z0, x1, z1 in ((P0, P0, P1, P0 + 2), (P0, P1 - 2, P1, P1), (P0, P0, P0 + 2, P1), (P1 - 2, P0, P1, P1)):
        slime(w, x0, HIGH - 2, z0, x1, HIGH - 1, z1)                       # the ring, walk 30
    w.spawn = (65, HIGH, 35)
    for k, (xa, xb) in enumerate(((61, 63), (58, 60), (55, 57), (52, 54))):  # the stair, walk 28 ... 22
        slime(w, xa, LOW, 37, xb, HIGH - 3 - 2 * k, 39)
    # Two lifts (2 x 2, 60 ticks, dwelling 40) from the floor up to the ring's west and south arms.
    for xs, zs in (((37, 38), (49, 50)), ((49, 50), (61, 62))):
        for x in xs:
            for z in zs:
                if calm:
                    w.set(x, HIGH - 1, z, SLIME)
                else:
                    lift(w, (x, LOW - 1, z), (x, HIGH - 1, z), ticks=60, stop=40)
        carve(xs[0], LOW - 1, zs[0], xs[1], LOW - 1, zs[1])
    if calm:                                                              # levelcheck's stand-in stairs
        for k in range(4):
            slime(w, 38 + k, LOW, 51, 38 + k, LOW + 1 + 2 * k, 51)
            slime(w, 51, LOW, 60 - k, 51, LOW + 1 + 2 * k, 60 - k)
    # The pool with a fish, the plaza's creatures, its helicopters and eggs.
    carve(41, LOW - 5, 53, 49, LOW - 1, 60)
    water(w, 41, LOW - 5, 53, 49, LOW - 1, 60)
    mover(O.FISH, (42, LOW - 3, 56), (48, LOW - 3, 56), 45, stop=47)
    for x, z in ((40, 42), (60, 46), (46, 62)):
        w.place(O.HELICOPTER, x, LOW, z)
    w.place(O.HELICOPTER, 36, HIGH, 60)
    mover(O.BULLDOZER, (40, LOW, 48), (52, LOW, 48), 90, stop=20)
    mover(O.BULLDOZER, (54, LOW, 58), (54, LOW, 46), 90, stop=20)
    for a, b in (((40, HIGH + 3, 40), (60, HIGH + 3, 40)), ((60, HIGH + 4, 60), (40, HIGH + 4, 60)),
                 ((50, HIGH + 2, 42), (50, HIGH + 2, 58))):
        mover(O.BIRD, a, b, 60)
    for x, z in ((44, 44), (56, 52), (48, 40), (58, 58)):
        bob(x, LOW, z, 3, 24)
    for x, z in ((39, 64), (61, 42), (45, 50), (55, 64)):
        if not calm:
            w.place(O.FOLLOW_STAR, x, LOW, z)
    for x, y, z in ((36, LOW, 36), (64, LOW, 64), (36, LOW, 64), (50, LOW, 44), (35, HIGH, 35), (65, HIGH, 65),
                    (35, HIGH, 50), (50, HIGH, 65)):
        w.place(O.EGG, x, y, z)

    # ---- the four enclaves at the floor ----
    # East, the machine (open): a pit of lava crossed on a lift, two mines bobbing over it, the red
    # key beyond, two helicopters.
    machine = Room("E", 43, LOW)
    enclave(machine, "machine")
    x0, z0, x1, z1 = machine.box(0, 4, 12, 8)
    carve(x0, LOW - 4, z0, x1, LOW - 1, z1)
    w.fill(x0, LOW - 4, z0, x1, LOW - 3, z1, LAVA)
    for u in (5, 6, 7):
        for v in (4, 5):
            x, z = machine.at(u, v)
            tx, tz = machine.at(u, v + 3)
            if calm:
                w.fill(min(x, tx), LOW - 1, min(z, tz), max(x, tx), LOW - 1, max(z, tz), MACHINE_FLOOR)
            else:
                lift(w, (x, LOW - 1, z), (tx, LOW - 1, tz), ticks=40, stop=40)
    for u in (2, 10):
        x, z = machine.at(u, 6)
        bob(x, LOW - 1, z, 3, 18)
    put(machine, O.KEY1, 6, 11)
    put(machine, O.HELICOPTER, 1, 1)
    put(machine, O.HELICOPTER, 11, 1)
    put(machine, O.EGG, 11, 11)
    put(machine, O.EGG, 1, 11)

    # North, the brick library (red door): a stair of bookcases, the green key on its top row.
    library = Room("N", 43, LOW, height=9)
    enclave(library, "brick", RED_DOOR)
    for k, (va, vb) in enumerate(((3, 4), (5, 6), (7, 8))):
        x0, z0, x1, z1 = library.box(0, va, 12, vb)
        for x in range(x0, x1 + 1):
            for z in range(z0, z1 + 1):
                for y in range(LOW, LOW + 2 * (k + 1)):
                    w.set(x, y, z, rng.choice(BOOKCASES))
    put(library, O.KEY2, 6, 8, dy=6)
    put(library, O.CRATE, 2, 1)
    put(library, O.EGG, 10, 4, dy=2)
    put(library, O.EGG, 3, 8, dy=6)
    put(library, O.EGG, 11, 8, dy=6)

    # West, the village street (green door): a row of houses along its side, an enemy Blupi walking
    # the street firing along it, two mines, the blue key at the street's end.
    village = Room("W", 57, LOW)
    enclave(village, "village", GREEN_DOOR)
    for v in range(1, 12):
        for u in range(0, 3):
            x, z = village.at(u, v)
            for y in range(LOW, LOW + 3):
                w.set(x, y, z, rng.choice(WINDOWS) if u == 2 else 190)
            w.set(x, LOW + 3, z, rng.choice(SLATE))
    xa, za = village.at(7, 2)
    xb, zb = village.at(7, 10)
    mover(O.CREATURE, (xa, LOW, za), (xb, LOW, zb), 90, stop=20)
    for u, v in ((5, 5), (9, 8)):
        x, z = village.at(u, v)
        hazard(x, LOW, z)
    put(village, O.KEY3, 7, 12)
    put(village, O.EGG, 11, 1)
    put(village, O.EGG, 11, 11)

    # South, the cheese (green door): a row of crushers across it (cross in their open phase), two
    # mines bobbing, the blue key and a second green key at its far end.
    cheese_room = Room("S", 57, LOW)
    enclave(cheese_room, "cheese", GREEN_DOOR)
    for u in range(0, 13):
        x, z = cheese_room.at(u, 6)
        if not calm:
            w.set(x, LOW + 2, z, CRUSHER)
    for u in (3, 9):
        x, z = cheese_room.at(u, 9)
        bob(x, LOW, z, 2, 16)
    put(cheese_room, O.KEY3, 9, 11)
    put(cheese_room, O.KEY2, 3, 11)
    put(cheese_room, O.EGG, 6, 12)
    put(cheese_room, O.EGG, 1, 2)

    # ---- the four enclaves at the ring ----
    # East, the toys (red door): sleeping follow stars along the way in, a tower of balls the
    # lollipop's power jump reaches, a second red key on its top.
    toys = Room("E", 57, HIGH, height=9)
    enclave(toys, "toys", RED_DOOR)
    x0, z0, x1, z1 = toys.box(4, 8, 8, 11)
    material_box("toys", x0, HIGH, z0, x1, HIGH + 3, z1)                  # the tower, walk 34
    if calm:                                                              # levelcheck's stand-in step
        sx0, sz0, sx1, sz1 = toys.box(5, 7, 7, 7)
        material_box("toys", sx0, HIGH, sz0, sx1, HIGH + 1, sz1)
    put(toys, O.LOLLIPOP, 6, 2)
    put(toys, O.KEY1, 6, 10, dy=4)
    for u, v in ((2, 4), (10, 4), (2, 6), (10, 6), (6, 5), (1, 9)):
        if not calm:
            put(toys, O.FOLLOW_STAR, u, v)
    put(toys, O.EGG, 11, 11)
    put(toys, O.EGG, 4, 9, dy=4)
    put(toys, O.EGG, 1, 12)

    # North, the palace (blue door): two rows of saws in its white floor, each stopped by a switch
    # at its west end; birds over them; a third green key; eggs.
    palace = Room("N", 57, HIGH)
    enclave(palace, "palace", BLUE_DOOR)
    for v in (4, 8):
        for u in range(0, 13):
            x, z = palace.at(u, v)
            w.set(x, HIGH - 1, z, SWITCH if u == 0 else (396 if calm else SAW))
    for a, b in (((0, 6), (12, 6)), ((12, 10), (0, 10))):
        (xa, za), (xb, zb) = palace.at(*a), palace.at(*b)
        mover(O.BIRD, (xa, HIGH + 1, za), (xb, HIGH + 1, zb), 48)
    put(palace, O.KEY2, 6, 11)
    put(palace, O.EGG, 1, 1)
    put(palace, O.EGG, 11, 1)
    put(palace, O.EGG, 11, 11)

    # West, the brown rock (green door): big creatures walking it, two pools of lava, the invert, a
    # third blue key.
    rock_room = Room("W", 43, HIGH)
    enclave(rock_room, "rock", GREEN_DOOR)
    for u0, v0, u1, v1 in ((1, 4, 4, 7), (8, 6, 11, 9)):
        x0, z0, x1, z1 = rock_room.box(u0, v0, u1, v1)
        w.fill(x0, HIGH - 1, z0, x1, HIGH - 1, z1, LAVA)
    for a, b in (((0, 2), (12, 2)), ((12, 10), (0, 10)), ((6, 1), (6, 12))):
        (xa, za), (xb, zb) = rock_room.at(*a), rock_room.at(*b)
        mover(O.BIG_CREATURE, (xa, HIGH, za), (xb, HIGH, zb), 120, stop=30)
    put(rock_room, O.INVERT, 12, 12)
    put(rock_room, O.KEY3, 6, 12)
    put(rock_room, O.EGG, 0, 12)
    put(rock_room, O.EGG, 12, 0)

    # South, the finale (blue door): a hall of star blocks, an inner blue door across it, the red
    # arrow beyond in a frame of star blocks; eggs.
    finale = Room("S", 43, HIGH)
    enclave(finale, "stars", BLUE_DOOR)
    x0, z0, x1, z1 = finale.box(0, 6, 12, 6)
    w.fill(x0, HIGH, z0, x1, HIGH + finale.height, z1, STAR)
    dx0, dz0, dx1, dz1 = finale.box(5, 6, 7, 6)
    carve(dx0, HIGH, dz0, dx1, HIGH + 1, dz1)
    if not calm:
        w.fill(dx0, HIGH, dz0, dx1, HIGH + 1, dz1, BLUE_DOOR)
    fx0, fz0, fx1, fz1 = finale.box(4, 10, 8, 12)
    w.fill(fx0, HIGH, fz0, fx1, HIGH + 3, fz1, STAR)                        # the frame...
    ix0, iz0, ix1, iz1 = finale.box(5, 10, 7, 12)
    carve(ix0, HIGH, iz0, ix1, HIGH + 2, iz1)                              # ...open toward the hall
    put(finale, O.EXIT, 6, 11)
    for u, v in ((1, 2), (11, 2), (1, 9), (11, 9), (6, 3)):
        put(finale, O.EGG, u, v)

    skin(w, rng)
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
