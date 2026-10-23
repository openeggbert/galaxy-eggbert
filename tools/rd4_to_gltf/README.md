# rd4_to_gltf — original Speedy Blupi 3D scenes to skinned glTF

Converts the original Ray Dream scenes that Speedy Blupi's sprites were rendered from into a
rigged, animated glTF binary (`.glb`), and checks the result against the game's sprites by
rendering it with the original game camera. This is the route to the real 3D Blupi (`plan.md`
`069`).

Status (2026-10-23): proof of concept. The walk cycle (`marche-profil.rd4`) and the standing pose
(`stop-profil.rd4`) convert with silhouettes matching the sprites at IoU 0.975–0.987 (walk) and
0.976 (stand). Generated files are not committed; they go to `build-probe/rd4-blupi/`.

## Inputs

The originals are in the `speedyblupi-data` repository (`originals/`, GPLv3, © Daniel Roux &
EPSITEC SA), next to this repository:

- `originals/Blupi/*.rd4|*.rds` — 97 Ray Dream Designer 3 / Studio 4 scenes in the **ASCII**
  `3DC` format, one per action. 56 of them contain the full rigged Blupi.
- `originals/Blupi/blupi.dxf` — Ray Dream's own triangulation of Blupi, exported from
  `stop-profil.rd4`; the only source of free-form geometry used so far.
- Sprites: `../mobile-eggbert/Content/icons/blupi.png` (60x60 icons, 10 per row).

## Usage

```bash
B=../speedyblupi-data/originals/Blupi
python3 tools/rd4_to_gltf/rd4_to_gltf.py --scene $B/marche-profil.rd4 \
    --reference-scene $B/stop-profil.rd4 --reference-dxf $B/blupi.dxf \
    --out build-probe/rd4-blupi/blupi-marche.glb
python3 tools/rd4_to_gltf/verify_sprites.py --glb build-probe/rd4-blupi/blupi-marche.glb \
    --scene $B/marche-profil.rd4 --sheet ../mobile-eggbert/Content/icons/blupi.png \
    --icons 5,6,7,8,9,10 --out build-probe/rd4-blupi/blupi-marche-compare.png --min-iou 0.95
```

The verifier reads the `.glb` back, skins it on the CPU, renders every scene frame (`FSec` per
second) with the scene's `Caméra Jeu`, and prints the silhouette IoU against each icon, an
IoU matrix (frames x icons), and a montage (sprite / render / overlap). It needs only numpy and
Pillow. The `.glb` also imports into Blender 4.3 (one armature, one action) and converts with
CNA's `tools/avatar_asset_pipeline/convert_avatar.py --embedded-clips`.

## Output

- One joint per scene object under `--root` (default `BLUPI`): 21 joints for Blupi. Groups are
  joints without geometry; duplicate names get `.001`, `.002`, ... (`Paume.001` is the left palm).
- Every part is skinned rigidly (weight 1.0) to its own joint, as in the original, where Blupi is
  built from separate rigid parts.
- One mesh per material, named after it (`BLUPI_rd_ffff00`), all sharing one skin: CNA's converter
  keeps only the mesh name of each part, not its colour (`upstream_bugs.md` UB-001, UB-002).
- One animation per scene, sampled at 30 fps with LINEAR interpolation.
- glTF space: Y up, Blupi facing +Z, feet on y = 0 in the bind pose, 0.05 m per Ray Dream unit
  (`--scale`), which makes Blupi about 0.72 m tall. `asset.extras.rd4_to_gltf` records the scale,
  offset and axis conversion so tools can map back to the scene.

## Verified Ray Dream conventions

Each was checked numerically, not assumed:

| Convention | Evidence |
|---|---|
| Lines split on LF only; a bare CR inside `TILE` values separates the `$FFFFFFFF` header | byte dump of `marche-profil.rd4` |
| `mat` (row-major) maps local to parent as `M @ v`; Z up; cameras look down local −Z, +Y up | both cameras in `marche-profil.rd4` point at Blupi only under this reading |
| `ROTA w,x,y,z,n` keys hold the inverse rotation: `mat == quat(w,x,y,z).T` | slerped keys reproduce the stored `mat` to 0.0009 |
| Keys without a tweener (`void`) are linear; 1800 ticks per second | every stored static value equals linear interpolation at tick 3000 |
| Hot point `hp` (local, after the `affi` deformer) lands on `po`; the object rotates about it | DXF match below |
| `Scal` scales only the object's own geometry; a child's `po` is in the parent's unscaled frame | arm segments: elbow at `hp - po` = 3.725 vs. segment length 3.741 |
| `link` joints rotate inside the `mat` base frame: `axis` (`XAXI`/`YAXI`/`ZAXI`, angle `AAVa`), `gene` (`AXVa..AZVa`, `ORDE 6`), `ball` (`matR`), `lock` | DXF match below; `ball` is read but not yet exercised |
| `ORDE 6` composes Z·Y·X | only "Y before X" is verified (the stored poses have `AZVa` = 0); Z first is assumed from `ORDE 6` being the sixth permutation of XYZ |
| Primitives: sphere radius 3; cylinder radius 3, z in [−2.5, 2.5] | recovered DXF geometry |
| DXF units = 4 x scene units | sphere/cylinder sizes and hand spacing |
| Orthographic camera: frame is `size` / `zoom`; the image fits its height; the axis hits the centre of the middle pixel | best IoU at 0.8775–0.885 x zoom; 60 / 68.03 = 0.882 |

**DXF match:** forward kinematics of `stop-profil.rd4`'s stored pose reproduces the sphere and
cylinder centres in `blupi.dxf` with an RMS error of 0.0011 units (zero offset); `ferme.rd4`, the
same standing pose, matches at 0.0022. Every other scene, and X-before-Y orders, are off by 0.15
or more.

## Not converted yet (next steps)

- **Face.** In `marche-profil.rd4` the body's primary shading domain is a 640x480 texture map
  embedded in the scene (`sc/TABL/IMAP`; `rdimage.py` already decodes it: eyes, pupils, mouth,
  tongue). Still needed: the free-form surface UV parameterisation and the texture placement
  flags (`rotc`, `flpv`, `whit`), and the layering of the other shading domains (`srct`, `sovl`,
  `srgn` with 1-bit masks).
- **Free-form shapes that differ from the DXF.** Geometry comes from the one DXF, keyed by object
  name. `marche-profil.rd4`'s foot master differs slightly from `stop-profil.rd4`'s (a moved middle
  cross-section); the tool warns and uses the DXF shape. A sweep/envelope tessellator for `ex`
  masters (validated against the DXF) or per-scene DXF exports from Ray Dream Studio in the
  Windows XP VM would remove this limit. Both are needed for vehicles and props anyway.
- Other scenes and the icon <-> (scene, frame) mapping for all 87 `BlupiAction` states.
- Animated `ball` joints and explicit tweeners (`twee`/`line`, `EASE`) stop the conversion with
  an error; an animated hot point, `Scal` or deformer (`plat`/squash scenes) is reported as a
  warning and ignored.
- Highlight, shininess and the other shader channels; Ray Dream's lighting in the verifier.
- Game integration (CNA `SkinnedModelEXT`; see `upstream_bugs.md` UB-002 for lost colours).
