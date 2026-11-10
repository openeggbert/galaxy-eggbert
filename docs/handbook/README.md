# Galaxy Eggbert handbook for human authors

Galaxy Eggbert is a faithful 3D remake of *Mobile Eggbert* (the C++ port of *Speedy Blupi*, a
2013 Windows Phone game), built on the CNA engine. This handbook is for a developer who knows
C++ and Python, has never seen this repository, and will make changes by hand.

It is a condensed guide, not a replacement for the rest of the documentation. Every statement was
checked against the source tree, `CMakeLists.txt`, the scripts and the tools as of **2026-11-07**
(branch `develop`). Where something could not be checked it says "not verified". Many older
documents in the repository (`handoff.md`, `plan.md`, `plans/`, the code comments) are long
working logs; use them for history and evidence, and use this handbook to find your way in.

## Reading order

| File | Read it for |
|---|---|
| [01-overview.md](01-overview.md) | what the project is, the rules a contributor must keep, the repository map, a glossary |
| [02-architecture.md](02-architecture.md) | the game host, the engine-independent model, a frame's flow, the registries, rendering, the editor, the autoplay harness |
| [03-build-and-run.md](03-build-and-run.md) | prerequisites, configure/build/run, Windows, Android, web, the build-hygiene rules |
| [04-testing.md](04-testing.md) | ctest suites, golden frames and trace, autoplay scripts, live QA, the checking scripts |
| [05-worlds.md](05-worlds.md) | the `.vwr` format, generating and checking worlds, the authoring rules, the in-game editor |
| [06-assets-and-tools.md](06-assets-and-tools.md) | where models, textures, sounds and music come from and the scripts that produce them |
| [07-contributing.md](07-contributing.md) | coding conventions, commit messages, the ledgers in `plans/audit/`, upstream bugs, release status |
| [08-backlog.md](08-backlog.md) | everything still open at the end of the agent work, who has to act, where the evidence is |
| [09-multiplayer.md](09-multiplayer.md) | the network game (branch `multiplayer`): playing it, ZeroTier over the internet, its code and tests |

## First hour

1. Read [01-overview.md](01-overview.md), sections "The rules" and "Repository map".
2. Make sure the sibling repositories (CNA, cna-engine, ...) are where CMake expects them
   ([03-build-and-run.md](03-build-and-run.md), "Prerequisites").
3. Build into the existing tree and run `ctest` (about two seconds once built).
4. Run the game from its build directory and walk through world 001, then `--mission 999` (the
   showcase) and `--mission 998` (the gallery of every block and object).
5. Skim one generator (`tools/worlds/world012.py`) and its brief (`plans/briefs/world012.md`):
   they show how a world is authored and verified end to end.

## Where the project's own documents fit

| Document | What it is | Trust |
|---|---|---|
| `CURRENT.md` | verified state, limitations, open work | the status source, but parts of its tables are older than its header date (see [07-contributing.md](07-contributing.md)) |
| `handoff.md` | a long log of where work stood and which commands were used | useful evidence; partly historical |
| `plan.md`, `NEXT.md` | historical task inventory and migration write-ups (`NEXT.md` is frozen) | history only |
| `plans/GALAXY_EGGBERT_COMPLETION.md` | the completion plan: direction, art-fidelity policy, defect list D1-D60, milestones | the design intent |
| `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`, `plans/audit/` | the fidelity audit and its ledgers | evidence for every fixed difference |
| `plans/briefs/worldNNN.md` | one design brief per campaign world | read before touching that world |
| `World Format.md` | the `.vwr` binary format | accurate for what ships; has a hypothetical "future header" section |
| `mobile-eggbert-reference/` | researched notes on Mobile Eggbert's behaviour | background; check against the reference code |
| `upstream_bugs.md` | defects found in sibling repositories (`UB-NNN`) | live list |
| `CLAUDE.md`, `AGENTS.md` (the same text under two names) and the shared `../CLAUDE.md` one level up | the maintainer's working rules: fidelity, boundaries, build hygiene; condensed in chapters 1 and 3 | authoritative for those rules |
| `ANDROID.md`, `WINDOWS.md`, `packaging/README-linux-bundle.md` | platform notes | see [03-build-and-run.md](03-build-and-run.md) |

## Conventions in this handbook

- Paths are relative to the repository root unless they start with `../`, which means a sibling
  repository next to this one.
- Commands are meant to be run from the repository root.
- "Original" or "the reference" means Mobile Eggbert (`../mobile-eggbert`), whose C++ source is the
  authority for gameplay.
