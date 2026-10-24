# rd4_to_gltf — original Speedy Blupi 3D scenes to skinned glTF

Converts the original Ray Dream scenes that Speedy Blupi's sprites were rendered from into a
rigged, animated glTF binary (`.glb`), and checks the result against the game's sprites by
rendering it with the original game camera. This is the route to the real 3D Blupi (`plan.md`
`069`).

Status (2026-10-23): the walk (`marche-profil.rd4`) is complete as a model and in the game
(`avatars3d/blupi/`, see its README for the exact commands): geometry generated from the scene's
own masters, the face texture, the walk cycle and the standing pose. With the original game camera
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

`--clip NAME=SCENE[@FRAME]` (repeatable) puts several scenes' animations on the main scene's
skeleton (`@FRAME`: one rendered frame as a static pose), and `--frame-rate 20` times them at the
game's sprite cadence. `export_cna_avatar.py` turns the `.glb` into the game's CNA
`SkinnedModelEXT` directory with each part's real colour or texture, its triangles turned
clockwise for XNA's default culling (`upstream_bugs.md` UB-003).

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
- A part whose geometry frame (`Scal`, `affi` deformer, hot point) changes in some clip, or which
  some clip's scene does not have, is skinned to a *shape joint* instead: a leaf child of its
  joint named `<joint>.shape`, whose translation, rotation and (possibly uneven) scale carry that
  frame. Ray Dream's deformer is a rotation times an axis scale, so the frame is exactly a glTF
  TRS. A part missing from a clip's scene (the legs of the squashed `blupi-plat.rd4`) is shrunk to
  nothing there. Without such clips there are no shape joints.
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
| A mirrored object (`det(mat) < 0`) with `ROTA` keys keeps its mirror in the parent's frame: `mat == L @ quat(q_k).T` at one key k (usually the first) with `L` a reflection, and the object is `L @ quat(q(t)).T` at every t | keys alone drop the mirror; with it `blupi-sucette.rd4`'s lollipop turns its face to the camera as in icons 234-236 (frame IoU 0.929 -> 0.963), `blupi-bouteille` 0.965 -> 0.981, `oups-profil` 0.960 -> 0.973; the mirror on the object's own side instead matches worse |
| Keys without a tweener (`void`) are linear; 1800 ticks per second | every stored static value equals linear interpolation at tick 3000 |
| Hot point `hp` (local, after the `affi` deformer) lands on `po`; the object rotates about it | DXF match below |
| `Scal` scales only the object's own geometry; a child's `po` is in the parent's unscaled frame | arm segments: elbow at `hp - po` = 3.725 vs. segment length 3.741 |
| `link` joints rotate inside the `mat` base frame: `axis` (`XAXI`/`YAXI`/`ZAXI`, angle `AAVa`), `gene` (`AXVa..AZVa`, `ORDE 6`), `ball` (`matR`), `lock` | DXF match below |
| Animated `ball` joints: `ACom/matR` holds `ROTA` keys with the same inverse convention, `matR == quat(ROTA).T` | stored `matR` equals the keys at the stored time to 0.0001 in `saute-profil`, `nage`, `glu` (0.008 in `bye`) |
| Tweener `twee { line { EASE a  EASO b } }` on a key: speed rises over [0, a], is constant to b, falls over [b, 1] of the segment; `void` = linear | the values seen: (0, 1) linear, (0.61, 1) speed-up, (0, 0.4) slow-down, (0.41, 0.61) both; frames of the scenes using it match their sprites (`blupi-plat`: MarchEcrase 0.876) |
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

## Matching scene frames with sprite icons

`match_icons.py` renders every frame of every scene containing Blupi straight from the scene
(`scene_render.py`: its own kinematics, generated geometry, face, game camera and lighting; no
glTF in between), compares each with every icon of the sprite sheet, as rendered and mirrored,
and writes `icons.csv`, `frames.csv`, a per-action `report.md` (actions and icon sequences from
mobile-eggbert's `table_blupi`, read in place) and `icons/NNN.png` (icon beside its best render):

```bash
python3 tools/rd4_to_gltf/match_icons.py --scenes ../speedyblupi-data/originals/Blupi \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --reference ../mobile-eggbert \
    --out build-probe/rd4-blupi/match
```

First run (2026-10-24, about 3.5 minutes): 243 frames from 54 scenes; of the 231 icons used by
actions, 87 match confidently (score >= 0.85), 60 probably (>= 0.70) and 84 not. The unmatched
ones are the vehicles (unsupported cube, torus and multi-section masters), the turn icons
(front views mid-turn, which a 3D model replaces with a real rotation), Hide (drawn at another
scale), and frames with 2D effects added (Electro's glow).

## Clip library (the game's model)

`build_clip_library.py` turns the matches into the one model the game loads: one clip per
`BlupiController::AnimState` (plus `DeathLocked.<cause>` and `PickupBusy.<kind>`), each the
state's own icon table from `BlupiController.cpp` (read in place) played at `kAnimFps`:

```bash
B=../speedyblupi-data/originals/Blupi
python3 tools/rd4_to_gltf/build_clip_library.py --scenes $B --base marche-profil.rd4 \
    --icons build-probe/rd4-blupi/match/icons.csv \
    --controller src/GalaxyEggbert/Game/BlupiController.cpp \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --out build-probe/rd4-blupi/library/blupi.glb
python3 tools/rd4_to_gltf/export_cna_avatar.py --glb build-probe/rd4-blupi/library/blupi.glb \
    --cna-home ../../libcna/cna --out avatars3d/blupi
```

- Icon k of a table becomes a key at k / 20 s holding the pose of the scene frame it matched as
  rendered (not mirrored; score >= 0.70), LINEAR in between, with a last key closing the loop. An
  icon without a usable match holds the previous pose.
- Every scene is shifted, frame by frame, so its game camera sits where `marche-profil.rd4`'s
  does (depth excluded), which keeps each sprite's placement (`genou-profil.rd4`'s camera follows
  Blupi down).
- Joints are matched by name. `blupi-ballon.rd4` has no `BLUPI` group (its body is the root);
  `blupi-plat.rd4` names both feet `Pied-d`, so a joint without a namesake takes the free base
  joint of the same kind nearest to it in the body's frame. Parts a scene lacks (the squashed
  Blupi's legs) are hidden; scale and deformer changes (squashed, inflated) ride on shape joints.
- Props hung under Blupi in a used scene (the tongue of `pied-de-nez.rd4`, the bottle, the
  lollipop) join the skeleton and are hidden everywhere else. A frame shows its scene's props only
  if its icon matches at least as well with them (icon 2, a front view taken from
  `blupi-sucette.rd4`, has no lollipop).
- `clips.txt` beside the .glb: each frame's expression is the body texture of its scene, so it
  lists `rate`, the `part` that wears the face, the face PNGs and, per clip, the face of every
  frame; and `viewer <clip>` for the clips with a frame turned more than 60 degrees toward
  Blupi's right, the side the original camera saw (the squashed and inflated Blupi, the door
  turn, the dynamite and lollipop scenes, ...). `export_cna_avatar.py` copies it with the faces;
  the game swaps the face part's texture and puts the camera on his right for those clips.
- Without `--skip-check` every clip frame is rendered from the .glb with the base camera and
  compared with its icon (`clips.md`).

Result (2026-10-24, 34 clips, 45 joints including 15 shape joints, 5 faces), mean silhouette IoU
against the icons: March 0.981, Stop 0.970, Jump 0.979, Air 0.961, Down 0.946, Up 0.980, Push
0.988, StopEcrase 0.965, MarchEcrase 0.950, Balloon 0.976, StopNage/MarchNage 0.980,
StopSurf/MarchSurf 0.981, Drown 0.982, Mockery 0.971, Mockeryp 0.942, PutDynamite 0.966,
TakeDynamite 0.912, Sucette 0.957, Drink 0.940; lower where the icons are front or back views
(Teleporting, Switch, Charge, Mockeryi), carry a skateboard (Skate clips, 0.73-0.89) or 2D
effects (deaths Clear1/Clear3/Glu). Not built, no matching scene frame: the vehicles (Helico, Jeep,
Tank, Over, FireTank), Hide, Clear2 and Clear4; the game shows `Stop` for them.

## Not converted yet (next steps)

- Vehicles and the skateboard: cube and cone primitives, the torus, several cross-sections per
  sweep, curved and twisted sweep paths; texture maps on primitives (the lollipop's spiral).
- Front and back views (Turn, Switch, Hide): match frames turned about the vertical axis, so a
  clip can take a pose seen from another side.
- Highlight, shininess and the other shader channels (the glTF material is plain PBR).
