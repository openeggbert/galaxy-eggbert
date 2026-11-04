#!/usr/bin/env python3
"""Which Ray Dream scene each terrain tile of object-m.png was rendered from, by its pixels.

The survey (``tools/rd4_to_gltf/survey.py``) ranks renders by silhouette IoU times a colour score.
For a tile that fills its whole square (planks, bricks, panels, shelves) every square scene has an
IoU of 1, and the colour score (one minus the mean colour difference) is swayed by the scenes'
lighting, so those tiles were often given the wrong scene (Q-012: the bookshelves 139-141 a ball on
a plate). Here every ``Decor`` scene is rendered with each of its orthographic cameras, every
distinct frame, straight and mirrored, at the tile size, and compared with each tile by its
structure: the normalized cross-correlation of the luminance over the pixels both cover (blind to
the scenes' brightness and contrast), times the IoU of the coverages.

Writes ``<out>/tiles.csv`` (icon, the best three matches: scene, frame, camera, mirrored, score,
ncc, iou) and ``<out>/tiles.png`` (per icon: the sprite, the best and the second render), and keeps
the renders in ``<out>/renders.npz`` (``--reuse`` skips rendering).

    python3 tools/terrain_relief/match_tiles.py --originals ../speedyblupi-data/originals \\
        --sheet ../mobile-eggbert/Content/icons/object-m.png --out build-probe/terrain-match
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "rd4_to_gltf"))

import rd4  # noqa: E402
import scene as sc  # noqa: E402
import scene_render  # noqa: E402
import survey  # noqa: E402

SIZE = 64


def render_scene(args: tuple[str, str]) -> list[tuple[str, int, str, np.ndarray, np.ndarray]]:
    """Every distinct frame of every orthographic camera of one scene: (scene, frame, camera, rgb, cov)."""
    originals, key = args
    out = []
    try:
        s = sc.read_scene(rd4.parse_file(Path(originals) / key))
    except Exception as e:  # noqa: BLE001
        print("skip", key, type(e).__name__, e, flush=True)
        return out
    for cam in (o.name for o, _p, _c, _g in sc.world_frames(s, None) if o.kind == "ca"):
        renderer = scene_render.SceneRenderer(s, cam)
        renderer.shadows = False
        try:
            ticks = renderer.frame_ticks()
            renderer.camera(ticks[0])
        except NotImplementedError:
            continue
        seen = set()
        for k, t in enumerate(ticks):
            try:
                rgb, cov = renderer.render(t, size=SIZE)
            except Exception as e:  # noqa: BLE001
                print("skip", key, cam, k, type(e).__name__, e, flush=True)
                break
            digest = hashlib.sha1(np.round(rgb * 255).astype(np.uint8).tobytes() + (cov > 0.5).tobytes()).hexdigest()
            if digest in seen:
                continue
            seen.add(digest)
            out.append((key, k, cam, rgb.astype(np.float32), cov.astype(np.float32)))
    print(key, len(out), flush=True)
    return out


def luminance(rgb: np.ndarray) -> np.ndarray:
    return rgb[..., 0] * 0.299 + rgb[..., 1] * 0.587 + rgb[..., 2] * 0.114


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--originals", required=True, type=Path)
    ap.add_argument("--sheet", required=True, type=Path)
    ap.add_argument("--out", required=True, type=Path)
    ap.add_argument("--folders", default="Decor", help="scene folders (and their subfolders) to render")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--reuse", action="store_true", help="reuse <out>/renders.npz")
    args = ap.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)
    cache = args.out / "renders.npz"
    if args.reuse and cache.exists():
        data = np.load(cache, allow_pickle=True)
        meta, rgbs, covs = list(data["meta"]), data["rgb"], data["cov"]
    else:
        keys = []
        for folder in args.folders.split(","):
            for p in sorted((args.originals / folder).rglob("*")):
                if p.suffix.lower() in (".rd4", ".rds"):
                    keys.append(str(p.relative_to(args.originals)))
        meta, rgbs, covs = [], [], []
        with ProcessPoolExecutor(max_workers=args.jobs) as pool:
            for renders in pool.map(render_scene, [(str(args.originals), k) for k in keys]):
                for key, frame, cam, rgb, cov in renders:
                    meta.append((key, frame, cam))
                    rgbs.append(rgb)
                    covs.append(cov)
        rgbs, covs = np.stack(rgbs), np.stack(covs)
        np.savez_compressed(cache, meta=np.array(meta, dtype=object), rgb=rgbs, cov=covs)
    # Straight and mirrored.
    rgb_all = np.concatenate([rgbs, rgbs[:, :, ::-1]])
    cov_all = np.concatenate([covs, covs[:, :, ::-1]]) > 0.5
    mirrored = np.concatenate([np.zeros(len(rgbs), bool), np.ones(len(rgbs), bool)])
    lum_all = luminance(rgb_all).astype(np.float32)

    grid = survey.GRIDS["object-m"]
    icon_rgb, icon_mask = survey.load_sheet(args.sheet, grid)
    rows = []
    sheet_rows = []
    for icon in range(len(icon_rgb)):
        mask = icon_mask[icon]
        if mask.sum() < 64:
            continue
        lum = luminance(icon_rgb[icon]).astype(np.float32)
        both = cov_all & mask[None]
        n = both.sum(axis=(1, 2))
        union = (cov_all | mask[None]).sum(axis=(1, 2))
        iou = n / np.maximum(union, 1)
        a = np.where(both, lum[None], 0.0)
        b = np.where(both, lum_all, 0.0)
        ma = a.sum(axis=(1, 2)) / np.maximum(n, 1)
        mb = b.sum(axis=(1, 2)) / np.maximum(n, 1)
        da = np.where(both, lum[None] - ma[:, None, None], 0.0)
        db = np.where(both, lum_all - mb[:, None, None], 0.0)
        ncc = (da * db).sum(axis=(1, 2)) / np.maximum(np.sqrt((da * da).sum(axis=(1, 2)) * (db * db).sum(axis=(1, 2))), 1e-9)
        ncc = np.where(n >= 64, ncc, -1.0)
        score = np.clip(ncc, 0.0, 1.0) * iou
        order = np.argsort(-score)
        best = []
        used_scenes = set()
        for j in order:
            key = meta[j % len(meta)][0]
            if key in used_scenes:
                continue
            used_scenes.add(key)
            best.append(j)
            if len(best) == 3:
                break
        row = [icon]
        for j in best:
            key, frame, cam = meta[j % len(meta)]
            row += [key, frame, cam, int(mirrored[j]), f"{score[j]:.3f}", f"{ncc[j]:.3f}", f"{iou[j]:.3f}"]
        rows.append(row)
        sheet_rows.append((icon, [icon_rgb[icon] * mask[..., None]] + [rgb_all[j] * cov_all[j][..., None] for j in best[:2]],
                           [float(score[j]) for j in best[:2]], [meta[j % len(meta)][0] for j in best[:2]]))
    with open(args.out / "tiles.csv", "w", newline="") as f:
        w = csv.writer(f)
        head = ["icon"]
        for k in (1, 2, 3):
            head += [f"scene{k}", f"frame{k}", f"camera{k}", f"mirrored{k}", f"score{k}", f"ncc{k}", f"iou{k}"]
        w.writerow(head)
        w.writerows(rows)
    # Contact sheet: 6 icons a row, each sprite | best | second with the scores.
    per_row, cell = 6, (3 * SIZE + 8, SIZE + 26)
    sheet = Image.new("RGB", (per_row * cell[0], ((len(sheet_rows) + per_row - 1) // per_row) * cell[1]), "black")
    draw = ImageDraw.Draw(sheet)
    for i, (icon, images, scores, scenes) in enumerate(sheet_rows):
        x, y = (i % per_row) * cell[0], (i // per_row) * cell[1]
        for k, im in enumerate(images):
            sheet.paste(Image.fromarray((np.clip(im, 0, 1) * 255).astype(np.uint8)), (x + k * SIZE, y))
        label = f"{icon} " + " ".join(f"{Path(s).stem}:{v:.2f}" for s, v in zip(scenes, scores))
        draw.text((x + 2, y + SIZE + 2), label[:34], fill="yellow")
    sheet.save(args.out / "tiles.png")
    print(f"{len(rows)} icons -> {args.out / 'tiles.csv'}, {args.out / 'tiles.png'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
