#!/usr/bin/env python3
"""Tessellate the converted models' spheres to their size (2026-11-04, performance).

``primitives.sphere()`` makes every Ray Dream sphere 32 segments by 16 rings, 960 triangles,
whatever its size: the treasure chest's 59 studs, 2.5 sprite pixels across, are 56 640 of its
68 068 triangles, the girder's 20 rivets 19 200 of its 19 348. Ray Dream drew them as true
spheres; the tessellation is the converter's choice, so it can follow their size.

This rewrites a CNA model directory (``export_cna_avatar.py``) in place: each piece (a connected
set of triangles) that is such a sphere -- 960 triangles, its vertices at one distance from their
centre, one texture coordinate (a primitive has none of its own), one joint -- becomes a sphere of
the same centre, radius, joint and winding with ``segments`` = 32 x diameter / FULL_DIAMETER
(rounded up to a multiple of 4, at least 8, at most 32) and half as many rings: full detail from
a quarter block across, 16 x 8 at a tenth, 8 x 4 (48 triangles) for a stud.

    python3 tools/rd4_to_gltf/lower_spheres.py avatars3d/objects/treasure_coffre

``export_objects.py`` runs it on every model it exports.
"""

from __future__ import annotations

import argparse
import json
import math
import sys
from pathlib import Path

import numpy as np

VERTEX = np.dtype([("pos", "<f4", 3), ("nrm", "<f4", 3), ("uv", "<f4", 2), ("w", "<f4", 4), ("j", "u1", 4)])
SPHERE_TRIANGLES = 960      # primitives.sphere(32, 16)
FULL_DIAMETER = 0.25        # blocks: from this size on, the full 32 segments
ROUND_TOLERANCE = 0.02      # of the radius: how far a vertex may be off the sphere


def pieces(pos: np.ndarray, tris: np.ndarray) -> np.ndarray:
    """The connected piece of every triangle (vertices joined where their positions meet)."""
    _keys, vid = np.unique(np.round(pos, 5), axis=0, return_inverse=True)
    vid = vid.ravel()
    parent = np.arange(vid.max() + 1)

    def find(a: int) -> int:
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for a, b, c in vid[tris]:
        ra = find(a)
        parent[find(b)] = ra
        parent[find(c)] = ra
    return np.array([find(a) for a in vid[tris[:, 0]]])


def segments_for(diameter: float) -> int:
    n = math.ceil(32.0 * diameter / FULL_DIAMETER / 4.0) * 4
    return max(8, min(32, n))


def uv_sphere(segments: int) -> tuple[np.ndarray, np.ndarray]:
    """A unit sphere: a vertex at each pole, rings between them; triangles wound outward by the
    right-hand rule (fixed against the original's winding by the caller)."""
    rings = segments // 2
    pos = [(0.0, 0.0, 1.0)]
    for i in range(1, rings):
        th = math.pi * i / rings
        for j in range(segments):
            ph = 2.0 * math.pi * j / segments
            pos.append((math.sin(th) * math.cos(ph), math.sin(th) * math.sin(ph), math.cos(th)))
    pos.append((0.0, 0.0, -1.0))
    bottom = len(pos) - 1
    tris = []
    for j in range(segments):
        tris.append((0, 1 + j, 1 + (j + 1) % segments))
    for i in range(rings - 2):
        a, b = 1 + i * segments, 1 + (i + 1) * segments
        for j in range(segments):
            j1 = (j + 1) % segments
            tris.append((a + j, b + j, b + j1))
            tris.append((a + j, b + j1, a + j1))
    base = 1 + (rings - 2) * segments
    for j in range(segments):
        tris.append((base + j, bottom, base + (j + 1) % segments))
    return np.array(pos), np.array(tris, dtype=np.int64)


def outward_sign(pos: np.ndarray, tris: np.ndarray, centre: np.ndarray) -> float:
    p = pos[tris]
    n = np.cross(p[:, 1] - p[:, 0], p[:, 2] - p[:, 0])
    return float(np.sign(np.sum(np.einsum("ij,ij->i", n, p.mean(axis=1) - centre))))


def lower_spheres(model_dir: Path) -> list[str]:
    manifest = json.loads((model_dir / "avatar.skinnedmodel.json").read_text())
    report = []
    for entry in manifest["parts"]:
        if entry["vertexStride"] != VERTEX.itemsize:
            raise ValueError(f"{model_dir.name}/{entry['name']}: vertex stride {entry['vertexStride']}")
        verts = np.fromfile(model_dir / entry["vertices"], dtype=VERTEX)
        tris = np.fromfile(model_dir / entry["indices"], dtype="<u2").reshape(-1, 3).astype(np.int64)
        piece = pieces(verts["pos"].astype(np.float64), tris)
        keep = np.ones(len(tris), dtype=bool)
        new_verts, new_tris = [], []
        replaced, saved = 0, 0
        for p in np.unique(piece):
            mask = piece == p
            if mask.sum() != SPHERE_TRIANGLES:
                continue
            own = tris[mask]
            used = np.unique(own)
            pos = verts["pos"][used].astype(np.float64)
            centre = (pos.min(axis=0) + pos.max(axis=0)) / 2.0
            dist = np.linalg.norm(pos - centre, axis=1)
            radius = dist.mean()
            if radius <= 0.0 or np.abs(dist - radius).max() > ROUND_TOLERANCE * radius:
                continue
            first = verts[used[0]]
            if (np.ptp(verts["uv"][used], axis=0).max() > 1e-4 or (verts["j"][used] != first["j"]).any()
                    or np.abs(verts["w"][used] - first["w"]).max() > 1e-4):
                continue
            segments = segments_for(2.0 * radius)
            if segments * segments >= SPHERE_TRIANGLES:
                continue
            unit, unit_tris = uv_sphere(segments)
            sphere_pos = centre + radius * unit
            if outward_sign(sphere_pos, unit_tris, centre) != outward_sign(verts["pos"].astype(np.float64), own,
                                                                           centre):
                unit_tris = unit_tris[:, [0, 2, 1]]
            block = np.zeros(len(unit), dtype=VERTEX)
            block["pos"] = sphere_pos
            block["nrm"] = unit
            block["uv"] = first["uv"]
            block["w"] = first["w"]
            block["j"] = first["j"]
            new_verts.append(block)
            new_tris.append(unit_tris)
            keep &= ~mask
            replaced += 1
            saved += SPHERE_TRIANGLES - len(unit_tris)
        if not replaced:
            continue
        kept = tris[keep]
        used = np.unique(kept)
        remap = np.full(len(verts), -1, dtype=np.int64)
        remap[used] = np.arange(len(used))
        out_verts = [verts[used]]
        out_tris = [remap[kept]]
        count = len(used)
        for block, block_tris in zip(new_verts, new_tris):
            out_verts.append(block)
            out_tris.append(block_tris + count)
            count += len(block)
        if count > 65535:
            raise ValueError(f"{model_dir.name}/{entry['name']}: {count} vertices, 16-bit indices")
        np.concatenate(out_verts).tofile(model_dir / entry["vertices"])
        np.concatenate(out_tris).astype("<u2").tofile(model_dir / entry["indices"])
        report.append(f"{model_dir.name}/{entry['name']}: {replaced} sphere(s) to their size, "
                      f"{len(tris)} -> {len(tris) - saved} triangles")
    return report


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("models", type=Path, nargs="+")
    args = ap.parse_args(argv)
    for model in args.models:
        for line in lower_spheres(model):
            print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main())
