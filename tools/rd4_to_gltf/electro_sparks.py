#!/usr/bin/env python3
"""The electrode's sparks (object-m 238-242) as decals (owner 2026-11-04, Q-005).

Decor/electro.rd4 holds the ball on its spring; the sparks round the ball were painted on its
renders. This cuts them out of each sprite -- its warm (orange, red, yellow) texels, and the small
white ones outside the ball -- into <model>/sparks/<icon>.png (64 x 64, the rest clear), which the
game draws facing the camera over the model.

    python3 tools/rd4_to_gltf/electro_sparks.py <model directory>
"""
import sys
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]


def main() -> int:
    out = Path(sys.argv[1]) / "sparks"
    out.mkdir(parents=True, exist_ok=True)
    sheet = Image.open(REPO / "Content" / "icons" / "object-m.png").convert("RGBA")
    for icon in range(238, 243):
        r, c = divmod(icon, 20)
        x0, y0 = 1 + c * 65, 1 + r * 65
        t = np.asarray(sheet.crop((x0, y0, x0 + 64, y0 + 64))).astype(int)
        rgb, a = t[:, :, :3], t[:, :, 3]
        grey = (np.abs(rgb[:, :, 0] - rgb[:, :, 2]) < 24) & (np.abs(rgb[:, :, 0] - rgb[:, :, 1]) < 24) & (a >= 128) & (rgb.max(axis=2) < 235)
        ys, xs = np.nonzero(grey)
        cy, cx = ys.mean(), xs.mean()
        radius = max(np.hypot(ys - cy, xs - cx).max(), 1.0)
        yy, xx = np.mgrid[0:64, 0:64]
        outside = np.hypot(yy - cy, xx - cx) > radius + 1.0
        warm = (rgb[:, :, 0] > 170) & (rgb[:, :, 2] < 140) & (a >= 128)
        white = (rgb.min(axis=2) > 200) & (a >= 128) & outside
        mask = warm | white
        img = np.zeros((64, 64, 4), dtype=np.uint8)
        img[mask] = t[mask].astype(np.uint8)
        Image.fromarray(img, "RGBA").save(out / ("%d.png" % icon))
        print(icon, "spark texels", int(mask.sum()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
