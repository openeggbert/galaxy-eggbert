# Brief E: cheats `perf` and `cascades` (the owner, 2026-11-14)

The second part of the owner's six cheats; brief D (`plans/codex/cheats_step_practice_replay_warp.md`:
`step`, `practice`, `replay`, `warp`) is implemented and committed -- read it and the earlier cheat
briefs in `plans/codex/` and follow the same conventions: `TypedCheatWords()` entry (English
description, origin "Galaxy Eggbert", help page 2), MODERN `TypedCheat`, no collisions
(`VerifyGEInputPad`), toggles in the active list, off with `fairplay`/`honest`, in
`resumeLocalCheats_` and `ResumeCheckSignature()`. Both are display only: they must not change
the simulation, and with them off every golden frame must stay byte-identical.

## `perf`

A toggle: an overlay graph of the last ~4 seconds of frames -- for each drawn frame its real time
since the previous one, and how long `Update` and `Draw` took, plus how many simulation ticks that
frame advanced (0, 1, 2: the frame-rate setting above 60 interpolates between ticks,
`frameRateInterpolating_`, `drawAlpha_`) -- with lines at 1/60 s and at the frame-rate setting's
target, and numbers: average, worst, the 1 % slowest. It exists to see stutter (the owner's rule:
camera changes need a smoothness check at 144 fps, `memory`: camera feel checks), so measure with
`std::chrono::steady_clock` around the existing `Update()`/`Draw()` (the `perfUpdateTimer` and the
`fps` cheat's counting are already there: reuse them) and keep the cost negligible when off. Hidden
by `photo` like the other overlays.

## `cascades`

A toggle: when the sun's shadow map is on (effects High/Ultra; `SceneShaders.cpp`, the cascaded
shadow map, `plans/codex/shadows_high_check.md`), tint what is drawn by the cascade that shadows it
(e.g. cascade 0 red, 1 green, 2 blue, 3 yellow, outside any cascade unchanged), terrain and models
and sprites alike, so the cascades' ranges and seams can be seen. Add the debug tint to the shaders
behind a uniform/define that costs nothing when off; do not change the shadow computation itself.
When shadows are off (Off/Low/Medium), the notice `cascades: shadows off` and nothing else.

## Verification

- `VerifyGEInputPad`: both words, descriptions, page 2 fits at 800x480 and 800x360.
- A pure-logic test for the perf statistics (average, worst, 1 % slowest over a ring of samples).
- Probes: both toggled then `back` -> `BACK SELFTEST equal` (their flags restored).
- Golden frames (all levels) and the golden trace byte-identical with both off.
- Screenshots in `build-probe/cheats-e/`: `perf` at the 60 and 144 frame-rate settings in a busy
  campaign world; `cascades` at High and Ultra in two campaign worlds showing several cascades and
  their seams; the notice at Off. Silent runs under `AUTOPLAY_RUN=build-probe/cheats-e-run`;
  remove run directories when done.
- Build only in the existing tree (`CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv`,
  `cmake --build build-cna-opengl3 --parallel 4`, never more than 4 jobs, no new build directories,
  nothing in /tmp), then `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass.
- Docs: `CHANGELOG.md`, a short `CURRENT.md` entry. Do not edit `handoff.md`.

## Rules

Do not commit or push. Do not modify anything outside this repository. Another agent session may be
working in this repository: touch only the files this task needs. Never add AI attribution anywhere.
Report at the end (also to `build-probe/cheats-e/report.md`): files changed, how each works, tests,
screenshots, anything left open.
