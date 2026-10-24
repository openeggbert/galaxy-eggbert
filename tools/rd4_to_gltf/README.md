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
| A free-form's cross-section x, its y and the path's tangent form a right-handed frame (`freeform_mesh.tessellate`) | the bird's body (`oiseau.rd4`) is a side profile swept through its thickness: only so is its head under its eyes (side icons 90-105 IoU 0.854 against 0.742); Blupi's symmetric sections do not tell (`marche-profil` 0.9806 both ways, face colour 0.906 / 0.907) |
| A free-form whose primary domain keeps a colour while rectangle/oval domains carry texture maps (`shading.freeform_texture`): one atlas over (around, along) = (perimeter, path length), the domains' `bbox` as (around, along, around, along) with "around" running against the section loop, an image's columns along the path, its `cnth`/`cntv` repeats inside the rectangle | the bulldozer's blade (`tracks-profil.rd4`): its hazard stripes land on the curved front face in the front icons 60-63 only so (along the loop they sat on the flat face toward the cab; columns around gave smeared stripes) |
| The sprites of animated objects include the scene's end key, one frame past what Ray Dream renders (`build_object_library.setup_scene`) | the bird's raised wings (icons 96, 97, 104, 105) are its last key: IoU 0.83-0.95 there against 0.67-0.72 at the last rendered frame; the wasp (`guepe.rd4`) 0.839 -> 0.955, the eyed creature 0.970 -> 0.995 |
| Studio 5's `xyzS` (a leaf of the object beside `tran`) scales the object's own geometry along its local axes about its hot point (`scene.geometry_frame`) | the creature's eyes (`creature1y.rds`) are spheres scaled into tall eyeballs (0.79 x 0.35 x 1.11) and small pupils; with it icons 242-245 match 0.78-0.82, as plain spheres 0.68-0.71 (the pupils cover the eyes), normalized 0.61-0.67. 55 Studio 5 files have it, `blupi-scie.rds` among them |
| A group's `Scal` scales its contents (it has no geometry of its own) | `tank.rd4`'s `tracks` group is 1.15442 and the `BLUPI` group inside it 0.86617, its inverse; best frame IoU against the icons: tank 0.774 → 0.943, jeep 0.898 → 0.952, skateboard 0.920 → 0.988 (scaling only a group's direct geometric children: 0.684 / 0.932 / 0.900) |
| `link` joints rotate inside the `mat` base frame: `axis` (`XAXI`/`YAXI`/`ZAXI`, angle `AAVa`), `gene` (`AXVa..AZVa`, `ORDE 6`), `ball` (`matR`), `lock` | DXF match below |
| Animated `ball` joints: `ACom/matR` holds `ROTA` keys with the same inverse convention, `matR == quat(ROTA).T` | stored `matR` equals the keys at the stored time to 0.0001 in `saute-profil`, `nage`, `glu` (0.008 in `bye`) |
| Tweener `twee { line { EASE a  EASO b } }` on a key: speed rises over [0, a], is constant to b, falls over [b, 1] of the segment; `void` = linear | the values seen: (0, 1) linear, (0.61, 1) speed-up, (0, 0.4) slow-down, (0.41, 0.61) both; frames of the scenes using it match their sprites (`blupi-plat`: MarchEcrase 0.876) |
| `ORDE 6` composes Z·Y·X | only "Y before X" is verified (the stored poses have `AZVa` = 0); Z first is assumed from `ORDE 6` being the sixth permutation of XYZ |
| Orthographic camera: frame is `size` / `zoom`; the image fits its height; the axis hits the centre of the middle pixel | best IoU at 0.8775–0.885 x zoom; 60 / 68.03 = 0.882 |
| `ofst x,y` on an orthographic camera shifts its frame right and up by `ofst / zoom` scene units (`scene.camera_centre`) | `creature1-5.rds`' Caméra 2 (`ofst 24.94,60.80`) against icons 247-251: IoU 0.998 / 0.995 / 0.998 / 0.989 / 0.978 with the shift, 0 without; 29 Blupi cameras have one (the overcraft's `aero-*.rd4`: −8.50, 0.85) |

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
| Section `a` → −X, `b` → +Z for a path along +Y: section x, section y and the path's tangent are right-handed | `blupi.dxf` (`geometry.compare_with_reference`, stop-profil): the eyelash, an asymmetric section, lies 0.0136 from the DXF on average this way and 0.463 mirrored; the body 0.0018 / 0.0033, the foot 0.0082 / 0.0092; and the bird below |
| Mesh masters (`im`, Ray Dream Studio 5 boolean results such as `blupi-scie.rds`'s half body "Corps et Cube"): one `ff` line per triangle, 3 x (position, a second triple, u, v); the first triple is the position; `idls` gives each triangle's surface domain (0 = the swept body's own surface, u along the path, v around) | the saw scene's falling half matches Clear4's icons at frame IoU 0.71-0.87 that way, 0.31-0.44 with the second triple |
| Ray Dream Studio 5 shaders nest the channels in `scmp`/`ssub`; the saw's half face is a texture map in a rectangle domain (`srct`, bbox = around, along, around, along like Ray Dream 4's domains) over a plain yellow primary domain, placed unflipped | the eye lands where Clear4's icons have it (colour 0.83-0.94 per frame); the 7 other orientations score worse |
| The light casts shadows (`spot { shad 1 shdw 1 }`): the verifier renders them with a shadow map from the light's position; shadowed points keep only the ambient light | colour against the icons: walk 0.900 -> 0.926, tank 0.784 -> 0.794, overcraft 0.728 -> 0.750, jeep 0.888 -> 0.891 |
| Cube primitive (`prim { cube }`, no parameters): half-size 2.5, a 5-unit cube like the cylinder's height | helicopter frames of `helico2.rd4` against icons 61-65: IoU 0.972 / 0.981 / 0.985 / 0.976 / 0.966 at half-size 2.3 / 2.4 / 2.5 / 2.6 / 2.7 |
| Cone primitive (`prim { co }`, no parameters): base radius 3 at local Z −2.5, apex at +2.5, the cylinder's box | the mines of `bombe.rd4` (spikes) against icons 12-20: mean IoU 0.844 (icon 12: 0.981) with the apex at +Z, 0.629 with it at −Z; radius 2.5 / 3.5 or half-height 2 / 3 give 0.819-0.831 |
| Flat surface (`prim { COfl }`, no parameters, one object "Flat Surface" per scene that has it) is the scene's ground plane: not drawn | in all 87 scenes with one it lies horizontal at Z −15 (two scaled 0.9 in X, one moved to −7, −5.5) and is exactly edge-on to every orthographic camera (normal · view direction 0.000), so no sprite shows it; the game's own ground replaces it |
| Mesh form (`prim { vmda }`, Studio 5's polygon modeler): `xprm/PmSt/Pmsh` holds `Vrts` (`VerD`: `Posn`, `Flag`, `UV`) and `Pgns` (`PgSz` n, `Pgon` n indices; 98 % quads, a few n-gons up to 36), positions in the same units as the other masters, placed by the object's hot point like them; `CAng 80` in every file (the converter keeps its 60-degree crease) | the creatures above, with the object's `Scal` 0.125 and `hp` (0, 0, −81.51) |
| A texture map on a cube primitive: the surface is a 4 x 3 cross (`shading.cube_surface_coordinates`): sides -Y, +X, +Y, -X in the middle row (u 0-4, v 1-2), top above +X, bottom below; +X is the face toward the cameras; v runs down on -X. The primary domain's image spans its `bbox` and repeats, `srct`/`sovl` domains lie on top with their colour or image (white texels show what is beneath with `whit 1`); an image's first row is at the domain's low v unless `flpv 1` | `Decor/techno02.rd4`'s screen and the switch's picture sit at u 1-2 on the camera's face; the invert sign (`panneau-invert.rd4`) shows its upside-down Blupi on both board faces in icons 187-194 only so (front colour 0.850 against 0.815 the other way up) |
| A mix shader (`smix`: two colours by a pattern such as wood, `swod`) is drawn in the mean of its two colours | the invert sign's post: gold, not the master's red |
| A cross-section may hold several closed curves (`p2`, in nested `pg`/`pc` groups) of cubic `s3` and straight `s1` segments; curves inside others are holes (the tank's track loop, the wheel rims) | vehicle frames against their icons (below) |
| A point without its own cross-section keeps the previous one; different cross-sections along the path blend into each other; a path ending where it starts (torus, tyre) is closed, without caps | the jeep's tyres and the overcraft's skirt are tori built this way |

`freeform_mesh.py` documents the rest (sampling, caps by ear clipping, holes bridged into their
outline).

### Face texture

| Convention | Evidence |
|---|---|
| Bitmaps: `TILE` lines of PackBits streams, edge tiles cropped; 8-bit with a `CTAB` palette, 1-bit masks; 32-bit tiles are three planes for the whole tile (every red byte, then green, then blue) | the decoded face; the lollipop's 32-bit textures decode to clean, seamless diagonal stripes only this way (per-row planes or interleaved bytes give fringed grey) |
| A texture map on a cylinder primitive: `s` = angle about the axis; `t` the meridian fraction of the canonical cylinder (radius 3, height 5): the -Z cap from its centre out (0 to 3/11), the side along +Z (to 8/11), the +Z cap back to its centre; `cnth`/`cntv` repeats (baked into the image), `flph`/`flpv` flips | the pickup lollipop (`sucette.rd4`) shows the sprites' four-arm pinwheel, icons 136-143 IoU x colour 0.821 (0.807 with `t` the cap's radius over the whole map: a tight many-turn spiral); the lollipop Blupi holds, icons 234-236, 0.850 / 0.851 |
| Older scenes (`t.rd4`) write lowercase `ctab`/`tdta`/`tile` leaves: `$`, 8 hex digits of length, a CR, that many raw bytes (CR and LF included) and a closing `$`; tiles have no `FFFFFFFF` header | `t.rd4`'s face decodes to the same layout as the others |
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

Every frame is also tried turned about the vertical through Blupi's body (`--yaw-step`, 45
degrees, refined to 5.6 on each icon's best frame), not mirrored: the turn's front views
(icons 1-3), the switch's back view (268-269) and the vehicles' slight yaws. The `--tumble` icons
(Hide's, 276-284) are also tried pitched about the camera's horizontal axis and scaled. A turned
match is kept (`view_*` columns) and the clip library uses it when it beats the plain frame.

First run (2026-10-24, about 3.5 minutes): 243 frames from 54 scenes; of the 231 icons used by
actions, 87 matched confidently (score >= 0.85), 60 probably (>= 0.70) and 84 not. With the
vehicle geometry, group scales and turned views (2026-10-24, 37 minutes): 259 frames from 56
scenes; 114 confident, 71 probable, 46 unmatched. With the saw scene's meshes and the lollipop's
textures (273 frames): 116 confident, 78 probable, 37 unmatched. With the saw's eye and cast
shadows (about 80 minutes): 121 confident, 76 probable, 34 unmatched. Still unmatched: the vehicles' own turn
icons, the overcraft (0.55-0.63: silhouette right, colours lighter than the sprites), frames with
2D effects (Electro's glow, deaths Clear2/Clear4) and the ones rendered from scenes that are not
in `speedyblupi-data`. Hide's tumbling small Blupi comes from `naissance.rd4`, where the `BLUPI`
group is scaled 0.635.

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

Vehicle clips (`VEHICLES`: helicopter, jeep, tank, overcraft, skateboard) also write
`vehicles/<name>.glb`: every object of their frames' scenes that is not Blupi's body, as rigid
parts (one joint each under a root, carrying the part's whole geometry frame, hidden by scaling
where a frame's scene lacks it) with the same clips, in the same space as Blupi. They are
separate models because CNA's skinning takes 72 bones and the tank alone has 40 parts; the game
draws a vehicle with Blupi's transform and clip time. `clips.txt` names them (`vehicle <name>`).
Vehicle clips accept matches down to 0.55 (`VEHICLE_USABLE`): without cast shadows, and with the
overcraft's skirt lighter than in the sprites, their colours score lower while silhouette and
pose still agree.

Result (2026-10-24, 45 clips, 58 joints, 5 faces, 5 vehicles), mean silhouette IoU against the
icons (vehicle clips rendered with their vehicle): March 0.981, Stop 0.970, Jump 0.979, Air
0.961, Down 0.946, Up 0.980, Push 0.988, StopEcrase 0.965, MarchEcrase 0.950, Balloon 0.976,
StopNage/MarchNage 0.980, StopSurf/MarchSurf 0.981, Drown 0.982, Mockery 0.971, Mockeryi 0.943,
Mockeryp 0.950, TakeDynamite 0.962, PutDynamite 0.966, Sucette 0.957, Drink 0.940, Charge 0.950,
Teleporting 0.944, Switch 0.946, Hide 0.921; StopHelico 0.981, MarchHelico 0.935,
StopJeep/MarchJeep 0.951, StopTank 0.893, MarchTank 0.931, FireTank 0.878, StopSkate 0.950,
MarchSkate 0.982, JumpSkate 0.979, AirSkate 0.914, TakeSkate/DeposeSkate 0.951, StopOver 0.764,
MarchOver 0.739; the deaths with 2D effects lower (Clear1 0.775, Clear3 0.655, Glu 0.814) and
Clear4 0.843 (the saw scene's half body with its eye). Clear2 has no clip: every frame of it is
invisible (the game draws no model on the tables' -1 frames).

## Object models (billboard replacement)

`build_object_library.py` turns the game's other objects into models the same way: per object of
`objects.txt` (name, sprite sheet, icons, scenes, `yaw=STEP`, `vflip=1`, `camera=PART`,
`recolour=`), every icon is matched against every distinct frame of its scenes, with each of
their orthographic cameras (straight, flipped left-right, with `vflip` upside down, and with
`yaw` turned about the vertical), and each winning scene becomes a model of rigid parts (parts
moving together share a joint: the treasure's 91 objects need 34) with its frames as the
clip `Frames` and an `icons.txt` naming each icon's frame, mirror and yaw. The model is checked
by rendering it back exactly as the game draws it; `export_objects.py` writes
`avatars3d/objects/` (its README has the commands). Mean IoU against the sprites
(`build-probe/rd4-objects/report.md`, 2026-10-24): treasure 0.964, egg 0.971 and exit arrow
0.937 (both turned: their sprites spin), keys and secret exit 0.962, lollipop 0.985, invert sign
0.999, dynamite 0.975, drink 0.871, ammo 0.992, enemy projectile 0.995, mine 0.943 (also upside
down for the hanging mine), moving mine 0.984, decoys 0.957 (the mine recoloured), creature
0.978-0.998, bird 0.894, fish 0.920, wasp 0.955 / 0.961, bulldozer 0.950, follow star 0.870,
tentacle 0.973-0.994 (17 scenes). explo.png's icons are compared in their own square
(`survey.EXPLO_ICON_HEIGHTS`).

A flipped icon is the sprite image flipped about its centre line, as the original game flipped
sprites: the hanging mine's icons 48-56 are exactly 12-20 flipped (IoU 1.000), and the rendered
mine flipped that way matches them at 0.981 against 0.769 for a mirror about the camera axis
(which passes through the middle pixel's centre, half a pixel off that line). The game flips a
model about the same line (`ObjectModelTable.hpp`, `kFlipShift`).

## Archive survey

`survey.py scenes` reads every scene of the archive and lists, per scene, whether it parses, its
master kinds and the masters the converter cannot build; `survey.py icons --sheet <png>` matches
every icon of a sprite sheet against renders of the scenes without Blupi (their own orthographic
cameras), giving the scene each object's sprites were rendered from. Commands in the script's
docstring; results feed `assets-3d-analysis.md` and `plan.md` Phase 7B (`BB3D-*`).

## Not converted yet (next steps)

- Procedural shaders (the skateboard's wood shows its flat colour); texture maps in rectangle
  domains of swept (free-form) parts (only the mesh masters' are read).
- Masters still skipped: stars (`COFA`), 3D text (`tx`), fog (`fogg`); 3ds Max (`max/`,
  `Element/shield*.max`) and Amapi (`.a3d`) files are not Ray Dream at all.
- Texture maps on spheres; twisted sweeps (none in the Blupi scenes). Procedural patterns are flat
  colours (`smix` the mean of its two).
- Highlight, shininess and the other shader channels (the glTF material is plain PBR).
