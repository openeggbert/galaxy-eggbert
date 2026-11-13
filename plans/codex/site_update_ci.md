# Brief: the websites updated by CI after a release (the owner, 2026-11-13)

After the release workflow (`.github/workflows/release.yml`) publishes a version, both websites must
be brought to it automatically, as they are by hand today:

1. **This repo's `web/`** (the project site, deployed to GitHub Pages by `.github/workflows/pages.yml`).
   See what the 0.4.1 hand update changed: `git show ad96bfc1 -- web` (the version and date in
   `web/index.html`, `build.html`, `running.html`, `project-structure.html`,
   `development/known-issues.html`, a new "new in 0.4.1" section in `releases.html`, a history row in
   `overview.html`, the Releases entry of the search index in `web/assets/script.js`). Earlier ones:
   `git log --oneline -- web/releases.html`.
2. **galaxyeggbert.com**, the separate repository checked out at `../galaxyeggbert.com`
   (openeggbert/galaxyeggbert.com, branch `main`, GitHub Pages "legacy" build from `main`). See
   `git -C ../galaxyeggbert.com show 99bd5b2` (0.4.1): `index.html` (og:description,
   `softwareVersion`, `downloadUrl`, the hero's news line and platforms line, the network line "Version
   0.4.1 plays with 0.4.0 and 0.3.0, but not with 0.2.0 or 0.1.x", the download section's version,
   date "12&nbsp;November 2026", the GitHub links with tag and file names, package sizes),
   `play/index.html`, and `play/game/*` (the web build, copied by its `tools/update-web-build.sh`).

## Design (follow it; deviate only with a reason you report)

- **Markers, not guessing.** Mark every place that names the *current* release once, by hand now, so
  the script rewrites exactly those and never a historical mention ("new in 0.4.0", old history rows):
  e.g. `<span data-release="version">0.4.1</span>`, `<span data-release="date">…</span>` (each site's
  own date format), and for links an attribute template such as
  `data-release-href="https://github.com/openeggbert/galaxy-eggbert/releases/download/v{version}/GalaxyEggbert-{version}-web.zip"`
  that the script expands into `href`; sizes `data-release-size="web"` filled from the package's real
  size. Markers must not change how the pages look. In `<pre><code>` a span is fine. JSON-LD and
  `<meta>` content cannot hold spans: handle them by their exact keys/properties.
- **New content from CHANGELOG.md.** The entry `## [X.Y.Z] — YYYY-MM-DD` gives: its first paragraph
  (the summary), and its `### Added/Changed/Fixed/…` bullets. `releases.html` gets a new "New in X.Y.Z"
  section in the style of the existing ones (look at them), `overview.html` a history row (the date and
  "Release X.Y.Z: " + the summary, shortened to its first sentence if long), the search index entry's
  keywords gain the new version and words from the summary.
- **The hero's news line on galaxyeggbert.com**: if the CHANGELOG entry contains a line
  `<!-- site-headline: … -->`, the news line becomes "Version X.Y.Z is out: …"; without one only the
  version number in it changes (what the 0.4.1 hand update did). Document this in docs/releasing.md.
- **The network line** is computed: a version plays with the released versions whose
  `kProtocolVersion` (`src/GalaxyEggbert/Net/NetProtocol.hpp`, read at each `v*` tag with `git show`)
  equals the new one's. Keep the current wording; group older incompatible versions of one minor as
  `0.1.x` when all its releases are incompatible.
- **One script**, `tools/release/update_sites.py` (Python 3, stdlib only):
  `--version`, `--date`, `--packages <dir with the four packages>` (sizes; the web zip, unpacked to a
  temp dir and copied into the site by `../galaxyeggbert.com/tools/update-web-build.sh <dir>`),
  `--site <galaxyeggbert.com checkout>`, `--web-dir <this repo's web/>`, `--dry-run` (print a diff,
  write nothing). Idempotent: run twice, the second run changes nothing. It never commits.
- **CI**: a job `sites` in release.yml after `publish`, tag runs only, skipped for pre-releases:
  checks out this repo's `develop` (not the tag; `fetch-depth: 0`) and galaxyeggbert.com with the
  deploy key secret `GALAXYEGGBERT_COM_DEPLOY_KEY` (`actions/checkout` `ssh-key:`), downloads the
  release's packages (the workflow artifacts), runs the script, then commits as
  `github-actions[bot]` and pushes: this repo to `develop` (`permissions: contents: write`; on a
  rejected push `git pull --rebase` once and retry), galaxyeggbert.com to `main` (the deploy key).
  A push with GITHUB_TOKEN starts no workflow, so afterwards run `gh workflow run pages.yml --ref
  develop` (`permissions: actions: write`). Nothing to commit = no commit. The commit messages as the
  hand ones: "web: release X.Y.Z" and "Galaxy Eggbert X.Y.Z: version and download links, the web
  build, the network versions that play together".
- `docs/releasing.md`: the sites step (what is automatic, the headline comment, the deploy key, the
  manual fallback `tools/release/update_sites.py …`). `CHANGELOG.md` `## [Unreleased]`: one line.

## Verification (no builds)

- Insert the markers, then run the script for **0.4.1** against both sites: it must change nothing but
  what the markers' insertion already changed (the pages render the same text).
- Copy both sites into `build-probe/sites-test/` and run it there for a pretend **0.4.2** dated
  2026-11-14 with a pretend CHANGELOG entry (use a copy of CHANGELOG.md; do not edit the real one for
  this): show the diff; it should look like the 0.4.1 hand diffs. Then run it again: no change.
- YAML loads with PyYAML; `python3 -m py_compile`; `bash -n` on shell.

## Rules

Edit only `web/`, `tools/release/`, `.github/workflows/release.yml`, `docs/releasing.md`,
`CHANGELOG.md` (Unreleased) in this repo, and `../galaxyeggbert.com` (markers; its README's
maintenance section). Another agent edits `src/`, `tools/Verify*` and the lightning's docs at the same
time: do not touch those. No commits, no pushes, no builds, no `gh` calls that change anything. The
pretend-version test must not leave anything behind outside `build-probe/sites-test/`. Report: files
changed, the markers used, the two test results, anything left by hand.
