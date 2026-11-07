# Material that derives from the original game

Galaxy Eggbert is a 3D remake of **Mobile Eggbert** (this workspace's C++ port of *Speedy Blupi*),
and the repository says so on its first lines. This file lists what in the tree derives from the
original game, according to the repository's own records, and what permission or licence status those
records document.

Terms. *The original*: Speedy Blupi, by Daniel Roux and EPSITEC SA (the names in the repository's own
text), a Windows Phone XNA game of 2013 (`README.md`), with Speedy Eggbert 1
(1998, per `render_music.py`) named as the source of the music. *Mobile Eggbert*: `../mobile-eggbert`, "a modified version of
Speedy Blupi ... decompiled by the ILSpy to the C# source code, migrated from XNA 4.0 to Monogame,
migrated from C# to C++, migrated from Monogame to CNA" (its README, read). *speedyblupi-data*:
`../speedyblupi-data`, the original Ray Dream and other 3D sources, which this audit did **not**
read (instruction); what is said about it comes from this repository's files.

Nothing here decides whether any of it may be redistributed. The open questions are in `README.md`
and `licences.md`.

## 1. Summary by kind

| Kind | Where | Size | Relation to the original |
|---|---|---|---|
| Sprite sheets and UI images | `Content/icons/*.png` (9: `blupi`, `blupi1`, `element`, `explo`, `object-m`, `button`, `jauge`, `text`, `pad`) | 4.6 MB | Copied files |
| Tile textures and screen images | `Content/backgrounds/` (38 PNG: `decor000..031`, `init`, `pause`, `lost`, `win`, `setup`, `trial`, `wait`, `gear`, `speedyblupi`, `blupiyoupie`) | 13 MB | Copied files |
| Sound effects | `Content/sounds/sound000..092.wav` (93) | 4.7 MB | Copied files |
| Music | `Content/music/music000..009.ogg` (10) | 27 MB | The original's MIDI tracks, rendered by a script (this repo's own rendering) |
| Original-format level files | `worlds/world001..005.txt` | 5 files, 1 223 lines | Copied files |
| Cropped sprite images and animated GIFs | `mobile-eggbert-reference/images/` (542 PNG, 129 GIF) | about 9 MB | Cut from the sprite sheets |
| Screenshots with original sprites in them | `plans/evidence/` (89 PNG, 32 JPG), `tests/golden/*.png` | | The game rendering the above |
| 3D models | `avatars3d/` (Blupi, five vehicles, 121 object model directories, 145 terrain reliefs) | 70 MB, 2 491 files | Geometry, rigs, textures and animation timing converted from the original's Ray Dream scenes by `tools/rd4_to_gltf`; colours tuned to the original sprites |
| Data tables and constants in C++ | `include/GalaxyEggbert/Def/*.hpp`, `src/GalaxyEggbert/Game/*Table*.hpp`, `ObjectIcons.cpp`, `BlupiController.*`, `CameraShake.cpp`, `TextMetrics.hpp`, `Sound.cpp`, `TrainingHints.*`, `Editor/AutoTile.hpp`, `BlockDefinitionRegistry.cpp` | see section 4 | Transcribed from Mobile Eggbert's C++, as the records state |
| Behaviour | `src/GalaxyEggbert/Game/` | 38 files carry 662 lines that cite the original's `Decor.cpp`, `Tables.cpp`, `Sound.cpp`, `InputPad.cpp` ... | Re-implemented in 3D from the original's logic with file:line citations; not copied text (the prose spec is `mobile-eggbert-reference/`) |
| Data headers traced from original art | `BoltPaths.hpp` (sprites 305-307), `GlueStrands.hpp` (168-171), `LavaSurfaceData.hpp` (original Amapi `.a3d` sources) | | Generated from original sprites or sources by scripts |
| World design | `worlds3d/` (81 `.vwr`) | | Original designs: each mission takes theme, lesson, obstacles and objects from the original level of the same number (`plans/briefs/worldNNN.md`), not its map; `world001.py` takes the sign pairing and `ORIGINAL_MUSIC` the music number from the original's level headers |
| Names, IDs, story | everywhere | | Block icon numbers, `ObjectType` IDs 0-203, sound channels 0-92, mission numbers, object names are the original's (CLAUDE.md: "Preserve real numeric IDs ... they encode reference data") |

## 2. Sprites, backgrounds, sounds, level files

- Origin record: commit `92bd4bd5` (2026-06-11), "Phase 2.2 - copy assets from mobile-eggbert (Speedy
  Blupi)": "93 WAV sound effects (sound000-sound092), 9 sprite sheets (...), 28 tile textures
  (decor000-decor031) and 9 screen backgrounds (...). Textures and screens are placeholders -
  can be replaced with 3D-native artwork in the future without code changes."
- `worlds/world001..005.txt`: commit `b360df3a` (2026-06-13), "real mobile-eggbert world loading
  (.txt format)". `worlds/world001.txt` is still read for the main hub's sign pairing and by
  `tools/qa_original.sh`.
- Byte-for-byte equality of these files with the ones in `../mobile-eggbert` was **not** checked for
  this audit. The repository's statement is that they are copies.
- `Content/Content.mgcb`, `Content/icons/*.xml`: first added in `ef03c601` (2026-05-12) and
  `e741cc24`; origin not recorded.
- The build used to re-copy `../mobile-eggbert/Content` on every build; since then the game reads only
  this repository's `Content/` (`CMakeLists.txt` comment: "the game only reads this repository's ~22
  MiB Content tree"; the 4x-scale variants in Mobile Eggbert are not used). `Content/music/` (27 MB)
  was added afterwards.
- `Content/`, `worlds/`, `worlds3d/`, `textures3d/` and `avatars3d/` are all installed into the Linux
  bundle (`install(DIRECTORY Content worlds worlds3d textures3d avatars3d ...)`); the web build
  preloads `Content/icons`, `backgrounds` and `sounds` (the music is not in the web build yet, per the
  commit message of `49ce625f`).
- `plans/audit/asset_sources.csv`: 703 rows, one per file of the original 3D sources
  (`../speedyblupi-data`): 528 Ray Dream Studio scenes, 105 Amapi `.a3d`, 56 3ds Max `.max`, 10 DXF, 1
  glTF, 1 VRML, 1 Ray Dream Designer 3 scene, 1 3DS. Status in the ledger: 288 FIXED_AND_VERIFIED
  (used), 10 INTENTIONAL_3D_ADAPTATION, 405 NOT_APPLICABLE. It describes which source file is
  drawn by Galaxy and how it was verified; it records no licence per file.

## 3. Music and sound processing

- `Content/music/music000..009.ogg`: "the game's own `SOUND/MUSIC000-009.BLP` (Standard MIDI,
  `Speedy_Eggbert_1.7z`), rendered by `tools/music/render_music.py` through libfluidsynth with the GM
  soundfont, the trailing silence cut ... and the loudness normalised ... (Vorbis, ~2.7 MB each)"
  (commit `49ce625f`, 2026-11-02; `handoff.md`; `World Format.md`).
- The MIDI files are **not** in the repository. They come from an archive named `Speedy_Eggbert_1.7z`
  (Speedy Eggbert 1, 1998, per `render_music.py`), whose location and licence the repository does not
  record. The original's music is Speedy Eggbert's, not Mobile Eggbert's; the repository does not say
  which notice (MIT in Mobile Eggbert, GPLv3 in speedyblupi-data, neither, or a different one) covers it.
- The soundfont used is not recorded (see `third-party.md`). The rendering is therefore a derivative
  of two inputs, the MIDI and a soundfont.
- Sound effects: the original's WAV files are played as they are; `Sound.cpp` applies the original's
  `tableVolumePitch` (volume and pitch per channel), measured by `tools/audit/sound_pitch.py`
  (`release_audit.md` section 4).

## 4. Data tables and values transcribed from the original C++

The rule recorded in `CLAUDE.md` / `AGENTS.md`: "`../mobile-eggbert` is read-only ... Copying code/data
tables, enum values, byte layouts, or sprite-frame tables into this repository requires explicit user
approval; since 2026-10-25 a standing approval covers small data tables needed for fidelity (glyph widths,
animation and timing tables), citing the source. Larger code still needs asking." "Reusing approved
media assets by their existing paths is allowed where already established. Do not add new copied asset
sets without approval."

"Approval" in these documents is the **owner's** permission to the working agents, given as owner of
Mobile Eggbert and of this repository. It is not a licence from the original's authors.

| What | File | Source cited | Approval recorded in the source |
|---|---|---|---|
| Game enums: `ObjectType` (0-203), `SoundChannel` (93), `BlupiAction`, `DecorAction`, `SpriteChannel`, `Direction`, `GamePhase`, `SecretPower`, ... | `include/GalaxyEggbert/Def/*.hpp` (12 files, 674 lines) | Mobile Eggbert's enums ("Values mirror the CH* constants from the original game") | Commit `d6d57677` 2026-06-11, "port game enums from mobile-eggbert (Speedy Blupi)"; "Numeric IDs must NOT be renumbered" |
| `table_decor_quart`: 441 icons x 16 sub-cell solidity bits, 7 056 values | `src/GalaxyEggbert/Game/DecorQuartTable.hpp` (466 lines) | `Tables.cpp:469-1177` | "Transcribed verbatim (explicit user approval, 2026-07-21, INFRA-005 ...) extracted mechanically and cross-checked via two independent extraction passes" |
| `table_decor_action[519]` (camera shake) | `CameraShake.cpp` | `Tables.cpp:2027-2081` | "data-table transcription approved" |
| `table_blupi` icon sequences (Bye, Clear1/4, Glu, Drown, DeathLocked, Teleporting, Balloon, StopEcrase ...) and `table_mirror` | `BlupiController.cpp`, `.hpp` | `Tables.cpp`, `table_mirror` at `Tables.cpp:413-447` "transcribed verbatim" | "explicit user approval" dated 2026-07-11 and 2026-07-16 in comments |
| `table_dynamitef`, `table_electro[90]`, `kSploutch2/3` | `ObjectIcons.cpp`, `.hpp` | `Tables.cpp:1578-1590` and others | "standing approval" / "explicit user approval" |
| `Text::table_width` (glyph widths, 128 values) | `src/GalaxyEggbert/Game/TextMetrics.hpp` | Mobile Eggbert's `Text::table_width`; the comment states "mobile-eggbert is MIT, Daniel Roux and Robert Vokac" | "the owner's approval (2026-10-25)" |
| `tableVolumePitch` (volume and pitch per sound channel, 100 channels x 2) | `src/GalaxyEggbert/Game/Sound.cpp` | Mobile Eggbert's `Sound.hpp` `tableVolumePitch`; playback rule from `Sound.cpp:96-110` | "Ported verbatim, with approval" |
| `table_training1..4` | `TrainingHints.cpp`, `.hpp` | `Tables.cpp` | "explicit user approval, 2026-07-13" |
| `table_adapt_decor` (nine families of sixteen) | `src/GalaxyEggbert/Editor/AutoTile.hpp` | `Tables.cpp:1668`; rules from `Decor::IsRightBorder` (`Decor.cpp:10367-10675`) "ported below rule by rule" | "standing approval for small data tables" |
| Button rectangles and layout formulas of the on-screen pad | `InputPad.cpp`, `.hpp` | `InputPad.cpp` of the original, "independently re-derived" | n/a (derived values) |
| Per-world background music number | `tools/worldkit.py` `ORIGINAL_MUSIC` | header `music=` of the original levels | n/a |
| HUD digits | `Hud.hpp` | "NOT transcribed from mobile-eggbert's table_char" (measured from the sheet instead) | |

The citations above come from comments in the sources. Which individual numbers match the original
exactly was not re-verified for this audit.

For behaviour (as opposed to tables), the prose specification in `mobile-eggbert-reference/` (files 01
to 15, "DOC-001..DOC-306") "reads directly from `../mobile-eggbert`", with "explicit, scoped user
approval recorded in `CLAUDE.md` (behavior in prose + key numeric constants; not pseudocode, not
verbatim code transcription ...)". The same file warns: "This is documentation, not a license to copy
the logic into actual game code." `REMAKE-ANALYSIS.md:339` (2026-07-20): "No copied code/data. P0-2
*validates* transcription; it does not license copying tables in."

The source files of the game do not copy any other file of the original, as far as the repository
says; this was not checked line against line.

## 5. 3D models converted from the original Ray Dream scenes

**The converter** (`tools/rd4_to_gltf/`, 30 files, 7 076 lines; README):

- *What it reads.* Original Speedy Blupi 3D scenes in Ray Dream Designer 3 / Studio 4-5 ASCII "3DC" format
  (`.rd4`, `.rds`), "one per action"; 97 in `Blupi/`, 56 of them with the full rigged Blupi; scenes
  for objects, terrain and decor in `Decor/`, `Explo/`, `Element/`. Also, for checking only,
  `Blupi/blupi.dxf` (Ray Dream's own triangulation of Blupi) and the original sprite sheets.
- *What it does.* Parses the scene graph, keyframes, joints and materials; regenerates the geometry from
  the scene's own masters (spheres, cylinders, cubes, cones, free-form sweeps, mesh forms), including
  texture mapping, the face bitmap (PackBits) and Ray Dream's camera and light; writes a skinned glTF;
  verifies it by rendering every frame with the game camera against the original sprite icons
  (silhouette IoU; Blupi's walk 0.978-0.985). `build_object_library.py` and `export_objects.py` do this
  for all objects; `build_clip_library.py` builds Blupi's clip library from the sprite-icon tables.
- *Documentation.* The README lists each Ray Dream convention with a numeric evidence column; the
  format is documented from the files, not from vendor documentation.
- *What the output contains.* Original geometry, original face texture, original animation timing and
  poses; tone corrections (`calibrate_tone.py`: "per-object colour tones calibrated from the sprites"),
  generated spark decals (`dynamite_sparks.py` cuts "the three painted stars from element.png",
  `electro_sparks.py`), sign decals (`sign_decals.py`), generated bolt paths. Procedural textures
  (palm bark, wood grain, metal noise) are flat colours because "Ray Dream's noise algorithms are
  unknown".
- *Records of the source status.* `avatars3d/blupi/README.md`, `avatars3d/objects/README.md`,
  `tools/rd4_to_gltf/README.md`, `plans/inventory/assets.md` (generated by `tools/asset_inventory.py`)
  all say: the scenes are in `speedyblupi-data/originals`, "GPLv3, (c) Daniel Roux & EPSITEC SA (the
  `speedyblupi-data` repository's README)". That is the only licence statement for these sources in
  this repository. It was not verified against the speedyblupi-data README.
- Models in the repository: `avatars3d/blupi` (one skinned model, five vehicles under `vehicles/`),
  `avatars3d/objects/` (121 model directories; `plans/inventory/assets.md` lists 103 and may be older),
  `avatars3d/terrain/` (145 `.gtr` reliefs, "each ... a terrain tile's original Ray Dream scene ... the
  scene's front half, decimated to about 700 triangles ... textured by the tile's sprite projected from
  the front").
- `Blupi` shields and the charge stay the original sprites; the 56 `.max` files are not used
  (see `third-party.md` section 5).

## 6. What permission or licence status the repository documents

Documented:

1. **This repository is GPLv3** (`LICENSE`). Nothing in the repository says it relies on any other
   grant for the original's material.
2. **speedyblupi-data**: "GPLv3, (c) Daniel Roux & EPSITEC SA", quoted from "the speedyblupi-data
   repository's README" in four places (`avatars3d/blupi/README.md`, `avatars3d/objects/README.md`,
   `tools/rd4_to_gltf/README.md`, `plans/inventory/assets.md`).
3. **mobile-eggbert**: `../mobile-eggbert/LICENSE` (read) is MIT, "Copyright (c) 2013 Daniel Roux,
   Copyright (c) 2024-2026 Robert Vokac". `TextMetrics.hpp` repeats this ("mobile-eggbert is MIT, Daniel
   Roux and Robert Vokac"). The repository does not say whether that MIT notice covers the game assets
   (PNG, WAV) in addition to the C++ code.
4. **The owner's approvals to the agents** for copying tables and assets (CLAUDE.md, ledgers,
   comments), described in section 4. They are permissions inside the project.
5. **A licence concern about a tool, not about an asset**: the 3ds Max episode (MR-0001, in
   `third-party.md`).
6. The original's 3D sources are kept out of the repository and "never uploaded anywhere either"
   (`plans/codex/README.md`; `plans/audit/manual_review.md`: "Never upload them to an online
   converter").

Not documented anywhere in the repository:

- Any correspondence, grant or permission from Daniel Roux or EPSITEC SA.
- Whether EPSITEC SA's rights in the original game's graphics, sounds and music are covered by the
  GPLv3 statement quoted above, by the MIT notice of Mobile Eggbert, or by neither.
- The licence or source of the Speedy Eggbert 1 MIDI files and of the soundfont used to render them.
- The status of the sprites reused as textures of the 3D terrain, as `Content/`, in
  `mobile-eggbert-reference/images/`, in the committed screenshots, and in the Linux bundle.
- Any attribution file that a distribution of the game would carry for the original's authors
  (the Linux bundle carries `LICENSE` only).
- The origin of `Content/Content.mgcb` and of the Android launcher icons.
