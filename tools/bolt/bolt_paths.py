#!/usr/bin/env python3
"""The lightning bolt's 3D paths (owner 2026-11-04, Q-002: a thin glowing 3D bolt following each
sprite's path; the original has no scene for it, only the painted frames 305-307).

Reads object-m.png's icons 305-307, thins each bolt to its one-pixel centre line (Zhang-Suen),
walks it into polylines from its ends and forks, simplifies them (Ramer-Douglas-Peucker, 1 px) and
writes src/GalaxyEggbert/Game/BoltPaths.hpp: per frame, polylines of points (x, y) in the cell
(-0.5..0.5, y up) with the bolt's half-width there (from the distance to the sprite's edge).

    python3 tools/bolt/bolt_paths.py
"""
from __future__ import annotations

import math
from pathlib import Path

import numpy as np
from PIL import Image

REPO = Path(__file__).resolve().parents[2]
ICONS = (305, 306, 307)
TILE, GAP, COLS = 64, 1, 20


def tile_alpha(sheet: Image.Image, icon: int) -> np.ndarray:
    r, c = divmod(icon, COLS)
    x0, y0 = GAP + c * (TILE + GAP), GAP + r * (TILE + GAP)
    rgba = np.asarray(sheet.crop((x0, y0, x0 + TILE, y0 + TILE)).convert("RGBA"))
    return rgba[:, :, 3] >= 128


def thin(mask: np.ndarray) -> np.ndarray:
    img = mask.astype(np.uint8).copy()
    changed = True
    while changed:
        changed = False
        for step in (0, 1):
            pad = np.pad(img, 1)
            p2, p3, p4 = pad[:-2, 1:-1], pad[:-2, 2:], pad[1:-1, 2:]
            p5, p6, p7 = pad[2:, 2:], pad[2:, 1:-1], pad[2:, :-2]
            p8, p9 = pad[1:-1, :-2], pad[:-2, :-2]
            nb = [p2, p3, p4, p5, p6, p7, p8, p9]
            b = sum(n.astype(int) for n in nb)
            seq = nb + [p2]
            a = sum(((seq[i] == 0) & (seq[i + 1] == 1)).astype(int) for i in range(8))
            if step == 0:
                c1, c2 = p2 * p4 * p6, p4 * p6 * p8
            else:
                c1, c2 = p2 * p4 * p8, p2 * p6 * p8
            rm = (img == 1) & (b >= 2) & (b <= 6) & (a == 1) & (c1 == 0) & (c2 == 0)
            if rm.any():
                img[rm] = 0
                changed = True
    return img.astype(bool)


def neighbours(skel, y, x):
    out = []
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            if (dy or dx) and 0 <= y + dy < skel.shape[0] and 0 <= x + dx < skel.shape[1] and skel[y + dy, x + dx]:
                out.append((y + dy, x + dx))
    return out


def trace(skel: np.ndarray) -> list[list[tuple[int, int]]]:
    pts = list(zip(*np.nonzero(skel)))
    degree = {p: len(neighbours(skel, *p)) for p in pts}
    nodes = {p for p in pts if degree[p] != 2}
    seen = set()
    paths = []
    for start in sorted(nodes):
        for nxt in neighbours(skel, *start):
            edge = frozenset((start, nxt))
            if edge in seen:
                continue
            path = [start, nxt]
            seen.add(edge)
            prev, cur = start, nxt
            while cur not in nodes:
                options = [n for n in neighbours(skel, *cur) if n != prev and frozenset((cur, n)) not in seen]
                if not options:
                    break
                n = options[0]
                seen.add(frozenset((cur, n)))
                prev, cur = cur, n
                path.append(cur)
            paths.append(path)
    return [p for p in paths if len(p) >= 2]


def rdp(points, eps=1.0):
    if len(points) < 3:
        return points
    (y0, x0), (y1, x1) = points[0], points[-1]
    dx, dy = x1 - x0, y1 - y0
    norm = math.hypot(dx, dy) or 1.0
    dists = [abs(dy * (x - x0) - dx * (y - y0)) / norm for y, x in points]
    i = max(range(len(dists)), key=dists.__getitem__)
    if dists[i] <= eps:
        return [points[0], points[-1]]
    return rdp(points[: i + 1], eps)[:-1] + rdp(points[i:], eps)


def half_width(mask: np.ndarray, y: int, x: int) -> float:
    for r in range(1, 12):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                yy, xx = y + dy, x + dx
                if not (0 <= yy < TILE and 0 <= xx < TILE) or not mask[yy, xx]:
                    if math.hypot(dy, dx) <= r:
                        return max(1.0, math.hypot(dy, dx) - 0.5)
    return 6.0


def main() -> int:
    sheet = Image.open(REPO / "Content" / "icons" / "object-m.png")
    frames = []
    cores = []
    for icon in ICONS:
        mask = tile_alpha(sheet, icon)
        r, c = divmod(icon, COLS)
        x0, y0 = GAP + c * (TILE + GAP), GAP + r * (TILE + GAP)
        rgba = np.asarray(sheet.crop((x0, y0, x0 + TILE, y0 + TILE)).convert("RGBA")).astype(int)
        # The bolt's core colour: its whitest opaque texel, 3 x 3 uniform round it.
        best, where = -1, (32, 32)
        for yy in range(1, TILE - 1):
            for xx in range(1, TILE - 1):
                block = rgba[yy - 1:yy + 2, xx - 1:xx + 2]
                if (block[:, :, 3] < 255).any():
                    continue
                score = int(block[:, :, :3].min())
                if score > best:
                    best, where = score, (xx, yy)
        cores.append(((where[0] + 0.5) / TILE, (where[1] + 0.5) / TILE))
        polylines = []
        for path in trace(thin(mask)):
            simple = rdp(path)
            line = [((x + 0.5) / TILE - 0.5, 0.5 - (y + 0.5) / TILE, half_width(mask, y, x) / TILE)
                    for y, x in simple]
            # Ends within a few texels of the cell's top or bottom reach it, so bolts in cells one
            # over the other join into one column.
            line = [(px, 0.5 if py > 0.44 else -0.5 if py < -0.44 else py, pw) for px, py, pw in line]
            polylines.append(line)
        frames.append(polylines)
    lines = [
        "#pragma once",
        "",
        "// Generated by tools/bolt/bolt_paths.py from object-m.png's lightning frames 305-307: each frame's",
        "// centre lines (x, y in the cell, -0.5..0.5, y up) with the bolt's half-width there (Q-002).",
        "",
        "#include <array>",
        "",
        "namespace GalaxyEggbert::Game",
        "{",
        "    struct BoltPoint",
        "    {",
        "        float x, y, halfWidth;",
        "    };",
        "    struct BoltPolyline",
        "    {",
        "        int first, count; // into kBoltPoints",
        "    };",
    ]
    points, polys, frame_ranges = [], [], []
    for polylines in frames:
        start = len(polys)
        for pl in polylines:
            polys.append((len(points), len(pl)))
            points.extend(pl)
        frame_ranges.append((start, len(polys) - start))
    lines.append("    inline constexpr std::array<BoltPoint, %d> kBoltPoints{{" % len(points))
    for x, y, w in points:
        lines.append("        {%.4ff, %.4ff, %.4ff}," % (x, y, w))
    lines.append("    }};")
    lines.append("    inline constexpr std::array<BoltPolyline, %d> kBoltPolylines{{" % len(polys))
    for f, n in polys:
        lines.append("        {%d, %d}," % (f, n))
    lines.append("    }};")
    lines.append("    // Per frame: a texel of the bolt's white core (u, v in the tile, v down).")
    lines.append("    inline constexpr std::array<std::array<float, 2>, 3> kBoltCoreTexel{{")
    for u, v in cores:
        lines.append("        {%.4ff, %.4ff}," % (u, v))
    lines.append("    }};")
    lines.append("    // Per frame (305, 306, 307): its first polyline and how many.")
    lines.append("    inline constexpr std::array<BoltPolyline, 3> kBoltFrames{{")
    for f, n in frame_ranges:
        lines.append("        {%d, %d}," % (f, n))
    lines.append("    }};")
    lines.append("}")
    out = REPO / "src" / "GalaxyEggbert" / "Game" / "BoltPaths.hpp"
    out.write_text("\n".join(lines) + "\n")
    print("wrote", out, "frames", [n for _, n in frame_ranges], "points", len(points))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
