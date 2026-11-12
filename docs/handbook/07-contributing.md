# 7. Contributing: conventions, ledgers and release status

## Branches and workflow

- `develop` is the working branch (`master` is the main branch). CI (`.github/workflows/cna-ci.yml`)
  runs on pushes and pull requests to either.
- The project's habit: one discrete task per commit, with the checks that apply
  ([04-testing.md](04-testing.md), "Which checks for which change") run first. Pushing to `origin` is
  the maintainer's decision, not part of finishing a task.
- Keep unrelated working-tree changes out of your commit: stage by path.
- Keep changes focused. A fix for a reproduced defect comes with a regression test where one is
  possible; do not restructure a working system "while you are there".
- Before changing gameplay, read what the original does (`../mobile-eggbert`, mostly `Decor.cpp`) and
  check the ledgers below: the same difference may already have been analysed.
- A change that alters the verified state, a supported build, a known limitation or the open work
  should be reflected in `CURRENT.md`; `plan.md` and `handoff.md` are historical logs and need no
  edits from you.

## Code conventions as they exist

There is no `.clang-format` or `.editorconfig`; follow the surrounding file.

**C++**

- C++23, `#pragma once`, four-space indentation, braces on their own line in `Game/`, `Editor/` and
  the host. `[[nodiscard]]`, `constexpr` and `enum class` are used freely.
- Namespaces: `GalaxyEggbert` (record codecs, registries), `GalaxyEggbert::Worlds` (voxel container;
  lower-camel member functions such as `getBlock`, `saveToFile`), `GalaxyEggbert::Game`,
  `GalaxyEggbert::Editor`, `GalaxyEggbert::Def` (the original's enums) and `GalaxyEggbert::CNA` (the
  host only). Types and member functions elsewhere are PascalCase, members end in `_`, constants
  start with `k` (`kMoveSpeed`).
- **No legacy `GE` prefix on game-owned or editor-owned names.** Some historic names keep it (the
  verifier executables `VerifyGE...`, the CMake variable `GE_SHARED_SOURCES`, `GEEditorVerifySupport`);
  do not add new ones and do not rename those without a reason.
- Public engine-independent headers are included as `<GalaxyEggbert/...>` from `include/`; sibling
  headers in `src/GalaxyEggbert/Game/` as `"..."`. Keep engine-dependent code (CNA includes) out of
  `include/GalaxyEggbert/` and `Worlds/`.
- Put per-type facts in the registries, not in `switch` statements spread over files
  ([02-architecture.md](02-architecture.md)).
- Comments in this code base are long and carry dates and ledger ids. When you change behaviour that
  a comment documents, fix the comment; cite the reference file and line (for example
  `Decor.cpp:7131`) for anything copied from the original's behaviour; and record a deliberate
  deviation as such.
- No wall-clock time or unseeded randomness in the simulation (determinism, see the architecture
  chapter).

**Python**

- Python 3 with `from __future__ import annotations`, a module docstring that says what the tool does,
  how to run it (a `python3 tools/...` line) and which ledger or brief it serves; scripts are run from
  the repo root and resolve paths from `__file__`. `argparse` for options. Tools in `tools/worlds/`
  start with the `sys.path.insert(...)  # noqa: E402` pattern to import `worldkit` and the kits.
- Generated files carry no hand edits; the generator is the source.
- Third-party Python dependencies are limited to `numpy` and `Pillow` (and Blender or ffmpeg as
  external programs).

**Shell**: `bash`, `set -u` (or `-euo pipefail`), a header comment with usage that the script prints
when called without arguments, paths from `BASH_SOURCE`, scratch output under `build-probe/`.

## Texts and languages

Every text the player sees is in English, French, German, Czech, Slovak, Spanish, Italian, Polish,
Brazilian Portuguese, Dutch, Hungarian, Romanian, Swedish, Russian, Ukrainian, Finnish, Danish, Norwegian
(Bokmål), Turkish and Greek (owner Q-023, BD-0126;
`src/GalaxyEggbert/Game/Localization.hpp`). The English is the key:

- `Tr("Continue")` shows a text where it is written;
- `TrMark("Export")` marks a literal stored in a table or passed on, which is shown later through
  `Tr(...)`;
- `TrFormat("Position {0} is named {1}.", {position, name})` is a sentence with values in it, translated
  whole so a language can put the values where its word order wants them. Never build a shown
  sentence with `+`;
- `TrCount("{0} place", "{0} places", n, {count})` is a number with its noun: the first when `n` is 1,
  else the second, whose Czech, Slovak, Polish and Romanian hold two forms joined by `|` (the "few"
  form, then the "many": Czech and Slovak 2-4; Polish 2-4, 22-24, ... but not 12-14; Romanian 0 and
  2-19 against "de" from 20), and Russian and Ukrainian three (few, many, then the form of 21, 31, ...).

A new text needs its translation in every `src/GalaxyEggbert/Game/Translations<Code>.cpp` (in its
source's group). `VerifyLocalization` (CTest) reads the sources'
`Tr`/`TrMark`/`TrFormat`/`TrCount` literals and fails on one without a translation, on a translation no
code asks for, on placeholders or end spaces that differ, and on a character text.png cannot draw (the
original's lower-case à â é è ë ê ï î ô ù û ä ö ü ç, and Galaxy's glyphs 128-324 from
`tools/make_text_glyphs.py`: the Latin letters with marks of the twenty languages and their capitals,
¿ ¡ °, æ ø, the Cyrillic and Greek letters that look like no Latin one (rendered from the font Comic
Relief, kept outside the repository; the look-alikes are mapped to Latin glyphs in
`TextMetrics::GlyphOf`); no ß,
write "ss"; write "oe" for the ligature). A letter more is a line in that script's table, run again, and its width in
`TextMetrics::kWidth`. A language more: its enum value at the end of `Language` and `kLanguages`, its
code and name in `Localization.cpp`, a table, a column in the training hints, a flag in
`tools/make_flags.py`. The training hints keep their texts beside the English in `TrainingHints.cpp`'s
table.

Text is UTF-8: a layout walks it with `TextMetrics::NextGlyph` and shortens it with
`TextMetrics::PopBack`, never byte by byte. A label on a button or in a column of fixed width is
scaled down to its room when a translation is wider (see the help screen and the menu's buttons).
The cheat list, the debug panels, log lines and data written to files (a demo's title) stay English.

## Commit messages

The history uses conventional subjects, `type(scope): subject`, in English, usually under 95
characters, with a body for anything non-obvious.

- Types in use (count over the history): `docs` (most), `feat`, `fix`, `test`, `tools`, `refactor`,
  `build`, `perf`, `ci`, `chore`, `wip` (a series in progress).
- Common scopes: `worlds`, `render`, `gameplay`, `editor`, `anim`, `models`, `terrain`, `camera`,
  `audio`, `blupi`, `qa`, `tools`, `cheats`, `resume`, `golden`, `audit`, `handoff`.
- **Put the ledger id in the subject** when the change closes or advances a ledger row:
  `fix(cheats): BD-0081 match reference ghost behavior`,
  `feat(worlds): LV-0078 world 199 rebuilt as a journey of twelve places (A to L)`,
  `perf(terrain): a crumbling clay bridge patches the model cells ... (OR-0074)`.
- The body says what the original does, what Galaxy did before, what was changed, and how it was
  verified (for example "ctest 112/112, golden frames and trace unchanged, playthroughs 012 and 045
  pass"). Re-baselined golden files are committed on their own with the reason.
- Look at `git log --oneline -30` before writing yours.

## Ledgers: `plans/audit/`

The fidelity audit (`plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`) compares Galaxy with the original row by
row. Its tables are the project's memory of *what was found and why*. Read them before re-deriving a
difference; add to them when you find one.

| Ledger | Id | What it is for |
|---|---|---|
| `owner_reports.md` | `OR-NNNN` | defects and requests as the owner reported them (translated from Czech), each with analysis, fix and evidence. Newest first. Renumbered once on 2026-11-04 (see its header) |
| `behavior_differences.md` | `BD-NNNN` | one numbered divergence from the original each: cause, reference evidence (file and line), reproduction, fix, validation. Kinds: gameplay, visual, editor, doc |
| `mechanics_3d_decisions.md` | `M3D-NNNN` | intentional 3D adaptations where a 2D rule has no unique reading |
| `editor_parity.md` | `ED-NNNN` | the original editors' behaviour against Galaxy's editor |
| `world_lessons.csv` (+ `.notes.csv`) | `LV-NNNN` | one row per world: the original's lesson, the 3D reading, bypasses, status. Generated by `tools/audit/world_lessons.py`; hand-written parts live in the notes file |
| `lesson_inventory.md` | | per world, original object roles and mechanic tiles Galaxy leaves out or adds (generated) |
| `gameplay_use_cases.csv` | `GP-NNNN` | every action, object type, sound channel and `Decor::` function of the original with its Galaxy implementation and evidence (538 rows; generated, plus notes) |
| `render_representation.csv` | `VIS-NNNN` | how every block icon and object model is drawn (565 rows) |
| `asset_sources.csv`, `model_sources.md`, `a3d_format.md`, `blupi_animations.md` | `AS-NNNN` | what became of each original 3D source |
| `test_coverage.csv` | | which test covers which case |
| `target_reach.md` | | evidence that every treasure, egg, key and exit is reachable (generated) |
| `ui_use_cases.md` | | the original's screens and buttons against Galaxy's |
| `gallery_map.md` | | where everything stands in world 998 (generated by `gallery.py`) |
| `manual_review.md` | `MR-NNNN` | decisions and items only the owner can settle, with the answers |
| `owner_questions.md` | `Q-NNN` | open and answered questions for the owner |
| `release_audit.md` | | the release audit at pinned revisions |

Status vocabulary: `UNREVIEWED`, `CONFIRMED_MATCH`, `INTENTIONAL_3D_ADAPTATION`, `REPRODUCED_DEFECT`,
`FIXED_AND_VERIFIED`, `BLOCKED`, `NOT_APPLICABLE`. Generated rows start `UNREVIEWED`; only evidence moves
them. Generators have a `--check` mode that reports staleness; run the generator after a change that
affects its table (`tools/audit/*.py`, `tools/world_inventory.py`, `tools/asset_inventory.py`,
`tools/model_parity.py`).

Other ids you will meet: `D1`-`D60` (the owner-reported defects of 2026-10-25 and after, in
`plans/GALAXY_EGGBERT_COMPLETION.md` section 5), `UB-NNN` (below), and the task ids of `plan.md`
(`E3D-MIG-`, `BB3D-`, `INFRA-`, `TEST-`, `MENU-`, `EDITOR-`, ...), which appear in old code comments.
Some ledger rows contain Czech text from the owner's reports. `plans/evidence/` holds the screenshots
and CSVs the rows cite.

## Bugs in sibling repositories: `upstream_bugs.md`

CNA, cna-engine, easy-gl, sharp-runtime and the data repositories are not edited from here. A defect
found in one goes at the end of `upstream_bugs.md` as the next `UB-NNN`, in English:

- Repository, file and line, the date found, and how it was found.
- The problem, a minimal reproduction, and the suggested fix.
- The workaround used here, if any.
- Status: when fixed upstream, keep the entry, add the fixing commit, and remove the workaround only
  after re-verifying.

Seven entries exist (UB-001 to UB-007), all marked fixed in CNA at the time of writing. A bug in
cna-engine that blocks this game is fixed in cna-engine with a test and a changelog entry rather than
worked around here (the owner's rule); that requires the task to be scoped into that repository.
CI pins sibling revisions; after verifying a build against newer siblings run
`python3 tools/ci_pin_siblings.py`.

## Release status (2026-11-07)

Sources: `CURRENT.md`, `plans/audit/release_audit.md`, `handoff.md`, and what was re-run for this
handbook (ctest).

| Target | State |
|---|---|
| Linux desktop | the maintained, playable target. 112 of 112 ctest pass (re-run 2026-11-07). The last recorded golden-frame and trace check passed (2026-11-06), and the last full playthrough pass recorded 78 of 78 (2026-11-07, before world 199's latest rework) |
| Web (WebGL 2) | builds and loads in headless Chrome (2026-11-06); manual, not in CI |
| Android | builds a debug APK and starts; in an emulator it loads and draws the 3D models (CNA fix UB-007); not played on a device |
| Windows | **not verified**: the current `SDL_GPU` configuration has no fresh build and live 3D run |
| Linux bundle (`package` target) | produces a runtime archive; launching it is a host check, not a gate |

The owner's decision is **no release yet**; `release_audit.md` says a desktop and web release then
waits only on the owner's word. Content: all 78 campaign worlds, the showcase and gallery worlds, the
title scene and the editor exist; every campaign mission has a scripted playthrough.

What is open, as far as the documents say (verify against them before relying on it):

- World 199 is being reworked after the owner's review (a richer palace, awake follow stars, a
  helicopter, a longer main route); the generator, brief and both playthroughs change with it.
- The editor's lift look (BD-0082's note) and the editor's variant behaviour follow-ups.
- Not tried on real hardware: gamepad, touch controls, accelerometer steering (the sensitivity
  slider is not drawn yet).
- A human play pass of the whole game has not been done; the owner accepted the automated
  playthroughs plus the reachability audit instead (`MR-0007`). The whole-campaign run
  (`campaign.txt`) is last recorded on 2026-10-27 in `CURRENT.md`.
- Android and Windows verification as above; the `.max` sources are closed as not convertible.
- Known visual soft spots are listed in `CURRENT.md` and `plans/audit/render_representation.csv`
  notes; the owner's visual judgement settles them.
- Intermittent window-focus or input loss has been seen under Xvfb; it has not been traced to a
  defect in this code.

## Inconsistencies found in the existing documents (as of 2026-11-07)

These are worth knowing and worth fixing when you touch the documents involved.

- `README.md` and `ANDROID.md` tell you to run `git submodule update --init --recursive`; the repository
  has no submodules.
- Job counts: `README.md`, CI and some script messages say `-j2`; the limit is six
  (the example command in `CLAUDE.md` uses four). Build directory names in `README.md` (`build-package`,
  `build-windows`) are outside the closed list of the build-hygiene rules.
- CMake's default sibling location (`../cna`, `../cna-engine`) is not where the maintainer's copies are;
  they are passed with `-DCNA_HOME`/`-DCNA_ENGINE_HOME`.
- `CURRENT.md`'s verification table still shows 93 tests and a quarantined `easy-gl` test, and says CI
  excludes a failing test; today there are 112 tests, all run, and CI runs them all. Its open-work list
  mentions waves the audit has since closed.
- `handoff.md` lists the zoom-cheat question (Q-016) as waiting; it was answered and built on
  2026-11-07 (`OR-0079`).
- `plans/briefs/README.md` describes a teleporter arrival one cell +Z of the partner; since BD-0017 he
  lands under the partner.
- Generated tables can lag: on 2026-11-07 `python3 tools/asset_inventory.py --check` reports
  `plans/inventory/assets.md` stale, and `tools/audit/world_lessons.py --check` reports
  `world_lessons.csv` stale (probably from the world 199 work in progress).
- The header comment of `Autoplay.hpp` omits the `vehicle` condition field the parser accepts.
- `SaveData.hpp`'s and `GalaxyEggbertGame.hpp`'s leading comments describe earlier states (a single
  world, no gamer slots, an invisible placeholder Blupi).
