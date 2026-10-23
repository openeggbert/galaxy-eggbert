#!/usr/bin/env python3
"""Convert an animated Ray Dream scene (Speedy Blupi originals) into a skinned glTF binary.

Every scene object that references a master becomes one rigidly skinned part on its own joint;
groups become joints without geometry. The scene's own animation is sampled into one clip.

    python3 rd4_to_gltf.py --scene marche-profil.rd4 --reference-scene stop-profil.rd4 \\
        --reference-dxf blupi.dxf --out blupi-marche.glb

Geometry comes from the reference DXF (see geometry.py); joint positions, rotations and keys
come from the scene. See README.md for the verified conventions.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))

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
    def __init__(self, scene: sc.Scene, parts: dict[str, geometry.Part], root_name: str,
                 scale: float, warnings: list[str]):
        self.scene = scene
        self.parts = parts
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
        """Bind-pose geometry grouped by material: {material key: (pos, nrm, tris, joints)}."""
        frames = self.rd_frames(t_bind)
        groups: dict[tuple, list] = {}
        for i, o in enumerate(self.joints):
            if not o.master:
                continue
            part = self.parts.get(o.name)
            if part is None:
                self.warnings.append(f"{o.name}: no reference geometry, skipped")
                continue
            fp = geometry.master_fingerprint(self.scene.masters[o.master])
            if fp != part.fingerprint:
                self.warnings.append(f"{o.name}: master {o.master!r} [{fp}] differs from reference "
                                     f"{part.source} [{part.fingerprint}]; using reference shape")
            cw, gw = frames[i]
            # Canonical -> RD world -> glTF space.
            m = np.eye(4)
            m[:3, :3] = RD_TO_GLTF @ gw[:3, :3] * self.scale
            m[:3, 3] = RD_TO_GLTF @ (gw[:3, 3] * self.scale) + self.offset
            pos, nrm, tris = geometry.smooth_normals(part.mesh)
            wpos = pos @ m[:3, :3].T + m[:3, 3]
            nm = np.linalg.inv(m[:3, :3]).T
            wn = nrm @ nm.T
            wn /= np.linalg.norm(wn, axis=1, keepdims=True)
            if np.linalg.det(m[:3, :3]) < 0:
                tris = tris[:, ::-1]
            key = shading.material_key(self.scene, o)
            groups.setdefault(key, []).append((wpos, wn, tris, i))
        return groups

    def ground_offset(self, t_bind: float):
        groups = self.build_vertices(t_bind)
        lowest = min(float(w[:, 1].min()) for g in groups.values() for w, _n, _t, _i in g)
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
    def write(self, path: Path, clip_name: str, sample_fps: float, t_bind: float, extras: dict):
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
                materials[key] = b.add("materials", shading.material_json(key))
            pos = np.concatenate([w for w, _n, _t, _i in items])
            nrm = np.concatenate([n for _w, n, _t, _i in items])
            tris, joints, base = [], [], 0
            for w, _n, t, i in items:
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
            name = f"{self.root.name}_{b.doc['materials'][materials[key]]['name']}"
            mesh = b.add("meshes", {"name": name, "primitives": [primitive]})
            mesh_nodes.append(b.add("nodes", {"name": name, "mesh": mesh, "skin": skin}))
        ibm = np.array([np.linalg.inv(m).T for m in bind_world], dtype=np.float32)  # column-major
        b.doc["skins"][skin]["inverseBindMatrices"] = b.accessor(ibm.reshape(-1, 16), gltf.FLOAT)

        # Animation clip sampled from the scene's own keys.
        t0, t1 = self.scene.begin, self.scene.end
        if t1 > t0:
            n = int(round((t1 - t0) / sc.TICKS_PER_SECOND * sample_fps)) + 1
            ticks = np.linspace(t0, t1, n)
            seconds = (ticks - t0) / sc.TICKS_PER_SECOND
            locals_ = [self.local_matrices(float(t)) for t in ticks]
            time_acc = b.accessor(seconds.astype(np.float32), gltf.FLOAT, minmax=True)
            samplers, channels = [], []
            for i in range(len(self.joints)):
                trs = np.array([m[i][:3, 3] for m in locals_], dtype=np.float32)
                quats = []
                for m in locals_:
                    q = matrix_to_quat_xyzw(m[i][:3, :3])
                    if quats and q @ quats[-1] < 0:
                        q = -q
                    quats.append(q)
                quats = np.array(quats, dtype=np.float32)
                for path_name, data in (("translation", trs), ("rotation", quats)):
                    if np.allclose(data, data[0], atol=1e-6):
                        continue
                    s_idx = len(samplers)
                    samplers.append({"input": time_acc, "output": b.accessor(data, gltf.FLOAT),
                                     "interpolation": "LINEAR"})
                    channels.append({"sampler": s_idx, "target": {"node": node_of[i], "path": path_name}})
            b.add("animations", {"name": clip_name, "samplers": samplers, "channels": channels})

        b.doc["scenes"] = [{"name": self.root.name, "nodes": [node_of[0], *mesh_nodes]}]
        b.doc["scene"] = 0
        extras = dict(extras)
        extras.update({"scale": self.scale, "offset": [float(x) for x in self.offset],
                       "rd_to_gltf": RD_TO_GLTF.tolist(), "bind_tick": t_bind,
                       "clip_ticks": [t0, t1], "scene_fps": self.scene.fps})
        b.doc["asset"]["extras"] = {"rd4_to_gltf": extras}
        b.write(path)
        return len(self.joints), len(mesh_nodes)


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
    ap.add_argument("--reference-scene", required=True, help="scene the reference DXF was exported from")
    ap.add_argument("--reference-dxf", required=True, help="Ray Dream DXF export of the reference scene")
    ap.add_argument("--root", default="BLUPI", help="object whose subtree becomes the skeleton")
    ap.add_argument("--clip", help="animation name (default: scene file stem)")
    ap.add_argument("--scale", type=float, default=0.05, help="glTF metres per Ray Dream unit")
    ap.add_argument("--sample-fps", type=float, default=30.0, help="animation sampling rate")
    ap.add_argument("--out", required=True, help="output .glb")
    args = ap.parse_args(argv)

    warnings: list[str] = []
    ref = sc.read_scene(rd4.parse_file(args.reference_scene))
    parts, report = geometry.recover_parts(ref, args.reference_dxf)
    print("Reference geometry:")
    for line in report:
        print("  " + line)
    target = sc.read_scene(rd4.parse_file(args.scene))
    conv = Converter(target, parts, args.root, args.scale, warnings)
    clip = args.clip or Path(args.scene).stem
    joints, meshes = conv.write(Path(args.out), clip, args.sample_fps, target.begin,
                               {"scene": Path(args.scene).name,
                                "reference_scene": Path(args.reference_scene).name,
                                "reference_dxf": Path(args.reference_dxf).name})
    print(f"Wrote {args.out}: {joints} joints, {meshes} skinned meshes, clip {clip!r} "
          f"{(target.end - target.begin) / sc.TICKS_PER_SECOND:.3f} s "
          f"(scene renders at {target.fps:g} fps)")
    for w in dict.fromkeys(warnings):
        print("WARNING: " + w)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
