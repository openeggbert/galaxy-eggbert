#!/usr/bin/env python3
"""Turn a .glb from rd4_to_gltf.py into the game's CNA SkinnedModelEXT avatar directory.

Runs CNA's own converter (``tools/avatar_asset_pipeline/convert_avatar.py --embedded-clips``),
then fixes what it leaves for the game (upstream_bugs.md):

* UB-003: it keeps glTF's counter-clockwise front faces, which XNA's default
  ``CullCounterClockwise`` culls, so the model would draw inside out. Every part's triangles are
  turned clockwise, unless the converter has already done so.
* UB-002 (fixed in newer CNA): each part gets its real colour or texture from the .glb.

A ``faces.txt`` beside the .glb (build_clip_library.py) is copied along with the face PNGs it
lists.

    python3 export_cna_avatar.py --glb blupi.glb --cna-home ../../libcna/cna --out avatars3d/blupi
"""

from __future__ import annotations

import argparse
import io
import json
import shutil
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import gltf  # noqa: E402

FLAT_TEXTURE_SIZE = 4


def _linear_to_srgb(c: float) -> float:
    return c * 12.92 if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055


def part_images(glb: Path) -> dict[str, Image.Image]:
    """{mesh name: image} for every mesh of the .glb."""
    doc, blob = gltf.read_glb(glb)
    out = {}
    for mesh in doc["meshes"]:
        material = doc["materials"][mesh["primitives"][0]["material"]]
        pbr = material["pbrMetallicRoughness"]
        if "baseColorTexture" in pbr:
            image = doc["images"][doc["textures"][pbr["baseColorTexture"]["index"]]["source"]]
            view = doc["bufferViews"][image["bufferView"]]
            start = view.get("byteOffset", 0)
            out[mesh["name"]] = Image.open(io.BytesIO(blob[start:start + view["byteLength"]])).convert("RGBA")
        else:
            rgb = [round(_linear_to_srgb(c) * 255) for c in pbr.get("baseColorFactor", [1, 1, 1, 1])[:3]]
            out[mesh["name"]] = Image.new("RGBA", (FLAT_TEXTURE_SIZE, FLAT_TEXTURE_SIZE), (*rgb, 255))
    return out


def glb_indices(glb: Path) -> dict[str, np.ndarray]:
    """{mesh name: (T, 3) triangle indices} as stored in the .glb (counter-clockwise)."""
    doc, blob = gltf.read_glb(glb)
    return {m["name"]: gltf.read_accessor(doc, blob, m["primitives"][0]["indices"]).reshape(-1, 3)
            for m in doc["meshes"]}


def make_clockwise(idx_path: Path, ccw: np.ndarray) -> str:
    """Rewrite a CNA part's 16-bit index file with clockwise triangles; returns what was done."""
    tris = np.fromfile(idx_path, dtype="<u2").reshape(-1, 3)
    if np.array_equal(tris, ccw):
        tris[:, [1, 2]] = tris[:, [2, 1]]
        tris.astype("<u2").tofile(idx_path)
        return "turned clockwise"
    if np.array_equal(tris, ccw[:, [0, 2, 1]]):
        return "already clockwise"
    raise SystemExit(f"{idx_path}: triangles differ from the .glb's; cannot tell their winding")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--glb", required=True, type=Path)
    ap.add_argument("--cna-home", required=True, type=Path, help="CNA source checkout (for its converter)")
    ap.add_argument("--out", required=True, type=Path, help="avatar directory to write")
    args = ap.parse_args(argv)

    converter = args.cna_home / "tools" / "avatar_asset_pipeline" / "convert_avatar.py"
    subprocess.run([sys.executable, str(converter), "--body", str(args.glb), "--out", str(args.out),
                    "--embedded-clips"], check=True)
    manifest = json.loads((args.out / "avatar.skinnedmodel.json").read_text())
    images = part_images(args.glb)
    indices = glb_indices(args.glb)
    for part in manifest["parts"]:
        image = images.get(part["name"])
        if image is None:
            raise SystemExit(f"part {part['name']!r} has no mesh of that name in {args.glb}")
        image.save(args.out / part["texture"])
        winding = make_clockwise(args.out / part["indices"], indices[part["name"]])
        print(f"{part['name']}: texture {image.size[0]}x{image.size[1]}, triangles {winding}")
    faces = args.glb.parent / "faces.txt"
    if faces.exists():
        shutil.copyfile(faces, args.out / "faces.txt")
        for line in faces.read_text().splitlines():
            if line.startswith("face "):
                shutil.copyfile(args.glb.parent / line.split()[1], args.out / line.split()[1])
        print(f"faces.txt and its faces copied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
