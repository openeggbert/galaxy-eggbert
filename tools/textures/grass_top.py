#!/usr/bin/env python3
"""textures3d/grass_top.png: the grass seen from above, made from the original's own grass.

The grass tiles 107/108/109/157 of Content/icons/object-m.png show the bush of leaves on top of
the earth (side view); the 3D terrain needs the same bush seen from above. This takes the bush band
of tile 107 (the leaves, no earth, no outline), lays feathered patches of it at random places on a
64 x 64 canvas that wraps round (so it tiles seamlessly) and writes textures3d/grass_top.png.
Deterministic (seeded); replaces the first version, a procedural noise that looked like nothing
in the original (owner 2026-11-07: "the grass has a nonsensical texture from above").

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
    placed = 0
    while placed < 70:
        py = rng.randrange(0, band.shape[0] - PATCH + 1)
        px = rng.randrange(0, band.shape[1] - PATCH + 1)
        if alpha[py:py + PATCH, px:px + PATCH].mean() < 0.97:
            continue                                # no outline, no hole
        patch = rgb[py:py + PATCH, px:px + PATCH]
        # Leaves only: every texel green over red and blue, none of the dark outline or the earth.
        if not ((patch[..., 1] > patch[..., 0] + 8) & (patch[..., 1] > patch[..., 2]) & (patch[..., 1] > 60)).all():
            continue
        if rng.random() < 0.5:
            patch = patch[:, ::-1]
        if rng.random() < 0.5:
            patch = patch[::-1]
        ox, oy = rng.randrange(size), rng.randrange(size)
        ys = (np.arange(PATCH) + oy) % size
        xs = (np.arange(PATCH) + ox) % size
        canvas[np.ix_(ys, xs)] += patch * feather[..., None]
        weight[np.ix_(ys, xs)] += feather
        placed += 1
    weight = np.maximum(weight, 1e-3)
    out = (canvas / weight[..., None]).clip(0, 255).astype(np.uint8)
    Image.fromarray(out, "RGB").save(REPO / "textures3d" / "grass_top.png")
    print(f"wrote textures3d/grass_top.png from tile {ICON}'s leaves ({placed} patches)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
