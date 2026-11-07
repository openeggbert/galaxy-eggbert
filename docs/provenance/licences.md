# Licence of this repository, and how the other material relates to it

Facts from the repository and from the sibling repositories' own licence files. **This is not legal
advice and the file decides nothing.** Where a question needs the owner's or a lawyer's decision, it
states the facts and the question.

## 1. The licence files in this repository

| File | Content | Added |
|---|---|---|
| `LICENSE` | GNU General Public License, version 3, 29 June 2007 (674 lines, the standard FSF text) | initial commit `ef03c601`, 2026-05-12 |
| `web/LICENSE` | MIT, "Copyright (c) 2026 Robert Vokac"; covers the project website imported by `git subtree` (merge `3ead7c05`, 2026-09-30) from the former repository `openeggbert/galaxyeggbert.openeggbert.com`, where "Add MIT license" is dated 2026-07-05 | 2026-09-30 |

There is no other `LICENSE`, `COPYING`, `NOTICE`, `licenses/` directory or third-party notices file.

What the repository says about its licence:

- `README.md` has no licence section. Its only licence-related line is about the Linux bundle ("...
  the SDL3 runtime libraries, licenses, and a launcher ...").
- `packaging/README-linux-bundle.md`: "`LICENSE` applies to Galaxy Eggbert. Licenses for the bundled SDL
  libraries are in `licenses/SDL/`. This archive is a runtime bundle, not the full source release; the
  source tree contains the build instructions and GPLv3 license."
- Source files carry **no** licence or copyright header: 0 files in `src/`, `include/`, `tests/`,
  `tools/`, `cmake/` contain `SPDX-License-Identifier`, "GNU General Public License" or "Copyright".
- No document states whether GPLv3 is meant "only" or "or later"; no document names a copyright holder
  for the source files; the copyright holders of the project are not listed.
- The Linux bundle installs `LICENSE` (GPLv3) and, when SDL3 is found, the SDL licence files. It does not
  install CNA's licence or notices (`CMakeLists.txt`, `install(...)` rules).

## 2. The licences of what the game is built on (summary; detail in `third-party.md`)

| Component | Licence recorded | Relation to this repo |
|---|---|---|
| CNA | Ms-PL (derived in part from FNA, Ms-PL) | Linked into the game; source from a sibling checkout |
| cna-engine, easy-gl, sharp-runtime, meta-gl | MIT | Linked into the game |
| SDL3, SDL3_mixer | zlib | Linked into the game; licence files bundled on Linux |
| GoogleTest | BSD-3-Clause (general knowledge) | Tests only |
| Khronos Fox model | CC0 (model), CC BY 4.0 (rig and animation) | `avatars3d/blupi_placeholder/`, fallback only |
| `speedyblupi-data` originals | "GPLv3, (c) Daniel Roux & EPSITEC SA", as quoted in this repository | Source of the converted models |
| `mobile-eggbert` | MIT, (c) 2013 Daniel Roux, (c) 2024-2026 Robert Vokac | Source of the copied sprites, sounds, levels and transcribed tables |

## 3. Open questions for the owner

Each question is stated with the facts the repository holds. None is answered here.

1. **Original sprites, backgrounds, sounds and level files.** `Content/icons`, `Content/backgrounds`,
   `Content/sounds` (140 files, 22 MB) and `worlds/world001..005.txt` are copies from Mobile Eggbert
   (commits `92bd4bd5`, `b360df3a`), shipped in the Linux bundle and the web build, and cut up into
   `mobile-eggbert-reference/images/` (671 files) and reused as source images of the terrain reliefs.
   Facts: Mobile Eggbert's `LICENSE` is MIT, (c) 2013 Daniel Roux and (c) 2024-2026 Robert Vokac; this
   repository's records label the original's 3D sources "GPLv3, (c) Daniel Roux & EPSITEC SA"; the
   repository holds no permission or correspondence from either name. Question: on what grant do the
   original images and sounds stand, and does the MIT notice or the GPLv3 label cover them at all?
2. **Rendered music.** `Content/music/` (10 OGG, 27 MB, shipped in the Linux bundle) is the original
   Speedy Eggbert 1 (1998) MIDI tracks rendered through a soundfont. Facts: the MIDI archive
   (`Speedy_Eggbert_1.7z`) and the soundfont used are not recorded; the default soundfont on the audit
   machine is `TimGM6mb.sf2`, GPL-2 per its Debian package. Question: what is the status of the MIDI
   tracks, which soundfont was used, and may the renders be redistributed?
3. **Ray Dream-derived models.** `avatars3d/` (70 MB; Blupi, five vehicles, 121 object models, 145
   terrain reliefs) is generated from the original's Ray Dream scenes, with textures and colours taken
   from the original sprites. Facts: the repository labels the scenes GPLv3, (c) Daniel Roux & EPSITEC
   SA, quoting the speedyblupi-data README, which this audit did not read; the models are tool output
   from those scenes. Question: is that label correct and sufficient for the converted models, for the
   regenerated geometry, and for the face textures embedded in the scenes?
4. **Transcribed tables and ported behaviour.** Twelve enum headers and at least ten tables or table families were transcribed
   from Mobile Eggbert's C++ (largest: `DecorQuartTable.hpp`, 7 056 values) and 662 lines of comments
   cite its source lines for re-implemented behaviour. Mobile Eggbert itself is described in its README
   as decompiled from the original (ILSpy) and migrated. Facts: the repository records the owner's
   approvals to the working agents (CLAUDE.md, source comments), which are not a licence from the
   original's authors. Question: does the MIT notice of Mobile Eggbert cover the transcribed material,
   and what covers Mobile Eggbert's own derivation?
5. **AI-authored code and data, and the repository licence.** The code, tests, tools, world generators
   and documents were produced in sessions with AI tools directed by the owner (`ai-involvement.md`);
   the git identity is the owner's alone; GPLv3 is the repository licence; the generated assets
   (worlds, models) inherit from their inputs. Facts: the repository contains no statement about the
   copyright status of AI-assisted output, no contributor agreement, and no record of the AI providers'
   terms for the output. Question: in whose name and on what basis is GPLv3 granted for the AI-authored
   parts, and for data generated by AI-written generators from third-party inputs?
6. **CNA's Ms-PL and the GPLv3 repository licence.** The game links CNA (Ms-PL, derived from FNA, Ms-PL)
   and ships a GPLv3-licensed binary. `cna-engine/NOTICE.md` states that a distribution that links CNA
   "must carry CNA's Ms-PL terms for the CNA portions". The Linux bundle carries `LICENSE` only.
   Question: can a binary combining Ms-PL and GPLv3 code be distributed under GPLv3, which notices does
   the bundle owe, and should the bundle also carry CNA's `THIRD_PARTY_NOTICES.md`?
7. **GPLv3 "only" or "or later", and copyright notices.** No file says which. No source file has a
   header, and no file names the copyright holder of the source. Question: which wording does the
   owner want, and should the files or `README.md` say it?
8. **The fox placeholder.** `avatars3d/blupi_placeholder/` carries the Khronos Fox: model CC0;
   rigging and animation CC BY 4.0 with attribution required. The attribution exists only in that
   directory's README, which the Linux bundle copies (the directory is installed). Question: keep it,
   replace it, or add attribution to the distribution's notices?
9. **`web/LICENSE` (MIT) inside a GPLv3 repository.** The website sources are in this repository under
   an MIT notice of their own; who wrote the HTML is not recorded. Question: is the mixed licensing
   intended, and does the website text derive from anything not owned by the owner?
10. **Origin of a few files.** `Content/Content.mgcb`, `Content/icons/*.xml`, the Android launcher icons,
    `SpeedyBlupiActivity.java` and the web shells (`cmake/web/shell*.html`) appeared in the first
    commits without a recorded source. Question: where do they come from, and what licence do they carry?
11. **Evidence images.** `plans/evidence/` and `tests/golden/` hold screenshots and golden frames (124 files) that show
    original sprites. Question: do they need the same treatment as question 1?
12. **Soundfont and tool licences for generated outputs.** If `TimGM6mb.sf2` was used, its Debian
    package states "GPL-2" (the text quoted says the samples are the author's or public domain or GPL).
    Question: is a GPL-2-only input compatible with GPLv3 for the rendered OGG files? (Linked to
    question 2.)

## 4. Compatibility notes from the documents (not conclusions)

- The repository itself records no incompatibility. It records two mismatches that a reader will notice:
  Mobile Eggbert is MIT while speedyblupi-data is labelled GPLv3, both for material derived from the
  same original; and CNA is Ms-PL while the game is GPLv3.
- The repository's own rules limit what the agents copy (CLAUDE.md "Faithful-remake rules"; "Do not add
  new copied asset sets without approval"); the 3ds Max episode (MR-0001) shows the project declining to
  use sources whose licence it could not confirm.
- The sibling repositories record their own licensing carefully (`cna/THIRD_PARTY_NOTICES.md`,
  `cna-engine/NOTICE.md`); this repository has no equivalent file.
