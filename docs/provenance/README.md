# Provenance and licence inventory

What in this repository was produced with AI, which assets are generated, which parts are third-party,
what derives from the original game, and under which licences. Taken at HEAD `114a4d66`
(2026-11-07, 1723 commits).

**This is an inventory with evidence, not legal advice.** Where a licence question needs the owner's
or a lawyer's decision, the documents state the question and the facts and do not decide it. Where the
repository holds no record, they say "cannot be determined from the repository".

## Files

| File | Content |
|---|---|
| `README.md` | This summary, the key findings, the open questions, how to keep it current |
| `ai-involvement.md` | What git and the repository's own records show about AI use; a table per top-level area; AI-written generators and the data they produce |
| `third-party.md` | Every dependency and vendored or embedded piece, with licence and where the statement comes from; snippets copied from elsewhere |
| `original-game-material.md` | What derives from Speedy Blupi / Mobile Eggbert: sprites, sounds, music, level files, transcribed tables, converted models; the permission status the repository documents and what it does not |
| `generated-assets.md` | Per asset directory: source, tool, output, whether the tool is in the repository and whether the output is reproducible |
| `licences.md` | The licence files and headers present, the licences of the dependencies, the open questions with facts |
| `inventory.sh` | Read-only script: file and line counts per area, history facts, dependency list, original-game citations, licence files. Prints to stdout only |
| `inventory-2026-11-07.txt` | The output of `inventory.sh` for this audit |

## Method and limits

- Sources: `git ls-files`, `wc`, `git log`, `git grep` on this repository; `CLAUDE.md`, `AGENTS.md`,
  `handoff.md`, `plan.md`, `plans/` (ledgers in `plans/audit`, briefs in `plans/codex`), the READMEs of
  the generated asset directories, source-file comments, `CMakeLists.txt`, `.github/workflows`,
  `android/`. From the siblings under `../../libcna/` and `../mobile-eggbert` only the `LICENSE`,
  `NOTICE`, `THIRD_PARTY_NOTICES.md` and README files. `../speedyblupi-data` and the sibling source
  code were **not** read. Licences of well-known tools (GoogleTest, numpy, FFmpeg ...) are marked
  "general knowledge" in `third-party.md`: verify them before relying on them.
- Nothing was built or run; no generator was executed (they write into the tree). The only file system
  writes are the files of `docs/provenance/`.
- No percentage of AI-written code is given, because the repository does not support one. Sizes are
  counts of tracked files and text lines.

## Key findings

### AI involvement

- The history cannot answer "who wrote this commit": 1723 commits, one author and committer identity,
  zero `Co-Authored-By` trailers, and the owner's design is to keep AI attribution out of the history
  (`CLAUDE.md`/`AGENTS.md`; 13 commits of 2026-10-24 that carried attribution "were rewritten").
- The repository's own records do show AI tool use: an agent instruction file from 2026-06-11
  (`CLAUDE.md`; `AGENTS.md` from 2026-10-17); a branch named `claude/galaxy-eggbert-3d-remake-n0ga3y`
  merged on 2026-07-20; `handoff.md`, written "for the AI agent that continues this work in a fresh
  context", saying Claude Code worked until 2026-11-06, then the Codex CLI, then Claude Code again;
  `plans/SESSION_SUMMARY_2026-10.md`, "one long AI-assisted working session ... plus background
  sub-agents it directed" (361 commits); seven briefs for the Codex CLI in `plans/codex/`; ledger lines
  "Implemented with the Codex CLI to a brief; reviewed here" (BD-0077, BD-0078, BD-0079, OR-0073,
  OR-0074, and the queue items BD-0081, OR-0076, OR-0077, BD-0082, BD-0083, LV-0078).
- By area: the C++ game, editor, tests and tools, the 78 world generator scripts, 83 playthrough
  scripts, the converters and the audit ledgers are AI-written under the owner's direction (recorded
  for 2026-10-23 onwards; inferred for 2026-06-11 to 2026-10-22; not determinable for the first 8
  commits, 2026-05-12 to 2026-06-10, and for the 2026-06-11 commits made before `CLAUDE.md` was added). The 81 worlds in `worlds3d/` are data produced by those generators
  (the repository's "hand-authored" means designed, not converted from the 2D maps). The models in
  `avatars3d/` are tool output from original scenes. `Content/` and `worlds/` are copies from Mobile
  Eggbert, not AI-produced. Only 19 text files (3 412 lines) of the current code tree predate the first
  agent instruction file.
- The owner is the decision-maker in every record: scope, direction, acceptance, and the numbered
  owner questions and reports (`plans/audit/owner_questions.md`, `owner_reports.md`).

### Third-party code and licences

- Build dependencies are sibling checkouts, none vendored here: CNA (**Ms-PL**, derived from FNA),
  cna-engine, easy-gl, sharp-runtime, meta-gl (**MIT** each), through them SDL3 and SDL3_mixer
  (**zlib**), plus GoogleTest v1.14.0 fetched for tests. No `.gitmodules`, no `licenses/` directory, no
  notices file in this repository; CI pins the sibling commits.
- No third-party source code is copied into `src/`, `include/` or `tools/`. One third-party asset is in
  the tree: the Khronos Fox model in `avatars3d/blupi_placeholder/` (CC0 model; **CC BY 4.0
  rig/animation, attribution required**, recorded in the directory's README).
- Tools used: Python with numpy and Pillow (no requirements file), Blender, ffmpeg, fluidsynth, Xvfb,
  xdotool, ImageMagick, Gradle 8.12 / AGP 8.7.3 / NDK r29, Emscripten.

### Original-game material and its documented status

- Copies of Mobile Eggbert files: 9 sprite sheets, 38 background and tile images, 93 WAV sounds
  (`Content/`, 22 MB) and 5 level files (`worlds/`); 671 cropped images in `mobile-eggbert-reference/`.
- Music: the original Speedy Eggbert 1 (1998) MIDI tracks rendered to 10 OGG (27 MB) by
  `tools/music/render_music.py` with a soundfont the repository does not name.
- Models: 3D models in `avatars3d/` (70 MB) converted by `tools/rd4_to_gltf` from the original Ray Dream
  scenes in `../speedyblupi-data`, which the repository labels "GPLv3, (c) Daniel Roux & EPSITEC SA".
- Data: twelve enum headers and at least ten transcribed tables in the C++, and 662 comment lines
  citing the original's source lines for re-implemented behaviour.
- Documented status: this repository is GPLv3; Mobile Eggbert's `LICENSE` is MIT, (c) 2013 Daniel Roux
  and (c) 2024-2026 Robert Vokac; speedyblupi-data is labelled GPLv3 as above; the owner's approvals to
  the working agents for copying tables and assets are recorded (CLAUDE.md and source comments).
  **Not documented**: any permission or correspondence from Daniel Roux or EPSITEC SA; the status of the
  sprites, sounds and music as such; the source and licence of the music's MIDI files and soundfont; any
  attribution file for a distribution.

### Licence of this repository

- `LICENSE` is GPLv3 (since the first commit); `web/LICENSE` is MIT for the website. No source file has a
  licence header; "only" versus "or later" is not stated; the README has no licence section; the Linux
  bundle ships `LICENSE` and the SDL licences but not CNA's Ms-PL notices.

## Open questions for the owner

Detail and facts for each are in `licences.md` section 3. None is answered in these documents.

1. What grant do the copied original sprites, backgrounds, sounds and level files stand on, and does
   Mobile Eggbert's MIT notice (or the GPLv3 label on speedyblupi-data) cover them?
2. What is the status of the original's MIDI music and of the soundfont used, and may the 27 MB of
   rendered OGG be redistributed (it is in the Linux bundle)?
3. What is the status of the models converted from the original Ray Dream scenes (70 MB), given only
   the repository's own quotation of the speedyblupi-data GPLv3 label?
4. Does Mobile Eggbert's licence cover the tables, enums and behaviour transcribed from its C++, given
   how Mobile Eggbert itself derives from the original?
5. In whose name and on what basis is GPLv3 granted for the AI-authored code, generators and generated
   data, which derive in part from third-party inputs?
6. Can a binary that links CNA (Ms-PL) be distributed under GPLv3, and which notices must the bundle
   carry?
7. "GPLv3 only" or "or later", and should files or the README name the copyright holder?
8. The Fox placeholder: keep, replace, or attribute (CC BY 4.0 for rig and animation)?
9. Is `web/LICENSE` (MIT) inside the GPLv3 repository intended, and who wrote the website text?
10. Where do `Content/Content.mgcb`, the Android icons and Activity, and the web shells come from?
11. Do the committed screenshots and golden frames with original sprites need the same treatment as
    question 1?
12. If `TimGM6mb.sf2` (Debian: GPL-2) was the soundfont, is that compatible with GPLv3 for the
    rendered music?

## How to keep this inventory current

```bash
bash docs/provenance/inventory.sh                    # everything, to stdout
bash docs/provenance/inventory.sh areas              # files, lines, bytes per area
bash docs/provenance/inventory.sh history            # commits, authors, trailers, AI-named commits, merges
bash docs/provenance/inventory.sh firstadded         # code tree by first-add period
bash docs/provenance/inventory.sh records            # files that mention Claude or Codex; the Codex briefs
bash docs/provenance/inventory.sh deps               # CMake, CI pins, Gradle, Python modules, host programs, symlinks
bash docs/provenance/inventory.sh original           # citations of the original's sources, GPL labels, asset ledger statuses
bash docs/provenance/inventory.sh licences           # licence files, headers, sibling licences
bash docs/provenance/inventory.sh > /tmp/now.txt && diff docs/provenance/inventory-2026-11-07.txt /tmp/now.txt
```

When to update which document:

- A new dependency, `FetchContent`, CI action or Python import: re-run `deps`, update `third-party.md`.
- A new asset set, a new copied or transcribed table, a new converter: update `original-game-material.md`
  and `generated-assets.md`; run `original` and `areas`. `plans/audit/asset_sources.csv` and the other
  ledgers are generated by `tools/audit/*.py` and are the better record of per-file status.
- A new work session by an AI tool: the repository records it in `handoff.md` and in the ledgers; run
  `history` and `records` and update `ai-involvement.md`.
- A change of a `LICENSE`, a header policy, or the bundle's notices: update `licences.md`.
- A licence statement that the repository quotes (`avatars3d/*/README.md`, `plans/inventory/assets.md`)
  should be checked at its source (`../speedyblupi-data` README) by the owner; this audit could not.

Counts in these documents are for 2026-11-07 and drift with every commit.
