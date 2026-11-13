# Brief B: cheats `freecam`, `xray`, `hitbox` (the owner, 2026-11-13)

The second half of the owner's seven new MODERN cheats. Brief A
(`plans/codex/cheats_mark_recall_photo_freeze.md`, already implemented and committed) added
`mark`, `recall`, `photo`, `freeze` and the cheats' help in two pages; read it and its code first --
follow the same conventions: an entry in `TypedCheatWords()` (English one-line description, origin
"Galaxy Eggbert" so it lands on help page 2), a MODERN `TypedCheat`, no word collisions
(`VerifyGEInputPad`), persistent toggles in the active-cheats list, off with `fairplay`/`honest`
(`TurnCheatsOff`), part of `resumeLocalCheats_` and `ResumeCheckSignature()` so `back`, `recall`
and the self-tests stay exact. All three are display/camera aids: none may change what the
simulation does (except that `freecam` holds it still, below).

## `freecam`

A toggle. While on, the world stands still (the simulation does not step: Blupi, objects,
hazards, clocks) and the camera leaves Blupi and flies freely so the player can look round the
3D level; typing `freecam` again puts the camera back behind Blupi exactly where the chase camera
would be, and the world goes on from the same moment.
- Controls, all from the recorded input inside the tick (keys, mouse/touch drag; the gamepad arrives
  as keys, `GamepadInput.hpp`, so its left stick/d-pad, A and LB work through them): forward/back
  (Up/Down, W/S), turn or strafe (Left/Right, A/D -- choose and say), up (jump: Left Ctrl, gamepad
  A), down (crouch: Left Shift, gamepad LB), look by dragging the mouse/finger (as the play
  camera's drag-look). A sensible speed in cells per second; no collision (it may pass walls).
  Letters typed for cheats keep working (the typed-cheat suppression of gameplay letters applies).
- Off on a new world, on leaving Play for a mission's end, and by `fairplay`/`honest`. Pause works
  as usual and returns to the free camera. Refused in a network match (it would hold the world
  still), with one stdout line.
- The camera pose belongs to the replayed state: a `back`/`recall`/self-test over a stretch with
  `freecam` must end equal (the camera is in `ResumeSignature()`).
- With `photo` it is a photo mode; the help line may say so.
- Implement the hold as one gate where the tick's world steps are called (`UpdateTick` and its
  steps in `GalaxyEggbertGameUpdate.cpp`), not scattered checks; the camera's own update is where
  the chase camera is updated (`UpdateTickCamera`), with the frame interpolation above 60 fps kept
  smooth.

## `xray`

A toggle that shows, through walls, what the player looks for in this world: every remaining
treasure chest, key, life egg and other collectible/pickup (`ObjectDefinitionRegistry`'s semantic
kinds `Collectible`/`Pickup`), and the exit. Draw them so they read clearly over the terrain from
any distance at every effects level (Off ... Ultra): e.g. their usual icon or model again with the
depth test off, translucent and tinted by kind, or a marker on top -- choose the cheapest robust way
in the SceneFrame/WorldRenderer path (`src/GalaxyEggbert/Game/SceneFrame.hpp`, `WorldRenderer`), and
say why. Things already collected are not shown. If the block registry has walls that look solid
but let Blupi through (look in `BlockDefinitionRegistry` for such secrets; `showsecret` already
draws the flight barriers), outline those too; report what you found.

## `hitbox`

A toggle that draws the collision boxes the game really uses, as wire boxes in the world: Blupi's
body (the box his movement resolves against the blocks, crouched or not, in a vehicle), each active
object's contact box (the one `InteractionSystem` tests for pickups, harm and riding), lifts'
platforms and crates' cells, coloured by kind (Blupi, enemies, pickups, lifts/platforms, crates,
projectiles). **The numbers must come from the collision code itself** (expose the boxes from where
they are computed; do not re-derive them in the renderer), so the drawing can never disagree with
the game. Look for an existing line/outline drawing to reuse (the editor's selection outlines, the
`position` cheat's axes in `GalaxyEggbertGameDraw.cpp`). Visible through walls or occluded: choose
and say.

## Verification

- `VerifyGEInputPad`: the three words fire once, have descriptions, sit on help page 2, no
  collisions. A pure-logic test for any box/xray-target function you add (e.g. the boxes equal the
  collision code's for a few poses/objects; collected things are not xray targets).
- Rewind/self-test probes as `tools/autoplay/probes/back_*.txt` with `GALAXY_BACK_CHECKPOINTS` and
  `GALAXY_RESUME_SELFTEST`: `freecam` on, fly (hold keys), `freecam` off, play on, then `back`
  across it -> equal; the world's state (objects, Blupi, clocks) unchanged across the frozen
  stretch; `xray`/`hitbox` toggled then `back` -> equal.
- Build only in the existing tree: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4` (never more than 4 jobs, no new build directories,
  nothing in /tmp, no compiler cache of your own), then
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass, goldens unchanged.
- Screenshots in `build-probe/cheats-b/`: the free camera in two places of a campaign world (and
  with `photo`), `xray` in a world whose chests and keys are behind walls (before/after), `hitbox`
  with Blupi, an enemy, a lift and a crate in view; help page 2 with all its words. Check `xray` and
  `hitbox` at effects Off and High at least. Silent runs (`SDL_AUDIODRIVER=dummy`) under
  `AUTOPLAY_RUN=build-probe/cheats-b-run`; remove run directories when done.
- Docs: `CHANGELOG.md` (`### Added`), a short `CURRENT.md` entry. Do not edit `handoff.md`.

## Rules

Do not commit or push. Do not modify anything outside this repository (`../cna`, `../cna-engine`,
`../mobile-eggbert` are read only). Another agent session may be working in this repository at the
same time: touch only the files this task needs and do not revert changes you did not make.
Report: the files changed, where freecam holds the world and drives the camera, how xray and hitbox
draw and where their data comes from, the tests and probes with results, the screenshots' paths,
and anything left open.
