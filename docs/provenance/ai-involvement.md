# AI involvement: what the repository itself can and cannot show

This is an inventory with evidence. It states what the repository's own records say. It does not
estimate a percentage of AI-written code, and where something cannot be established it says
"cannot be determined from the repository".

Method and limits are in `README.md`. Counts below come from `git ls-files`, `wc` and `git grep`
(`inventory.sh` reproduces them). Figures were taken at HEAD `114a4d66`, 2026-11-07.

## 1. What the git history can and cannot say

| Fact | Value | Meaning |
|---|---|---|
| Commits | 1723, from 2026-05-12 to 2026-11-07 | |
| Author and committer identity | one: Robert Vokac, for all 1723 | Authorship per commit cannot be read from git. |
| `Co-Authored-By` trailers | 0 | Not evidence of absence: see next row. |
| "Generated with" lines | 0 | Same. |
| Attribution was removed on purpose | `CLAUDE.md` / `AGENTS.md`: "Attribution is deliberately disabled in the Claude Code settings ... (13 commits of 2026-10-24 got it that way and were rewritten)". Commit `e92cf124` (2026-10-25) adds the rule. | History was rewritten at least once to strip attribution. So the trailer count says nothing about who wrote what. |
| Merge of a named AI branch | `a5d20159` (2026-07-20) merges `origin/claude/galaxy-eggbert-3d-remake-n0ga3y`, 6 commits (`REMAKE-ANALYSIS.md`, `renderers.md`, a header, plans). `NEXT.md` §"docs: merge REMAKE-ANALYSIS.md/renderers.md from a separate branch" calls it "a separate session's exploratory work". | The only branch name in the history that names an AI tool. |
| Commit subject style | `docs` 657, `feat` 518, `fix` 303, `test` 129, `tools` 17, `refactor` 15, `build` 10, `perf` 7, `ci` 4, `chore` 4, `wip` 3, no prefix 50 | From 2026-06-11 the subjects are phase-numbered ("Phase 4 - ..."), then ticket-numbered (`BD-0081`, `OR-0076`, `LV-0078`). This is consistent with agent-driven sessions working from written plans, but a style is not proof. |
| Volume | 7 commits in 2026-05, 137 in 2026-06, 630 in 2026-07, 3 in 2026-08, 2 in 2026-09, 458 in 2026-10, 486 in 2026-11 | Busiest days: 309 commits on 2026-10-26, 165 on 2026-07-04, 133 on 2026-11-03, 127 on 2026-11-04, 123 on 2026-11-02. `SESSION_SUMMARY` notes that its commit dates are system-clock dates and that some documents give 2026-10-27 for the same work. |
| The first 8 commits (2026-05-12 to 2026-06-10; the ninth is `92a29e57`, 2026-06-11, 18:28, and `CLAUDE.md` follows at 20:19 the same day) | Messages such as "Added files", "Added world files", "Upgraded to C++23", "Update project to use Nova3D instead of CNA" | No record in the repository says whether these were written by the owner or with AI help. **Cannot be determined from the repository.** |

## 2. Records in the repository that say AI tools were used

| Record | What it says | Date / commit |
|---|---|---|
| `CLAUDE.md` (root of this repo) | Contributor instructions for a coding agent: direction lock, faithful-remake rules, build rules, "Never add AI attribution to commit messages". The file name is the convention Claude Code reads. First appears with engine rules for the (then) Urho3D target. | first commit of the file `5cbb27b5`, 2026-06-11; rewritten since |
| `AGENTS.md` (root of this repo) | Same content, "kept in sync"; this is the file name the Codex CLI reads. | `44d27bf7`, 2026-10-17 |
| `../CLAUDE.md`, `../AGENTS.md` (workspace, not in this repo) | Build rules "for every agent, every session": around ten agents run on the machine. | read, not tracked here |
| `easy3d.md` §5.1 | "If a Mobile Eggbert change would help, Claude Code must stop and ask the user." | 2026-07-01 |
| `texture-distance-washout-bug.md` | "a separate Claude Code agent is actively working in `../cna`" | July 2026 |
| `NEXT.md` (frozen) | Records a standing instruction that "any investigation of free-eggbert's code must be delegated to a subagent"; records the branch merge above; many "User asked / explicit user approval" entries. | June to October 2026 |
| `handoff.md` | Opens "For the AI agent that continues this work in a fresh context". States that Claude Code's last session ended on 2026-11-06, that the owner then moved work to the Codex CLI, and later told Claude Code to do everything itself. Lists the Codex queue (tasks 1 to 6) and names which ledger items Codex implemented. | first commit of the file `c1f56db9`, 2026-10-24 |
| `plans/SESSION_SUMMARY_2026-10.md` | "A status report of one long AI-assisted working session (several context windows, plus background sub-agents it directed)". 361 commits since restore point `00ba088` (124 feat, 76 fix, 75 test, 70 docs, 12 tools), 1718 files changed, +42 608 / -1 993 lines "(most of the bulk is world data, converted models and playthrough scripts)". Says the campaign worlds were built "partly by this session, partly by background agents it briefed and reviewed". | 2026-10-25 to 2026-10-27 |
| `plans/codex/` (7 files) | Self-contained briefs written "so that the Codex CLI can do it unattended", with the command line (`codex exec -m gpt-6-astra ...`) and the rules each task keeps. Briefs: `ghost.md`, `editor_cursor.md`, `exhibit_saws.md`, `lift_tiles.md`, `bigdecor_clearance.md`, `world199_rebuild.md`. | 2026-11-06 |
| `plans/audit/behavior_differences.md` | Says "Implemented with the Codex CLI to a brief; reviewed here" for BD-0077 (Resume), BD-0078 (Blupi starts standing), BD-0079 (dynamite spark and table), and the re-routing of eleven playthroughs after BD-0080. | 2026-11-06 |
| `plans/audit/owner_reports.md` | "Fix (Codex CLI to a brief, reviewed)" for OR-0073 (mission-sign digits) and OR-0074 (crumbling bridge). | 2026-11-06 |
| `plans/codex/2026-11-12-*.md`, `plans/audit/camera_quality_2026-11-12.md`, `camera_fix_2026-11-12.md` | The owner re-enabled the Codex CLI on 2026-11-12 (models gpt-6.1-sol or gpt-6-astra only). The chase camera's measurement (`tools/camera_probe.py`) and fix, the text sheet's edge bleeding and the music/camera-distance measurement were done by the Codex CLI to these briefs and reviewed, built, tested and committed by Claude Code. | 2026-11-12 |
| `handoff.md` "Codex queue" | Tasks 1 to 6 DONE: BD-0081 ghost cheat, OR-0076 editor aim, OR-0077 exhibit saws, BD-0082 lifts drawn with any tile's look, BD-0083 BigDecor clearance, LV-0078 world 199 rebuild. Q-015 (Resume): "the owner: 'udelej q 015', and from now on use the Codex CLI where possible". | 2026-11-06 |
| `plans/audit/owner_questions.md`, `release_audit.md` | The owner's answers are recorded as decisions (Q-001..Q-014, MR-0001..MR-0010) taken during these sessions. The owner is the decision-maker for direction, scope and acceptance in every record. | October to November 2026 |

What the records do **not** say: which agent wrote any given file before 2026-10-23, which lines of any
file were typed by the owner, or which model versions were used outside `plans/codex/README.md`
(which names `gpt-6-astra` and `gpt-6.1-sol` for Codex tasks and does not name a Claude model).

## 3. Per area: what it is and how it was produced

Confidence scale:

- **Recorded**: a document in the repository says it explicitly.
- **Inferred**: consistent with the records (same sessions, same working mode) but not stated for this
  item.
- **Not determinable**: the repository has no record.

"Lines" are text lines from `wc`; "files" from `git ls-files`.

| Area | What it is | Size | How it was produced, as far as the records say | Confidence and evidence |
|---|---|---|---|---|
| `src/GalaxyEggbert/` | Shared game, editor and world model (namespaces `GalaxyEggbert::Game`, `::Editor`, worlds) | 116 files, 35 498 lines | **AI-written code** under the owner's direction, in sessions driven by `CLAUDE.md` and the plans; some features implemented by the Codex CLI. 2026-10-25 onwards: Recorded (SESSION_SUMMARY, handoff, ledgers). 2026-06-11 to 2026-10-24: Inferred (CLAUDE.md in force, session-numbered tickets, branch `claude/...` merged). Before 2026-06-11: Not determinable. | Per file, not determinable. By first-add date, see section 4. |
| `src/GalaxyEggbertCNA/` | CNA application host (`GalaxyEggbertGame.cpp` 8 406 lines, `AndroidAssets.cpp`) | 5 files, 9 819 lines | As above. `AndroidAssets.cpp` exists for the Android build (`OR-0011`, 2026-11). | Same. |
| `include/` | Engine-independent definitions (`Def/*.hpp`, block registry, world format headers) | 29 files, 2 933 lines | As above. Enum values and constants mirror the original (see `original-game-material.md`). | Same. |
| `tests/` | 9 GoogleTest files, 3 golden-frame PNGs, `golden_trace.txt` | 13 files, 1 433 text lines, 2.7 MB (mostly the PNGs) | AI-written tests; golden frames are renders of the game itself (re-baselined 2026-11-06 for BD-0077, recorded in `handoff.md`). | Inferred for the tests; Recorded for the re-baseline. |
| `tools/*.cpp` | 26 `Verify*` test executables (`VerifyInteractionSystem.cpp` 5 850 lines, `VerifyBlupiMovement.cpp` 4 031) and two generators | 26 files, 18 256 lines | AI-written; they are the CTest suite (112 tests on 2026-11-06). | Recorded (SESSION_SUMMARY "110 automated tests", release_audit.md). Authorship per file: Inferred. |
| `tools/worlds/` | One Python generator per campaign world (`world011.py` ...), a shared `kit.py`, theme kits (`cheese.py`, `village.py`, `palace.py` ...) | 90 files, 16 372 lines; 78 `world*.py` | **AI-written generators.** "Each world has a brief, a builder script, staged variants for reachability checks and a live check"; built "partly by this session, partly by background agents it briefed and reviewed"; world 199 rebuilt by Codex-then-Claude. | Recorded (SESSION_SUMMARY §2.2, handoff, `plans/briefs/README.md`). |
| `worlds3d/` | 81 binary `.vwr` worlds (7.4 MB): main hub, 12 theme hubs, 64 missions, world 199, title, gallery 998, showcase 999 | 81 files | **Data produced by the generators above**: 80 of the 81 by Python scripts in `tools/worlds/` (78 `worldNNN.py`, `title.py`, and `gallery.py` for world 998; `worldkit.py`: "build `.vwr` worlds from Python, deterministically"); `world999` is written by the C++ tool `GenerateSampleWorld3D`. Not converted from the original's 2D maps (CLAUDE.md, briefs README: "Never convert, extrude or trace the original's 2D map"). The designs use the original's map "for identity, theme and mechanics" only. | Recorded. Note: the repo's word "hand-authored" means "designed, not converted"; it does not say a human typed the layouts. |
| `tools/autoplay/` | 83 scripted playthroughs plus `spare/` and `probes/`; step-script text read by the game's own `--autoplay` player | 84 files, 13 375 lines | AI-written scripts, partly searched and re-timed by tools (`autoplay_search.py`, `autoplay_chains.py`). Deterministic. | Recorded (SESSION_SUMMARY "79 autoplay scripts"; `autoplay_chains.py` docstring). |
| `tools/qa_steps/`, `tools/audit/`, other `tools/*.py` / `*.sh` | QA shell scripts (`qa_live.sh`, `qa_sweep.sh`), ledger generators, `levelcheck.py`, `campaigncheck.py`, `ci_pin_siblings.py` | 9 + 10 + 22 files, about 4 300 lines | AI-written helper tools. | Inferred (same sessions); several ledgers state in their header that they are generated by a named script. |
| `tools/rd4_to_gltf/` | Converter and verifier for original Ray Dream scenes (see `generated-assets.md`) | 30 files, 7 076 lines | **AI-written converter.** Its README records "every verified Ray Dream convention" with numeric evidence (e.g. DXF match RMS 0.0011). The file format was reverse-engineered from the scenes themselves; no vendor documentation is cited. First commit `573fc62e`, 2026-10-23. | Authorship Inferred; method Recorded. |
| `tools/terrain_relief/`, `tools/a3d/`, `tools/bolt/`, `tools/glue/`, `tools/music/` | Generators for terrain reliefs, the Amapi `.a3d` reader and lava data, lightning-bolt paths, glue strands, the music renderer | 11 files, 1 803 lines | AI-written generators; their outputs are committed (see `generated-assets.md`). | Inferred. |
| `Content/` | Original sprites, sounds, backgrounds; music renders | 157 files, 50 MB | **Not AI-produced: copied from Mobile Eggbert** (commit message of `92bd4bd5`: "copy assets from mobile-eggbert"); music rendered by a script from the original's MIDI. See `original-game-material.md`. | Recorded. |
| `avatars3d/` | 3D models, 2 491 files, 65 MB | | **Converted from original Ray Dream scenes by the AI-written converter**, plus one third-party placeholder. The geometry is the original's; the conversion, clip building, tone calibration and file layout are tool output. | Recorded (READMEs in each subdirectory). |
| `textures3d/` | `grass_top.png`, 64 x 64 | 1 file | **Procedurally generated** by "a small Python/Pillow script ... deterministically seeded" (commit `1d0d62a4`, 2026-07-08, "the user's chosen approach (procedurally generate rather than source externally)"). The script itself is not in the repository. | Recorded in the commit message; script not determinable. |
| `worlds/` | 5 original-format `.txt` levels (`world001..005.txt`) | 5 files, 1 223 lines | **Copied from Mobile Eggbert** (commit `b360df3a`, "real mobile-eggbert world loading (.txt format)"). Not AI-produced. | Recorded. |
| `mobile-eggbert-reference/` | Research notes (19 `.md`), 542 PNG and 129 GIF crops of the original sprites, two helper scripts | 692 files, 10 MB | Notes written by AI sessions ("DOC-001..DOC-306"), at the owner's scoped approval; images cut from the original sprite sheets by `extract-blupi-action.py` / `make-gif.sh`. | Recorded (`00-overview.md`). |
| `plans/` | Plans, 79 world briefs, audit ledgers, evidence screenshots (89 PNG, 32 JPG) | 240 files, 45 MB | AI-written documents and generated ledgers (`tools/audit/*.py`), at the owner's direction; owner questions and answers are quoted from the owner. Evidence images are screenshots of the game. | Recorded. |
| Root documents | `README.md`, `CURRENT.md`, `handoff.md`, `plan.md` (6 000+ lines), `NEXT.md` (frozen), `easy3d.md`, `REMAKE-ANALYSIS.md`, ... | 24 files, 19 330 lines | Mixed. `handoff.md`, `plan.md`, `CURRENT.md`, `REMAKE-ANALYSIS.md` are AI-maintained (their own text). `README.md` and `ANDROID.md` / `WINDOWS.md` / `Doxyfile` / `LICENSE` first appear in the 2026-05-12 "Added files" commit; their authorship is not determinable. | Mixed; see text. |
| `web/` | Project website (49 HTML, 1 CSS, 1 JS) deployed to GitHub Pages | 53 files, 5 948 lines | Imported with `git subtree` on 2026-09-30 from the former repository `openeggbert/galaxyeggbert.openeggbert.com`. Commit there: "Rewrite documentation to match actual GalaxyEggbertCNA architecture" (2026-07-04). The pages quote `CLAUDE.md` as their source of rules. Who wrote the HTML is **not determinable**. | Not determinable. |
| `android/` | Gradle project, manifest, `SpeedyBlupiActivity.java`, launcher icons, four symlinks to the asset directories | 28 files | Build glue and one 123-line Java file; the SDL Java sources are taken from CNA at build time (no copy committed). Launcher icons and the Activity were part of the initial 2026-05-12 commit; origin not recorded. | Not determinable for the icons and the Activity. |
| CI, CMake, packaging | `.github/workflows/cna-ci.yml`, `pages.yml`; `CMakeLists.txt` (997 lines); `cmake/`; `packaging/` | 9 files | `cna-ci.yml` says it implements BUILD-009 of `plan.md` and is pinned by `tools/ci_pin_siblings.py`; both AI-era artefacts. The first `CMakeLists.txt` is from 2026-05-12. | Inferred. |
| Sibling repositories | CNA, cna-engine, easy-gl, sharp-runtime, meta-gl | not in this repo | Each carries its own `CLAUDE.md`/`AGENTS.md` (observed by listing; contents not audited). | Not audited here. |

## 4. How much of the code tree existed before and after the agent files

First-add date is not authorship. It shows how much of `src/`, `include/`, `tests/`, `tools/` already
existed when `CLAUDE.md` appeared (2026-06-11) and when the completion campaign started
(2026-10-25). Text files only, current tree:

| First added | Files | Lines |
|---|---|---|
| before 2026-06-11 | 19 | 3 412 |
| 2026-06-11 to 2026-07-31 | 117 | 56 254 |
| 2026-08 to 2026-09 | 0 | 0 |
| 2026-10-01 to 2026-10-24 | 27 | 6 734 |
| 2026-10-25 and later | 279 | 44 510 |

Only 19 files (3 412 lines) of the current code tree predate the first agent instruction file. For
those, and for what the owner typed or reviewed in any file, the repository gives no answer.

## 5. AI-written generators and the data they produce

| Generator (in repo) | Input | Output committed | Notes |
|---|---|---|---|
| `tools/worlds/world*.py`, theme kits, `kit.py`, `tools/worldkit.py` | A written brief (`plans/briefs/worldNNN.md`); the original's world header (region, music) via `tools/world_inventory.py` | `worlds3d/*.vwr` (81 files) | Deterministic by the `worldkit.py` docstring; 79 files in `tools/worlds/` construct a seeded random generator. Not run for this audit: running writes into `worlds3d/`. |
| `tools/rd4_to_gltf/*` | Original Ray Dream scenes (`../speedyblupi-data/originals`, not in this repo) and the original sprite sheets | `avatars3d/blupi/`, `avatars3d/objects/` (121 model directories), `avatars3d/terrain/` (145 `.gtr`) | Also needs CNA's `tools/avatar_asset_pipeline/convert_avatar.py` (outside this repo). |
| `tools/terrain_relief/*` | Terrain scenes plus `Content/icons/object-m.png` | `avatars3d/terrain/*.gtr` | Uses Blender (`blender_decimate.py`). |
| `tools/bolt/bolt_paths.py`, `tools/glue/glue_strands.py` | `object-m.png` frames 305-307; `element.png` frames 168-171 (original sprites) | `src/GalaxyEggbert/Game/BoltPaths.hpp`, `GlueStrands.hpp` | C++ data headers traced from original sprites. |
| `tools/a3d/lava_surface.py` | Original Amapi `.a3d` sources | `src/GalaxyEggbert/Game/LavaSurfaceData.hpp` | |
| `tools/music/render_music.py` | Original MIDI tracks `MUSIC000..009.BLP` | `Content/music/music000..009.ogg` | Needs libfluidsynth, a General MIDI soundfont, ffmpeg. |
| `tools/audit/*.py`, `tools/asset_inventory.py`, `tools/world_inventory.py`, `tools/model_parity.py` | The tree plus the original asset inventory | `plans/audit/*.csv|md`, `plans/inventory/*.md` | Ledgers: "generated, do not edit by hand". |
| `tools/autoplay_*.py` | A playthrough script | `tools/autoplay/*.txt` | Re-timing and search. |

The generators' outputs are committed; their intermediate outputs go to `build-probe/` and are not.
Reproducibility per directory is in `generated-assets.md`.
