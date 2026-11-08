# 3. Build and run

Facts here come from `CMakeLists.txt`, `android/app/build.gradle`, `.github/workflows/cna-ci.yml`,
the scripts in `tools/` and the cache of the existing tree `build-cna-opengl3/`. Nothing in this
file was verified by running a fresh configure or build in this session; the commands for the
existing tree are the ones its `CMakeCache.txt` was produced with, and `ctest` on that tree was run
(112 tests, all passed, 2.4 s). Marked "not verified" where that applies.

## Prerequisites

**Toolchain**: CMake 3.21 or newer, a C++23 compiler (GCC or Clang; the project sets
`CMAKE_CXX_STANDARD 23`), Ninja (the existing tree uses it), and ccache.

**Sibling repositories.** The game is built against CNA and cna-engine *as source*, through
`add_subdirectory`. They are plain checkouts, not git submodules (this repository has no
`.gitmodules`; the instruction `git submodule update --init --recursive` in `README.md` and
`ANDROID.md` does nothing here).

| Repository | Needed for |
|---|---|
| `cna` (`libcna/cna`) | the engine; configure fails without it. Builds its vendored SDL3 and SDL3_mixer from `third_party/` on the first configure (several minutes) |
| `cna-engine` (`libcna/cna-engine`) | the strict helper modules; configure fails without it |
| `easy-gl`, `meta-gl`, `sharp-runtime` | build dependencies of CNA, expected next to it (`../easy-gl`, `../meta-gl`, `../sharp-runtime` from CNA's point of view) |
| `mobile-eggbert` | `VerifyMoveObjectTypesCna` and `VerifyBigDecorParsingCna` read `../mobile-eggbert/worlds/*.txt`; some Python tools (`world_inventory.py`, the audit generators) read it too. Read-only. CI checks out only its `worlds/` directory |
| `speedyblupi-data` | only the asset converters in `tools/rd4_to_gltf/` etc.; not needed to build or play |

CMake looks for CNA and cna-engine at `../cna` and `../cna-engine` relative to this repository, or
wherever `-DCNA_HOME=<path>` and `-DCNA_ENGINE_HOME=<path>` point. CI uses the flat layout (all
five checked out side by side next to `galaxy-eggbert`). On the maintainer's machine they live in a
different directory, `/rv/data/development/github.com/libcna/`, so the existing tree was configured
with explicit `-D` paths (its cache has `CNA_HOME` and `CNA_ENGINE_HOME` set there). The revisions
CI uses are pinned in `.github/workflows/cna-ci.yml`; `python3 tools/ci_pin_siblings.py` bumps
them to the local sibling HEADs and `--check` reports drift.

**System libraries** (from the CI job; Debian/Ubuntu names): `build-essential ninja-build
pkg-config ccache libasound2-dev libpulse-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev
libxfixes-dev libxi-dev libxss-dev libxtst-dev libxkbcommon-dev libdrm-dev libgbm-dev
libgl1-mesa-dev libgles2-mesa-dev libegl1-mesa-dev libdbus-1-dev libudev-dev libwayland-dev
wayland-protocols libdecor-0-dev libavcodec-dev libavformat-dev libavutil-dev libswresample-dev
libcurl4-openssl-dev nlohmann-json3-dev` and a few more audio/IME headers; the workflow file has the
full list.

**GoogleTest** is fetched by CMake (`FetchContent`, v1.14.0 from GitHub) when `BUILD_TESTING` is on
and no `gtest_main` target exists. To avoid a download, pass
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<checkout>`.

**Optional tools**: Python 3 with `numpy` and `Pillow` (asset tools, play-pass sheets);
`xvfb-run`, `xdotool` and ImageMagick's `import` (live QA scripts); `ffmpeg` (music, video
evidence); Blender (one terrain-relief step); Emscripten SDK (web); Android Studio, SDK API 35, NDK
29.0.14206865, JDK 17 or 21 (Android).

## Build-hygiene rules

These come from `/rv/data/development/github.com/openeggbert/CLAUDE.md` (shared by every project
in that directory) and this repository's `CLAUDE.md`. They exist because many builds share one SSD
and rebuilding is the main source of wear. They are written for the maintainer's workstation; the
principles still apply on yours, and a change that "needs" breaking one needs a reason.

1. **One ccache.** Export before building:
   ```bash
   export CCACHE_DIR=/rv/cnaccache
   export CCACHE_BASEDIR=/rv
   ```
   `/rv/cnaccache` is a symlink to `~/.cache/ccache`, the one 50 GB cache. `CCACHE_BASEDIR` makes
   paths relative so identical translation units hit across different build trees. Never point
   `CCACHE_DIR` at a new directory, never set `CCACHE_MAXSIZE` (it overrides the shared cache's own
   config), never export `CCACHE_HASHDIR` (the cache config already sets `hash_dir = false`). Pass
   the launchers to CMake: `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache`.
   If ccache is missing, say so instead of silently building without it. (CMake's own
   `CNA_USE_CCACHE` option defaults to OFF in this project; the launchers above are what actually
   enable it.)
2. **Reuse build directories; the list of names is closed**: `build/`, `build-asan/`,
   `build-ubsan/`, `build-tsan/`, `build-probe/` (throwaway probes and experiments, every task
   shares it), `build-consumer/`. Never invent another one, never suffix a name with a ticket,
   date or branch. Separate a task's work by file-name prefix inside the shared directory. This
   repository's established default tree is `build-cna-opengl3/`, with `build-cna-webgl2/` for the
   web; reconfigure only when it is broken or the configuration genuinely changed. Before creating
   a tree for a sample or fixture, look for an existing one.
3. **Never build or clone in `/tmp`** (or any session scratch directory): it is thrown away and
   rebuilt from nothing every time.
4. **Do not re-clone third-party dependencies.** Keep one checkout under `~/deps/<name>` and point
   CMake at it (`FETCHCONTENT_SOURCE_DIR_<NAME>` or `-D<NAME>_ROOT=...`); prefer
   `FETCHCONTENT_UPDATES_DISCONNECTED=ON`.
5. **At most 6 parallel jobs** here (the owner's setting of 2026-11-01; four is the habit):
   `cmake --build <dir> --parallel 6`. Never a bare `--parallel` or `-j$(nproc)`. The machine has
   30 GB of RAM and no swap; a heavy translation unit that starts swapping is rebuilt with `-j2` or
   `-j1` for that target only.
6. **Sanitizer builds are expensive** (400-900 MB each): build only the variant the task needs.
7. **Clean up your probe output, never delete somebody else's.** Before removing a directory you did
   not create, check that no process holds it open.

Notes on the rules versus the repository's own files:

- `README.md` still shows `-j2`, a `build-package` directory and a `rm -rf build-windows`; the first
  is outdated (the limit is 6), the others are not in the closed list. `CLAUDE.md` says the same
  about `-j2`.
- `.gitignore` lists the project's own tree names (`/build/`, `/build-cna-opengl3/`,
  `/build-cna-webgl2/`, `/build-probe/`, `build-windows/`) but not `build-asan` or the other names of
  the closed list; add an ignore entry if you create one.
- CI configures `-B build-cna` and builds with `-j2`; that is CI's business.

## Linux desktop (the verified configuration)

The existing tree `build-cna-opengl3/` was configured as (reconstructed from its cache; do not
re-run it unless the tree is broken):

```bash
cmake -S . -B build-cna-opengl3 -G Ninja -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCMAKE_BUILD_TYPE=Release -DCNA_GRAPHICS_RENDERER=OPENGL33 \
  -DCNA_HOME=<path to cna> -DCNA_ENGINE_HOME=<path to cna-engine> \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache
```

`GALAXY_EGGBERT_BUILD_CNA` defaults to ON, so it can be omitted; `CNA_HOME` and `CNA_ENGINE_HOME` can
be omitted when the siblings are at `../cna` and `../cna-engine`. Without `CNA_GRAPHICS_RENDERER`
the native default is `OPENGLES3` (`WEBGL2` under Emscripten); `OPENGL33` is what the maintainer
runs. CNA's renderer identity list (`../cna/cmake/RendererSelection.cmake`) is the authority for
other values.

Build and test (normal work, in the existing tree):

```bash
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build build-cna-opengl3 --target GalaxyEggbertCNA --parallel 6
cmake --build build-cna-opengl3 --parallel 6          # also the Verify* test executables
ctest --test-dir build-cna-opengl3 -j4 --output-on-failure
```

`ctest` needs the whole tree built because the `Verify*` executables and
`GalaxyEggbertWorldsTests` are separate targets from `GalaxyEggbertCNA`. Expected result today:
`100% tests passed, 0 tests failed out of 113` (verified 2026-11-08). The last five are the
`easy-gl-*` tests contributed by the dependency; a failure there is a real signal.

Other useful targets: `VerifyShippedWorlds`, `DumpBlockDefinitions`, `GenerateSampleWorld3D`
([04-testing.md](04-testing.md)), `GalaxyEggbertDocs` (Doxygen, only when Doxygen is installed).

### Run

The game reads every asset by a path relative to its working directory, so **run it from the build
directory**. A post-build step copies `Content/`, `worlds3d/`, `textures3d/` and `avatars3d/` next to
the executable (changed files only; `cmake/CopyDirectoryIfDifferent.cmake`).

```bash
cd build-cna-opengl3
./GalaxyEggbert                       # the target is GalaxyEggbertCNA, the file is GalaxyEggbert
./GalaxyEggbert --mission 12          # straight into mission 12 (skips the menus)
./GalaxyEggbert --mission 999 --spawn 34,1,6,0
```

The build directory also receives `savedata.txt` (settings, per-gamer progress), `resume.txt` (an
interrupted mission) and `customworlds/` (the editor's worlds): this is the player's own data when
you play from there. Scripted runs (`tools/autoplay.sh`, `tools/qa_live.sh`) deliberately use their
own run directories under `build-probe/` with symlinks to the repository's asset directories, so they
never touch it. `ctest` leaves it alone too (`VerifyGESaveData` uses its own
`build-cna-opengl3/verify-savedata/`).

Press **F1** for controls help (black text on a translucent white panel). It freezes the current
screen; F1 or Esc closes it. **WASD or the arrows** move Blupi: W/Up walks forward, A/D or
Left/Right turns, S/Down turns around and then walks in the new direction. **X or Right Ctrl**
backs up while keeping his facing. Left Ctrl jumps on foot and raises the helicopter; Left Shift lowers it.
**F fires glue in a tank or helicopter; Left Ctrl also fires in the tank.** Right Shift can also raise the helicopter. Space acts/uses/enters/leaves, Left Shift crouches or dives, Right Shift looks up. Crouch + S/Down pulls a crate.
Esc pauses/continues, F3 switches first/third person (third is the default), F11 toggles full screen,
F5/F6 selects normal/fast speed, Tab toggles slow motion, F12 opens the cheat menu. Left-click menu
buttons; hold the left mouse button and drag to look around. Walking returns the camera behind him.
The main menu's **Exit** button closes the game. **Help** opens the same panel as F1; [Close]
dismisses it by mouse/touch without activating the menu underneath. Its upper-left **Fullscreen / Window** button
switches display mode and always names the other mode. The first run is windowed; button and F11
changes persist globally as `fullscreen=0/1` in `savedata.txt`, restored at the next startup.

Type **compass** to toggle the lower-right objective aid (off by default). It shows the nearest
active chest's straight-line **3D distance in blocks**, rounded to the nearest integer. After all
chests are collected, or if the level has none, it points to the nearest red exit arrow or golden
key. The second row is `<` or `>` for the shortest turn left/right relative to Blupi's facing.
The third is `^` or `v` when the target is above/below him. There are no X/Y/Z labels; correct
facing shows `|` (within3 degrees), and the same height shows `-` (within0.1 block). A target exactly behind
chooses right, since either half-turn is valid.
It is a direction/distance through space, not a route finder. Touch layouts keep the whole panel
clear of the controls. Recognized cheat prefixes consume their following letter keys until
release, so the S ending `weelkeys` cannot turn Blupi around; a fresh S still works.
Non-touch play captures ordered text events, including short key presses between frames, so
rapid cheat typing works in complex editor worlds too. No Enter is needed.
Type **help** or **cheats** during play, without Enter, for every supported cheat word and its
explanation. This overlay pauses the game and closes with F1 or Esc. `cheats` is the Mobile Eggbert
MODERN name (`InputPad.cpp:1318`); `help` is Galaxy's requested alias. `killegg` is listed with its
actual behavior: no effect, as in the reference.

Type **fairplay** to turn off megablupi, ghost, opendoors, showsecret, compass, quick and zoom,
then block every other cheat word, F12 and the hidden cheat menu. Type **fairplay** again to
permit cheats; their old effects do not switch back on automatically. Lives, bombs, ammunition,
keys, vehicles, powers and earlier world changes are retained. F1 help always works. If Ghost
ends inside a wall, Blupi returns to his last valid position. This lock is retained by mission
resume; a build compiled with cheats OFF cannot enable cheats through fairplay.

Water renders the exterior of connected water cells rather than six overlapping faces per cell.
This keeps the animated surface while making underwater swimming readable in both camera views.
A submerged camera also gets a blue haze and reduced warm colours, fading at the waterline;
the HUD and menus retain their normal colours. A camera above the surface gets no extra tint.

The first connected gamepad in XNA slots 1-4 controls play (`GamepadInput.hpp`):

| Gamepad control (Xbox names) | Action |
|---|---|
| Left stick / D-pad | Walk and turn; Down turns around and then walks |
| A | Jump / helicopter up; Play or Continue in menus, return from Win/Lost |
| B or X | Action / use / enter / leave |
| Right trigger (RT) | Tank glue shot / helicopter glue drop |
| RB | Back up |
| LB | Crouch / dive; LB + Down pulls a crate |
| Y | Look up |
| Start or Back | Pause / continue; Back/Escape from the main menu exits |

Play's on-screen controls appear only when CNA detects touch-screen capability, including on
devices with both a keyboard and a touch screen. Desktop mouse clicks in their former areas go
to camera look. The desktop and an SDL virtual gamepad were checked live on 2026-11-08; physical
gamepad/touch-screen verification remains outstanding. Interrupted-mission logs use format v6 to
record touch capability, compass, fairplay and ordered cheat letters; an older/incompatible log falls back to the saved checkpoint.

Tank shots follow its exact heading at any angle; helicopter glue drops vertically, only in flight and off a lift.
Both need ammunition (the green ammo pack; `birdlime` supplies ten when cheats are enabled).
Holding Fire repeats at the original cooldown. Touch devices show a separate **Fire** button
while driving either armed vehicle; entering/leaving remains Action. The tank-only physical Ctrl alias is
mapped before gamepad keys are merged. Helicopter Ctrl, gamepad A and touch Jump all climb.

### 3D editor controls

Ctrl raises the camera, Z lowers it; Space invokes Place. Shift + NumPad 4/6, 7/9, 8/2 extends an
inclusive 3D selection in X/Y/Z from the initial aimed cell. Place fills it with the selected block,
Remove clears it, Escape cancels it; the whole edit is one Undo. Dimensions appear while selecting.

Touch: the Move pad flies horizontally; Up/Down changes height; drag the world with another finger
to look. Choose palette entries and Place with their existing buttons. **Tools > Select 3D** replaces
holding Shift; the six X-/X+/Y-/Y+/Z-/Z+ buttons extend the selection, including 3×2×8 cuboids.
Lifts have yellow numbered open cubes: 1 is the path start, 2 its end. Both faces of each wall
remain visible through transparent parts, including the same marker tiles placed in terrain.
Platforms move along their authored paths in the editor, including their waits and return journey;
the numbered endpoints stay fixed and preview movement is not saved into the world.
Tools also exposes Save, Undo/Redo, Cancel, variant/turn, object/end-point/field editing, copy/paste,
box fill, background-related palette access, music, help and Stop. Unsaved-change protection remains.

Gamepad editing: left stick flies, right stick looks, RT rises, LT descends. A places, X removes,
B cancels; D-pad offsets X/Z and LB/RB offsets Y. Y switches between flying and a visible UI cursor;
in cursor mode the sticks/D-pad move it and A clicks palette/Tools controls. Start saves and Back
stops (with the existing unsaved-change guard). Menus/browser use the cursor automatically, so an
entire editing session can start and finish without a keyboard. Virtual SDL gamepad and direct-touch
events were checked live, including a touch-only 3×2×8 selection/fill; real hardware is still untested.

### Building with cheats disabled

Cheats default to enabled. To compile them out of the player's input paths in the existing tree:

```bash
cmake -S . -B build-cna-opengl3 -DGALAXY_EGGBERT_ENABLE_CHEATS=OFF
cmake --build build-cna-opengl3 --target GalaxyEggbertCNA --parallel 4
```

The macro has the same name, with numeric value 0/1. OFF disables typed words (including help and
compass), F12, the hidden gesture/menu and cheat application. Set the option back to **ON** and
rebuild to restore them. Both configurations were tested; the delivered local build is **ON**.

Headless runs: the game needs a GL context. Under `xvfb-run` set `SDL_AUDIODRIVER=dummy` to keep it
silent; the QA scripts already do.

### Linux runtime bundle

CMake's `package` target produces `GalaxyEggbert-linux-x86_64.tar.gz` (executable, assets, SDL
libraries, a launcher that sets the working directory; the host still supplies graphics, FFmpeg and
the C++ runtime). `README.md` builds it in a separate `build-package` Release tree. Following the
closed-list rule you would use an existing tree instead, for example
`cmake --build build-cna-opengl3 --target package --parallel 6` (not verified; the tree is Release
and `GALAXY_EGGBERT_ENABLE_LINUX_BUNDLE` is ON in its cache). Details: `packaging/README-linux-bundle.md`.

## Windows

Notes in `WINDOWS.md`; commands in `README.md`. Status: **not verified**. The earlier packaging
run worked with a renderer that CNA has since retired; the current configuration uses `SDL_GPU`
and "has not been rebuilt and run live in 3D", so Windows is not claimed as supported.

- Native (MinGW, e.g. CLion's toolchain): configure with `-DGALAXY_EGGBERT_BUILD_CNA=ON` and build
  the `GalaxyEggbertCNA` target.
- Cross-build from Linux with MinGW-w64: configure with
  `-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake -DCNA_GRAPHICS_RENDERER=SDL_GPU
  -DBUILD_TESTING=OFF` in a clean tree (never reuse a Linux tree for it; `README.md` suggests the
  directory name `build-windows`). The README also records the ZLIB overrides the earlier run needed.
- On MinGW the target links the GCC and C++ runtime statically and copies `SDL3.dll`,
  `SDL3_image.dll`, `SDL3_mixer.dll` and `libwinpthread-1.dll` next to the executable
  (`CMakeLists.txt`, `if(MINGW)`).

## Web (Emscripten)

Status: builds and loads in headless Chrome (verified 2026-11-06 per `CURRENT.md`); manual, not in CI.

```bash
source <emsdk>/emsdk_env.sh
emcmake cmake -S . -B build-cna-webgl2 -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_HOME=<cna> -DCNA_ENGINE_HOME=<cna-engine> \
  -DCNA_GRAPHICS_RENDERER=WEBGL2 -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build-cna-webgl2 --target GalaxyEggbertCNA --parallel 6
emrun build-cna-webgl2/GalaxyEggbert.html          # or serve the directory over HTTP
```

Output: `GalaxyEggbert.{html,js,wasm,data}`. `Content/icons`, `Content/backgrounds`,
`Content/sounds`, `worlds3d`, `textures3d` and `avatars3d` are baked into the `.data` pack
(`--preload-file`); the 27 MB of music is **not** packed: it is staged beside the page in
`Content/music/` and fetched when a world first plays a track. `file:` URLs are not a valid runtime
check. Saves and custom worlds go to `/save` (IndexedDB, via `-lidbfs.js`). The browser may require a
user gesture before audio starts. Tests are disabled for this configuration.

## Android

Full instructions in `ANDROID.md`. Status: builds an x86_64 debug APK and starts; in the emulator it
draws the 3D models since CNA's UB-007 fix; not played on a device (owner's decision, `Q-009`).

```bash
cd android
./gradlew assembleDebug                                   # all ABIs
./gradlew assembleDebug -Pandroid.injected.build.abi=x86_64   # emulator ABI only, faster
```

The native build is the root `CMakeLists.txt`, target `GalaxyEggbertCNA`, built as `libmain.so`
(SDLActivity loads it). `android/app/build.gradle` uses ccache when it is on `PATH` and pools Ninja's
compile jobs to six. CNA and cna-engine are found beside this repository, or named in
`android/local.properties` (`cna.home`, `cna.engine.home`; see `local.properties.template`) or the
`CNA_HOME` environment variable. The assets (`Content/`, `worlds3d/`, `avatars3d/`, `textures3d/`)
are symlinked into the APK's assets and copied into internal storage on first start
(`src/GalaxyEggbertCNA/AndroidAssets.cpp`). Release signing uses `key.properties` and a keystore,
both git-ignored: never commit them. Install and log commands (`adb install`, `adb logcat -s SDL
SpeedyBlupi`) are in `ANDROID.md`.

## CI

`.github/workflows/cna-ci.yml` checks out the five siblings at pinned revisions plus the `worlds/`
directory of `mobile-eggbert`, installs the system packages, configures with the ccache launchers,
builds with `-j2` and runs `ctest --output-on-failure`. Golden frames, the trace, autoplay and the
web and Android builds are not in CI. `pages.yml` deploys `web/` (the project's website) and ignores
everything else.
