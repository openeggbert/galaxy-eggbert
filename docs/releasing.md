# Versioning and releasing Galaxy Eggbert

*Current release: 0.4.0 (2026-11-12), tag `v0.4.0`, published as a GitHub release of
openeggbert/galaxy-eggbert with its Windows, Linux, Android and web packages (0.2.0 and 0.3.0 the
same day, 0.1.0-0.1.3 on 2026-11-11).*

Galaxy Eggbert follows [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html), set up
after CNA's own scheme (`../cna/docs/releasing.md`): the version is decided in one place,
everything else derives from it, and a release is a reviewed commit with an annotated tag and a
`CHANGELOG.md` entry.

## Where the version lives

In exactly **one** place, the first line of `VERSION.txt` at the repository's root:

```text
0.1.0          a final version
0.1.0-rc.1     a pre-release (alpha.N, beta.N, rc.N)
```

No leading `v` (only the Git tag carries one). The root `CMakeLists.txt` reads the file before
`project()`, refuses anything that is not a Semantic Versioning number, and splits it as CNA does:
the numeric part goes to `project(GalaxyEggbert VERSION …)`, the pre-release identifier to
`GALAXY_EGGBERT_VERSION_PRERELEASE`, and `GALAXY_EGGBERT_VERSION_STRING` is the whole spelling.
Changing `VERSION.txt` reconfigures the build by itself.

| Consumer | How it gets the version |
|---|---|
| C++ code | `#include <GalaxyEggbert/Version.hpp>` → `GalaxyEggbert::VersionString()`, `GALAXY_EGGBERT_VERSION_MAJOR`, … (generated from `cmake/templates/Version.hpp.in` into `<build>/generated/include/`; never edit the copy) |
| The main menu | "Version 0.1.0" left of Play (`InputPad::DrawInit`) |
| The Linux archive | its file name, `GalaxyEggbert-<version>-linux-x86_64.tar.gz` (CPack); `VERSION.txt` is in it |
| Android | `versionName` (`android/app/build.gradle` reads `VERSION.txt`); `versionCode` is raised there by hand |
| `tools/release/make_release.sh` | the archive's and the APK's names |

Maintained by hand on a version change: `CHANGELOG.md` and `packaging/RELEASE_NOTES.md`.

## The build's date and time

Under the version, the main menu shows when the build was made, `YYYY-MM-DD HH:MM` in the build
machine's local time (`GalaxyEggbert::CNA::BuildStampText()`, `src/GalaxyEggbertCNA/BuildStamp.hpp`).
`cmake/BuildStamp.cmake` writes its source again only when something the game is linked from
changed: one of its sources, a header under `include/` or `src/`, or a library of its link closure
(CNA's modules, sharp-runtime, cna-engine, easy-gl, meta-gl). A build that changes nothing keeps the
stamp and does not relink the game; `SOURCE_DATE_EPOCH`, when set, gives the stamp instead of the
clock.

## Numbers that are *not* the version

- **`VoxelConfig::FormatVersion`** (2): the `.vwr` world format (`World Format.md`).
- **`kDemoSimulationRevision`** (`DemoReplay.hpp`): which simulation a demo recording replays on.
- **Android's `versionCode`** (`android/app/build.gradle`, 1 today): an integer the store and
  the phone compare; it must grow with every published APK, independently of the version's
  spelling.
- **CNA's version** (0.2.0 today): the framework's own, in `CNA/Version.hpp`.

## Pre-1.0 policy

While the major version is 0, a minor version (0.2.0) may change saved games, world files, demos
or controls; a patch version (0.1.1) only fixes. Pre-release identifiers are `alpha.N` → `beta.N`
→ `rc.N`, ordered as Semantic Versioning orders them: `0.1.0-rc.1` precedes `0.1.0`.

## Cutting a release (only when the owner asks)

1. **The version.** Set `VERSION.txt` (and raise Android's `versionCode` when an APK will be
   published).
2. **The changelog.** Move what is under `## [Unreleased]` in `CHANGELOG.md` into
   `## [x.y.z] — YYYY-MM-DD`, add its link at the bottom, and update
   `packaging/RELEASE_NOTES.md`.
3. **Build and test** in the existing Release trees (`../CLAUDE.md`: reuse them, at most six
   jobs), then make the packages into one `<out-dir>` (e.g. `build-probe/release/<version>`), the
   zips first so that the checksums cover them:

   ```bash
   tools/release/make_windows_zip.sh build-probe/windows <out-dir>      # MinGW cross-build, WINDOWS.md
   tools/release/make_web_zip.sh build-cna-webgl2 <out-dir>             # Emscripten build
   tools/release/make_release.sh build-cna-opengl3 <out-dir> [--no-android]   # Linux, APK, notes, SHA256SUMS
   ```

   Check each package unpacked as a player would: the Linux archive from a folder it cannot write
   to, the Windows zip under wine (`SDL_VIDEODRIVER=windows`), the web zip served over HTTP to
   Chrome on X11 under Xvfb (`--ozone-platform=x11`; headless Chrome gets no WebGL 2 here).
4. **Commit** the version-bearing files by name (`VERSION.txt`, `CHANGELOG.md`,
   `packaging/RELEASE_NOTES.md`, `android/app/build.gradle` when `versionCode` changed).
5. **Tag** with a `v` prefix, annotated: `git tag -a v0.1.0 -m "Galaxy Eggbert 0.1.0"`.
6. **Push and publish only when the owner asks** (`git push origin HEAD`, `git push origin v0.1.0`),
   then a GitHub release of the tag with the packages, SHA256SUMS.txt and the notes (Q-018):

   ```bash
   gh release create v0.1.0 --repo openeggbert/galaxy-eggbert --title "Galaxy Eggbert 0.1.0" \
       --notes-file packaging/RELEASE_NOTES.md --verify-tag <out-dir>/GalaxyEggbert-0.1.0-* <out-dir>/SHA256SUMS.txt
   ```

   Android's `versionCode`: 1 for 0.1.0, 2 for 0.1.1, 3 for 0.1.2, 4 for 0.1.3, 5 for 0.2.0, 6 for
   0.3.0, 7 for 0.4.0; the next published APK needs 8.
7. **Open the next cycle** with an empty `## [Unreleased]` section in `CHANGELOG.md`.
