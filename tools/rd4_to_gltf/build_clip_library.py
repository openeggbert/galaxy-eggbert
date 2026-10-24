#!/usr/bin/env python3
"""Build Blupi's clip library: one animation per game animation state, from the sprite icons.

Every ``BlupiController::AnimState`` (and every death cause / pickup kind variant) plays a table
of sprite icons at ``kAnimFps`` (``BlupiController.cpp``: ``GetAnimIcon`` and the ``k...Frames``
tables, read in place). ``match_icons.py`` found which Ray Dream scene frame each icon was rendered
from. Each clip here is that icon sequence turned into 3D poses: icon k becomes a key at k /
frame_rate seconds holding the pose of its scene frame, LINEAR in between, with one more key
closing the loop. Icons without a usable match hold the previous pose.

Every scene is moved, frame by frame, so that its game camera sits where the base scene's does:
a sprite shows Blupi relative to that camera, so poses from different scenes keep their sprite
placement (a jump frame drawn higher stays higher; genou-profil.rd4's camera follows him down).

Each frame's face is the body texture of the scene it comes from. ``faces.txt`` beside the
.glb gives the frame rate (``rate``), the part that wears the face (``part``), the face PNGs
(``face <file>``, numbered from 0) and per clip the face of every frame (``clip <name> <face>
<face> ...``); the game swaps that part's texture accordingly.

    python3 build_clip_library.py --scenes ../speedyblupi-data/originals/Blupi \\
        --base marche-profil.rd4 --icons build-probe/rd4-blupi/match/icons.csv \\
        --controller src/GalaxyEggbert/Game/BlupiController.cpp \\
        --sheet ../mobile-eggbert/Content/icons/blupi.png \\
        --out build-probe/rd4-blupi/library/blupi.glb
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import re
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import rd4  # noqa: E402
import rd4_to_gltf as conv_mod  # noqa: E402
import scene as sc  # noqa: E402
import scene_render  # noqa: E402
import shading  # noqa: E402
import verify_sprites as vs  # noqa: E402

USABLE = 0.70


def controller_clips(path: Path) -> dict[str, list[int]]:
    """{clip name: icon sequence} from BlupiController.cpp's GetAnimIcon and frame tables."""
    text = path.read_text()
    tables = {}
    for name, body in re.findall(r"constexpr int (k\w+Frames)\[\]\s*=\s*\{(.*?)\};", text, flags=re.S):
        body = re.sub(r"//[^\n]*", "", body)
        tables[name] = [int(x) for x in re.findall(r"-?\d+", body)]
    start = text.index("int BlupiController::GetAnimIcon() const noexcept")
    body = text[start:text.index("bool BlupiController::AnimIconUsesElementSheet", start)]
    clips: dict[str, list[int]] = {}
    for state, table in re.findall(
            r"case AnimState::(\w+):\s*(?:default:\s*)?(?:\{\s*const int icon = |return )(k\w+Frames)\[", body):
        clips[state] = tables[table]
    for cause, table in re.findall(r"case DeathCause::(\w+):\s*(?:default:\s*)?icon = (k\w+Frames)\[", body):
        clips[f"DeathLocked.{cause}"] = tables[table]
    for kind, table in re.findall(r"case PickupFreezeKind::(\w+):\s*(?:default:\s*)?return (k\w+Frames)\[", body):
        clips[f"PickupBusy.{kind}"] = tables[table]
    return clips


def load_matches(path: Path) -> dict[int, tuple[str, float, float]]:
    """{icon: (scene file, tick, score)} for icons whose as-rendered match is usable."""
    out = {}
    for row in csv.DictReader(open(path)):
        if row.get("plain_score") and float(row["plain_score"]) >= USABLE:
            out[int(row["icon"])] = (row["plain_scene"], float(row["plain_tick"]), float(row["plain_score"]))
    return out


def body_parent(scene: sc.Scene) -> sc.Obj:
    stack = [scene.universe]
    while stack:
        o = stack.pop()
        if any(c.name == "Corps" for c in o.children):
            return o
        stack.extend(o.children)
    raise ValueError("no Corps")


def stem(name: str) -> str:
    """Object name without Blender-style uniqueness suffix and side ("Pied-d.001" -> "Pied")."""
    return re.sub(r"(-[dg])?(\.\d{3})?$", "", name)


def match_names(base: conv_mod.Converter, c: conv_mod.Converter):
    """Rename ``c``'s joints that have no namesake in ``base`` after the unmatched base joint of
    the same kind nearest to it in the body's frame: blupi-plat.rd4 names both feet "Pied-d"."""
    def body_local(conv):
        frames = conv.rd_frames(conv.scene.begin)
        inv = np.linalg.inv(frames[conv.names.index("Corps")][0])
        return [(inv @ cw)[:3, 3] for cw, _gw in frames]

    free = [i for i, n in enumerate(base.names) if n not in c.names]
    if not free:
        return
    bp, cp = body_local(base), body_local(c)
    for k, n in enumerate(list(c.names)):
        if n in base.names:
            continue
        cands = [i for i in free if stem(base.names[i]) == stem(n)]
        if cands:
            i = min(cands, key=lambda i: np.linalg.norm(bp[i] - cp[k]))
            c.names[k] = base.names[i]
            free.remove(i)


def name_props(base: conv_mod.Converter, c: conv_mod.Converter) -> set[str]:
    """Give the joints of ``c`` that Blupi's base skeleton lacks (props: the tongue, the lollipop,
    the bottle) names that say where they hang: "Sucette", "Sucette/Cylindre", ... Many prop
    objects are unnamed; their master names them then."""
    taken = set(base.names)
    for k, o in enumerate(c.joints):
        if c.names[k] in base.names:
            continue
        own = o.name or o.master or "prop"
        p = c.parent[k]
        name = own if c.names[p] in base.names else f"{c.names[p]}/{own}"
        unique, n = name, 0
        while unique in taken:
            n += 1
            unique = f"{name}.{n:03d}"
        taken.add(unique)
        c.names[k] = unique
    return {n for n in c.names if n not in base.names}


def add_props(base: conv_mod.Converter, c: conv_mod.Converter):
    """Append ``c``'s props (name_props) to the base skeleton under their named parents."""
    for k, o in enumerate(c.joints):
        if c.names[k] in base.names:
            continue
        base.add_joint(o, c.names[k], base.names.index(c.names[c.parent[k]]), c.scene, c.scene.begin)


def game_camera(scene: sc.Scene, tick: float) -> np.ndarray:
    for o, _p, cw, _g in sc.world_frames(scene, tick):
        if o.kind == "ca" and "Jeu" in o.name:
            return cw[:3, 3]
    raise ValueError("no game camera")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--scenes", required=True, type=Path)
    ap.add_argument("--base", default="marche-profil.rd4", help="scene giving geometry, skeleton and camera")
    ap.add_argument("--icons", required=True, type=Path, help="icons.csv from match_icons.py")
    ap.add_argument("--controller", required=True, type=Path, help="BlupiController.cpp (read in place)")
    ap.add_argument("--sheet", required=True, type=Path, help="sprite sheet, for the per-clip check")
    ap.add_argument("--frame-rate", type=float, default=8.0, help="icons per second (kAnimFps)")
    ap.add_argument("--scale", type=float, default=0.05)
    ap.add_argument("--out", required=True, type=Path, help="output .glb; faces go beside it")
    ap.add_argument("--skip-check", action="store_true", help="do not render every clip frame against its icon")
    args = ap.parse_args(argv)
    args.out.parent.mkdir(parents=True, exist_ok=True)

    warnings: list[str] = []
    base_scene = sc.read_scene(rd4.parse_file(args.scenes / args.base))
    base = conv_mod.Converter(base_scene, "BLUPI", args.scale, warnings)
    base.scene_label = args.base
    base_cam = game_camera(base_scene, base_scene.begin)
    converters: dict[str, conv_mod.Converter] = {args.base: base}
    props_of: dict[str, frozenset] = {args.base: frozenset()}
    sheet = np.array(Image.open(args.sheet).convert("RGBA"))
    renderers: dict[str, scene_render.SceneRenderer] = {}
    shows_props: dict[int, bool] = {}
    faces: list[str] = []
    face_index: dict[str, int] = {}

    def converter(name: str) -> conv_mod.Converter:
        if name not in converters:
            s = sc.read_scene(rd4.parse_file(args.scenes / name))
            # Blupi's root is the group holding his body ("BLUPI"; blupi-ballon.rd4 has none and
            # hangs the body straight under the universe), matched to the base root by role.
            c = conv_mod.Converter(s, body_parent(s).name, args.scale, warnings)
            c.names[0] = base.names[0]
            match_names(base, c)
            props_of[name] = frozenset(name_props(base, c))
            c.scene_label = name
            def shift(t, s=s):
                d = base_cam - game_camera(s, t)
                d[0] = 0.0  # depth along the profile camera's axis is invisible in the sprites
                return d
            c.world_shift = shift
            converters[name] = c
        return converters[name]

    def face_of(name: str) -> int:
        c = converter(name)
        if name not in face_index:
            tex = shading.texture_map(c.scene, c.scene.find("Corps"))
            digest = hashlib.sha1(tex.image.tobytes()).hexdigest()[:10]
            file = f"face_{digest}.png"
            if file not in faces:
                faces.append(file)
                Image.fromarray(tex.image).save(args.out.parent / file)
            face_index[name] = faces.index(file)
        return face_index[name]

    def hidden_props(icon: int, m: tuple) -> frozenset:
        """The props of the icon's scene frame unless the icon shows them: a scene may hold a prop
        that only some of the icons rendered from it show (blupi-sucette.rd4's lollipop is not in
        icon 2). They are shown when the frame matches the icon at least as well with them."""
        props = props_of[m[0]]
        if not props:
            return props
        if icon not in shows_props:
            c = converter(m[0])
            r = renderers.setdefault(m[0], scene_render.SceneRenderer(c.scene))
            row, col = divmod(icon, 10)
            sprite = sheet[row * 60:(row + 1) * 60, col * 60:(col + 1) * 60, 3] > 127
            r.exclude = set()
            with_props = vs.iou(r.render(m[1])[1] > 0.5, sprite)
            r.exclude = {id(o) for o, n in zip(c.joints, c.names) if n in props}
            without = vs.iou(r.render(m[1])[1] > 0.5, sprite)
            shows_props[icon] = with_props >= without
        return frozenset() if shows_props[icon] else props

    matches = load_matches(args.icons)
    clips = controller_clips(args.controller)
    pose_clips, clip_faces, report = [], {}, []
    for name, icons in clips.items():
        poses, face_seq, sources = [], [], []
        last = None
        fallbacks = 0
        for k, icon in enumerate(icons):
            source = icon if icon in matches else None
            if source is None:
                fallbacks += 1
                source = last
            if source is None:
                # Nothing usable yet: borrow the first usable icon later in the sequence.
                source = next((i for i in icons[k:] if i in matches), None)
            if source is None:
                break
            last = source
            m = matches[source]
            poses.append((k / args.frame_rate, converter(m[0]), m[1], hidden_props(source, m)))
            face_seq.append(face_of(m[0]))
            sources.append((icon, m))
        if len(poses) != len(icons) or not poses:
            report.append((name, len(icons), 0, fallbacks, "no usable icon: not built"))
            continue
        poses.append((len(icons) / args.frame_rate, *poses[0][1:]))
        pose_clips.append(conv_mod.PoseClip(name, poses))
        clip_faces[name] = face_seq
        scenes = sorted({m[0] for _i, m in sources})
        report.append((name, len(icons), len(icons) - fallbacks, fallbacks, ", ".join(scenes)))

    # Props of the scenes the clips use join the skeleton; every other clip hides them.
    for scene_name, c in list(converters.items()):
        if any(p[1] is c and p[3] != props_of[scene_name] for clip in pose_clips for p in clip.poses_):
            add_props(base, c)
    props = sorted(base.optional)
    joints, meshes = base.write(args.out, pose_clips, 30.0, base_scene.begin,
                                {"scene": args.base, "library": True}, args.frame_rate)
    faces_txt = [f"# Blupi's faces per clip frame ({args.frame_rate:g} frames per second); "
                 "generated by tools/rd4_to_gltf/build_clip_library.py"]
    faces_txt += [f"rate {args.frame_rate:g}", "part BLUPI_rd_tex_Corps"]
    faces_txt += [f"face {f}" for f in faces]
    faces_txt += [f"clip {name} " + " ".join(str(i) for i in seq) for name, seq in clip_faces.items()]
    (args.out.parent / "faces.txt").write_text("\n".join(faces_txt) + "\n")
    print(f"Wrote {args.out}: {joints} joints, {meshes} meshes, {len(pose_clips)} clips, {len(faces)} faces; "
          f"props: {', '.join(props) or 'none'}")

    for w in dict.fromkeys(warnings):
        print("WARNING: " + w)
    if args.skip_check:
        return 0
    # Check every clip frame against its icon with the base scene's camera.
    model = vs.Model(args.out)
    extras = model.doc["asset"]["extras"]["rd4_to_gltf"]
    cam = vs.Camera(base_scene, "Jeu", extras)
    px = cam.zoom * vs.ICON / cam.frame_height / extras["scale"]
    lines = ["| Clip | Icons | Matched | Held | Mean IoU | Min IoU | Scenes |", "|---|---|---|---|---|---|---|"]
    for name, n, matched, held, scenes in report:
        if matched == 0:
            lines.append(f"| {name} | {n} | 0 | {held} | - | - | {scenes} |")
            continue
        model.select_clip(name)
        ious = []
        pos, right, up, back = cam.frame(base_scene.begin)
        for k, icon in enumerate(clips[name]):
            if icon < 0:
                continue
            color, cover = vs.render(model.skinned(k / args.frame_rate), pos, right, up, back,
                                     px * vs.SUPERSAMPLE, vs.ICON * vs.SUPERSAMPLE, cam.light(base_scene.begin),
                                     vs.SUPERSAMPLE)
            _c, a = vs.downsample(color, cover, vs.SUPERSAMPLE)
            r, c = divmod(icon, 10)
            spr = sheet[r * 60:(r + 1) * 60, c * 60:(c + 1) * 60, 3] > 127
            ious.append(vs.iou(a > 0.5, spr))
        lines.append(f"| {name} | {n} | {matched} | {held} | {np.mean(ious):.3f} | {np.min(ious):.3f} | {scenes} |")
    (args.out.parent / "clips.md").write_text("\n".join(lines) + "\n")
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
