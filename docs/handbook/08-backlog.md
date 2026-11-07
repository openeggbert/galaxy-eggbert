# Backlog and open items (2026-11-07)

Everything that was still open when AI-agent work on this repository stopped, written for the human
authors who continue by hand. Nothing here is a promise; each item says who has to decide or act, and
where the evidence is. Checked against the tree at the commit that added this file (branch `develop`).

## 1. Waiting on the owner (decisions and actions only a person can take)

| Item | What is open | Where to look |
|---|---|---|
| Push | The local `develop` is ahead of `origin/develop` (`git log origin/develop..HEAD --oneline`). Nothing from the agent sessions was pushed: the push was never requested, and one attempt was denied by the permission classifier. | `git status -sb` |
| Sibling pins for CI | `python3 tools/ci_pin_siblings.py --check` fails: the CNA checkout's HEAD (`feb0654f`) is not in any remote branch. The CNA session has to push it before the CI pins can be bumped. Galaxy Eggbert must not edit `../cna`. | `tools/ci_pin_siblings.py`, `.github/workflows/` |
| Release | The desktop and web builds pass every automated check; a release waits only on the owner's word. | `plans/audit/release_audit.md` §6 |
| Android | Builds, starts and draws the 3D models (UB-006 and UB-007 are fixed in CNA and were confirmed on an x86_64 emulator APK). Playing it on a device or an emulator with KVM was never done (Q-009); the owner said they will test on a phone. | `ANDROID.md`, `plans/audit/owner_questions.md` Q-009 |
| Grass on the title screen and plants | The owner reported a "nonsensical grass texture from above" five times. The terrain top texture `textures3d/grass_top.png` was rebuilt twice (OR-0082, `tools/textures/grass_top.py`; the last version has no black dot per tile, seen only tiled, not in the game). The screenshot of 2026-11-07 10:54 shows something else: tufts of BigDecor plants with long upright blades and flat blades lying nearly horizontal. Which model that is was never identified (not ids 16-31 or 36-47 of the gallery; the green ribbons of ids 45-47 are candidates). The owner will deal with this by hand. | `avatars3d/objects/bd_plante*/`, `plans/audit/owner_reports.md` OR-0082 |
| World 199 | The journey A-L was accepted by the owner on 2026-11-07 (gate 7). The main route plays in 40 138 ticks (about 11 minutes), the alternate in 53 074; the owner asked for more than 8 minutes. No human play pass was done, only the automated ones. | `plans/briefs/world199.md`, `tools/autoplay/world199.txt` |
| Skateboard | The parked skateboard hangs upright and spins (OR-0083); its height above the floor, `kSkateLift = 0.45f` in `GalaxyEggbertGame.cpp`, was tuned by eye on a probe world, not seen in a campaign level. | `plans/evidence/skate-upright-spin.png` |
| FNA AI policy | Not introduced yet (`/rv/data/development/github.com/ai-policy/AGENTS.md`, v2: AI may only advise, never modify). The owner's answers so far: it applies to new changes from the introduction date (the old history cannot be made to comply); the question whether the "never add AI attribution" rule of `CLAUDE.md` stays was left open. Introduce it only after the work in section 3 is committed, because from then on agents may not change anything. | `docs/provenance/`, section 4 below |

## 2. Technical backlog (small, none of it blocks a release)

From `plans/audit/animation_coverage.md` (ranked fix list; items 1-3 are done: Jump now ends in Air
after 3 ticks, the skateboard spins, TakeDynamite plays and holds Blupi 18 ticks):

- Item 4: done 2026-11-07 (BD-0084): the life-loss window plays the `Hide` clip, as the original draws
  table_blupi's Hide there.
- Item 5: ObjectType 31 (charge pickup) shows the electrode model and has no pose for icon 243; it is
  never placed in a shipped level.
- Item 6: turn clips for vehicles, swimming and the air from the `Turn*` icon tables. Large, and by
  design today (the model yaws over the original's own turn time).
- Item 7: the lightning's frames are shuffled now (2026-11-07). Kept on purpose: the spring's phase
  `(x+z)*7` (the 3D form of the original's column offset) and the fish head (BB3D-032, every hypothesis
  tested; reopen only if the owner names it). One difference stays: every bolt cell shows the same frame
  at a time, the original draws each cell's own random frame.

Other known gaps:

- The zoom cheat (OR-0079, steps 100 / 50 / 25 / 12 / 6 %) is limited by walls: the chase camera stops
  at them, so caves keep a short view. A transparent cut-away of walls between the camera and Blupi
  (option (b) of Q-016) is not built and means work in the terrain renderer's chunking.
- The saws' switch machine (384/385) keeps the open side as its front. With free cells on all four sides
  it can stand edge-on to the player. A version that turned it toward Blupi (OR-0081) was reverted
  because it swung as he walked past. A stable choice (for example toward the default camera) is open.
- The house's window recess is dark in the sprite (a cast shadow in the scene) and lit in the game, which
  has no cast shadows on models (cosmetic, `handoff.md`).
- Switch reach: `WorldRuntime::TryActivateSwitch` accepts a machine up to half a cell from Blupi's middle
  (OR-0084). The original needs him in its tile. If a switch fires when it should not, tighten
  `kReach` there.

Closed on 2026-11-07, for the record: a switch's saws no longer rebuild the terrain (they patch their
anim blocks, 0.03 ms instead of about 20 ms in the gallery, frames identical); the girder 201 stops the
chase camera like any wall since OR-0069 (`plans/evidence/camera-girder-vs-wall.png`).

## 3. Verified state at the end of the agent work

- `ctest --test-dir build-cna-opengl3`: 112/112.
- Golden frames and trace: refreshed on 2026-11-07. The trace for the Jump-to-Air change (positions
  identical, only the animation state differs); frames 0060 and 0120 for the new grass top texture
  (OR-0082: 36 pixels of a distant grass tile in world 999; checked by swapping the old texture back,
  which matches the old references exactly).
- Playthroughs: every campaign world has an autoplay script (`tools/autoplay/world*.txt`); the full pass
  of all 78 worlds on the final tree (`d44f6397`, after the saw patch, Hide and the lightning) passed,
  78/78 (`build-probe/play-pass-final-1107c/results.txt`, recorded in `handoff.md`). See [04-testing.md](04-testing.md) for how to rerun it.
- Probe leftovers: `build-probe/` was cleared on 2026-11-07 at the owner's request (about 4 GB). The
  `build-probe/...` paths that the ledgers and `handoff.md` cite (play-pass results, probe runs) are gone;
  their results are written down where they are cited, and the frames the ledgers rely on are in
  `plans/evidence/`. Only `build-probe/p052-run/` (12 MB) stayed: an orphaned `Xvfb :106` from a run on
  2026-11-04 has it as its working directory (no client, parent gone). Stop that process, then delete it.

## 4. Where the details are

- Open owner questions: `plans/audit/owner_questions.md` (nothing waits there; Q-009 and Q-016 are
  answered as above).
- Owner reports and their fixes: `plans/audit/owner_reports.md` (OR-0001 to OR-0084).
- Differences from the original: `plans/audit/behavior_differences.md`.
- Upstream bugs found in CNA and friends: `upstream_bugs.md` (UB-001 to UB-007, all fixed in CNA; UP-014,
  the editor wireframe on OpenGL ES, was Galaxy Eggbert's own bug and is fixed here).
- Provenance and licences: `docs/provenance/`.
