"""Minimal reader for the 3DFACE-only DXF files Ray Dream Designer exports."""

from __future__ import annotations

from pathlib import Path

import numpy as np


def read_3dfaces(path: str | Path) -> dict[str, np.ndarray]:
    """Return {layer: (N, 3, 3) triangles}. Quads (4th corner != 3rd) are split in two."""
    lines = [l.strip() for l in Path(path).read_bytes().decode("latin-1").splitlines()]
    pairs = list(zip(lines[0::2], lines[1::2]))
    layers: dict[str, list] = {}
    i = 0
    while i < len(pairs):
        code, value = pairs[i]
        if code == "0" and value == "3DFACE":
            layer = ""
            pts = np.zeros((4, 3))
            i += 1
            while i < len(pairs) and pairs[i][0] != "0":
                c, v = int(pairs[i][0]), pairs[i][1]
                if c == 8:
                    layer = v
                elif 10 <= c <= 13 or 20 <= c <= 23 or 30 <= c <= 33:
                    pts[c % 10, c // 10 - 1] = float(v)
                i += 1
            tris = layers.setdefault(layer, [])
            tris.append(pts[[0, 1, 2]])
            if not np.allclose(pts[3], pts[2]):
                tris.append(pts[[0, 2, 3]])
            continue
        i += 1
    return {k: np.array(v) for k, v in layers.items()}
