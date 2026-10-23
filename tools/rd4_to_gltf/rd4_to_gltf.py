#!/usr/bin/env python3
"""Convert an animated Ray Dream scene (Speedy Blupi originals) into a skinned glTF binary.

Every scene object that references a master becomes one rigidly skinned part on its own joint;
groups become joints without geometry. The scene's own animation is sampled into one clip.

    python3 rd4_to_gltf.py --scene marche-profil.rd4 --out blupi-marche.glb \\
        [--reference-scene stop-profil.rd4 --reference-dxf blupi.dxf]

Geometry is generated from the scene's own masters (primitives.py, freeform_mesh.py); joint
positions, rotations and keys come from the scene. With a reference scene and its Ray Dream DXF
export, the generated shapes are also measured against Ray Dream's own triangulation. See
README.md for the verified conventions.
"""

from __future__ import annotations

import argparse
import io
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import freeform_mesh  # noqa: E402
import geometry  # noqa: E402
import gltf  # noqa: E402
import rd4  # noqa: E402
import scene as sc  # noqa: E402
import shading  # noqa: E402

# Ray Dream world (Z up, Blupi facing +Y) -> glTF (Y up, asset facing +Z).
RD_TO_GLTF = np.array([[-1.0, 0.0, 0.0], [0.0, 0.0, 1.0], [0.0, 1.0, 0.0]])
# Mirror applied in a joint's local space to keep every joint rotation proper.
MIRROR = np.diag([-1.0, 1.0, 1.0])


def matrix_to_quat_xyzw(m: np.ndarray) -> np.ndarray:
    t = np.trace(m)
    if t > 0:
        s = np.sqrt(t + 1.0) * 2
        q = [(m[2, 1] - m[1, 2]) / s, (m[0, 2] - m[2, 0]) / s, (m[1, 0] - m[0, 1]) / s, 0.25 * s]
    elif m[0, 0] > m[1, 1] and m[0, 0] > m[2, 2]:
        s = np.sqrt(1.0 + m[0, 0] - m[1, 1] - m[2, 2]) * 2
        q = [0.25 * s, (m[0, 1] + m[1, 0]) / s, (m[0, 2] + m[2, 0]) / s, (m[2, 1] - m[1, 2]) / s]
    elif m[1, 1] > m[2, 2]:
        s = np.sqrt(1.0 + m[1, 1] - m[0, 0] - m[2, 2]) * 2
        q = [(m[0, 1] + m[1, 0]) / s, 0.25 * s, (m[1, 2] + m[2, 1]) / s, (m[0, 2] - m[2, 0]) / s]
    else:
        s = np.sqrt(1.0 + m[2, 2] - m[0, 0] - m[1, 1]) * 2
        q = [(m[0, 2] + m[2, 0]) / s, (m[1, 2] + m[2, 1]) / s, 0.25 * s, (m[1, 0] - m[0, 1]) / s]
    q = np.array(q)
    return q / np.linalg.norm(q)


class Converter:
    def __init__(self, scene: sc.Scene, root_name: str, scale: float, warnings: list[str]):
        self.scene = scene
        self.canonical: dict[str, tuple] = {}
        self.scale = scale
        self.warnings = warnings
        self.offset = np.zeros(3)
        root = scene.find(root_name)
        if root is None:
            raise SystemExit(f"no object named {root_name!r} in the scene")
        self.root = root
        # Joints in depth-first order; names made unique Blender-style.
        self.joints: list[sc.Obj] = []
        self.parent: dict[int, int] = {}
        self.names: list[str] = []
        seen: dict[str, int] = {}

        def rec(o: sc.Obj, parent_index: int):
            idx = len(self.joints)
            self.joints.append(o)
            n = seen.get(o.name, 0)
            seen[o.name] = n + 1
            self.names.append(o.name if n == 0 else f"{o.name}.{n:03d}")
            self.parent[idx] = parent_index
            for c in o.children:
                if c.kind in ("ca", "li"):
                    continue
                rec(c, idx)

        rec(root, -1)
        self.ancestors = _path_to(scene.universe, root)[:-1]

    # ---- frames -------------------------------------------------------------------
    def above_root(self, t: float | None) -> np.ndarray:
        """The chain above the root (normally just the universe) is folded into the root joint."""
        m = np.eye(4)
        for o in self.ancestors:
            m = m @ sc.child_frame(o, t)
        return m

    def rd_frames(self, t: float | None) -> list[tuple[np.ndarray, np.ndarray]]:
        """(child-space-to-world, geometry-to-world) per joint, in Ray Dream world units."""
        out = []
        base = self.above_root(t)
        worlds: list[np.ndarray] = []
        for i, o in enumerate(self.joints):
            p = self.parent[i]
            parent_world = base if p < 0 else worlds[p]
            cw = parent_world @ sc.child_frame(o, t)
            gw = parent_world @ sc.geometry_frame(o, t)
            worlds.append(cw)
            out.append((cw, gw))
        return out

    def joint_world(self, cw: np.ndarray) -> tuple[np.ndarray, bool]:
        """Proper rigid glTF-space joint matrix for a Ray Dream child frame, and whether it was mirrored."""
        lin = cw[:3, :3]
        mirrored = np.linalg.det(lin) < 0
        if mirrored:
            lin = lin @ MIRROR
        m = np.eye(4)
        m[:3, :3] = RD_TO_GLTF @ lin
        m[:3, 3] = RD_TO_GLTF @ (cw[:3, 3] * self.scale) + self.offset
        return m, mirrored

    # ---- geometry -----------------------------------------------------------------
    def build_vertices(self, t_bind: float):
        """Bind-pose geometry grouped by material: {material key: [(pos, nrm, tris, joint, uv)]}.

        A textured part gets its own material key ("texture", object name); its TextureMap is
        kept in ``self.textures``.
        """
        frames = self.rd_frames(t_bind)
        groups: dict[tuple, list] = {}
        self.textures: dict[tuple, shading.TextureMap] = {}
        for i, o in enumerate(self.joints):
            if not o.master:
                continue
            if o.master not in self.canonical:
                try:
                    self.canonical[o.master] = geometry.canonical_mesh(self.scene.masters[o.master])
                except NotImplementedError as e:
                    self.canonical[o.master] = None
                    self.warnings.append(f"{o.name}: {e}; skipped")
            if self.canonical[o.master] is None:
                continue
            mesh, ff = self.canonical[o.master]
            cw, gw = frames[i]
            # Canonical -> RD world -> glTF space.
            m = np.eye(4)
            m[:3, :3] = RD_TO_GLTF @ gw[:3, :3] * self.scale
            m[:3, 3] = RD_TO_GLTF @ (gw[:3, 3] * self.scale) + self.offset
            pos, nrm, tris, src = geometry.smooth_normals(mesh, return_index=True)
            uv = None
            key = shading.material_key(self.scene, o)
            try:
                tex = shading.texture_map(self.scene, o)
            except NotImplementedError as e:
                tex = None
                self.warnings.append(f"{o.name}: texture not converted ({e}); flat colour used")
            if tex is not None and ff is None:
                self.warnings.append(f"{o.name}: texture map on a primitive not converted")
            elif tex is not None:
                self.warnings.extend(tex.warnings)
                st = freeform_mesh.texture_coordinates(ff, tex.quarter_turns)[src]
                pos, nrm, tris, uv = freeform_mesh.split_seam(pos, nrm, tris, st, 1.0)
                key = ("texture", o.name)
                self.textures[key] = tex
            wpos = pos @ m[:3, :3].T + m[:3, 3]
            nm = np.linalg.inv(m[:3, :3]).T
            wn = nrm @ nm.T
            wn /= np.linalg.norm(wn, axis=1, keepdims=True)
            if np.linalg.det(m[:3, :3]) < 0:
                tris = tris[:, ::-1]
            groups.setdefault(key, []).append((wpos, wn, tris, i, uv))
        return groups

    def ground_offset(self, t_bind: float):
        groups = self.build_vertices(t_bind)
        lowest = min(float(item[0][:, 1].min()) for g in groups.values() for item in g)
        self.offset = np.array([0.0, -lowest, 0.0])

    # ---- animation ----------------------------------------------------------------
    def check_supported_animation(self):
        for o in self.joints:
            if o.hotp_track is not None and _varies(o.hotp_track):
                self.warnings.append(f"{o.name}: animated hot point ignored")
            if o.scal_track is not None and _varies(o.scal_track):
                self.warnings.append(f"{o.name}: animated scale ignored")
            if any(tr is not None and _varies(tr) for tr in o.affi_tracks):
                self.warnings.append(f"{o.name}: animated deformer ignored")

    def local_matrices(self, t: float) -> list[np.ndarray]:
        frames = self.rd_frames(t)
        worlds = [self.joint_world(cw)[0] for cw, _gw in frames]
        out = []
        for i in range(len(self.joints)):
            p = self.parent[i]
            out.append(worlds[i] if p < 0 else np.linalg.inv(worlds[p]) @ worlds[i])
        return out

    # ---- output -------------------------------------------------------------------
    @staticmethod
    def _add_image(b: gltf.GltfBuilder, tex: shading.TextureMap) -> int:
        """Embed the baked texture as PNG; returns the glTF texture index."""
        buf = io.BytesIO()
        Image.fromarray(tex.image).save(buf, format="PNG")
        view = b.add_blob(buf.getvalue())
        image = b.add("images", {"bufferView": view, "mimeType": "image/png"})
        # u wraps around the body; v ends at the caps. Ray Dream samples without interpolation
        # (intp 0), but a game camera gets much closer than the 60-pixel sprites, so filter.
        sampler = b.add("samplers", {"magFilter": 9729, "minFilter": 9987,
                                     "wrapS": 10497, "wrapT": 33071})
        return b.add("textures", {"source": image, "sampler": sampler})

    def write(self, path: Path, clips: list["Clip"], sample_fps: float, t_bind: float, extras: dict,
              frame_rate: float | None = None):
        self.check_supported_animation()
        self.ground_offset(t_bind)
        b = gltf.GltfBuilder("galaxy-eggbert tools/rd4_to_gltf")
        bind_frames = self.rd_frames(t_bind)
        bind_world = [self.joint_world(cw)[0] for cw, _gw in bind_frames]
        bind_local = self.local_matrices(t_bind)

        # Joint nodes.
        node_of = []
        for i, name in enumerate(self.names):
            m = bind_local[i]
            node = {"name": name, "translation": [float(x) for x in m[:3, 3]],
                    "rotation": [float(x) for x in matrix_to_quat_xyzw(m[:3, :3])]}
            node_of.append(b.add("nodes", node))
        for i in range(len(self.joints)):
            p = self.parent[i]
            if p >= 0:
                b.doc["nodes"][node_of[p]].setdefault("children", []).append(node_of[i])

        # One skinned mesh per material, each named after its material: CNA's convert_avatar.py
        # names each part after its mesh and keeps no material colour (upstream_bugs.md UB-001,
        # UB-002), so the name is the only place the converted part's colour survives.
        skin = b.add("skins", {"name": self.root.name, "joints": node_of, "skeleton": node_of[0]})
        groups = self.build_vertices(t_bind)
        materials = {}
        mesh_nodes = []
        for key, items in groups.items():
            if key not in materials:
                if key in self.textures:
                    material = shading.textured_material_json(key[1], self._add_image(b, self.textures[key]))
                else:
                    material = shading.material_json(key)
                materials[key] = b.add("materials", material)
            pos = np.concatenate([w for w, _n, _t, _i, _uv in items])
            nrm = np.concatenate([n for _w, n, _t, _i, _uv in items])
            tris, joints, base = [], [], 0
            for w, _n, t, i, _uv in items:
                tris.append(t + base)
                joints.append(np.full(len(w), i))
                base += len(w)
            tris = np.concatenate(tris).astype(np.uint32)
            j = np.zeros((len(pos), 4), dtype=np.uint8)
            j[:, 0] = np.concatenate(joints)
            wts = np.zeros((len(pos), 4), dtype=np.float32)
            wts[:, 0] = 1.0
            primitive = {
                "attributes": {
                    "POSITION": b.accessor(pos, gltf.FLOAT, gltf.ARRAY_BUFFER, minmax=True),
                    "NORMAL": b.accessor(nrm, gltf.FLOAT, gltf.ARRAY_BUFFER),
                    "JOINTS_0": b.accessor(j, gltf.UBYTE, gltf.ARRAY_BUFFER),
                    "WEIGHTS_0": b.accessor(wts, gltf.FLOAT, gltf.ARRAY_BUFFER),
                },
                "indices": b.accessor(tris.ravel(), gltf.UINT, gltf.ELEMENT_ARRAY_BUFFER),
                "material": materials[key],
            }
            if key in self.textures:
                uv = np.concatenate([u for *_rest, u in items])
                primitive["attributes"]["TEXCOORD_0"] = b.accessor(uv, gltf.FLOAT, gltf.ARRAY_BUFFER)
            name = f"{self.root.name}_{b.doc['materials'][materials[key]]['name']}"
            mesh = b.add("meshes", {"name": name, "primitives": [primitive]})
            mesh_nodes.append(b.add("nodes", {"name": name, "mesh": mesh, "skin": skin}))
        ibm = np.array([np.linalg.inv(m).T for m in bind_world], dtype=np.float32)  # column-major
        b.doc["skins"][skin]["inverseBindMatrices"] = b.accessor(ibm.reshape(-1, 16), gltf.FLOAT)

        # Animation clips: each samples its own scene's keys onto this skeleton (joints matched by
        # name) with this scene's ground offset, so clips from scenes that place Blupi differently
        # keep that difference, as the sprites do.
        for clip in clips:
            clip.write_animation(b, self, node_of, sample_fps, frame_rate)

        b.doc["scenes"] = [{"name": self.root.name, "nodes": [node_of[0], *mesh_nodes]}]
        b.doc["scene"] = 0
        extras = dict(extras)
        extras.update({"scale": self.scale, "offset": [float(x) for x in self.offset],
                       "rd_to_gltf": RD_TO_GLTF.tolist(), "bind_tick": t_bind,
                       "clip_ticks": [self.scene.begin, self.scene.end], "scene_fps": self.scene.fps,
                       "frame_rate": frame_rate,
                       "clips": [{"name": c.name, "scene": c.label, "ticks": [c.t0, c.t1]} for c in clips]})
        b.doc["asset"]["extras"] = {"rd4_to_gltf": extras}
        b.write(path)
        return len(self.joints), len(mesh_nodes)


class Clip:
    """One animation: a scene's ticks [t0, t1] (one tick for a static pose) on the main skeleton."""

    def __init__(self, name: str, conv: "Converter", label: str, t0: float, t1: float):
        self.name, self.conv, self.label, self.t0, self.t1 = name, conv, label, t0, t1

    def write_animation(self, b: gltf.GltfBuilder, main: "Converter", node_of: list[int],
                        sample_fps: float, frame_rate: float | None):
        conv = self.conv
        conv.offset = main.offset
        index = {n: i for i, n in enumerate(conv.names)}
        missing = [n for n in main.names if n not in index]
        if missing:
            main.warnings.append(f"clip {self.name!r}: joints {missing} not in {self.label}; bind pose kept")
        # Seconds per scene tick: real time, or one rendered frame per 1/frame_rate seconds.
        per_tick = 1.0 / sc.TICKS_PER_SECOND
        if frame_rate:
            per_tick = conv.scene.fps / (sc.TICKS_PER_SECOND * frame_rate)
        if self.t1 > self.t0:
            n = max(2, int(round((self.t1 - self.t0) * per_tick * sample_fps)) + 1)
            ticks = np.linspace(self.t0, self.t1, n)
        else:
            ticks = np.array([self.t0, self.t0])
        seconds = (ticks - self.t0) * per_tick
        if self.t1 <= self.t0:
            seconds = np.array([0.0, 1.0 / (frame_rate or 1.0)])
        locals_ = [conv.local_matrices(float(t)) for t in ticks]
        time_acc = b.accessor(seconds.astype(np.float32), gltf.FLOAT, minmax=True)
        samplers, channels = [], []
        for i, name in enumerate(main.names):
            if name not in index:
                continue
            k = index[name]
            trs = np.array([m[k][:3, 3] for m in locals_], dtype=np.float32)
            quats = []
            for m in locals_:
                q = matrix_to_quat_xyzw(m[k][:3, :3])
                if quats and q @ quats[-1] < 0:
                    q = -q
                quats.append(q)
            # Every joint gets both channels: a clip from another scene must not inherit the
            # main scene's bind pose for joints it holds still.
            for path_name, data in (("translation", trs), ("rotation", np.array(quats, dtype=np.float32))):
                s_idx = len(samplers)
                samplers.append({"input": time_acc, "output": b.accessor(data, gltf.FLOAT),
                                 "interpolation": "LINEAR"})
                channels.append({"sampler": s_idx, "target": {"node": node_of[i], "path": path_name}})
        b.add("animations", {"name": self.name, "samplers": samplers, "channels": channels})


def _path_to(node: sc.Obj, target: sc.Obj) -> list[sc.Obj]:
    if node is target:
        return [node]
    for c in node.children:
        p = _path_to(c, target)
        if p:
            return [node] + p
    return []


def _varies(track: sc.Track) -> bool:
    return any(not np.allclose(v, track.values[0]) for v in track.values)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--scene", required=True, help="animated .rd4/.rds scene to convert")
    ap.add_argument("--reference-scene", help="scene the reference DXF was exported from (check only)")
    ap.add_argument("--reference-dxf", help="Ray Dream DXF export of the reference scene (check only)")
    ap.add_argument("--root", default="BLUPI", help="object whose subtree becomes the skeleton")
    ap.add_argument("--clip", action="append", default=[], metavar="NAME=SCENE[@FRAME]",
                    help="animation from a scene (repeatable); @FRAME exports that rendered frame as a "
                         "static pose. Default: the --scene's own animation, named after the file")
    ap.add_argument("--frame-rate", type=float,
                    help="play one rendered scene frame per 1/FRAME_RATE s (the game's sprite cadence) "
                         "instead of the scene's real time")
    ap.add_argument("--scale", type=float, default=0.05, help="glTF metres per Ray Dream unit")
    ap.add_argument("--sample-fps", type=float, default=30.0, help="animation sampling rate")
    ap.add_argument("--out", required=True, help="output .glb")
    args = ap.parse_args(argv)

    warnings: list[str] = []
    if bool(args.reference_scene) != bool(args.reference_dxf):
        ap.error("--reference-scene and --reference-dxf go together")
    if args.reference_dxf:
        ref = sc.read_scene(rd4.parse_file(args.reference_scene))
        print("Generated geometry against Ray Dream's DXF:")
        for line in geometry.compare_with_reference(ref, args.reference_dxf):
            print("  " + line)
    target = sc.read_scene(rd4.parse_file(args.scene))
    conv = Converter(target, args.root, args.scale, warnings)
    clips = []
    for spec in args.clip or [f"{Path(args.scene).stem}={args.scene}"]:
        name, _, rest = spec.partition("=")
        path, _, frame = rest.partition("@")
        if not name or not path:
            ap.error(f"--clip {spec!r}: expected NAME=SCENE[@FRAME]")
        cs = target if Path(path).resolve() == Path(args.scene).resolve() else sc.read_scene(rd4.parse_file(path))
        cconv = conv if cs is target else Converter(cs, args.root, args.scale, warnings)
        if frame:
            tick = cs.begin + int(frame) * sc.TICKS_PER_SECOND / cs.fps
            clips.append(Clip(name, cconv, Path(path).name, tick, tick))
        else:
            clips.append(Clip(name, cconv, Path(path).name, cs.begin, cs.end))
    joints, meshes = conv.write(Path(args.out), clips, args.sample_fps, target.begin,
                               {"scene": Path(args.scene).name}, args.frame_rate)
    print(f"Wrote {args.out}: {joints} joints, {meshes} skinned meshes, clips "
          + ", ".join(f"{c.name} ({c.label}, ticks {c.t0:g}-{c.t1:g})" for c in clips))
    for w in dict.fromkeys(warnings):
        print("WARNING: " + w)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
