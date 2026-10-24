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

A cross-section is one or more closed curves (``p2``, possibly nested in ``pg`` groups) of cubic
(``s3``) and straight (``s1``) segments; curves inside others are holes. A point without its own
cross-section (``oo 1``) keeps the previous one; between two different ones the shape blends
linearly along the path (point by point, resampled to equal counts). A path that ends where it
starts (torus, tyre) is closed and has no caps. Twisted sweeps (``twst``) are not supported.
"""

from __future__ import annotations

from dataclasses import dataclass

import numpy as np

import rd4

PATH_STEPS = 16
SECTION_STEPS = 8
# Rings and section points that interpolating their neighbours reproduces within this fraction of
# the master's size are dropped (_simplify); 0 keeps Ray Dream's full sampling.
SIMPLIFY_TOLERANCE = 5e-4


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


@dataclass
class Loop:
    """One sampled closed curve of a cross-section."""
    points: np.ndarray          # (N, 2)
    us: np.ndarray              # (N,) control-polygon length position of each point
    perimeter: float


def _loop(p2: rd4.Chunk) -> Loop:
    """A closed curve: ``s3`` cubic segments sampled at SECTION_STEPS, ``s1`` lines at their start."""
    cur = np.array(p2.floats("st"))
    pts, us = [], []
    poly_len = 0.0
    for tag, value in p2.chunk("ss").items:
        v = np.array(rd4.parse_floats(value))
        if tag == "s3":
            c1, c2, end = v.reshape(3, 2)
            seg_len = np.linalg.norm(c1 - cur) + np.linalg.norm(c2 - c1) + np.linalg.norm(end - c2)
            for k in range(SECTION_STEPS):
                t = k / SECTION_STEPS
                pts.append(_bezier(cur, c1, c2, end, t))
                us.append(poly_len + seg_len * t)
        elif tag == "s1":
            end = v.reshape(2)
            seg_len = np.linalg.norm(end - cur)
            pts.append(cur)
            us.append(poly_len)
        else:
            raise NotImplementedError(f"cross-section segment {tag!r}")
        poly_len += seg_len
        cur = end
    return Loop(np.array(pts), np.array(us), poly_len)


def _section(pv: rd4.Chunk) -> list[Loop]:
    """Every closed curve (``p2``) of a cross-section, in file order (``pg`` groups nest)."""
    loops = []

    def walk(c: rd4.Chunk):
        for it in c.items:
            if isinstance(it, rd4.Chunk):
                if it.tag == "p2":
                    loop = _loop(it)
                    if len(loop.points) >= 3:  # a stray start point without segments (Decor/meca*)
                        loops.append(loop)
                else:
                    walk(it)
    walk(pv)
    return loops


def _area(pts: np.ndarray) -> float:
    return 0.5 * float(np.sum(pts[:, 0] * np.roll(pts[:, 1], -1) - np.roll(pts[:, 0], -1) * pts[:, 1]))


def _inside(pt: np.ndarray, poly: np.ndarray) -> bool:
    x, y = pt
    inside = False
    for (x0, y0), (x1, y1) in zip(poly, np.roll(poly, -1, axis=0)):
        if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            inside = not inside
    return inside


def _depths(loops: list[np.ndarray]) -> list[int]:
    """How many other loops contain each loop (even: an outline, odd: a hole)."""
    return [sum(_inside(a[0], b) for j, b in enumerate(loops) if j != i) for i, a in enumerate(loops)]


def _orient(section: list[Loop]) -> list[Loop]:
    """Outlines counter-clockwise, holes clockwise, so every tube faces out of the solid. A single
    loop is left as it is (Blupi's body: its u, and so his face, depend on its direction)."""
    if len(section) < 2:
        return section
    out = []
    for loop, depth in zip(section, _depths([lp.points for lp in section])):
        if (_area(loop.points) > 0) != (depth % 2 == 0):
            loop = Loop(loop.points[::-1].copy(), loop.perimeter - loop.us[::-1], loop.perimeter)
        out.append(loop)
    return out


def _resample(loop: Loop, n: int) -> Loop:
    """The loop with n points evenly spaced along it (for blending sections of unequal sampling)."""
    if len(loop.points) == n:
        return loop
    closed = np.vstack([loop.points, loop.points[:1]])
    d = np.concatenate([[0.0], np.cumsum(np.linalg.norm(np.diff(closed, axis=0), axis=1))])
    at = np.linspace(0.0, d[-1], n, endpoint=False)
    pts = np.stack([np.interp(at, d, closed[:, k]) for k in range(2)], axis=1)
    us = np.interp(at, d, np.concatenate([loop.us, [loop.perimeter]]))
    return Loop(pts, us, loop.perimeter)


def _common_topology(sections: list[list[Loop]]) -> list[list[Loop]]:
    """Give every section the same loops with the same point counts. A loop a section lacks (the
    jeep's wheel rim has 3 or 4) is collapsed onto its centre in the nearest section that has it."""
    n_loops = max(len(sec) for sec in sections)
    counts = [max(len(sec[j].points) for sec in sections if j < len(sec)) for j in range(n_loops)]
    out = []
    for i, sec in enumerate(sections):
        loops = []
        for j in range(n_loops):
            if j < len(sec):
                loops.append(_resample(sec[j], counts[j]))
                continue
            near = min((k for k, other in enumerate(sections) if j < len(other)), key=lambda k: abs(k - i))
            src = _resample(sections[near][j], counts[j])
            loops.append(Loop(np.repeat(src.points.mean(axis=0)[None], counts[j], axis=0), src.us, src.perimeter))
        out.append(loops)
    return out


def _blend(a: list[Loop], b: list[Loop], f: float) -> list[Loop]:
    if a is b or f <= 0.0:
        return a
    return [Loop((1 - f) * x.points + f * y.points, x.us, x.perimeter) for x, y in zip(a, b)]


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
    for s in saps:
        if s.chunk("cs") is not None and s.chunk("cs").leaf("twst", "0") != "0":
            raise NotImplementedError("twisted sweeps")
    # A point's cross-section, or the previous one's ("oo 1": same as before). Points before the
    # first one with a section take that section (maison.rd4's chair "Seat" starts without one).
    sections = []
    for s in saps:
        cs = s.chunk("cs")
        pv = cs.chunk("pv") if cs is not None else None
        sections.append(_orient(_section(pv)) if pv is not None else sections[-1] if sections and sections[-1] else None)
    first = next((sec for sec in sections if sec is not None), None)
    if first is None:
        raise NotImplementedError("free-form without a cross-section")
    sections = [first if sec is None else sec for sec in sections]
    distinct = list({id(sec): sec for sec in sections}.values())
    if len(distinct) > 1:
        unified = dict(zip((id(sec) for sec in distinct), _common_topology(distinct)))
        sections = [unified[id(sec)] for sec in sections]
    perimeter = sections[0][0].perimeter
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
    # A path ending where it starts (the torus, the tyre) closes on itself: no caps, and the last
    # ring joins the first.
    closed = len(saps) > 2 and np.linalg.norm(cv[0] - cv[-1]) < 1e-3

    def path_at(p):
        x = np.array([np.interp(p, dense_pos, dense[:, k]) for k in range(3)])
        j = int(np.clip(np.searchsorted(dense_pos, p), 1, len(dense) - 1))
        tan = dense[j] - dense[j - 1]
        return x, tan / np.linalg.norm(tan)

    # Rings: 16 envelope-parameter steps per segment, positions from the top envelope's x(t).
    rings = []  # (position, {side: scale}, cross-section)
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
            f = (p - seg_bounds[i]) / max(seg_bounds[i + 1] - seg_bounds[i], 1e-12)
            rings.append((p, scales, _blend(sections[i], sections[i + 1], float(np.clip(f, 0, 1)))))
    if closed:
        rings.pop()

    positions, uv, ring_of = [], [], []
    sizes = [len(loop.points) for loop in rings[0][2]]
    n = sum(sizes)
    previous = None
    for r, (p, sc, section) in enumerate(rings):
        center, tan = path_at(p)
        # A path that steps back on itself (the palm trunk's rings, Explo/plante*) keeps its
        # frame: flipped, the section mirrors and the rings join crosswise into a thin twist.
        if previous is not None and tan @ previous < 0:
            tan = -tan
        previous = tan
        ax = np.array([1.0, 0.0, 0.0]) - tan * tan[0]
        ax /= np.linalg.norm(ax)
        bx = np.cross(ax, tan)
        for loop in section:
            # The section's x, its y and the path's tangent form a right-handed frame: the
            # bird's body (oiseau.rd4), a side profile swept through its thickness, has its head
            # under its eyes only so (side icons 90-105 IoU 0.854 against 0.742); Blupi's
            # symmetric sections do not tell (marche-profil 0.9806 either way).
            a = -loop.points[:, 0] * np.where(loop.points[:, 0] < 0, sc["l"], sc["r"])
            b = loop.points[:, 1] * np.where(loop.points[:, 1] > 0, sc["t"], sc["b"])
            positions.extend(center + np.outer(a, ax) + np.outer(b, bx))
            uv.extend(np.stack([loop.us, np.full(len(loop.us), p)], axis=1))
        ring_of.extend([r] * n)
    positions, uv, ring_of, rings, sizes, n = _simplify(positions, uv, rings, sizes, n)
    tris = []
    starts = np.concatenate([[0], np.cumsum(sizes)[:-1]])
    n_rings = len(rings)
    for r in range(n_rings if closed else n_rings - 1):
        r1 = (r + 1) % n_rings
        for start, size in zip(starts, sizes):
            for k in range(size):
                a0, a1 = r * n + start + k, r * n + start + (k + 1) % size
                b0, b1 = r1 * n + start + k, r1 * n + start + (k + 1) % size
                tris.append((a0, b0, a1))
                tris.append((a1, b0, b1))
    # Flat end caps (fl 1 on the first cross-section), triangulated in the section's 2D plane
    # (the eyelash's section is a concave crescent, so a centre fan would not do; holes are
    # bridged into their outline).
    if saps[0].chunk("cs").leaf("fl", "0") == "1" and not closed:
        for r, flip in ((0, True), (n_rings - 1, False)):
            for tri in cap(rings[r][2]):
                idx = [r * n + int(k) for k in tri]
                tris.append(tuple(idx[::-1]) if flip else tuple(idx))
    mesh = FreeformMesh(np.array(positions), np.array(tris), np.array(uv), np.array(ring_of), perimeter)
    return _orient_outwards(mesh)


def _chain_ends(count: int, fits, closed: bool) -> list[int]:
    """Indices kept by a greedy pass: from each kept index the chain runs as far as ``fits(a, b)``
    allows (every index between a and b reproduced by interpolating a and b)."""
    kept, a = [0], 0
    last = count if closed else count - 1
    while a < last:
        b = a + 1
        while b + 1 <= last and fits(a, b + 1):
            b += 1
        if b < last or not closed:
            kept.append(b)
        a = b
    return kept


def _simplify(positions, uv, rings, sizes, n):
    """Drop the rings and cross-section points that interpolating their kept neighbours
    reproduces within SIMPLIFY_TOLERANCE of the master's size (positions) and of the
    cross-section's perimeter (u). The shape and texture stay; the palm trunks (Explo/plante1*,
    13 segments of 16 rings of 464 points) lose most of their 200 000 triangles, straight
    stretches of a sweep their rings, straight runs of a cross-section their points."""
    P = np.asarray(positions, dtype=float).reshape(len(rings), n, 3)
    UV = np.asarray(uv, dtype=float).reshape(len(rings), n, 2)
    tol = SIMPLIFY_TOLERANCE * float(np.linalg.norm(np.ptp(P.reshape(-1, 3), axis=0)))
    if tol <= 0:
        return positions, uv, list(np.repeat(np.arange(len(rings)), n)), rings, sizes, n
    along = UV[:, 0, 1]
    u_tol = SIMPLIFY_TOLERANCE * max(rings[0][2][0].perimeter, 1e-12)

    def ring_fits(a, b):
        span = along[b] - along[a]
        if span <= 1e-12:
            return False
        f = ((along[a + 1:b] - along[a]) / span)[:, None, None]
        mid = P[a] + f * (P[b] - P[a])
        mid_u = UV[a, :, 0] + f[:, :, 0] * (UV[b, :, 0] - UV[a, :, 0])
        return (np.linalg.norm(mid - P[a + 1:b], axis=2).max() <= tol
                and np.abs(mid_u - UV[a + 1:b, :, 0]).max() <= u_tol)

    # A closed path's last ring joins the first one as it is (its rings are not merged across).
    keep_rings = _chain_ends(len(rings), ring_fits, False)
    Pk, Uk = P[keep_rings], UV[keep_rings]
    keep_points, new_sizes, starts = [], [], np.concatenate([[0], np.cumsum(sizes)[:-1]])
    for start, size, loop in zip(starts, sizes, rings[0][2]):
        period = loop.perimeter
        idx = lambda k: start + k % size
        u_of = lambda k: Uk[:, idx(k), 0] + (period if k >= size else 0.0)

        def point_fits(a, b):
            ua, ub = u_of(a), u_of(b)
            span = ub - ua
            if np.any(span <= 1e-12) or np.any(span > period / 4):
                return False
            for k in range(a + 1, b):
                f = ((u_of(k) - ua) / span)[:, None]
                mid = Pk[:, idx(a)] + f * (Pk[:, idx(b)] - Pk[:, idx(a)])
                if np.linalg.norm(mid - Pk[:, idx(k)], axis=1).max() > tol:
                    return False
            return True

        kept = _chain_ends(size, point_fits, True) if size > 4 else list(range(size))
        keep_points.extend(start + k for k in kept)
        new_sizes.append(len(kept))
    keep_points = np.array(keep_points)
    new_rings = []
    for r in keep_rings:
        p, scales, section = rings[r]
        loops = []
        for start, size, loop in zip(starts, sizes, section):
            local = keep_points[(keep_points >= start) & (keep_points < start + size)] - start
            loops.append(Loop(loop.points[local], loop.us[local], loop.perimeter))
        new_rings.append((p, scales, loops))
    m = len(keep_points)
    return (list(Pk[:, keep_points].reshape(-1, 3)), list(Uk[:, keep_points].reshape(-1, 2)),
            list(np.repeat(np.arange(len(keep_rings)), m)), new_rings, new_sizes, m)


def _segments_cross(p, q, a, b) -> bool:
    def side(o, x, y):
        return (x[0] - o[0]) * (y[1] - o[1]) - (x[1] - o[1]) * (y[0] - o[0])
    d1, d2 = side(p, q, a), side(p, q, b)
    d3, d4 = side(a, b, p), side(a, b, q)
    return d1 * d2 < 0 and d3 * d4 < 0


def cap(section: list[Loop]) -> list[tuple[int, int, int]]:
    """Triangles over a cross-section's area, as indices into its loops' points in order."""
    pts = np.concatenate([loop.points for loop in section])
    starts = np.concatenate([[0], np.cumsum([len(loop.points) for loop in section])[:-1]])
    loops = [list(range(s, s + len(loop.points))) for s, loop in zip(starts, section)]
    if len(loops) == 1:
        return ear_clip(pts)
    depths = _depths([loop.points for loop in section])
    out = []
    for i, outline in enumerate(loops):
        if depths[i] % 2:
            continue
        holes = [loops[j] for j in range(len(loops)) if depths[j] == depths[i] + 1
                 and _inside(pts[loops[j][0]], pts[outline])]
        polygon = list(outline)
        for hole in sorted(holes, key=lambda h: -pts[h][:, 0].max()):
            hi = int(np.argmax(pts[hole][:, 0]))
            h = hole[hi]
            edges = [(polygon[k], polygon[(k + 1) % len(polygon)]) for k in range(len(polygon))]
            edges += [(hh[k], hh[(k + 1) % len(hh)]) for hh in holes for k in range(len(hh))]
            for k in sorted(range(len(polygon)), key=lambda k: np.linalg.norm(pts[polygon[k]] - pts[h])):
                v = polygon[k]
                if not any(_segments_cross(pts[h], pts[v], pts[a], pts[b])
                           for a, b in edges if v not in (a, b) and h not in (a, b)):
                    break
            ring = hole[hi:] + hole[:hi]
            polygon = polygon[:k + 1] + ring + [h] + polygon[k:]
        out += [tuple(polygon[q] for q in tri) for tri in ear_clip(pts[polygon])]
    return out


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
