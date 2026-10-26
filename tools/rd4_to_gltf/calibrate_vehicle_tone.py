#!/usr/bin/env python3
"""The vehicles' tone against Blupi's sprites, as calibrate_tone.py fits it for the objects.

Every vehicle clip frame (build_clip_library.py's VEHICLES) is rendered as that script's check
renders it, Blupi and the vehicle apart, with the base scene's camera; the pixels where only
the vehicle shows are compared with the frame's icon in blupi.png, and one map
``sprite = a * model + b`` is fitted per vehicle over their colour distributions.

    python3 tools/rd4_to_gltf/calibrate_vehicle_tone.py --library build-probe/rd4-blupi/library \\
        --scenes ../speedyblupi-data/originals/Blupi --controller src/GalaxyEggbert/Game/BlupiController.cpp \\
        --sheet ../mobile-eggbert/Content/icons/blupi.png

The suggested ``a,b`` goes to ``export_cna_avatar.py --tone`` when the vehicle is exported.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))

import build_clip_library as bcl  # noqa: E402
import calibrate_tone as ct  # noqa: E402
import rd4  # noqa: E402
import scene as sc  # noqa: E402
import verify_sprites as vs  # noqa: E402


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--library", required=True, type=Path, help="directory of blupi.glb and vehicles/")
    ap.add_argument("--scenes", required=True, type=Path)
    ap.add_argument("--base", default="marche-profil.rd4")
    ap.add_argument("--controller", required=True, type=Path)
    ap.add_argument("--sheet", required=True, type=Path)
    ap.add_argument("--frame-rate", type=float, default=20.0)
    args = ap.parse_args(argv)

    sheet = np.array(Image.open(args.sheet).convert("RGBA")).astype(float)
    clips = bcl.controller_clips(args.controller)
    base_scene = sc.read_scene(rd4.parse_file(args.scenes / args.base))
    blupi = vs.Model(args.library / "blupi.glb")
    extras = blupi.doc["asset"]["extras"]["rd4_to_gltf"]
    cam = vs.Camera(base_scene, "Jeu", extras)
    px = cam.zoom * vs.ICON / cam.frame_height / extras["scale"]
    pos, right, up, back = cam.frame(base_scene.begin)
    light = cam.light(base_scene.begin)

    def draw(meshes):
        color, cover = vs.render(meshes, pos, right, up, back, px * vs.SUPERSAMPLE, vs.ICON * vs.SUPERSAMPLE, light,
                                 vs.SUPERSAMPLE)
        return vs.downsample(color, cover, vs.SUPERSAMPLE)

    for vehicle, names in bcl.VEHICLES.items():
        glb = args.library / "vehicles" / f"{vehicle}.glb"
        if not glb.exists():
            continue
        vmodel = vs.Model(glb)
        sprite_px, model_px = [], []
        for name in names:
            if name not in clips:
                continue
            try:
                blupi.select_clip(name)
                vmodel.select_clip(name)
            except (SystemExit, KeyError, ValueError):     # a clip the library predates
                continue
            for k, icon in enumerate(clips[name]):
                if icon < 0:
                    continue
                t = k / args.frame_rate
                _bc, ba = draw(blupi.skinned(t))
                vc, va = draw(vmodel.skinned(t))
                r, c = divmod(icon, 10)
                spr = sheet[r * 60:(r + 1) * 60, c * 60:(c + 1) * 60]
                only = (va > 0.9) & (ba < 0.1) & (spr[..., 3] > 127)
                if only.sum() < 10:
                    continue
                sprite_px.append(spr[..., :3][only])
                model_px.append(np.clip(vc[only], 0, 1) * 255)    # the renderer's colours are 0..1
        if not sprite_px:
            print(f"{vehicle:8s} no vehicle-only pixels")
            continue
        s, m = np.concatenate(sprite_px), np.concatenate(model_px)
        a, b = ct.fit(s, m)
        before, after = ct.error(s, m), ct.error(s, np.clip(a * m + b, 0, 255))
        print(f"{vehicle:8s} {len(s):6d} px  error {before:5.1f} -> {after:5.1f}  tone={a:.2f},{b:.0f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
