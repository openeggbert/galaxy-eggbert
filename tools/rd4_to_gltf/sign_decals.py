#!/usr/bin/env python3
"""The signs' painted faces as decals (owner's decision MR-0006: the world signs as their Ray Dream
model with the digit on it).

The sign scenes (Decor/world1.rd4, world2a.rd4, world2b.rd4) hold the sign but not what was
painted on the renders: the mission's digit (174-181), the world's picture (158-165, 309, 411-415
and their completed twins). Within a group of icons drawn from one scene, the pixels where the
icons differ are the painting; the per-pixel median over the group is the bare sign. Each icon's
decal is its sprite in that region, opaque where it differs from the bare sign.

Writes ``<model dir>/decals/<icon>.png`` and ``<model dir>/decals.txt``:
``decal <icon> <png> <x0> <y0> <x1> <y1> <z>`` -- the region in the icon's 64-pixel square
(pixels, right and down from its top-left) and the depth of the model's front (model units, +Z
toward the camera). The game puts the decal on the front and, mirrored, on the back.

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


def front_depth(glb: Path) -> float:
    """The largest Z of the model's vertices (its front, toward the sprite camera)."""
    data = glb.read_bytes()
    length = struct.unpack("<I", data[12:16])[0]
    doc = json.loads(data[20:20 + length])
    return max(doc["accessors"][p["attributes"]["POSITION"]]["max"][2]
               for m in doc["meshes"] for p in m["primitives"])


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--sheet", required=True, type=Path)
    ap.add_argument("--model", required=True, type=Path, help="the exported model directory")
    ap.add_argument("--glb", type=Path, help="the converter's model.glb (default: none, read --depth)")
    ap.add_argument("--depth", type=float, help="the model's front depth when no --glb is given")
    ap.add_argument("--icons", required=True)
    args = ap.parse_args(argv)
    icons = parse_icons(args.icons)
    rgb, mask = survey.load_sheet(args.sheet, survey.GRIDS["object-m"])
    stack = np.stack([rgb[i] for i in icons])
    bare = np.median(stack, axis=0)
    painted = [np.linalg.norm(rgb[i] - bare, axis=2) > THRESHOLD for i in icons]
    union = np.any(painted, axis=0) & np.all([mask[i] for i in icons], axis=0)
    ys, xs = np.where(union)
    x0, x1, y0, y1 = int(xs.min()), int(xs.max()) + 1, int(ys.min()), int(ys.max()) + 1
    depth = front_depth(args.glb) if args.glb else float(args.depth)
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
