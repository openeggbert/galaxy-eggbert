"""Canonical Ray Dream primitives, sized as recovered from blupi.dxf (README.md)."""

from __future__ import annotations

import numpy as np

from geometry import Mesh

SPHERE_RADIUS = 3.0
CYLINDER_RADIUS = 3.0
CYLINDER_HALF_HEIGHT = 2.5   # along local Z


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
