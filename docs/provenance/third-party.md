# Third-party code and data

What this repository depends on, vendors or embeds, and the licence each carries. Every licence is
tagged with where the statement comes from:

- **[recorded here]**: a file or comment in this repository says it.
- **[sibling LICENSE]**: read from the `LICENSE` / `NOTICE` / `THIRD_PARTY_NOTICES.md` of the sibling
  repository under `../../libcna/` (read only).
- **[general knowledge]**: the licence of a well-known project, from the auditor's general knowledge,
  not from any file in the workspace. The owner should verify these before relying on them.

This is not legal advice and does not decide whether the combinations are permitted.

## 1. Summary

| Item | Role | How it enters | Licence | Source of the statement |
|---|---|---|---|---|
| **CNA** (`libcna/cna`) | XNA 4.0 reimplementation the game is written against | `add_subdirectory(../cna)` from a sibling checkout; CI pin `b3e855ca...` | Microsoft Public License (Ms-PL); derived in part from FNA (Ms-PL, (c) 2009-2021 Ethan Lee and the MonoGame Team) | [sibling LICENSE] `cna/LICENSE`, `cna/NOTICE.md` |
| **cna-engine** (`libcna/cna-engine`) | Strict helper modules (`CnaEngine::Core`, `::Graphics`): camera, mesh builder, GPU meshes | `add_subdirectory(../cna-engine)`; CI pin `6737a1cf...` | MIT, (c) 2026 Robert Vokac and the cna-engine contributors | [sibling LICENSE] |
| **easy-gl** | OpenGL family behind CNA's renderers | Transitive, through CNA | MIT, (c) 2026 Robert Vokac | [sibling LICENSE] |
| **sharp-runtime** | .NET-style runtime library under CNA | Transitive, `add_subdirectory(../sharp-runtime)` by CNA | MIT, (c) Robert Vokac and contributors | [sibling LICENSE] |
| **meta-gl** | GL loader under easy-gl | Transitive | MIT, (c) 2026 Robert Vokac; vendors Khronos GL headers (Khronos MIT text in `third_party/Khronos/LICENSE-MIT.txt`) | [sibling LICENSE] |
| **SDL3, SDL3_mixer** (and SDL3_image named in `CMakeLists.txt`) | Windowing, input, audio | Vendored by CNA as git submodules (`cna/third_party/SDL`, `SDL_mixer`), built from source into `cna/.sdl-prebuilt-*`; `find_package(SDL3)` in this repo's `CMakeLists.txt` | zlib licence, (c) 1997-2026 Sam Lantinga | [sibling LICENSE] `cna/third_party/SDL/LICENSE.txt` |
| **SDL Java glue** (`SDLActivity` and friends) | Android | Not committed here since the initial commit; Gradle takes `cna/third_party/SDL/android-project/.../java` as a source directory | zlib | [recorded here] `android/app/build.gradle`; [sibling LICENSE] |
| **GoogleTest v1.14.0** | Unit tests | `FetchContent_Declare` with a GitHub archive URL (`CMakeLists.txt:952-957`), only when `BUILD_TESTING` and not Android/Emscripten and no GTest target already exists | BSD-3-Clause | [general knowledge]; sharp-runtime's vendored copy begins "Copyright 2008, Google Inc." [sibling file] |
| **Doxygen** | Optional documentation target | `find_package(Doxygen)` | GPL-2.0-or-later (tool only; its output is not shipped) | [general knowledge] |
| **FFmpeg libraries** | Video/audio decode on the host | Not vendored; CI installs `libavcodec-dev`, `libavformat-dev`, `libavutil-dev`, `libswresample-dev`; `README.md` and `packaging/README-linux-bundle.md` say the bundle relies on the host's FFmpeg | LGPL-2.1+ or GPL depending on build | [general knowledge] |
| **libcurl, nlohmann-json** | Required by CNA's networking module; installed by CI | System packages | curl licence; MIT | [general knowledge]; CI comment says CNA requires them |
| Linux system libraries in CI (`libasound2-dev`, `libpulse-dev`, `libx11-dev`, `libwayland-dev` and about 30 more) | SDL's backends | `apt-get` in `cna-ci.yml` | Various | [recorded here] list in `.github/workflows/cna-ci.yml` |
| **Emscripten** | Web build | `cmake/web/shell*.html`, `--preload-file` | MIT / UIUC | [general knowledge] |
| **Gradle 8.12, Android Gradle Plugin 8.7.3, NDK r29, compileSdk 35** | Android build | `android/gradle/wrapper/gradle-wrapper.jar` is committed (a binary); plugin from Google/Maven Central | Apache-2.0 (Gradle, AGP); NDK has its own licences | [general knowledge]; versions [recorded here] |
| **Python packages** `numpy`, `Pillow`; **Blender** `bpy` | Tool scripts only (`numpy` 32 imports, `PIL` 19, `bpy` 1, in `tools/`) | No `requirements.txt` or other manifest exists; versions are not recorded | BSD-3 (numpy); HPND (Pillow); GPL (Blender) | [general knowledge] |
| **ffmpeg (program), fluidsynth (libfluidsynth), xvfb-run, xdotool, ImageMagick (`import`, `convert`), ccache, ninja** | Tool scripts and QA (`render_music.py`, `qa_live.sh`, `verify_golden_frames.sh`) | Host programs, not vendored | Various | [general knowledge] |
| **Comic Relief** (font, Regular) | `tools/make_text_glyphs.py` renders the Cyrillic letters of text.png's glyphs 218-267 from it (2026-11-12) | Not in the repository: `~/deps/fonts/ComicRelief-Regular.ttf` (from `github.com/google/fonts`, `ofl/comicrelief`), or the path in `COMIC_RELIEF` | SIL OFL 1.1, Copyright 2013 The Comic Relief Project Authors; the OFL covers the font, not the pictures drawn with it | [recorded here] |
| **TimGM6mb.sf2** (General MIDI soundfont) | Rendering the music (see section 3) | **Not recorded in the repository.** `render_music.py` defaults to `/usr/share/sounds/sf2/default-GM.sf2`. On the workstation where this audit ran that path resolves (via Debian alternatives) to `TimGM6mb.sf2` from the Debian package `timgm6mb-soundfont`. Whether it was the soundfont actually used for the committed OGG files **cannot be determined from the repository**. | GPL-2 per that package's `copyright` file: "I created some of the samples myself and all others are either public domain or fall under the terms of the GNU General Public License" | observed on the audit machine in `/usr/share/doc/timgm6mb-soundfont/copyright`; not a repository record |

Nothing is vendored into this repository's `src/`, `include/` or `tools/`: the only `#include`s
outside the standard library, CNA, cna-engine and `Microsoft/Xna` headers are SDL3 (one file,
`AndroidAssets.cpp`), GoogleTest (tests), Emscripten, Windows/POSIX/Android system headers. No
`.gitmodules` exists. The repository carries **no `licenses/` directory, no `NOTICE` and no
third-party notices file**.

## 2. What the sibling repositories vendor (and so may be linked into the game)

Which of these ends up in a given Galaxy Eggbert binary depends on CNA's configuration, which this
repository does not record. They are listed from `cna/THIRD_PARTY_NOTICES.md` and the vendor
directories so the owner can see the chain.

| Component | Where | Licence as recorded there |
|---|---|---|
| FNA-derived code | throughout CNA | Ms-PL, (c) Ethan Lee and the MonoGame Team |
| DirectXMesh-derived `DirectXMeshOptimizeFaces.cpp` (content pipeline only) | `cna/modules/content-pipeline` | MIT, (c) Microsoft Corporation; adapted file |
| Draco 1.5.7 (glTF mesh compression) | `cna/third_party/draco` (submodule) | Apache-2.0, (c) The Draco Authors |
| cgltf | `cna/third_party/cgltf` | MIT, (c) 2018-2021 Johannes Kuhlmann |
| ENet | `cna/third_party/enet` | MIT, (c) 2002-2024 Lee Salzman |
| dr_mp3, dr_flac | `cna/third_party/dr_libs` | public domain (Unlicense) or MIT No Attribution |
| stb_image, stb_image_write, stb_vorbis | `cna/third_party/stb` | public domain or MIT, the user's choice |
| FAudio reverb extraction | `cna/third_party/faudio-reverb` | zlib, (c) 2011-2024 Ethan Lee, Luigi Auriemma and the MonoGame Team (altered source) |
| XNA stock-effect HLSL / compiled `.fxb` | CNA D3D9 and FNA3D backends | Ms-PL (copied verbatim from FNA) |
| FNA3D + MojoShader (optional renderer) | fetched, not copied | zlib |
| wgpu-native (optional WebGPU backend) | downloaded | Apache-2.0 / MIT |
| FreeType (optional, content pipeline only) | system package | FreeType License (credit clause) or GPL-2 |
| CNA avatar catalog | `libcna/cna-avatar-assets` submodule | "original CNA content ... generated by CNA's own code", CNA's licence |
| miniz, tinyxml2, nlohmann json, googletest | `sharp-runtime/vendor` | licence files not read for this audit |
| Khronos GL headers | `meta-gl/third_party/Khronos` | MIT (Khronos) |

`cna-engine/NOTICE.md` records the other direction: "A binary distribution that links CNA must carry
CNA's Ms-PL terms for the CNA portions". It also lists material copied into cna-engine from other
projects of the owner (MIT). `packaging/README-linux-bundle.md` and `CMakeLists.txt` show what the
Linux `.tar.gz` bundle actually contains: the executable, `Content/`, `worlds/`, `worlds3d/`,
`textures3d/`, `avatars3d/`, `LICENSE` (GPLv3) and, when found, `libSDL3*.so` with the SDL licence
files in `licenses/SDL/`. **CNA's Ms-PL text, FNA's notice and the other notices above are not in
the bundle** as the install rules are written.

## 3. Third-party and external data in the repository

| Item | Where | What the repository records | Licence |
|---|---|---|---|
| Khronos "Fox" model, converted to CNA's skinned format | `avatars3d/blupi_placeholder/` (9 files) | `README.md`: model by PixelMannen (https://opengameart.org/content/fox-and-shiba); rigging and animation by @tomkranis on Sketchfab; glTF conversion by @AsoboStudio and @scurest; source `KhronosGroup/glTF-Sample-Models` `2.0/Fox`; converted by CNA's `convert_avatar.py` with a synthetic index buffer | Model: CC0 1.0. Rigging and animation: **CC BY 4.0, attribution required** (the README says so). The attribution lives only in that directory's README; the game uses it only as a fallback when `avatars3d/blupi/` is missing. [recorded here] |
| General MIDI soundfont used for the music renders | not in the repository | see section 1 | see section 1 |
| Original game's music (MIDI), sprites, sounds, 3D scenes | see `original-game-material.md` | | |
| `Content/Content.mgcb` (1 137 lines), `Content/icons/*.xml` | `Content/` | First added in the initial commit `ef03c601` (2026-05-12) and `e741cc24`; origin not recorded | not recorded |
| `android` launcher icons, `SpeedyBlupiActivity.java` | `android/` | First added in the initial commit; origin not recorded. Same-named files (`ANDROID.md`, `WINDOWS.md`, `Doxyfile`) exist at the root of `../mobile-eggbert`, which suggests a carry-over, but nothing was compared | not recorded |
| `cmake/web/shell.html`, `shell-cna.html` | web build | Emscripten HTML shells; `shell.html` has comments about "Urho3D" from the retired engine target. Origin (Emscripten's `shell_minimal.html`, the Urho3D project's shell, or hand-written) is not recorded | not recorded |
| `web/` | project website | Own text; the pages are imported from `openeggbert/galaxyeggbert.openeggbert.com`. `web/LICENSE` is MIT, (c) 2026 Robert Vokac. No third-party scripts or fonts are referenced (a grep for CDN, jQuery, Bootstrap and similar found none) | MIT (the file); see `licences.md` |
| `plans/evidence/` | 89 PNG, 32 JPG, 2 CSV | Screenshots of the game, and `video-timeline.csv` derived from a gameplay video of the original game that is **not** in the repository (path `/rv/big/my-blupi-videos-on-youtube/...` in `plans/briefs/README.md`) | The screenshots contain original sprites |

## 4. Snippets copied from elsewhere that the comments reveal

A grep of `src/`, `include/`, `tools/`, `tests/`, `cmake/` and `CMakeLists.txt` for "copied from",
"adapted from", "based on", "taken from", "ported from", "from the original" and URLs found:

- **No copied third-party code.** The hits are about the original game's data and are listed in
  `original-game-material.md`, or are ordinary English ("based on where the press STARTED").
- The only URL in the sources is the GoogleTest archive in `CMakeLists.txt`.
- `REMAKE-ANALYSIS.md:339` says "No copied code/data. P0-2 *validates* transcription; it does not
  license copying tables in." That statement is from 2026-07-20; later explicit approvals are recorded in
  the sources (for example 2026-07-21 for `DecorQuartTable.hpp`) and a standing approval for small data
  tables in CLAUDE.md (2026-10-25). See `original-game-material.md`.
- Source files carry no licence or copyright header (0 of the files in `src/`, `include/`, `tests/`,
  `tools/`, `cmake/`).

## 5. Facts about the tools that handled sources, from the repository's own records

- Ray Dream Studio: `handoff.md` says a VirtualBox VM `xp` with Ray Dream Studio exists and "must
  never get network access". The tool reads the `.rd4`/`.rds` text format directly; the Ray Dream
  program itself is not part of this repository.
- 3D Studio MAX: `plans/audit/manual_review.md` MR-0001 records that the owner offered a `3dsmax3.1.iso`
  and key file, that setup was started and **stopped unaccepted** because the serial was "the generic
  one that circulates with key generators" and the ISO carried a `CRACKS` folder, that nothing was
  installed and the crack "will not be used in any case". The owner closed it on 2026-11-06 (MR-0010):
  the 56 `.max` files are not converted. No model in the repository comes from them.
