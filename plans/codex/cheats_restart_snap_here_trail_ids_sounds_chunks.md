# Brief C: cheats `restart`, `snap`, `here`, `trail`, `ids`, `sounds`, `chunks` (the owner, 2026-11-13)

Seven more MODERN typed cheats, after brief A (`plans/codex/cheats_mark_recall_photo_freeze.md`:
`mark`, `recall`, `photo`, `freeze`, the help in two pages) and brief B
(`plans/codex/cheats_freecam_xray_hitbox.md`: `freecam`, `xray`, `hitbox`), both implemented and
committed: read them and their code first and follow the same conventions -- `TypedCheatWords()`
entry (English one-line description, origin "Galaxy Eggbert", help page 2), MODERN `TypedCheat`, no
word collisions (`VerifyGEInputPad`), persistent toggles in the active-cheats list, off with
`fairplay`/`honest`, and **every toggle part of `resumeLocalCheats_` and `ResumeCheckSignature()`**:
typed letters are replayed by `back`/`recall`/`restart`/Continue, so a toggle missing from the
captured start state would be flipped twice by an in-process replay. Help page 2 must still fit
(check 800x480 and 800x360; shrink or re-flow the page's columns if needed, never overlap).

## `restart`

Back to the start of the current world through the same replay as `back`/`recall` (a rewind to tick
0), not a reload: no life lost, no fade. Same conditions as `back`; notice `restart`. The mark is
dropped as for any rewind before it.

## `snap`

Saves a PNG screenshot of the game view to the player's folder (`UserFolder.hpp`; e.g.
`screenshots/galaxy-YYYYMMDD-HHMMSS.png`, never overwriting) and shows the notice `snap: <file name>`
**after** the capture, so the notice is not in the picture. Reuse the existing screenshot code
(`GetBackBufferData`, the autoplay shots, `GetBackBufferSize` in `GalaxyEggbertGame.cpp`) and its
known caveats (no mid-frame read on Vulkan). Capture a fully drawn frame (with `photo` on, the bare
world). **Never during a replay** (`back`/Continue would otherwise save again), nor in a demo. Linux
desktop must work; say what happens on Android and the web (if the folder there is not reachable by
the player or a download is not simple, report it rather than inventing a mechanism).

## `here`

With `freecam` on: Blupi moves to the ground below the free camera (the first place under it where
his body fits standing on a floor; `PlaceBlupi()` / `SettleOnGround` as at a mission start), then
the free camera ends behind him. Without `freecam`, or with nothing to stand on below: the notice
`here: ...` saying why, and nothing changes. Deterministic (from the replayed camera pose and world),
refused in a network match like `freecam`.

## `trail`

A toggle: a line or ribbon in the world along Blupi's positions over the last 10 seconds (sampled each
tick inside the tick, so a rewind's replay rebuilds it exactly; cleared on a new world). Drawn over
the terrain, fading with age. Display only.

## `ids`

A toggle: above each active object (on screen and within a sensible distance, to avoid clutter) a
small label with its index in the world's object list and its `ObjectType` number (and the registry's
name if there is one), so a bug report can say "object 22 at ...". Display only.

## `sounds`

A toggle: an overlay listing the last ~10 sound requests -- channel number (and a short name if the
code has one), where it came from (a position, or Blupi), its distance from the listener, and what
happened: played, out of reach (`Sound::Audible`), channel still playing, muted (replay, hidden
Blupi, sounds off). Record these in `Sound` as a small ring buffer filled where the decision is made
(`PlayAt`, `Play`, `PlayChannel`), **without changing what is played**. Display only.

## `chunks`

A toggle: the terrain's chunk boundaries as wire boxes, the ones rebuilt in the last second or so
highlighted (the chunked terrain rebuild, OR-0074; `TerrainRenderer`). Reuse `hitbox`'s wire-box
drawing. Display only.

## Verification

- `VerifyGEInputPad`: the seven words fire once, have descriptions, sit on page 2, no collisions;
  page 2 fits. Pure-logic tests for what can be tested without a device: the sound ring buffer's
  outcomes (out of reach, busy, muted, played) with `Sound`'s decision logic, `here`'s ground search,
  the trail's 10-second window, the snapshot file naming (no overwrite).
- Probes with `GALAXY_BACK_CHECKPOINTS` / `GALAXY_RESUME_SELFTEST` (as `tools/autoplay/probes/back_*`):
  `restart` -> equal to tick 0; every new toggle typed then `back` -> equal; `freecam`, fly, `here`
  -> Blupi on the ground below, then `back` across it -> equal; `snap` typed, then `back`: exactly one
  file written.
- Build only in the existing tree: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4` (never more than 4 jobs, no new build directories,
  nothing in /tmp, no compiler cache of your own), then
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass, goldens unchanged.
- Screenshots in `build-probe/cheats-c/`: `trail`, `ids`, `sounds` (a world with sounds out of reach,
  e.g. hub 020 after `opendoors`), `chunks`, a `snap` file next to the frame it shows, `here` before/
  after, help page 2 at 800x480 and 800x360. Silent runs (`SDL_AUDIODRIVER=dummy`) under
  `AUTOPLAY_RUN=build-probe/cheats-c-run`; remove run directories and test screenshots written into
  the player's folder of those runs when done.
- Docs: `CHANGELOG.md` (`### Added`), a short `CURRENT.md` entry. Do not edit `handoff.md`.
- The door sound's reach is **not** to be changed (the owner, 2026-11-13): `sounds` only shows it.

## Rules

Do not commit or push. Do not modify anything outside this repository (`../cna`, `../cna-engine`,
`../mobile-eggbert` are read only). Another agent session may be working in this repository at the
same time: touch only the files this task needs and do not revert changes you did not make.
Report: the files changed, how each cheat is built and where its data comes from, the tests and
probes with results, the screenshots' paths, and anything left open.
