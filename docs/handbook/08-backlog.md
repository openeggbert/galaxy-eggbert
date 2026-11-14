# Backlog and open items (2026-11-14)

Everything that is still open, written for the human authors who continue by hand once the owner
introduces the AI policy (section 1). Nothing here is a promise; each item says who has to decide or
act, and where the evidence is. Checked on 2026-11-14 against branch `develop` (equal to
`origin/develop`, nothing waiting to be pushed) right after release **0.5.0**.

The short answer to "is anything big missing?": no gameplay, content or tooling gap is known. The
campaign (main hub, 12 hubs, 64 missions, world 199), the editor, multiplayer, twenty languages, the
release pipeline and the cheats are in, and every audit ledger is settled (section 3). What remains is
owner decisions, checks on real devices, platforms not started (macOS, iPhone) and small items.

## 1. Waiting on the owner (decisions and actions only a person can take)

| Item | What is open | Where to look |
|---|---|---|
| FNA-style AI policy | Not introduced. Two candidate texts in `/rv/data/development/github.com/ai-policy/` (git): **v2** `019b172` (AI only advises, never modifies the project) and **v3** `149d063` (AI edits only within a task a human explicitly authorizes, never creates commits; the human reviews and discloses AI use). The owner has not chosen yet. Decided so far: it applies to changes from its introduction date (the AI-written history cannot be made to comply). Still open: `CLAUDE.md`'s and `AGENTS.md`'s "never add AI attribution" rule, which v3's disclosure contradicts; both files still describe how agents work here and need rewriting with the policy. | `ai-policy/AGENTS.md`, `plans/plan_gemultiplayer.md` §2.5, this file |
| galaxyeggbert.com deploy key | The release workflow's last job updates both websites. The repository secret **`GALAXYEGGBERT_COM_DEPLOY_KEY`** (private half of an SSH deploy key with write access to `openeggbert/galaxyeggbert.com`) is missing, so that push fails with 403 (release run 43586953931). For 0.5.0 the site was updated by the documented manual fallback (`e91b384`); without the key every release needs that by hand. | `docs/releasing.md` ("The websites after a release") |
| Checks on real devices | Never done by the agents, only under Xvfb with a simulated touch screen or SDL's virtual gamepad: on a **phone or tablet** the cheats' keyboard (hold the middle of the screen 1 s), the cheats' help in large letters (8 pages), the touch controls; a **physical gamepad** (LT for the cheats' keyboard, d-pad/A in panels); **Windows** natively (the CI checks the zip's DLL imports, it was played mostly under Wine); **smoothness at 144 fps** on the owner's machine (the `perf` cheat now draws frame times). The owner played Android on a phone on 2026-11-10 (Q-009). On the desktop, `GALAXY_TOUCH_CONTROLS=1` plays as on a touch screen. | `plans/audit/owner_questions.md` Q-009, `CURRENT.md` |
| macOS and iPhone | Announced on galaxyeggbert.com ("on the way"); nothing is built or configured for them (only the resume code's executable-hash path names macOS). The only large piece of work still open, if wanted. | galaxyeggbert.com's Download section, `docs/releasing.md` (the four platforms built today) |
| Internet play without port forwarding | CNA's PlayerMatch relay (`cna-gamer-services-server`) would need a public server with TLS, accounts created by its administrator and a title `galaxy-eggbert`. The owner's decision (Q-022, open). Local network and VPN play work today. | `owner_questions.md` Q-022, `docs/handbook/10-server.md` |
| Speedy Blupi 1's network worlds 101-110 | They are Speedy Blupi 2's 201-210 under other numbers: the same cells, objects, starts, music and region, except 103 (four cells), 108 (SB2 has fewer ammunition packs, helicopters and a lift fewer) and 110 (SB2's 210 is wider, with a shield and bombs). Galaxy has the network worlds 201-217, and its own numbers 100-125 are the campaign's worlds 10-12. Recommendation: leave it (option (a)); more network worlds would be Galaxy's own design. | `owner_questions.md` Q-022 point 3 |
| World 199 | Accepted by the owner (2026-11-07, gate 7); played only by the automated passes, never by a person. | `plans/briefs/world199.md`, `tools/autoplay/world199.txt` |
| Parked skateboard's height | It hangs upright and turns about its long axis as the original (OR-0083, `d6190223`); its height above the floor, `kSkateLift = 0.45f` in `GalaxyEggbertGameScene.cpp`, was tuned by eye on a probe world, never judged in a campaign level. | `plans/evidence/skate-upright-spin.png` |
| The `.max` sources | Blocked by licence (MR-0001); the offered copy is not used. Nothing else in the asset ledger is open. | `plans/audit/manual_review.md` MR-0001 |
| Sibling pins for CI | `python3 tools/ci_pin_siblings.py --check` reports the pins in `.github/actions/checkout-siblings/action.yml` older than the local sibling trees. The release built and passed with the current pins; bump them with `python3 tools/ci_pin_siblings.py` only after verifying the newer siblings locally (they must be pushed in their own repositories, which Galaxy Eggbert does not edit). | `tools/ci_pin_siblings.py`, `docs/releasing.md` |

## 2. Technical backlog (small, none of it blocks a release)

From `plans/audit/animation_coverage.md` (items 1-4 and 7 are done):

- Item 5: ObjectType 31 (the charge pickup) shows the electrode model and has no pose for icon 243;
  it is never placed in a shipped level.
- Item 6: turn clips for vehicles, swimming and the air from the `Turn*` icon tables. Large, and by
  design today (the model yaws over the original's own turn time).
- The fish's head (BB3D-032): every hypothesis tested; reopen only if the owner names it.

Other known gaps:

- **Camera.** `plans/audit/camera_quality_2026-11-12.md` measured jumps and terrain occlusions of the
  chase camera and names their likely causes in `GalaxyEggbertGameUpdate.cpp`. A fix was reverted;
  the owner decided on 2026-11-12 to keep the current camera. Reopen only on the owner's word, and
  then measure at 144 fps with interpolation, in ghost mode and for smoothness against the old camera
  before merging.
- **The `zoom` cheat** (OR-0079) is limited by walls: the chase camera stops at them, so caves keep a
  short view. A cut-away of the walls between the camera and Blupi (option (b) of Q-016) is not built
  and means work in the terrain renderer's chunking.
- **Sun shadows (High, Ultra).** At the `zoom` cheat's widest step (16x) part of the view near the
  camera lies outside both cascades and gets no shadow (seen with the `cascades` cheat in world 001).
  The house's window recess is dark in the sprite (a cast shadow in the scene) and lighter in the
  game, where High keeps the ambient and fill light in full shadow.
- **The saws' switch machine (384/385)** keeps the open side as its front; with free cells on all four
  sides it can stand edge-on to the player. Turning it toward Blupi (OR-0081, `4bbfae47`) was reverted
  (`d6190223`) because it swung as he walked past. A stable choice (for example toward the default
  camera) is open.
- **Switch reach.** `WorldRuntime::TryActivateSwitch` accepts a machine up to `kReach = 0.55f` of a cell
  from Blupi's middle (OR-0084); the original needs him in its tile. Tighten it if a switch fires when
  it should not.
- **Sounds out of reach.** World sounds are heard within 15 cells of Blupi along X and Z and 11.25 up
  or down (BD-0076). In the 3D hubs some doors lie farther (hub 020: 16 cells, the main hub's gold
  doors: 31), so `opendoors` opens them in silence. The owner decided to keep it (2026-11-13); the
  `sounds` cheat shows such requests as "out of reach".
- **CNA:** UB-014 (the Guide's own texts are English only) is open in CNA, not here. UB-001..013 are
  fixed or explained.
- **Not requested, proposed:** Speedy Blupi 1's network debug words `netpacked`, `znetdebug`,
  `ynosmooth` (`../blupi_recon/src/event.cpp:74-76`) are not implemented.
- **Documentation.** This handbook's chapters 01-07 were checked on 2026-11-07. Since then came the
  network game (`09-multiplayer.md`, `10-server.md`), twenty languages, the release workflow
  (`docs/releasing.md`), the resume/demo formats and the cheats (`help` in the game lists all 54 words;
  their briefs are in `plans/codex/`). Read `CURRENT.md` and `CHANGELOG.md` alongside them, or refresh
  them.

## 3. Verified state on 2026-11-14

- `ctest --test-dir build-cna-opengl3 -j4`: 136/136, including the ten shipped demos' replays.
- Golden checks outside CTest: `SDL_AUDIODRIVER=dummy xvfb-run -a tools/verify_golden_frames.sh
  build-cna-opengl3` (nine frames) and `... tools/verify_golden_trace.sh build-cna-opengl3` (the
  trace needs the display too) pass.
- Playthroughs: every world's autoplay script replayed on 2026-11-14 on the 0.5.0 tree
  (`tools/autoplay_all.sh build-cna-opengl3`): **78/78 passed** (world 199's main route 40 259 ticks),
  the first full pass since 2026-11-07, after the network game, the camera's look up/down, the
  lightning, the shadows and the cheats. Results in `plans/evidence/playpass-2026-11-14.txt`.
- Audit ledgers (`plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`): the gameplay ledger 538 rows, the asset
  ledger 703 rows, the render representations, every world's lesson (80) and target reach (921) are
  settled, none open; only the `.max` (section 1).
- Release 0.5.0 (2026-11-14): tag `v0.5.0`, built, checked and published by
  `.github/workflows/release.yml` (Windows, Linux, Android signed with the release key, web, and
  `SHA256SUMS.txt`); galaxyeggbert.openeggbert.com and galaxyeggbert.com show it. Android's next
  versionCode is 10.
- `build-probe/` was cleared on 2026-11-14 at the owner's request (about 7.3 GB of probe output from
  many sessions, after checking that no process used it). Only `build-probe/windows/` stays: the
  MinGW cross-build tree that `tools/release/make_windows_zip.sh` reuses for a manual Windows release.
  The `build-probe/...` paths that `handoff.md` and the ledgers cite are gone; their results are
  written down where they are cited, and frames the ledgers rely on are in `plans/evidence/`.
- The worktree `../galaxy-eggbert-localization` was removed the same day (clean, at a commit already in
  `develop`). Its branches `localization` and `look-camera` are merged into `develop` and can be
  deleted.

## 4. Where the details are

- Verified current state: `CURRENT.md`; what changed in each version: `CHANGELOG.md`; where work stood
  and how it was checked, task by task: `handoff.md` (a long log; newest first).
- Releasing and the websites: `docs/releasing.md`.
- Owner questions and answers: `plans/audit/owner_questions.md`; owner reports and their fixes:
  `plans/audit/owner_reports.md`; differences from the originals: `plans/audit/behavior_differences.md`
  (BD-0001..0128, among them BD-0127 the cheats' keyboard and BD-0128 `opendoors` / `wellkeys`).
- The cheats added in November 2026, one brief each: `plans/codex/back_cheat.md`, `cheat_keyboard.md`,
  `cheats_mark_recall_photo_freeze.md`, `cheats_freecam_xray_hitbox.md`,
  `cheats_restart_snap_here_trail_ids_sounds_chunks.md`, `cheats_step_practice_replay_warp.md`,
  `cheats_perf_cascades.md`.
- Upstream bugs found in CNA and friends: `upstream_bugs.md` (UB-001..014).
- Provenance and licences: `docs/provenance/`.
