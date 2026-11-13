# Versioning and releasing Galaxy Eggbert

*Current release: 0.4.1 (2026-11-12), tag `v0.4.1`, published as a GitHub release of
openeggbert/galaxy-eggbert with its Windows, Linux, Android and web packages (0.2.0, 0.3.0 and 0.4.0
the same day, 0.1.0-0.1.3 on 2026-11-11).*

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

## Releasing from GitHub Actions

After updating the version, Android's versionCode, changelog and release notes, commit the
reviewed release and push its annotated `vX.Y.Z` tag. `.github/workflows/release.yml` checks the
metadata, builds Linux, Windows, web and Android in parallel, checks each package, and publishes
the GitHub release with the four packages and `SHA256SUMS.txt`. A version containing `-` is marked
as a pre-release. Publishing and the sites job have `contents: write`; the sites job also has
`actions: write` to dispatch the project site's Pages workflow.

Use the workflow's **Run workflow** button on any branch for a dry run, or:

```bash
gh workflow run release.yml --repo openeggbert/galaxy-eggbert --ref <branch>
```

A manual run never publishes, even if run against a tag. Packages are kept as workflow artifacts
for 14 days. Without signing secrets, a dry run skips Android with a notice; a tag run fails
instead of publishing an incomplete release.

The owner must add these four repository Actions secrets, using the **existing release key**
(a newly generated key cannot update installed copies):

| Secret | Local source |
|---|---|
| `ANDROID_KEYSTORE_BASE64` | base64 of the keystore named by `storeFile` in `android/key.properties` |
| `ANDROID_KEYSTORE_PASSWORD` | `storePassword` in `android/key.properties` |
| `ANDROID_KEY_ALIAS` | `keyAlias` in `android/key.properties` |
| `ANDROID_KEY_PASSWORD` | `keyPassword` in `android/key.properties` |

Resolve a relative `storeFile` from `android/app/`, as Gradle does. From the owner's machine, set
the keystore from its absolute path; the other three commands prompt for the corresponding
values from `android/key.properties`, without putting passwords in shell history:

```bash
base64 -w0 /absolute/path/to/the/existing-release.keystore |
    gh secret set ANDROID_KEYSTORE_BASE64 --repo openeggbert/galaxy-eggbert
gh secret set ANDROID_KEYSTORE_PASSWORD --repo openeggbert/galaxy-eggbert
gh secret set ANDROID_KEY_ALIAS --repo openeggbert/galaxy-eggbert
gh secret set ANDROID_KEY_PASSWORD --repo openeggbert/galaxy-eggbert
```

CI decodes the keystore in the runner's temporary directory and writes an absolute `storeFile`
into `android/key.properties`; both files are deleted by an `always()` cleanup step. No signing
material belongs in tracked files.

The jobs check:

- **Metadata:** tag = `v` + VERSION.txt, a dated changelog heading, release notes naming the
  version, and Android's versionCode greater than the preceding `v*` tag. Dry runs check the
  version, changelog and notes without comparing tags.
- **Linux:** Release/OPENGL33 on Debian 13 (the FFmpeg 7 ABI), every target built and every CTest
  run, then the unpacked archive started under Xvfb/Mesa from a read-only directory as an
  unprivileged user. It must still run after 40 seconds and log Greek selected from `el_GR.UTF-8`.
- **Windows:** Debian 13's MinGW cross-build, hash-verified Windows libcurl, then the executable's
  and every bundled DLL's imports checked against the bundle and an explicit system-DLL list.
  This job does not run wine.
- **Web:** emsdk 6.0.9, Release/WEBGL2 with the local build's WasmFS and single-threaded options;
  tests, Draco and optional Devices/CNAExt modules disabled, vendored SDL bootstrapped on the
  runner. The zip must contain nonempty HTML, JS, WASM, data and `Content/music`.
- **Android:** Temurin JDK 21, NDK 29.0.14206865, CMake 4.1.2, signed arm64 APK. `apksigner` must
  verify the release certificate SHA-256
  `4cb06914c5c08f8a207e9cc4fe39ae8f754daf104d9af60fdc48ec51d22a8baf`, and `aapt` must report
  VERSION.txt as versionName.

All packages use the checked commit's time for `SOURCE_DATE_EPOCH` and `TZ=Europe/Prague`, so
their menu stamps agree. ccache, CNA's platform SDL prebuilts (keyed on both SDL submodule
commits), emsdk, the Windows curl archive and Gradle are cached. The siblings' pins live once in
`.github/actions/checkout-siblings/action.yml`, shared with the ordinary CNA CI; continue updating
them with `tools/ci_pin_siblings.py` after local verification.

Re-run failed jobs from the Actions run after fixing a transient failure or adding missing
secrets. Once all package jobs pass, publishing calculates their checksums. If the release
already exists (for example a previous upload stopped partway through), publishing uses
`gh release upload --clobber` to replace the assets. A tag's source and metadata must already be
correct; source fixes require a reviewed commit and a new version tag.

The manual steps below remain the fallback. Local release scripts honor `CCACHE_DIR` and
`CCACHE_BASEDIR`, defaulting to `/rv/cnaccache` and `/rv`, and use `GALAXY_BUILD_JOBS` (default 4).
The websites are updated automatically after a final release, as described below.

## The websites after a release

After `publish` succeeds for a final version tag, the `sites` job updates both websites from
that release's four workflow artifacts. Pre-releases and branch dry runs skip it. It checks out
`develop` with all tags for the project site (`web/`), and `main` of
`openeggbert/galaxyeggbert.com` for the presentation site and online game. The Python 3 standard
library script `tools/release/update_sites.py` never commits; CI commits only changed site files
as `github-actions[bot]`, pushes the two branches, and explicitly dispatches `pages.yml` on
`develop` because a `GITHUB_TOKEN` push does not start another workflow. A rejected project-site
push is rebased once and retried. The presentation site's push starts its legacy Pages build.
No change means no commit.

The owner must add **`GALAXYEGGBERT_COM_DEPLOY_KEY`** to this game repository's Actions secrets:
the private half of a dedicated SSH deploy key whose public half is installed with **Allow write
access** in `openeggbert/galaxyeggbert.com` → Settings → Deploy keys. The checkout action uses it
only for that repository; never commit the key. Both repositories must allow the bot/key to push
their respective branches. This setup and the first live CI run remain an owner action.

Current-release HTML fields have explicit `data-release` spans (version, ISO date, long English
date, hero headline and network compatibility), `data-release-href` URL templates with
`{version}`, and `data-release-size` package names. Description meta elements use
`data-release-content` templates; the VideoGame JSON-LD uses the exact `softwareVersion` and
`downloadUrl` keys. Keep these markers when editing the pages. Historical sections and rows
have separate version identifiers and insertion comments; they are never rewritten. The Releases
search entry records `releaseCurrent` and `releaseVersions` to add keywords only once.
Package sizes are rounded up to MiB (displayed as MB, matching the existing site). Protocol
compatibility is read from `kProtocolVersion` at stable `vX.Y.Z` tags, excluding later releases;
for an untagged local rehearsal only, the new protocol comes from the worktree.

The dated changelog entry's first paragraph becomes the summary. Its `### Added`, `### Changed`,
`### Fixed` (or other category) bullets become the new "New in X.Y.Z" section; the overview gets a
history row, and search gets the version and summary words. An optional comment inside the entry
sets the presentation site's news line:

```markdown
<!-- site-headline: twenty languages and a faster editor -->
```

This produces "Version X.Y.Z is out: twenty languages and a faster editor". Without the comment,
only the news line's version changes; its existing headline stays. Review the headline's link by
hand if a new topic needs a different destination. Historical marketing prose, platform status,
requirements, screenshots and language counts stay editorial work.

Manual fallback (no builds or commits are performed by this command):

```bash
python3 tools/release/update_sites.py --version 0.4.1 --date 2026-11-12 \
  --packages build-probe/release/0.4.1 --site ../galaxyeggbert.com --web-dir web --dry-run
# Review the diff, then repeat without --dry-run to write it.
```

Use the four real packages for that version. The web zip is validated, unpacked in a temporary
folder under the package directory, and copied by the presentation site's `tools/update-web-build.sh`.
`--dry-run` prints diffs (content-change notices for game files), writing nothing. A second run is
a no-op. For a rehearsal, copy both sites and packages under `build-probe/sites-test/` and pass
`--changelog build-probe/sites-test/CHANGELOG.md` with a copied, pretend release entry. Once reviewed,
commit and push each site's changes manually if CI is unavailable; dispatch `pages.yml` on `develop`
if its push used `GITHUB_TOKEN`. Rerunning the release's sites job is safe after a transient failure.

## Cutting a release (only when the owner asks)

1. **The version.** Set `VERSION.txt` (and raise Android's `versionCode` when an APK will be
   published).
2. **The changelog.** Move what is under `## [Unreleased]` in `CHANGELOG.md` into
   `## [x.y.z] — YYYY-MM-DD`, add its link at the bottom, and update
   `packaging/RELEASE_NOTES.md`.
3. **For the manual fallback, build and test** in the existing Release trees (`../CLAUDE.md`: reuse
   them, at most six jobs), then make the packages into one `<out-dir>` (e.g. `build-probe/release/<version>`), the
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
6. **Push and publish only when the owner asks** (`git push origin HEAD`, `git push origin v0.1.0`).
   The tag push starts the Actions release above. For a manual fallback, make a GitHub release of
   the tag with the packages, SHA256SUMS.txt and the notes (Q-018):

   ```bash
   gh release create v0.1.0 --repo openeggbert/galaxy-eggbert --title "Galaxy Eggbert 0.1.0" \
       --notes-file packaging/RELEASE_NOTES.md --verify-tag <out-dir>/GalaxyEggbert-0.1.0-* <out-dir>/SHA256SUMS.txt
   ```

   Android's `versionCode`: 1 for 0.1.0, 2 for 0.1.1, 3 for 0.1.2, 4 for 0.1.3, 5 for 0.2.0, 6 for
   0.3.0, 7 for 0.4.0, 8 for 0.4.1; the next published APK needs 9.
7. **Open the next cycle** with an empty `## [Unreleased]` section in `CHANGELOG.md`.
