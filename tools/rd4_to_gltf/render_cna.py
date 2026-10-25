#!/usr/bin/env python3
"""Render a CNA SkinnedModelEXT directory the way the game draws it, back faces culled.

The converters' own checks (verify_sprites.py) draw both sides of every triangle, which hides
faces wound the wrong way; the game culls them (XNA's CullCounterClockwise on clockwise
triangles). This renders the shipped files (skeleton.bin, clips, parts) with the same skinning
as CNA's SkinnedModelEXT::ComputeBoneTransformsEXT and keeps only the triangles facing the
camera, from several directions around the model:

    python3 tools/rd4_to_gltf/render_cna.py avatars3d/blupi --clip Stop --time 0.5 \\
        --out build-probe/blupi-stop.png
    python3 tools/rd4_to_gltf/render_cna.py avatars3d/objects/exit_fleche --clip Frames

Views: yaw 0/90/180/270 degrees at the given --pitch (default 25 degrees above), plus straight
from above. Also prints, per part, the triangles facing inwards (from the part's centre, which
is exact only for convex parts) as a quick winding report.
"""

from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import verify_sprites as vs  # noqa: E402


def _quat_matrix(q):
    """XNA Matrix.CreateFromQuaternion (row vectors)."""
    x, y, z, w = q
    return np.array([[1 - 2 * (y * y + z * z), 2 * (x * y + z * w), 2 * (x * z - y * w), 0],
                     [2 * (x * y - z * w), 1 - 2 * (x * x + z * z), 2 * (y * z + x * w), 0],
                     [2 * (x * z + y * w), 2 * (y * z - x * w), 1 - 2 * (x * x + y * y), 0],
                     [0, 0, 0, 1]])


def _trs(t, q, s):
    scale = np.diag([s[0], s[1], s[2], 1.0])
    move = np.eye(4)
    move[3, :3] = t
    return scale @ _quat_matrix(q) @ move


def _slerp(a, b, f):
    a, b = np.asarray(a, float), np.asarray(b, float)
    d = float(np.dot(a, b))
    if d < 0:
        b, d = -b, -d
    if d > 0.9995:
        r = a + f * (b - a)
        return r / np.linalg.norm(r)
    th = np.arccos(d)
    return (np.sin((1 - f) * th) * a + np.sin(f * th) * b) / np.sin(th)


def load(model_dir: Path):
    raw = (model_dir / "skeleton.bin").read_bytes()
    n = struct.unpack_from("<i", raw, 0)[0]
    p = 4
    parents = list(struct.unpack_from(f"<{n}i", raw, p))
    p += 4 * n
    bind = [np.array(struct.unpack_from("<16f", raw, p + 64 * i)).reshape(4, 4) for i in range(n)]
    p += 64 * n
    ibm = [np.array(struct.unpack_from("<16f", raw, p + 64 * i)).reshape(4, 4) for i in range(n)]
    manifest = json.loads((model_dir / "avatar.skinnedmodel.json").read_text())
    parts = []
    for part in manifest["parts"]:
        stride = part["vertexStride"]
        vb = (model_dir / part["vertices"]).read_bytes()
        count = len(vb) // stride
        rows = [struct.unpack_from("<3f3f2f4f4B", vb, i * stride) for i in range(count)]
        a = np.array(rows, dtype=float)
        tex = np.array(Image.open(model_dir / part["texture"]).convert("RGB")).astype(float) / 255
        parts.append({"name": part["name"], "pos": a[:, 0:3], "nrm": a[:, 3:6], "uv": a[:, 6:8],
                      "w": a[:, 8:12], "j": a[:, 12:16].astype(int),
                      "tri": np.fromfile(model_dir / part["indices"], dtype="<u2").reshape(-1, 3).astype(int),
                      "tex": tex})
    return parents, bind, ibm, parts, manifest


def pose(model_dir: Path, parents, bind, ibm, manifest, clip: str, seconds: float):
    local = [m.copy() for m in bind]
    entry = next((a for a in manifest.get("animations", []) if a["name"] == clip), None)
    if entry is None:
        raise SystemExit(f"no clip {clip!r}: {[a['name'] for a in manifest.get('animations', [])]}")
    c = (model_dir / entry["clip"]).read_bytes()
    duration, = struct.unpack_from("<d", c, 0)
    tracks, = struct.unpack_from("<i", c, 8)
    p = 12
    t = min(max(seconds, 0.0), duration) if duration > 0 else 0.0
    for _ in range(tracks):
        bone, nk = struct.unpack_from("<ii", c, p)
        p += 8
        keys = [struct.unpack_from("<d10f", c, p + 48 * k) for k in range(nk)]
        p += 48 * nk
        if not keys:
            continue
        i = max([k for k, key in enumerate(keys) if key[0] <= t] or [0])
        a = keys[i]
        b = keys[min(i + 1, nk - 1)]
        f = 0.0 if b[0] <= a[0] else (t - a[0]) / (b[0] - a[0])
        lerp = lambda u, v: [x * (1 - f) + y * f for x, y in zip(u, v)]  # noqa: E731
        local[bone] = _trs(lerp(a[1:4], b[1:4]), _slerp(a[4:8], b[4:8], f), lerp(a[8:11], b[8:11]))
    world = []
    for i, parent in enumerate(parents):
        world.append(local[i] if parent < 0 else local[i] @ world[parent])
    return [ibm[i] @ world[i] for i in range(len(parents))]


def skin(part, bones):
    pos = np.c_[part["pos"], np.ones(len(part["pos"]))]
    out = np.zeros((len(pos), 3))
    nrm = np.zeros((len(pos), 3))
    for k in range(4):
        w = part["w"][:, k:k + 1]
        if not w.any():
            continue
        m = np.array(bones)[part["j"][:, k]]
        out += w * np.einsum("ni,nij->nj", pos, m)[:, :3]
        nrm += w * np.einsum("ni,nij->nj", part["nrm"], m[:, :3, :3])
    nrm /= np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12)
    return out, nrm


def render_view(meshes, yaw_deg: float, pitch_deg: float, size: int):
    yaw, pitch = np.radians(yaw_deg), np.radians(pitch_deg)
    back = np.array([np.sin(yaw) * np.cos(pitch), np.sin(pitch), np.cos(yaw) * np.cos(pitch)])
    right = np.cross([0.0, 1.0, 0.0], back)
    if np.linalg.norm(right) < 1e-6:
        right = np.array([1.0, 0.0, 0.0])
    right /= np.linalg.norm(right)
    up = np.cross(back, right)
    pts = np.concatenate([m[0] for m in meshes])
    centre = (pts.min(0) + pts.max(0)) / 2
    extent = np.abs((pts - centre) @ np.stack([right, up]).T).max() * 2.2
    culled = []
    for pos, nrm, tri, colour, uv, tex in meshes:
        a, b, c = pos[tri[:, 0]], pos[tri[:, 1]], pos[tri[:, 2]]
        outward = -np.cross(b - a, c - a)  # clockwise front faces
        culled.append((pos, nrm, tri[(outward @ back) > 0], colour, uv, tex))
    rgb, cover = vs.render(culled, centre + back * 10, right, up, back, size / extent, size,
                           vs.Lighting(back + up, ambient=0.45, diffuse=0.55, highlight=0.0))
    img = np.where(cover[..., None], rgb, np.array([0.16, 0.35, 0.55]))
    return Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("model", type=Path)
    ap.add_argument("--clip", default="Frames")
    ap.add_argument("--time", type=float, default=0.0)
    ap.add_argument("--pitch", type=float, default=25.0)
    ap.add_argument("--size", type=int, default=220)
    ap.add_argument("--out", type=Path)
    args = ap.parse_args(argv)
    parents, bind, ibm, parts, manifest = load(args.model)
    bones = pose(args.model, parents, bind, ibm, manifest, args.clip, args.time)
    meshes = []
    for part in parts:
        pos, nrm = skin(part, bones)
        keep = part["tri"][np.ptp(pos[part["tri"]], axis=1).max(1) > 1e-9]  # parts hidden by scale 0
        if len(keep):
            meshes.append((pos, nrm, keep, np.array([1.0, 1.0, 1.0]), part["uv"], part["tex"]))
            a, b, c = pos[keep[:, 0]], pos[keep[:, 1]], pos[keep[:, 2]]
            inward = ((-np.cross(b - a, c - a)) * ((a + b + c) / 3 - pos[keep].reshape(-1, 3).mean(0))).sum(1) < 0
            if inward.any():
                print(f"{part['name']}: {int(inward.sum())}/{len(keep)} triangles face its centre")
    views = [render_view(meshes, yaw, args.pitch, args.size) for yaw in (0, 90, 180, 270)]
    views.append(render_view(meshes, 0, 89.0, args.size))
    sheet = Image.new("RGB", (args.size * len(views), args.size))
    for k, v in enumerate(views):
        sheet.paste(v, (k * args.size, 0))
    out = args.out or Path(f"{args.model.name}-{args.clip}.png")
    sheet.save(out)
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
