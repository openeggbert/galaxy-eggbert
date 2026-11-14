# Replays 1/3 — a recording keeps its world, the game version and the date (format 2)

The owner (2026-11-14) asked for a private collection of replays: (1) a "My recordings" screen,
(2) recording straight into the collection, (3) "protecting old recordings: store the exact world
data and the version with them". This brief is part 3, the foundation; parts 1 and 2 follow in
their own briefs (`replays_2_screen.md`, `replays_3_record.md`) — do NOT start them here.

Read first: `AGENTS.md`, `CLAUDE.md`, `plans/codex/README.md` (rules every task keeps), then
`src/GalaxyEggbert/Game/DemoReplay.hpp/.cpp`, `src/GalaxyEggbert/Game/ResumeLog.hpp/.cpp`,
`src/GalaxyEggbertCNA/GalaxyEggbertGameDemo.cpp` (`StartDemoFile`, `ResolveDemoWorld`,
`FinishDemoRecording`, `ImportDemo` use, `ListDemoImports`), `tools/VerifyDemoReplay.cpp`,
`tools/verify_demos.sh`, handoff.md "Main-menu demos" and "demo recordings" sections.

## Today

A `.gdemo` (format `GALAXY_DEMO 1`, `kDemoFormatVersion = 1`) holds the simulation revision, title,
recording build (information only), the hashed end signature, then the resume-log body (mission,
world path + FNV hash of the world bytes, seed, start state, input runs) and an FNV footer. It does
not hold the world: when the campaign world (or the player's galaxy world) changes or is gone, the
recording is refused (`WorldChanged`/`WorldMissing`). Another simulation revision is refused too.

## What to build

1. **Format 2** (`kDemoFormatVersion = 2`; `ParseDemo` reads 1 and 2, every check it does today
   kept, transactional as today). New fields, written by `Serialize()`:
   - the game version (`GalaxyEggbert::VersionString()` or the generated `GALAXY_EGGBERT_VERSION_STRING`
     — use whatever Version.hpp offers; it is generated into the build tree),
   - the recording date and time, UTC, ISO 8601 (`2026-11-14T09:30:00Z`), set by the host when the
     recording ends (not inside Serialize, so tests stay deterministic),
   - optionally the world's exact bytes, base64 (write a small base64 encoder/decoder in
     DemoReplay.cpp or a tiny header; no new dependency), with their length; `ParseDemo` checks that
     the decoded bytes hash (`ResumeLog::HashBytes`) to the body's `worldHash` (else `Corrupt`).
     Keep the text format line-based and readable like today; the FNV footer covers everything.
   - `DemoRecording` gets `gameVersion`, `recordedAt`, `std::optional<std::string> worldBytes`.
   A file without the world (format 1, or 2 without it) behaves exactly as today. The shipped demos
   (`Content/demos/*.gdemo`, format 1) stay byte-for-byte unchanged and must keep verifying
   (`tools/verify_demos.sh`, CTest `VerifyDemoReplays`, `VerifyDemoFiles`). The developer's
   `--record-demo` keeps writing without the world (the shipped demos must not grow by 300-500 KB
   each); the player's recordings (`StartPlayerRecording` -> `FinishDemoRecording`) embed it.
2. **Playing from the embedded world.** When the world the recording names is missing or changed
   but the file carries its world: write those bytes once to a content-addressed cache
   `UserDemoDir()/worlds/<hash>.vwr` (atomic write: temp + rename; reused when already there with the
   right hash) and play the mission from that file — the mission number as recorded (mission-
   specific behaviour, hints and the hub's rules depend on it), never touching the campaign's
   `worlds3d/` or the player's galaxy files. Find how a galaxy mission is loaded from its own path
   (`galaxyGamer_`, `loadedMissionPath_`, `ResolveDemoWorld`) and reuse that route; if the campaign
   route cannot take another path, add the smallest hook that can. Resume/Continue/back during a
   demo are already refused; keep it so. `DemoWorldStatus` gets a way to say "plays from its own
   copy" so the callers can tell the player (a message variant, see below).
3. **Another simulation revision** — the owner's decision is open (write Q-024, below); build the
   recommended default: `ParseDemo` gets an explicit way to accept another revision (a parameter or
   an out flag; the default call sites keep refusing, the menu demos and `verify_demos` included).
   Nothing in this part plays such a file yet; part 1's screen will (best effort, with a warning).
4. `ListDemoImports()` rows: show the version and date when the file has them (keep rows short:
   e.g. `"<name>  -  mission 11, 45 s, 0.4.1"`), and a file that plays from its own world copy is no
   longer "world changed/missing". Keep its English text short and add every new or changed
   Tr/TrMark/TrFormat string to all 19 `src/GalaxyEggbert/Game/Translations*.cpp` files
   (`VerifyLocalization` fails otherwise); translate properly for each language, as the existing
   entries there do.
5. Owner question **Q-024** in `plans/audit/owner_questions.md` (under "Open", the file's format:
   question, options, recommendation, what was done meanwhile): "A recording made before a change of
   the game's rules (simulation revision) may play differently. (a) refuse it (today), (b) list it
   and let it play best effort with a warning, never as a menu demo (built meanwhile, default),
   (c) keep old simulation code to replay it exactly (expensive, not recommended)."

## Verification (all required)

- New checks in `tools/VerifyDemoReplay.cpp`: base64 round trip (empty, 1-3 byte tails, binary
  bytes 0-255); format 2 serialize -> parse round trip with and without the world, the version and
  date fields kept; format 1 text still parses; a world whose bytes do not match `worldHash` ->
  `Corrupt`; a flipped byte anywhere -> `Corrupt` (footer); other revision refused by default,
  accepted with the explicit opt-in; the cache path and its reuse (a temp folder under
  `build-probe/`, never /tmp).
- `cmake --build build-cna-opengl3 --parallel 4` (CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv), then
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass (136 before this task).
- Live: record a player demo (`tools/qa_live.sh build-cna-opengl3 build-probe/replays-1/rec <steps>
  --mission 11`: Escape -> Pause -> Record -> a position -> play ~5 s -> Escape), confirm the file in
  the run folder's `demos/` is format 2 with the world; then change that world's bytes in a COPY
  (use `QA_LIVE_WORLDS=<dir>` with a modified copy of world011.vwr; never edit `worlds3d/`) and play
  the position from the menu (Demos / Demo button): it must play from `demos/worlds/<hash>.vwr`
  and end "finished" (the game log says so). Screenshots and a short `report.md` in
  `build-probe/replays-1/`.
- `tools/verify_demos.sh build-cna-opengl3`: all ten `equal`.
- Do not commit (Claude reviews and commits). Do not push. Leave the tree building and tested.
  End with a summary: files changed, design choices, anything not done or doubtful.
