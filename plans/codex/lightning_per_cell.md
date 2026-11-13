# Brief: each lightning cell its own frame (the owner, 2026-11-13)

## What the original does

`../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp:1079-1083` (read only): every drawn cell
whose icon is 305 and where `BlitzActif(i, j)` holds is drawn with `m_random.get()->Next(305, 308)`,
i.e. **each cell picks one of the frames 305, 306, 307 at random, independently, every time it is
drawn**. `BlitzActif` (`Decor.cpp:620-634`): the bolt is shown, and deadly, only on the even ticks of
the first half of its 100-tick cycle.

## What Galaxy does now

- `src/GalaxyEggbert/BlockDefinitionRegistry.cpp:173-214`: `blitzFrames[100]`, one icon per tick
  for the whole world (a reproducible shuffle in bags of three, -1 = not drawn).
- `src/GalaxyEggbert/Game/TerrainRenderer.cpp:~1714-1720`: the lightning's meshes are cached by that
  icon (`m_blitzFrames[m_blitzIcon]`, at most 3 meshes), built by `RebuildAnimatedRenderer` with
  `AppendBoltCell(mesh, center, icon - Blitz, tileUv, BoltCellYaw(...))`. So every bolt cell shows
  the same frame at a time. Lava already does it per cell (BD-0065: cached by the step of its cycle,
  each cell its own frame on it) -- follow that pattern.
- The hazard timing (`WorldRuntime::IsBlitzActiveAtPhase`) must not change, nor anything about when
  a bolt is drawn; only which frame each cell shows.

## The change

- On each strike (each drawn tick of the cycle; 25 per cycle), each cell shows frame
  `305 + hash(cell x, y, z, strike) % 3`: deterministic (demos, golden frames and autoplay must stay
  reproducible), independent per cell, repeats allowed as with the original's random. Choose a good
  integer hash (the registry already has one, `bagHash`; reuse it or share one helper).
- Cache the meshes by strike (at most 25), built once each, dropped when the lightning cells change
  (as now). Keep the `-1` (not drawn) phases drawing nothing.
- The bolt's point lights, glow or sound, if anything else reads the per-tick icon, must keep working
  (grep for `Blitz`, `m_blitzIcon`, `BlitzShown`, `BlitzCells`).
- Update the comment in BlockDefinitionRegistry.cpp, `docs/handbook/08-backlog.md` §2 item 7 ("every
  bolt cell shows the same frame" is gone) and `plans/audit/behavior_differences.md` if it lists it;
  add a `CHANGELOG.md` line under `## [Unreleased]` (`### Changed`: the lightning's cells flicker
  independently, as in the original).

## Verification

- A pure-logic check (extend the verifier that already covers the lightning or the block animations,
  see `tools/Verify*.cpp` and CMakeLists.txt's tests): within one strike several cells of a column
  differ; one cell's frame varies over the strikes; all three frames occur; the same inputs give the
  same frames; non-strike ticks draw nothing.
- Build in the existing tree only: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4` (never more than 4 jobs; no new build directories;
  nothing in /tmp), then `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: everything
  must pass. If golden frames change because a captured view contains lightning, say which, and
  re-baseline them only if the difference is exactly the lightning (show it).
- A live look: find a campaign world with lightning (`worlds3d/`, cells of the Blitz block), run the
  game silently under Xvfb (`SDL_AUDIODRIVER=dummy`; the repo's autoplay/screenshot tools,
  `docs/handbook/04-testing.md`) and capture two strikes; save the frames under
  `build-probe/lightning/` and describe what they show (cells of one column with different frames).

## Rules

Do not commit, push, or modify anything outside this repository (`../mobile-eggbert` is read only).
Another agent edits `web/`, `tools/release/` and `.github/` at the same time: do not touch those.
Report: files changed, the hash used, the test results, the frames' paths.
