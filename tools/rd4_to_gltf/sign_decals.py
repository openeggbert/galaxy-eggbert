#!/usr/bin/env python3
"""The signs' painted faces as decals (owner's decision MR-0006: the world signs as their Ray Dream
model with the digit on it).

The sign scenes (Decor/world1.rd4, world2a.rd4, world2b.rd4) hold the sign but not what was
painted on the renders: the mission's digit (174-181), the world's picture (158-165, 309, 411-415
and their completed twins). Mission digits 174-181 are red paint: recover its coverage from the
colour blend with the blue face, then supersample its contour at 8x the sprite resolution. The
picture signs keep the original group-median difference extraction.

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
DIGIT_REGION = (22, 12, 42, 35)  # existing face placement, in the 64-pixel sprite
DIGIT_SCALE = 8


def digit_coverage(rgb: np.ndarray) -> np.ndarray:
    """Unmix the red paint in Mobile Eggbert's object-m.png icons 174-181.

    The solid ink is (255, 0, 0). The blue face's G-R is nearly constant across each row
    (about 108/255); sample it just outside the painted rectangle and interpolate across.
    For C = alpha * red + (1-alpha) * blue, R-G = alpha - (1-alpha) * (blue.G-blue.R).
    This also recovers faint red edge pixels that are still predominantly blue. Discard only
    the <=2/255 colour-distance residual from the face's shading/8-bit rounding, not those
    edge pixels. No other digit participates in the mask.
    """
    x0, y0, x1, y1 = DIGIT_REGION
    chroma = rgb[..., 1] - rgb[..., 0]
    t = np.arange(1, x1 - x0 + 1, dtype=np.float64) / (x1 - x0 + 1)
    blue = chroma[y0:y1, x0 - 1, None] * (1 - t) + chroma[y0:y1, x1, None] * t
    excess = blue - chroma[y0:y1, x0:x1]
    return np.where(excess > 2 / 255.0, np.clip(excess / (1 + blue), 0, 1), 0)


def digit_image(coverage: np.ndarray) -> Image.Image:
    """Reconstruct the half-coverage contour, with sub-texel antialiasing at the new size.

    Bicubic interpolation uses the sprite's fractional edge coverage to locate the contour,
    rather than enlarging square pixels. Rasterize at four samples per output axis and area
    average: a crisp edge with partial alpha, without an eight-texel-wide blur or ringing halo.
    Padding makes the outside transparent even for a stroke reaching the crop boundary.
    Store source red premultiplied by coverage: the game's decal pass uses XNA AlphaBlend
    (One, InverseSourceAlpha). Transparent RGB must therefore be zero, or the entire rectangle
    adds red to the sign. Filtering premultiplied colour/alpha together avoids edge halos.
    Geometry/UV placement is unchanged.
    """
    samples = 4
    scale = DIGIT_SCALE * samples
    field = Image.fromarray(np.pad(coverage, 2).astype(np.float32))
    field = field.resize((field.width * scale, field.height * scale), Image.Resampling.BICUBIC)
    inside = np.asarray(field)[2 * scale:-2 * scale, 2 * scale:-2 * scale] >= 0.5
    height, width = coverage.shape
    alpha = inside.reshape(height * DIGIT_SCALE, samples, width * DIGIT_SCALE, samples).mean(axis=(1, 3))
    rgba = np.zeros((*alpha.shape, 4), dtype=np.uint8)
    rgba[..., 3] = (alpha * 255).round().astype(np.uint8)
    rgba[..., 0] = rgba[..., 3]
    return Image.fromarray(rgba)


def front_depth(glb: Path, region: tuple[int, int, int, int]) -> float:
    """The model's front (largest Z toward the sprite camera) over the decal's region (x0, y0, x1,
    y1 in the icon's 64-pixel square; model units are blocks of 64 pixels, the icon's centre the
    origin, +Y up): the surface's Z where it covers a grid of points of the region, the largest.
    The whole model's front stood the treasure doors' number plate 0.09 of a block in front of
    their golden post (its base reaches further out than the shaft behind the plate) and the mission
    signs' digit 0.2 in front of their board: owner 2026-11-05, MR-0008. (Vertices alone would not
    do: a board's lie at its corners, outside the painted region.)"""
    data = glb.read_bytes()
    length = struct.unpack("<I", data[12:16])[0]
    doc = json.loads(data[20:20 + length])
    offset = 20 + length
    blob_length = struct.unpack("<I", data[offset:offset + 4])[0]
    blob = data[offset + 8:offset + 8 + blob_length]

    def accessor(index: int, components: int) -> np.ndarray:
        acc = doc["accessors"][index]
        view = doc["bufferViews"][acc["bufferView"]]
        dtype = {5126: np.float32, 5125: np.uint32, 5123: np.uint16, 5121: np.uint8}[acc["componentType"]]
        arr = np.frombuffer(blob, dtype=dtype, count=acc["count"] * components,
                            offset=view.get("byteOffset", 0) + acc.get("byteOffset", 0))
        return arr.reshape(-1, components) if components > 1 else arr

    x0, y0, x1, y1 = region
    xs = np.linspace((x0 - 32) / 64.0, (x1 - 32) / 64.0, 9)
    ys = np.linspace((32 - y1) / 64.0, (32 - y0) / 64.0, 9)
    px, py = (a.ravel() for a in np.meshgrid(xs, ys))
    best = np.full(px.shape, -np.inf)
    for mesh in doc["meshes"]:
        for prim in mesh["primitives"]:
            pos = accessor(prim["attributes"]["POSITION"], 3).astype(np.float64)
            tris = accessor(prim["indices"], 1).astype(np.int64).reshape(-1, 3)
            a, b, c = pos[tris[:, 0]], pos[tris[:, 1]], pos[tris[:, 2]]
            den = (b[:, 1] - c[:, 1]) * (a[:, 0] - c[:, 0]) + (c[:, 0] - b[:, 0]) * (a[:, 1] - c[:, 1])
            ok = np.abs(den) > 1e-12
            a, b, c, den = a[ok], b[ok], c[ok], den[ok]
            for k in range(len(px)):
                l1 = ((b[:, 1] - c[:, 1]) * (px[k] - c[:, 0]) + (c[:, 0] - b[:, 0]) * (py[k] - c[:, 1])) / den
                l2 = ((c[:, 1] - a[:, 1]) * (px[k] - c[:, 0]) + (a[:, 0] - c[:, 0]) * (py[k] - c[:, 1])) / den
                l3 = 1.0 - l1 - l2
                inside = (l1 >= 0) & (l2 >= 0) & (l3 >= 0)
                if inside.any():
                    z = l1[inside] * a[inside, 2] + l2[inside] * b[inside, 2] + l3[inside] * c[inside, 2]
                    best[k] = max(best[k], float(z.max()))
    if not np.isfinite(best).any():
        raise ValueError(f"{glb}: no surface behind the decal's region")
    return float(best[np.isfinite(best)].max())


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
    digits = not args.plate and bool(icons) and all(174 <= i <= 181 for i in icons)
    if digits:
        painted = [digit_coverage(rgb[i]) for i in icons]
    elif args.plate:
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
    if digits:
        x0, y0, x1, y1 = DIGIT_REGION
    else:
        ys, xs = np.where(union)
        x0, x1, y0, y1 = int(xs.min()), int(xs.max()) + 1, int(ys.min()), int(ys.max()) + 1
    depth = front_depth(args.glb, (x0, y0, x1, y1)) if args.glb else float(args.depth)
    out = args.model / "decals"
    out.mkdir(parents=True, exist_ok=True)
    lines = [f"# {args.model.name}: the painted faces of icons {args.icons} (tools/rd4_to_gltf/sign_decals.py)"]
    for icon, mine in zip(icons, painted):
        if digits:
            digit_image(mine).save(out / f"{icon}.png")
            lines.append(f"decal {icon} decals/{icon}.png {x0} {y0} {x1} {y1} {depth:.4f}")
            continue
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
