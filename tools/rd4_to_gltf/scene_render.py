"""Render a frame of a Ray Dream scene directly, without going through glTF.

Uses the scene's own forward kinematics (scene.py), geometry generated from its masters
(geometry.canonical_mesh), the body's face texture and flat colours (shading.py), its orthographic
game camera and Ray Dream's lighting, with the rasterizer of verify_sprites.py. Every object with
a master is drawn, props included; objects whose master is not supported yet are skipped and
listed in ``SceneRenderer.skipped``.
"""

from __future__ import annotations

import numpy as np

import freeform_mesh
import geometry
import scene as sc
import shading
import verify_sprites as vs


class SceneRenderer:
    def __init__(self, scene: sc.Scene, camera: str = "Jeu"):
        self.scene = scene
        self.camera_name = camera
        self.meshes: dict[str, tuple | None] = {}
        self.textures: dict[str, tuple | None] = {}
        self.skipped: dict[str, str] = {}

    def _canonical(self, master: str):
        if master not in self.meshes:
            try:
                mesh, ff = geometry.canonical_mesh(self.scene.masters[master])
                pos, nrm, tris, src = geometry.smooth_normals(mesh, return_index=True)
                self.meshes[master] = (pos, nrm, tris, src, ff)
            except (NotImplementedError, KeyError, AttributeError, IndexError) as e:
                self.meshes[master] = None
                self.skipped[master] = str(e) or type(e).__name__
        return self.meshes[master]

    def _texture(self, obj: sc.Obj, ff):
        key = id(obj)
        if key not in self.textures:
            self.textures[key] = None
            if ff is not None:
                try:
                    tex = shading.texture_map(self.scene, obj)
                except NotImplementedError:
                    tex = None
                if tex is not None:
                    self.textures[key] = (tex, freeform_mesh.texture_coordinates(ff, tex.quarter_turns))
        return self.textures[key]

    def camera(self, tick: float):
        cams = [(o, cw) for o, _p, cw, _g in sc.world_frames(self.scene, tick)
                if o.kind == "ca" and self.camera_name in o.name]
        if not cams:
            raise ValueError(f"no camera matching {self.camera_name!r}")
        o, cw = cams[0]
        proj = o.chunk.chunk("proj")
        iso = proj.chunk("iso") if proj is not None else None
        if iso is None:
            raise NotImplementedError(f"camera {o.name!r} is not orthographic")
        frame_height = o.chunk.floats("size")[1]
        return cw[:3, 3], cw[:3, 0], cw[:3, 1], cw[:3, 2], iso.float("zoom") / frame_height

    def lighting(self, tick: float) -> vs.Lighting:
        for o, _p, cw, _g in sc.world_frames(self.scene, tick):
            if o.kind == "li":
                return vs.Lighting(cw[:3, 3], point=True)
        return vs.Lighting(np.array([0.5, 0.0, 0.85]))

    def frame_meshes(self, tick: float) -> list:
        out = []
        for o, _p, _cw, gw in sc.world_frames(self.scene, tick):
            if not o.master:
                continue
            canon = self._canonical(o.master)
            if canon is None:
                continue
            pos, nrm, tris, src, ff = canon
            lin = gw[:3, :3]
            wpos = pos @ lin.T + gw[:3, 3]
            wn = nrm @ np.linalg.inv(lin)
            wn /= np.maximum(np.linalg.norm(wn, axis=1, keepdims=True), 1e-12)
            t = tris[:, ::-1] if np.linalg.det(lin) < 0 else tris
            tex = self._texture(o, ff)
            if tex is not None:
                st = tex[1][src]
                p2, n2, t2, uv = freeform_mesh.split_seam(wpos, wn, t, st, 1.0)
                out.append((p2, n2, t2, np.ones(3), uv, tex[0].image.astype(float) / 255.0))
            else:
                out.append((wpos, wn, t, np.array(shading.material_key(self.scene, o))))
        return out

    def render(self, tick: float, size: int = 60, supersample: int = 4):
        """(rgb (size, size, 3) in 0..1, coverage (size, size)) of the scene at ``tick``."""
        pos, right, up, back, px_per_unit_per_px = self.camera(tick)
        px = px_per_unit_per_px * size
        color, cover = vs.render(self.frame_meshes(tick), pos, right, up, back, px * supersample,
                                 size * supersample, self.lighting(tick), supersample)
        return vs.downsample(color, cover, supersample)

    def frame_ticks(self) -> list[float]:
        """Ticks of the frames Ray Dream rendered: FSec per second from begin to before end."""
        s = self.scene
        n = int(round((s.end - s.begin) / sc.TICKS_PER_SECOND * s.fps))
        return [s.begin + k * sc.TICKS_PER_SECOND / s.fps for k in range(max(n, 1))]
