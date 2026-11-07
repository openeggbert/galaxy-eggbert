#!/usr/bin/env python3
"""textures3d/grass_top.png: the grass seen from above, made from the original's own grass.

The grass tiles 107/108/109/157 of Content/icons/object-m.png show the bush of leaves on top of
the earth (side view); the 3D terrain needs the same bush seen from above. This takes the bush band
of tile 107 (the leaves, no earth, no outline), lays feathered patches of it at random places on a
64 x 64 canvas that wraps round (so it tiles seamlessly) and writes textures3d/grass_top.png.
Deterministic (seeded); replaces the first version, a procedural noise that looked like nothing
in the original (owner 2026-11-07, again after the black dot per tile: "the grass has a nonsensical texture from above").

    python3 tools/textures/grass_top.py            # writes textures3d/grass_top.png
"""
from __future__ import annotations

import random
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
TILE, GAP, COLUMNS = 64, 1, 20
ICON = 107
# The bush band of the tile: leaves only (rows 11-30, columns 6-57 of its 64 x 64).
BAND = (6, 11, 58, 31)
PATCH = 18
SEED = 107


def tile_pixels(sheet: Image.Image, icon: int) -> np.ndarray:
    col, row = icon % COLUMNS, icon // COLUMNS
    x, y = GAP + col * (TILE + GAP), GAP + row * (TILE + GAP)
    return np.asarray(sheet.crop((x, y, x + TILE, y + TILE)).convert("RGBA")).astype(np.float32)


def main() -> int:
    sheet = Image.open(REPO / "Content" / "icons" / "object-m.png")
    band = tile_pixels(sheet, ICON)[BAND[1]:BAND[3], BAND[0]:BAND[2]]
    rgb, alpha = band[..., :3], band[..., 3] > 200
    rng = random.Random(SEED)
    size = 64
    canvas = np.zeros((size, size, 3), np.float32)
    weight = np.zeros((size, size), np.float32)
    # Feather: a patch fades to nothing at its edges, so the patches blend.
    ramp = np.minimum(np.arange(PATCH) + 1, PATCH - np.arange(PATCH)).astype(np.float32)
    feather = np.minimum.outer(ramp, ramp)
    feather /= feather.max()
    # The band holds a few texels that are not leaf (the outline, a bud's dark dot): paint each over with
    # the mean of the leaf texels within 3, so no patch carries a dot that would repeat on every tile.
    leaf = (rgb[..., 1] > rgb[..., 0] + 8) & (rgb[..., 1] > rgb[..., 2]) & (rgb[..., 1] > 60)
    rgb = rgb.copy()
    for y, x in zip(*np.where(~leaf)):
        win = (slice(max(0, y - 3), y + 4), slice(max(0, x - 3), x + 4))
        if leaf[win].any():
            rgb[y, x] = rgb[win][leaf[win]].mean(axis=0)
    valid = [(py, px) for py in range(band.shape[0] - PATCH + 1) for px in range(band.shape[1] - PATCH + 1)]
    cover = np.zeros((size, size), np.int32)
    placed = 0
    # Until every texel is under at least 4 patches (a texel no patch reached came out black: a dot per tile).
    while placed < 90 or cover.min() < 4:
        py, px = rng.choice(valid)
        patch = rgb[py:py + PATCH, px:px + PATCH]
        if rng.random() < 0.5:
            patch = patch[:, ::-1]
        if rng.random() < 0.5:
            patch = patch[::-1]
        patch = np.rot90(patch, rng.randrange(4))   # leaves point every way, no horizontal streaks
        ox, oy = rng.randrange(size), rng.randrange(size)
        ys = (np.arange(PATCH) + oy) % size
        xs = (np.arange(PATCH) + ox) % size
        canvas[np.ix_(ys, xs)] += patch * feather[..., None]
        weight[np.ix_(ys, xs)] += feather
        cover[np.ix_(ys, xs)] += 1
        placed += 1
    weight = np.maximum(weight, 1e-3)
    out = (canvas / weight[..., None]).clip(0, 255).astype(np.uint8)
    Image.fromarray(out, "RGB").save(REPO / "textures3d" / "grass_top.png")
    print(f"wrote textures3d/grass_top.png from tile {ICON}'s leaves ({placed} patches)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
