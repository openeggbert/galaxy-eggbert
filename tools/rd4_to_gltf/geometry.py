"""Canonical part geometry recovered from a Ray Dream DXF export.

Ray Dream wrote ``blupi.dxf`` from ``stop-profil.rd4`` at that scene's stored time: one layer per
geometric object, named ``<object name with ' '/'-' as '_'><depth-first index>``, in world
coordinates at ``DXF_UNITS_PER_SCENE_UNIT`` times the scene's units. Mapping every layer back
through the inverse of its object's geometry frame yields the master's canonical mesh (the shape
before the instance transform and the ``affi`` deformer).

Master *names* differ between scene files (``Corps`` here, `` Wizard`` in marche-profil.rd4), so
parts are keyed by object name and each master is fingerprinted by its geometric definition.
"""

from __future__ import annotations

import hashlib
from dataclasses import dataclass

import numpy as np

import dxf
import rd4
import scene as sc

DXF_UNITS_PER_SCENE_UNIT = 4.0

# Chunks of a master that describe its shading or editor state rather than its shape.
_NON_GEOMETRY = {"sh", "grid", "nm"}


@dataclass
class Mesh:
    positions: np.ndarray  # (V, 3)
    triangles: np.ndarray  # (T, 3) int


@dataclass
class Part:
    mesh: Mesh             # canonical master geometry
    fingerprint: str       # of the master's geometric definition
    source: str            # DXF layer it came from


def weld(tris: np.ndarray, decimals: int = 5) -> Mesh:
    flat = tris.reshape(-1, 3)
    uniq, inverse = np.unique(np.round(flat, decimals), axis=0, return_inverse=True)
    inverse = inverse.ravel()
    pos = np.zeros_like(uniq)
    np.add.at(pos, inverse, flat)
    pos /= np.bincount(inverse, minlength=len(uniq))[:, None]
    t = inverse.reshape(-1, 3)
    t = t[(t[:, 0] != t[:, 1]) & (t[:, 1] != t[:, 2]) & (t[:, 0] != t[:, 2])]
    return Mesh(pos, t)


def layer_name(obj_name: str, index: int) -> str:
    return obj_name.replace(" ", "_").replace("-", "_") + str(index)


def master_fingerprint(master: rd4.Chunk) -> str:
    """Hash of everything in a master except shading/editor state (and its name)."""
    h = hashlib.sha1(master.tag.encode())

    def rec(c: rd4.Chunk):
        for it in c.items:
            if isinstance(it, rd4.Chunk):
                if it.tag in _NON_GEOMETRY:
                    continue
                h.update(b"{" + it.tag.encode())
                rec(it)
                h.update(b"}")
            elif it[0] not in _NON_GEOMETRY:
                h.update(f"{it[0]}={it[1]};".encode("latin-1"))

    rec(master)
    return h.hexdigest()[:12]


def geometric_objects(scene: sc.Scene, t: float | None):
    """Depth-first (obj, geometry_to_world) for objects that reference a master."""
    return [(o, gw) for o, _p, _cw, gw in sc.world_frames(scene, t) if o.master]


def recover_parts(scene: sc.Scene, dxf_path: str) -> tuple[dict[str, Part], list[str]]:
    """Return ({object name: Part}, report lines) from a DXF exported from ``scene`` at its stored time."""
    layers = dxf.read_3dfaces(dxf_path)
    report = []
    parts: dict[str, Part] = {}
    for index, (obj, gw) in enumerate(geometric_objects(scene, None)):
        name = layer_name(obj.name, index)
        if name not in layers:
            raise KeyError(f"DXF has no layer {name!r} for object {obj.name!r}")
        world = layers[name] / DXF_UNITS_PER_SCENE_UNIT
        inv = np.linalg.inv(gw)
        mesh = weld(world @ inv[:3, :3].T + inv[:3, 3])
        if np.linalg.det(gw[:3, :3]) < 0:
            mesh = Mesh(mesh.positions, mesh.triangles[:, ::-1].copy())
        fp = master_fingerprint(scene.masters[obj.master])
        if obj.name in parts:
            if parts[obj.name].fingerprint != fp:
                report.append(f"{name}: WARNING same object name, different master than {parts[obj.name].source}")
            continue
        parts[obj.name] = Part(mesh, fp, name)
        lo, hi = mesh.positions.min(0), mesh.positions.max(0)
        report.append(f"{name}: {obj.name!r} <- master {obj.master!r} [{fp}], {len(mesh.triangles)} triangles, "
                      f"canonical box {np.round(lo, 3)}..{np.round(hi, 3)}")
    return parts, report


def smooth_normals(mesh: Mesh, crease_degrees: float = 60.0, return_index: bool = False):
    """Average face normals per vertex, splitting vertices along creases.

    Returns (positions, normals, triangles) and, with ``return_index``, the source vertex of each
    output vertex.
    """
    p, t = mesh.positions, mesh.triangles
    fn = np.cross(p[t[:, 1]] - p[t[:, 0]], p[t[:, 2]] - p[t[:, 0]])
    fn_unit = fn / np.maximum(np.linalg.norm(fn, axis=1, keepdims=True), 1e-12)
    cos_limit = np.cos(np.radians(crease_degrees))
    corners: dict[int, list[int]] = {}
    for f, tri in enumerate(t):
        for v in tri:
            corners.setdefault(int(v), []).append(f)
    out_p, out_n, out_src = [], [], []
    new_t = t.copy()
    for v, faces in corners.items():
        groups: list[list[int]] = []
        for f in faces:
            for g in groups:
                if fn_unit[g[0]] @ fn_unit[f] >= cos_limit:
                    g.append(f)
                    break
            else:
                groups.append([f])
        for g in groups:
            n = fn[g].sum(0)
            n /= max(np.linalg.norm(n), 1e-12)
            idx = len(out_p)
            out_p.append(p[v])
            out_n.append(n)
            out_src.append(v)
            for f in g:
                new_t[f][t[f] == v] = idx
    if return_index:
        return np.array(out_p), np.array(out_n), new_t, np.array(out_src)
    return np.array(out_p), np.array(out_n), new_t


def orient_outwards(mesh: Mesh) -> Mesh:
    """Counter-clockwise winding seen from outside (positive signed volume)."""
    p, t = mesh.positions, mesh.triangles
    vol = np.einsum("ij,ij->i", p[t[:, 0]], np.cross(p[t[:, 1]], p[t[:, 2]])).sum()
    return Mesh(p, t[:, ::-1].copy()) if vol < 0 else mesh


@dataclass
class MeshUV:
    """Surface coordinates of a mesh master's vertices: ``uv`` as stored (along the path, around
    the section, for the swept body's own surface), each vertex's surface ``domain`` (0 = that
    surface, others the boolean's cut faces) and domain 0's ``extent`` (path, around)."""
    uv: np.ndarray
    domain: np.ndarray
    extent: tuple


def mesh_form(master: rd4.Chunk) -> Mesh:
    """A mesh form (``prim { vmda }``, Ray Dream Studio 5's polygon modeler): ``xprm/PmSt/Pmsh``
    holds the vertices (``Vrts``: ``VerD`` with ``Posn``, ``Flag``, ``UV``) and the polygons
    (``Pgns``: ``PgSz`` n then ``Pgon`` with n vertex indices; quads mostly, up to 36-gons),
    positions in scene units. Polygons are triangulated in their own plane, keeping their
    winding."""
    import freeform_mesh
    pm = master.chunk("xprm").chunk("PmSt").chunk("Pmsh")
    pos = np.array([rd4.parse_floats(v.leaf("Posn")) for v in pm.chunk("Vrts").chunks("VerD")])
    tris: list[tuple[int, int, int]] = []
    for pgon in pm.chunk("Pgns").leaves("Pgon"):
        idx = [int(x) for x in pgon.split(",")]
        if len(idx) == 3:
            tris.append(tuple(idx))
            continue
        pts = pos[idx]
        # Newell's normal, then the polygon in its plane.
        n = np.sum(np.cross(pts - pts.mean(0), np.roll(pts, -1, axis=0) - pts.mean(0)), axis=0)
        if np.linalg.norm(n) < 1e-12:
            continue
        n /= np.linalg.norm(n)
        u = np.cross(n, [1.0, 0.0, 0.0] if abs(n[0]) < 0.9 else [0.0, 1.0, 0.0])
        u /= np.linalg.norm(u)
        flat = np.stack([pts @ u, pts @ np.cross(n, u)], axis=1)
        tris += [(idx[i], idx[j], idx[k]) for i, j, k in freeform_mesh.ear_clip(flat)]
    return Mesh(pos, np.array(tris, dtype=int).reshape(-1, 3))


def canonical_mesh(master: rd4.Chunk):
    """Generated canonical geometry of a master: (Mesh, FreeformMesh or MeshUV or None), or None
    for a master no sprite shows: the flat surface ``COfl`` (README.md, "Geometry")."""
    import freeform_mesh
    import primitives
    if master.tag == "ex":
        ff = freeform_mesh.tessellate(master)
        return Mesh(ff.positions, ff.triangles), ff
    if master.tag == "im":
        # An explicit triangle mesh (Ray Dream Studio 5's boolean results, blupi-scie.rds): one
        # "ff" line per triangle, 3 x (position xyz, a second xyz, u v); the second triple is
        # not the position (rendering it puts the half body nowhere near the sprites). "idls"
        # gives each triangle's surface domain, "uvls" each domain's u, v extent.
        faces = np.array([rd4.parse_floats(f) for f in master.chunk("fl").leaves("ff")]).reshape(-1, 3, 8)
        ids = np.array([int(x) for x in master.chunk("idls").leaves("id")])
        corners = np.concatenate([faces[:, :, 0:3].reshape(-1, 3), faces[:, :, 6:8].reshape(-1, 2),
                                  np.repeat(ids, 3)[:, None]], axis=1)
        uniq, inverse = np.unique(np.round(corners, 5), axis=0, return_inverse=True)
        tris = inverse.ravel().reshape(-1, 3)
        tris = tris[(tris[:, 0] != tris[:, 1]) & (tris[:, 1] != tris[:, 2]) & (tris[:, 0] != tris[:, 2])]
        extent = next((tuple(c.floats("max")) for c in master.chunk("uvls").chunks("uvif") if c.leaf("id") == "0"),
                      (1.0, 1.0))
        return orient_outwards(Mesh(uniq[:, 0:3], tris)), MeshUV(uniq[:, 3:5], uniq[:, 5].astype(int), extent)
    prim = master.chunk("prim")
    kind = prim.chunks()[0].tag if prim is not None and prim.chunks() else master.tag
    if kind == "sp":
        return orient_outwards(primitives.sphere()), None
    if kind == "cyli":
        return orient_outwards(primitives.cylinder()), None
    if kind == "cube":
        return orient_outwards(primitives.cube()), None
    if kind == "co":
        return orient_outwards(primitives.cone()), None
    if kind == "vmda":
        return orient_outwards(mesh_form(master)), None
    if kind == "COfl":
        # The scenes' ground plane, horizontal at Z -15 and seen edge-on by every orthographic
        # camera: no sprite shows it, and the game's own ground replaces it.
        return None
    raise NotImplementedError(f"master {master.string('nm')!r}: primitive {kind!r}")


def compare_with_reference(scene: sc.Scene, dxf_path: str) -> list[str]:
    """Deviation of generated geometry from Ray Dream's DXF of ``scene`` (canonical units)."""
    parts, _ = recover_parts(scene, dxf_path)
    lines = []
    done = set()
    for obj, _gw in geometric_objects(scene, None):
        if obj.master in done or obj.name not in parts:
            continue
        done.add(obj.master)
        mine, _ff = canonical_mesh(scene.masters[obj.master])
        ref = parts[obj.name].mesh
        d = _vertices_to_surface(ref.positions, mine)
        lines.append(f"{obj.master!r} ({obj.name}): DXF vertices to generated surface max {d.max():.4f}, "
                     f"mean {d.mean():.4f} (extent {_extent(ref.positions):.2f})")
    return lines


def _extent(p: np.ndarray) -> float:
    return float(np.linalg.norm(p.max(0) - p.min(0)))


def _vertices_to_surface(points: np.ndarray, mesh: Mesh) -> np.ndarray:
    a, b, c = (mesh.positions[mesh.triangles[:, k]] for k in range(3))
    out = np.full(len(points), np.inf)
    for i in range(0, len(points), 128):
        p = points[i:i + 128, None, :]
        ab, ac = b - a, c - a
        n = np.cross(ab, ac)
        nn = np.maximum((n * n).sum(-1), 1e-30)
        # Distance to each triangle's plane where the projection falls inside, else to its edges.
        t = ((p - a) * n).sum(-1) / nn
        q = p - t[..., None] * n
        def inside(x, y, z):
            return (np.cross(y - x, q - x) * n).sum(-1) >= 0
        ins = inside(a, b, c) & inside(b, c, a) & inside(c, a, b)
        dist = np.where(ins, np.abs(t) * np.sqrt(nn), np.inf)
        for x, y in ((a, b), (b, c), (c, a)):
            e = y - x
            s_ = np.clip(((p - x) * e).sum(-1) / np.maximum((e * e).sum(-1), 1e-30), 0, 1)
            dist = np.minimum(dist, np.linalg.norm(p - (x + e * s_[..., None]), axis=-1))
        out[i:i + 128] = dist.min(1)
    return out
