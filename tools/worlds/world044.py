#!/usr/bin/env python3
"""Mission 044, the labyrinth of twenty-two treasures (worlds3d/world044.vwr): brief
plans/briefs/world044.md.

Mobile Eggbert's level 044, under the region-2 background of cobbles: a labyrinth of caves in
cheese, the biggest hunt of the world -- twenty-two treasures scattered through chambers at
many heights. Blupi starts in the middle beside the red arrow, four helicopters round it;
thirteen little lifts (two to four cells, 20-40 ticks) join the chambers' levels, mines hang
from the passages' ceilings and lie in the way, helicopters stand here and there, an invert
pickup swaps his left and right, an egg; the arrow counts only with all twenty-two. Here the
labyrinth is fourteen caves in a block of cheese at heights from 12 to 29, joined by
tunnels, lifts in shafts (both ways) and drops (one way), on foot or flying; four more lifts
inside chambers rise to treasure ledges.

    python3 tools/worlds/world044.py         # writes worlds3d/world044.vwr
    python3 tools/worlds/world044.py --check lifts out
                     a block under each lift's low end, as the jump up onto the lift resting
                     on the floor: levelcheck rides a lift from the cells beside its ends,
                     it does not jump onto one
    Check with `tools/flightcheck.py --mine-margin 0` (mines can be jumped, as in the game).
"""

import random
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from kit import O, World, mine  # noqa: E402
from cheese import PLANTS, cave, cheese, hang, helicopters, low_mine, shaft, tube  # noqa: E402

# The chambers: name -> (x, z, walking level, rx, rz, walls, dome).
CH = {
    "A": (50, 50, 20, 6, 6, 5, 2),      # the hub: spawn, arrow, four helicopters
    "B": (33, 46, 20, 4.5, 6, 5, 2),
    "C": (22, 31, 16, 5, 5, 5, 2),
    "D": (36, 21, 20, 5.5, 4, 5, 2),
    "E": (56, 28, 24, 6, 4.5, 5, 2),
    "F": (75, 24, 20, 5, 5, 8, 2),
    "G": (76, 45, 16, 5, 6, 8, 2),
    "H": (70, 65, 12, 5, 5, 8, 2),
    "I": (75, 80, 16, 5, 4, 5, 2),
    "J": (55, 77, 20, 6, 5, 5, 2),
    "K": (36, 74, 24, 5, 5, 8, 2),
    "L": (21, 59, 20, 5, 6, 5, 2),
    "M": (51, 46, 29, 5, 5, 4, 2),      # the top room, over the hub
    "N": (20, 80, 12, 4, 4, 5, 2),      # the deep pocket
}
SPAWN = (49, 20, 53)
R = 2.1          # tunnels: three wide at the floor, five above, four high
STAGED = False   # --check lifts


def f(name):
    return CH[name][2]


def at(name, y=None):
    x, z, fy = CH[name][:3]
    return (x, fy if y is None else y, z)


def walk(w, a, b, via=()):
    """A tunnel at one walking level from chamber a to chamber b (through `via` points)."""
    y = f(a)
    pts = [at(a)] + [(x, y, z) for x, z in via] + [at(b, y)]
    for p, q in zip(pts, pts[1:]):
        tube(w, p, q, R, floor=y)


def lifted(w, a, b, p, side, ticks, via=()):
    """A lift between a (lower) and b (higher) in a shaft at p, both ways: the lower tunnel
    from a ends in a straight bay three wide that stops at p (nothing open beyond the lift at
    a's level, or Blupi walks off it there), the upper tunnel leaves p toward `side` (dx, dz)
    on to b. The lift cube rests on a's floor (a jump up onto it: a lift in a pit flush
    with the floor traps Blupi under it when he steps in while it is up) and rises until
    flush with b's floor (the original's ~10 ticks a cell); with the lift up, the shaft is a
    drop down to a."""
    (px, pz), (sx, sz) = p, side
    pts = [at(a)] + [(x, f(a), z) for x, z in via] + [(px - 4 * sx, f(a), pz - 4 * sz)]
    for u, v in zip(pts, pts[1:]):
        tube(w, u, v, R, floor=f(a))
    for k in range(0, 4):                                  # the bay, p - 3 side .. p
        cx, cz = px - k * sx, pz - k * sz
        w.fill(cx - abs(sz), f(a), cz - abs(sx), cx + abs(sz), f(a) + 3, cz + abs(sx), 0)
    tube(w, (px + sx, f(b), pz + sz), at(b), R, floor=f(b))
    shaft(w, px, pz, f(a), f(b) + 3, 0.5)
    if STAGED:
        w.set(px, f(a), pz, 248)
    # It waits two seconds at the bottom, half a second at the top.
    w.place(O.LIFT, px, f(a), pz, end=(px, f(b) - 1, pz), advance=ticks, recede=ticks,
            stop_start=40, stop_end=10)


def drop(w, a, b, p):
    """From a (higher) a tunnel to the point p, a hole down to b's level there, on to b: one
    way (back up another way, or flying)."""
    tube(w, at(a), (p[0], f(a), p[1]), R, floor=f(a))
    shaft(w, p[0], p[1], f(b), f(a), 1.6)
    tube(w, (p[0], f(b), p[1]), at(b), R, floor=f(b))


def ledge(w, rng, name, box, height, lift_at, ticks):
    """A ledge `height` above chamber name's floor (cells x0, z0, x1, z1) and a lift beside it
    at lift_at (x, z) resting on the floor (a jump up onto it) and rising until flush with the
    ledge's top."""
    y = f(name)
    x0, z0, x1, z1 = box
    cheese(w, x0, y, z0, x1, y + height - 1, z1, rng)
    lx, lz = lift_at
    if STAGED:
        w.set(lx, y, lz, 248)
    w.place(O.LIFT, lx, y, lz, end=(lx, y + height - 1, lz), advance=ticks, recede=ticks,
            stop_start=40, stop_end=10)
    return y + height


def main(argv) -> int:
    global STAGED
    STAGED = len(argv) > 3 and argv[1] == "--check" and argv[2] == "lifts"
    rng = random.Random(44)
    w = World(mission=44, sky=2)
    cheese(w, 12, 6, 12, 88, 37, 89, rng)
    for x, z, y, rx, rz, walls, dome in CH.values():
        cave(w, x, y, z, rx, walls, dome, rng, rz=rz)

    # The links: the hub's four ways (west, north by lift, east down, south), and the rings
    # they lead into.
    walk(w, "A", "B")
    lifted(w, "C", "B", (27, 41), (1, 0), ticks=40)
    lifted(w, "C", "D", (28, 24), (1, 0), ticks=40)
    lifted(w, "D", "E", (45, 22), (1, 0), ticks=40)
    lifted(w, "A", "E", (58, 39), (0, -1), ticks=40)
    lifted(w, "E", "M", (48, 37), (0, 1), ticks=50, via=((50, 33),))
    drop(w, "E", "F", (67, 26))
    lifted(w, "G", "F", (75, 34), (0, -1), ticks=40)
    drop(w, "A", "G", (65, 49))
    drop(w, "G", "H", (73, 56))
    lifted(w, "H", "I", (72, 73), (0, 1), ticks=40)
    lifted(w, "I", "J", (65, 80), (-1, 0), ticks=40)
    walk(w, "A", "J", via=((52, 62),))
    lifted(w, "J", "K", (45, 75), (-1, 0), ticks=40)
    drop(w, "K", "L", (27, 67))
    walk(w, "L", "B", via=((25, 50),))
    drop(w, "L", "N", (20, 71))

    # Ledges with their own lifts (four of the thirteen).
    top_g = ledge(w, rng, "G", (78, 48, 80, 50), 4, (77, 49), ticks=40)
    top_h = ledge(w, rng, "H", (72, 67, 74, 69), 4, (71, 68), ticks=40)
    top_f = ledge(w, rng, "F", (77, 26, 79, 28), 3, (76, 27), ticks=30)
    top_k = ledge(w, rng, "K", (33, 76, 35, 78), 4, (36, 77), ticks=40)

    # Twenty-two treasures.
    for x, y, z in ((32, 20, 50),                                   # B
                    (19, 16, 29), (25, 16, 35),                     # C
                    (33, 20, 19), (40, 20, 23),                     # D
                    (52, 24, 26), (61, 24, 30),                     # E
                    (78, top_f, 27),                                # F, its ledge
                    (74, 16, 41), (79, top_g, 49),                  # G
                    (67, 12, 63), (73, top_h, 68),                  # H
                    (72, 16, 82), (79, 16, 79),                     # I
                    (51, 20, 79), (59, 20, 75),                     # J
                    (39, 24, 71), (34, top_k, 77),                  # K
                    (18, 20, 63),                                   # L
                    (48, 29, 44), (54, 29, 49),                     # M
                    (19, 12, 82)):                                  # N
        w.place(O.TREASURE, x, y, z)
    w.place(O.EGG, 38, 20, 19)                                      # D
    w.place(O.EXIT, 50, 20, 50)
    helicopters(w, [(47, 20, 47), (53, 20, 47), (47, 20, 53), (53, 20, 53),      # round the arrow
                    (24, 16, 33), (58, 24, 26), (77, 16, 43), (68, 12, 66), (57, 20, 78),
                    (37, 24, 72), (22, 20, 57), (21, 12, 78), (53, 29, 46), (73, 20, 22)])
    w.place(O.INVERT, 52, 20, 76)                                   # J, on the way to K's lift
    # Mines: two rows of three lying across tunnels, hanging ones low in tunnels (crouch) and
    # under the chambers' domes and ledges.
    for x in (51, 52, 53):
        mine(w, x, 20, 66)
    for z in (52, 53, 54):
        mine(w, 23, 20, z)
    for x, y, z in ((41, 20, 48), (62, 20, 49), (24, 20, 55)):          # low in tunnels
        low_mine(w, x, y, z, rng)
    for x, y, z in ((22, 18, 31), (36, 22, 21), (55, 27, 29), (61, 27, 27),
                    (76, 21, 45), (70, 17, 64), (75, 18, 80), (55, 22, 77), (56, 23, 73),
                    (36, 27, 74), (51, 31, 47)):
        hang(w, x, y, z)
    for icon, x, z, name in ((PLANTS[0], 46, 54, "A"), (PLANTS[2], 54, 54, "A"), (PLANTS[1], 17, 59, "L")):
        w.bigdecor(icon, x, f(name), z)
    w.spawn = SPAWN
    out = Path(argv[3]) if STAGED else Path(__file__).resolve().parents[2] / "worlds3d" / "world044.vwr"
    w.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
