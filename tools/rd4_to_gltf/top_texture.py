#!/usr/bin/env python3
"""Puts an original texture over an exported model seen from above (Q-003: the owner asked for a
clay texture on the clay bridge, 2026-11-02; Decor/terre.tif is the originals' clay, which no
scene uses).

The exported part keeps its geometry; its UVs become a projection from above (u = x + 0.5,
v = z + 0.5 in model units, one texture tile a cell) and its texture the given image, recoloured
to the mean colour of the part's own flat texture so that the model keeps the tone it was
calibrated to (`objects.txt` tone=) and gains only the texture's grain.

Run after `export_cna_avatar.py` (which writes the flat texture and zero UVs):

    python3 tools/rd4_to_gltf/top_texture.py --model avatars3d/objects/bridge364_grotte-fragile1 \\
        --texture ../speedyblupi-data/originals/Decor/terre.tif
"""
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image

STRIDE = 52      # VertexPositionNormalTextureSkinned: position 0, normal 12, UV 24, weights 32, joints 48
UV_OFFSET = 24


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--model", required=True, type=Path, help="the exported model directory")
    ap.add_argument("--texture", required=True, type=Path, help="the image to lay over it")
    ap.add_argument("--name", default="top.png", help="the texture's file name in the model directory")
    args = ap.parse_args(argv)
    manifest_path = args.model / "avatar.skinnedmodel.json"
    manifest = json.loads(manifest_path.read_text())
    source = Image.open(args.texture).convert("RGB")
    grain = np.asarray(source, dtype=np.float64) / 255.0
    for index, part in enumerate(manifest["parts"]):
        name = args.name if len(manifest["parts"]) == 1 else f"{Path(args.name).stem}_{index}.png"
        flat = np.asarray(Image.open(args.model / part["texture"]).convert("RGB"), dtype=np.float64) / 255.0
        target = flat.reshape(-1, 3).mean(axis=0)
        gain = target / np.maximum(grain.reshape(-1, 3).mean(axis=0), 1e-3)
        image = np.clip(grain * gain, 0.0, 1.0)
        Image.fromarray((image * 255).round().astype(np.uint8), "RGB").save(args.model / name)
        verts_path = args.model / part["vertices"]
        raw = np.fromfile(verts_path, dtype=np.uint8).reshape(-1, STRIDE).copy()
        pos = raw[:, 0:12].copy().view(np.float32).reshape(-1, 3)
        uv = np.stack([pos[:, 0] + 0.5, pos[:, 2] + 0.5], axis=1).astype(np.float32)
        raw[:, UV_OFFSET:UV_OFFSET + 8] = uv.view(np.uint8).reshape(-1, 8)
        raw.tofile(verts_path)
        old_texture = args.model / part["texture"]
        part["texture"] = name
        if old_texture.name != name and old_texture.exists():
            old_texture.unlink()
        print(f"{args.model.name}/{part['name']}: {len(raw)} vertices, texture {args.texture.name} "
              f"recoloured to mean {np.round(target * 255).astype(int).tolist()}")
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
