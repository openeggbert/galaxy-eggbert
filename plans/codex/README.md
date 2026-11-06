# plans/codex/ — task briefs for the Codex CLI

Each `*.md` here (except this file) is a self-contained brief for one task, written so that the Codex
CLI can do it unattended. The queue and its order are in `handoff.md` §0 ("Codex queue").

## How to run one

```bash
cd /rv/data/development/github.com/openeggbert/galaxy-eggbert
codex exec -m gpt-6-astra -c 'model_reasoning_effort="high"' -s workspace-write \
  -C /rv/data/development/github.com/openeggbert/galaxy-eggbert \
  --add-dir /home/robertvokac/.cache/ccache --add-dir /rv/cnaccache \
  -o build-probe/codex-last.md - < plans/codex/<task>.md
```

Model and effort (the owner asked, 2026-11-06): **GPT-6-Astra** (`gpt-6-astra`), effort **high**, for
every task here -- they need reading the original C++ closely, judging fidelity and verifying
unattended, where Astra's depth matters more than speed. Use effort **xhigh** for the world 199
rebuild (`world199_rebuild.md`: large spatial design plus a full playthrough). GPT-6.1-Sol
(`gpt-6.1-sol`, effort high) is fine for purely mechanical follow-ups (rerouting a playthrough after
a world change, regenerating a world, ledger bookkeeping).

## Rules every task keeps (from `CLAUDE.md`, `../CLAUDE.md`, `AGENTS.md`)

- Never modify a sibling repository (`../../libcna/cna`, `cna-engine`, `easy-gl`, `sharp-runtime`,
  `meta-gl`, `../mobile-eggbert`); never read `../speedyblupi-data` (original sources: never
  uploaded anywhere either). Bugs found in a sibling go to `upstream_bugs.md` (UB-NNN).
- Build only in `build-cna-opengl3/` (web: `build-cna-webgl2/`, scratch: `build-probe/`), with
  `CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv`, at most 4-6 jobs; never a new build directory or
  cache, never in /tmp. If a build fails inside `../../libcna/cna` (another session edits that live
  tree), stop and report.
- Game runs are silent (`SDL_AUDIODRIVER=dummy`; `tools/autoplay.sh` and `tools/qa_live.sh` do it).
- Every gameplay change traces to Mobile Eggbert's C++ (cite file:line); the editor is the one place
  where UX may be invented, never gameplay. Worlds are hand-authored true 3D: no 2D-to-3D
  conversion or extrusion.
- Verify before committing: `ctest --test-dir build-cna-opengl3 -j4` (all pass; 112 on 2026-11-06),
  `xvfb-run -a tools/verify_golden_frames.sh build-cna-opengl3` and `..._trace.sh` (re-baseline only
  for an explained change, and say so), the playthroughs a change can affect (`tools/autoplay.sh`;
  a full regression is `PLAY_PASS_JOBS=4 tools/play_pass.sh build-cna-opengl3 build-probe/play-pass-NN`,
  78/78 expected), and a screenshot check for anything visible.
- Record the result in the ledger the brief names (`plans/audit/*.md`, in its format), and a line in
  `handoff.md` §0's queue (done, the commit).
- Commit each task once, discretely: a conventional subject (`fix(...)`, `feat(...)`, `perf(...)`)
  naming the ledger id, a body saying what was wrong, what changed and how it was verified. **No AI
  attribution** (no Co-Authored-By, no "Generated with"). Never push: the owner pushes or asks.
