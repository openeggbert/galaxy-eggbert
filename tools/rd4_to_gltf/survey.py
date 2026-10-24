"""Survey of the Ray Dream archive: what the converter reads, and which scene each sprite came from.

``scenes`` reads every ``.rd4``/``.rds`` under ``--originals`` and writes ``scenes.csv``: whether
it parses, whether it contains Blupi, its object and frame counts, its master kinds, the masters
the converter cannot build (``tag:exception``) and its cameras.

``icons`` renders every parsed scene without Blupi (of ``--folders``) with each of its cameras,
the first ``--frames`` frames, straight and mirrored, and keeps for every icon of ``--sheet`` the
best render by IoU x colour score; it writes ``<sheet>_icons.csv`` (icon, score, IoU, colour,
scene, frame, camera, mirrored). Cameras the renderer does not take (perspective) are skipped.

    python3 tools/rd4_to_gltf/survey.py scenes --originals ../speedyblupi-data/originals \\
        --out build-probe/rd4-survey
    python3 tools/rd4_to_gltf/survey.py icons --originals ../speedyblupi-data/originals \\
        --sheet ../mobile-eggbert/Content/icons/element.png --out build-probe/rd4-survey
"""

from __future__ import annotations

import argparse
import csv
from collections import Counter
from pathlib import Path

import numpy as np

import geometry
import match_icons
import rd4
import scene as sc
import scene_render


def folder_of(path: Path, originals: Path) -> str:
    return str(path.parent.relative_to(originals)) if path.parent != originals else "."


def survey_scenes(originals: Path, out: Path) -> None:
    out.mkdir(parents=True, exist_ok=True)
    files = sorted(p for p in originals.rglob("*") if p.suffix.lower() in (".rd4", ".rds"))
    with open(out / "scenes.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["folder", "file", "status", "blupi", "objects", "geometric", "frames", "animated",
                    "master_kinds", "unsupported", "cameras"])
        for p in files:
            folder = folder_of(p, originals)
            try:
                root = rd4.parse_file(p)
                if root.chunk("sc") is None:
                    w.writerow([folder, p.name, "no scene"] + [""] * 8)
                    continue
                s = sc.read_scene(root)
            except Exception as e:  # noqa: BLE001 -- the survey records every failure
                w.writerow([folder, p.name, f"not read: {type(e).__name__}"] + [""] * 8)
                continue
            frames = sc.world_frames(s, None)
            objs = [o for o, _p, _c, _g in frames if o.kind not in ("ca", "li") and o is not s.universe]
            kinds = Counter(m.tag for m in s.masters.values())
            unsupported = set()
            for m in s.masters.values():
                try:
                    geometry.canonical_mesh(m)
                except Exception as e:  # noqa: BLE001
                    unsupported.add(f"{m.tag}:{type(e).__name__}")
            animated = any(
                o.orie_track is not None or any(t is not None for t in o.pos_tracks)
                or (o.joint is not None and (o.joint.ball_track is not None
                                             or any(t is not None for t in getattr(o.joint, "angle_tracks", []) or [])))
                for o in objs)
            n_frames = int(round((s.end - s.begin) / sc.TICKS_PER_SECOND * s.fps)) if s.fps else 0
            cams = [o.name for o, _p, _c, _g in frames if o.kind == "ca"]
            w.writerow([folder, p.name, "ok", int(s.body() is not None), len(objs),
                        sum(1 for o in objs if o.master), n_frames, int(animated),
                        " ".join(f"{k}={v}" for k, v in sorted(kinds.items())),
                        " ".join(sorted(unsupported)), "|".join(cams)])
    print(f"{len(files)} scenes -> {out / 'scenes.csv'}")


def survey_icons(originals: Path, sheet: Path, out: Path, folders: list[str], max_frames: int) -> None:
    rows = [r for r in csv.DictReader(open(out / "scenes.csv"))
            if r["status"] == "ok" and r["blupi"] == "0" and r["folder"] in folders]
    icon_rgb, icon_mask = match_icons.load_icons(sheet)
    best: list[tuple | None] = [None] * len(icon_rgb)
    for r in rows:
        path = originals / r["file"] if r["folder"] == "." else originals / r["folder"] / r["file"]
        s = sc.read_scene(rd4.parse_file(path))
        for cam in (o.name for o, _p, _c, _g in sc.world_frames(s, None) if o.kind == "ca"):
            renderer = scene_render.SceneRenderer(s, cam)
            renderer.shadows = False
            try:
                ticks = renderer.frame_ticks()
                renderer.camera(ticks[0])
            except NotImplementedError:
                continue
            try:
                for k, t in enumerate(ticks[:max_frames]):
                    rgb, cov = renderer.render(t)
                    for mirrored in (False, True):
                        rg, cv = (rgb[:, ::-1], cov[:, ::-1]) if mirrored else (rgb, cov)
                        iou, col = match_icons.scores(rg, cv, icon_rgb, icon_mask)
                        score = iou * col
                        current = np.array([b[0] if b else -1.0 for b in best])
                        for i in np.nonzero(score > current)[0]:
                            best[i] = (score[i], iou[i], col[i], f"{r['folder']}/{r['file']}", k, cam, mirrored)
            except Exception as e:  # noqa: BLE001 -- keep surveying the other scenes
                print("skip", r["file"], cam, type(e).__name__, e)
        print(r["file"], flush=True)
    with open(out / f"{sheet.stem}_icons.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["icon", "score", "iou", "colour", "scene", "frame", "camera", "mirrored"])
        for i, b in enumerate(best):
            if not icon_mask[i].any():
                continue
            w.writerow([i] + ([f"{b[0]:.3f}", f"{b[1]:.3f}", f"{b[2]:.3f}", b[3], b[4], b[5], int(b[6])] if b else []))
    print(f"-> {out / (sheet.stem + '_icons.csv')}")


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("what", choices=("scenes", "icons"))
    ap.add_argument("--originals", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    ap.add_argument("--sheet", type=Path)
    ap.add_argument("--folders", default="Blupi,.,Element,Icon",
                    help="comma-separated folders whose scenes the icons survey renders")
    ap.add_argument("--frames", type=int, default=24, help="frames rendered per scene and camera")
    a = ap.parse_args()
    if a.what == "scenes":
        survey_scenes(a.originals, a.out)
    else:
        if a.sheet is None:
            ap.error("icons needs --sheet")
        survey_icons(a.originals, a.sheet, a.out, a.folders.split(","), a.frames)


if __name__ == "__main__":
    main()
