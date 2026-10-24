#!/usr/bin/env python3
"""Match every rendered frame of the Ray Dream Blupi scenes against every sprite icon.

Renders each frame of each scene that contains Blupi (scene_render.py, the scene's own game
camera), compares it with all 60x60 icons of the sprite sheet, as rendered and mirrored (the
sheet's left-facing icons are mirrored copies), and writes:

* ``icons.csv`` — per icon: the best scene frame and its scores, the runner-up, and the best
  frame as rendered (not mirrored), the one a 3D pose can be taken from;
* ``frames.csv`` — per scene frame: the best icon;
* ``report.md`` — coverage per icon and per action of mobile-eggbert's ``table_blupi``;
* ``icons/NNN.png`` — the icon next to its best render, for checking by eye.

Scores: ``iou`` of the silhouettes, ``colour`` = 1 - mean absolute RGB difference where both
cover, ``score`` = iou x colour.

    python3 match_icons.py --scenes ../speedyblupi-data/originals/Blupi \\
        --sheet ../mobile-eggbert/Content/icons/blupi.png \\
        --reference ../mobile-eggbert --out build-probe/rd4-blupi/match
"""

from __future__ import annotations

import argparse
import csv
import re
import sys
import time
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import rd4  # noqa: E402
import scene as sc  # noqa: E402
import scene_render  # noqa: E402

ICON = 60
CONFIDENT = 0.85
PROBABLE = 0.70


def load_icons(sheet: Path) -> tuple[np.ndarray, np.ndarray]:
    img = np.array(Image.open(sheet).convert("RGBA")).astype(np.float64) / 255.0
    rows, cols = img.shape[0] // ICON, img.shape[1] // ICON
    tiles = img.reshape(rows, ICON, cols, ICON, 4).transpose(0, 2, 1, 3, 4).reshape(-1, ICON, ICON, 4)
    return tiles[..., :3], tiles[..., 3] > 0.5


def table_blupi(reference: Path) -> tuple[dict[int, list[int]], dict[int, str]]:
    """{action id: icon sequence} from Tables.cpp and {action id: name} from BlupiAction.hpp."""
    text = (reference / "src/WindowsPhoneSpeedyBlupi/Tables.cpp").read_text(errors="replace")
    start = text.index("const shortcs Tables::table_blupi[")
    body = text[text.index("{", start) + 1:text.index("};", start)]
    body = re.sub(r"/\*.*?\*/", "", re.sub(r"//[^\n]*", "", body), flags=re.S)
    nums = [int(x) for x in re.findall(r"-?\d+", body)]
    actions: dict[int, list[int]] = {}
    i = 0
    while i + 3 <= len(nums):
        action, count = nums[i], nums[i + 1]
        if i > 0 and action == 0 and count <= 0:
            break
        actions.setdefault(action, nums[i + 3:i + 3 + count])
        i += 3 + count
    names = {}
    enum = (reference / "include/WindowsPhoneSpeedyBlupi/def/BlupiAction.hpp").read_text(errors="replace")
    for name, value in re.findall(r"^\s+(\w+) = (\d+),", enum, flags=re.M):
        names[int(value)] = name
    return actions, names


def scores(rgb: np.ndarray, cover: np.ndarray, icon_rgb: np.ndarray, icon_mask: np.ndarray):
    """(iou, colour) of one render against every icon."""
    mask = cover > 0.5
    inter = (icon_mask & mask).sum(axis=(1, 2))
    union = (icon_mask | mask).sum(axis=(1, 2))
    iou = np.where(union > 0, inter / np.maximum(union, 1), 0.0)
    both = icon_mask & mask
    diff = np.abs(icon_rgb - rgb[None]).mean(axis=-1)
    colour = 1.0 - np.where(both.sum(axis=(1, 2)) > 0,
                            (diff * both).sum(axis=(1, 2)) / np.maximum(both.sum(axis=(1, 2)), 1), 1.0)
    return iou, colour


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--scenes", required=True, type=Path)
    ap.add_argument("--sheet", required=True, type=Path)
    ap.add_argument("--reference", required=True, type=Path, help="mobile-eggbert checkout (read only)")
    ap.add_argument("--out", required=True, type=Path)
    args = ap.parse_args(argv)
    args.out.mkdir(parents=True, exist_ok=True)
    (args.out / "icons").mkdir(exist_ok=True)

    icon_rgb, icon_mask = load_icons(args.sheet)
    n_icons = len(icon_rgb)
    frames = []       # (scene, frame index, tick, rgb, cover)
    problems = []
    t_start = time.time()
    for path in sorted(list(args.scenes.glob("*.rd4")) + list(args.scenes.glob("*.rds"))):
        try:
            root = rd4.parse_file(path)
            if root.chunk("sc") is None:
                continue
            scene = sc.read_scene(root)
        except Exception as e:  # noqa: BLE001 - report every unreadable scene and go on
            problems.append(f"{path.name}: not read ({e})")
            continue
        if scene.find("Corps") is None:
            continue
        renderer = scene_render.SceneRenderer(scene)
        try:
            for k, tick in enumerate(renderer.frame_ticks()):
                rgb, cover = renderer.render(tick)
                frames.append((path.name, k, tick, rgb, cover))
        except Exception as e:  # noqa: BLE001
            problems.append(f"{path.name}: not rendered ({e})")
            continue
        for master, why in renderer.skipped.items():
            problems.append(f"{path.name}: master {master!r} skipped ({why})")
        print(f"{path.name}: {len(renderer.frame_ticks())} frame(s)", flush=True)
    print(f"rendered {len(frames)} frames in {time.time() - t_start:.0f} s", flush=True)

    # Score every frame, as rendered and mirrored, against every icon.
    best = [None] * n_icons      # (score, iou, colour, frame index, mirrored)
    second = [None] * n_icons
    plain = [None] * n_icons     # best as rendered, not mirrored
    frame_best = []
    for fi, (_name, _k, _tick, rgb, cover) in enumerate(frames):
        top = None
        for mirrored in (False, True):
            r = rgb[:, ::-1] if mirrored else rgb
            c = cover[:, ::-1] if mirrored else cover
            iou, colour = scores(r, c, icon_rgb, icon_mask)
            score = iou * colour
            for icon in range(n_icons):
                cand = (score[icon], iou[icon], colour[icon], fi, mirrored)
                if not mirrored and (plain[icon] is None or cand[0] > plain[icon][0]):
                    plain[icon] = cand
                if best[icon] is None or cand[0] > best[icon][0]:
                    if best[icon] is not None and (second[icon] is None or best[icon][0] > second[icon][0]):
                        second[icon] = best[icon]
                    best[icon] = cand
                elif second[icon] is None or cand[0] > second[icon][0]:
                    second[icon] = cand
            j = int(np.argmax(score))
            if top is None or score[j] > top[0]:
                top = (score[j], iou[j], colour[j], j, mirrored)
        frame_best.append(top)

    def label(entry):
        _s, _i, _c, fi, mirrored = entry
        name, k, tick, _rgb, _cov = frames[fi]
        return f"{name}#{k}{' (mirrored)' if mirrored else ''}"

    with open(args.out / "icons.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["icon", "status", "score", "iou", "colour", "scene", "frame", "tick", "mirrored",
                    "runner_up", "runner_up_score", "plain_scene", "plain_frame", "plain_tick",
                    "plain_score"])
        for icon in range(n_icons):
            b = best[icon]
            if not icon_mask[icon].any():
                w.writerow([icon, "empty"] + [""] * 13)
                continue
            name, k, tick, _rgb, _cov = frames[b[3]]
            status = "confident" if b[0] >= CONFIDENT else "probable" if b[0] >= PROBABLE else "unmatched"
            pn, pk, pt, _r, _c = frames[plain[icon][3]]
            w.writerow([icon, status, f"{b[0]:.3f}", f"{b[1]:.3f}", f"{b[2]:.3f}", name, k, int(tick),
                        int(b[4]), label(second[icon]) if second[icon] else "",
                        f"{second[icon][0]:.3f}" if second[icon] else "",
                        pn, pk, int(pt), f"{plain[icon][0]:.3f}"])
    with open(args.out / "frames.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["scene", "frame", "tick", "best_icon", "score", "iou", "colour", "mirrored"])
        for (name, k, tick, _rgb, _cov), top in zip(frames, frame_best):
            w.writerow([name, k, int(tick), top[3], f"{top[0]:.3f}", f"{top[1]:.3f}", f"{top[2]:.3f}",
                        int(top[4])])

    # Montage per icon: sprite | best render.
    bg = np.array([40, 90, 140]) / 255.0
    for icon in range(n_icons):
        if not icon_mask[icon].any():
            continue
        _s, _i, _c, fi, mirrored = best[icon]
        _n, _k, _t, rgb, cover = frames[fi]
        if mirrored:
            rgb, cover = rgb[:, ::-1], cover[:, ::-1]
        a = icon_mask[icon][..., None]
        left = icon_rgb[icon] * a + bg * (1 - a)
        right = rgb * cover[..., None] + bg * (1 - cover[..., None])
        tile = np.concatenate([left, right], axis=1)
        Image.fromarray((tile.clip(0, 1) * 255).astype(np.uint8)).resize((240, 120), Image.NEAREST) \
            .save(args.out / "icons" / f"{icon:03d}.png")

    # Report.
    actions, names = table_blupi(args.reference)
    status = {}
    for icon in range(n_icons):
        if not icon_mask[icon].any():
            status[icon] = "empty"
        else:
            status[icon] = "confident" if best[icon][0] >= CONFIDENT else \
                "probable" if best[icon][0] >= PROBABLE else "unmatched"
    used = sorted({i for seq in actions.values() for i in seq if 0 <= i < n_icons})
    lines = ["# Sprite icons vs. Ray Dream scene frames", "",
             f"{len(frames)} frames rendered from the Blupi scenes; {n_icons} icons in "
             f"`{args.sheet.name}`, {len(used)} of them used by `table_blupi`.", "",
             f"Status: confident = score >= {CONFIDENT}, probable >= {PROBABLE} "
             "(score = silhouette IoU x colour similarity).", ""]
    counts = {k: sum(1 for i in used if status[i] == k) for k in ("confident", "probable", "unmatched")}
    lines.append(f"Icons used by actions: {counts['confident']} confident, {counts['probable']} probable, "
                 f"{counts['unmatched']} unmatched.")
    lines += ["", "## Actions", "", "| Action | Icons | Confident | Probable | Unmatched | Scenes |",
              "|---|---|---|---|---|---|"]
    for aid in sorted(actions):
        seq = [i for i in actions[aid] if 0 <= i < n_icons]
        distinct = sorted(set(seq))
        c = [sum(1 for i in distinct if status[i] == k) for k in ("confident", "probable", "unmatched")]
        scenes = sorted({frames[best[i][3]][0] for i in distinct if status[i] != "unmatched"})
        lines.append(f"| {aid} {names.get(aid, '?')} | {len(distinct)} | {c[0]} | {c[1]} | {c[2]} | "
                     f"{', '.join(scenes)} |")
    unmatched = [i for i in used if status[i] == "unmatched"]
    lines += ["", "## Unmatched icons used by actions", "",
              ", ".join(str(i) for i in unmatched) or "none", "", "## Problems", ""]
    lines += [f"- {p}" for p in problems] or ["none"]
    (args.out / "report.md").write_text("\n".join(lines) + "\n")
    print(f"wrote {args.out}/report.md, icons.csv, frames.csv, icons/*.png")
    print("\n".join(lines[:12]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
