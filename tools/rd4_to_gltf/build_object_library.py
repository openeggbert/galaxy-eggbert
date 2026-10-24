#!/usr/bin/env python3
"""3D models of the game's objects (enemies, pickups, decor) from the Ray Dream scenes their
sprites were rendered from, with a table of which pose draws which sprite icon.

Every object of ``--objects`` (``objects.txt``: name, sheet, icons, scenes, options) is matched
icon by icon against every distinct frame of its scenes, straight and mirrored, rendered with the
scene's orthographic camera at the sheet's icon size (``survey.GRIDS``); with ``yaw=STEP`` also
turned about the vertical every STEP degrees (the egg's sprites spin, its scene does not); with
``vflip=1`` also upside down (the hanging mine's icons are the mine's flipped vertically). Each
scene that wins an icon becomes one model: one rigid joint per Ray Dream object the camera sees,
and a clip ``Frames`` holding the scene's frames at 20 per second (frame k at k / 20 s).

Model space is the sprite's: +X its right, +Y up, +Z toward its camera, the origin on the camera
axis (the icon's centre) at the depth of the object's middle, in blocks of the game (64 sprite
pixels, as for Blupi: ``avatars3d/blupi/README.md``). The game draws a model where it would have
drawn the icon's billboard: turned by the icon's yaw about +Y (+Z toward +X, XNA's
``CreateRotationY``), then flipped as the icon is (1: left-right, 2: upside down; about the
icon's centre line, half a pixel off the camera axis: ``mirror_matrix``).

Writes ``<out>/<object>_<scene>/model.glb`` and beside it ``icons.txt`` (``icon <sheet> <icon>
<seconds> <mirror> <yaw degrees> <iou>`` per icon the model draws), ``compare.png`` (rows:
sprite, the model rendered back as the game would draw it, overlap) and ``<out>/report.md``.
``export_objects.py`` turns them into the game's ``avatars3d/objects/<object>_<scene>/``.

    python3 tools/rd4_to_gltf/build_object_library.py --originals ../speedyblupi-data/originals \\
        --sheets ../mobile-eggbert/Content/icons --objects tools/rd4_to_gltf/objects.txt \\
        --out build-probe/rd4-objects [--only treasure,egg]
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import match_icons  # noqa: E402
import rd4  # noqa: E402
import rd4_to_gltf  # noqa: E402
import scene as sc  # noqa: E402
import scene_render  # noqa: E402
import survey  # noqa: E402
import verify_sprites as vs  # noqa: E402

FPS = 20.0
BLOCK_PX = 64.0
SUPERSAMPLE = 4
MAX_BONES = 72  # CNA's SkinnedEffect


@dataclass
class ObjectSpec:
    name: str
    sheet: str
    icons: list[int]
    scenes: list[str]
    camera: str | None = None
    yaw_step: float = 0.0
    vflip: bool = False


@dataclass
class Match:
    icon: int
    score: float
    iou: float
    colour: float
    scene: int
    frame: int
    mirror: int  # 0 none, 1 left-right (X), 2 upside down (Y)
    yaw: float


@dataclass
class SceneSetup:
    """A scene as the object's sprites saw it: its camera, frames, the distinct ones, the parts
    the camera sees and the model's origin (on the camera axis, at the parts' middle depth)."""
    scene: sc.Scene
    camera: str
    ticks: list[float]
    distinct: list[int]
    parts: list[tuple[int, sc.Obj]] = field(default_factory=list)
    depth: float = 0.0

    def camera_frame(self, tick: float):
        """(frame centre, right, up, back, zoom, frame height) of the camera at ``tick``."""
        for o, _p, cw, _g in sc.world_frames(self.scene, tick):
            if o.kind == "ca" and self.camera in o.name:
                iso = o.chunk.chunk("proj").chunk("iso")
                return (sc.camera_centre(o, cw), cw[:3, 0], cw[:3, 1], cw[:3, 2], iso.float("zoom"),
                        o.chunk.floats("size")[1])
        raise ValueError(f"no camera {self.camera!r}")

    def origin(self, tick: float) -> np.ndarray:
        pos, _r, _u, back, _z, _h = self.camera_frame(tick)
        return pos + back * self.depth


def parse_icons(text: str) -> list[int]:
    out = []
    for part in text.split(","):
        a, _, b = part.partition("-")
        out += list(range(int(a), int(b or a) + 1))
    return out


def read_specs(path: Path) -> list[ObjectSpec]:
    specs = []
    for line in path.read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        opts = dict(f.split("=", 1) for f in fields[4:])
        specs.append(ObjectSpec(fields[0], fields[1], parse_icons(fields[2]), fields[3].split(","),
                                opts.get("camera"), float(opts.get("yaw", 0)), opts.get("vflip") == "1"))
    return specs


def ortho_camera(scene: sc.Scene, wanted: str | None) -> str:
    """The camera the sprites were rendered with: ``wanted``, else the orthographic game camera."""
    if wanted:
        return wanted
    cams = [o for o, _p, _c, _g in sc.world_frames(scene, None) if o.kind == "ca"]
    iso = [o.name for o in cams if o.chunk.chunk("proj") is not None and o.chunk.chunk("proj").chunk("iso")]
    for key in ("Jeu", "2"):
        for name in iso:
            if key in name:
                return name
    if iso:
        return iso[0]
    raise NotImplementedError("no orthographic camera")


def setup_scene(scene: sc.Scene, camera: str) -> SceneSetup:
    renderer = scene_render.SceneRenderer(scene, camera)
    ticks = renderer.frame_ticks()
    setup = SceneSetup(scene, camera, ticks, [])
    seen = set()
    for f, tick in enumerate(ticks):
        key = tuple(np.round(gw, 4).tobytes() for o, _p, _c, gw in sc.world_frames(scene, tick)
                    if o.master or o.kind == "ca")
        if key not in seen:
            seen.add(key)
            setup.distinct.append(f)
    # Parts: every geometric object inside the camera's frame (10 % margin) in some frame.
    per_frame = [sc.world_frames(scene, t) for t in ticks]
    middle = []
    for i, (o, _p, _c, _g) in enumerate(per_frame[0]):
        if not o.master or o.kind in ("ca", "li"):
            continue
        canon = renderer._canonical(o.master)
        if canon is None:
            continue
        for f in setup.distinct:
            pos, right, up, back, zoom, frame_h = setup.camera_frame(ticks[f])
            gw = per_frame[f][i][3]
            w = canon[0] @ gw[:3, :3].T + gw[:3, 3]
            half = frame_h / zoom / 2 * 1.1
            x, y = (w - pos) @ right, (w - pos) @ up
            if x.max() > -half and x.min() < half and y.max() > -half and y.min() < half:
                setup.parts.append((i, o))
                middle.append(w)
                break
    if not setup.parts:
        raise ValueError("the camera sees no part")
    pts = np.concatenate(middle)
    pos0, _r, _u, back0, _z, _h = setup.camera_frame(ticks[0])
    setup.depth = float((((pts.min(0) + pts.max(0)) / 2) - pos0) @ back0)
    return setup


def match_object(spec: ObjectSpec, setups: list[SceneSetup], icon_rgb, icon_mask, size: int) -> dict[int, Match]:
    wanted = np.array(spec.icons)
    yaws = list(np.arange(0.0, 360.0, spec.yaw_step)) if spec.yaw_step > 0 else [0.0]
    best: dict[int, Match] = {}
    for si, setup in enumerate(setups):
        renderer = scene_render.SceneRenderer(setup.scene, setup.camera)
        renderer.shadows = False
        for fi in setup.distinct:
            tick = setup.ticks[fi]
            renderer.view_centre = setup.origin(tick)
            for yaw in yaws:
                renderer.view = (float(yaw), 0.0, 1.0) if yaw else None
                rgb, cov = renderer.render(tick, size=size)
                for mirror in (0, 1, 2) if spec.vflip else (0, 1):
                    rg, cv = reflect(rgb, mirror), reflect(cov, mirror)
                    iou, col = match_icons.scores(rg, cv, icon_rgb[wanted], icon_mask[wanted])
                    for k, icon in enumerate(spec.icons):
                        score = float(iou[k] * col[k])
                        if icon not in best or score > best[icon].score:
                            best[icon] = Match(icon, score, float(iou[k]), float(col[k]), si, fi, mirror,
                                               float(yaw))
    return best


def reflect(img: np.ndarray, mirror: int) -> np.ndarray:
    """An image flipped as the original game flipped sprites (1: left-right, 2: upside down),
    about its centre line: the hanging mine's icons 48-56 are exactly icons 12-20 flipped. The
    camera axis passes through the centre of the middle pixel, so in model space that line is
    half a pixel off the origin (``mirror_matrix``)."""
    if mirror == 1:
        return img[:, ::-1]
    if mirror == 2:
        return img[::-1]
    return img


def mirror_matrix(mirror: int) -> np.ndarray:
    """4x4 of a sprite flip in model space (blocks; a pixel is 1/64): x' = -x - 1/64 left-right,
    y' = -y + 1/64 upside down. The game applies the same (GalaxyEggbertGame.cpp)."""
    m = np.eye(4)
    if mirror == 1:
        m[0, 0], m[0, 3] = -1.0, -1.0 / BLOCK_PX
    elif mirror == 2:
        m[1, 1], m[1, 3] = -1.0, 1.0 / BLOCK_PX
    return m


def part_name(index: int, o: sc.Obj) -> str:
    label = re.sub(r"[^\w.-]", "_", o.name or o.master or "part")
    return f"p{index:02d}_{label}"


def build_model(spec: ObjectSpec, setup: SceneSetup, icon_px: int, path: Path, warnings: list[str]) -> None:
    def to_model(tick: float) -> np.ndarray:
        """RD world -> the sprite's space, written in RD axes (rd4_to_gltf.RD_TO_GLTF then gives
        +X right, +Y up, +Z toward the camera)."""
        _pos, right, up, back, _z, _h = setup.camera_frame(tick)
        lin = np.stack([-right, back, up])
        return sc.affine(lin, -lin @ setup.origin(tick))

    per_frame = [sc.world_frames(setup.scene, t) for t in setup.ticks]
    names = [part_name(i, o) for i, o in setup.parts]
    a0 = to_model(setup.ticks[0])
    bind = [(names[k], setup.scene, o, a0 @ per_frame[0][i][3]) for k, (i, o) in enumerate(setup.parts)]
    poses = []
    for f, tick in enumerate(setup.ticks):
        a = to_model(tick)
        poses.append((f / FPS, {names[k]: a @ per_frame[f][i][3] for k, (i, _o) in enumerate(setup.parts)}))
    # Parts that move together (the same motion from frame 0 in every frame) share a joint.
    joint_of, motions = [], {}
    for k, name in enumerate(names):
        m0 = np.linalg.inv(poses[0][1][name])
        key = np.round(np.stack([pp[name] @ m0 for _s, pp in poses]), 4).tobytes()
        joint_of.append(motions.setdefault(key, k))
    _pos, _r, _u, _b, zoom, frame_h = setup.camera_frame(setup.ticks[0])
    scale = zoom * icon_px / (frame_h * BLOCK_PX)
    joints, _meshes = rd4_to_gltf.write_parts_model(
        path, spec.name, bind, [("Frames", poses)], scale, np.zeros(3), warnings,
        {"object": spec.name, "camera": setup.camera}, joint_of)
    if joints > MAX_BONES:
        raise ValueError(f"{spec.name}: {joints} joints, CNA's skinning takes {MAX_BONES}")


def yaw_matrix(degrees: float) -> np.ndarray:
    """XNA's CreateRotationY for column vectors: +Z turns toward +X."""
    c, s = np.cos(np.radians(degrees)), np.sin(np.radians(degrees))
    return np.array([[c, 0.0, s], [0.0, 1.0, 0.0], [-s, 0.0, c]])


def render_model(model: vs.Model, seconds: float, mirror: int, yaw: float, icon_px: int):
    """The model as the game draws it for an icon, seen by the sprite's camera."""
    turn = yaw_matrix(yaw)
    flip = mirror_matrix(mirror)
    meshes = []
    for pos, nrm, tris, color, uv, tex in model.skinned(seconds):
        pos, nrm = pos @ turn.T, nrm @ turn.T
        if mirror:
            pos = pos @ flip[:3, :3].T + flip[:3, 3]
            nrm, tris = nrm @ flip[:3, :3].T, tris[:, ::-1]
        meshes.append((pos, nrm, tris, color, uv, tex))
    light = vs.Lighting(np.array([0.4, 0.6, 0.7]) / np.linalg.norm([0.4, 0.6, 0.7]))
    color, cover = vs.render(meshes, np.array([0.0, 0.0, 50.0]), np.array([1.0, 0.0, 0.0]),
                             np.array([0.0, 1.0, 0.0]), np.array([0.0, 0.0, 1.0]), BLOCK_PX * SUPERSAMPLE,
                             icon_px * SUPERSAMPLE, light, SUPERSAMPLE)
    return vs.downsample(color, cover, SUPERSAMPLE)


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--originals", required=True, type=Path)
    ap.add_argument("--sheets", required=True, type=Path, help="directory of the sprite sheets")
    ap.add_argument("--objects", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--only", help="comma-separated object names")
    args = ap.parse_args(argv)

    specs = read_specs(args.objects)
    if args.only:
        keep = set(args.only.split(","))
        specs = [s for s in specs if s.name in keep]
    args.out.mkdir(parents=True, exist_ok=True)
    report_path = args.out / "report.md"
    rows = {}
    if report_path.exists():
        for line in report_path.read_text().splitlines()[2:]:
            rows[line.split("|")[2].strip()] = line
    for spec in specs:
        grid = survey.GRIDS[spec.sheet]
        icon_px = grid[0]
        icon_rgb, icon_mask = survey.load_sheet(args.sheets / f"{spec.sheet}.png", grid)
        setups = []
        for s in spec.scenes:
            scene = sc.read_scene(rd4.parse_file(args.originals / s))
            setups.append(setup_scene(scene, ortho_camera(scene, spec.camera)))
        best = match_object(spec, setups, icon_rgb, icon_mask, icon_px)
        for si, setup in enumerate(setups):
            matches = sorted((m for m in best.values() if m.scene == si), key=lambda m: m.icon)
            if not matches:
                continue
            stem = Path(spec.scenes[si]).stem
            out = args.out / f"{spec.name}_{stem}"
            out.mkdir(parents=True, exist_ok=True)
            warnings: list[str] = []
            build_model(spec, setup, icon_px, out / "model.glb", warnings)
            lines = [f"# {spec.name}: {spec.scenes[si]}, camera {setup.camera!r}; generated by build_object_library.py",
                     "# icon <sheet> <icon> <seconds> <mirror 0/1 left-right/2 upside down> <yaw degrees> <iou>"]
            model = vs.Model(out / "model.glb")
            model.select_clip("Frames")
            tiles, checks = [], []
            for m in matches:
                seconds = m.frame / FPS
                lines.append(f"icon {spec.sheet} {m.icon} {seconds:.4f} {m.mirror} {m.yaw:g} {m.iou:.3f}")
                rgb, cov = render_model(model, seconds, m.mirror, m.yaw, icon_px)
                sm, rm = icon_mask[m.icon], cov > 0.5
                checks.append(vs.iou(rm, sm))
                bg = np.array([40, 90, 140]) / 255
                spr = icon_rgb[m.icon] * sm[..., None] + bg * (1 - sm[..., None])
                ren = rgb * rm[..., None] + bg * (1 - rm[..., None])
                diff = np.zeros(rgb.shape) + 0.12
                diff[sm & rm] = 0.8
                diff[sm & ~rm] = (0.9, 0.25, 0.25)
                diff[~sm & rm] = (0.25, 0.8, 0.35)
                tiles.append(np.concatenate([spr, ren, diff], axis=0))
            (out / "icons.txt").write_text("\n".join(lines) + "\n")
            montage = (np.concatenate(tiles, axis=1).clip(0, 1) * 255).astype(np.uint8)
            Image.fromarray(montage).resize((montage.shape[1] * 3, montage.shape[0] * 3), Image.NEAREST).save(
                out / "compare.png")
            ious = [m.iou for m in matches]
            name = f"{spec.name}_{stem}"
            rows[name] = (f"| {spec.name} | {name} | {len(matches)} | {np.mean(ious):.3f} | {min(ious):.3f} | "
                          f"{min(checks):.3f} |")
            print(f"{name}: {len(matches)} icons, IoU mean {np.mean(ious):.3f} min {min(ious):.3f}, "
                  f"model check min {min(checks):.3f}")
            for w in warnings:
                print("  warning:", w)
    report = ["| Object | Model | Icons | Mean IoU | Min IoU | Model check (min IoU) |",
              "|---|---|---|---|---|---|"] + [rows[k] for k in sorted(rows)]
    report_path.write_text("\n".join(report) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
