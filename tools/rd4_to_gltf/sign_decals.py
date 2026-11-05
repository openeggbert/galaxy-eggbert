#!/usr/bin/env python3
"""The signs' painted faces as decals (owner's decision MR-0006: the world signs as their Ray Dream
model with the digit on it).

The sign scenes (Decor/world1.rd4, world2a.rd4, world2b.rd4) hold the sign but not what was
painted on the renders: the mission's digit (174-181), the world's picture (158-165, 309, 411-415
and their completed twins). Within a group of icons drawn from one scene, the pixels where the
icons differ are the painting; the per-pixel median over the group is the bare sign. Each icon's
decal is its sprite in that region, opaque where it differs from the bare sign.

With ``--plate`` the painting is a light grey plate carrying a number (the treasure doors 421-439
on the golden post 183's scene, Decor/porte2.rd4): each decal is the plate's rectangle, opaque.

Writes ``<model dir>/decals/<icon>.png`` and ``<model dir>/decals.txt``:
``decal <icon> <png> <x0> <y0> <x1> <y1> <z>`` -- the region in the icon's 64-pixel square
(pixels, right and down from its top-left) and the depth of the model's front behind that region (model
units, +Z toward the camera). The game puts the decal on the front and, mirrored, on the back.

    python3 tools/rd4_to_gltf/sign_decals.py --sheet ../mobile-eggbert/Content/icons/object-m.png \\
        --model avatars3d/objects/sign174_world1 --icons 174-181
"""
from __future__ import annotations

import argparse
import json
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
import survey  # noqa: E402
from build_object_library import parse_icons  # noqa: E402

THRESHOLD = 0.12  # colour distance (0..sqrt 3) from the bare sign counted as painted


def front_depth(glb: Path, region: tuple[int, int, int, int]) -> float:
    """The largest Z of the model's vertices (its front, toward the sprite camera) behind the decal's
    region (x0, y0, x1, y1 in the icon's 64-pixel square: model units are blocks of 64 pixels, the
    icon's centre the origin, +Y up). The whole model's front stood the treasure doors' number
    plate 0.09 of a block in front of their golden post (the post's base reaches further out than
    its shaft behind the plate): owner 2026-11-05, MR-0008."""
    data = glb.read_bytes()
    length = struct.unpack("<I", data[12:16])[0]
    doc = json.loads(data[20:20 + length])
    offset = 20 + length
    blob_length = struct.unpack("<I", data[offset:offset + 4])[0]
    blob = data[offset + 8:offset + 8 + blob_length]
    x0, y0, x1, y1 = region
    lo_x, hi_x = (x0 - 32) / 64.0, (x1 - 32) / 64.0
    lo_y, hi_y = (32 - y1) / 64.0, (32 - y0) / 64.0
    best = None
    for mesh in doc["meshes"]:
        for prim in mesh["primitives"]:
            acc = doc["accessors"][prim["attributes"]["POSITION"]]
            view = doc["bufferViews"][acc["bufferView"]]
            pos = np.frombuffer(blob, dtype=np.float32, count=acc["count"] * 3,
                                offset=view.get("byteOffset", 0) + acc.get("byteOffset", 0)).reshape(-1, 3)
            inside = (pos[:, 0] >= lo_x) & (pos[:, 0] <= hi_x) & (pos[:, 1] >= lo_y) & (pos[:, 1] <= hi_y)
            if inside.any():
                z = float(pos[inside, 2].max())
                best = z if best is None else max(best, z)
    if best is None:
        raise ValueError(f"{glb}: no vertex behind the decal's region")
    return best


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--sheet", required=True, type=Path)
    ap.add_argument("--model", required=True, type=Path, help="the exported model directory")
    ap.add_argument("--glb", type=Path, help="the converter's model.glb (default: none, read --depth)")
    ap.add_argument("--depth", type=float, help="the model's front depth when no --glb is given")
    ap.add_argument("--icons", required=True)
    ap.add_argument("--plate", action="store_true",
                    help="the painting is a light grey plate with its number (the treasure doors): "
                         "each decal is the plate's whole rectangle, opaque")
    args = ap.parse_args(argv)
    icons = parse_icons(args.icons)
    rgb, mask = survey.load_sheet(args.sheet, survey.GRIDS["object-m"])
    if args.plate:
        # The plate: light grey pixels (channels within 0.08, value over 0.55), the same rectangle
        # on every icon; the number painted on it differs.
        # The rows and columns where most icons are grey hold the plate, not a highlight on a ball.
        grey = np.mean([(np.ptp(rgb[i], axis=2) < 0.08) & (rgb[i].min(axis=2) > 0.55) & mask[i]
                        for i in icons], axis=0) > 0.5
        rows = np.where(grey.sum(axis=1) >= 8)[0]
        cols = np.where(grey[rows.min():rows.max() + 1].sum(axis=0) >= 4)[0]
        box = np.zeros_like(grey)
        box[rows.min():rows.max() + 1, cols.min():cols.max() + 1] = True
        painted = [box & mask[i] for i in icons]
        union = box
    else:
        stack = np.stack([rgb[i] for i in icons])
        bare = np.median(stack, axis=0)
        painted = [np.linalg.norm(rgb[i] - bare, axis=2) > THRESHOLD for i in icons]
        union = np.any(painted, axis=0) & np.all([mask[i] for i in icons], axis=0)
    ys, xs = np.where(union)
    x0, x1, y0, y1 = int(xs.min()), int(xs.max()) + 1, int(ys.min()), int(ys.max()) + 1
    depth = front_depth(args.glb, (x0, y0, x1, y1)) if args.glb else float(args.depth)
    out = args.model / "decals"
    out.mkdir(parents=True, exist_ok=True)
    lines = [f"# {args.model.name}: the painted faces of icons {args.icons} (tools/rd4_to_gltf/sign_decals.py)"]
    for icon, mine in zip(icons, painted):
        crop = rgb[icon][y0:y1, x0:x1]
        alpha = mine[y0:y1, x0:x1]
        if not alpha.any():
            continue  # a sign with nothing painted (the demo portal's)
        image = np.dstack([crop, alpha.astype(np.float64)])
        Image.fromarray((image * 255).round().astype(np.uint8), "RGBA").save(out / f"{icon}.png")
        lines.append(f"decal {icon} decals/{icon}.png {x0} {y0} {x1} {y1} {depth:.4f}")
    (args.model / "decals.txt").write_text("\n".join(lines) + "\n")
    print(f"{args.model}: {len(lines) - 1} decals, region x {x0}-{x1} y {y0}-{y1}, front at z {depth:.3f}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
