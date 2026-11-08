# Objects (3D models replacing sprite billboards)

The game's objects converted from the Ray Dream scenes their Speedy Blupi sprites were rendered
from, in CNA's `SkinnedModelEXT` format, one directory per object and scene
(`<object>_<scene>/`). Generated, do not edit by hand; regenerate with the commands below.
Plan and status: `plan.md` Phase 7B (`BB3D-*`).

## Source and license

- Scenes: `speedyblupi-data/originals/**` as listed in `tools/rd4_to_gltf/objects.txt`. GPLv3,
  © Daniel Roux & EPSITEC SA (the `speedyblupi-data` repository's README).
- Converted with `tools/rd4_to_gltf/build_object_library.py` and `export_objects.py` (CNA's
  `tools/avatar_asset_pipeline/convert_avatar.py`).

## Contents

- `index.txt`: the model directories, loaded in that order.
- Per model: the CNA avatar files (`avatar.skinnedmodel.json`, `skeleton.bin`, parts, the clip
  `clips/Frames.clip.bin`) and `icons.txt`: per sprite icon the model replaces, `icon <sheet>
  <icon> <seconds> <mirror> <yaw degrees> <iou>` (mirror 0 none, 1 left-right, 2 upside down).
  The clip holds the scene's frames at 20 per second; `iou` is how well that pose rendered with
  the scene's camera covers the icon.
- Model space is the sprite's: +X right, +Y up, +Z toward the sprite's camera, origin at the
  icon's centre, in blocks (64 sprite pixels, as Blupi's model). The game draws a model where it
  would have drawn the icon's billboard, turned by the yaw about +Y, then mirrored as listed
  (`GalaxyEggbertGame.cpp`, `ObjectModelTable`); icons not listed stay billboards.

Saw exception (owner,2026-11-08): only the upper half of `saw378_scie` is visible. After the
source poses are exported, `tools/rd4_to_gltf/saw_upper_half.py` clips each pose at the disk's
centre, closes the cut and bakes three exclusive poses into the existing animation format.
The cut is local Y=0; the game places it on the cell bottom. Diameter, texture and icon IDs/times
are unchanged. `upper-half.json` records the input hashes and bounds. The source GLB/IoU images
still describe the complete original disk before this requested display cut.

`export_objects.py` applies the cut automatically. A direct single-model export needs the
same post-step; `python3 tools/rd4_to_gltf/saw_upper_half.py --check avatars3d/objects/saw378_scie`
verifies all poses and their closed base without numpy.

## Regenerate

```bash
python3 tools/rd4_to_gltf/build_object_library.py --originals ../speedyblupi-data/originals \
    --sheets ../mobile-eggbert/Content/icons --objects tools/rd4_to_gltf/objects.txt \
    --out build-probe/rd4-objects
python3 tools/rd4_to_gltf/export_objects.py --library build-probe/rd4-objects \
    --cna-home ../../libcna/cna --out avatars3d/objects
```

`build-probe/rd4-objects/report.md` and each model's `compare.png` (sprite, the model rendered
back as the game draws it, overlap) show the match.
