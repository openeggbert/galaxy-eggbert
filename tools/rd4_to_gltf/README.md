# rd4_to_gltf — original Speedy Blupi 3D scenes to skinned glTF

Converts the original Ray Dream scenes that Speedy Blupi's sprites were rendered from into a
rigged, animated glTF binary (`.glb`), and checks the result against the game's sprites by
rendering it with the original game camera. This is the route to the real 3D Blupi (`plan.md`
`069`).

Status (2026-10-23): the walk (`marche-profil.rd4`) is complete as a model: geometry generated
from the scene's own masters, the face texture, and the walk cycle. With the original game camera
its silhouettes match sprite icons 5-10 at IoU 0.978-0.985; front views with faces match too
(`blupi-ouf4.rd4` 0.965, `blupi-sucette.rd4` 0.929, its lollipop is not converted). Generated
files are not committed; they go to `build-probe/rd4-blupi/`.

## Inputs

The originals are in the `speedyblupi-data` repository (`originals/`, GPLv3, © Daniel Roux &
EPSITEC SA), next to this repository:

- `originals/Blupi/*.rd4|*.rds` — 97 Ray Dream Designer 3 / Studio 4 scenes in the **ASCII**
  `3DC` format, one per action. 56 of them contain the full rigged Blupi.
- `originals/Blupi/blupi.dxf` — Ray Dream's own triangulation of Blupi, exported from
  `stop-profil.rd4`. Only used to check the generated geometry (`--reference-dxf`).
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

With `--reference-*`, the converter first prints how far Ray Dream's DXF vertices lie from the
generated surfaces (canonical units; Blupi's parts are 3.4-10.4 across):

| Master | max | mean |
|---|---|---|
| body (`Corps`) | 0.013 | 0.002 |
| leg (`Jambe`) | 0.006 | 0.001 |
| sphere (nose, palm, thumb) | 0.018 | 0.004 |
| cylinder (arms) | 0.017 | 0.004 |
| foot (`Pied`) | 0.036 | 0.009 |
| eyelash (`Cil`) | 0.111 | 0.036 |

The eyelash difference (under 0.1 scene units after its deformer, a third of a sprite pixel) is
not explained yet.

The verifier reads the `.glb` back, skins and textures it on the CPU, renders every scene frame
(`FSec` per second) with the scene's `Caméra Jeu` and Ray Dream's lighting (ambient 0.5 plus the
scene's light at 0.5, highlight 0.5), and prints the silhouette IoU against each icon, an IoU
matrix (frames x icons), and a montage (sprite / render / overlap). It needs only numpy and
Pillow. The `.glb` also imports into Blender 4.3 (one armature, one action) and converts with
CNA's `tools/avatar_asset_pipeline/convert_avatar.py --embedded-clips`.

## Output

- One joint per scene object under `--root` (default `BLUPI`): 21 joints for Blupi. Groups are
  joints without geometry; duplicate names get `.001`, `.002`, ... (`Paume.001` is the left palm).
- Every part is skinned rigidly (weight 1.0) to its own joint, as in the original, where Blupi is
  built from separate rigid parts.
- One mesh per material, named after it (`BLUPI_rd_ffff00`, `BLUPI_rd_tex_Corps`), all sharing one
  skin: CNA's converter keeps only the mesh name of each part, not its colour (`upstream_bugs.md`
  UB-001, UB-002).
- The face is the body's texture map, embedded as PNG with "white is transparent" already turned
  into the body colour. Each scene embeds its own face (expression).
- One animation per scene, sampled at 30 fps with LINEAR interpolation.
- glTF space: Y up, Blupi facing +Z, feet on y = 0 in the bind pose, 0.05 m per Ray Dream unit
  (`--scale`), which makes Blupi about 0.72 m tall. `asset.extras.rd4_to_gltf` records the scale,
  offset and axis conversion so tools can map back to the scene.

## Verified Ray Dream conventions

Each was checked numerically, not assumed.

### Scene graph and animation

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
| Orthographic camera: frame is `size` / `zoom`; the image fits its height; the axis hits the centre of the middle pixel | best IoU at 0.8775–0.885 x zoom; 60 / 68.03 = 0.882 |

**DXF match:** forward kinematics of `stop-profil.rd4`'s stored pose reproduces the sphere and
cylinder centres in `blupi.dxf` with an RMS error of 0.0011 units (zero offset); `ferme.rd4`, the
same standing pose, matches at 0.0022. Every other scene, and X-before-Y orders, are off by 0.15
or more.

### Geometry

| Convention | Evidence |
|---|---|
| DXF units = 4 x scene units | sphere/cylinder sizes and hand spacing |
| Sphere radius 3; cylinder radius 3, local Z in [−2.5, 2.5] | recovered DXF geometry |
| Free form: sweep path through the `sap` points (`cv`, handles `cnc`/`cpc`), one cross-section (`st` + cubic `s3` segments) | table above |
| Envelopes `t`/`b`/`l`/`r` scale the section's `b` axis (positive/negative side) and `a` axis (negative/positive); each is a cubic Bezier in (path position, scale) through `(p0, v0), (p0 + nc[1], nc[0]), (p1 − pc[1], pc[0]), (p1, v1)` | DXF ring positions fall exactly on 16 equal Bezier-parameter steps; ring radii within 0.002 |
| Section `a` → +X, `b` → +Z for a path along +Y | eyelash (asymmetric section) and foot (unequal envelopes) |

`freeform_mesh.py` documents the rest (sampling, caps by ear clipping).

### Face texture

| Convention | Evidence |
|---|---|
| Bitmaps: `TILE` lines of PackBits streams, edge tiles cropped; 8-bit with a `CTAB` palette, 1-bit masks; 32-bit rows are R, G, B planes | the decoded face; the 32-bit layout is only a best reading of the lollipop texture |
| The body's primary domain is a texture map (`smap`); `whit 1` makes pure white (255) transparent, showing the master's colour; near-white (254) eye whites stay | renders against sprites |
| Texture `s` = surface `u` (control-polygon length from `st`) turned by `rotc` quarter turns; `t` = the vertex's fraction of the meridian length, flat end caps included | best of 64 orientation candidates against front and profile sprites, then the meridian rule fixed the face's height |
| The primary domain's `bbox` is not the texture extent: some scenes store `0,0,2,2` | `blupi-ouf4.rd4` faces were off-centre until the period came from the section itself |

Only `rotc 3`, `flpv 1`, `flph 0`, one tile, is verified (every Blupi scene uses it); other
placements produce a warning. `srct`/`sovl` domains are baked over the texture; masked `srgn`
domains are not converted.

## Not converted yet (next steps)

- Other scenes and the icon <-> (scene, frame) mapping for all 87 `BlupiAction` states.
- Animated `ball` joints and explicit tweeners (`twee`/`line`, `EASE`) stop the conversion with
  an error; an animated hot point, `Scal` or deformer (`plat`/squash scenes) is reported as a
  warning and ignored.
- Props and vehicles: cube and cone primitives, texture maps on primitives (the lollipop), several
  cross-sections per sweep, curved and twisted sweep paths.
- Highlight, shininess and the other shader channels (the glTF material is plain PBR).
