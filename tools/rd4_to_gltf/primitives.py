"""Canonical Ray Dream primitives, sized as recovered from blupi.dxf (README.md)."""

from __future__ import annotations

import numpy as np

from geometry import Mesh

SPHERE_RADIUS = 3.0
CYLINDER_RADIUS = 3.0
CYLINDER_HALF_HEIGHT = 2.5   # along local Z
CUBE_HALF_SIZE = 2.5         # fitted to the helicopter sprites, see README.md
CONE_RADIUS = 3.0            # base at local Z -2.5, apex at +2.5: the cylinder's box (README.md)
CONE_HALF_HEIGHT = 2.5


def sphere(segments: int = 32, rings: int = 16) -> Mesh:
    pos = [[0.0, 0.0, SPHERE_RADIUS]]
    for i in range(1, rings):
        th = np.pi * i / rings
        for j in range(segments):
            ph = 2 * np.pi * j / segments
            pos.append([SPHERE_RADIUS * np.sin(th) * np.cos(ph), SPHERE_RADIUS * np.sin(th) * np.sin(ph),
                        SPHERE_RADIUS * np.cos(th)])
    pos.append([0.0, 0.0, -SPHERE_RADIUS])
    tris = []
    for j in range(segments):
        tris.append((0, 1 + j, 1 + (j + 1) % segments))
    for i in range(rings - 2):
        a, b = 1 + i * segments, 1 + (i + 1) * segments
        for j in range(segments):
            j1 = (j + 1) % segments
            tris.append((a + j, b + j, a + j1))
            tris.append((a + j1, b + j, b + j1))
    last = len(pos) - 1
    base = 1 + (rings - 2) * segments
    for j in range(segments):
        tris.append((base + j, last, base + (j + 1) % segments))
    return Mesh(np.array(pos), np.array(tris))


def cylinder(segments: int = 32) -> Mesh:
    ang = 2 * np.pi * np.arange(segments) / segments
    ring = np.stack([CYLINDER_RADIUS * np.cos(ang), CYLINDER_RADIUS * np.sin(ang)], axis=1)
    top = np.c_[ring, np.full(segments, CYLINDER_HALF_HEIGHT)]
    bot = np.c_[ring, np.full(segments, -CYLINDER_HALF_HEIGHT)]
    pos = np.concatenate([top, bot, [[0, 0, CYLINDER_HALF_HEIGHT], [0, 0, -CYLINDER_HALF_HEIGHT]]])
    ct, cb = 2 * segments, 2 * segments + 1
    tris = []
    for j in range(segments):
        j1 = (j + 1) % segments
        tris.append((j, segments + j, j1))
        tris.append((j1, segments + j, segments + j1))
        tris.append((ct, j, j1))
        tris.append((cb, segments + j1, segments + j))
    return Mesh(pos, np.array(tris))


def cone(segments: int = 32) -> Mesh:
    ang = 2 * np.pi * np.arange(segments) / segments
    ring = np.c_[CONE_RADIUS * np.cos(ang), CONE_RADIUS * np.sin(ang), np.full(segments, -CONE_HALF_HEIGHT)]
    pos = np.concatenate([ring, [[0, 0, CONE_HALF_HEIGHT], [0, 0, -CONE_HALF_HEIGHT]]])
    apex, base = segments, segments + 1
    tris = []
    for j in range(segments):
        j1 = (j + 1) % segments
        tris.append((apex, j, j1))
        tris.append((base, j1, j))
    return Mesh(pos, np.array(tris))


def cube(half: float | None = None) -> Mesh:
    h = CUBE_HALF_SIZE if half is None else half
    pos = np.array([[x, y, z] for x in (-h, h) for y in (-h, h) for z in (-h, h)], dtype=float)
    quads = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    tris = []
    for a, b, c, d in quads:
        tris += [(a, b, c), (a, c, d)]
    return Mesh(pos, np.array(tris))
