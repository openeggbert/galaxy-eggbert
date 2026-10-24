#!/usr/bin/env python3
"""Render a converted .glb with the original Ray Dream game camera and compare it with sprites.

The .glb is read back and skinned on the CPU (so the file's skin and animation are what gets
checked), the camera comes from the Ray Dream scene, and every rendered frame is compared with a
60x60 icon from the game's sprite sheet by silhouette overlap (intersection over union).

    python3 verify_sprites.py --glb blupi-marche.glb --scene marche-profil.rd4 \\
        --sheet mobile-eggbert/Content/icons/blupi.png --icons 5,6,7,8,9,10 --out compare.png
"""

from __future__ import annotations

import argparse
import io
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gltf  # noqa: E402
import rd4  # noqa: E402
import scene as sc  # noqa: E402

ICON = 60
SUPERSAMPLE = 4


# ---- glTF evaluation ------------------------------------------------------------------

def quat_matrix(q: np.ndarray) -> np.ndarray:
    x, y, z, w = q / np.linalg.norm(q)
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
        [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
        [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)],
    ])


def trs(node: dict) -> np.ndarray:
    m = np.eye(4)
    if "matrix" in node:
        return np.array(node["matrix"]).reshape(4, 4).T
    r = quat_matrix(np.array(node.get("rotation", [0, 0, 0, 1]), dtype=float))
    s = np.array(node.get("scale", [1, 1, 1]), dtype=float)
    m[:3, :3] = r * s
    m[:3, 3] = node.get("translation", [0, 0, 0])
    return m


def slerp(a, b, f):
    if a @ b < 0:
        b = -b
    d = float(np.clip(a @ b, -1, 1))
    th = np.arccos(d)
    if th < 1e-9:
        return a
    return (np.sin((1 - f) * th) * a + np.sin(f * th) * b) / np.sin(th)


class Model:
    def __init__(self, path: Path):
        self.doc, self.blob = gltf.read_glb(path)
        d = self.doc
        self.parent = {}
        for i, n in enumerate(d["nodes"]):
            for c in n.get("children", []):
                self.parent[c] = i
        skinned = [n for n in d["nodes"] if "skin" in n and "mesh" in n]
        if len({n["skin"] for n in skinned}) != 1:
            raise SystemExit("expected every mesh to share one skin")
        self.skin = d["skins"][skinned[0]["skin"]]
        self.ibm = gltf.read_accessor(d, self.blob, self.skin["inverseBindMatrices"]).reshape(-1, 4, 4)
        self.ibm = np.transpose(self.ibm, (0, 2, 1))  # column-major storage
        self.prims = []
        for node in skinned:
            for p in d["meshes"][node["mesh"]]["primitives"]:
                a = p["attributes"]
                pbr = d["materials"][p["material"]]["pbrMetallicRoughness"]
                mat = pbr.get("baseColorFactor", [1.0, 1.0, 1.0, 1.0])
                srgb = [((c ** (1 / 2.4)) * 1.055 - 0.055) if c > 0.0031308 else c * 12.92 for c in mat[:3]]
                tex = uv = None
                if "baseColorTexture" in pbr:
                    image = d["images"][d["textures"][pbr["baseColorTexture"]["index"]]["source"]]
                    view = d["bufferViews"][image["bufferView"]]
                    raw = self.blob[view.get("byteOffset", 0):view.get("byteOffset", 0) + view["byteLength"]]
                    tex = np.array(Image.open(io.BytesIO(raw)).convert("RGB")).astype(float) / 255
                    uv = gltf.read_accessor(d, self.blob, a["TEXCOORD_0"]).astype(float)
                self.prims.append({
                    "pos": gltf.read_accessor(d, self.blob, a["POSITION"]).astype(float),
                    "nrm": gltf.read_accessor(d, self.blob, a["NORMAL"]).astype(float),
                    "joints": gltf.read_accessor(d, self.blob, a["JOINTS_0"]).astype(int),
                    "weights": gltf.read_accessor(d, self.blob, a["WEIGHTS_0"]).astype(float),
                    "tris": gltf.read_accessor(d, self.blob, p["indices"]).reshape(-1, 3).astype(int),
                    "color": np.array(srgb),
                    "uv": uv,
                    "tex": tex,
                })
        self.anim = d.get("animations", [None])[0]

    def select_clip(self, name: str | None):
        anims = self.doc.get("animations", [])
        if name is not None:
            match = [a for a in anims if a.get("name") == name]
            if not match:
                raise SystemExit(f"no animation {name!r}; have {[a.get('name') for a in anims]}")
            self.anim = match[0]
        return self.anim

    def pose(self, seconds: float) -> list[np.ndarray]:
        d = self.doc
        local = [trs(n) for n in d["nodes"]]
        if self.anim:
            overrides: dict[int, dict] = {}
            for ch in self.anim["channels"]:
                s = self.anim["samplers"][ch["sampler"]]
                times = gltf.read_accessor(d, self.blob, s["input"])
                vals = gltf.read_accessor(d, self.blob, s["output"]).astype(float)
                i = int(np.clip(np.searchsorted(times, seconds) - 1, 0, len(times) - 2))
                f = float(np.clip((seconds - times[i]) / max(times[i + 1] - times[i], 1e-9), 0, 1))
                path = ch["target"]["path"]
                if s.get("interpolation", "LINEAR") == "STEP":
                    v = vals[i] if f < 1 else vals[i + 1]
                elif path == "rotation":
                    v = slerp(vals[i], vals[i + 1], f)
                else:
                    v = vals[i] * (1 - f) + vals[i + 1] * f
                overrides.setdefault(ch["target"]["node"], {})[path] = v
            for n, o in overrides.items():
                node = dict(d["nodes"][n])
                node.update({k: list(v) for k, v in o.items()})
                local[n] = trs(node)
        world: dict[int, np.ndarray] = {}

        def g(i):
            if i not in world:
                world[i] = local[i] if i not in self.parent else g(self.parent[i]) @ local[i]
            return world[i]

        return [g(j) @ self.ibm[k] for k, j in enumerate(self.skin["joints"])]

    def skinned(self, seconds: float):
        jm = self.pose(seconds)
        # Normals by the inverse transpose: shape joints scale their parts unevenly.
        nm = np.array([np.linalg.inv(m[:3, :3]).T for m in jm])
        out = []
        for p in self.prims:
            pos = np.zeros_like(p["pos"])
            nrm = np.zeros_like(p["nrm"])
            for k in range(4):
                w = p["weights"][:, k:k + 1]
                if not w.any():
                    continue
                m = np.array(jm)[p["joints"][:, k]]
                pos += w * (np.einsum("nij,nj->ni", m[:, :3, :3], p["pos"]) + m[:, :3, 3])
                nrm += w * np.einsum("nij,nj->ni", nm[p["joints"][:, k]], p["nrm"])
            nrm /= np.maximum(np.linalg.norm(nrm, axis=1, keepdims=True), 1e-12)
            out.append((pos, nrm, p["tris"], p["color"], p["uv"], p["tex"]))
        return out


# ---- camera ---------------------------------------------------------------------------

class Camera:
    def __init__(self, scene: sc.Scene, name: str | None, extras: dict):
        cams = [(o, cw) for o, _p, cw, _g in sc.world_frames(scene, None) if o.kind == "ca"]
        pick = [c for c in cams if name and name in c[0].name] if name else cams[-1:]
        if not pick:
            raise SystemExit(f"no camera matching {name!r}; have {[c[0].name for c in cams]}")
        self.obj, cw = pick[0]
        self.scene = scene
        self.c = np.array(extras["rd_to_gltf"])
        self.scale = extras["scale"]
        self.offset = np.array(extras["offset"])
        proj = self.obj.chunk.chunk("proj")
        iso = proj.chunk("iso") if proj else None
        if iso is None:
            raise SystemExit(f"camera {self.obj.name!r} is not orthographic ('iso'); not supported yet")
        # The orthographic frame is ``size`` / ``zoom`` Ray Dream units; the rendered image fits its
        # height (fitted against marche-profil's sprites: best IoU at 0.8775-0.885 x zoom for a 60 px
        # image, and 60 / 68.03136 = 0.882).
        self.zoom = iso.float("zoom")
        self.frame_height = self.obj.chunk.floats("size")[1]

    def light(self, tick: float) -> Lighting:
        """The scene's first light as a point light in glTF space (direction fallback)."""
        for o, _p, cw, _g in sc.world_frames(self.scene, tick):
            if o.kind == "li":
                return Lighting(self.c @ (cw[:3, 3] * self.scale) + self.offset, point=True)
        return Lighting(np.array([0.4, 0.7, 0.6]) / np.linalg.norm([0.4, 0.7, 0.6]))

    def frame(self, tick: float):
        cw = next(cw for o, _p, cw, _g in sc.world_frames(self.scene, tick) if o is self.obj)
        r = self.c @ cw[:3, :3]
        pos = self.c @ (cw[:3, 3] * self.scale) + self.offset
        return pos, r[:, 0], r[:, 1], r[:, 2]


# ---- rasterizer -----------------------------------------------------------------------

class Lighting:
    """Ray Dream's look, approximately: ambient + one light, Gouraud-shaded, with a highlight.

    ``light`` is either a direction (towards the light) or, with ``point=True``, a position.
    The default intensities are the Blupi scenes' (ambient 0.5 grey, spot colour 0.5 grey at
    brightness 1, shader highlight 0.5).
    """

    def __init__(self, light, point=False, ambient=0.5, diffuse=0.5, highlight=0.5, shininess=24.0):
        self.light = np.asarray(light, dtype=float)
        self.point = point
        self.ambient, self.diffuse, self.highlight, self.shininess = ambient, diffuse, highlight, shininess

    def shade(self, pos, nrm, view_dir):
        to_light = self.light - pos if self.point else np.broadcast_to(self.light, pos.shape)
        to_light = to_light / np.maximum(np.linalg.norm(to_light, axis=1, keepdims=True), 1e-12)
        ndl = np.clip((nrm * to_light).sum(1), 0, 1)
        half = to_light + view_dir
        half /= np.maximum(np.linalg.norm(half, axis=1, keepdims=True), 1e-12)
        spec = self.highlight * np.clip((nrm * half).sum(1), 0, 1) ** self.shininess * (ndl > 0)
        return self.ambient + self.diffuse * ndl, spec


def render(meshes, cam_pos, right, up, back, px_per_unit, size, light, pixel=1):
    """Orthographic z-buffer render; the camera axis passes through the centre of the middle pixel
    (``pixel`` is the size of one output pixel in render samples). ``light`` is a Lighting or a
    plain direction vector."""
    if not isinstance(light, Lighting):
        light = Lighting(light, ambient=0.35, diffuse=0.65, highlight=0.0)
    w = h = size
    depth = np.full((h, w), np.inf)
    color = np.zeros((h, w, 3))
    cover = np.zeros((h, w), dtype=bool)
    ys, xs = np.mgrid[0:h, 0:w] + 0.5
    for mesh in meshes:
        pos, nrm, tris, base = mesh[:4]
        uv, tex = (mesh[4], mesh[5]) if len(mesh) > 5 and mesh[4] is not None else (None, None)
        rel = pos - cam_pos
        sx = w / 2 + pixel / 2 + (rel @ right) * px_per_unit
        sy = h / 2 + pixel / 2 - (rel @ up) * px_per_unit
        sz = -(rel @ back)  # distance in front of the camera
        shade, spec = light.shade(pos, nrm, back)
        lit = base * shade[:, None] + spec[:, None]
        for a, b, c in tris:
            x0, x1 = int(max(0, np.floor(min(sx[a], sx[b], sx[c])))), int(min(w, np.ceil(max(sx[a], sx[b], sx[c]))))
            y0, y1 = int(max(0, np.floor(min(sy[a], sy[b], sy[c])))), int(min(h, np.ceil(max(sy[a], sy[b], sy[c]))))
            if x0 >= x1 or y0 >= y1:
                continue
            px, py = xs[y0:y1, x0:x1], ys[y0:y1, x0:x1]
            d = (sx[b] - sx[a]) * (sy[c] - sy[a]) - (sx[c] - sx[a]) * (sy[b] - sy[a])
            if abs(d) < 1e-12:
                continue
            l1 = ((sx[c] - px) * (sy[a] - py) - (sx[a] - px) * (sy[c] - py)) / d
            l2 = ((sx[a] - px) * (sy[b] - py) - (sx[b] - px) * (sy[a] - py)) / d
            l0 = 1 - l1 - l2
            inside = (l0 >= 0) & (l1 >= 0) & (l2 >= 0)
            if not inside.any():
                continue
            z = l0 * sz[a] + l1 * sz[b] + l2 * sz[c]
            sub = depth[y0:y1, x0:x1]
            win = inside & (z < sub)
            sub[win] = z[win]
            if tex is None:
                col = (l0[..., None] * lit[a] + l1[..., None] * lit[b] + l2[..., None] * lit[c])
            else:
                # Nearest texel (Ray Dream's texture maps here have interpolation off), wrapping.
                su = l0 * uv[a, 0] + l1 * uv[b, 0] + l2 * uv[c, 0]
                sv = l0 * uv[a, 1] + l1 * uv[b, 1] + l2 * uv[c, 1]
                th, tw = tex.shape[:2]
                tx = np.mod(np.floor(su * tw), tw).astype(int)
                ty = np.clip(np.floor(sv * th), 0, th - 1).astype(int)
                sh_ = l0 * shade[a] + l1 * shade[b] + l2 * shade[c]
                sp_ = l0 * spec[a] + l1 * spec[b] + l2 * spec[c]
                col = tex[ty, tx] * sh_[..., None] + sp_[..., None]
            color[y0:y1, x0:x1][win] = col[win]
            cover[y0:y1, x0:x1][win] = True
    return color, cover


def downsample(color, cover, f):
    h, w = cover.shape
    a = cover.reshape(h // f, f, w // f, f).mean((1, 3))
    c = (color * cover[..., None]).reshape(h // f, f, w // f, f, 3).sum((1, 3)) / np.maximum(
        cover.reshape(h // f, f, w // f, f).sum((1, 3)), 1)[..., None]
    return c, a


def iou(a: np.ndarray, b: np.ndarray) -> float:
    u = (a | b).sum()
    return float((a & b).sum() / u) if u else 1.0


def best_shift(a: np.ndarray, b: np.ndarray, r: int = 3):
    best = (iou(a, b), 0, 0)
    for dy in range(-r, r + 1):
        for dx in range(-r, r + 1):
            s = np.roll(np.roll(a, dy, 0), dx, 1)
            v = iou(s, b)
            if v > best[0] + 1e-9:
                best = (v, dx, dy)
    return best


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--glb", required=True)
    ap.add_argument("--scene", required=True, help="Ray Dream scene that holds the camera")
    ap.add_argument("--camera", default="Jeu", help="substring of the camera name (default: game camera)")
    ap.add_argument("--sheet", required=True, help="sprite sheet of 60x60 icons, 10 per row")
    ap.add_argument("--icons", required=True, help="comma-separated icon indices, one per rendered frame")
    ap.add_argument("--out", required=True, help="comparison montage .png")
    ap.add_argument("--min-iou", type=float, default=0.0, help="fail if any frame scores below this")
    ap.add_argument("--clip", help="animation to check (default: the first)")
    args = ap.parse_args(argv)

    model = Model(Path(args.glb))
    anim = model.select_clip(args.clip)
    extras = model.doc["asset"]["extras"]["rd4_to_gltf"]
    clip_info = next((c for c in extras.get("clips", []) if anim and c["name"] == anim.get("name")), None)
    scene = sc.read_scene(rd4.parse_file(args.scene))
    cam = Camera(scene, args.camera, extras)
    sheet = np.array(Image.open(args.sheet).convert("RGBA"))
    icons = [int(x) for x in args.icons.split(",")]
    t0, t1 = clip_info["ticks"] if clip_info else extras["clip_ticks"]
    step = sc.TICKS_PER_SECOND / scene.fps
    per_tick = 1.0 / sc.TICKS_PER_SECOND
    if extras.get("frame_rate"):
        per_tick = scene.fps / (sc.TICKS_PER_SECOND * extras["frame_rate"])
    px = cam.zoom * ICON / cam.frame_height / extras["scale"]

    renders, rows, scores = [], [], []
    for f, icon in enumerate(icons):
        tick = t0 + f * step
        pos, right, up, back = cam.frame(tick)
        color, cover = render(model.skinned(max(0.0, tick - t0) * per_tick), pos, right, up, back,
                              px * SUPERSAMPLE, ICON * SUPERSAMPLE, cam.light(tick), SUPERSAMPLE)
        c, a = downsample(color, cover, SUPERSAMPLE)
        renders.append((c, a))
        r, col = divmod(icon, 10)
        spr = sheet[r * ICON:(r + 1) * ICON, col * ICON:(col + 1) * ICON]
        sm, rm = spr[..., 3] > 127, a > 0.5
        score = iou(rm, sm)
        shifted = best_shift(rm, sm)
        scores.append(score)
        print(f"frame {f} tick {tick:6.0f} -> icon {icon:3d}: IoU {score:.3f}"
              f" (best {shifted[0]:.3f} at shift dx={shifted[1]} dy={shifted[2]})")
        rows.append((spr, c, a, sm, rm))

    # Which icon each frame matches best (detects ordering problems).
    print("IoU matrix (rows: frames, columns: icons):")
    for f, (_c, a) in enumerate(renders):
        line = []
        for icon in icons:
            r, col = divmod(icon, 10)
            spr = sheet[r * ICON:(r + 1) * ICON, col * ICON:(col + 1) * ICON]
            line.append(f"{iou(a > 0.5, spr[..., 3] > 127):.2f}")
        print("  " + " ".join(line))

    z = 4
    bg = np.array([40, 90, 140], dtype=float)
    tiles = []
    for spr, c, a, sm, rm in rows:
        s_rgb = spr[..., :3] * (spr[..., 3:] / 255) + bg * (1 - spr[..., 3:] / 255)
        r_rgb = c * 255 * a[..., None] + bg * (1 - a[..., None])
        diff = np.zeros((ICON, ICON, 3)) + 30
        diff[sm & rm] = (200, 200, 200)
        diff[sm & ~rm] = (230, 60, 60)    # sprite only
        diff[~sm & rm] = (60, 200, 90)    # render only
        tiles.append(np.concatenate([s_rgb, r_rgb, diff], axis=0))
    montage = np.concatenate(tiles, axis=1).clip(0, 255).astype(np.uint8)
    Image.fromarray(montage).resize((montage.shape[1] * z, montage.shape[0] * z), Image.NEAREST).save(args.out)
    print(f"mean IoU {np.mean(scores):.3f}; wrote {args.out} (rows: sprite, render, overlap -"
          f" red sprite only, green render only)")
    return 0 if min(scores) >= args.min_iou else 1


if __name__ == "__main__":
    raise SystemExit(main())
