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
    """Latitude rings between the poles; each pole is a vertex per segment (at the segment's
    middle angle) so a texture's longitude is not fanned from one vertex (cylinder())."""
    ang = 2 * np.pi * np.arange(segments) / segments
    mid = 2 * np.pi * (np.arange(segments) + 0.5) / segments
    pos = list(np.c_[np.zeros(segments), np.zeros(segments), np.full(segments, SPHERE_RADIUS)])
    for i in range(1, rings):
        th = np.pi * i / rings
        pos.extend(np.c_[SPHERE_RADIUS * np.sin(th) * np.cos(ang), SPHERE_RADIUS * np.sin(th) * np.sin(ang),
                         np.full(segments, SPHERE_RADIUS * np.cos(th))])
    bottom = len(pos)
    pos.extend(np.c_[np.zeros(segments), np.zeros(segments), np.full(segments, -SPHERE_RADIUS)])
    tris = []
    for j in range(segments):
        tris.append((j, segments + j, segments + (j + 1) % segments))
    for i in range(rings - 2):
        a, b = segments + i * segments, segments + (i + 1) * segments
        for j in range(segments):
            j1 = (j + 1) % segments
            tris.append((a + j, b + j, a + j1))
            tris.append((a + j1, b + j, b + j1))
    base = segments + (rings - 2) * segments
    for j in range(segments):
        tris.append((base + j, bottom + j, base + (j + 1) % segments))
    pos = np.array(pos)
    # The pole vertices sit at their segment's middle angle (an infinitesimal step off the axis
    # keeps their longitude).
    eps = 1e-6
    pos[:segments, 0], pos[:segments, 1] = eps * np.cos(mid), eps * np.sin(mid)
    pos[bottom:, 0], pos[bottom:, 1] = eps * np.cos(mid), eps * np.sin(mid)
    return Mesh(pos, np.array(tris))


def cylinder(segments: int = 32, cap_rings: int = 6) -> Mesh:
    """Side as one band; each cap as ``cap_rings`` concentric rings with its own centre vertex
    per segment, so a texture's angle (``shading.primitive_texture``) is interpolated across
    the cap instead of fanning from one centre vertex at angle 0 (the pickup lollipop's
    pinwheel came out as two smeared arms)."""
    ang = 2 * np.pi * np.arange(segments) / segments
    mid = 2 * np.pi * (np.arange(segments) + 0.5) / segments
    pos, tris = [], []

    def ring(radius, z, angles):
        start = len(pos)
        pos.extend(np.c_[radius * np.cos(angles), radius * np.sin(angles), np.full(len(angles), z)])
        return start

    top = ring(CYLINDER_RADIUS, CYLINDER_HALF_HEIGHT, ang)
    bot = ring(CYLINDER_RADIUS, -CYLINDER_HALF_HEIGHT, ang)
    for j in range(segments):
        j1 = (j + 1) % segments
        tris.append((top + j, bot + j, top + j1))
        tris.append((top + j1, bot + j, bot + j1))
    for z, outward in ((CYLINDER_HALF_HEIGHT, True), (-CYLINDER_HALF_HEIGHT, False)):
        rings = [ring(CYLINDER_RADIUS * k / cap_rings, z, ang) for k in range(cap_rings, 0, -1)]
        centre = ring(0.0, z, mid)
        for outer, inner in zip(rings[:-1], rings[1:]):
            for j in range(segments):
                j1 = (j + 1) % segments
                quad = [(outer + j, outer + j1, inner + j1), (outer + j, inner + j1, inner + j)]
                tris.extend(quad if outward else [q[::-1] for q in quad])
        inner = rings[-1]
        for j in range(segments):
            j1 = (j + 1) % segments
            tri = (centre + j, inner + j, inner + j1)
            tris.append(tri if outward else tri[::-1])
    return Mesh(np.array(pos), np.array(tris))


def cone(segments: int = 32, cap_rings: int = 6) -> Mesh:
    """Side from the base ring to an apex vertex per segment, base as concentric rings with a
    centre vertex per segment (cylinder()), so texture angles are not fanned."""
    ang = 2 * np.pi * np.arange(segments) / segments
    mid = 2 * np.pi * (np.arange(segments) + 0.5) / segments
    eps = 1e-6
    pos, tris = [], []

    def ring(radius, z, angles):
        start = len(pos)
        pos.extend(np.c_[radius * np.cos(angles), radius * np.sin(angles), np.full(len(angles), z)])
        return start

    base = ring(CONE_RADIUS, -CONE_HALF_HEIGHT, ang)
    apex = ring(eps, CONE_HALF_HEIGHT, mid)
    for j in range(segments):
        tris.append((apex + j, base + j, base + (j + 1) % segments))
    rings = [base] + [ring(CONE_RADIUS * k / cap_rings, -CONE_HALF_HEIGHT, ang) for k in range(cap_rings - 1, 0, -1)]
    centre = ring(eps, -CONE_HALF_HEIGHT, mid)
    for outer, inner in zip(rings[:-1], rings[1:]):
        for j in range(segments):
            j1 = (j + 1) % segments
            tris.extend([(outer + j1, outer + j, inner + j1), (inner + j1, outer + j, inner + j)])
    for j in range(segments):
        tris.append((rings[-1] + (j + 1) % segments, rings[-1] + j, centre + j))
    return Mesh(np.array(pos), np.array(tris))


def cube(half: float | None = None) -> Mesh:
    h = CUBE_HALF_SIZE if half is None else half
    pos = np.array([[x, y, z] for x in (-h, h) for y in (-h, h) for z in (-h, h)], dtype=float)
    quads = [(0, 1, 3, 2), (4, 6, 7, 5), (0, 4, 5, 1), (2, 3, 7, 6), (0, 2, 6, 4), (1, 5, 7, 3)]
    tris = []
    for a, b, c, d in quads:
        tris += [(a, b, c), (a, c, d)]
    return Mesh(pos, np.array(tris))
