#!/usr/bin/env python3
"""Cut the painted fuse sparks from element icons 253-255, as Q-005's electrode.

    python3 tools/rd4_to_gltf/dynamite_sparks.py avatars3d/objects/dynamite_dynamite

The 60-pixel cells stay registered to the model's sprite origin. Only the
yellow/white star, absent from unlit icon 252, survives; stick highlights and
the slightly different antialiased stick edges do not become part of the decal.
Uses the already approved Content assets; no Ray Dream sources are read.
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]


def main() -> int:
    out = Path(sys.argv[1]) / "sparks"
    out.mkdir(parents=True, exist_ok=True)
    sheet = Image.open(REPO / "Content/icons/element.png").convert("RGBA")

    def tile(icon):
        row, col = divmod(icon, 10)
        return np.asarray(sheet.crop((col * 60, row * 60, col * 60 + 60, row * 60 + 60)))

    unlit = tile(252)
    for icon in range(253, 256):
        pixels = tile(icon)
        # The painted star has exactly these two opaque colours. Comparing
        # against 252 excludes the white specular highlights on the sticks.
        star = ((pixels[:, :, 0] == 255) & (pixels[:, :, 1] == 255)
                & ((pixels[:, :, 2] == 0) | (pixels[:, :, 2] == 255))
                & (pixels[:, :, 3] == 255) & np.any(pixels != unlit, axis=2))
        assert star.any(), f"icon {icon}: no spark found"
        ys, xs = np.nonzero(star)
        # The renderer crops this shared rectangle and anchors its centre to
        # the shipped model's terminal fuse ring. Fail if the asset changes.
        assert xs.min() >= 38 and xs.max() < 55 and ys.min() >= 44 and ys.max() < 60
        decal = np.zeros_like(pixels)
        decal[star] = pixels[star]
        Image.fromarray(decal).save(out / f"{icon}.png")
        print(f"{icon}: {int(star.sum())} spark texels")
    return 0


if __name__ == "__main__":
    sys.exit(main())
