#!/usr/bin/env python3
"""The 2x terrain sheet against the 1x one: no cube tile may carry a line the 1x tile does not have.

Each opaque tile (under a quarter clear, the cubes: TileCoverage.hpp) of Content/icons2x/object-m.png
is box-reduced to 64x64 and compared with Content/icons/object-m.png's: the mean |RGB| difference
over the texels opaque in both, per row and per column (those with at least 8 such texels). A
rendered cell that caught a neighbour's edge or shadow shows as one row or column far off (the
owner's "thick lines", 2026-11-10: up to 96 before ray's builder rejected them; 30 is its limit).

    python3 tools/check_2x_sheets.py [repo]     # exit 0 ok, 1 a tile over the limit, 77 skipped
"""
import sys
from pathlib import Path

LIMIT = 30.0
try:
    import numpy as np
    from PIL import Image
except ImportError:
    print("check_2x_sheets: numpy or Pillow missing, skipped")
    sys.exit(77)

repo = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent
one, two = repo / "Content/icons/object-m.png", repo / "Content/icons2x/object-m.png"
if not two.exists():
    print("check_2x_sheets: no 2x sheet, skipped")
    sys.exit(77)
a = np.asarray(Image.open(one).convert("RGBA")).astype(float)
b = np.asarray(Image.open(two).convert("RGBA")).astype(float)
if b.shape[0] != 2 * a.shape[0] or b.shape[1] != 2 * a.shape[1]:
    print(f"check_2x_sheets: {two.name} is not exactly twice {one.name}")
    sys.exit(1)
columns = a.shape[1] // 64
bad, checked = [], 0
for icon in range(columns * (a.shape[0] // 64)):
    x0, y0 = 1 + (icon % columns) * 65, 1 + (icon // columns) * 65
    t1 = a[y0:y0 + 64, x0:x0 + 64]
    t2 = b[2 * y0:2 * y0 + 128, 2 * x0:2 * x0 + 128]
    if t1.shape[:2] != (64, 64) or t2.shape[:2] != (128, 128):
        continue
    if (t1[..., 3] < 128).sum() * 4 >= 64 * 64:
        continue  # a cutout or translucent tile, not a cube's face
    r = t2.reshape(64, 2, 64, 2, 4).mean(axis=(1, 3))
    both = (t1[..., 3] > 128) & (r[..., 3] > 128)
    d = np.abs(t1[..., :3] - r[..., :3]).mean(axis=2)
    lines = [d[:, j][both[:, j]].mean() for j in range(64) if both[:, j].sum() >= 8]
    lines += [d[i][both[i]].mean() for i in range(64) if both[i].sum() >= 8]
    checked += 1
    worst = max(lines, default=0.0)
    if worst > LIMIT:
        bad.append((round(worst, 1), icon))
print(f"check_2x_sheets: {checked} cube tiles, {len(bad)} over {LIMIT}: {sorted(bad, reverse=True)[:10]}")
sys.exit(1 if bad else 0)
