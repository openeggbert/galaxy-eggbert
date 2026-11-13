# Brief: automatic releases in GitHub Actions (the owner, 2026-11-13)

Goal: pushing an annotated tag `vX.Y.Z` to openeggbert/galaxy-eggbert builds the four packages in
GitHub Actions (Linux, Windows, web, Android), checks them, and publishes the GitHub release with
them and `SHA256SUMS.txt`, exactly as the manual process in `docs/releasing.md` does today. A
manual run (`workflow_dispatch`) on any branch builds and checks the same packages and keeps them
as workflow artifacts without publishing anything (a dry run).

Read first: `docs/releasing.md`, `tools/release/make_release.sh`, `make_windows_zip.sh`,
`make_web_zip.sh`, `.github/workflows/cna-ci.yml` (the working CI, green), `tools/ci_pin_siblings.py`,
`WINDOWS.md` ("The cross-build from Linux"), `android/app/build.gradle`,
`../../libcna/cna/.github/workflows/emscripten-multi-renderer-ci.yml` (emsdk caching pattern; read
only), `../../libcna/cna/cmake/ThirdPartySDL.cmake` (CNA builds its vendored SDL3 at configure time
into `cna/.sdl-prebuilt-<System>-<arch>[-wayland]` or `.sdl-prebuilt-emscripten`; read only).

## Hard rules

- Do not modify anything outside this repository. Do not commit, tag or push. Do not compile the
  game or run CMake/Gradle builds (the machine is shared; the real test is a run on GitHub). Allowed
  checks: `bash -n`, `python3 -m py_compile`, loading the YAML with PyYAML, and running the new
  metadata check script against the existing tags (`v0.4.1`, `v0.4.0`).
- Local defaults must not change: scripts still use `CCACHE_DIR=/rv/cnaccache` and
  `CCACHE_BASEDIR=/rv` when the environment does not set them, and at most 4 parallel jobs by
  default (`GALAXY_BUILD_JOBS`, default 4, replaces the hard-coded `--parallel 6` in the three
  release scripts).
- English comments in the house style of the existing workflow (explain *why*, briefly).
- Never put secrets in files; the Android key comes only from repository secrets.

## Facts already established (do not re-derive)

- **Linux package must be built on Debian 13** (container `debian:13`): the program links the
  host's FFmpeg 7 (`libavcodec.so.61`, `libavformat.so.61`, `libavutil.so.59`, `libswresample.so.5`),
  plus libcurl, libzstd, libopus; the archive bundles only SDL3/SDL3_mixer. The local releases are
  built on Debian 13 (glibc 2.41, cmake 3.31.6); an Ubuntu 24.04 build would need FFmpeg 6 and not
  run on Debian 13. Debian package names: use `libjack-jackd2-dev` (not the virtual `libjack-dev`);
  the rest of cna-ci.yml's list exists in Debian 13. In a container, install `git` (and
  `ca-certificates`) before `actions/checkout` so submodules work; no `sudo`.
- **Linux configure** (as the local `build-cna-opengl3`): `-G Ninja -DCMAKE_BUILD_TYPE=Release
  -DGALAXY_EGGBERT_BUILD_CNA=ON -DCNA_GRAPHICS_RENDERER=OPENGL33` plus the ccache launchers. Build
  everything, run `ctest` (needs the mobile-eggbert worlds/ sparse checkout, as cna-ci.yml), then
  `tools/release/make_release.sh <build> <out> --no-android`.
- **Linux smoke test**: unpack the archive into a folder made read-only (`chmod -R a-w`), run
  `./run-galaxy-eggbert` under `xvfb-run -a -s "-screen 0 1024x768x24"` with
  `SDL_AUDIODRIVER=dummy`, `GALAXY_USER_DIR=<writable dir>`, `LANG/LC_ALL=el_GR.UTF-8` (install and
  generate that locale, or pick C.UTF-8 and skip the language check), Mesa software GL, `timeout 40`.
  Pass = exit 124 (still running when killed) and the log has the line
  `GalaxyEggbertCNA: language el` (printed in `src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp:471`).
  Print the log tail on failure.
- **Windows**: MinGW cross-build on Linux, also in `debian:13` (local uses Debian's
  `g++-mingw-w64-x86-64` 14.2, win32 thread model, which `update-alternatives` picks by default).
  Configure as WINDOWS.md (`-DCMAKE_TOOLCHAIN_FILE=<cna>/cmake/toolchains/mingw-w64.cmake
  -DCMAKE_BUILD_TYPE=Release -DCNA_GRAPHICS_RENDERER=OPENGL33 -DCURL_INCLUDE_DIR=... -DCURL_LIBRARY=
  .../lib/libcurl.dll.a`). WINDOWS.md passes `SDL3_DIR`/`SDL3_mixer_DIR` pointing at
  `cna/.sdl-prebuilt-Windows-x86_64/install/...`; find out in this repo's `CMakeLists.txt` and CNA's
  ThirdPartySDL.cmake whether a fresh CI checkout can configure without them (CNA then builds the
  prebuilt) or whether they are needed (e.g. for the DLL-copy fallback), and do what works on a
  fresh runner (a first configure that builds the prebuilt, then the explicit dirs, is acceptable).
  libcurl for Windows: `https://curl.se/windows/dl-8.22.0_3/curl-8.22.0_3-win64-mingw.zip`, sha256
  `f38b0ac2e3a280fd414b607c874a21e6dd4d1ed0e2ae622afbf0020917446561`; verify the hash, keep it in
  `actions/cache` (curl.se drops old builds), unpack to `curl-8.22.0_3-win64-mingw/`. Then
  `tools/release/make_windows_zip.sh`. Check: every DLL imported by `GalaxyEggbert.exe` and by the
  bundled DLLs (`x86_64-w64-mingw32-objdump -p`, "DLL Name:") is either in the zip or a Windows
  system DLL (a short explicit allow-list: KERNEL32, USER32, GDI32, ADVAPI32, SHELL32, OLE32,
  OLEAUT32, WS2_32, WINMM, IMM32, VERSION, SETUPAPI, CRYPT32, BCRYPT, msvcrt, ucrt api-ms-win-*,
  OPENGL32, DWMAPI, CFGMGR32, HID, SHLWAPI, IPHLPAPI, WLDAP32, NORMALIZ, SECUR32, DNSAPI, ...: derive
  the real list from what the imports show and keep it tight). No wine run.
- **Web**: `ubuntu-24.04` runner, emsdk **6.0.9** (the local one), cloned and cached as CNA's
  emscripten CI does. Configure `emcmake cmake -G Ninja -DCMAKE_BUILD_TYPE=Release
  -DCNA_GRAPHICS_RENDERER=WEBGL2` (compare with `build-cna-webgl2/CMakeCache.txt` for any other
  non-default option the local web build was configured with, and copy it). Then
  `tools/release/make_web_zip.sh` with `EMSDK_ENV=$HOME/emsdk/emsdk_env.sh`. Check: the zip holds
  GalaxyEggbert.html/.js/.wasm/.data and Content/music.
- **Android**: `ubuntu-24.04`, Temurin JDK 21 (`actions/setup-java`), the preinstalled SDK
  (`$ANDROID_HOME`) plus `sdkmanager --install "ndk;29.0.14206865" "cmake;4.1.2"` (versions from
  `android/app/build.gradle`). Siblings via `CNA_HOME`/`CNA_ENGINE_HOME` env or the default
  `../cna`, `../cna-engine` beside the repo (see `siblingPath` in build.gradle; the CI layout has
  them beside it). Build `./gradlew --no-daemon assembleRelease -PgalaxyAbis=arm64-v8a` (never
  `android.injected.build.abi`). Signing from secrets `ANDROID_KEYSTORE_BASE64`,
  `ANDROID_KEYSTORE_PASSWORD`, `ANDROID_KEY_ALIAS`, `ANDROID_KEY_PASSWORD`: decode the keystore to a
  file outside the repo's tracked paths, write `android/key.properties` with an **absolute**
  `storeFile` (build.gradle resolves it relative to `android/app`), delete both at the end
  (`if: always()`). Without the secrets: a tag run fails with a clear message; a dry run skips the
  APK with a `::notice::`. Put the APK-building part into a new `tools/release/make_android_apk.sh
  <out-dir>` and have `make_release.sh` call it, so the name `GalaxyEggbert-<version>-android-arm64.apk`
  lives in one place. Check with `apksigner verify --print-certs` that the signer's SHA-256 is
  `4cb06914c5c08f8a207e9cc4fe39ae8f754daf104d9af60fdc48ec51d22a8baf` (the owner's release key, public
  fingerprint; a different key would make the APK unable to update installed copies), and with
  `aapt dump badging` that versionName = VERSION.txt. ccache: build.gradle adds the launcher when
  `ccache` is on PATH.
- **Metadata check** (new `tools/release/check_release_metadata.sh <tag>`, first job, fast): the tag
  is `v` + VERSION.txt's first line; CHANGELOG.md has a `## [<version>] — ` heading;
  `packaging/RELEASE_NOTES.md` names the version; Android's `versionCode` is greater than in the
  previous `v*` tag's `android/app/build.gradle` (`git describe --tags --abbrev=0 --match 'v*' <tag>^`;
  skip when there is none). Outputs `version` and `prerelease` (true when the version has a `-`).
  In a dry run (no tag) check VERSION.txt/CHANGELOG/notes consistency only.
- **Publishing** (last job, tag runs only, `permissions: contents: write` on that job alone):
  download the four packages, write `SHA256SUMS.txt` over them (factor the checksum part of
  make_release.sh into `tools/release/make_checksums.sh <out-dir>` and use it from both), then
  `gh release create "$tag" --title "Galaxy Eggbert $version" --notes-file packaging/RELEASE_NOTES.md
  --verify-tag [--prerelease] <packages> SHA256SUMS.txt`. Re-running a failed run must work: when
  the release exists, `gh release upload --clobber` instead.
- **The build's stamp**: check `cmake/BuildStamp.cmake` and `src/GalaxyEggbertCNA/BuildStamp.hpp`;
  if `SOURCE_DATE_EPOCH` is honoured, set it to the tagged commit's time so all packages show the
  same stamp, and set `TZ=Europe/Prague` if the stamp is formatted in local time (the owner's
  releases are made there).
- **Caches** (`actions/cache@v4`): ccache per job (in a container use
  `CCACHE_DIR=${{ github.workspace }}/.ccache`, `CCACHE_BASEDIR=${{ github.workspace }}`), the CNA SDL
  prebuilt per platform keyed on the SDL/SDL_mixer submodule commits (as cna-ci.yml), emsdk, Gradle.
- **Sibling pins in one place**: move the six sibling checkouts of cna-ci.yml into a composite
  action `.github/actions/checkout-siblings/action.yml` (input `mobile-eggbert: 'true'|'false'`),
  used by cna-ci.yml and the release workflow (`uses: ./galaxy-eggbert/.github/actions/checkout-siblings`),
  keeping the same refs, comments and step names; point `tools/ci_pin_siblings.py` at the action
  file (its step-name/regex logic stays). cna-ci.yml must behave exactly as before.
- **Workflow**: `.github/workflows/release.yml`, triggers `push: tags: ['v*']` and
  `workflow_dispatch`; top-level `permissions: contents: read`; `concurrency` per ref, no cancelling
  of tag runs; job timeouts (Linux 60, Windows 60, web 60, Android 90 minutes); each package
  uploaded with `actions/upload-artifact@v4` (retention 14 days); the package jobs run in parallel
  after the metadata check.

## Documentation

- `docs/releasing.md`: a section "Releasing from GitHub Actions" (what a tag push does, the dry
  run, the four secrets and how the owner sets them with `gh secret set` from the local
  keystore/key.properties, what each job checks, re-running a failed run); the manual steps stay as
  the fallback. Mention that the website (web/ and galaxyeggbert.com) is still updated by hand.
- `CHANGELOG.md` under `## [Unreleased]`: one line under a `### Changed` heading saying releases
  are built and published by GitHub Actions from a version tag.

## Report back

The files changed, the decisions you made on the open points (SDL3_DIR on Windows, the stamp, the
web options), anything you could not verify, and the exact secrets the owner must add.
