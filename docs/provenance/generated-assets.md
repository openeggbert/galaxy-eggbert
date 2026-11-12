# Assets generated or processed by tools: source, tool, output, reproducibility

For each asset directory: where the input comes from, which tool turned it into the committed file,
whether that tool is in this repository, and whether the output can be reproduced from what is
available. "Reproducible" here is judged from the repository's records and file layout. **Nothing was
re-run for this audit** (no builds, no game runs, no generator runs; generators write into the tree).

Legend for the last column: **yes** = inputs and tool are available from the repository plus the
named sibling repositories and the documented commands; **partly** = something needed is outside the
repository or not recorded; **no** = cannot be rebuilt from the repository.

| Asset directory | Source | Tool | In repo? | Output | Reproducible |
|---|---|---|---|---|---|
| `Content/icons/`, `Content/backgrounds/`, `Content/sounds/` | `../mobile-eggbert/Content` | none: copied by hand/commit `92bd4bd5` (2026-06-11). Earlier CMake re-copied them on every build; this was removed in favour of this repo's tree | no tool | 9 sprite sheets, 38 images, 93 WAV | The copy is the artefact. No script re-creates it; the source is the sibling repository. |
| `Content/music/` | `MUSIC000..009.BLP` (Standard MIDI) from `Speedy_Eggbert_1.7z`; a General MIDI soundfont | `tools/music/render_music.py` (libfluidsynth via `ctypes`; `--ogg` uses ffmpeg: trailing silence cut, EBU R128 -18 LUFS, Vorbis q4) | yes, the script | 10 OGG, 27 MB | **partly**. The MIDI archive and the soundfont used are not recorded (location, checksum, licence); the fluidsynth and ffmpeg versions are not recorded; byte-identical output is not claimed. |
| `worlds/` | Mobile Eggbert's level `.txt` files | none: copied (commit `b360df3a`) | no tool | 5 files | Copy only. |
| `worlds3d/*.vwr` | Written briefs (`plans/briefs/worldNNN.md`), the original level's header, the shared kits | `tools/worlds/worldNNN.py`, `title.py`, `gallery.py`, shared `kit.py`, `tools/worldkit.py`; `world999` by `GenerateSampleWorld3D` (C++) | yes | 81 `.vwr` | **yes, in principle**. `worldkit.py`: "deterministically"; 79 files seed their RNG; `handoff.md` records one byte-for-byte reproduction (`--stage -1` reproduces world 061). Not re-run here; a full byte-identical regeneration of all 81 files is not recorded as checked. |
| `tools/autoplay/*.txt` | The worlds | written, then re-timed by `autoplay_search.py` and `autoplay_chains.py` | yes | 83 scripts + `spare/`, `probes/` | Scripts are the artefact; they are replayed by the game's `--autoplay` and `tools/play_pass.sh` (78/78 on 2026-11-07 per `handoff.md`). |
| `avatars3d/blupi/` (model, clips, face PNGs, five vehicles) | Original `Blupi/*.rd4` Ray Dream scenes (`speedyblupi-data`, not read here), `blupi.dxf`, sprite sheet `blupi.png`, `BlupiController.cpp`'s transcribed icon tables | `match_icons.py` -> `build_clip_library.py` -> `export_cna_avatar.py`, then CNA's `tools/avatar_asset_pipeline/convert_avatar.py` | repo tools yes; CNA converter in a sibling repo | 39 files for Blupi, 99 for the vehicles, 68 clips | **partly**. The exact commands are in `avatars3d/blupi/README.md`. Needs `speedyblupi-data`, `mobile-eggbert` and CNA at a compatible revision. `handoff.md` records one rebuild of `blupi.glb` that was byte-identical ("build a byte-identical `blupi.glb`"), not a standing guarantee. Intermediate files go to `build-probe/` and are not committed. |
| `avatars3d/objects/` (121 model directories, with `icons.txt`, parts, PNG textures, `Frames.clip.bin`) | Original scenes listed in `tools/rd4_to_gltf/objects.txt` (191 lines), sprite sheets | `build_object_library.py` -> `export_objects.py`, then CNA's converter; extras: `lower_spheres.py`, `top_texture.py`, `calibrate_tone.py`, `sign_decals.py`, `dynamite_sparks.py`, `electro_sparks.py` | repo tools yes; CNA converter outside | 589 PNG textures, 1 457 `.bin` across `avatars3d`, JSON, text | **partly**, as above; commands in `avatars3d/objects/README.md`; `handoff.md` §"adding a model" gives the per-model procedure. |
| `avatars3d/terrain/*.gtr` | Original `Decor/*.rd4|rds` terrain scenes, `Content/icons/object-m.png`, `tools/terrain_relief/tiles.txt` ("reviewed by eye") | `tools/terrain_relief/build_relief.py` (uses Blender via `blender_decimate.py`), `build_object_library.py` | yes, plus Blender | 145 `.gtr` | **partly**: needs the originals, Blender, and the tile-to-scene choices recorded by eye in `tiles.txt`. |
| `avatars3d/blupi_placeholder/` | Khronos glTF Sample Assets "Fox" | CNA's `convert_avatar.py --embedded-clips`, with a synthetic index buffer added | no | 9 files | **no** from this repo alone; the source is third-party and named in the README. |
| `Content/icons/text.png` and `Content/icons2x/text.png`, rows 8-23 (glyphs 128-324; the original's rows 0-7 unchanged) | The sheet's own glyphs (the letters and marks of the original's accented letters) for the Latin letters with marks; the Cyrillic and Greek letters without a Latin look-alike, and æ ø, rendered from **Comic Relief Regular** (SIL OFL 1.1, Google Fonts `ofl/comicrelief`, sha256 `9ef4958a...c7bc22`; kept in `~/deps/fonts`, not in the repository; `COMIC_RELIEF` names another path) | `tools/make_text_glyphs.py` (Pillow, numpy) | yes | 16 rows of 32 px cells (64 px in the 2x sheet) | **partly**: the composed Latin glyphs from the repository alone; the Cyrillic ones need the font file (2026-11-12, BD-0126). |
| `Content/icons/flags.png`, `Content/icons2x/flags.png` | The flags' official geometry (proportions, colours), simplified: Slovakia's arms, Spain's civil flag, Brazil without its stars; Turkey's crescent and star from the flag law's proportions | `tools/make_flags.py` (Pillow) | yes | 20 cells of 48 x 32 (96 x 64) | yes (2026-11-12, BD-0126). |
| `textures3d/grass_top.png` | none (procedural) | "a small Python/Pillow script ... deterministically seeded", mentioned in the commit message of `1d0d62a4` | **no, the script was not committed** | 64 x 64 PNG | **no** (the PNG is the artefact). |
| `src/GalaxyEggbert/Game/BoltPaths.hpp` | `object-m.png` frames 305-307 | `tools/bolt/bolt_paths.py` ("python3 tools/bolt/bolt_paths.py") | yes | C++ header | yes (needs numpy/Pillow, the sprite sheet) |
| `src/GalaxyEggbert/Game/GlueStrands.hpp` | `element.png` frames 168-171 | `tools/glue/glue_strands.py` | yes | C++ header | yes |
| `src/GalaxyEggbert/Game/LavaSurfaceData.hpp` | Original Amapi `.a3d` sources in `speedyblupi-data` | `tools/a3d/a3d.py` + `lava_surface.py` | yes | C++ header "Do not edit: rerun the tool" | **partly**: needs the originals. Format notes in `plans/audit/a3d_format.md`. |
| `src/GalaxyEggbert/Game/DecorQuartTable.hpp` and the other transcribed tables | Mobile Eggbert's C++ | "extracted mechanically (sed+grep, not hand-retyped) and cross-checked via two independent extraction passes" (the extraction commands are not committed) | no | C++ | Cross-checks are described in comments; `VerifyObjIconAgainstReferenceDoc` and `VerifyDefinitionRegistries` test some tables against the repo's own reference documents, not against the original source. |
| `mobile-eggbert-reference/images/` | Original sprite sheets in `../mobile-eggbert/Content/icons` | `mobile-eggbert-reference/tools/extract-blupi-action.py`, `make-gif.sh` | yes | 542 PNG + 129 GIF | yes (needs the sprite sheets and ImageMagick (`make-gif.sh` calls `convert`)) |
| `plans/audit/*.csv`, `plans/audit/*.md` (generated ones), `plans/inventory/*.md` | The tree; the original asset inventory | `tools/audit/*.py`, `tools/asset_inventory.py`, `tools/world_inventory.py`, `tools/model_parity.py` | yes | ledgers | **partly**: `asset_sources.csv` is a survey of `speedyblupi-data`, which is outside the repository (each row carries a SHA-1 prefix and byte size). The `*.notes.csv` files hold hand-maintained status that the generators merge. |
| `tests/golden/*` | The game itself | `tools/verify_golden_frames.sh`, `verify_golden_trace.sh` | yes | 3 PNG, 1 trace | yes, with a display (Xvfb) and the build; re-baselined 2026-11-06 for BD-0077 (`handoff.md`). |
| `plans/evidence/*` | The running game, a video (not in the repo) | `tools/qa_live.sh` and manual capture | partly | 89 PNG, 32 JPG | Screenshots; `video-timeline.csv` derives from a gameplay video outside the repository. |
| `web/` | prose | none (static HTML) | | 53 files | Not generated, as far as the records say. |
| `android/app/src/main/assets/{Content,worlds3d,textures3d,avatars3d}` | The directories of the same name | Symlinks (mode 120000) | | 4 links | Not copies; the APK packs the targets. |

## What the processing changes in the originals

- Sprites are never altered in `Content/`; the tools crop, trace, project and compare them. Derived
  images are separate files (`mobile-eggbert-reference/images/`, `plans/evidence/`).
- Models: geometry is regenerated from the scene's masters, not exported by Ray Dream. Tone
  corrections change colours towards the sprites' (colour error "37.6 -> 14.0 on the toned objects").
  Textures embedded in scenes are decoded and re-encoded as PNG.
- Music: a MIDI file becomes an OGG through a synthesiser and loudness normalisation. The audible
  result depends on the soundfont.
- Terrain reliefs: each relief is a decimated front half of an original scene textured by projecting
  the original sprite.

## Records that already track asset provenance in the repository

- `plans/audit/asset_sources.csv` (703 rows) and `asset_sources.notes.csv`: every file of the original
  3D sources, its format, byte size, SHA-1 prefix, whether Galaxy draws it, and a status.
- `plans/audit/model_sources.md`: every original scene against the sprites rendered from it; 165 "in
  game", 152 "in game otherwise", 11 "terrain tile, scene unconfirmed", 200 "no game sprite".
- `plans/audit/render_representation.csv` (565 rows): how each tile and object is drawn.
- `plans/inventory/assets.md`: one row per converted model with its source scene and silhouette IoU
  against its sprites.
- `plans/audit/release_audit.md` section 5: ledger totals.

None of them records a licence per file or per scene.
