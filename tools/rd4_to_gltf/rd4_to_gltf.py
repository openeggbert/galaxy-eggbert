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
        self.canonical: dict[tuple, tuple] = {}
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
        # Joints appended from other scenes (props, add_joint): their scene and the tick they are
        # posed at, relative to their parent, in the bind pose.
        self.scene_of: dict[int, sc.Scene] = {}
        self.tick_of: dict[int, float] = {}
        self.optional: set[str] = set()
        # Extra Ray Dream world translation per tick, e.g. to line this scene's game camera up with
        # another scene's (build_clip_library.py).
        self.world_shift = lambda t: np.zeros(3)
        self.scene_label = "scene"

    def add_joint(self, o: sc.Obj, name: str, parent_index: int, scene: sc.Scene, tick: float) -> int:
        """Append a joint from another scene (a prop) under ``parent_index``, posed relative to it
        as in its own scene at ``tick``. A clip whose scene lacks it hides it, without a warning."""
        i = len(self.joints)
        self.joints.append(o)
        self.names.append(name)
        self.parent[i] = parent_index
        self.scene_of[i] = scene
        self.tick_of[i] = tick
        self.optional.add(name)
        return i

    # ---- frames -------------------------------------------------------------------
    def above_root(self, t: float | None) -> np.ndarray:
        """The chain above the root (normally just the universe) is folded into the root joint."""
        m = np.eye(4)
        for o in self.ancestors:
            m = m @ sc.child_frame(o, t)
        return m

    def rd_frames(self, t: float | None, view=None) -> list[tuple[np.ndarray, np.ndarray]]:
        """(child-space-to-world, geometry-to-world) per joint, in Ray Dream world units; ``view``
        turns them about Blupi's body (scene.view_matrix)."""
        out = []
        base = self.above_root(t)
        shift = self.world_shift(t)
        if shift.any():
            base = sc.affine(np.eye(3), shift) @ base
        worlds: list[np.ndarray] = []
        for i, o in enumerate(self.joints):
            p = self.parent[i]
            parent_world = base if p < 0 else worlds[p]
            tt = self.tick_of.get(i, t)
            cw = parent_world @ sc.child_frame(o, tt)
            gw = parent_world @ sc.geometry_frame(o, tt)
            worlds.append(cw)
            out.append((cw, gw))
        if view is not None:
            turn = sc.view_matrix(out[self.names.index("Corps")][0][:3, 3], view)
            out = [(turn @ cw, turn @ gw) for cw, gw in out]
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

    def shape_world(self, gw: np.ndarray) -> np.ndarray:
        """glTF-space matrix placing a part's canonical geometry (Ray Dream geometry frame: scale,
        deformer, hot point), kept proper like the joints by moving a mirror into the geometry."""
        m = np.eye(4)
        m[:3, :3] = RD_TO_GLTF @ gw[:3, :3] * self.scale
        m[:3, 3] = RD_TO_GLTF @ (gw[:3, 3] * self.scale) + self.offset
        if np.linalg.det(m[:3, :3]) < 0:
            m[:3, :3] = m[:3, :3] @ MIRROR
        return m

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
            part = part_geometry(self.scene_of.get(i, self.scene), o, self.names[i], frames[i][1],
                                 self.scale, self.offset, self.canonical, self.warnings)
            if part is None:
                continue
            key, tex, wpos, wn, tris, uv = part
            if tex is not None:
                self.textures[key] = tex
            groups.setdefault(key, []).append((wpos, wn, tris, i, uv))
        return groups

    def ground_offset(self, t_bind: float):
        groups = self.build_vertices(t_bind)
        # Props taken from other scenes (add_joint) hang wherever their bind pose puts them.
        lowest = min(float(item[0][:, 1].min()) for g in groups.values() for item in g
                     if item[3] not in self.scene_of)
        self.offset = np.array([0.0, -lowest, 0.0])

    # ---- animation ----------------------------------------------------------------
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
        self.ground_offset(t_bind)
        b = gltf.GltfBuilder("galaxy-eggbert tools/rd4_to_gltf")
        bind_frames = self.rd_frames(t_bind)
        bind_world = [self.joint_world(cw)[0] for cw, _gw in bind_frames]
        bind_local = self.local_matrices(t_bind)
        groups = self.build_vertices(t_bind)
        with_geometry = sorted({item[3] for items in groups.values() for item in items})
        bind_shape = {i: np.linalg.inv(bind_world[i]) @ self.shape_world(bind_frames[i][1]) for i in with_geometry}

        # Every clip's poses on this skeleton. A part whose geometry frame (scale, deformer, hot
        # point) changes in some pose, or which some pose's scene does not have, gets a shape
        # joint: a leaf child of its joint carrying that frame, with the part skinned to it.
        animations = []
        for clip in clips:
            poses = [(pose[0], *pose_on(self, pose[1], pose[2], bind_local, clip.name, *pose[3:]))
                     for pose in clip.poses(self, sample_fps, frame_rate)]
            animations.append((clip.name, poses))
        shaped = [i for i in with_geometry
                  if any(shapes[i] is None or not np.allclose(shapes[i], bind_shape[i], atol=1e-6)
                         for _n, poses in animations for _s, _j, shapes in poses)]

        # Joint nodes.
        node_of = []
        for i, name in enumerate(self.names):
            node_of.append(b.add("nodes", trs_node(name, bind_local[i])))
        for i in range(len(self.joints)):
            p = self.parent[i]
            if p >= 0:
                b.doc["nodes"][node_of[p]].setdefault("children", []).append(node_of[i])
        shape_node_of = {}
        for i in shaped:
            shape_node_of[i] = b.add("nodes", trs_node(f"{self.names[i]}.shape", bind_shape[i], scale=True))
            b.doc["nodes"][node_of[i]].setdefault("children", []).append(shape_node_of[i])
        skin_index = {i: i for i in range(len(self.joints))}
        skin_index.update({i: len(self.joints) + k for k, i in enumerate(shaped)})

        # One skinned mesh per material, each named after its material: CNA's convert_avatar.py
        # names each part after its mesh and keeps no material colour (upstream_bugs.md UB-001,
        # UB-002), so the name is the only place the converted part's colour survives.
        skin = b.add("skins", {"name": self.root.name, "joints": node_of + [shape_node_of[i] for i in shaped],
                               "skeleton": node_of[0]})
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
                joints.append(np.full(len(w), skin_index[i]))
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
        binds = bind_world + [bind_world[i] @ bind_shape[i] for i in shaped]
        ibm = np.array([np.linalg.inv(m).T for m in binds], dtype=np.float32)  # column-major
        b.doc["skins"][skin]["inverseBindMatrices"] = b.accessor(ibm.reshape(-1, 16), gltf.FLOAT)

        # Animation clips: each samples its own scene's keys onto this skeleton (joints matched by
        # name) with this scene's ground offset, so clips from scenes that place Blupi differently
        # keep that difference, as the sprites do.
        for name, poses in animations:
            write_animation(b, name, node_of, shape_node_of, bind_shape, poses)

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
        return len(self.joints) + len(shaped), len(mesh_nodes)


def part_geometry(scene: sc.Scene, o: sc.Obj, label: str, gw: np.ndarray, scale: float, offset: np.ndarray,
                  canonical: dict, warnings: list[str]):
    """One object's master in glTF space, placed by its Ray Dream geometry frame ``gw``:
    (material key, TextureMap or None, positions, normals, triangles, texture coordinates or None),
    or None when it has no supported master. A textured part's key is ("texture", label)."""
    if not o.master:
        return None
    master = (id(scene), o.master)
    if master not in canonical:
        try:
            canonical[master] = geometry.canonical_mesh(scene.masters[o.master])
        except NotImplementedError as e:
            canonical[master] = None
            warnings.append(f"{label}: {e}; skipped")
    if canonical[master] is None:
        return None
    mesh, ff = canonical[master]
    # Canonical -> RD world -> glTF space.
    m = np.eye(4)
    m[:3, :3] = RD_TO_GLTF @ gw[:3, :3] * scale
    m[:3, 3] = RD_TO_GLTF @ (gw[:3, 3] * scale) + offset
    pos, nrm, tris, src = geometry.smooth_normals(mesh, return_index=True)
    uv = None
    key = shading.material_key(scene, o)
    try:
        tex = shading.texture_map(scene, o)
    except NotImplementedError as e:
        tex = None
        warnings.append(f"{label}: texture not converted ({e}); flat colour used")
    if tex is not None and ff is None:
        warnings.append(f"{label}: texture map on a primitive not converted")
        tex = None
    elif tex is not None:
        warnings.extend(tex.warnings)
        st = freeform_mesh.texture_coordinates(ff, tex.quarter_turns)[src]
        pos, nrm, tris, uv = freeform_mesh.split_seam(pos, nrm, tris, st, 1.0)
        key = ("texture", label)
    wpos = pos @ m[:3, :3].T + m[:3, 3]
    nm = np.linalg.inv(m[:3, :3]).T
    wn = nrm @ nm.T
    # Degenerate faces (a loop collapsed to a point) have no normal; keep them finite.
    wn /= np.maximum(np.linalg.norm(wn, axis=1, keepdims=True), 1e-12)
    if np.linalg.det(m[:3, :3]) < 0:
        tris = tris[:, ::-1]
    return key, tex, wpos, wn, tris, uv


def write_parts_model(path: Path, root_name: str, parts: list, clips: list, scale: float, offset: np.ndarray,
                      warnings: list[str], extras: dict) -> tuple[int, int]:
    """A model of rigid parts that are not Blupi (a vehicle): a root joint and under it one joint
    per part carrying the part's whole geometry frame (Ray Dream scale and deformer included, as
    on a shape joint). ``parts`` is [(name, scene, obj, bind frame gw)], ``clips`` is
    [(clip name, [(seconds, {part name: gw})])] in Ray Dream world units; a part missing from a
    pose is hidden there. Built in the same glTF space as Blupi's model (``scale``, ``offset``),
    so the game draws both with the same transform and clip time."""
    def shape(gw):
        m = np.eye(4)
        m[:3, :3] = RD_TO_GLTF @ gw[:3, :3] * scale
        m[:3, 3] = RD_TO_GLTF @ (gw[:3, 3] * scale) + offset
        if np.linalg.det(m[:3, :3]) < 0:
            m[:3, :3] = m[:3, :3] @ MIRROR
        return m

    b = gltf.GltfBuilder("galaxy-eggbert tools/rd4_to_gltf")
    root = b.add("nodes", {"name": root_name})
    node_of, bind = [], []
    for name, _scene, _o, gw in parts:
        bind.append(shape(gw))
        node_of.append(b.add("nodes", trs_node(name, bind[-1], scale=True)))
    b.doc["nodes"][root]["children"] = list(node_of)
    skin = b.add("skins", {"name": root_name, "joints": [root] + node_of, "skeleton": root})
    canonical: dict = {}
    groups: dict[tuple, list] = {}
    textures: dict[tuple, shading.TextureMap] = {}
    for k, (name, scene, o, gw) in enumerate(parts):
        part = part_geometry(scene, o, name, gw, scale, offset, canonical, warnings)
        if part is None:
            continue
        key, tex, wpos, wn, tris, uv = part
        if tex is not None:
            textures[key] = tex
        groups.setdefault(key, []).append((wpos, wn, tris, k + 1, uv))
    mesh_nodes = []
    materials = {}
    for key, items in groups.items():
        if key in textures:
            materials[key] = b.add("materials", shading.textured_material_json(
                key[1], Converter._add_image(b, textures[key])))
        else:
            materials[key] = b.add("materials", shading.material_json(key))
        pos = np.concatenate([w for w, _n, _t, _i, _uv in items])
        nrm = np.concatenate([n for _w, n, _t, _i, _uv in items])
        tris, joints, base = [], [], 0
        for w, _n, t, i, _uv in items:
            tris.append(t + base)
            joints.append(np.full(len(w), i))
            base += len(w)
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
            "indices": b.accessor(np.concatenate(tris).astype(np.uint32).ravel(), gltf.UINT,
                                  gltf.ELEMENT_ARRAY_BUFFER),
            "material": materials[key],
        }
        if key in textures:
            primitive["attributes"]["TEXCOORD_0"] = b.accessor(
                np.concatenate([u for *_rest, u in items]), gltf.FLOAT, gltf.ARRAY_BUFFER)
        mesh_name = f"{root_name}_{b.doc['materials'][materials[key]]['name']}"
        mesh = b.add("meshes", {"name": mesh_name, "primitives": [primitive]})
        mesh_nodes.append(b.add("nodes", {"name": mesh_name, "mesh": mesh, "skin": skin}))
    ibm = np.array([np.eye(4)] + [np.linalg.inv(m).T for m in bind], dtype=np.float32)  # column-major
    b.doc["skins"][skin]["inverseBindMatrices"] = b.accessor(ibm.reshape(-1, 16), gltf.FLOAT)

    hidden = np.diag([HIDDEN_SCALE] * 3 + [1.0])
    for clip_name, poses in clips:
        seconds = np.array([sec for sec, _p in poses], dtype=np.float32)
        time_acc = b.accessor(seconds, gltf.FLOAT, minmax=True)
        samplers, channels = [], []
        for k, (name, *_rest) in enumerate(parts):
            mats = [shape(pp[name]) if name in pp else bind[k] @ hidden for _sec, pp in poses]
            dec = [decompose(m) for m in mats]
            quats = []
            for _t, q, _s in dec:
                if quats and q @ quats[-1] < 0:
                    q = -q
                quats.append(q)
            for path_name, values in (("translation", [t for t, _q, _s in dec]), ("rotation", quats),
                                      ("scale", [s_ for _t, _q, s_ in dec])):
                samplers.append({"input": time_acc, "output": b.accessor(np.array(values, dtype=np.float32),
                                                                        gltf.FLOAT), "interpolation": "LINEAR"})
                channels.append({"sampler": len(samplers) - 1, "target": {"node": node_of[k], "path": path_name}})
        b.add("animations", {"name": clip_name, "samplers": samplers, "channels": channels})

    b.doc["scenes"] = [{"name": root_name, "nodes": [root, *mesh_nodes]}]
    b.doc["scene"] = 0
    extras = dict(extras)
    extras.update({"scale": scale, "offset": [float(x) for x in offset], "rd_to_gltf": RD_TO_GLTF.tolist(),
                   "clips": [c for c, _p in clips]})
    b.doc["asset"]["extras"] = {"rd4_to_gltf": extras}
    b.write(path)
    return len(parts) + 1, len(mesh_nodes)


def trs_node(name: str, m: np.ndarray, scale: bool = False) -> dict:
    t, r, sc_ = decompose(m)
    node = {"name": name, "translation": [float(x) for x in t], "rotation": [float(x) for x in r]}
    if scale or np.abs(sc_ - 1.0).max() > 1e-4:
        node["scale"] = [float(x) for x in sc_]
    return node


def decompose(m: np.ndarray) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
    """(translation, rotation xyzw, scale) of an affine matrix without shear, as glTF composes
    them (scale first, in the node's own axes). Ray Dream's deformer is a rotation times an axis
    scale, so a geometry frame never shears."""
    lin = m[:3, :3]
    s = np.linalg.norm(lin, axis=0)
    r = lin / np.where(s > 0, s, 1.0)
    if np.abs(r.T @ r - np.eye(3)).max() > 1e-4 or np.linalg.det(r) < 0:
        raise ValueError("sheared or mirrored transform cannot become glTF TRS")
    return m[:3, 3], matrix_to_quat_xyzw(r), s


# A part that a pose's scene does not have is shrunk to nothing on its shape joint.
HIDDEN_SCALE = 1e-4


def pose_on(main: "Converter", conv: "Converter", tick: float, bind_local: list[np.ndarray],
            clip: str, hide: frozenset = frozenset(), view=None) -> tuple[list[np.ndarray], list[np.ndarray | None]]:
    """One pose of ``conv``'s scene on ``main``'s skeleton: local joint matrices, and per joint
    its geometry frame relative to the joint (None for a part the scene does not have, or one
    named in ``hide``), seen as ``view`` (Converter.rd_frames). Joints are matched by name; one
    the scene lacks keeps its bind pose relative to its parent."""
    conv.offset = main.offset
    frames = conv.rd_frames(float(tick), view)
    # Joints a converter took from other scenes (add_joint) are not in its own scene.
    index = {n: k for k, n in enumerate(conv.names) if n not in hide and k not in conv.scene_of}
    missing = [n for n in main.names if n not in index and n not in main.optional]
    if missing:
        main.warnings.append(f"clip {clip!r}: {conv.scene_label} has no {missing}; hidden")
    worlds, local, shapes = [], [], []
    for i, n in enumerate(main.names):
        p = main.parent[i]
        k = index.get(n)
        if k is None:
            w = bind_local[i] if p < 0 else worlds[p] @ bind_local[i]
            shape = None
        else:
            cw, gw = frames[k]
            w = conv.joint_world(cw)[0]
            shape = np.linalg.inv(w) @ conv.shape_world(gw) if conv.joints[k].master else None
        worlds.append(w)
        local.append(w if p < 0 else np.linalg.inv(worlds[p]) @ w)
        shapes.append(shape)
    return local, shapes


def write_animation(b: gltf.GltfBuilder, name: str, node_of: list[int], shape_node_of: dict[int, int],
                    bind_shape: dict[int, np.ndarray], poses: list):
    """One glTF animation from (seconds, local joint matrices, shapes) poses, LINEAR between them.
    Every joint gets every channel, so a pose never inherits another clip's state."""
    seconds = np.array([p[0] for p in poses], dtype=np.float32)
    time_acc = b.accessor(seconds, gltf.FLOAT, minmax=True)
    samplers, channels = [], []

    def channel(node: int, mats: list[np.ndarray], with_scale: bool):
        parts = [decompose(m) for m in mats]
        # A turned view may scale the whole Blupi (Hide): then the root carries a scale too.
        with_scale = with_scale or any(np.abs(sc_ - 1.0).max() > 1e-4 for _t, _q, sc_ in parts)
        quats = []
        for _t, q, _s in parts:
            if quats and q @ quats[-1] < 0:
                q = -q
            quats.append(q)
        data = [("translation", np.array([t for t, _q, _s in parts], dtype=np.float32)),
                ("rotation", np.array(quats, dtype=np.float32))]
        if with_scale:
            data.append(("scale", np.array([s_ for _t, _q, s_ in parts], dtype=np.float32)))
        for path_name, values in data:
            samplers.append({"input": time_acc, "output": b.accessor(values, gltf.FLOAT),
                             "interpolation": "LINEAR"})
            channels.append({"sampler": len(samplers) - 1, "target": {"node": node, "path": path_name}})

    for i, node in enumerate(node_of):
        channel(node, [local[i] for _s, local, _sh in poses], False)
    hidden = np.diag([HIDDEN_SCALE] * 3 + [1.0])
    for i, node in shape_node_of.items():
        channel(node, [sh[i] if sh[i] is not None else bind_shape[i] @ hidden for _s, _l, sh in poses], True)
    b.add("animations", {"name": name, "samplers": samplers, "channels": channels})


class Clip:
    """One animation: a scene's ticks [t0, t1] (one tick for a static pose) on the main skeleton."""

    def __init__(self, name: str, conv: "Converter", label: str, t0: float, t1: float):
        self.name, self.conv, self.label, self.t0, self.t1 = name, conv, label, t0, t1

    def poses(self, main: "Converter", sample_fps: float, frame_rate: float | None):
        """[(seconds, converter, tick)] sampling the clip."""
        conv = self.conv
        conv.scene_label = self.label
        # Seconds per scene tick: real time, or one rendered frame per 1/frame_rate seconds.
        per_tick = 1.0 / sc.TICKS_PER_SECOND
        if frame_rate:
            per_tick = conv.scene.fps / (sc.TICKS_PER_SECOND * frame_rate)
        if self.t1 > self.t0:
            n = max(2, int(round((self.t1 - self.t0) * per_tick * sample_fps)) + 1)
            ticks = np.linspace(self.t0, self.t1, n)
            seconds = (ticks - self.t0) * per_tick
        else:
            ticks = np.array([self.t0, self.t0])
            seconds = np.array([0.0, 1.0 / (frame_rate or 1.0)])
        return [(float(s_), conv, float(t)) for s_, t in zip(seconds, ticks)]


class PoseClip:
    """An animation assembled from single poses of possibly different scenes: (seconds,
    converter, tick[, joint names to hide])."""

    def __init__(self, name: str, poses: list[tuple]):
        self.name, self.poses_ = name, poses
        self.label = ", ".join(sorted({p[1].scene_label for p in poses}))
        self.t0 = self.t1 = 0.0

    def poses(self, main, sample_fps, frame_rate):
        return self.poses_


def _path_to(node: sc.Obj, target: sc.Obj) -> list[sc.Obj]:
    if node is target:
        return [node]
    for c in node.children:
        p = _path_to(c, target)
        if p:
            return [node] + p
    return []


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
