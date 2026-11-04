#!/usr/bin/env python3
"""Terrain reliefs: each terrain tile's Ray Dream scene as the relief the game lays on a cell's face
(owner 2026-11-04, Q-001/Q-004: every terrain tile as its model).

The terrain scenes are not blocks: each is a slab modelled to be seen from the front only (0.15-0.55
of a block deep, up to 1.37 blocks wide, so that the lumps of neighbouring tiles overlap and the
cropped renders join). A relief is that slab's front half:

1. the model of ``build_object_library.py`` (``--library``: one ``<object>_<scene>/`` per scene, its
   ``icons.txt`` naming the sprite icons it draws and whether mirrored), posed at the icon's frame;
2. its triangles facing away from the camera dropped (they lie inside the cell);
3. welded, decimated to ``--budget`` triangles and smoothed by angle in Blender
   (``blender_decimate.py``);
4. cut along the cell's edges (x = +-0.5, y = +-0.5): every triangle then lies in one of nine
   squares of the face's plane, the face's own or an overhang into a neighbour's, tagged with the
   sides it overhangs (bit 0 +x, 1 -x, 2 +y, 3 -y), and textured by the tile's sprite projected from
   the front (the camera the sprite was rendered with), wrapped into the tile: the front view is
   exactly the sprite, and an overhang shows the tile's own continuation;
5. moved so its front-most point lies at z = 0 (the face's plane; the relief is inside the cell),
   its triangles wholly deeper than ``--max-depth`` dropped (the opposite face's relief covers the
   cell's back half).

``reliefs.txt`` beside this script holds per-model settings (``drop=R,G,B``: scene props the
sprites were rendered without).

Writes ``<out>/<model>[.m].gtr`` (little-endian: ``GTR1``, uint32 vertex and triangle counts,
float32 x y z nx ny nz u v per vertex, uint32 a b c per triangle, uint8 overhang mask per
triangle) and ``<out>/index.txt`` (``icon <icon> <file>`` per icon). Model space: +x right,
+y up, +z toward the viewer, in blocks.

    python3 tools/terrain_relief/build_relief.py --library build-probe/rd4-terrain \\
        --out avatars3d/terrain [--only t_roc4a_roc4a] [--budget 1500]
"""

from __future__ import annotations

import argparse
import fnmatch
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "rd4_to_gltf"))

import verify_sprites as vs  # noqa: E402

BACK_FACING = -0.3  # a triangle whose normal's z is below this faces away from the sprite camera


def read_icons(model_dir: Path) -> list[tuple[int, float, int]]:
    """(icon, seconds, mirror) per icon line of the model's icons.txt."""
    out = []
    for line in (model_dir / "icons.txt").read_text().splitlines():
        p = line.split()
        if p and p[0] == "icon":
            out.append((int(p[2]), float(p[3]), int(p[4])))
    return out


def read_settings(path: Path) -> list[tuple[str, dict[str, str]]]:
    out = []
    if path.exists():
        for line in path.read_text().splitlines():
            p = line.split("#", 1)[0].split()
            if p:
                out.append((p[0], dict(o.split("=", 1) for o in p[1:])))
    return out


def settings_for(name: str, settings) -> dict[str, str]:
    found: dict[str, str] = {}
    for pattern, options in settings:
        if fnmatch.fnmatch(name, pattern):
            found.update(options)
    return found


def posed_front(model_dir: Path, seconds: float, mirror: int, drop=()) -> tuple[np.ndarray, np.ndarray]:
    """The model's triangles facing the camera: positions (n, 3) and triangles (m, 3)."""
    model = vs.Model(model_dir / "model.glb")
    model.select_clip("Frames")
    positions, triangles, base = [], [], 0
    for pos, nrm, tris, color, *_ in model.skinned(seconds):
        if any(np.allclose(np.asarray(color, float), d, atol=0.01) for d in drop):
            continue
        if mirror:
            pos = pos * np.array([-1.0, 1.0, 1.0])
            nrm = nrm * np.array([-1.0, 1.0, 1.0])
            tris = tris[:, ::-1]
        a, b, c = pos[tris[:, 0]], pos[tris[:, 1]], pos[tris[:, 2]]
        geo = np.cross(b - a, c - a)
        # Orient by the part's own normals (the converter's winding is not relied on).
        flip = (geo * (nrm[tris[:, 0]] + nrm[tris[:, 1]] + nrm[tris[:, 2]])).sum(1) < 0
        geo[flip] *= -1
        length = np.linalg.norm(geo, axis=1)
        keep = (length > 1e-12) & (geo[:, 2] >= BACK_FACING * np.maximum(length, 1e-12))
        t = tris[keep].copy()
        t[flip[keep]] = t[flip[keep]][:, ::-1]
        positions.append(pos)
        triangles.append(t + base)
        base += len(pos)
    return np.vstack(positions), np.vstack(triangles)


def write_obj(path: Path, pos: np.ndarray, tris: np.ndarray) -> None:
    lines = ["v %.6f %.6f %.6f" % tuple(p) for p in pos]
    lines += ["f %d %d %d" % (a + 1, b + 1, c + 1) for a, b, c in tris]
    path.write_text("\n".join(lines) + "\n")


def read_obj(path: Path) -> tuple[np.ndarray, np.ndarray, list[tuple[tuple[int, int], ...]]]:
    """Positions, normals and triangles of (position, normal) index pairs."""
    pos, nrm, faces = [], [], []
    for line in path.read_text().splitlines():
        p = line.split()
        if not p:
            continue
        if p[0] == "v":
            pos.append([float(x) for x in p[1:4]])
        elif p[0] == "vn":
            nrm.append([float(x) for x in p[1:4]])
        elif p[0] == "f":
            corners = []
            for c in p[1:]:
                parts = c.split("/")
                corners.append((int(parts[0]) - 1, int(parts[2]) - 1 if len(parts) > 2 and parts[2] else -1))
            for k in range(1, len(corners) - 1):
                faces.append((corners[0], corners[k], corners[k + 1]))
    return np.array(pos), np.array(nrm), faces


def decimate(pos: np.ndarray, tris: np.ndarray, budget: int, angle: float, blender: str):
    with tempfile.TemporaryDirectory(dir=HERE.parent.parent / "build-probe") as tmp:
        src, dst = Path(tmp) / "in.obj", Path(tmp) / "out.obj"
        write_obj(src, pos, tris)
        run = subprocess.run([blender, "-b", "--python", str(HERE / "blender_decimate.py"), "--", str(src),
                              str(dst), str(budget), str(angle)], capture_output=True, text=True)
        if not dst.exists():
            raise SystemExit(run.stdout[-2000:] + run.stderr[-2000:])
        return read_obj(dst)


def split(poly: list[tuple[np.ndarray, np.ndarray]], axis: int, value: float):
    """A convex polygon of (position, normal) cut by the line coordinate[axis] == value."""
    below, above = [], []
    n = len(poly)
    for i in range(n):
        a, b = poly[i], poly[(i + 1) % n]
        da, db = a[0][axis] - value, b[0][axis] - value
        (below if da <= 0 else above).append(a)
        if (da < 0 < db) or (db < 0 < da):
            t = da / (da - db)
            v = (a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]))
            below.append(v)
            above.append(v)
    return [p for p in (below, above) if len(p) >= 3]


def cut_and_project(pos, nrm, faces):
    """Vertices (x y z nx ny nz u v), triangles and their overhang masks."""
    verts, tris, masks = [], [], []
    index: dict[tuple, int] = {}
    for face in faces:
        pieces = [[(pos[p].copy(), nrm[n].copy() if n >= 0 else np.zeros(3)) for p, n in face]]
        for axis in (0, 1):
            for value in (-0.5, 0.5):
                pieces = [q for piece in pieces for q in split(piece, axis, value)]
        for piece in pieces:
            centre = np.mean([v[0] for v in piece], axis=0)
            rx = int(np.floor(centre[0] + 0.5))  # -1, 0, 1: the square along x
            ry = int(np.floor(centre[1] + 0.5))
            mask = (1 if rx > 0 else 0) | (2 if rx < 0 else 0) | (4 if ry > 0 else 0) | (8 if ry < 0 else 0)
            ids = []
            for p, n in piece:
                length = np.linalg.norm(n)
                n = n / length if length > 1e-9 else np.array([0.0, 0.0, 1.0])
                u, v = p[0] + 0.5 - rx, 0.5 - p[1] + ry
                key = tuple(np.round([p[0], p[1], p[2], n[0], n[1], n[2], u, v], 6))
                if key not in index:
                    index[key] = len(verts)
                    verts.append(key)
                ids.append(index[key])
            for k in range(1, len(ids) - 1):
                if len({ids[0], ids[k], ids[k + 1]}) == 3:
                    tris.append((ids[0], ids[k], ids[k + 1]))
                    masks.append(mask)
    return np.array(verts, dtype=np.float32), np.array(tris, dtype=np.uint32), np.array(masks, dtype=np.uint8)


def write_gtr(path: Path, verts: np.ndarray, tris: np.ndarray, masks: np.ndarray) -> None:
    with path.open("wb") as f:
        f.write(b"GTR1")
        f.write(struct.pack("<II", len(verts), len(tris)))
        f.write(verts.astype("<f4").tobytes())
        f.write(tris.astype("<u4").tobytes())
        f.write(masks.astype("u1").tobytes())


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--library", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--only", help="comma-separated model directory names")
    ap.add_argument("--budget", type=int, default=1500, help="triangles of the front half before the cut")
    ap.add_argument("--angle", type=float, default=40.0, help="smooth-by-angle threshold, degrees")
    ap.add_argument("--max-depth", type=float, default=0.5, help="triangles wholly deeper are dropped")
    ap.add_argument("--blender", default="blender")
    args = ap.parse_args(argv)
    settings = read_settings(HERE / "reliefs.txt")
    args.out.mkdir(parents=True, exist_ok=True)
    keep = set(args.only.split(",")) if args.only else None
    index_path = args.out / "index.txt"
    index = {}
    if index_path.exists():
        for line in index_path.read_text().splitlines():
            p = line.split()
            if p and p[0] == "icon":
                index[int(p[1])] = p[2]
    for model_dir in sorted(d for d in args.library.iterdir() if (d / "icons.txt").exists()):
        if keep is not None and model_dir.name not in keep:
            continue
        done: dict[tuple[float, int], str] = {}
        for icon, seconds, mirror in read_icons(model_dir):
            if (seconds, mirror) not in done:
                options = settings_for(model_dir.name, settings)
                drop = [tuple(float(x) for x in c.split(",")) for c in options.get("drop", "").split(";") if c]
                pos, tris = posed_front(model_dir, seconds, mirror, drop)
                dpos, dnrm, faces = decimate(pos, tris, args.budget, args.angle, args.blender)
                dpos[:, 2] -= dpos[:, 2].max()  # the front-most point on the face's plane
                faces = [f for f in faces if max(dpos[p][2] for p, _ in f) >= -args.max_depth]
                verts, out_tris, masks = cut_and_project(dpos, dnrm, faces)
                name = model_dir.name + (".m" if mirror else "") + (f".{seconds:g}" if seconds else "") + ".gtr"
                write_gtr(args.out / name, verts, out_tris, masks)
                done[(seconds, mirror)] = name
                print(f"{name}: {len(tris)} front triangles -> {len(faces)} -> {len(out_tris)} cut, "
                      f"{len(verts)} vertices, depth {-dpos[:, 2].min():.2f}", flush=True)
            index[icon] = done[(seconds, mirror)]
    index_path.write_text("# icon <icon> <relief>: the terrain tile drawn as that relief (build_relief.py)\n" +
                          "".join(f"icon {i} {n}\n" for i, n in sorted(index.items())))
    return 0


if __name__ == "__main__":
    sys.exit(main())
