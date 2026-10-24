# 3D sources and the billboards — analysis (2026-10-24)

User questions: were all Ray Dream models and animations (and other 3D formats) converted to glTF;
are Galaxy Eggbert's Blupi models glTF; can every billboard be replaced by a 3D model, generating
one where no 3D source exists.

Evidence: `build-probe/rd4-survey/` (regenerable): `scenes.csv` reads every `.rd4`/`.rds` in
`speedyblupi-data/originals` with `tools/rd4_to_gltf`; `element_icons.csv` matches every
`element.png` icon against renders of the scenes that do not contain Blupi, with each scene's own
orthographic cameras; `object_icons.txt` lists the icons, sheet and render mode of every
MoveObject type (`ResolveObjectVisual`, phases 0-399).

## 1. What the game loads

The real Blupi is **not** loaded as glTF. `tools/rd4_to_gltf` writes `.glb` files as the
intermediate (`build-probe/rd4-blupi/library/blupi.glb`, `vehicles/*.glb`, not committed);
`export_cna_avatar.py` turns them into CNA's `SkinnedModelEXT` directory format, which is what
`avatars3d/blupi/` holds: `avatar.skinnedmodel.json`, `skeleton.bin`, per part `*.verts.bin`,
`*.idx.bin` and a texture PNG, `clips/*.clip.bin`, plus `clips.txt` and the face PNGs. The vehicles
use the same format under `avatars3d/blupi/vehicles/<name>/`.

## 2. 3D sources in `speedyblupi-data/originals`

| Format | Files | Content | Status |
|---|---|---|---|
| Ray Dream Designer 4 / Studio ASCII `.rd4` | ~420 | Blupi (87), decor tiles and objects (`Decor/`, 190), big decor (`Explo/`, 30), menu frames (`Cadre/`, 32), backgrounds (`Back/`, 12) | read by the converter |
| Ray Dream Studio 5 `.rds` | ~65 | the same kinds, plus the creatures, the saw death, tentacles, drops | 47 read, 28 not (an RDS 5 variant the parser does not take yet: `Decor/goutte`, `tentacule`, `smooth`, `palais`) |
| 3ds Max `.max` (OLE compound files) | 44 | menu screens and buttons (`max/`), Blupi variants (`blupi-main/skate/stop.max`), shields (`Element/shield*.max`) | not readable without 3ds Max or a third-party importer (Blender's community `.max` importer reads only some versions' meshes) |
| Amapi `.a3d` (`AMAP BLOKSHAP`) | 6 | `creature1-5`, `foreuse` (drill) | proprietary; the creatures also exist as `.rds` |
| VRML `.wrl`, glTF `.glb` | 1 + 1 | `Back/sapins` (fir-tree background) | the `.glb` already exists (a Khronos converter's output) |
| DXF | 6 | Ray Dream exports of Blupi, `bnc`, `coffre` | used as reference for Blupi's geometry |

Of 528 Ray Dream scenes, 500 parse. Masters the converter does not build yet, by count over all
scenes: mesh-form models `vmda` (83: creatures, tentacles, drops, lava, the palace, smooth decor),
flat surfaces `COfl` (87), cones `co` (15: the bombs' fuses, the fish's fins), 3D text `tx` (14,
menus only), stars `COFA` (2), fog `fogg` (1), and 18 free-form variants that fail (`ex`).

## 3. What is converted

- 32 of the 74 scenes containing Blupi feed his clip library (45 clips) and the five vehicle
  models. The other 42 are actions the game has no state for (hanging on a bar
  `suspendu*`, the pistol `pistolet*`/`tire-profil`, Electro, sick, angel, head bump), alternates
  of used ones (`nage`, `helico`, `blupi-ouf1/2/4`, `aero-left/stop/turn`), the menu frames
  (`Cadre/`, 12 with Blupi), and test scenes (`t.rd4`, `tt.rd4`).
- No scene without Blupi is converted: enemies, pickups, decor, big decor, menus and backgrounds.

So: not all models and animations are glTF; only what the third-person Blupi uses.

## 4. Billboards in the game and their sources

The game draws as camera-facing billboards: MoveObjects whose sprite comes from `element.png`,
`object-m.png`, `explo.png`, `blupi.png` or `blupi1.png`, and the big decor cells (`explo.png`).
Terrain is voxels; platform lifts and crates (types 1, 12, 47, 48) are textured cubes; vehicles
parked or ridden are now 3D.

**Objects with a confirmed 3D source** (best icon IoU of the scene's render against the sprites,
own orthographic camera):

| Type | Object | Scene | IoU |
|---|---|---|---|
| 5 | treasure | `Blupi/coffre.rd4` | 0.93 mean, 0.97 max |
| 6 | extra-life egg | `Blupi/oeuf.rd4` | 0.84 / 0.99 |
| 7 | level exit | `Blupi/fleche.rd4` | 0.77 / 0.99 |
| 17 | fish | `Blupi/poisson.rd4` | 0.89 / 0.91 (fins are cones, not built yet) |
| 20 | bird | `Blupi/oiseau.rd4` | 0.77 / 0.83 |
| 23 | enemy projectile | `Blupi/balle-glu.rd4` | 0.99 |
| 29 | ammo pack | `Blupi/munitions-glu.rd4` | 0.99 |
| 30 | drink | `Blupi/bouteille.rd4` | 0.87 |
| 96, 97 | follow enemies | `Blupi/bombe-follow.rd4` | 0.61-0.64 mean, 0.97 max |
| 13, 19, 24, 28, 46 | vehicles | `*-seul.rd4`, `skate.rd4`, models done | 0.86-0.97 |

**Objects whose source is named but not yet confirmed** (matches 0.4-0.7: the scene renders with
the wrong camera, a perspective one the verifier cannot render yet, or with masters skipped). By
what their icons show:
- red spiked mines (types 2, 3: icons 12-20, 48-56; also among 16's and 32's): `bombe.rd4`,
  `bombe-perso*.rd4`, `bombe-move.rd4` (the spikes are cones, not built yet; 0.62/0.69);
- the yellow bulldozer (type 4: icons 65-67) and the flying robot (icon 68): `robot-profil.rd4`,
  `tracks-profil.rd4`;
- the green blob creature (types 33, 54: icons 247-251): `creature*.rds` (mesh-form `vmda`);
- the wasp (type 44): `guepe.rd4`, `guepe-tourne.rd4`; the spider (type 16): `bug*.rd4` at the
  archive's root;
- keys and the secret exit (types 49-51, 21): `cle*.rd4`; shield (25): `shield1/2.rd4`; lollipop
  pickup (26): `sucette.rd4`; invert sign (40): `panneau-invert.rd4`; dynamite (55, 56):
  `Decor/dynamite.rd4`; charge (31, object-m.png 238-243): not found yet (`Element/power.rds`,
  first named here, is an icosahedron);
- tentacle (53): `Decor/tentacule/*.rds`; electric arc (38): `Decor/electro.rd4`, `eclair.rd4`;
  door (22): `Decor/porte*.rd4`; drops and splashes: `Decor/goutte/*.rds`; cloud puff (36):
  `Explo/nuage.rds`.

The code's names for types 32 and 33 ("blupih", "blupit") do not match what their icons show
(66-70: bulldozer, robot, mines; 248-250: green blobs); worth checking against mobile-eggbert
before converting them.

**Big decor** (`explo.png` cells): `Explo/` holds their scenes (`cabane`, `maison`, `plante1a-3c`,
`enfant01-13`, `meca1-4`, `herbe`); 10 of 31 build today, the rest need `COfl`/`co`/fog.

**Other Blupis** (types 200-203, `blupi.png`/`blupi1.png`): the real Blupi model with recoloured
textures; `originals/readme.txt` documents the recolouring (yellow to orange 237,168,60, blue
63,194,237, green 38,233,46).

**No 3D source found**: the explosions (8-11, `explo.png`), flashes and sparks (90-93), water
splashes from `explo.png` (98-100) and `object-m.png` (14, 15, 35), sparkle trails (27, 39, 57),
particle bursts (41, 42), goo (34), the bridge being built (52). `Explo/` despite its name holds
big decor, not explosions; these effects were drawn or rendered elsewhere.

## 5. Can every billboard go?

- **Objects (enemies, pickups, exits, keys, projectile, other Blupis): yes.** Their sprites were
  rendered from scenes that are in the archive; the same pipeline (static models or clip
  libraries per object, the scene frame matched to every sprite icon) applies. Needed first:
  `vmda`, `COfl`, `co`, `COFA` masters, the 28 RDS 5 files that do not parse, and perspective
  cameras in the verifier (wasp, keys, robot). Animated objects (bird's wings, fish, wasp,
  creature) get clips per state the way Blupi did.
- **Big decor: yes**, static models from `Explo/`, after the same master work.
- **Effects: no source to convert.** Billboards are the usual way to draw such effects in 3D and
  keep the original look exactly. Replacing them means generating new 3D (particle systems,
  procedural meshes for explosions and splashes), which the faithful-remake rule allows only as
  presentation, not new behaviour; it would not trace to anything in the originals.
- **3ds Max and Amapi files** matter only for menus and alternates already present as Ray Dream
  scenes (shields, creatures), so no world object depends on them.

Suggested order: masters (`vmda`, `COfl`, `co`, `COFA`) and the RDS 5 variant → perspective
cameras in the verifier → static pickups (treasure, egg, exit, keys, drink, ammo, projectile,
dynamite, shield, invert, lollipop) → animated enemies (bird, fish, wasp, mines, bulldozer and
robot, spider, creature) → big decor → other Blupis → decide on effects (keep billboards or
generate).
