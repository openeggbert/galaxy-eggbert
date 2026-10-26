#!/usr/bin/env python3
"""Each object's tone against its sprites, from build_object_library.py's compare sheets.

Ray Dream rendered each scene's sprites with that scene's own lights; the library (and the game)
lights every object alike (the Blupi scenes' ambient, light and highlight), so some objects come
out more saturated, lighter or darker than their sprites. For every object this fits one map
``sprite = a * model + b`` (the same for R, G and B: a gain, and a lift that greys strong
colours) between the colour distributions of the sprite and the model inside both silhouettes
(their quantiles, which a pixel's misplacement does not disturb), and prints it with the mean
colour error before and after.

    python3 tools/rd4_to_gltf/calibrate_tone.py build-probe/rd4-objects \
        --objects tools/rd4_to_gltf/objects.txt [--min-gain 3]

With ``--objects`` the scenes of one object (its library directories ``<object>_<scene>``) are
pooled into one fit, as ``tone=`` belongs to the object. An object whose error drops by at least
``--min-gain`` gets a ``tone=a,b`` suggestion for objects.txt (build_object_library.py applies it
to the object's colours and textures).
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
from PIL import Image

QUANTILES = np.linspace(0.02, 0.98, 49)
GAIN_RANGE = (0.5, 1.3)
LIFT_RANGE = (-30.0, 70.0)


def sheet_pixels(path: Path):
    """(sprite pixels, model pixels) inside both silhouettes of a compare sheet, or None."""
    im = np.asarray(Image.open(path).convert("RGB")).astype(float)
    h = im.shape[0] // 3
    sprite, model = im[:h], im[h:2 * h]
    inside = ((np.abs(sprite - sprite[1, 1]).sum(-1) > 40) & (np.abs(model - model[1, 1]).sum(-1) > 40))
    if inside.sum() < 50:
        return None
    return sprite[inside], model[inside]


def fit(sprite: np.ndarray, model: np.ndarray) -> tuple[float, float]:
    """Least-squares (a, b) of sprite = a * model + b over the channels' quantiles, clamped."""
    s = np.concatenate([np.quantile(sprite[:, c], QUANTILES) for c in range(3)])
    m = np.concatenate([np.quantile(model[:, c], QUANTILES) for c in range(3)])
    (a, b), *_ = np.linalg.lstsq(np.stack([m, np.ones_like(m)], 1), s, rcond=None)
    return float(np.clip(a, *GAIN_RANGE)), float(np.clip(b, *LIFT_RANGE))


def error(sprite: np.ndarray, model: np.ndarray) -> float:
    """Mean colour error between the two distributions (their quantiles, per channel)."""
    return float(np.mean([np.abs(np.quantile(sprite[:, c], QUANTILES) - np.quantile(model[:, c], QUANTILES)).mean()
                          for c in range(3)]))


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("library", type=Path)
    ap.add_argument("--min-gain", type=float, default=3.0)
    ap.add_argument("--objects", type=Path, help="objects.txt: pool each object's scenes")
    args = ap.parse_args(argv)
    dirs = sorted(p for p in args.library.iterdir() if (p / "compare.png").exists())
    if args.objects is not None:
        names = [line.split("#", 1)[0].split()[0] for line in args.objects.read_text().splitlines()
                 if line.split("#", 1)[0].strip()]
        groups = [(n, [d for d in dirs if d.name.startswith(n + "_")]) for n in names]
    else:
        groups = [(d.name, [d]) for d in dirs]
    for name, members in groups:
        pixels = [px for px in (sheet_pixels(d / "compare.png") for d in members) if px is not None]
        if not pixels:
            continue
        sprite = np.concatenate([px[0] for px in pixels])
        model = np.concatenate([px[1] for px in pixels])
        a, b = fit(sprite, model)
        before = error(sprite, model)
        after = error(sprite, np.clip(a * model + b, 0, 255))
        mark = f"tone={a:.2f},{b:.0f}" if before - after >= args.min_gain else "-"
        print(f"{name:34s} error {before:5.1f} -> {after:5.1f}  {mark}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
