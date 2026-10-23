"""Tessellation of Ray Dream free-form (``ex``) masters straight from their definition.

Decoded against Ray Dream's own DXF triangulation of Blupi (see README.md):

* The sweep path runs through the ``sap`` points ``cv``; segment i is the cubic Bezier
  ``cv[i], cnc[i], cpc[i+1], cv[i+1]`` (zero-length handles make it straight).
* Four envelopes scale the cross-section: ``l``/``r`` (left/right) the cross-section's 2D ``a`` axis
  on its negative/positive side, ``t``/``b`` (top/bottom) its ``b`` axis on the positive/negative
  side. Between two ``sap`` points each envelope is a cubic Bezier in (path position, scale):
  ``(p0, v0), (p0 + nc[1], nc[0]), (p1 - pc[1], pc[0]), (p1, v1)`` where ``v``/``nc``/``pc`` are the
  ``?v``/``?nc``/``?pc`` pairs of the two points and positions are lengths along the path.
* The 2D cross-section maps to 3D as ``a`` -> +X and ``b`` -> +Z for a path along +Y (the eyelash and
  foot DXF layers fix this); in general ``a`` is X made perpendicular to the path tangent and
  ``b = a x tangent``.
* Ray Dream samples each path segment at 16 equal steps of the envelope's Bezier parameter and
  each cross-section Bezier segment at 8; ``fl 1`` closes the ends with flat caps.

Only cross-sections given at the first point (later points say ``oo 1``, "same as before") and
untwisted sweeps (``twst 0``) are supported.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

import rd4

PATH_STEPS = 16
SECTION_STEPS = 8


def _bezier(p0, p1, p2, p3, t):
    t = np.asarray(t, dtype=float)[..., None] if np.ndim(p0) else np.asarray(t, dtype=float)
    return (1 - t) ** 3 * p0 + 3 * (1 - t) ** 2 * t * p1 + 3 * (1 - t) * t ** 2 * p2 + t ** 3 * p3


@dataclass
class FreeformMesh:
    positions: np.ndarray       # (V, 3) canonical master coordinates
    triangles: np.ndarray       # (T, 3)
    uv: np.ndarray              # (V, 2) shading-domain (u, v): control-polygon length, path length
    ring_of: np.ndarray         # (V,) index of the ring along the path
    perimeter: float = 0.0      # control-polygon perimeter of the cross-section (the u period)


def _section(p2: rd4.Chunk) -> tuple[np.ndarray, np.ndarray, float]:
    """Sampled closed cross-section: (N, 2) points, their control-polygon length positions and
    the control polygon's perimeter."""
    st = np.array(p2.floats("st"))
    pts, us = [], []
    cur = st
    poly_len = 0.0
    for seg in p2.chunk("ss").leaves("s3"):
        c1, c2, end = np.array(rd4.parse_floats(seg)).reshape(3, 2)
        seg_len = np.linalg.norm(c1 - cur) + np.linalg.norm(c2 - c1) + np.linalg.norm(end - c2)
        for k in range(SECTION_STEPS):
            t = k / SECTION_STEPS
            pts.append(_bezier(cur, c1, c2, end, t))
            us.append(poly_len + seg_len * t)
        poly_len += seg_len
        cur = end
    return np.array(pts), np.array(us), poly_len


def _envelope(saps, side: str, i: int, pos0: float, pos1: float):
    a, b = saps[i], saps[i + 1]
    v0 = a.floats(f"{side}v")[0]
    v1 = b.floats(f"{side}v")[0]
    nc = a.floats(f"{side}nc")
    pc = b.floats(f"{side}pc")
    return np.array([[pos0, v0], [pos0 + nc[1], nc[0]], [pos1 - pc[1], pc[0]], [pos1, v1]])


def tessellate(master: rd4.Chunk) -> FreeformMesh:
    ph = master.chunk("ph")
    saps = ph.chunks("sap")
    for s in saps[1:]:
        cs = s.chunk("cs")
        if cs is not None and cs.chunk("pv") is not None:
            raise NotImplementedError("several cross-sections")
    for s in saps:
        if s.chunk("cs") is not None and s.chunk("cs").leaf("twst", "0") != "0":
            raise NotImplementedError("twisted sweeps")
    section, sec_u, perimeter = _section(saps[0].chunk("cs").chunk("pv").chunk("pg").chunk("p2"))
    # Path segments and their lengths (sampled finely for arc length).
    cv = [np.array(s.floats("cv")) for s in saps]
    cn = [np.array(s.floats("cnc")) for s in saps]
    cp = [np.array(s.floats("cpc")) for s in saps]
    dense, dense_pos = [], []
    pos = 0.0
    seg_bounds = [0.0]
    for i in range(len(saps) - 1):
        h1 = cn[i] if np.linalg.norm(cn[i]) > 0 else cv[i]
        h2 = cp[i + 1] if np.linalg.norm(cp[i + 1]) > 0 else cv[i + 1]
        t = np.linspace(0, 1, 513)
        pts = _bezier(cv[i], h1, h2, cv[i + 1], t)
        d = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
        dense.append(pts if i == 0 else pts[1:])
        dense_pos.append(pos + (d if i == 0 else d[1:]))
        pos += d[-1]
        seg_bounds.append(pos)
    dense = np.concatenate(dense)
    dense_pos = np.concatenate(dense_pos)

    def path_at(p):
        x = np.array([np.interp(p, dense_pos, dense[:, k]) for k in range(3)])
        j = int(np.clip(np.searchsorted(dense_pos, p), 1, len(dense) - 1))
        tan = dense[j] - dense[j - 1]
        return x, tan / np.linalg.norm(tan)

    # Rings: 16 envelope-parameter steps per segment, positions from the top envelope's x(t).
    rings = []  # (position, {side: scale})
    for i in range(len(saps) - 1):
        env = {s: _envelope(saps, s, i, seg_bounds[i], seg_bounds[i + 1]) for s in "tblr"}
        steps = PATH_STEPS
        for k in range(steps + 1):
            if i > 0 and k == 0:
                continue
            t = k / steps
            p = float(_bezier(*env["t"][:, 0], t))
            scales = {}
            for s in "tblr":
                tt = np.linspace(0, 1, 2049)
                xs = _bezier(*env[s][:, 0], tt)
                ys = _bezier(*env[s][:, 1], tt)
                scales[s] = float(np.interp(p, xs, ys))
            rings.append((p, scales))

    positions, uv, ring_of = [], [], []
    n = len(section)
    for r, (p, sc) in enumerate(rings):
        center, tan = path_at(p)
        ax = np.array([1.0, 0.0, 0.0]) - tan * tan[0]
        ax /= np.linalg.norm(ax)
        bx = np.cross(ax, tan)
        a = section[:, 0] * np.where(section[:, 0] < 0, sc["l"], sc["r"])
        b = section[:, 1] * np.where(section[:, 1] > 0, sc["t"], sc["b"])
        positions.extend(center + np.outer(a, ax) + np.outer(b, bx))
        uv.extend(np.stack([sec_u, np.full(n, p)], axis=1))
        ring_of.extend([r] * n)
    tris = []
    for r in range(len(rings) - 1):
        for k in range(n):
            a0, a1 = r * n + k, r * n + (k + 1) % n
            b0, b1 = a0 + n, a1 + n
            tris.append((a0, b0, a1))
            tris.append((a1, b0, b1))
    positions = list(positions)
    uv = list(uv)
    ring_of = list(ring_of)
    # Flat end caps (fl 1 on the first cross-section), triangulated in the section's 2D plane
    # (the eyelash's section is a concave crescent, so a centre fan would not do).
    if saps[0].chunk("cs").leaf("fl", "0") == "1":
        cap = ear_clip(section)
        for r, flip in ((0, True), (len(rings) - 1, False)):
            for tri in cap:
                idx = [r * n + int(k) for k in tri]
                tris.append(tuple(idx[::-1]) if flip else tuple(idx))
    mesh = FreeformMesh(np.array(positions), np.array(tris), np.array(uv), np.array(ring_of), perimeter)
    return _orient_outwards(mesh)


def ear_clip(poly: np.ndarray) -> list[tuple[int, int, int]]:
    """Triangulate a simple 2D polygon (either winding) by ear clipping."""
    idx = list(range(len(poly)))
    area = 0.5 * np.sum(poly[:, 0] * np.roll(poly[:, 1], -1) - np.roll(poly[:, 0], -1) * poly[:, 1])
    sign = 1.0 if area > 0 else -1.0

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    out = []
    guard = 0
    while len(idx) > 3 and guard < 10000:
        guard += 1
        for k in range(len(idx)):
            i0, i1, i2 = idx[k - 1], idx[k], idx[(k + 1) % len(idx)]
            a, b, c = poly[i0], poly[i1], poly[i2]
            if sign * cross(a, b, c) <= 1e-12:
                continue
            if any(sign * cross(a, b, poly[j]) > 0 and sign * cross(b, c, poly[j]) > 0
                   and sign * cross(c, a, poly[j]) > 0 for j in idx if j not in (i0, i1, i2)):
                continue
            out.append((i0, i1, i2))
            idx.pop(k)
            break
        else:
            break  # degenerate: give up on the rest
    if len(idx) == 3:
        out.append(tuple(idx))
    return out


def _orient_outwards(m: FreeformMesh) -> FreeformMesh:
    """Make triangle winding counter-clockwise seen from outside (positive signed volume)."""
    p, t = m.positions, m.triangles
    vol = np.einsum("ij,ij->i", p[t[:, 0]], np.cross(p[t[:, 1]], p[t[:, 2]])).sum()
    if vol < 0:
        m.triangles = t[:, ::-1].copy()
    return m


def meridian_fraction(m: FreeformMesh) -> np.ndarray:
    """Per vertex: fraction of the surface length from the first end, end caps included."""
    rings = np.unique(m.ring_of)
    centre = np.array([m.positions[m.ring_of == r].mean(axis=0) for r in rings])
    radius = np.array([np.linalg.norm(m.positions[m.ring_of == r] - c, axis=1).mean()
                       for r, c in zip(rings, centre)])
    along = np.array([m.uv[m.ring_of == r, 1].mean() for r in rings])
    arc = radius[0] + np.concatenate([[0.0], np.cumsum(np.hypot(np.diff(along), np.diff(radius)))])
    total = arc[-1] + radius[-1]
    return (arc / total)[np.searchsorted(rings, m.ring_of)]


def texture_coordinates(m: FreeformMesh, quarter_turns: int) -> np.ndarray:
    """Texture (s, t) of a texture map on the primary domain, which covers the whole surface.

    ``s`` follows u around the cross-section, turned by ``quarter_turns`` quarter turns (the
    map's ``rotc``); ``t`` is the meridian fraction (see ``meridian_fraction``). Only ``rotc 3``
    with ``flpv 1`` has been checked against sprites (Blupi's face). The primary domain's
    ``bbox`` is not used: some scenes store the placeholder ``0,0,2,2`` there.
    """
    period = m.perimeter
    s = np.mod(m.uv[:, 0] - quarter_turns * period / 4, period) / period
    return np.stack([s, meridian_fraction(m)], axis=1)


def split_seam(positions: np.ndarray, normals: np.ndarray, triangles: np.ndarray, uv: np.ndarray,
               period: float = 1.0):
    """Duplicate vertices so no triangle's s spans the wrap-around seam (s in [0, period))."""
    pos, nrm, tex = list(positions), list(normals), list(uv)
    out = triangles.copy()
    extra: dict[int, int] = {}
    for f, tri in enumerate(triangles):
        us = uv[tri, 0]
        if us.max() - us.min() <= period / 2:
            continue
        for k, vi in enumerate(tri):
            if uv[vi, 0] < period / 2:
                if vi not in extra:
                    extra[vi] = len(pos)
                    pos.append(positions[vi])
                    nrm.append(normals[vi])
                    tex.append(uv[vi] + np.array([period, 0.0]))
                out[f, k] = extra[vi]
    return np.array(pos), np.array(nrm), out, np.array(tex)
