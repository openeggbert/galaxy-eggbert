# handoff.md — where work stands and how to continue

For the AI agent that continues this work in a fresh context. Read this file first, then the
documents it points to. Update it after every committed task (what was done, where it stopped,
what to run next). `NEXT.md` is frozen history (do not add to it); the backlog with checkboxes is
`plan.md`; verified current state is `CURRENT.md`; the active plan is
`plans/GALAXY_EGGBERT_COMPLETION.md`.

_Last updated: 2026-11-12 (fifteen languages: Dutch, Hungarian, Romanian, Swedish, Russian, Ukrainian on branch `localization`; nine languages and the picker of flags merged; release 0.2.0 published, the site brought up to date; the language's flag in the menu; German and Czech, Q-023; the terrain rebuilt by chunks, OR-0074; English and French merged into develop, Q-023, BD-0126; release 0.1.3: the 1.5x speed on F4, F9 being CNA's debug context loss, UB-013; release 0.1.2 with the 1.5x speed; release 0.1.1 tagged and published; the repeated death sounds and the stuttering explosions fixed, BD-0121/0122; release 0.1.0 tagged and published on GitHub: Windows, Linux, Android, web; the branch `multiplayer` merged into develop by the owner; versioning: 0.1.0, not released, shown on the main menu with the build time; the outside review of 2026-11-07 checked: .vwr loading bounded, atomic world saves, every metadata type in the round-trip test; multiplayer Q-022 on branch `multiplayer`; Ultra: HDR, PBR, sky light, SSAO/SSR/contact, voxel GI, volumetric fog, TAA/DoF/motion blur/grain, finer atlas with maps, smoother models; frame rate option; fps and position cheats; teleporter top a floor; lightning zap; editor axes; High water texture fix; 3D effects in three levels, cascaded shadows for sprites too, Ray Dream materials, effects golden frames, UpdateTick split; 3D effects: FXAA, ambient occlusion, sun shadow map, point lights, moments as particles; GalaxyEggbertGame.cpp split by topic; the world drawn from a SceneFrame by WorldRenderer; optional 3D effects; brick wall ends ragged towards the open, Q-011; galaxy and demo sharing, galaxy gates in the menu; galaxy delete and Continue; demo recordings, ten demos, xnjttjpo, players' galaxies; main-menu demos; ghost/lava depth; editor/device controls, compass, fullscreen/help, glue fire, markers and Jeep recovery)._

## Six more languages (2026-11-12, the owner) — done on branch `localization`

Dutch, Hungarian, Romanian, Swedish, Russian, Ukrainian (BD-0126, Q-023's last note): their tables
(`Translations{Nl,Hu,Ro,Sv,Ru,Uk}.cpp`), the hints' columns, six flags, the picker 5 x 3. text.png's
glyphs 206-217 composed as before, 218-267 Cyrillic rendered from Comic Relief (`~/deps/fonts`, not in the
repository; `COMIC_RELIEF=<ttf>` for another path; without it `make_text_glyphs.py` cannot redraw those
rows); the Cyrillic letters shaped like Latin ones go through `TextMetrics::GlyphOf`. `TrCount`: Romanian,
Russian and Ukrainian plurals (three forms for ru/uk). Screenshots `build-probe/l10n/<code>-*`; the
system-locale check `build-probe/l10n/run-locales.sh`. Release 0.3.0 next (the owner's choice: after these
languages, published by the agent; versionCode 6).

## Nine languages and a picker of flags (2026-11-12, the owner) — merged into develop (54f1f6b7)

Slovak, Spanish, Italian, Polish, Brazilian Portuguese (BD-0126, Q-023's last note): their tables, the
training hints as per-language arrays, glyphs 171-205, five flags, `TrCount` plurals for Slovak and
Polish. The menu's flag and the Setup's Language button open `InputPad::DrawLanguagePicker` (host:
`OpenLanguagePicker`, `UpdateLanguagePicker`, `ChooseLanguage`; closed by SetPhase); the morning's
pulsing Setup button is gone. A Czech fix on the way: the kill count's trailing space. Screenshots:
`build-probe/l10n/{sk,es,it,pl,pt}-*`, `picker/` in the worktree. Not released (0.2.0 has four).

## Release 0.2.0 (2026-11-12, the owner's Q-023 answers) — tagged and published

`v0.2.0` on the release commit; GitHub release "Galaxy Eggbert 0.2.0" with the four packages,
SHA256SUMS.txt and packaging/RELEASE_NOTES.md; Android versionCode 5 (the next published APK needs 6).
Built against CNA `087db8818` (clean before and after each package; it has 518762a30). Packages in
`build-probe/release/0.2.0/` (script `build-probe/release/build-0.2.0.sh`); checked unpacked: Linux from a
read-only folder (Czech from the locale), Windows under wine (German from the locale), web over HTTP in
Chrome under Xvfb (Czech from the browser, "Verze 0.2.0", the flag), the APK's versionCode 5 / 0.2.0.
Network protocol 4: 0.2.0 does not play with 0.1.x. The site (`web/`) names 0.2.0 now (Releases rewritten:
four packages, languages, limitations; the platform tables; the menu's flag); deployed by the push.

## The language's flag in the main menu (2026-11-12, the owner) — done, released in 0.2.0

Right of Help, the flag of the game's language (`Content/icons/flags.png`, `tools/make_flags.py`); a press
opens the Setup with its Language button pulsing 3 s (`InputPad::SpotlightSetupLanguage`); BD-0126.

## German and Czech (2026-11-12, the owner's Q-023 answers) — done on branch `localization`

The owner's answers are in Q-023. Four languages (BD-0126): `TranslationsDe.cpp`, `TranslationsCs.cpp`,
the hints' columns; `TrCount` for Czech's plurals; text.png and its 2x sheet with glyphs 128-170 from
`tools/make_text_glyphs.py` (capitals with a mark drawn shorter; the original rows unchanged);
network names and chat in UTF-8, protocol 4 (0.1.x cannot play with it), CNA's profile name in ASCII
(`Net::AsciiText`). Screenshots: `build-probe/l10n/de-*`, `cs-*` in the worktree. The owner reads the
Czech in the game and sends corrections. The network game's sign-in hung in these runs because the
`cna` session had uncommitted Guide sign-in changes (GuideSignInFlow.cpp) that the build compiled; it
signed in with CNA's state before them: build packages only against a clean CNA commit.

## The terrain rebuilt by chunks (2026-11-12, the owner's choice) — done on branch `terrain-lag`

The rest of the lag of the 0.1.0 report (OR-0074 in owner_reports.md has the numbers): a terrain change
PatchModelCells refuses (dynamite cutting a saw, a drip, a fan) built the whole world's terrain on the
main thread, 60-70 ms (150 at Ultra). `TerrainRenderer` keeps the static meshes in chunks of 32^3
(`StaticChunk`; `BuildChunk` is the CPU part, `UploadChunk` the upload, `FinishChunks` the lists in
grid order and the totals) and `RebuildChangedChunks` builds only the chunks `ChunksReachedBy` gives,
called by `RefreshTerrainMeshIfChanged` after the model patch; a full build stays for an unknown
history, ShowSecret and the models loaded. 5-7 ms in world065. `VerifyTerrainChunks` (CTest) guards the
reach; golden frames byte-equal. Not done: the other parts of that report (the fixed-step catch-up
after a long frame, web audio on the main thread); the editor still rebuilds its whole presentation
after each edit (RebuildWorldPresentation).

## English and French (2026-11-11, owner Q-023) — merged into develop 2026-11-12 (the owner's acceptance after the web and Android checks)

Where: branch `localization`, worktree `../galaxy-eggbert-localization` (its own `build-cna-opengl3`,
configured with `-DCNA_HOME=/rv/data/development/github.com/libcna/cna -DCNA_ENGINE_HOME=.../cna-engine
-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=<main worktree>/build-cna-opengl3/_deps/googletest-src`). Merge
into `develop` only on the owner's acceptance (Q-023 a); `develop` moved on meanwhile in the shared
worktree, so merge `develop` into the branch first and run VerifyLocalization (a new text on develop
needs its French).

Done: `Localization.hpp/.cpp` (Tr, TrMark, TrFormat, the language, ParseLanguage),
`TranslationsFr.cpp` (390 texts; Mobile Eggbert's MyResource French where it has them), the training
hints' French in their table, `TextMetrics` on UTF-8 (the 15 accented glyphs, capitals unaccented,
PopBack), the Setup's Language button (top right; `language=` in savedata.txt; else CNA's UI culture,
the system's), every screen converted (menu, Setup, pause, help, demos, goodbye, editor, galaxy, network
game, notices). Labels too wide for their button or column are scaled down. `VerifyLocalization` in
CTest; CTest 130/130. Live in French (`build-probe/l10n/`): menu, Setup both ways, help, demo positions,
multiplayer list, editor, galaxy, goodbye, mission 11's hint, pause.

Open (owner_questions Q-023): acceptance and merge (a), the two corrected spellings of the original's
hints (b), a native speaker's proofreading (c), CNA's English sign-in toast (d), German or Czech next
(e: Czech needs new glyphs on text.png and its 2x sheet).

To check French live: put `language=fr` in the run folder's `savedata.txt` (tools/qa_live.sh's
`build-probe/qa-run/`), or press the Setup's Language button.

## Release 0.1.3 (2026-11-11, the owner) — tagged and published

`v0.1.3`: the 1.5x speed on F4 (below). GitHub release "Galaxy Eggbert 0.1.3" with the four packages,
SHA256SUMS.txt and the notes; Android versionCode 4 (the next published APK needs 5). The site's play
build and pages are 0.1.3. Built against CNA `46f8243ca`, where the `cna` session gated F9/F10 to
Debug builds (UB-013 fixed; option CNA_DEBUG_CONTEXT_HOTKEYS): in 0.1.3 F9 and F10 do nothing. The
first 0.1.3 package build had caught CNA mid-edit (Linux, Android) and without it (Windows, web); all
four were built again from the clean tree before the release.
For the next release (the `cna` session, 2026-11-11): a Windows package must not be built against
CNA `a4568ea54..ab834c8b6` -- there a search (NetworkSession::Find) as a process's first network call
runs before WSAStartup and throws NetworkNotAvailableException. Use `518762a30` or later for any
package (it also fixes three SystemLink QoS faults of `bee4190f1`: two upstream trains for an
invitation around its announce, a late alias saturating the downstream rate, an unclaimed report
credited to the only host). Those commits also fix SystemLink searches: a search no
longer binds port 61190, so a host on the same machine as a searching instance is found again.

## The 1.5x speed moves from F9 to F4 (2026-11-11) — done, released in 0.1.3

The owner found that CNA's `Game::PollEvents` takes F9 and F10 itself: its debug simulation of a
lost graphics context and its restore, in every build (UB-013; checked live: F9 logs "[CNA]
Simulating desktop GL context loss + immediate recreate"). On the web F9 leaves the game black until
F10, and 0.1.2 told players to press F9. My first search had cut its output at 15 lines and called
F9 free. The speed is on F4 now (BD-0125, CHANGELOG), released in 0.1.3.

## Release 0.1.2 (2026-11-11, the owner) — tagged and published

`v0.1.2`: the 1.5x speed on F9 (below). GitHub release "Galaxy Eggbert 0.1.2" with the four packages,
SHA256SUMS.txt and the notes; Android versionCode 3 (the next published APK needs 4). Packages in
`build-probe/release/0.1.2/`.

## 1.5x speed on F9 (2026-11-11, the owner) — done, released in 0.1.2

BD-0125: `GameSpeed::OneAndHalf` (0x20, factor 1.5), F9 in UpdateTick's speed keys, "1.5x" in the HUD,
the F1 help and the handbook. F9 was checked free in the whole stack first.

## Release 0.1.1 (2026-11-11, the owner) — tagged and published

`v0.1.1` on the release commit, the GitHub release "Galaxy Eggbert 0.1.1" with the four packages,
SHA256SUMS.txt and packaging/RELEASE_NOTES.md (its "New in 0.1.1"). Holds BD-0121..0124 below.
Android versionCode 2 (the next published APK needs 3). Packages in `build-probe/release/0.1.1/`.

## A new editor world gets the yellow Blupi (2026-11-11, the owner) — done, released in 0.1.1

BD-0124: CreateEditorStarterWorld sets the yellow start (51,1,50), facing -X, as Decor::InitDecor
puts Blupi at the platform's end facing the chest and the exit. VerifyGEEditorStorage checks it.
Note: the editor's camera starts above the world's centroid looking forward and down, so the board
is just below the view until the camera is moved back (S) -- unchanged, not part of this fix.

## Nicknames for players A, B and C (2026-11-11, the owner) — done, released in 0.1.1

BD-0123. `SaveData::GetNickname/SetNickname/GetPlayerName` ("gamerN.nickname=", kept by Reset); the
main Setup's Nickname button (top-left, `InputPad::SetupNicknameRect`) opens a panel
(`GalaxyEggbertGame::Start/Update/Finish/DrawNicknameEditing`, the network game's PanelRect and
ScreenKeyboard; the real keyboard's keys through browserTypedKeys_; the focus navigator gives its
keys to the arrows and a gamepad's d-pad). Shown in the menu's player row, the Setup's Erase row and
as the network game's name (`OpenMultiplayer`; GALAXY_NET_NAME still wins for tests). The panel is
the network screens' translucent one, the Setup showing through.

## Sounds repeated and stuttered (a Discord player on 0.1.0, 2026-11-11) — fixed, released in 0.1.1

The report (relayed by the owner and by the cna session, which analysed it read-only): "the sounds
seem to cause severe lagging sometimes; every sound is played several times without limitation".
- BD-0121: the hazards ran every tick of the death lock (Decor.cpp:5494 skips them while dying):
  the death's sound again each time it ended, the soul's voyage restarted, the trap's tentacle and
  the saw's burst spawned every tick; the large creature caught a dying Blupi every tick. Gated on
  `dying` / `blupiOutOfPlay`. Test: `tools/verify_death_sounds.sh` (CTest VerifyDeathSounds; it
  fails on the old code: 27/331/301 requests instead of 1).
- BD-0122: channel 10 restarted one instance; now overlapping instances as the original.
- `GALAXY_SOUND_LOG=1` prints every sound asked for and started (the test counts the requests).
- Not done: the lag's other parts (full terrain rebuild on the main thread after dynamite cuts a
  saw or drip, OR-0074; the fixed-step catch-up after it; web audio on the main thread), and the cna
  session's low-confidence list (edge-triggered Stop/Play pairs such as the crate push ch38 or the
  vehicle motor loop) -- not reproduced. Released in 0.1.1.

## Release 0.1.0 (2026-11-11, the owner) — tagged and published

`v0.1.0` (annotated) on the release commit; the GitHub release "Galaxy Eggbert 0.1.0" of
openeggbert/galaxy-eggbert carries the Windows, Linux, Android and web packages, SHA256SUMS.txt and
packaging/RELEASE_NOTES.md as its text. The packages were rebuilt from the release commit into
`build-probe/release/0.1.0/` (the APK too: `make_release.sh` without `--no-android`). CHANGELOG.md
has `## [0.1.0] — 2026-11-11` and an empty `[Unreleased]`. Android's next published APK needs
versionCode 2. The website (`web/`) still names 1.0.0-rc1 on several pages: not changed here.

## Packages 0.1.0 for Windows x64, Linux and the web (2026-11-11, the owner) — built, then published (above)

In `build-probe/release/0.1.0/` (`docs/releasing.md` step 3 has the commands and checks):
`GalaxyEggbert-0.1.0-windows-x86_64.zip` (166 MB), `-linux-x86_64.tar.gz` (163 MB, glibc 2.38+),
`-web.zip` (83 MB), RELEASE_NOTES.md (Windows, Linux, web; multiplayer), SHA256SUMS.txt over all
three. No APK (not asked). The Windows tree is now the main worktree's `build-probe/windows`
(configured as WINDOWS.md says; 94 % ccache hits), so the multiplayer worktree's can go with it.
New: `tools/release/make_web_zip.sh`, `packaging/README-web-bundle.md`; `make_release.sh`'s
checksums take the zips and no longer fail without an APK (nullglob). Found and fixed on the way:
- The game aborted on its first frame from a folder it cannot write ("ImageLoader: failed to save
  PNG: screenshot.png"): the startup screenshots are now skipped with a line in the log.
- The web build linked CNA::Net (CNA builds it under Emscripten now): Multiplayer in the page and a
  35 MB .wasm; the net is kept off under Emscripten (Q-019 Q4), 8 MB again.
- The build stamp missed a configuration change (flags, definitions): it now also follows a
  fingerprint `file(GENERATE)` rewrites only when those change. CMakeCache.txt is not rewritten by
  an unchanged configure, so it could not serve.
Checked unpacked: Linux from a read-only folder (`chmod a-w`), Windows under wine 10
(`SDL_VIDEODRIVER=windows`), web over `python3 -m http.server` in Chrome on X11 under Xvfb
(`env -u WAYLAND_DISPLAY google-chrome --ozone-platform=x11`; headless Chrome 155 creates no WebGL 2
context here) with a DevTools screenshot: menu, demo after 40 s, music fetched; each shows
"Version 0.1.0" and its build time, the web without Multiplayer. Full CTest 133/133.

## Versioning: 0.1.0, not released (2026-11-11, the owner) — done

As in `../cna` (`docs/releasing.md` there): Semantic Versioning, one place for the version, a
generated header, `CHANGELOG.md` (Keep a Changelog) and `docs/releasing.md`. The one place is
`VERSION.txt` (it was 1.0.0-rc1, now `0.1.0`), because Android's versionName and
`tools/release/make_release.sh` already read it: the root CMakeLists.txt validates it before
`project()`, gives the numeric part to `project(VERSION)` and the pre-release identifier to
`GALAXY_EGGBERT_VERSION_PRERELEASE`, and generates `<build>/generated/include/GalaxyEggbert/Version.hpp`
(`cmake/templates/Version.hpp.in`). The main menu (`InputPad::DrawInit`) shows "Version 0.1.0"
and under it the build's date and time, right-aligned left of Play's panel on a small panel of
its own. The stamp (`BuildStampText()`, `src/GalaxyEggbertCNA/BuildStamp.hpp`) is a source
`cmake/BuildStamp.cmake` writes again only when a game source, a header under include/ or src/, or
a library of the link closure changed: a no-op build stays "no work to do" (checked), a change
relinks with a new stamp (checked). Checked live on desktop (800x480 and 640x480) and built for
Android arm64 (`android/app/.cxx/RelWithDebInfo/2vu174b6`, the stamp and "Version " are in
libmain.so); no APK made. Releasing (tag `v0.1.0`, publishing) waits for the owner (Q-018).

## An outside review of 2026-11-07, checked against the code (2026-11-11) — done

An external review (written against `300041de`, a commit no longer on develop) was checked point by
point; the owner chose what to do. Not chosen (left as they are): material for the owner's own
review of R5 (D11 models) and R6 (a human play pass), and of the licence questions (R8). Done:

- R7: the Android accelerometer fix (86017658) confirmed by the owner on the phone (CURRENT.md).
- R4: the 56 `.max` rows of `plans/audit/asset_sources.csv` now say why they are NOT_APPLICABLE
  (MR-0001/MR-0010); `tools/audit/asset_sources.py` writes that note itself (`MAX_NOTE`). Do not
  regenerate the whole CSV while `build-probe/rd4-survey/scenes.csv` is missing (cleared
  2026-11-07): without it the generator blanks the survey columns of 528 Ray Dream rows.
- R3: `VerifyShippedWorlds`' round trip compares every chunk's whole metadata list (types 1-4 only
  before; 5-8 exist), the music track, the saw-floor flag and the size; on the first world it also
  checks that a record of an unknown type (0x1234) is seen by the comparison and survives a save.
- R1: `.vwr` loading refuses what a broken or hostile file states (World Format.md "What loading
  refuses"): chunksPerAxis 1..`VoxelConfig::MaxWorldChunksPerAxis` (10, also in the `World`
  constructor), the chunk table and every chunk's data within the file's bytes
  (`Binary::remainingBytes`), no chunk listed twice, a chunk's extra-metadata size within its
  payload, every palette index checked in `Chunk::read`. Before, a 56-byte header could ask ~4 GB.
  Negative tests in WorldTests.cpp/ChunkTests.cpp (`*TooMany*`, `*PastTheEnd*`, `*ListedTwice*`,
  `*LargerThanTheLimit*`, `*PaletteIndexOutside*`).

- R2: `World::saveToFile` is atomic: `saveToStream` into `<file>.tmp`, close checked (a full disk
  shows only there; before, an unchecked close let the editor mark a cut-off file saved), rename
  over the file. `WorldEditor::SaveWorld` returns false on a failure: the file stays as it was,
  `dirty_` stays, the status lines show "NOT SAVED: the file on disk is unchanged" and the reason,
  and a play test is not started (it would load the older file). Creating a new world in the
  browser that fails shows "Not created: ..." on the browser instead of ending the game
  (`WorldEditor::ShowBrowserMessage`). Tests: WorldTests.cpp `*KeepsTheFileAsItWas*` (one via a
  link to /dev/full), `*NoTemporaryFile*`. Checked live (tools/qa_live.sh, a directory put where
  `custom_00N.vwr.tmp` goes): Enter in the editor showed the two NOT SAVED lines, the game ran on
  and the file stayed byte for byte its backup; a new slot showed "Not created: Cannot open world
  file for writing" under the slots and stayed empty.


## Multiplayer (2026-11-10, owner Q-019) — merged into `develop` 2026-11-11 (the owner)

Where: branch `multiplayer`, worktree `../galaxy-eggbert-multiplayer` (its own build tree
`build-cna-opengl3`). The owner had it merged into `develop` on 2026-11-11 (merge commit after
`ed0ba050`); further work on the branch needs another merge. Design and status:
`plans/plan_gemultiplayer.md` (its header); the player's guide and ZeroTier:
`docs/handbook/09-multiplayer.md`; the differences from SB2: BD-0120.

Done: the protocol, the lobby, the CNA session, the screens (games on the network, lobby with chat),
the match (SB2's rules), poses and world replication, team colours, Android permissions and
multicast lock, worlds 201-205, `VerifyNetProtocol`, `tools/net_two_players.sh` with the scenarios
bomb/laid/shot/vehicle (all PASS), CTest 117/117 (single player unchanged).
2026-11-11 (the owner's Q-021, everything proposed): the others' sounds, the host's lifts, the
others' shield/power/cloud and the cloud's touch (SB2's); names over the players, a rematch back to
the lobby, the match's results, a silent player gone after 6 s, the host's address searched (CNA's
SystemLinkDiscovery from the `cna` session, CNA next 5d7579919) with the host on UDP 61191 (Galaxy's);
protocol 2; scenarios cloud and rematch; all five over a bad network (NET_LOSS/NET_LATENCY) PASS;
worlds 206-212 (briefs, `arena.check()`, which also moved two of 202's drinks off the air).
CNA changes go to the `cna` Claude session (SendMessage), not into ../cna.
2026-11-11 (the owner's Q-022, everything proposed; in progress):
- **Done:**
  - `tools/net_players.sh` for 2-4 players (NET_EXPECT, world hashes; `net_two_players.sh` wraps
    it), with four-player (kills) and three-player (pause, a leaver) scenarios.
  - Autoplay's `pause`, `netpause`, `netplayers`, `wander` and `hash`.
  - FuzzNetProtocol (CTest) and `tools/fuzz_net_sanitized.sh`.
  - Lobby and Start accepted from the host only; a hardened replication.
  - The world bombs' kills counted (they were never counted).
  - The four-player test found UB-011: CNA's joined clients answered discovery. Fixed in CNA
    c1b520dc9; the build compiles against that working tree.
- **Not built:** SB1's worlds 101-110 are SB2's 201-210 (owner_questions Q-022).
- **Also done, all PASS:**
  - The long match: four players for ten minutes, the worlds hash the same.
  - The lobby's lives and time limit.
  - Who got whom, and a chat line in the match (Enter; autoplay's `say`).
  - The spectator (left/right on the Lost screen).
  - The host's editor worlds sent (WorldData).
  - A dropped player back in the match (Rejoin/Snapshot; NET_REJOIN).
  - Protocol 3.
  - All the scenarios: `build-probe/net_all.sh` is a local probe; each scenario's environment is in
    its script's head.
  - **Windows:** `build-probe/windows` (commands in `WINDOWS.md`), tried under wine against Linux
    (`NET_WINDOWS`).
  - **Internet games** through cna-gamer-services-server's relay. Its server is built in
    `build-probe/gss` (out of its repository); `tools/net_internet.sh`.
  - **CNA notes:** UB-012 (send before Dispose) is XNA's contract; Galaxy updates before leaving.
- **After the owner's answers (Q-022, 2026-11-11):**
  - Worlds 213-217 of Galaxy's own design; a match started in each (over the local server).
  - The server's deployment: `tools/server/galaxy-server.sh` and `docs/handbook/10-server.md`.
  - The Windows zip: `tools/release/make_windows_zip.sh` into `build-probe/release/`.
  - The release APK rebuilt with `-PgalaxyAbis` (`injected.build.abi` made testOnly APKs).
- **Next:**
  - The owner: install the server on a host of his and make the accounts.
  - The owner: try the Windows zip and the APK.
  - The owner's real-machine test (Q-020).
- **CNA's sweep, 2026-11-11 11:30-14:30:** no LAN/discovery tests (UDP 61190/61191) in that window;
  the `cna` session asked.

How to check it:
```bash
cd ../galaxy-eggbert-multiplayer
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build build-cna-opengl3 --target GalaxyEggbertCNA VerifyNetProtocol --parallel 6
NET_LIVES=1 tools/net_two_players.sh build-cna-opengl3 tools/autoplay/net/bomb_host.txt tools/autoplay/net/bomb_join.txt
tools/net_two_players.sh build-cna-opengl3 tools/autoplay/net/laid_host.txt tools/autoplay/net/laid_join.txt
```
Android: the arm64 release APK of the branch is built (`android/app/build/outputs/apk/release/
app-release.apk` in the worktree, signed with the release key: `android/key.properties` there points
at the main worktree's keystore; real networking in libmain.so; rebuilt 2026-11-11 10:41 with everything of Q-022, protocol 3, which the
older APKs do not speak: AGP left it in `app/build/intermediates/apk/release/`, copied to `outputs/`; a
stale merge needed `--rerun-tasks`), not installed: the owner's phone test is Q-020. Its trees (`android/app/.cxx` 1.4 GB, `build` 0.9 GB)
can go once that is done. Golden frames and trace pass on the branch.
Next: a game between real machines (Q-020); the owner's look. Merged into `develop` 2026-11-11 (the
owner); the AI policy comes after it (Q-019).

## Lines between the cells with the 3D effects (2026-11-10, owner report) — fixed, owner confirmed

At medium and up the terrain's cubes (`m_occluded`), the grass plates (`LitWith`) and the cutouts are
drawn by different programs; the driver compiled the same `Projection * View * World` differently
in each, so their shared edges fought in depth and the faces under the grass plates (the side faces
between two grass cubes are drawn: a grass cube is no occluder) showed as thin lines along the cells
(light or dark by the world's side texture). `CompileShaderEffect` (SurfaceShaders.cpp) now puts
`invariant gl_Position;` in every vertex shader. The owner saw the lines gone on his GPU (high);
llvmpipe under Xvfb still shows some at medium (its rasteriser, `GALAXY_EFFECTS_DEBUG=occlusion`
shows them white): if they come back on hardware, cull the side faces between two grass-top cubes
or lift the grass plates a hair in the effects' path. Golden frames (off/medium/high) unchanged.

, on-screen keyboard, gamepad, demo names, galaxy delete, goodbye, release packages

The owner's list (from the comparison with blupi_recon's port): finish the day's screens for
touch and gamepad, name demos, delete a whole galaxy, keep the player's files in the system's
user folder, build release packages, and a goodbye screen on quitting as in Speedy Blupi.

- **User folder (done, desktop verified).** `src/GalaxyEggbert/Game/UserFolder.hpp/.cpp`:
  `UserPath("savedata.txt")` etc.; savedata.txt, resume.txt, demos/ and customworlds/ all go
  through it (SaveData::SavePath(), ResumeLog::Path(), UserDemoDir(), CustomWorldsRoot()).
  main.cpp `ChooseUserFolder()`: `--user DIR`, else `GALAXY_USER_DIR`, else CNA's
  `StorageDevice::GetStorageRootEXT()` for "Galaxy Eggbert" (`$XDG_DATA_HOME/Galaxy Eggbert`,
  `~/.local/share/Galaxy Eggbert`, `%LOCALAPPDATA%\Galaxy Eggbert`). The first start without
  `--user`/env copies an earlier build's files from the working folder when the user folder has
  no savedata.txt (`CopyOldUserFiles`, nothing moved or overwritten). The web keeps `/save`,
  Android its private storage (the working folder), the tools and tests the working folder (the
  folder is empty until the host sets it). **Test runs must pass `GALAXY_USER_DIR`**:
  autoplay.sh, qa_live.sh, record_demo.sh, verify_demos.sh set it to their run folder,
  verify_golden_frames/trace.sh to the build folder (`.`), the live harness
  (`build-probe/demo-live/harness/demo_live.py`) to its run folder. A hand-started
  `./GalaxyEggbert` in a build folder now uses `~/.local/share/Galaxy Eggbert` (the first time it
  copies that folder's savedata.txt there). Checked live (build-probe/user-folder/): the copy
  once, the second start without a copy, `--user` over the default; CTest 120/120.
- **Arrows and gamepad on the new screens (done, desktop verified).** `Game/FocusNavigator.hpp`:
  the screen gives the rects of what can be pressed; the arrows (keyboard; the d-pad, which in
  Pause comes as the arrows) choose one, A or Enter presses it (a mouse at its centre, pressed one
  update, released the next), the mouse, a touch or the gamepad's stick ends the choice. Wired in
  `UpdateTickScreens`: the demo positions (Pause and the menu's Demos:
  `DemoSlotsFocusTargets()`), the galaxy screen and the world screen
  (`GalaxyBrowserScreen/EditorBrowserScreen::FocusTargets()`, drawn lit with
  `SetFocusHighlight`). In the editor and the menu the d-pad chooses instead of moving the pad's
  cursor and A goes to the choice while there is one; B in Pause leaves the positions a step.
  `EditorDeviceControls`: with a gamepad plugged in the real mouse works again (it moves the pad's
  cursor, its button clicks as A; before, the pad's cursor replaced the mouse in the menu).
  Neighbour: the nearest that way, a target in the same row/column always first.
- **Keyboard on the screen (done).** `Game/ScreenKeyboard.hpp`: digits, QWERTY, '-', Shift (on at
  an empty name's start), Del, Cancel, Space, OK; `TypeKey()` for the real keyboard. The galaxy's
  name (its keys over the worlds and places while naming; a click off it keeps the name) and a
  demo's name.
- **Demo names (done).** The positions' bottom row is Export, Import, Name, Cancel
  (`InputPad::DemoNameRect`); Name then a recorded position types its name (`StartDemoNaming`,
  the keyboard in place of the positions, real keys through `browserTypedKeys_` in Pause/Init).
  `DemoReplay`: `SanitizeDemoName` (24, letters/digits/space/'-'/'), `IsAutomaticDemoTitle`
  ("Recorded by player "), `DemoTitle`, `RenameDemo` (the title in the file, the rest as it was);
  a named demo shows its name on its position and in Import's list, and exports as
  `<name>.gdemo`.
- **Delete galaxy (done).** `CustomWorldStorage::DeleteGalaxy()`: the galaxy exported first into
  the shared folder (Import lists it: two clicks bring it back with its name and demos), then
  every place into its backups, the name removed; the galaxy screen's "Delete galaxy" under
  Export/Import (two clicks), its progress started over (`UpdateResult::deleted`).
- Live cases (build-probe/demo-live/harness/): `case_galaxy_keys.py` (keyboard on the screen,
  arrows + Enter, Delete galaxy, Import back: 7/7), `case_demo_names.py` (5/5),
  `case_demo_pad.py` (virtual gamepad `build-probe/controls/virtual-gamepad.so` via `extra_env`:
  the positions from the menu by the d-pad and A, a name typed on the screen's keyboard, B: 5/5).
  Xvfb loses the first pointer event at times: the cases move the mouse first and retry Demos.
- **Goodbye on quitting (done).** As Speedy Blupi's `WM_PHASE_BYE` (blupi_recon `src/event.cpp`
  1755-1777: two lines at the top, two at the bottom, a key or a click closes): the menu's Exit and
  Escape call `StartGoodbye()` instead of `Exit()`; `InputPad::DrawGoodbye` over the title scene
  (our own words, not the original strings), a new press of anything (`DemoNewPress`, not what
  clicked Exit) ends the game; the idle demos wait. Closing the window still quits at once.
  `case_goodbye.py`: 3/3.
- **Galaxy's loading screen from the first frame (done, owner report).** Wait drew Mobile Eggbert's
  wait.png until the title scene was up (about a second at 144 fps); now
  `DrawGalaxyLoading(..., overTitleScene=false)` draws the Galaxy Eggbert logo with Loading and the
  gauge until it is (checked without title.vwr).
- **Light lines along the cells (fixed, b827fad2 + b7756ee1).** Two causes: the grass cube (107,
  top open) was no occluder, so its neighbours' faces against it were drawn on the boundary and
  showed through the grass plates' edges with the effects (`IsTerrainOccluderFace`, per face); and
  the 2x stand-in sheet left in `build-cna-opengl3/Content/icons2x` (moved to
  `build-probe/stale-2x-standins/`) had a light 2-px band and soft alpha at every tile edge
  (Lanczos ringing; opaque tiles are now forced to alpha 255 at load, and ray fixed the builder).
- **2x sheets committed (ray's final set, handed over by ray and geultra).** Copied from
  `../speedyblupi-hd/galaxy/{icons2x,backgrounds2x}` (48 files, each checked exactly 2x its 1x file,
  icon 10's edge texels = the 1x edge colour, alpha 255); live at Ultra 1360x840 with
  `GALAXY_TEXTURES=2x`: the log says `textures 2x`, no light lines. Golden frames force 1x.
- **Megablupi keeps the breath under water (fixed, owner report):** `BlupiController` gauge gate
  now has the original's `!m_bSuperBlupi` (Decor.cpp:4617) beside Shield/Hide; VerifyBlupiMovement.
- **Release packages (Linux and Android done; Windows and glibc 2.35 open: Q-018).**
  `tools/release/make_release.sh <Release build> <out>`: CPack's `package` (now
  `GalaxyEggbert-<VERSION.txt>-linux-x86_64.tar.gz`, with THIRD_PARTY.txt, RELEASE_NOTES.md and
  VERSION.txt from `packaging/`), android/'s `assembleRelease` for arm64-v8a (versionName read
  from VERSION.txt), SHA256SUMS.txt; prints the glibc the program needs (2.38 today). 1.0.0-rc1
  built into `build-probe/release/`; the Linux archive (139 MB) extracted and run under Xvfb:
  SDL3 from its `lib/` (RUNPATH `$ORIGIN/lib`), the user folder as asked, the game loop reached.
- **Android release APK fixed (2026-11-10, evening).** `-Pandroid.injected.build.abi` made a
  test-only APK (`adb install` refused it without `-t`): one ABI is now `-PgalaxyAbis=arm64-v8a`
  (build.gradle's abiFilters and ndk.abiFilters; both ABIs without it). The manifest's hardcoded
  `android:debuggable="true"` overrode the release build type; removed (release not debuggable,
  debug still is). 1.0.0-rc1 was installed on the owner's phone (24040RN64Y) with `-r -t` from the
  test-only build: the same code; the proper APK is in `build-probe/release/`.
- **Thick lines on some 2x cubes (owner report, fixed in the assets).** Some cells ray rendered from
  the Ray Dream scenes caught a neighbour's edge or shadow (the toy cubes enfant01-06 a dark line
  at x 55, techno03 a bar at x 4, palace walls 389/390; techno11 lacked its painted arrows): his
  whole-cell check let a narrow line through. Measure: the 2x tile box-reduced to 1x against the
  1x tile, mean |RGB| per row/column over texels opaque in both; ray's builder now rejects >30
  (fallback: per-cell edge-padded Lanczos) and aligns every scale to the 1x grid (a quarter-pixel
  shift before). 8 files replaced; `tools/check_2x_sheets.py` = CTest `Verify2xSheets` (the old
  sheet: 18 cube tiles over 30; the new: 0 of 140).
- Was still to do from the goal: the release packages (Windows zip, Linux tar.gz for glibc 2.35, Android APK, checksums,
  README/COPYING/THIRD_PARTY, release notes), the 2x sheets when ray announces them (ray and
  geultra handed that to this session).

## 2x textures (../ray.md, 2026-11-09) — code done, the re-rendered sheets follow

- The game loads `Content/icons2x/` and `Content/backgrounds2x/` (sheets at twice the size, the
  1x layout times two, re-rendered from the original Ray Dream scenes in
  `../speedyblupi-hd/`) when 2x is wanted: Setup's new "Textures" (Auto / 1x / 2x, at x 450 on
  the Frame rate row; `SaveData textureScale`, applies at the next start) or
  `GALAXY_TEXTURES=1x|2x|auto`. Auto = 2x on desktop with Ultra on a display at least 1000 px tall
  (Galaxy draws on CNA's 800x480 logical surface; only Ultra draws the world at the window's
  pixels); always 1x on the web and Android. `TextureScale.hpp`: `ScaledContentPath` (falls back
  to 1x unless the 2x file is exactly twice the 1x one), per-texture scale, `LogicalWidth/Height`
  for every pixel-to-UV computation (Hud, InputPad, editor).
- Terrain at 2x: `TileAtlas(Layout::Padded, 2)` / `PaddedLayoutFor(2)` = 128-texel tiles in a
  16-texel border, 3200x3520 (twice the 1x layout would be 3840x4224, over HiDef's 4096);
  `ClassifyTileSheet`, `FillOpaqueTilesClearTexels`, `FillBrickEdgesFromWholeBrick`,
  `BuildPaddedSheet` take the scale; Ultra's maps keep their relief strength (x scale) and the 2x
  atlas is not upscaled again. Verifiers: VerifyTileFaces / VerifyTileCoverage on synthetic 2x
  sheets. Checked live with upscaled stand-in 2x sheets: 1x and 2x screenshots of mission 11 have
  the same layout (mean difference 0.65).
- Next: the Ray Dream 2x sheets into `Content/icons2x/` + `Content/backgrounds2x/` (owner-approved
  asset set, ray.md), then a live check in campaign worlds with `GALAXY_TEXTURES=2x GALAXY_EFFECTS=ultra`.

## Owner's batch, evening of 2026-11-09 — verified desktop; Android not run on a device

- `22c1ed3d` contact shadows without dither (separable normal/depth-weighted blur, upward faces
  only); `608e7a3a` SSR on metal and polished floors (the geometry pass reads the surface map).
- `8ed6ed0a` typed cheat `honest` (the owner's name for it; `nocheats` ended in the word `cheats`):
  every cheat off and cheat entry locked until the application quits, fairplay and honest
  included. Resume logs record it as `cheatsLocked 2`; a demo keeps the player's lock; a resume
  reconstruction ends with it applied again. Typed words match the longest word now.
- `1505aae6` the jeep drives off a lift onto the block beside it: its edge test (ground under
  each end) counts the tops of lifts level with its wheels (`InteractionSystem::LiftTopsNear`,
  `BlupiController::SetLiftTops`); it tipped and rolled back onto the lift before.
- `0e318cb4` Ultra sprites and particles lit (sun through its shadow, sky irradiance, point
  lights), fire/sparks/stars above white (`SceneQuad::glow`); relief GI (`ReliefChunkGi`, an
  8x8x8x6 table per relief chunk from its far cubes' bake, read as an ambient cube;
  `GALAXY_EFFECTS_DEBUG=gi`); the water's 2D tile replaced at medium/high/Ultra (body colour from
  `TileTint`, waves `WaterWaves.hpp`, Fresnel clearness, glint, Snell's window from inside; Ultra's
  geometry pass carries the waves, so the reflections ripple). Golden frames unchanged (no water).
- `1bc1ac82` Android: the Setup's frame rate is its own row over Sound effects (the sensitivity
  slider hid it); with the accelerometer delivering readings the stick becomes Forward/Back
  buttons (Back = back up; the back-up button hidden; `GALAXY_ACCEL_PAD=1` shows them on a
  desktop); the accelerometer's support is asked once instead of every tick, and the log says
  present / started / first reading / no reading in 3 s.

- `2e898a51`, `3bb6a3a4` verification and the cave sprites: `GALAXY_INTERP_TRACE=<file>` writes each
  frame drawn in Play (live at 240 fps under Xvfb: a lift and the tank's projectile in 051 drawn
  part way within a step, the camera's shake eased); `GALAXY_EFFECTS_DEBUG=sprites` shows Ultra's
  sprite light; sprites and particles now take the light from around at their cell
  (`SceneQuad::gi`, FaceIndirectLight looking up, cached by cell), so in a cave they are as dark as
  its floor; `GALAXY_RELIEF_GI=0` draws the reliefs without their GI to compare (011 with
  `GALAXY_TERRAIN_RELIEF=1`: 55-145 thousand of 384 thousand pixels differ). The reliefs are off by
  default (`terrainRelief=0`): their GI shows only with the Setup's "3D terrain" on.

Open (Android, owner's reports): the accelerometer did not react on the phone (fixed in 86017658,
the owner confirmed it on the phone 2026-11-11) and the game sometimes freezes until the app is
restarted. The freeze is not reproduced: no device was connected. Next:
`adb logcat -s SDL SDL/APP GalaxyEggbertCNA` (or the whole logcat) with Accelerometer on in a
mission, and after a freeze (when: after the app was in the background, during play, at Continue?).

## Android: own package id (2026-11-09) — installed and started on a phone

The APK's `applicationId` was `org.openeggbert.speedyblupi`, the same as mobile-eggbert's Android
app (`../mobile-eggbert/android/app/build.gradle`), so it could not install beside it (signature
mismatch) and `am start` opened mobile-eggbert instead. Now `org.openeggbert.galaxyeggbert`, label
"Galaxy Eggbert"; the namespace and the Java class `org.openeggbert.speedyblupi.SpeedyBlupiActivity`
stay. Launch: `adb shell am start -n
org.openeggbert.galaxyeggbert/org.openeggbert.speedyblupi.SpeedyBlupiActivity` (`ANDROID.md`).
Release APK (`./gradlew assembleRelease -Pandroid.injected.build.abi=arm64-v8a`, release key) installed
on a Xiaomi 24040RN64Y (HyperOS asks on the phone before a USB install of a new app): assets copied,
OpenGL ES 3.2, world001 loaded, the title screen drawn. Not played further. Lint reports
`HardcodedDebugMode` (the manifest's `android:debuggable="true"`), not fixed.

## Ultra; frame rate; fixes from the owner's reports (2026-11-09) — verified desktop

Ultra is a fifth "3D effects" level (`effectsLevel=4`, `GALAXY_EFFECTS=4|ultra`), desktop only in
practice. Levels 0-3 stay byte-identical (golden frames after every commit). Each point its commit:

- `e7074660` native resolution: the world into a target of the window's client size
  (`getClientBoundsProperty`), the HUD on the logical surface after it.
- `365af650` `UltraFinish`: RGBA16F (`HdrBlendable`, PreserveContents: a discarded target is XNA
  purple) scene, ACES (Hill, white normalised /0.79, exposure 1.35), auto exposure on the GPU
  (64 -> 1x1 log luminance, two 1x1 targets in turn, clamp 0.85-1.8), HDR bloom chain (6 halves, tent
  up), a 32^3 grade per world (`UltraLogic.hpp` BuildGradeTable from the sky's haze). Every pass
  fetches texels itself (half floats are point-only in XNA's rules).
- `90e1062e` PBR (`SceneShaders` Ultra programs compiled apart: GGX, Schlick, Smith, Karis split-sum),
  the sky's light (`BuildSkyEnvironment`: the background as 128x96 levels + SH9 irradiance at 40 % of
  its hue, made at load, its texture on Ultra's first frame: a texture made at load moved a High
  golden frame), Ultra water/lava, Ray Dream `surface` lines (highlight, shininess, reflection, glow)
  for all 121 object models (`procedural_materials.py`).
- `92a90955` geometry pass (`WorldRenderer::DrawGeometry`, RGBA32F: half floats were too coarse for
  depth): SSAO (12 rays), contact shadows (thin occluders nearer the camera only), SSR on the water.
- `ebfec1d1` voxel GI (`TerrainGi.hpp`: 16 rays a face over the grid, sky + one sunlit bounce of the
  tiles' mean colours, smoothed per corner, min 0.45), baked only at Ultra.
- `d9b5ae06` volumetric height fog with god rays (20 steps against the shadow cascades, half size).
- `afa8b3de` TAA (Halton jitter, depth reprojection, neighbourhood clamp), far DoF on Blupi's
  distance, camera motion blur (60 Hz shutter), grain; captures turn the frame-to-frame parts off
  (`SceneEffects::capture`).
- `9a76df0d` the atlas finer (Catmull-Rom to HiDef's 4096) with normals (Sobel of brightness) and a
  surface map (tile classes, cavity), made on Ultra's first frame (~2 s, logged).
- `6b4a2b62` PN-triangle subdivision of the model parts (`ModelSubdivision.hpp`, creases > 40 deg
  straight), made per part the first time Ultra draws it; the geometry pass uses it too.
- Tests: `VerifyUltra` (grade, ACES, sky, surfaces, GI, maps, subdivision). Debug views:
  `GALAXY_ULTRA_DEBUG=1..9`.

The owner's reports and requests the same day: `fb038986` High water sampled the last bound texture
(ShaderEffect textures bind when set; rebound before the water) and the editor's gizmos/selection with
the effects; `514a9e38` the far cascade every frame in captures (golden frames were timing-dependent);
`75b28c95` a teleporter's top is a floor from above (one-way, feet only); `e3d39b59` the lightning's
zap (channel 69, Decor::BlitzActif); `bf47cf4d` editor axes on the world's bounds; `4d92262a` `fps`,
`78c8a6ef`/`062a05f9` `position` cheats (position and turning axes); `65c8c168` the fade into the
Setup 0.25 s (measured 1.2-1.4 s before, the same in yesterday's build); `a75c642a`/`1c3e60bc` the
Setup's frame rate (60/120/144/240: a 60 Hz tick gate, camera and Blupi interpolated, vsync off above
60; never in captures, demo verification or resume replay). `391194ae` UB-010 marked fixed.

- Checked: web music plays in mission 11 on today's web build (audio output peak 0.42-0.49 in
  headless Chrome); the owner's site lacked `Content/music/` (solved there). The hub has no music in
  the original either (world001 `music=0`).
- Later the same day: `e2525543` Ultra's fog leaves the sky picture alone (a night sky went white);
  `fcc1a627` Tab steps 0.5/0.25/0.125/1 (`GameSpeed::SuperSlow`/`UltraSlow`, `IsBeyondFast`);
  `87078cf2` world054's playthrough round the machine room's column (solid since BD-0080, b19db064;
  world054 and world050 pass); `ccd7906c` above 60 fps every moving object interpolated too (snapshot
  per step, skipped on a jump over two blocks or another object); `c2a243b6` the atlas maps on up to
  eight threads (1.0 s, was 1.8-2.9) and the editor's red over the smoother models. Ultra checked in
  WebGL 2 (headless Chrome): every shader compiles, the maps take 4.1 s there. Loading 105 at Ultra
  takes 0.45 s more than at High (GI bake and programs).
- Open: Ultra not run on Android; the camera lag stays as it is (the owner's decision).

## 3D effects, third round; UpdateTick split (2026-11-09) — verified desktop and web

Owner's request: levels instead of one switch with a measurement, golden frames for the effects,
Ray Dream materials on the models, better shadows (sprites, cascades, softer), UpdateTick split.

- `8e6f15c1` levels (SaveData `effectsLevel`, Setup steps Off/Low/Medium/High, SceneEffects flags
  occlusion/surfaces/smoothing/shadowMap/pointLights/glow; PostProcess::End(glow) FXAA only at
  medium; shadow map 1024, glow at a quarter size). `83977567` cascades (near 44 every frame, far 160
  ahead every third, `m_farReady`), tent PCF 16 samples, sprites cast (upright across the sun, alpha
  cut) and receive (sprite shader, lookup 0.7 toward the sun), round shadows only without the map;
  materials (`procedural_materials.py` -> 19 `materials.txt`; `ProceduralMaterial.hpp`;
  `objectPartMaterials_`; model shader `Pattern()`/`Bumped()` in model space, 21.116 RD units a
  block). `5b063474` effects golden frames (`GOLDEN_UPDATE=1` writes them). `cb03fee7` UpdateTick
  split (UpdateTickScreens/UpdateScreenPlay/UpdateTickWorld/UpdateTickActions/UpdateTickEvents/
  UpdateTickCamera; a block's bare return became `return true` and the caller returns).
- Measured (GALAXY_PERF, Xvfb llvmpipe: relative only), mission 105 draw ms: off 23, low 24,
  medium 25, high 79 (first) -> 51 (smaller map, quarter glow) -> 99 (two cascades) -> 68 (far one
  every third frame).
- Verified: classic golden frames/trace, 1728 autoplay frames, 28 editor frames byte-equal after
  every commit; every level plays the 11 missions; effects golden frames equal over three runs;
  Pause/Setup live; WebGL 2 in headless Chrome at high (every shader compiled). 119/119 CTest.
  Android not run.

## 3D effects, second round; GalaxyEggbertGame split (2026-11-09) — verified desktop and web

Owner's request: FXAA, ambient occlusion, real sun shadows (shadow map), dynamic lights, more
presentation particles, split `GalaxyEggbertGame.cpp`, tests of what is drawn.

- Commits: `5a1b03cd` FXAA (PostProcess: composite into `m_graded`, FXAA pass to the screen);
  `0d8f40d6` `SceneShaders` (terrain: occluded / lit / cutout shaders; skinned model shader with the
  SkinnedEffect's own lights read from it; depth shaders solid / cutout / skinned; RGBA8 packed-depth
  shadow map 2048 or 1024 on GLSL ES, 44 blocks, texel-snapped; 3x3 lookup, a shadow takes 55 % of
  the sun; up to 8 point lights), `TerrainRenderer` built with `ambientOcclusion` (vertex colour,
  PositionNormalColorTexture; rebuilt when the toggle changes: `TerrainOcclusionWanted()`),
  `TerrainShading` for its draws, `DrawShadowCasters`; the frame now built before the shadow pass
  (DrawFrame: BuildSceneFrame, RenderShadowMap, PostProcess::Begin, DrawWorld); `5148bffa` moments as
  particles (`AppendSceneMoments`, `EffectParticles::Burst/Crossed`); `eb0b5350` the split
  (Load/Demo/Missions/Update files, `GameHelpers.hpp`); `3aafdb19` `SceneEffectsLogic.hpp` +
  `VerifySceneEffects`.
- Tuning notes: shadows at 80 % made deep corridors gloomy, 55 % kept; explosion lights merged per
  blast (3 blocks) at 0.85 with the point-light sum capped at 0.9 (8 stacked lights washed blue tiles
  cyan); explo.png 90-99 are green goo splats, not water: the water splash is object-m 99-102 (plouf),
  the bubbles object-m 103-106 (blup); particle sizes are of the whole icon cell (the drawing fills
  part of it).
- Verified: effects off -- golden frames and trace equal, 1728 autoplay frames and 28 editor frames
  byte-equal after every commit; effects on -- the 11 missions and the editor pass, the Setup toggle
  rebuilds the terrain live, `GALAXY_EFFECTS_DEBUG=shadow|occlusion` views checked; the WebGL 2 build
  in headless Chrome compiles every shader (no "3D effects:" line in the console). 119/119 CTest.
  Android not run.

## SceneFrame + WorldRenderer, and the optional 3D effects (2026-11-09) — verified desktop and web

Owner's request: wire `SceneFrame`, wrap the drawing as the renderer (no second renderer), then 3D
effects and particles instead of the effects' billboards; effects off by default (a Setup toggle),
on every platform, particles cut from the original sprites.

- Commits: the frame's drawing moved to `src/GalaxyEggbertCNA/GalaxyEggbertGameDraw.cpp` (DrawWorld,
  DrawOverlay, DrawFrameCaptures; shared helpers in `GameInternal.hpp`); then `SceneFrame` rewritten
  as the draw list, built by `GalaxyEggbertGameScene.cpp` (`BuildSceneFrame`, `AppendScene*`), drawn
  by `src/GalaxyEggbert/Game/WorldRenderer.cpp` from it and `WorldRenderAssets` (the game's loaded
  effects, lent each frame); then the effects.
- Effects: `WorldEffects.cpp` (fog on every world effect and the terrain's cutout pass, the shadow
  pass after the opaque terrain, the particles after the billboards), `SurfaceShaders.cpp` (water and
  lava GLSL, desktop 3.30 / ES 3.00 by `GetShaderDialectEXT()`, passed to
  `TerrainRenderer::DrawOpaque/DrawTransparent`), `PostProcess.cpp` (scene render target, bright
  pass, two blur passes, screen composite with saturation and vignette; HUD after it),
  `EffectParticles.cpp` (Fire 8-11, Splash 14/15/35/98-100, Sparkle 39/41/42/57/58/90-93; fire and
  splashes replace their billboard, sparkles keep it). The builder's `AppendSceneEffects` sets fog
  (start 20, end 140, colour = the background picture's middle rows, `backgroundHaze_`), shadows
  (`GroundBelow`, thrown along `kTerrainLight`) and diverts the effect objects' quads to particles.
  Setup: "3D effects" at x 450 on the Accelerometer row (the sensitivity slider ends at 440).
- The one-shot terrain check (`CheckTerrainVisibility`) skips while the world goes into the render
  target (CNA refuses `GetBackBufferData` with a target bound).
- Verified: effects off -- golden frames and trace equal, 1728 autoplay frames of 11 missions
  (`build-probe/render-refactor/shots.sh`, `compare.sh`; four frames of 055/065 differ between two
  runs of the same binary and are skipped, `flaky.txt`) and 28 editor frames byte-equal to the build
  before; effects on -- the 11 missions and the editor pass, the golden trace and the 065/199
  autoplay traces equal; the Setup toggle clicked live (saved `effects3d=1`); web: `build-cna-webgl2`
  rebuilt, headless Chrome (`build-probe/render-refactor/web_fx.py`) toggled it in the main menu's
  Setup and played, no shader error. `VerifyEffectParticles` (CTest, 118/118). Android not run.
- Next, if wanted: real shadow mapping (a depth pass from the sun and terrain/model shaders that
  read it), shadows on models, a GLES3/Android run.

## Brick wall ends ragged towards the open (2026-11-09, Q-011) — verified on desktop

Asked interactively (the owner had answered Q-010, the cheese, on 2026-11-04 already; only Q-011
was open): option (b), ragged only where a wall ends in the open, whole bricks elsewhere.

- `src/GalaxyEggbert/Game/BrickEdges.hpp` (pure): the four courses (texel rows 0/17/33/48/64) and how
  far each stops short on the tile's left/right side, measured from object-m.png's alpha
  (`kTiles`: 262 left 5/0/8/2, 263 right 0/1/10/3, 283 left 7/3/0/5 and right 5/1/3/0; the game
  measures the sheet it loads again, `MeasureCourseInsets`, and warns if it differs). `ShapeFor()`:
  a clear side is ragged when it borders air, so does a side across it (a wall's face has the wall
  on both sides along it) and, for 262/263, the wall goes on opposite. The first version without
  the "across" condition notched 945 cells of 124, mostly in the middle of walls: the generators
  (`tools/worlds/station.py`, `village.py` `BRICK_FACES`) scatter the pieces in brick masses.
- `TerrainRenderer` `AppendRaggedBrick`: each course a box inset on its ragged sides; side faces
  drawn unless a Whole (non-ragged occluding) neighbour hides them, horizontal steps only where
  exposed (`Subtract`), and where a Whole neighbour's face is hidden by the cell over a ragged end,
  that part drawn from here (caps). Faces across a ragged end wear the edge piece mapped by
  position (the bricks end with the course), the others and the caps the whole brick 261.
- `TileCoverage.hpp` `FillBrickEdgesFromWholeBrick`: the pieces' clear texels take 261's, so the
  whole cubes (and the faces of the ragged ones) show bricks, not the pale mortar-coloured bands the
  mean fill made in the walls of 123/124/121/034.
- Verified: `VerifyTileCoverage` (rules, sections, measuring, the fill); a probe world
  (`build-probe/brick-edges/make_world.py`: walls along X and Z with both ends, a lintel past an
  end, a free column) live, `build-probe/demo-live/bricks-sheet.png`, `bricks-detail.png`,
  `bricks-close-detail.png`; campaign 124, 123, 121, 034, 199 loaded (`bricks-campaign-sheet.png`;
  13/14/14/33/1 ragged cells, vertices as before within 1%); 117/117 CTest; golden trace equal;
  golden frame 0180 refreshed: 12 pixels of one row (a tree trunk far off) changed by 1/255 in
  blue, the atlas's mip levels round the changed tiles (frames 0060/0120 equal, two captures
  identical). Not run on the web (same code path).

## Sharing galaxies and demos, galaxy gates in the menu, xnjttjpo on the web (2026-11-09)

The owner's list after the galaxy work: a whole galaxy in one file, the galaxy's progress in the
menu, .gdemo export/import, xnjttjpo edits kept on the web, `CURRENT.md`'s open work brought in line
with the release audit.

- A galaxy in one file (`CustomWorldStorage` `PackGalaxy`/`UnpackGalaxy`/`ExportGalaxy`/
  `ImportGalaxy`/`ListSharedGalaxies`): `GALAXY_PACK 1`, `world <mission> <size>` + the .vwr bytes
  per place, `END <FNV-1a>`; an unpack refuses a changed byte, a cut, another version, a place a
  galaxy has not, a world that does not load. The galaxy screen's Export writes
  `customworlds/shared/A-galaxy.galaxy` (downloaded on the web); Import lists the `.galaxy` files
  there in place of the help (on the web the file picker first: `RequestUpload(dir, extension)`),
  a row's first click asks, the second makes it the galaxy: the places there go into their backups,
  the gamer's galaxy progress starts over (`SaveData::ResetGalaxyProgress`, via
  `BrowserRequest::galaxyImported`). Tests `VerifyGEEditorStorage`; live
  `harness/case_galaxy_share.py` (`galaxy-share-sheet.png`).
- A galaxy's name (same day): `GalaxyDir()/name.txt` (`GalaxyName`/`SetGalaxyName`, letters, digits,
  single spaces, '-' and '\'', 24 at most), a `name <size>` entry in its pack (`UnpackGalaxy`
  `GalaxyPack::name`, `PeekGalaxyPackName` for the list), the receiver's galaxy takes it on import
  (or none); its export is named after it (`Star-Fields.galaxy`). The galaxy screen's title is
  clicked to name it: typed keys (the host forwards `KeyDownEXT` to `WorldEditor::TypeKey` while
  `WantsText()`; a Shift press is caught as an event for the next letter, as the frame's snapshot
  misses a quick one), Enter or a click elsewhere keeps it, Escape leaves it. The import list and
  the menu's player panel show it ("Star Fields : 0 gates", cut with "..." when long; read again
  on each return to the menu). Typing needs a keyboard (not on a touch-only device). Tests
  `VerifyGEEditorStorage`; live `harness/case_galaxy_name.py` (`galaxy-name-sheet.png`). On the web
  too (`harness/web_names.py`, `web-names-sheet.png`, the rebuilt `build-cna-webgl2`): the menu's
  Demos, the same file picked twice kept as `demo001.gdemo` and `demo001-2.gdemo`, a bin deleting
  one from the browser's storage, the galaxy named "Star Fields" from the keyboard (Shift
  included), its export downloaded as `Star-Fields.galaxy` with its name; no exception.
- The demo positions from the main menu (same day): a Demos button left of Exit (over Galaxy,
  `InputPad` `InitLayout::demos`, `demosPressed`) opens the same screen over the pause screen's
  picture (`UpdateDemoSlotsScreen`/`DrawDemoSlotsScreen`, shared with Pause): Export, Import and
  Original work, a position clicked says a recording is made in a mission (Pause > Record); Escape
  closes it (not the game), and the menu's 40-s demo timer waits while it is open. Live
  `harness/case_menu_demos.py` (`menu-demos-sheet.png`, `menu-demos2-sheet.png`).
- No file replaces another any more (same day, `Game/FreeFileName.hpp`): a second export is
  numbered (`A-galaxy-2.galaxy`, `A-demo-5-2.gdemo`), so are a pack's demos landing among the shared
  demos and, on the web, a picked file whose name is taken (`PlatformFiles.cpp`). Tests
  `VerifyGEEditorStorage`, `VerifyDemoReplay`.
- The galaxy import list (same day): pages of eight with "<" / ">" and "p / n" under them, a bin at
  each row's right end deleting the shared file on the second click in a row (`ImportDeleteRect`;
  on the web the browser's storage is the only place a player could reach it); Export and Import
  on a dark ground of their own over the title scene's logo, both drawn as panels. Tests
  `VerifyGEEditorStorage`; live `harness/case_galaxy_list.py` (`galaxy-list-sheet.png`).
- A galaxy's demos travel with it (2026-11-09): the pack also holds the player's recordings made in
  that galaxy (`GalaxyRecordings()`: positions 1-10 whose world is one of the galaxy's files) as
  `demo <position> <size>` entries; an unpack keeps the galaxy and leaves out a demo this game cannot
  play. An import writes them among the shared demos as `<pack name>-demo-<position>.gdemo`, so no
  position of the receiver's is overwritten: the demo positions' Import lists them (its world is
  found by hash in the receiver's galaxy, `ResolveDemoWorld()`). The galaxy screen says how many
  went out or came in. `DemoReplay.cpp`/`ResumeLog.cpp` joined `GEEditorVerifySupport`. Tests
  `VerifyGEEditorStorage`; live `harness/case_galaxy_demos.py` (`galaxy-demos-sheet.png`).
- The menu's player panel: "Galaxy : N gates" on the Blupi line, right-aligned (smaller when it
  would reach the Blupi count), for a gamer whose galaxy has a main hub or progress
  (`InitGamerInfo::galaxyGates`, `SaveData::GetGalaxyGatesForGamer`). `VerifyGESaveData`; live
  (same sheet).
- Demos shared (`DemoReplay` `SharedDemoDir`/`DemoExportName`/`ExportDemo`/`ImportDemo`/
  `ListSharedDemos`): the demo positions (Pause > Record) have Export and Import beside Cancel.
  Export, then a position: its demo (the player's or the shipped one) is copied to
  `demos/shared/A-demo-N.gdemo` (downloaded on the web). Import (the web's file picker first),
  then a position: the newest shared `.gdemo` (Import again: the next one) becomes it once
  `ParseDemo()` accepts it; the line says whether its world is here. Cancel leaves the mode
  first. A recording made in a galaxy finds its world by hash in any player's galaxy, also under
  another platform's path (`ResolveDemoWorld()`, used by `StartDemoFile()`), so a demo shared
  with its galaxy plays wherever that galaxy was imported. Tests `VerifyDemoReplay`,
  `VerifyGEInputPad`; live `harness/case_demo_share.py` (`demo-share-sheet.png`: export of
  position 1, import of a recording from A's galaxy into position 7 with only B's galaxy holding
  that world, `--verify-demo` equal, 224 ticks). Since the same day Import shows the shared files
  as a list in place of the positions (up to 8 rows: the file, its mission or galaxy place and length,
  or why it cannot be read; `InputPad::DemoListRowRect`, mode 3 in the host): a row chooses the
  file, then a position imports it; Import from a chosen file shows the list again
  (`demo-import-list-sheet.png`). Later the same day: pages of seven with "<" / ">" and "p / n"
  under them, a bin at each row's right end deleting the shared file on the second click in a row
  (any other click disarms it), and the rows on the panel's own calm picture instead of a stretched
  button (`InputPad::DemoListDeleteRect`/`DemoListPrevRect`/`DemoListNextRect`; the host's
  `demoImportPage_`, `demoDeleteArmed_`). `VerifyGEInputPad`; live `harness/case_demo_list.py`
  (`demo-list-sheet.png`: 9 files on two pages, one deleted, one imported from page 2). 117/117
  CTest, golden frames equal.
- xnjttjpo on the web: the shipped worlds there are the page's packed data, in memory, so a save
  in the xnjttjpo session is also copied into `/save/customworlds/campaign-edits/` (the editor's
  `BeginOutsideSession(backupDir, keepCopyIn)`, given only under `__EMSCRIPTEN__` for a campaign
  world) and `main.cpp` lays those copies over `worlds3d/` before anything loads
  (`ApplyCampaignEdits()`, worldNNN.vwr names only). The desktop is unchanged (it edits the files
  themselves). Undone from Pause (same day): "Original", left of Record, while the campaign world
  played has a kept edit (`HasCampaignEdit()`; never on the desktop, which keeps none); the first
  click arms it ("Sure?"), the second `RevertCampaignEdit()`s and restarts the mission. The world as
  shipped is kept for the page's life in `worlds3d-shipped/` (`CampaignOriginalsDir()`, memory only):
  by `ApplyCampaignEdits()` before it lays an edit over it, and by `OpenMissionInEditor()` before
  the first save. Tests `VerifyGEWorldEditor`, `VerifyGEEditorStorage`, `VerifyGEInputPad`; web
  `harness/web_revert.py` (`web-revert-sheet.png`): undone right after an edit, and after a reload
  that laid the edit over the shipped world; another reload lays nothing. On the desktop too (same
  day): every xnjttjpo save of a campaign world is also kept in `customworlds/campaign-edits/`, the
  world as shipped in `customworlds/campaign-originals/` at the first visit (`KeepCampaignOriginal`
  renews it when the shipped file changed since, e.g. regenerated); `HasCampaignEdit()` is "the kept
  edit and the file are the same bytes", so a regenerated or updated world shows no Original. Live
  `harness/case_original_desktop.py` (`original-desktop-sheet.png`). Galaxy's own notices (this one,
  a recording's) are drawn under the training hint's banner while it shows (`DrawActiveCheats(...,
  top)`, under the cheats beside it too); the cheats keep the original's place
  (`notice-under-hint.png`).
- Web runtime of all of it (`harness/web_share.py`, the rebuilt `build-cna-webgl2`): the desktop's
  `A-galaxy.galaxy` imported through the file picker (CDP `Page.setInterceptFileChooserDialog`,
  `DOM.setFileInputFiles`), played, and exported again as a download byte for byte equal; the
  desktop's galaxy recording imported into position 7 ("plays ... now": its world found under the
  web's `/save/...` path) and exported again equal; xnjttjpo + Right + Enter changed
  `worlds3d/world001.vwr` and kept the copy, and after a page reload the edited world was laid over
  the shipped one again ("1 shipped world(s) as edited with xnjttjpo"), the imported demo still
  there; no exception. With that profile (`web_demo7.py`) the Demo cycle played demos 1-6 and then
  position 7, the desktop's galaxy recording, all seven "finished" (the desktop recording replays
  equal in wasm from the web's galaxy path). 117/117 CTest, golden frames equal. The Chrome
  profile (`build-probe/demo-live/web/chrome-profile`) was deleted afterwards.

## Galaxy: delete and Continue, web runtime (2026-11-09, BD-0119) — verified on desktop and web

The owner kept Q-017's three choices and asked for: deleting galaxy worlds, Continue (resume) in a
galaxy, a web runtime check, commits (`17435a54` the earlier work, `ef7e16a8` delete, `78e772ce`
Continue).

- Delete: `GalaxyBrowserScreen` puts the world screen's bin on each place that exists (main hub,
  199, the selected world's hub and missions): the first click arms it (tick, "Delete?"), any other
  click disarms, a second click deletes into `galaxy/backups/` (`DeleteSlotWorld()`). Backups
  (left of Back) switches to restoring: a click brings the place's newest backup back
  (`RestoreBackup()`, the line under the places says what happened); the bins are hidden then.
- Continue: `StartResumeLog()` records the resume log in a galaxy too; `BeginResume()` recognises a
  galaxy file in the log (`ParseGalaxyWorldPath()`), reads it from there and sets `galaxyGamer_`
  and the galaxy mode before `LoadMission()`. A campaign log clears both. One `resume.txt` as before.
- Web runtime (2026-11-09, the rebuilt `build-cna-webgl2` in headless Chrome over DevTools:
  `build-probe/demo-live/harness/web_live.py`, `web_case.py`, `web_demo5.py`; evidence
  `build-probe/demo-live/web-sheet.png`, `web/console.log`): the page loads (93/93 sounds, menu
  with Galaxy and Demo); Demo plays demo 1 to its end "finished" (the desktop recording replays
  equal in wasm) and demo 2 follows, Escape stops; Galaxy with no main hub -> the galaxy screen,
  the main hub made and edited, then played from `/save/customworlds/gamer0/galaxy/` (IDBFS);
  Pause > Record > position 5 in the galaxy writes `/save/demos/demo005.gdemo`; Home leaves the
  galaxy; Play + typed xnjttjpo opens the editor on mission 1 and Stop goes back; after a page
  reload the galaxy still plays and position 5 shows "Recorded, Galaxy 1"; with that profile the Demo cycle played demos 1-4 and then the player's
  recording from `/save/demos/demo005.gdemo`, all five "finished" (`web_demo5.py`). No exception on
  the page. On the web xnjttjpo saves the shipped world only in the session's memory (the packed
  `worlds3d/` is not persistent; its backup in `/save/customworlds/campaign-backups/` is), so an
  edit lasts until the page is reloaded. Headless Chrome: `DISPLAY` unset, a fresh
  `--user-data-dir` (deleted afterwards, 291 MB).
- Verified: `VerifyGEEditorStorage` (arm/disarm/delete, no bin on a place not made, Backups,
  "no backup"), 117/117 CTest; live `harness/case_galaxy_delete.py` (`galaxy-delete-sheet.png`)
  and `case_galaxy_resume.py` (focus lost on the galaxy's main hub -> resume.txt names
  `customworlds/gamer0/galaxy/world001.vwr` -> restart -> Continue -> the main hub from that file,
  its world sign -> hub 10 of the galaxy, progress in `galaxy0.*`, the campaign untouched);
  `GALAXY_RESUME_SELFTEST=60` and `=200` in `--galaxy --mission 11` autoplay: equal, mission
  completed into hub 10 (`galaxy-resume-selftest/`); the campaign's `case_resume.py` still passes.

## Recordings, ten demos, xnjttjpo, galaxies (2026-11-08, BD-0116-0119) — verified on desktop, committed 2026-11-09

Owner request after the main-menu demos: find how Speedy Blupi records demos in the game by a trick
and offer something more standard (a world played into a demo position), ten demos, editing the
shipped worlds in the 3D editor by a cheat typed in play ("xnjttjpo as in Speedy Blupi"), and a
world system of hubs and worlds for each of the three players ("Space, or something else":
**Galaxy**). Committed with the main-menu demos on 2026-11-09, after the owner kept the three
choices of Q-017 (the name Galaxy, xnjttjpo saving the shipped file, a pause ending a recording).

- Research: `plans/audit/speedy_blupi_demos.md`. The trick: XMISSION, typed `xnjttjpo` (each letter
  after the first one more), makes the Construction screens edit the official worlds up to 219;
  playing world 200-219 records automatically ("REC"); leaving Play writes `data\demo2NN.blp`.
- Recording (BD-0116): Pause shows a Record button (top right, only in a mission of the campaign
  or a galaxy: not in a play-test, a demo, a resume reconstruction or an autoplay). It opens the ten
  demo positions (`InputPad::DrawDemoSlots`, the pause buttons hidden under it): Shipped/Recorded
  and the mission of each, Original under a recorded one (deletes it), Cancel or Escape. A position
  chosen restarts the mission from its start (`StartPlayerRecording()`), normal speed, "REC" top
  left; the recording uses the developer path (`BeginDemoRecording()`/`FinishDemoRecording()`, now
  returning whether it wrote) with `demoRecordSlot_` 1-10. Leaving Play (pause, the lost screen)
  or the mission (a sign, the exit), 10 minutes, or closing the window end it. `LoadMission()` is held back
  (`deferredMission_`) until the file is written, so the recording ends where its replay will, then
  the mission loads (`EndPlayerRecording()`; a four-second notice). Progress is saved as in normal
  play. Files: `demos/demo%03d.gdemo` beside savedata.txt (`/save/demos` on the web;
  `UserDemoPath()`). `StartDemo(n)` plays the player's file of position n, the shipped one when it
  is missing or unplayable. A galaxy recording names its galaxy world (`start.worldPath`):
  `StartDemoFile()` accepts that path for the same mission and plays it in that galaxy
  (`ParseGalaxyWorldPath()`, only gamers 0-2).
- Ten demos (BD-0117): `Content/demos/demo004-010` added from the QA autoplay scripts:
  `tools/record_demo.sh build-cna-opengl3 Content/demos/demo004.gdemo 23 --autoplay
  tools/autoplay/world023.txt --demo-ticks 2700 --demo-title "World 2: the tower of keys"`; the
  others the same with 005/33 "World 3: the haunted house of the eight wasps", 006/55 "World 5:
  the dark works", 007/65 "World 6: the slime catacombs", 008/81 "World 8: the sky steps", 009/112
  "World 11: the machine cavern", 010/123 "World 12: the library" (all 2700 ticks; demo001-003 as
  below). The game plays positions 1-10 (`kDemoSlotCount`); `VerifyDemoFiles` wants exactly ten.
- xnjttjpo (BD-0118): `InputPad::TypedCheat::XMission` (word table, Help lists it). Typed in Play
  it sets `xmissionPending_`; after that tick `OpenMissionInEditor()` opens `MissionPath(mission)`
  in the editor with `WorldEditor::BeginOutsideSession(backup dir)`: Stop raises
  `ConsumeLeaveRequested()` instead of the browser (unsaved changes still need a second Stop),
  saves back up into `customworlds/campaign-backups/` (`BackUpWorldTo()`, three generations, the
  first save of each visit) and take no thumbnail. `LeaveMissionEditor()` reloads the mission from
  its start with the lives it had. The save writes the shipped `worlds3d/worldNNN.vwr` itself, as
  XMISSION wrote `data\worldNNN.blp`: in a checkout that is the repository's file (tools/worlds/
  generators would overwrite hand edits; a demo or autoplay of that world then needs re-recording).
  Ignored in a demo, resume reconstruction, recording or play-test. Restore by hand from the backups.
- Galaxies (BD-0119): `CustomWorldStorage.hpp` (`GalaxyDir()`, `GalaxyWorldPath()`,
  `IsGalaxyMission()`, `GalaxyMissionName()`, `CreateGalaxyStarterWorld()`): main hub 1, world hubs
  10-140, missions W1-W8, bonus 199 in `customworlds/gamerN/galaxy/`. Starters: the 3x3 board,
  Blupi at (50,1,51) facing -Z with the sign two cells ahead (world sign 1 / mission sign 1), a world
  hub's return sign beside him; missions are the editor's starter (chest and exit); every starter
  stores its mission number. Editor: the world screen's Galaxy button -> `GalaxyBrowserScreen`
  (main hub, 199, worlds 1-14 select, the selected world's hub and missions 1-8; faint = not made,
  a click makes it from its starter); Back/Escape -> the world screen (`SyncBackKey()` keeps one
  Escape from also leaving it). Play: the menu's Galaxy button (`StartGalaxy()`; with no main hub
  it opens the editor's galaxy screen) sets `galaxyGamer_` and `SaveData::SetGalaxyMode(true)`:
  every progress accessor then uses the gamer's galaxy slot (`galaxyN.*` in savedata.txt, written
  only once used; the mode is not saved; Reset keeps it). `MissionPath()` reads the galaxy's files,
  `LoadMission()` overrides the world's mission number with its place, main-hub sign 13 is world
  130 (`ComputeWorldSelectTarget(..., galaxy)`), the resume file as in the campaign (since
  2026-11-09, below). The menu (`SetPhase(Init)`) leaves
  the galaxy. A galaxy world's editor play-test follows its signs within the galaxy. QA:
  `--galaxy --mission N [--autoplay s]` plays the selected player's galaxy.
- Tests: `VerifyGEInputPad` (Galaxy and Record buttons, the positions screen, xnjttjpo, 30 words),
  `VerifyGESaveData` (galaxy progress apart, file unchanged when unused, round trip, Reset),
  `VerifyGEEditorStorage` (galaxy paths and places, parsing, starters, BackUpWorldTo, the galaxy
  screen through WorldEditor incl. Escape), `VerifyGEWorldEditor` (outside session: Stop, second
  Stop, backups elsewhere, no thumbnail, normal again after), `VerifyInteractionSystem` (sign 13),
  `VerifyDemoReplay` (user paths, ten shipped). Full CTest 117/117; `tools/verify_demos.sh`: all ten
  equal, player files unchanged; golden frames and trace identical to tests/golden.
- Live desktop evidence (`build-probe/demo-live/`, harness copies in `harness/`): `xmission/`:
  xnjttjpo in mission 11 opens the editor, Right+Enter saves the edit over the (copied) worlds3d
  file with the old version in campaign-backups, nothing else beside it, Stop reloads mission 11
  with the new background, again without edits one Stop, the repository's file untouched.
  `record/`: Pause -> Record -> position 4 -> mission 11 restarts with REC -> Escape writes
  `demos/demo004.gdemo` (notice shown) -> `--verify-demo` equal; the menu's Demo plays it as
  position 4; Original removes it; Cancel/Escape record nothing. `galaxy/`: Galaxy with no main hub
  opens the galaxy screen; main hub, hub 10 and mission 11 made there; Galaxy then plays the main
  hub from the player's folder, sign 1 -> hub 10; a recording there (position 5) ends at the mission
  sign, written before mission 11 loads, and replays equal; Home leaves the galaxy; savedata has
  `galaxy0.*`, the campaign's untouched; `--galaxy --mission 11` autoplay takes the chest and exit,
  back in hub 10 with door 12 open in `galaxy0.doorsUnlocked` only. `cycle10/`: the menu's Demo
  plays the seven new demos in turn (positions 1-3 left empty for the check), each drawn
  (`cycle10-sheet.png`, 20 s into each), 44.9-45.5 s apiece, each "finished" as recorded, the
  cycle wrapping after demo 10. The harness's first "none skipped" check also counted the
  intentionally empty positions 1-3. The corrected check, run on the same log, passes.
- The web build (`build-cna-webgl2`) compiles and links with all ten demos packed; its runtime was
  checked on 2026-11-09 (the section above). Not run: Android, a physical gamepad or touch screen.
  (Deleting galaxy places and Continue in a galaxy came on 2026-11-09, the section above.)
- While this was built, the CNA working tree (another session's uncommitted edit) briefly crashed
  every start (UB-010 in upstream_bugs.md); it was fixed there at 22:32 and Galaxy rebuilt. The
  verified binary includes CNA's then-uncommitted graphics edits.

## Main-menu demos (2026-11-08, BD-0115) — implemented and verified on desktop, committed 2026-11-09

Owner request: Speedy Blupi's automatic demos (reference `../blupi_recon/src/event.cpp:3666-3840`
DemoRecStart/DemoPlayStart/DemoPlayStop/DemoStep, TreatEvent `:4074-4090`, sb1 `port/games/sb1/
game/main.cpp:200-220`). Committed on 2026-11-09 with the recordings (the owner first asked not
to commit, then asked for it).

- What it does: the main menu's Demo button (under Exit) and 40 s without input in the settled
  main menu of the active window both call `StartNextDemo()` from demo 1; the demos play in turn
  and the first follows the last. A demo is a recording replayed tick by tick into the normal
  `UpdateTick()` at the normal 60 Hz pace, drawn and heard (music and effects), with "Demo" in the
  top-left panel (TX_DEMOPLAY). A new key (native key events catch taps shorter than an update),
  mouse button, wheel, gamepad button/stick or touch stops it (`DemoNewPress`); mouse movement
  alone does not; releases do not. The stopping input is latched until everything is released,
  so the Escape/Enter/click that stopped it never exits, plays or presses a menu button.
  Deactivation stops it. Loading another mission or leaving Play inside a demo ends that demo
  instead (`LoadMission()`/`SetPhase()` hooks), so a desync can never wander into the campaign.
- Modes kept apart: `demoActive_` (visible demo) vs `replayingResume_` (fast silent resume
  reconstruction) vs `autoplay_` (QA). Persistence: `SavePlayerData()` replaces every
  `if (!replayingResume_) saveData_.Save()` and is a no-op during a demo; `DeleteResumeFile()`
  is a no-op during a demo; `OnDeactivated()` writes/deletes nothing then; `ToggleFullscreen()`
  and the accelerometer are off. `StartDemo()` sets aside the SaveData profile, camera mode,
  speed, cheat flags/lock/list and lives; the demo plays with the recording's profile (doors,
  last world) but the player's own sound, volumes, terrain relief and fullscreen; `StopDemo()`
  restores all of it, reloads the title scene at once and resets the idle timer.
- Files (ten since BD-0117, above): `Content/demos/demo001.gdemo` (mission 011: pillars, mine pits, lifts, 2700 ticks),
  `demo002` (074: skateboard over mines and lava, 2700), `demo003` (105: helicopter over the
  cavern, 3000). Format `DemoReplay.hpp`: header `GALAXY_DEMO 1`, `kDemoSimulationRevision`
  (1), title, recording build (information only), the end-state signature (hashed
  `ResumeSignature()` entries), then the resume log's body (`ResumeLog::WriteBody/ReadBody`,
  refactored out unchanged) and the FNV footer. Resume files still need the identical build;
  demos need the same simulation revision and their world's exact bytes. Damaged, other-format,
  other-revision, changed-world, missing-world or too-long demos are skipped with a log line; with
  none playable the menu stays. Web preloads `Content/demos`; Android ships all of `Content/`.
- Recording (developer only): `tools/record_demo.sh build-cna-opengl3 Content/demos/demo001.gdemo
  11 --autoplay tools/autoplay/world011.txt --demo-ticks 2700 --demo-title "World 1: pillars,
  mine pits and lifts"` (scripted, Xvfb, seconds); demo002: mission 74, `world074.txt`, 2700,
  "World 7: skateboard over the mines and lava"; demo003: mission 105, `world105.txt`, 3000,
  "World 10: helicopter over the cavern". By hand: omit `--autoplay` and play; Escape (or closing
  the window) ends the recording; F-keys and Tab are left out. The game flags are
  `--record-demo <file> --mission N [--autoplay s] [--demo-ticks N] [--demo-title t]` and
  `--verify-demo <file>`.
- Verification: `tools/verify_demos.sh build-cna-opengl3` replays each demo undrawn and checks
  `DEMO VERIFY ...: equal` plus untouched stand-in savedata/resume files (CTest
  `VerifyDemoReplays` runs it when xvfb-run exists); `VerifyDemoFiles` checks every shipped demo
  against the revision and current world bytes; `VerifyDemoReplay` covers format, compatibility,
  idle timer, stop input and order; `VerifyGEInputPad` the button. After a world change the
  affected demo is refused (re-record it); after a simulation change verify_demos reports
  "different": bump `kDemoSimulationRevision` and re-record all ten (commands here and above).
- Live desktop evidence (`build-probe/demo-live/`: screenshots, timestamped game logs, the
  throwaway Python harness in `harness/`): the button; the idle start 40.02 s after the last mouse
  move (70 s after an earlier one, so activity restarts the count) and again 40.5 s after a stop
  (the stopping key was held 0.5 s); mouse movement ignored; Escape, Enter, a left click held over
  Play, a right click, the wheel, a 1 ms key tap and Space each stop it with nothing else
  activated (the game did not exit or play); no demo in Setup, the editor, Play or Pause after
  46 s; demos 1-3 end exactly as recorded both in the 800x480 window they were recorded in and
  in 1280x720 fullscreen (bare Xvfb cannot go fullscreen: `miniwm/`, a 90-line EWMH test window
  manager, makes SDL's switch work there); 2700 ticks took 44.9 s; demo 1 after demo 3 ends
  identically again; damaged, other-revision, changed-world and non-demo files are skipped and
  the cycle wraps to the valid one; with none, or no `Content/demos/`, the button and timer keep
  the menu; sound through SDL's disk driver (menu RMS 0, demo 99.5 % non-zero samples); another
  window taking the focus stops a demo; savedata.txt unchanged and no resume.txt; then Play, a
  real resume.txt on deactivation and, after a restart, Continue replaying it (the debug panel
  shows the same mission, position and yaw as before). Negative control: a demo with one input
  changed, or its stored signature changed, is reported "different" by verify_demos.sh (exit 1).
  Golden trace and the three golden frames equal tests/golden. The web build
  (`build-cna-webgl2`) compiles and links, its package holding the three demos. Not run: Android
  (not built) and web runtime,
  a physical gamepad or touch screen (their stop/activity logic is unit-tested).

## Latest input/editor/UI work (2026-11-08)

- Ghost/lava depth fixed (BD-0114): lava's recovered height-field mesh samples the tile's solid
  body, but still drew in DrawTransparent after ghost/Hide Blupi, which does not write depth.
  Move its cached frames into DrawOpaque; geometry, timing, collision and ghost alpha unchanged.
  `python3 tools/verify_lava_depth.py build-cna-opengl3` renders two isolated fixture worlds
  under Xvfb: background lava leaves the ghost's upper body visible, foreground lava covers it.
  The old binary fails (0 yellow body pixels), the fix passes (2236 behind,0 in front).
  Build and114/114 CTest pass; normal-render captures inspected. The three ordinary golden
  frames are pixel-identical old/new, although both disagree with stored references in this
  environment; do not refresh them for this fix. Evidence: build-probe/ghost-lava/ (before,
  after,foreground,verified,old-verified and golden/before-build). User saves untouched;
  no sibling edits or push.
- Start selection/rotation/opacity complete (BD-0113): start markers were outside the
  shared edit-target/G picker, fixed to yaw0 and drawn at .85 alpha. They now have a PlayerStart
  target carrying colour, persistent G selection, NumPad/cursor targeting and the usual red
  wire/model overlay. O/Tools Turn stores independent quarterTurns through spawn history;
  positioning preserves turns, removing clears only that colour's heading. Shared-cell choices
  use the selected colour; object mode still selects an overlapping MoveObject. Metadata8
  packs four two-bit headings, absent =0, without changing headers or position metadata7.
  The host reads yellow yaw on level entry (Mobile Decor.cpp m_blupiStartDir; SB2 PlayPrepare).
  Base models are alpha1/Opaque/DepthDefault; only selection uses the red overlay. Selected
  colour draws last if starts overlap. All114 CTest pass, including four shared starts plus
  a treasure, wrap/Undo/Redo/reload and G selection after flying away. Native captures select
  and turn all four, undo orange, reload their independent headings; no player files edited.
  Native Play also faces the yellow start's authored west direction (the debug capture after
  typing debug shows274.5deg). Evidence: build-probe/player-start-selection/. Previous
  translucency notes are superseded.
- Coloured starts corrected after owner clarification (BD-0112). The earlier rank0 alias
  followed original custom-world behaviour but was rejected: owner explicitly chose four
  independent coloured starts, with yellow used in normal play. PlayerStart.hpp keeps yellow
  in its existing header, stores orange/blue/green as metadata7 bit masks, allows shared cells
  and relocates/removes only the selected colour. Spawn history records colour; matching-colour
  Delete/Tools Remove at the preview works with Undo. Palette records now use ids1/2/3/4.
  Editor renders every start using the original Stop mesh, recolouring yellow materials from
  existing button96-98 palette colours; whites/eyes/other materials preserved, variants cached.
  Status lists each colour's XYZ. Tests cover each input selection/move/remove/history/save,
  overlap, invalid destinations, independent yellow and recolour. All114 CTest pass. Native own
  profile: yellow46,1,28; orange50->57 with Undo/delete/restore; blue53; green56. Reload retains
  four correctly coloured markers, Play/debug starts yellow46,1,28. Evidence:
  build-probe/player-start-colors/. No sibling edits or push. Earlier BD-0110 shared-start
  claims below are superseded by this correction.
- Fish/creature endpoint cubes complete (BD-0111): LiftEndpoint* generalized/renamed to
  ObjectPath*. The old filter admitted lifts only. Registry patrolTicksPerCell also admits
  all eight reference creatures/moving mines/fish; optional patrolMotion objects show their
  explicitly differing endpoints. Special fish/mine motion does not set patrolMotion, so that
  flag alone was insufficient. Same-cell endpoints draw one cube, ordinary stationary mines,
  treasure and chasing hazards stay unmarked. Existing depth testing and double-sided faces
  retained. Native fish first/second placement and moving-mine XYZ route shots, saved-file
  inspection and all 114 CTest pass. Evidence: build-probe/editor-fixes/. This editor batch
  (single exit, coloured player starts, route cubes) is complete; no sibling edits or push.
- Initial coloured-start fix (BD-0110; shared-start mapping superseded by BD-0112): characters slots1-4 now select the
  existing world-header spawn tool instead of personal mines200-203. PlaceDecor10 shares
  rank0 among all four colours in custom worlds, so no multiplayer mechanic or mine-record
  migration was added. Selected-button highlight stays singular. Slots5/6/7 also corrected
  to posts182/signs158/final key21. Editor draws the original Stop model translucently at
  the saved spawn and shows its XYZ; suppresses the stale former play character. All four
  buttons, save/reload and Undo/Redo tested. Native own-profile snapshots and saved-file
  inspection confirm no mines, unique spawn51,1,32 and Play from that start; 114/114 pass.
  Evidence: build-probe/editor-fixes/.
- Single editor exit complete (BD-0109): placing arrow7 or final key21 removes the old
  exit, including clipboard stamps whose old exit is outside the box. The operation is one
  history entry, preserves overwritten target objects on Undo and refreshes stale G selection.
  World data APIs and existing campaign contents are unchanged. Source: blupi_recon
  PlaceDecor10. Native arrow relocation/Undo/Redo/save and all 114 CTest tests pass. Evidence:
  build-probe/editor-fixes/ (own profile, no player files edited).
- Owner corrected the saw appearance: keep only the upper half, cut edge at cell bottom.
  Done: saw_upper_half.py poses the existing source mesh at .05/.25/.3, clips at centreY,
  caps the base and bakes three exclusive poses. Canonical export_objects runs it after export;
  --check uses stdlib only, is idempotent and verifies every pose/cap/activation. Texture and
  icon IDs/times are unchanged. Runtime draws one pose, shifts its local base0 to cell bottom,
  and crops fallback UVs. Contact tests only the upper semicircle; hidden lower/upper-clear
  spaces are safe. Native front/below checks,114/114 CTest and all15 saw missions pass.
  Full campaign retains its pre-existing world021 egg timeout26658. Evidence: build-probe/saw-half/.
- Latest debug-layout report complete: replaced text-measured width/right alignment with a
  fixed316-reference-pixel panel. Labels and values have fixed columns; XYZ/velocity each get
  three stable positions, paired fields/time ticks have fixed secondary columns, numbers use
  fixed glyph advance. Ground/walking/heli screenshots keep the same bounds; the touch panel
  stays centred and clear of controls. Native toggle script passes325 ticks; touch capture
  checked. Evidence: build-probe/debug-layout/. All current owner requests in this batch are
  committed separately;114/114 CTest and all15 saw missions pass. The unchanged full-campaign
  world021 egg timeout is recorded below. No sibling changes in this batch, main cheats ON.
- Coordinate follow-up complete: NumPad/offset controls hold the aim anchor rather than
  raycasting again after each camera move or newly written surface. NumPad5/Tools Reset returns
  to pointer aiming. Same-cell selection/Place draws only the red outline, avoiding overlapping
  cyan/red borders. Delete handles the whole extended cuboid, with one Undo. Pure coordinate
  stability/camera/region tests and native anchored-shots checks pass. All114 CTest pass.
  Saw mission checks also pass15/15; campaigncheck and watercheck pass, stage3 --rides has every
  target reachable (stage1/2 retain their documented locked sections).
- Standalone saw request is complete: generated floors/material heuristic and solid saw masks
  removed. Decor/scie.rd4 is bottom-aligned by+.425 (measured Y[-.499,.433]); disk contact uses
  the same offset, including body/swept fall and rotation. Stopped blades stay harmless.
  v2 flag bit0 distinguishes independent cells. Runtime upgrades flag0 files in memory,
  preserving explicit floor, rotation and type6 switch-row offset. Low-level load/save keeps
  old bytes; saving an upgraded editor world writes flag1. No player files edited. Clipboard
  copy/paste has an undoable metadata edit for type6. World062's saws were already authored
  above floor20 at walking21: its one-byte flag change makes that explicit, avoiding a raised
  cube under the low ceiling. Regeneration matches every byte. Historical builders opt into
  flag0; future worldkit worlds default independent. All15 affected scripts pass (199:40259),
  plus114/114 CTest and normal-render native probes over air/brick/panel. A temporary GPU-draw
  stub accelerates simulation scripts only. Full campaign hits the same world021 egg timeout
  at26658 as the preserved pre-change binary, a pre-existing verification limit. Evidence:
  build-probe/saw-independent/. Older support-cube notes below are superseded by this entry.
- Owner's14-43-15 screenshot showed the old ray-hit selection and the adjacent Place preview
  both red. These are now distinct: selected cell red wire/tint, Place cyan wire. Status shows
  the selected XYZ. V, O and all removal routes now use CurrentEditTarget; Delete works on
  the NumPad coordinate, object or BigDecor regardless of palette mode. Back/Delete retain
  short native presses too. Undo cases cover all three kinds. Shift-alone descends (Z stays),
  Shift+NumPad/held selection and device selecting mode never descend. Build,113/113 CTest
  and native Delete/Undo/Shift capture pass in build-probe/editor-turn/selection-*.
  The subsequent standalone-saw request is completed in the newer entry above.
- New owner-requested debug cheat:29-word catalogue, active-name toggle and read-only diagnostics
  derived from current world/controller/input data. Mobile MODERN field set adapted to raw XYZ,
  measured simulation-step velocity and the real20Hz clock. Desktop panel upper-right, touch
  upper-centre (the first right-side trial overlapped Action; fixed and captured). fairplay
  clears/blocks it; compile-OFF suppresses parser/render/action. Resume format7 stores its initial
  flag and signature includes flag/velocity. Pure formatter/29-word/lock/round-trip tests pass;
  live walking/heli/toggle/fairplay/help checks and moving resume SELFTEST equal pass. A separate
  OFF verifier reuses existing libraries without changing the main ON cache. Native build and
  all113 CTest pass. Evidence: build-probe/debug-cheat/. No sibling changes for this task.
- Editor V now consumes native non-repeat presses rather than losing complete down/up pairs
  between updates. Each queued press has its own release edge and Undo command; holding V
  does not duplicate it. All multi-variant terrain families and auto-tile materials are covered.
  Native before/after: three fast presses in one event batch changed no cells before, and the
  aimed saved voxel from1 to13 after (1→11→12→13). Build and all113 CTest pass.
  Evidence: build-probe/editor-v/. No sibling changes; reuses the published CNA input event.
  Specific V examples were requested but not supplied; the native input-loss repro is fixed.
- O/Tools Turn now shares an explicit target with red selection: offset NumPad coordinate first
  (empty means no fallback to the old ray hit), then G selection, preview object/decor, aimed
  terrain. Short V/O native commands are ordered, with release edges. Single-faced cubes such
  as house window190 now store four object turns; TerrainRenderer applies them to the chosen
  texturedSide before both flat and relief geometry. Previously the saved plate bit did nothing.
  Selected models get an exact posed-mesh35% red overlay with depth testing; target cells have
  a matching translucent red box, replacing the old magenta G box. Native tree/window and
  NumPad→O→save verified, plus113/113 CTest. First tint trial tried LightingEnabled=false,
  which SkinnedEffect rejects; corrected to an emissive overlay and repeated live successfully.
  Evidence: build-probe/editor-turn/. The subsequent standalone-saw request is completed above.
- Publication follow-up (owner: commit/push everything): both CNA changes were pushed to
  origin/next at594eb28dc before publishing Galaxy. The standard ci_pin_siblings.py script
  advances only CNA to that published revision; all other verified sibling pins are unchanged.
  CURRENT's former unpublished-dependency limitation is removed. Code verification remains the
  completed native build,113/113 CTest, five input-suite repeats and15 lifecycle GPU cases;
  publication changes only the dependency pin and this documentation.
- Owner screenshot13-22-15's large green forms are BigDecor66-69 (columns/vase); their registry
  entries explicitly remained passable. They now join kSolidBigDecorIcons, so existing recovered
  model-fit boxes become active in campaign and custom/editor play worlds. No generic full voxel
  substitute was added. levelcheck reads the registry instead of duplicating its list/height.
  A synthetic metadata/shape test covers all4 at full height; live probe786's walk changed from
  z37.91 (through the column) to z46.91 (stopped before it). Full world199 reaches Win at40139
  and its script passes at40259. Rebuilt all native targets;113/113 CTest pass. Evidence:
  build-probe/green-decor/. Small plants23-25 were already solid;203 remains its existing type.
  Static levelcheck199 --rides reports unreachable targets identically before/after this change
  (baseline receipt in the same directory); it is not a new collision failure. The full live
  autoplay is the completion witness for this vehicle/teleporter-heavy world.
- UB-009 is now fixed with explicit owner approval in CNAee3235984 (included in current
  CNA594eb28dc): EasyGL retains/replays higher authored raw mip bytes and existing DXT levels,
  including compressed base0. The initial analysis-only instruction was superseded by that
  approval. A new real-GPU textureLod test failed on levels1-3 after each native loss before the
  fix; all15 shader-lifecycle tests now pass, including Color partial-update preservation and
  DXT1 chains across two recoveries. Native Galaxy before/after F9 retains textured floors.
  Evidence: build-probe/ub009/. No game workaround or generated terrain mips were introduced.
- Latest givecopter follow-up: the owner also reproduced intermittent failures in palace hub020,
  sometimes relieved by turning. Their process was on Wayland and used the newly built binary;
  the early text-only fix and then a tested snapshot fallback were insufficient. A fixture proved
  that SDL text mode can be active without delivered letters, swallowing even slow cheats; the
  fallback still has the old between-frame limitations. The owner explicitly approved a CNA
  optional KeyDownEXT event after discussing XNA compatibility/other games. CNA594eb28dc emits
  ordered non-repeat physical presses without changing GetState; its full input suite passed
  five shuffled iterations (654 each:653pass,1harness-default skip). Galaxy now queues those
  events for cheats on all devices, with no TextInputEXT start/stop and no second snapshot feed.
  Autoplay retains snapshots and resume format6 records event letters. A palace-hub live test
  at180ms frames,25ms presses and dropped text events activates givecopter without rotating.
  Evidence: build-probe/cheat-source/. The exact live owner's failure was not instrumented, so
  turning's causal role was not established; input loss/IME dependence is removed at its source.
  Source needs CNA594eb28dc, now published on origin/next and pinned by Galaxy CI.
  Earlier text-input evidence below is historical.
- Reproduced the latest givecopter failure using a read-only copy of gamerC's custom_002.vwr:
  Editor → second slot → Play accepted150ms presses, but lost25ms presses. Snapshot-only parsing
  misses down/up between frames and can alphabetize multiple simultaneous fresh keys. Non-touch
  Play now uses CNA TextInputEXT events queued in arrival order, one letter/tick; physical held
  controls remain independent. Touch/autoplay retain snapshot input, and menus/help/focus loss
  clear pending text. All28 words, fairplay and snapshot de-duplication are tested. Resume format6
  stores each tick's event letter/mode; a live rapid-givecopter reconstruction reports equal.
  Before/after and source hash: build-probe/editor-copter/ and build-probe/latest-controls/.
  Player world/save data was not edited. Final native build and all113 CTest pass, cheats ON.
- Vehicle motor lifecycle: SetPhase now stops all8 vehicle start/run/stop channels on leaving
  Play and clears activeMotorLoop_; custom play-test replacement also stops them. Continue
  restarts the correct loop. Original Game1.cpp:1023 stops world sounds on transitions. A live
  sound-enabled dummy-driver run mounted/climbed, paused, returned to Init and started mission1:
  native channel15/16/18 states were Stopped (2), with no later motor Play. Temporary audio
  instrumentation was removed. Captures/log: build-probe/latest-controls/motor-menu-shots/.
- Tank glue follows continuous yaw instead of the integer tile-facing vector's eight directions.
  A shared engine-independent DDA was extracted from the editor raycast for wall range; its
  existing blocker policy and half-cell stand-off are retained. Eight heading/projectile tests,
  cardinal/oblique wall tests, the editor verifier and all113 tests pass. Live17°/73° captures:
  build-probe/latest-controls/tank-run/. Heli and enemy shot directions remain their original rules.
- Compass alignment follow-up: the facing row shows `|` when aligned and the height row `-`
  on the same layer. Turning and height arrows and their tolerances remain unchanged. Unit
  coverage and a live aligned capture (distance6, |, -) pass in build-probe/latest-controls/.
- Initial F9 report was analyzed only, as then requested: native CNA Game.cpp maps F9 to immediate GL context
  loss/recreate. EasyGLTextureRenderer::recreate_gl_resource uploads level0, then allocates higher
  mip levels with null data instead of restoring their contents. Galaxy's custom terrain mip atlas
  therefore samples black when minified (far or grazing floor), while near surfaces retain level0.
  Confirmed before/after captures in build-probe/f9-analysis/ and recorded as upstream_bugs UB-009.
  No F9/context/mip code or sibling repository was modified for this report.
- Follow-up `fairplay`: toggles InputPad's runtime cheat-entry lock, clearing persistent
  megablupi/ghost/doors/secrets/compass/quick/zoom flags on locking, and closes/blocks F12 and hidden
  menu/gesture. Only fairplay remains recognized while locked. Consumable rewards and earlier
  effects (lives, bombs, ammo, keys, vehicles, powers, world changes) are retained. Unlocking does
  not reactivate old toggles. The lock survives input resets and resume format6 (older versions
  checkpoint-fallback). Parser tests cover all28 words, locked matching and unlock; CMake OFF
  VerifyGEInputPad passed and cache restored ON. Live toggle test preserves9 lives,10 bombs,
  ammo/keys/dynamite and reports RESUME SELFTEST equal while locked. Evidence: build-probe/fairplay/.
- The owner's givecopter symptom later stopped reproducing for them. A separate confirmed
  ghost→givecopter conflict left GhostStep winning over the helicopter while the camera changed;
  vehicle cheats now end Ghost and remove its label, preserving independent megablupi. The live
  test reproduces the old failure and verifies an actual climb after the fix. Forced Ghost exit
  inside terrain uses the last valid position; ordinary ToggleGhost's original refusal is unchanged.
- Latest helicopter binding: physical Left Ctrl climbs again, Left Shift descends; Right Shift
  remains an alternate climb. F drops helicopter glue; tank F/LeftCtrl, pad RT and touch Fire
  retain firing. The physical-key remap now applies only in the tank, with F1/manual updated.
  Verified with real X11 LeftCtrl/LeftShift events (climb then land), a full vertical-control
  autoplay, and all113 tests. The descent retains the original inertia; the test waits through
  deceleration. The delivered binary and CMake cache are cheats ON.

- Keyboard-free editor: `EditorDeviceControls` fronts the existing palette/commands without world
  mutations of its own. Touch Move+look works together, Up/Down change height, Tools exposes commands;
  gamepad LS/RS fly/look, RT/LT up/down, A Place, X Remove, B Cancel, D-pad X/Z and shoulders Y offsets,
  Y pointer/fly, Start Save, Back Stop. Init/browser use the pad pointer. Releasing/disconnecting clears
  controls. First Moved/missing Released touch snapshots recover their edges by contact id, in Play
  as well as the editor (needed on the live CNA path). Physical hardware remains untested; SDL
  virtual gamepad/direct-touch events exercised menus/editor/play, including a touch-only 3×2×8 fill.
- Editor Ctrl up, Z down, Space Place. Shift+NumPad extends an inclusive 3D block selection; the
  dimensions display, Place fills, Remove clears, Escape cancels, one Undo restores all cells.
  Tools Select3D supplies Shift for touch/pad; six visible offset buttons or D-pad/shoulders extend it.
- `compass` is off by default. It displays rounded 3D distance to the nearest active chest, switching
  to red arrow/golden key if no chest remains or existed. The owner replaced world-axis labels with
  three rows: distance, relative turn (< or >), height (^ or v). ObjectiveCompassArrows uses Blupi's
  yaw and the shortest horizontal turn; a target directly behind consistently chooses right.
  Directions aligned within3 degrees show |; height within0.1 block shows -. No target,
  no panel. Touch layouts offset it clear of both stick orientations, Crouch, Jump and Fire.
  Relative-heading math and typed input tests pass; live probes783/784 show right/left as Blupi
  turns and up/down for targets at different heights. Evidence: build-probe/compass-relative/.
- Cheat catalog28 entries includes fairplay/compass/help/cheats. Recognized prefixes consume following
  letter keys until release, including the final S of weelkeys; the original rolling buffer survives
  slow typing. A fresh movement key still works. CMake/macro GALAXY_EGGBERT_ENABLE_CHEATS defaults ON;
  OFF gates typed words, hidden gesture/menu, F12 and cheat application. OFF build+113tests/live
  checked, then restored ON; current binary/cache are ON and all words tested. Resume format6 stores
  compass and the fairplay lock (older logs checkpoint-fallback); mission051 reconstruction reports RESUME SELFTEST equal.
- Main menu Help matches F1. Its visible [Close] consumes its pointer through release, preventing
  click-through into Exit/Play. Fullscreen/Window is upper left, names the other mode, and persists
  global fullscreen=0/1 in savedata.txt (default window). F11 is global and persists too. Native
  Xvfb with a minimal QA window manager verified both buttons, F11, fullscreen restart and window
  restart; raw Xvfb has no WM to acknowledge fullscreen, so its mode-switch timeout is a fixture limit.
- Glue fire was keyboard-F-only. Confirmed blupi_recon/src/event.cpp VK_SPACE/VK_SHIFT -> KEY_FIRE,
  decblupi.cpp:901/1387 and Mobile Decor.cpp:3694/4317: tank forwards, helicopter drops down, ammo
  required. Added physical LeftCtrl alias in those vehicles BEFORE merging pad A's synthetic Ctrl,
  preserving A/touch Jump climb. RT maps F, a separate held Fire touch button appears while armed.
  Space/B/X remain enter/leave. Keyboard Ctrl/F, RT and touch Fire showed real green balls; helicopter
  dropping too. F1 now documents F/Ctrl, RT and touch Fire (RightShift can climb the helicopter).
- Lift markers: `LiftEndpointMarkers` uses ObjectDefinitionRegistry's Lift semantic kind. Raw start
  receives icon30 (1), different end icon31 (2), as blupi_recon/src/decblock.cpp BUILD draws them.
  `LiftEndpointRenderer` caches one mesh and existing atlas, updates after edits/undo, draws only
  in the editor. Registry doubleSidedCutout on30/31 and shared double-sided box geometry also fix
  rear faces of the terrain exhibits: alpha-test holes show the back while paint writes depth.
  Live editor probe confirms endpoints and side/inside faces; pure tests cover markers and windings.
  Owner follow-up: endpoints were visible through terrain because the host's HUD depth override
  remained disabled independently of DepthStencilState::Default. LiftEndpointRenderer now enables
  the explicit CNA depth switch for its world draw, then restores the overlay state. Enclosed
  markers render identically to a marker-free baseline (zero differing pixels); exposed markers
  and inner walls remain visible. Evidence: build-probe/marker-occlusion/ fixed/baseline/visible.
  Follow-up: `blupi_recon/src/decblock.cpp:1114` calls MoveObjectStep before its phase check, even
  in BUILD. Editor now calls `InteractionSystem::AdvanceEditorPlatforms` after any presentation
  resync, reusing Play's exact AdvancePatrolStep for active lift families1/47/48. Only runtime
  positions move; raw endpoints/markers/save data stay fixed. Tests compare full XYZ cycles,
  dwell/return timing against Play and ensure other objects stay untouched.
- Interrupted drink: the original Mobile contact hides the item and its36-tick completion restores
  it. Glue cancels the action and its completion event, stranding the hidden bottle. The requested
  fix separates item return (`ReusablePickup`) from the power grant. Completion or interruption
  returns it once; an unfinished action grants no power. A hit on the same contact frame is also
  handled. Tests use an actual enemy projectile, return the source bottle and drink it again.
- Rare Jeep/palm: RecoverFromPenetration's old down-first phase could put the feet inside the floor
  when a BigDecor caught the head. Model overlap now picks a clear horizontal boundary along X/Z,
  preserving Y, with a floor snap for an already buried pose. Ordinary tile/ceiling recovery is
  unchanged. Tests cover foot and three vehicles plus half-buried Jeep; exact owner impact remains
  unreproduced. Latest evidence under build-probe/final-1108; source screenshot paths are in the
  task log build-probe/session-1108-tasks.txt.

The cancelled music previews were removed in commit `213e4cf8`; see the cancellation entry below.
No sibling repository or player save was edited. Full controls/build switch: docs/handbook/03-build-and-run.md.

## Visible hazard contacts and full exit cheers (2026-11-08)

Lava: the pure `LavaHeight.hpp` sampler is shared with `LavaSurface`; controller support uses that
surface in its actual per-cell animation frame, and `IsTouchingLava` tests the feet/body footprint.
The old invisible full-voxel floor is gone. Fractional lava support is recognized by ground lookup.
Live probe 777 survives above the cell and dies at the wave (tick18), not at the voxel top.

Saw: `IsTouchingSawBlade` checks the exposed upper disk, including turned blades and swept falls.
The owner's walking regression revealed that requiring downward motion missed walking into the
blade. Contact at constant height and rising into the exposed blade now count too. The supporting
voxel's sides/underside and stopped blades remain safe. Shield/Hide/vehicle/focus gates and Clear4
effects are unchanged. A movement regression test walks a mortal Blupi into the blade at floor height.
The next owner report found passable saw support cubes: their original empty 2D quarter mask
was still used horizontally. Registry masks for378-383 now make the rendered support solid on all
axes, keeping the lethal blade above it separate. Tests walk into active/stopped supports along
X and Z and verify blocking without death. Original mobile-transparent classification is retained.

Fan: `FanGeometry.hpp` uses bounds of the existing recovered `fan_ventillo-h` model: base/motor/
shaft/hub bones1-4 solid, the swept blades5-8 the lethal annulus. All four base directions and both
turned side directions share inverse model transforms. `TryConsumeFan` checks this volume instead
of any voxel top or mouth cell; consuming wind, immunity, coinflip death/effects are unchanged.
Live probe778: safe landing on the motor (180ticks) and death on the blades (tick17). This supersedes
the older whole-block/top-contact approximation of OR-0068.

Win: every exit now arms the reference 40tick/20Hz cheer, including 199/999/editor play-tests;
only after it completes does the host choose AdvanceAfterWin or the Win screen. Clip time follows
simDt as its action timer does. Showcase exit: still Play after 90cheer ticks, Win at tick276;
bonus199's complete script passes. Normal Win/Bye durations were already correct and were retained.

Verified: native build, 113/113 tests; scripts 021/034/051/055/066/084/102/113/114/115/124/199 all pass,
051 `RESUME SELFTEST equal`; golden trace unchanged. Dry golden images intentionally remove the
default-on number (compass is now opt-in). A preserved pre-task binary was rendered with the same
current dependencies: zero differing pixels outside that HUD rectangle versus the new binary.
Old committed references also had a small independent renderer drift reproduced with that old
binary; references were reviewed/refreshed and the repeated capture matches byte-for-byte.
Evidence: `build-probe/gameplay-1108/`, `build-probe/final-1108/`.

Final follow-up checks: all 113 tests pass after the endpoint-rendering and model-recovery changes.
Golden images still match exactly; complete mission021 and ending199 scripts pass again after the
recovery change. The compass touch inset also leaves room for Crouch in the mirrored button layout.

Final acceptance after the owner's subsequent saw/drink/editor reports: native build and all113
tests pass (`accepted-ctest.log`). Live probes779/780 show mortal walking into the blade dying,
and walking against its support being blocked without death. Probe781 catches an enemy glue ball
during drinking, shows the restored bottle, then drinks it again successfully (Hide and bottle
visible at tick780). Mission051 passes again with `GALAXY_RESUME_SELFTEST=5000` during drinking:
`RESUME SELFTEST equal`; full saw/switch mission065 passes too. Editor before/after captures show
the platform moving while markers1/2 stay fixed. Final golden images and trace match exactly.
All evidence is under `build-probe/final-1108/`; current build/cache leave cheats **ON**.

## Campaign lift looks and treasure gates (2026-11-08)

Completed: `tools/worldkit.py ORIGINAL_LIFT_ICONS` is the small approved fidelity table of
unique type-1 lift icons in each original worldNNN.txt. Authoring cycles that mission's palette;
where its 2D mission has no lift it uses the confirmed palette of the same themed world. All 52
campaign generators with ordinary lifts were rerun. The 829 changed MoveObject records differ
only in `icon`; paths, count, speed, dwell, spawn, sky and music are unchanged. There are 45 looks,
including the palace's 387, metal 340, station tiles, toys and bricks. Existing editor block-look
selection and `LiftVisual` already support these. No reference maps/assets were copied.

World021 is the first mission of palace hub020 (not 121). Restoring its stone lintel at (36,2,23)
closes the bypass above gate6. Auditing every campaign treasure gate found another upper lane in
055: its three-cell corridors had only two door cells; now all three open at the same threshold.
Other open-above cases are deliberate gallery exhibits (998/999). Existing playthroughs 021 and
055 pass; `build-probe/gameplay-1108/gate.txt` proves a jump fails with zero chests and crossing
succeeds with six. Lift-only metadata comparison: `build-probe/lift-style-verification.json`.
The 091 generator writes correctly but its pre-existing D41 authoring audit returns nonzero;
all regenerated static cells except the specified gates match the committed baseline exactly.

The input/editor/hazard follow-ups are completed in the latest entries above.

## Extra music cancelled (2026-11-08)

The owner rejected the ten new previews as outside the original's style and cancelled them.
Removed `music-previews/retro-010-019/` and its dedicated `tools/music/compose_previews.py`.
No world ever used those files. The original ten Content tracks and existing render_music.py
remain unchanged. Earlier listening links in the conversation no longer refer to present files.

## Latest follow-up — underwater colour and distance position (2026-11-08)

Owner: clear water looked dry; the distance number beside/behind the lives was unacceptable.
`WaterViewImmersion` checks the actual camera's grid cell and fades during the final 0.35 block
below the surface. Before any HUD/menu drawing, `Hud::DrawUnderwaterTint` applies mild warm-colour
absorption and a blue haze to the completed world, with a cached full-screen mesh. It does not
stack with water depth. Both camera views stay readable; a camera above water and HUD/help colours
are unchanged. The number now has a separate lower-right position at reference Y338, above the
existing HUD, Jump button and first-person indicator; it shifts left clear of a mirrored right
touch stick. Its meaning and disappearance after the final chest remain unchanged.

Verification: native build, 112/112 CTest, mission 051 and `RESUME SELFTEST equal`, unchanged golden
movement trace, first-/third-person underwater and above-water/help captures. The three dry
golden frames changed only inside the old/new number rectangles (zero differing pixels elsewhere)
and were refreshed for that intentional move. Evidence: `build-probe/underwater-tint/`.

## Latest completed task — controls, help, HUD and water (2026-11-08)

Owner-requested changes in this task:

- Play touch controls are drawn and mouse-hit-tested only when CNA's
  `TouchPanel::GetCapabilities()` reports a connected touch device. Desktop clicks in their former
  areas can drag the camera. Also fixed the dropped touch `crouchHeld` when merging multiple fingers.
- WASD aliases the arrow directions; S retains Down's turn-around behavior. X and Right Ctrl back
  up without turning. F1 opens black controls help on a translucent white panel; F1/Escape closes
  it. The current screen/simulation freezes while either help overlay is open.
- All XNA gamepad slots 1-4 are scanned for the first connected pad. Existing play mapping is
  retained; A now also confirms Resume/Pause/Win/Lost. Verified movement, jump, Start pause and
  A continue through an SDL virtual gamepad, not just synthesized keyboard keys. Physical gamepad
  and touch-screen hardware are still untested.
- Main menu: labeled Exit button calls the normal game exit. Setup and the title fallback use
  `Content/backgrounds/galaxyeggbert.png`, matching the old gold logo's style.
- Bottom HUD: just a small number, the direct 3D distance in blocks (one decimal) to the nearest
  active treasure chest (ObjectType5). No label at the top; no number after the last chest is
  collected, even before its Voyage reward lands. `InteractionSystem::NearestTreasureDistance`
  has no controller/camera/graphics dependency.
- Typing `help` or the reference's `cheats` alias opens every supported cheat word with its effect.
  Descriptions live beside the recognized words in `InputPad::TypedCheatWords`, including the
  deliberately ineffective `killegg`. No new gameplay effects or unimplemented reference cheats.
  Reference: Mobile Eggbert `InputPad.cpp:1318` (MODERN `cheats` overlay).
- Water: the six faces of every voxel were blending over the swimmer, including internal faces
  between adjacent water cells. `WaterSurfaceFaces` removes these, including mixed water families
  and stored animation frames, while preserving the animated exterior. First- and third-person
  before/after views in 051 show Blupi, fish and the cistern walls clearly.
- Resume format **v3** records the touch capability per input tick so replay is independent of the
  current hardware. Old versions are refused and use the existing checkpoint fallback. No sibling
  repository was modified.

Verification: native build, 112/112 CTest, live menu/Exit/Setup/F1/WASD/X/mouse checks, cheat help
and its frozen-frame equality, `alltreasure` hiding the distance, missions 012 and 051 with
`RESUME SELFTEST equal`. Evidence and the virtual-controller fixture are under
`build-probe/controls/` (ignored). Use held `keydown` / `keyup` with sleeps for typed cheats;
`qa_live.sh`'s bare `key` can be too short for a game tick. Native builds here need
`CCACHE_DISABLE=1` inside the sandbox because `/rv/cnaccache` is read-only; use the existing build
tree and at most six compiler jobs. Private Xvfb runs need local socket access beyond the sandbox.

The movement trace is unchanged. Three golden image references were reviewed and refreshed for
the hidden desktop buttons, bottom distance number and water surfaces; an independent capture
matches all three byte for byte. The pre-change host was
also built against the same libraries for comparison (`baseline-run/`): before adding the HUD
number/water fix, old/new renders were pixel-identical outside the six touch-control rectangles.
The older committed references also had minor renderer differences with that pre-change host;
the old references and A/B differences are retained in the task's ignored evidence directory.

The previous author/queue handoff follows unchanged; this task did not take over unrelated audit
or content work. Nothing was pushed.

## Who continues (2026-11-06, updated late evening) — read this first

**Update:** the owner later the same evening told Claude Code to let the running Codex job finish and
then do everything itself until further notice (no new `codex exec` jobs). Queue tasks 1-5 below are
done and committed (`82916f23`, `2efe28bf`, `95561969`, `04a282a2`, `c915f991`), plus two owner
reports found on the way: OR-0077/0078 (the saw machine: saws in the exhibit rows, and the machine
walked into was not found by `TryActivateSwitch`, `5a9e1768`). Open: task 6 (world 199). The text
below was written when Codex was to continue; its queue and briefs stay valid for whoever does the work.

## Codex continues from here (2026-11-06) — the original text

From the evening of 2026-11-06 the work continues with the **Codex CLI** (the owner's decision:
"Codex will continue"). Claude Code wrote this handoff at the end of its session. Everything below
is committed on `develop` and verified; nothing is pushed since `70bf7434` (see "Push" below).

### Model and effort

Use **GPT-6-Astra** (`gpt-6-astra`, the machine's configured default), reasoning effort **high**, for
every queued task: they need close reading of Mobile Eggbert's C++, fidelity judgement and unattended
verification. Use effort **xhigh** for the world 199 rebuild. **GPT-6.1-Sol** (`gpt-6.1-sol`, effort
high) is enough for mechanical follow-ups (rerouting a playthrough after a world change, regenerating
a world, ledger bookkeeping). How to run a brief, and the rules every task keeps (build, tests,
ledgers, commit format, no AI attribution, never push): **`plans/codex/README.md`**.

### Codex queue (in this order; each brief is self-contained)

| # | Task | Brief | Owner's report / decision |
|---|---|---|---|
| 1 | **DONE — BD-0081, this commit (`fix(cheats): BD-0081 match reference ghost behavior`)**. Ghost cheat as Mobile Eggbert: nothing acts on him (the lightning killed him), March/Stop animation instead of falling, drawn at 0.75 opacity | `plans/codex/ghost.md` | 2026-11-06: "the lightning killed ghost Blupi"; "no falling animation, see-through like a ghost" |
| 2 | **DONE — OR-0076, this commit.** Editor aims at the mouse cursor / touch point instead of the screen centre | `plans/codex/editor_cursor.md` | "after the click nothing appears"; chose "aim with the cursor" |
| 3 | **DONE — OR-0077, this commit.** Gallery 998 / showcase 999: saws in the rows of the exhibit saw switches | `plans/codex/exhibit_saws.md` | "the saw machine has no effect" (seen in the gallery); chose saws in the exhibit rows |
| 4 | **DONE — BD-0082, this commit.** Lifts drawn with any tile's look (MoveObjectRecord.visualIcon) | `plans/codex/lift_tiles.md` | chose only "draw the lift as the tile" |
| 5 | **DONE — BD-0083, this commit.** No solid BigDecor in a way a player must take (053's machine leaves a 0.13-cell lane to its door; 058's plant) -- move them in the generators, undo the squeezed routes | `plans/codex/bigdecor_clearance.md` | follows BD-0080 (houses, trees, statue passable) |
| 6 | **DONE — LV-0078, this series of commits.** World 199 rebuilt as a true 3D journey closely inspired by Mobile Eggbert's finale (see "World 199 rebuild" above) | `plans/codex/world199_rebuild.md` (+ `plans/audit/world199_comparison.md`) | "the final world is a mockery and sloppy"; "take strong inspiration from Mobile Eggbert's final world and do it in 3D" |

After each: tick it here (done, commit id), update the ledger the brief names, and continue with the
next. When the queue is empty, continue the fidelity audit (§0 below: `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`
and `plans/audit/`), and treat every new owner report first (owner reports go to
`plans/audit/owner_reports.md` as OR-NNNN; open questions to `plans/audit/owner_questions.md`).

### World 199 rebuild (queue task 6): done (2026-11-06 night)

World 199 is a journey of twelve places A-L (design `plans/briefs/world199.md`, generator
`tools/worlds/world199.py`, world `worlds3d/world199.vwr`): 196 objects / 20 types and 47 BigDecor as
the original, every tile budget of `plans/audit/world199_comparison.md` §7 met (the closing section
"Rebuild done" has the table and the gates). Playthroughs: `tools/autoplay/world199.txt` (main route to
the win screen, 28434 ticks, part of the campaign chain) and `tools/autoplay/spare/world199.txt` (the
alternate route through the optional branches, 41986 ticks; run it alone). Evidence:
`plans/evidence/world199-rebuild-*.jpg`. Ledger row LV-0078 and `lesson_inventory.md` (199 no longer
listed) regenerated. Left for the owner: the review of the journey (gate 7); known soft spots in the
audit's "Rebuild done" (pale palace, plain long tunnels). To change the world: edit the generator, run
`python3 tools/worlds/world199.py`, replay both scripts (mine phases are tick-bound: a changed route
needs the `clear`/`lift` waits re-tuned; probe helpers are in the session's `build-probe/world199-rebuild/`
and not versioned: a segment-per-place chain of the script plus `snap.sh` for a view at a spawn).

### World 199 follow-up after the owner's review (2026-11-07), in progress

Owner: gate 7 accepted; asked for (1) G's follow stars awake and chasing, (2) a helicopter in K with the
barriers filtering walkers, (3) a richer palace (balustrades 398, columns 399, windows), (4) decorated
connectors I->J, J->K (not shortened), (5) a main route of 15-20 minutes (54000-72000 ticks) by real
content. Plan and progress are in `plans/audit/world199_comparison.md` §10.1 (updated per commit);
probe helpers: `build-probe/world199-rebuild/` (seg/*.txt, assemble.py main, assemble_alt.py, run.sh, snap.sh).
Done: required branches; G stars awake; K helicopter + alcove; D balustrades/columns; connectors decorated (main 40138 ticks, alternate 53074). Left: item 5, the route is still under 54000 ticks.

### Waiting for the owner (do not start without an answer)

- **Q-016, the zoom cheat** (`plans/audit/owner_questions.md`): Mobile Eggbert's MODERN-only cheat
  "zoom" (100/50/25/12 %). Measured: the graphics take it (the terrain is one mesh drawn whole);
  the levels' walls limit a pulled-back camera. Recommendation (a): a farther, higher chase camera
  x2/x4/x8 behind the cheat word, reset on leaving the level. Evidence `plans/evidence/zoom-cheat-*.jpg`.
- Release: not yet (the owner's "Zatím ne"). Android stays at "builds and starts" (Q-009).

### Latest completed task — ghost (2026-11-06)

BD-0081 / OR-0075: world/contact gates, March/Stop and 0.75 opacity for model and sprite;
112/112 CTest, unchanged golden frames/trace, typed-cheat active-lightning probe (635 ticks),
clear walking/standing captures and five normal mission passes (012/014/025/051/094) under
`build-probe/ghost/`. Source-guard mapping and the original's
exceptions (awake followers still home; existing dynamite blasts can kill) are in
`plans/audit/behavior_differences.md`, BD-0081. The work is the single commit titled
`fix(cheats): BD-0081 match reference ghost behavior`; no push. Next queued task is editor cursor
only when that task is requested.

### Done in the last session (2026-11-06, newest first)

- `b19db064` **BD-0080** — the BigDecor (houses, trees, plants, toys, machines, the Blupi statue) were
  passable in play since `34ddadb3`: a fresh `BlupiController` at each load dropped the solid boxes.
  Given again after the three resets; probe: the old binary walks into world025's column, the fixed
  one stops. Eleven playthroughs that had walked through a plant, toy or machine rerouted
  (025, 031, 034, 051, 053, 054, 058, 071, 092, 094, 095) and the hub chains 020-090 pass;
  053 and 058 squeeze past (queue task 5).
- `815f727e` world 199 comparison with Mobile Eggbert's finale (`plans/audit/world199_comparison.md`):
  196 -> 95 objects, no BigDecor, a plaza instead of a journey; the 12-section rebuild plan.
- `f00e0aea` Q-016 zoom feasibility.
- `3d73bad3` **OR-0074** the clay bridge no longer rebuilds the whole terrain each frame of its
  crumble (World's change journal, `TerrainModelCells.hpp`); all bridge frames are models.
  A saw switch rebuilt the terrain once per toggle; since 2026-11-07 (`404ca503`) it patches the saws'
  anim blocks too.
- `2331015e` **OR-0073** the mission signs' digits clean (`sign_decals.py` keys the red paint).
- `ba7a7d18` BD-0079 the lit dynamite flickers and sparks. Earlier the same day: the editor's copy
  and paste, backups, export/import, thumbnails (ED-0010..0013), sound fidelity (BD-0031, 0075,
  0076), Resume (BD-0077), the grounded start (BD-0078), CI pinned to verified sibling revisions
  (`tools/ci_pin_siblings.py`), UB-006/UB-007 confirmed fixed in CNA.

### Verified state, 2026-11-07 midday (tree `d44f6397`)

- After the saw patch (`404ca503`), the Hide action in the life-loss window (BD-0084, `387dd111`) and the
  shuffled lightning frames (`bf9bf3bd`): ctest 112/112, golden frames and trace pass (re-based for the
  grass top and the lightning), **full play pass 78/78** (`build-probe/play-pass-final-1107c/results.txt`).
  The girder 201 needed no change (OR-0069). Open items: `docs/handbook/08-backlog.md`.

### Verified state, 2026-11-07 evening (end of the agent work, tree `e852a902`)

- ctest 112/112; golden frames and trace refreshed for the Jump-to-Air change and passing. **Full play pass
  78/78** (`build-probe/play-pass-final-1107b/results.txt`), after OR-0079..0084 (zoom 6 %, lift look in the
  editor, body tilt, switch facing reverted, skateboard upright, TakeDynamite, switch reach). World 199's
  journey was accepted by the owner. Open items for human authors: `docs/handbook/08-backlog.md`.

### Verified state, 2026-11-07 (after queue tasks 1-6, Claude)

- ctest 112/112, golden frames and trace pass. **Full play pass on the final tree (`5d371662`): 78/78**
  (`build-probe/play-pass-final-1107/results.txt`, sheets beside it); hub chains included. The queue
  (tasks 1-6) is done; open: the owner's review of world 199's journey (its weak spots are in the
  "World 199 rebuild" section), Q-016 (zoom cheat), the editor's lift look (BD-0082's note), release.

### Verified state at the end of the previous session

- `ctest --test-dir build-cna-opengl3 -j4`: 112/112. Golden frames and trace: pass.
- Full play pass on the final tree (`b19db064` + the bridge): **78/78**
  (`build-probe/play-pass-final-1106/results.txt`). A regression run:
  `PLAY_PASS_JOBS=4 tools/play_pass.sh build-cna-opengl3 build-probe/play-pass-NN`.
- Build jobs: at most 6 (the owner, 2026-11-01); 4 is the habit. The CLAUDE.md "-j2" is outdated.

### Push

`develop` is ahead of `origin/develop` by the commits from `ba7a7d18` on. Push only when the owner
asks (`git push origin develop`); CI (`.github/workflows/cna-ci.yml`) pins the sibling revisions:
if a task needs newer siblings, bump them with `python3 tools/ci_pin_siblings.py` (and `--check`).

## 0. Start here (fidelity audit, since 2026-11-01)

The owner's campaign brief of 2026-11-01 is milestone G: `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`
(plan, waves, next steps) and its ledgers in `plans/audit/` (regenerate the CSVs with
`tools/audit/*.py`; hand-written parts in the `*.notes.csv` sidecars and the `.md` ledgers). Read the
ledgers before repeating any discovery. Where it stands:

- **Owner reports OR-0001..0023** (`plans/audit/owner_reports.md`, the owner's spot checks of
  2026-11-01; the owner expects the audit to go deeper than the list). Done: OR-0001/0015 lives
  row and life-loss icon; **OR-0021/0022/0023 continuous crates** (BD-0014, M3D-0001 rewritten,
  M3D-0002 lane slide): crates are boxes at any X/Z, move by `CaisseGetMove` while all of the
  group fits, stop the frame he lets go, overhang by the original's 20 px corner supports; a pull
  needs room only for his collision point. Autoplay: `crate X Y Z [tol T]`; a push in a script
  now stops Blupi exactly 57 px (0.89) behind the crate's goal (`go ... tol 0.05`). All 24
  crate playthroughs pass. OR-0008: the executable is `GalaxyEggbert` (target still
  `GalaxyEggbertCNA`), the window "Galaxy Eggbert".
- Done 2026-11-02: OR-0002 (Bye froze: the world-select check returned early every frame of
  the farewell), OR-0003 (the skateboard is put down at DeposeSkate's tick 12, taken at
  TakeSkate's 8; seen live frame by frame 2026-11-05, `plans/evidence/skate-take-and-dismount.png`), OR-0007 (the port's 21 typed cheat words,
  `InputPad::TypedCheat`, `ApplyTypedCheat`), OR-0018 (a lift rider's clip: Stop, not Air; the
  riding itself unchanged -- a first cut that removed his per-frame fall broke 066's lift
  boarding), OR-0019 (the helicopter's ceiling crash, `HelicopterHitsCeiling`, sub-cell masks),
  OR-0020 (vehicles left only on the ground and off lifts, the helicopter/overcraft on both foot
  probes, the skateboard below 8 px a tick: `CanLeaveVehicle`), BD-0015 (a spring wrecks the
  vehicle). **OR-0005 / BD-0016: every creature at its type's original speed and rest**
  (`tools/worldkit.py` `CREATURE_TIMING`, the registry's `patrolTicksPerCell`, the 3D editor;
  `python3 tools/audit/patrol_envelope.py` must report 0). 46 worlds rebuilt; 046/051's mines
  given the original's three-cell beat; 043's chimney opened into the dome (LV-0021).
  Playthroughs re-timed with `tools/autoplay_search.py --jobs 3` (middle of each passing
  window); autoplay logs `vehicle ...` changes and has `expect/until vehicle`;
  `AUTOPLAY_REALTIME=1` paces a run at 60 Hz for `qa_live.sh` screenshots. All 78 pass.
- Done 2026-11-02 (later): OR-0016 (the drink's Hide: his own clips at 30 % opacity, as
  Decor.cpp:828-838; `plans/evidence/hide-drink.png`), OR-0013 (the editor runs the world's
  animation clocks; live look to do), OR-0010 (Setup: Jump side mirrors the stick and buttons,
  automatic zoom 1.3x while walking -- `plans/evidence/auto-zoom.png`, the accelerometer
  steers by tilt with the original's dead zone; settings saved as `jumpRight`, `autoZoom`,
  `accelActive`; the sensitivity slider is not drawn yet; tilt untested on hardware).
- Done: OR-0014 world music -- the original tracks (`SOUND/MUSIC000-009.BLP` in
  `Speedy_Eggbert_1.7z`) rendered by `tools/music/render_music.py --ogg` to `Content/music/`;
  a world's `musicTrack()` (header sky word, second byte), the originals' `music=` for every
  world (`worldkit.ORIGINAL_MUSIC`), the editor's M key; not on the web yet. OR-0015's death
  clips: Clear1/Clear3/Glu show the original element.png sprite (live look to do).
- Done: OR-0009 (Galaxy's loading screen: the title scene with "Loading" and the original's gauge
  on a menu panel, `InputPad::DrawGalaxyLoading`; Mobile Eggbert's picture only as the fallback),
  OR-0012 (the editor's world list stands on the title scene: a 5 x 2 grid of pad buttons under
  the tile letters, trash/tick to delete, Setup's return arrow; `EditorBrowserScreen`). The host
  loads `title.vwr` for Wait and for the list (`titleSceneUnavailable_` stops retrying when it
  fails). Evidence `plans/evidence/loading.png`, `editor-browser.png`.
- Seen live 2026-11-02: OR-0013 (editor animation, `plans/evidence/editor-animates.png`) and
  OR-0015's deaths in probe worlds (`build-probe/probe-worlds/world996.vwr` mines,
  `world997.vwr` lava, made by a throwaway script; run with `QA_LIVE_WORLDS=build-probe/probe-worlds
  tools/qa_live.sh ... --mission 996`): `plans/evidence/death-mine.png`, `death-lava.png`. Fixed
  on the way: the lava death's rising icon was drawn during its 30-tick hold (a second charred
  Blupi); the HUD now gets `VoyageActive() && VoyageIconVisible()`.
- OR-0017 (Blupi partly in walls): his model is drawn out of walls now (render only,
  `WallClearanceOffset`, 2026-11-02). The collision box (18 px each side) tried three times: the
  patch fails 55 of 78 playthroughs on today's tree, mostly timing and points by walls.
  The second try became the patch the playthroughs were re-planned for; in the game since
  2026-11-03 (`a45e785`, see below).
- Done: OR-0004 as far as it can go without detail -- teleporters looked at live (world034) and
  one difference fixed, BD-0017: Blupi lands under the partner pillar (was one cell +Z, turned
  about) and a teleporter takes him only once the contact grace (`m_blupiPosHelico`) is over;
  106's well pillars moved over the wells; 066/102 scripts adjusted. Then compared with the
  original running: the arrival's two sparkle clusters added (`SpawnTeleportArrival`), the second
  sound on arriving removed (`plans/evidence/teleport-arrival.png`).
- Done: BD-0018, six Blupi sounds the original plays and Galaxy never did (turn 5, bar grab 34,
  bar jump-off 35, hanging fidget 36, idle taps 37, crate grip 39), as `BlupiController`
  events. Hanging Blupi has his clips now (StopSuspend/MarchSuspend from `suspendu*.rd4`, only
  those two exported into `avatars3d/blupi/`; a full re-export would also change Hide and
  StopTank -- converter drift to look at first; checked 2026-11-02: the converters of 048c4c0
  and of today build a byte-identical `blupi.glb`, so the day's converter changes (planar caps,
  baked texture repeats) do not touch Blupi; the shipped library differs from any full export in
  ~10 clips (Stop, Mockery(i), Ouf4, Charge, Switch, Teleporting, Vertigo, Hide, StopTank) and
  holds one face more (`face_18abc13655.png`), being assembled clip by clip from builds of
  different days: do not export the whole library blind, compare clip by clip), and the 10-tick JumpSuspend wind-up before he
  lets go of a bar (bars in 064 065 066 071 074 075 083 084).
- Done: BD-0019, the head shake (Non) when the action button has nothing to do: the host's
  `actionUsed` collects every use of the press; BD-0020, Advance: the void behind him, he steps
  forward 0.31 of a cell; with the vehicles' ledge rules (helicopter/overcraft recede at once,
  a parked jeep/tank/skateboard with a wheel over the void rolls on 10 ticks, `m_rollTimer`). New clips are exported one at a time
  (`build_clip_library.py --skip-check` into `build-probe/rd4-blupi/library-susp/`, then copy
  only the new clip, `avatar.skinnedmodel.json` and `clips.txt`).
- Done: BD-0021, hazard contact by the original's boxes (`MoveObjectTouches`: 28 px across, by
  type's vertical band) instead of a 0.7-cell sphere; the contact grace 30 px; a teleporter waits
  until he leaves the cell he arrived in; pickups by the boxes too, the playthroughs re-aimed
  (detours onto items, lick points, exits, dives; found with a temporary probe that logged every
  item the old sphere would have taken -- the method is in BD-0021). BD-0022: a pickup's reward
  was lost when its HUD flight started off screen (projection now clamped).
- Done: BD-0023, keys held as the original's flags (a second key of a held colour stays put;
  Galaxy took it and lost it at the first door). Egg and treasure reviewed: match.
- Done: BD-0024, vehicles boarded by the original's box contact (was a 1.1-cell sphere), and a
  helicopter floating over nothing boarded on contact without the button.
- Done: BD-0025 (an invert sign is taken again while inverted, Decor.cpp:6040). **The bar in 3D**
  (OR-0024/0026/0027, M3D-0003, `BarRows.hpp`): a bar cell is two parallel pipes of the original
  model (`barre_barre`; 202 = its pipe alone, `barreplain_barre`, built with the new
  `objects.txt` option `drop=` that leaves scene parts out) along its row, one under each of
  Blupi's hands; the cell shows its mounts only under a tile they hold to (`ShownBarIcon`, the
  original editor's rule: `Decor::IsRightBorder` compiled from a copy and run over every icon in
  `build-probe/irb/`; 301 of the 303 bars of its levels), whichever bar icon the world holds. On a
  row Blupi hangs on its centre line facing along it and moves along it only; fields (071, 074,
  075) stay free. His model hangs 6 px lower (BLUPISUSPEND) with his right palm on his pipe per
  icon (`HangingModelShift`, measured on the clip library). Evidence
  `plans/evidence/bar-two-pipes.png`; probe world 995 (bar row along Z under a two-cell beam).
  **The teleporters** (OR-0025): each letter its scene's model in its one cell (330 alpha
  `teleporteur1`, 331 beta `2`, 332 delta `4`, 333 chi `3`; IoU 0.997), the device its top third,
  the tip under it; the front (lights, letter) turned to the cell's side with the most room to
  stand in (`ModelCell::openSide`, `kFrontModelIcons`); the old cube and hanging pyramid are
  only the fallback. The converter's flat end caps of prisms now take the whole image
  (`freeform_mesh.planar_caps`, kept out of `split_seam`): the letter panels were a magnified
  corner. Evidence `plans/evidence/teleporter-model.png`; probe world 998 (four teleporters).
  Also: texture repeats (`tile 1`, `cnth`/`cntv`) baked into the image for every mapping (the
  house's flowers at their size; the trees `plante2a-c` not re-exported: +0.0015 for 8 MB); the
  yellow trestle 199 as `Decor/support3.rd4` (front to the open side like the teleporters). The
  numbered world signs 174-181 (`Decor/world1.rd4`, outline 0.995) wait: the scene has no digits
  (painted on the renders) and its face converts as a disc; a digit decal needs the owner's word.
  All 78 playthroughs pass after the bar row change (2026-11-02). The posts 182/183 are their
  models too (`porte1/2.rd4`). Waiting on instanced drawing: the cave rocks 285-303
  (`grotte01-19.rd4`, 0.81-0.91; ~6,000 cells). The switch 384/385 (`interrupteur.rd4`, outline
  1.000): its scene does not swap the lamps (385's sprite has them recoloured: `recolour=`), and
  most switches sit in walls, where the pillar box's four faces still suit.
- Done 2026-11-02 (Wave E): BD-0029, an updraught held a falling Blupi in the air (the fall cap
  was 10 blocks/s, the original's is 40 px a tick = 12.5; the wind's 20 px a tick only halves it):
  `kFallLimit` -12.5, `kWindVerticalSpeed` 6.25; found in world 102, whose plate works go down a
  fan's updraught shaft. `tools/audit/lesson_inventory.py` lists per world what the original has
  and Galaxy leaves out (`plans/audit/lesson_inventory.md`).
- Done 2026-11-02 (Wave E, LV-0060): world 102 gets the original's lessons it lacked: two
  lightning curtains across the machine yard (x 23, 31; passed in one quiet half on the board),
  the plate works' way out (a fan's updraught shaft at (86, 17) down to a gallery, the switch
  stopping a saw on its row, a wind channel with a bar over it to a fourth teleporter 333, the
  atrium's (61, 82)), the red key on a crumbling bridge over lava. Playthroughs 102 and 100 pass;
  `plans/evidence/world102-lessons.png`. The lightning (305) seen live: drawn while deadly.
  `lesson_inventory.py` now counts the bars too: 22 worlds still leave out a mechanic of their
  original (bars in 024, 034, 046, 052, 061, 106, 113, 114, 125, 199); next by weight: 114, 113,
  115, 107, 105, 199, then the bars and the small ones (lava pits, single fans, crushers in 084).
- Done 2026-11-02 (Wave E, LV-0070): world 114 gets a bar over a lava pit on the way south
  (z 60-64, crossed hanging both ways), a one-wide passage over three updraught fans under a
  sweeping mine on the way west (an updraught jump peaks at y 24.8, the mine sweeps at 25), and
  lightning columns on the north and east ways. Playthroughs 114 and 110 pass;
  `plans/evidence/world114-lessons.png`. Open there: the original's sideways wind rooms.
- Done 2026-11-02 (Wave E, LV-0069): world 113 gets the original's bar over lava between two
  hanging mines (x 35-39, one wide; the lift pits moved to x 15, 20, 25, 30). Learned: hanging,
  Blupi touches anything in the bar's row within 0.44 (the hanging mine's box is the top 24 px of
  its cell); a jump off a bar under a ceiling regrabs it, Down held 5 ticks drops him.
- 2026-11-02 evening, the owner's live reports (OR-0028..0035, `plans/audit/owner_reports.md`;
  the queue with sources in `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md` §6 and the task list):
  BD-0031 (every effect at the table's pitch, the crate's effort once, not looped), the Setup's
  volume sliders (effects, music; Erase progress a row lower), the fans at divisor 2, the
  helicopter climbing with Jump, Right Ctrl backing up, a gamepad (`GamepadInput.hpp`), multi-touch
  with a back-up button (`InputPad::UpdatePlayTouches`), the active cheats top-left, the coloured
  doors 334-336 and the vanishing saucer 324-329 as their Ray Dream models
  (`kAnimatedModelBases` draws an animated tile's model per frame). BD-0030 (jump headroom as
  IsPassIcon) changed jumps under wind and bars: 107's saucer ledges moved to 46 and 54 with
  straight hops, 124's wind tunnel fills its height. Still queued from the owner: the crusher (it
  hangs in Blupi's cell in the original; Galaxy put it in the floor) and the flattened Blupi
  (`blupi-plat.rd4`), the clay bridge (`grotte-fragile1.rd4`), the spring (`ressort.rd4`), the
  lightning and the charred Blupi (no scene: to model), grass, the glue death (`glu.rd4`; its
  icons are element.png's, the clip library matches blupi.png's only), the world signs.
- Done 2026-11-02 late (OR-0038..0054): the saw as `Decor/scie.rd4` on the floor around it, its
  floor cubes drawn opaque (`m_sawFloorRenderer`: in the depth-read animated pass the blade's lower
  half showed through, OR-0051); a fan a whole block (M3D-0004); Down and Air hold their last frame
  (`GetAnimHoldFrame`); opendoors as `Decor::AdaptDoors` with `m_bCheatDoors`
  (`HubDoors::AdaptHubDoors`); the back-up button just over the stick's ring;
  `build_clip_library.py` `ease_doubled` (35 clips replaced, vehicles excepted; compare clip by
  clip with `--no-ease` before replacing anything); **BD-0032 the crusher** hangs in Blupi's own
  cell of a one-high passage, passable, `aplatisseur.rd4` via `kAnimatedModelBases`, the squash
  ends vehicles and powers (045/053 rebuilt, 199's invented row removed; levelcheck: a way, not a
  floor); the death angel (element 230-241) as `blupi-ange.rd4` in 3D at half opacity
  (`build_object_library` `alpha=`); water drawn after the models (`DrawOpaque`/`DrawTransparent`,
  OR-0052); the region background a panorama turning with the camera (OR-0053); the world's music
  in the editor palette's Background group (OR-0054). `GenerateSampleWorld3D` writes only world999
  (it overwrote every world). All 112 tests pass; playthroughs: all 78 after the saw/fan, then
  045/053/199 after the crusher (render-only changes since).
- Done after that: the gallery `worlds3d/world998.vwr` (`tools/worlds/gallery.py`, its map
  `plans/audit/gallery_map.md`; the editor seeds it into the first free slot and refreshes it while
  unedited, `SeedGallerySlot`); the web build of 2026-11-02 with it (checked in headless Chrome
  under Xvfb over DevTools: `--use-angle=swiftshader --enable-unsafe-swiftshader
  --remote-debugging-port=9333 --remote-allow-origins=...`, serve the build dir over HTTP first).
- Done 2026-11-02 night (after a full 78-world regression run, all passing): **BD-0033 the slime
  drip** (`a0d31de`: passable, kills in his own cell as `IsGoutte`, animated by
  `table_decor_goutte`, drawn as `goutte01-04.rds`; 062/063/066 replayed); OR-0050's sweep
  (`04c66c0`: the switch 384/385 and the conveyor 311-316 as models, the conveyor stretched along Z
  by `ModelDepthScale`; `tools/audit/model_sources.py` lists what is left: `electro.rd4` Q-005,
  the terrain tiles Q-004, `colonne1`); **OR-0055 the clay bridge** covers its cell: its one row of
  lumps drawn four times across, every other turned round (`kBridgeRows`), the scene's teal
  surface dropped (`drop=2`); the charred deaths (Clear1, Clear3's first half) as his own model
  burnt black and shrinking to the heap (`39d800a`, `CharredBlupi.hpp`), the glue sprite now on
  his feet (it hung half a cell up); the column 375 back to its sprite (`b887345`:
  `Decor/colonne1.rds` is a cube with a painted column, no turned shaft exists). Probe world for these: `build-probe/wave-e/probe_burn.py`
  (mission 995: lightning spawn 50,1,42,90; spikes 50,1,44,90; drip/conveyor/switch 50,1,48,90;
  bridge 44,1,56,90). Note for qa_live: a mine kills by a coin flip (Clear1 or the angel); the
  lightning is always Clear1; only spike 373 kills (347/374 are its animation). Measure a
  model's size at its frame-0 pose (`tools/rd4_to_gltf/posed_bounds.py`), not from the accessor
  bounds: the bridge's bind pose sat 0.109 off.
- Done after: the conveyor lifts 47/48 as the chenille model (`bda333e`, world 102); Wave E:
  084's three gates of crushers (`0cf9a4f`, LV-0051), 024's rods over the lava hall's last gap
  (`7fc63b9`, LV-0011), 034's lava pit across the gallery crossed on rods (`1e99936`, LV-0017),
  each with its playthrough; **OR-0056** the teleporters' letter plate and lights on all four sides
  (`3be74e6`); then 106's rods before the west teleporter (`74184d8`, LV-0064), 061's clay
  bridge to treasure 5 in a lava pool (`9894f64`, LV-0035: the bridge grows back after 156 ticks
  of 20, so a script waits ~520 frames on the far side), 066's crumbling bridge cell over lava in
  the upper corridor (`f79dacb`). 052's rods under its corner drop were tried and reverted: the
  helicopter parked at the well's foot then could not be boarded again (LV-0027, open). Web build
  refreshed (`CURRENT.md`). Rods are `ROD = 202` at the walk over a drop
  (`go ... max 600` crosses them in a script); levelcheck stages put a floor under them.
- Done after (2026-11-02, late night): a full 78-world regression run passed after all of the
  above; the inventory's spikes are icon 373 alone (`f27de41`: 347 is slime); **the glue death in
  3D** (`433b01d`): `glu.rd4` holds only a yellow Blupi, the sprites' green was added afterwards,
  so the converter's new `texrecolour` turns its yellow (texture skin too) green, and the game
  lays his flattened model under it as the puddle (strands not drawn; Q-006 updated).
- Wave E after that, each world replayed: 111's second pit over lava (`1fdcace`), 124's lava
  strip across the start passage (`4ddeab0`; not at x 8-9: teleporter 332's partner stands
  there), 122's pocket over lava on a lift (`8478ed7`; wait for the lift's next arrival before
  boarding back), 014's water channel in the key room (`62bca18`; climb out with `with lookup jump
  1.2`; levelcheck's surface stand-in), 112's two floor saws (`c03bd14`). 105's lava is its
  bottomless hall (LV-0063, reviewed earlier). Left in `lesson_inventory.md`: 013/124 thinned
  counts, the bars of 046/052/061/125, 113's fan (which way its wind runs against the route is
  not clear from the map), 125's lava, 199 (UNREVIEWED, many).
- Then: the render ledger counts blocks drawn as models (`2eeee31`: 97 were misfiled as plates
  or cubes); **the treasure doors 421-439** as the golden post 183's model with their number plate
  a decal (`5cd6968`, `sign_decals.py --plate`, listed with the signs in `kSignModelIcons`). The
  flagged cubes left in use (render ledger: toy balls/badges 215-236, pipes 250-260, gates 86-90,
  187-192) match `Decor/enfant*`, `tuyau*`, `meca*` scenes: they wait on Q-004 (terrain tiles as
  models).
- Then (2026-11-03): 052's rods (`35f8b4a`, `f3c8ec2`: the first try's failure was the script's
  re-boarding at the edge of the helicopter's box, found with a trace of parked helicopters; the
  pit is two deep, no lava: 052 has none). The use case ledger worked through by kind: every sound
  channel's call sites compared with Decor.cpp (`$CLAUDE_JOB_DIR`-free method: grep both sides),
  every action's frame table compared with table_blupi. Found and fixed: the ceiling tap (BD-0034,
  channel 4 + remaps), the mockery's timing and cut (BD-0035), the glue shot's sound (BD-0036),
  invented march footsteps removed (BD-0037), Ouf5 after dropping off a bar (BD-0038, its clip
  built with `build_clip_library.py --skip-check` in 38 s and only the new clip copied: a full
  rebuild changes Hide and StopTank, which were assembled selectively), Clear4's two missing frames
  (BD-0039). Ledger: 331 UNREVIEWED left (objects, functions).
- Then (2026-11-03, later): **the use case ledger finished** -- every function, action and object
  type of Decor.cpp compared (`plans/audit/gameplay_use_cases.csv`: 0 UNREVIEWED; 207 match, 96
  fixed, 53 3D adaptations, 181 not applicable). Found and fixed, each with a verifier check:
  the Cloud ends on the water's surface (BD-0040), the jump out of the water splashes at the jump
  (BD-0041), a fan touched with the shield still explodes (BD-0042), every door key flies from
  x 520 (BD-0043), MockeryDetect ported whole -- height, air, lava/crusher, the 20-tick window,
  Mockeryp without cooldown (BD-0044), no switch with the shield/Hide/SuperBlupi (BD-0045), the
  dynamite cuts floor saws and drips and kills within the original's box (BD-0046), the vertigo
  at a lift's edge (BD-0047), the respawn FIFO's whole gate incl. the objects' and shots'
  trajectories (BD-0048), the pieces flying away (ByeBye: enemies, crates, the invert potion,
  fans, the lost helicopter; BD-0049), cleanall's explosions (BD-0050), dynamite/decoy only with
  both feet down, no decoy by a charger or with dynamite (BD-0051), the overcraft over a lift in
  reach (BD-0052), the crusher/balloon/trap/creature effects
  (BD-0054), the decoy trap's dissolve that never went and the Cloud kill's flash (BD-0055),
  Hide's trail (BD-0056). All 78 playthroughs pass; the web build rebuilt and checked over
  DevTools under Xvfb (`$CLAUDE_JOB_DIR/tmp/scripts/cdp_shot.py`, see its handoff note above:
  world001, 118 models, 93/93 sounds, no exception). Wave E: 061's bars (`836147ae`, the lava
  hall's first trench three wide under two bars along Z; hub 060 replayed). Looked at without a
  change: 046's single bar (over a one-cell gap in the original's key room; its role in the
  route is not clear from the map) and 125's bars under girders and lava strip (a helicopter
  world; the original's on-foot route there needs the video). The follower star's wake (BD-0053, Q-007) was later
  (2026-11-03) brought to the original's box, 052/053 re-laid for it (78/78). Probe world `build-probe/wave-e/probe_burn.py` (995) has a fan, a pool,
  a lava cell and a bulldozer for live looks; typed cheats in `qa_live` steps need
  `keydown`/`keyup` with a sleep (a bare `key` is too short to be seen by the game).
- Then (2026-11-03, latest): **Wave E, world 199's lessons all in** (LV-0078; `lesson_inventory.md`
  lists nothing missing for 199): a lightning curtain in the machine's passage (`d1b3222`), the
  cheese's lava trench on a clay bridge (`8f33b24`), teleporters 330/331 floor-ring (`56661d7`),
  two spike traps and two drips (`4251c6e`), the wasp's well with a moving mine across it, its
  balloon the way out (`ec46f4a`), the invented fish removed (`a190777`), bars over the machine's
  pit (`3912fe2`), a saucer climb to the south arm (`7f1c3c6`), a fan in the machine's east wall
  (`f6ac155`). Learned on the way: a creature's shot passes a closed door (`IsBlocIcon`: a door's
  mask is not full), so 199's script crosses the village's line of fire (z 58) with `clear`; the
  egg cap (10) stops pickups, so no extra egg on a route; an updraught (118) does not lift Blupi on
  foot, it halves his fall (6.25 of 12.5 u/s; use it as world 102 does). The spare enclaves (toys,
  palace, rock) are still not played through.
- Then (2026-11-03, latest): **BD-0057** -- a bar over wind (any tile with an empty collision mask)
  is hung from, as the original's `GetTypeBarre`/`IsPassIcon` (`b07711f`; Galaxy's `IsSolidAt` had
  read wind as a floor). 113's high way got the original's windy stretch: a fan in its upper row,
  bars over two mines in a one-wide lane (`31c4733`); 105 the teleporter pair between the exit's hut
  and a ledge under the roof with two helicopters (`1f72bbc`); 125 a tube of four mines crossed
  under bars off the pool's south bank (an egg at its end) and a lava strip across the corridor's
  mouth (`7b652c9`). 046's single bar recorded as the catch of a high jump through a floor hole
  that Galaxy's deck does not have (LV-0024). 105's lava floor: a full one made the run ~6x slower
  (the animated mesh is rebuilt every tick), so the void stays -- Q-008 asks about a cheaper lava.
  `lesson_inventory.md` now lists only 105's lava, 112's water pipes (scenery, LV-0068), 046's bar,
  013/124's thinned counts.
- Then (2026-11-03, latest): **ED-0007, the original editor's auto-tiling** (`fb76c3f`,
  `src/GalaxyEggbert/Editor/AutoTile.hpp`: `IsRightBorder` rule by rule, `table_adapt_decor`, the
  random twins, the lightning column; run after every placement, erase and box-fill cell as the PC
  editor's `decdesign.cpp` does; right/left from X or Z as the owner chose in MR-0005; one change per
  cell so one undo restores all; the cheese/cave edge plates in empty cells are not laid). Ledgers:
  the render ledger reviewed by the model library's render-back IoU (`cf221a1`: 13 rows left, D11's
  candidates), the asset sources classified from the survey's evidence (`6961504`: 60 left, a3d files
  without a Ray Dream twin and the root test files). Run the editor verifiers from the build
  directory (`ctest`): run from the repository root they write `customworlds/` there.
  Also: ED-0003's moving mechanisms (`cdd8503`: the four lifts are type 1 at the PC editor's speeds
  per cell, scaled when the end is set; the conveyor lift 47; the slime creature 54, not 18); the
  balustrade 398 as its Ray Dream model (`e60f61f`, `Decor/palais/mur06.rds`, a front model);
  **world 199's spare enclaves played to the win** by `tools/autoplay/spare/world199.txt`
  (`051fd54`, superseded 2026-11-06 by the alternate route of the rebuilt world; not in the playthrough set, run it alone); **the 57 unreviewed world lessons reviewed**
  (`042dafd`: 53 CONFIRMED_MATCH by brief + lesson_inventory + the regression + a spawn look, 013 an
  adaptation, 998/999 not applicable; at six spawns the camera starts overhead). **Regression of
  2026-11-03 after all of it: 78/78 playthroughs pass; ctest 112/112;** the web build rebuilt and
  loaded in headless Chrome over DevTools (119 models, 93/93 sounds, the menu drawn; Chrome 155 needs
  a fresh `--user-data-dir` and a few seconds before port 9333 answers).
- 2026-11-03, after wave F: BD-0053 fixed -- the follow star wakes in the original's box
  (`InteractionSystem`, `VerifyInteractionSystem` 1.555); 053's crossing star, gallery approach,
  hall columns (y 21-24) and cage choreography, 052's S2 drop timed to its lift and its hangar and
  well stars re-laid from a traced flight (`tools/autoplay` `trace`; the star-free probe world
  trick: generate the world with `star` stubbed into `build-probe/probe-worlds-052/`). 78/78.
- OR-0014 on the web (`789ceee`): the music served beside the page and fetched per track (see the
  web build section; checked in headless Chrome).
- **OR-0017 done (`a45e785`)**: Blupi's body is the original's 36 px box (BlupiController
  `kBodyHalfWidth`, `ResolveMove`'s `halfWidth`): his face stops 18 px from a wall, he stands while a
  corner of the box is over a top, ceilings stop its whole width; on foot and swimming, vehicles keep
  their point. `WallClearanceOffset` is gone. The harness takes a `go` pressed against a wall short
  of its point as arrived only when he is still in x, z, yaw and y past its first ticks. 78/78 with
  it (066 last: `e5111ab`, its one-wide tunnels walked 0.3 off the mines' line). When a script now
  fails by a wall, the usual causes: a target within a half-width of a wall (go along the middle),
  a ledge left with the box's corner still on it (go further), a jump under an overhang (start
  further back), an `until x >=` the body's face never reaches.
- **The play pass (`06f1323`)**: `AUTOPLAY_SHOTS=N` saves a frame every N ticks of an autoplay run;
  `tools/play_pass.sh <build> <out> [worlds]` plays every script in lanes with it (4 lanes:
  `PLAY_PASS_JOBS=4`, ~75 min) and writes contact sheets `<out>/<world>-pN.jpg` (tick and Blupi's
  place on each frame) plus `results.txt` -- a full regression and a look at the whole campaign as
  the player sees it. Never run two passes at once: they share `build-probe/play-pass-run-<n>/`.
  The first pass (2026-11-03, `build-probe/play-pass-1/`) found: (1) **single-textured-side tiles
  always textured on +Z** (the machine panel 10 and kin, house walls 186-192, palace 390-397: 27 %
  of their visible sides textured; the owner's questionnaire answer has the side per block) --
  fixed `0b401be` (`SingleTexturedSide`: the open side facing the most room; 88 %); (2) **the
  camera cut Blupi at the picture's bottom** whenever a wall pulled it within ~2 cells (the look
  point a block over his feet, 45-degree picture) -- fixed `92b78b9` (`ChaseLookHeight` lowers the
  look point toward his middle as it comes in). A pickup's flight to its counter lasts as long as
  its way across the screen (Decor::VoyageInit), so **any camera change shifts the scripts' timing
  by a few ticks**: run a full pass after one (this one moved 022's lift boarding);
  (3) 045's black dots near (59.6, 52.6, 15.4) are tile 339 (cheese with see-through holes, 18 cells
  in 042/045): through its holes the cube's own inside shows, unlit -- open; (4) frames with Blupi out of view while he walks down a stairwell (024
  t3600) or flies the helicopter in a shaft (043 t6120) -- fixed by (2). The second pass (with (1) and (2), `build-probe/play-pass/`, 78/78 after 022's
  fix) is the current look at the campaign; its sheets wait to be reviewed world by world.
  Review of the second pass's 138 sheets (three agents in parallel, then each candidate checked by
  hand): fixed `f1166e7` -- the decals drawn from both sides (124's thin posts showed the far plate
  mirrored, a 9 as a 6) and the machine panels' plain sides sampled where a bolt or a green lamp sits
  (stripes and green wedges on 113-115's floors); in progress -- the smoothed camera eye cut through
  walls on turns (Blupi hidden behind a wall or the camera inside one: 022, 034, 043, 045, 051,
  052, 053, 054, 092-094, 101, 102, 104, 112, 114, 121, 122, 124, 199), now kept on this side of
  any wall along its line to him after the damping (needs a full pass: camera change); to the owner
  -- Q-010 (cheese holes 339), Q-011 (the brick edge pieces' slits, 73,773 cells). Not defects:
  the exit arrow seen edge-on (101, 113), the helicopter seen from above in a dive (111), the
  squashed Blupi under a crusher (053), the saw blades (021, 024), toy-ball tiles 215-236 as
  cubes (095, Q-004). The moire on far floors is fixed by `90da71f` (below).
  Night 2026-11-03/04, after the review: `2fe3aae` the smoothed camera eye kept on this side of a
  wall (045, 051 no longer inside a block); `4205ae3` **no black where an opaque tile's sprite is
  clear** (69 tiles: cheese 339, brick 262/263/283, slime 347-363, ... -- the black dots, slits and
  slashes; Q-010/Q-011 updated); `90786c7` with a wall right behind him the camera looks from the
  side with room (066 t10440, 034 t15480 fixed) and keeps within 0.35 of his height; 021/065's
  pickup waits 240 (a camera change lengthens a pickup's flight). Play passes 4-6 and the four
  re-runs: 78/78. **Still open (camera)**: frames with Blupi out of view right after a drop or on
  a lift ride in the cave and toy worlds (092 t5760, 094 t15120, 091 t5040, 073 t720/t9360, 075
  t10080 inside the lift's model, 101 t3600, 064 t18360) -- the camera is not inside him there;
  094 t15120 was shot live: a 1x1 dead-end pocket, walled on three sides and roofed -- no
  direction has room, the eye ends in his body and the view is in effect first person along his
  facing; acceptable there. 073's lift (t720) shot live from the same place and facing: Blupi in
  view on the lift all the way up -- the pass caught the camera mid-swing after a turn. 084's black
  quads are gone since `f1166e7`/`4205ae3`. Shot live too: 075 (in view on its lift), 091 (in view
  once the lift clears the ledge); 092 walking down a stair away from the camera was hidden by the
  step's edge -- `d800d3c`: the camera's height now also needs a line to his middle, so it rises
  over such an edge (092's low stairwell: his head shows; play pass 7: 78/78). A taller stairwell
  (cut to walk + 6, probe in `build-probe/probe-worlds-092/`) does not help: the higher step's
  edge right behind him holds the look point under it, so the elevation search finds no room at
  any height. The actual cause (found with a debug print of the chosen elevation): the line of sight
  went to a point half a block over his feet. Fixed by a line to his real feet: 092 shows him whole
  (play pass 9: 78/78; `plans/evidence/camera-stair-092.png`, `camera-feet-pass8-vs-pass9.jpg`).
  The same fix shows him in 094's pocket too: the camera looks into it from above (t14760, t15120,
  t16200 in pass 9, `plans/evidence/camera-pocket-094.png`). Left of the camera list: none (the girder
  201 noted here stops the camera since OR-0069, checked 2026-11-07: `camera-girder-vs-wall.png`).
  `90da71f`: **the terrain mipmapped** from its own padded copy of object-m.png (TileAtlas
  Layout::Padded: 16 px borders, 96 px pitch, box-filtered levels): 125's ring moire gone. The web
  build was rebuilt with it at 01:25 (2026-11-04, after a first try failed while the cna session had
  CNA mid-edit) and loaded in headless Chrome: 120 models, 93/93 sounds, the title scene drawn.
  `0dc7592`: the drip's smallest drops 408-410 as its smallest scene's drop scaled (the render
  ledger has no unreviewed row left); the ledgers regenerated and current (`725505e`, `80a840f`).
  (The girder 201 noted here as open stops the camera since OR-0069, the same night; checked
  2026-11-07.) 084's black quads are gone (see below).
  2026-11-04 early morning: **Q-008 done** -- `2d6c909` keeps each lava frame's mesh (all lava
  cells show the same frame) and gives a pool's inner cells a 4 x 3 wave grid; `f2eeded` puts
  105's lava lake back as the original's (a fall burns him, Clear3): 105 plays in 18.7 s against
  17.3 s for the void. Play pass 8 with mipmaps, the lava cache and 105's lake: 78/78
  (`build-probe/play-pass-8/`); the web build rebuilt with all of it (02:31) and checked in
  headless Chrome. Q-005's electrode is placed in no campaign world. Passes 7 and 8 compared frame
  by frame (2,204 frames, the same ticks): outside 105 (its new lake) the largest changes are the
  mipmaps' -- 125's rings, 053's and 123's far grain gone, nothing worse
  (`plans/evidence/mipmaps-pass7-vs-pass8.jpg`). The agents' last unchecked candidates looked at in
  pass 8 and, for 034, replayed with frames every 30 ticks: 034's blue panel is the pool seen through
  the opened blue door; 124's blue box on the wall is a fan (129) with its wind; 066's faint panel is
  the wind plates (INTENTIONAL, no 3D source); 043's cheese, 055's corridor and 123's ceiling are clean
  since the texel fill and the mipmaps. No review finding is left unexamined.
  The web build was rebuilt after all of it (again 2026-11-04 00:07, with the night's fixes) and loaded in headless Chrome:
  120 models, 93/93 sounds, the menu drawn, no error but the favicon's 404.
- Q-003 done (`22295e0`): the clay bridge wears `Decor/terre.tif` from above
  (`tools/rd4_to_gltf/top_texture.py`, run after the bridge's export; evidence
  `plans/evidence/clay-bridge-terre.png`). Q-001 (grass leaves) stays the owner's: 8,997 grass
  cells in the campaign (3,020 in 021) would need the leaves merged and simplified into the
  terrain mesh, not a model a cell; Q-002 (a bolt without a source scene) likewise.
- Then (2026-11-03, last): the editor ledger has no unreviewed row (ED-0002 by the storage verifier,
  ED-0004 driven live by `tools/qa_steps/editor_playtest.txt`, ED-0005 a mixed undo history in
  `VerifyGEWorldEditor`); the asset ledger has none either (`af4a487`: the survey's root folder was
  missed; a3d files without a twin are Amapi geometry of objects drawn from their scenes); world999's
  creatures at their types' timing (`491cdd2`, patrol_envelope 0; regenerated, it lost 19 stray
  trigger tiles the gallery shows on pillars). D11: the house's flower paper was fixed in `9582fc9`
  (the candidates sheet was made from a stale library); the fountain bowl and column are white in
  every channel of their shaders -- the sprites' dark bowl is Ray Dream's single light without
  ambient, a lighting difference. OR-0011 (later the same day): the Android port brought to the CNA
  game (`c5dcdc5`, `c48ee08`, `37ca3b3`; `ANDROID.md`): `libmain.so` with CNA's `Entrypoint.hpp`,
  the assets copied into internal storage on first start (`AndroidAssets.cpp`, Gradle's
  `assets.txt`), NDK r29 / API 24 / GLES 3, SDL's Java glue from CNA's SDL, stdout in logcat (tag
  `GalaxyEggbert`). The x86_64 APK runs in the API 35 emulator (`-read-only`; no KVM here, so
  software emulation, minutes per screen) through the menu into the main hub
  (`plans/evidence/android-emulator-*.png`), but draws no 3D model: CNA's skinned shader needs
  more than 256 vertex uniform vectors (UB-007, CNA's to fix). UB-006 (CNA lists the APK per
  missing `.xnb`) is worked around by an absolute content root. Q-009: KVM or a phone. The platform lift (type 1) is drawn as its Ray
  Dream model `Decor/techno29.rd4` (`7fc68ac`, IoU 0.94; the survey had filed it with the terrain
  for its folder; no other misfiled piece left there but the cave rocks, which need instancing).
  D11's candidates regenerated from a fresh library (`570aa48`, `c34dc2b`: 119 models; the known
  reasons now written in the list: decals, the fountain's lighting, the saw's specular). Web build
  rebuilt after all of it: 120 models, 93/93 sounds, no error in headless Chrome.
- 2026-11-03, evening (owner's reports OR-0056..0059): **OR-0056 reopened** -- the owner found the
  numbers "mostly turned wrong" after I had closed it from the gallery alone. Cause, found only by
  photographing the campaign: a front model's open side counted only room over a floor two or more
  cells down (written for the teleporters over Blupi's head), so every model standing on a floor
  -- the hubs' mission signs in their niches, the numbered posts' lower cells, 025's balustrades --
  fell to +X. Fixed in `9d363db` (`BlockModels.hpp FrontModelOpenSide`, `VerifyTileFaces`): all 273
  signs and numbered posts of the campaign face an open side. The booths themselves were photographed
  from the player's approach in every campaign world (`build-probe/qa-shots/booths/`, spawns three
  cells off each side with a floor). Lesson: check an owner's report in the
  campaign worlds as the player meets it, not only in 998. OR-0057 the conveyor's belt mirrored onto
  its back (`2e9e055`); OR-0058/0059 were the gallery's layout (`d8233a7`: the saw switches in the
  floor, the crusher on an open beam). The full regression with the committed build: 78/78.
  Noticed on the way, not yet looked into: in a dead-end one cell wide (102's booth at (70,3,12))
  and in some low passages (095 at (30,31,20), 105 at (37,18,48)) the chase camera ends up over
  Blupi's head or inside it.
- 2026-11-04 morning, **the owner answered** (asked interactively, recorded in
  `plans/audit/owner_questions.md`, `58aec2f`): Q-009 Android stays as it is; MR-0001 no 3ds Max
  licence, the .max files stay unused; Q-010 the cheese 339's holes see-through (`dcbb7d8`,
  `TileCoverage.hpp kSeeThroughTiles`; Q-011's brick slits stay filled); Q-002 the lightning
  305-307 as a generated 3D bolt (`707a09d`: `tools/bolt/bolt_paths.py` traces each painted frame's
  centre lines to `BoltPaths.hpp`, `BoltMesh.hpp` builds 6-sided tubes, each column cell turned by
  `BoltCellYaw`); Q-005 the electrode 238-242 as `Decor/electro.rd4` with its painted sparks as a
  camera-facing decal (`4818a92`, `electro238_electro`, `tools/rd4_to_gltf/electro_sparks.py`);
  Q-006 the glue death's strands (`3eb4e4b`: `tools/glue/glue_strands.py` -> `GlueStrands.hpp`,
  drawn with the shared `AppendPolylineTubes`, `plans/evidence/glue-strands.png`, probe world 995).
  **Q-001 + Q-004: every terrain tile as its Ray Dream model, the grass as its whole lumpy models
  with the joining variants** ("hlina s listy nahore ma i preci varianty ze se pojí se sousednim
  hlinou s travou nahore, pouzij cely hrbolaty modely"). What was found and built (Q-012 has it all):
  the terrain scenes are **front slabs, not blocks** (0.15-0.55 deep, up to 1.37 wide, 30,000-58,000
  triangles for the rocks and caves), and the campaign has 1.3 million exposed terrain cells, so a
  cell shows its scene's **relief on each exposed face, near the camera only** (`TerrainRelief.hpp/
  .cpp`, `TerrainRenderer::UpdateRelief`: 8^3 chunks drawn as cubes, or as reliefs within 8 blocks
  of the camera's target, built on worker threads, taken over across terrain rebuilds when their
  cells did not change; `GALAXY_TERRAIN_RELIEF*` in `GalaxyEggbertGame.hpp`, `..._TIMING=1` prints the
  load-time build). A relief is the
  scene's front half, decimated in Blender to ~700 triangles, cut along the cell's edges, textured
  by the sprite projected from the front, hanging only into solid neighbours (Q-012 B; C keeps the
  overhangs: `GALAXY_TERRAIN_RELIEF_OVERHANG=1`), with a backing that reaches into the solid
  neighbours too (it closes the inner corners). The grass is its slab twice back to back (leaves on
  top) plus the crosswise slab at open row ends; a DirectionalCube tile (the commonest, 10) gets the
  relief on the faces that wear the tile only. **The survey's scene for each square tile was wrong
  more often than not** (silhouette IoU is 1 for every square scene): `tools/terrain_relief/
  match_tiles.py` ranks the scenes by the tile's pixels and `tools/terrain_relief/tiles.txt` holds
  the 53 tiles checked by eye (98.7 % of the campaign's exposed faces are in the 80 reviewed; the
  bricks, the grey metal plates and a few panels have no scene in the archive and stay cubes).
  Generated into `avatars3d/terrain/` (3.2 MB, README there has the commands). Commits `7b3a7c0`,
  `4a5967e`, `6c19f1f`, `ee7eb89` (main prints an escaping exception's text: on the web it was
  only a pointer), `07501bd` (ledgers), `64eeebe` (700 triangles, radius 8, worker threads: the first
  version, 1,500 triangles within 12 blocks, held 4-8 million triangles in 093/021 and froze 1.3-1.9 s
  at every door opening; now 093 has 1.5 million, built in 0.4 s at load). Web build checked in
  headless Chrome: 55 then 53 reliefs, 121 models, 93/93 sounds,
  the title scene's grass in 3D (`plans/evidence/web-title-grass-relief.png`). **Headless Chrome needs
  DISPLAY unset** (`env -u DISPLAY google-chrome --headless=new --remote-debugging-port=9333
  --remote-allow-origins=* --use-angle=swiftshader --enable-unsafe-swiftshader ...`), or it has no
  WebGL ("Could not create webgl context"). Evidence `plans/evidence/terrain-reliefs-7-worlds.png`.
  The cave's edge plates 285/302 (InnerFlatPlate) are their scenes' reliefs too, both ways from the
  cell's middle (`7716fa0`; probe world 991). `abe0e5a`: 28 more (the next 40 used tiles reviewed:
  pipes, bookshelves, windows, the palace's column and arch, machine parts, toys, slime edges, two
  more edge plates) and 8 more of the remaining used tiles: **91 tiles** now. `e5e8631`: a one-sided panel chose its room side wrongly
  beside relief cells (082's near wall showed its plain side). Play pass 10 (the first version's
  binary) passed its 54 worlds before it was stopped; **play pass 11 with the final binary: 78/78**
  (`build-probe/play-pass-11/`, sheets per world: the reliefs seen along every playthrough).
  Later (`1e35bf0`): the tiles used only in the galleries reviewed too: **158 tiles** with a relief
  (`avatars3d/terrain/` 5.7 MB); the render ledger has no row left BLOCKED (a tile without a scene of
  its own is a textured cube, INTENTIONAL; animation frames NOT_APPLICABLE), the asset ledger none
  either (the .max files NOT_APPLICABLE by MR-0001; terrain scenes no sprite came from NOT_APPLICABLE).
  OR-0031 closed (every billboard with a 3D source of its own is 3D). ED-0007: the editor still lays
  no edge plates round a mass (they would stand before every corridor's faces in 3D).
- 2026-11-04 afternoon: `193caa4` the 60 relief tiles with clear texels (pipes, machine parts, toy
  sticks, edge plates) get alpha-tested reliefs (they had kept flat cut-out cubes). `269fb28`
  two of BD-0010's open points from Decor.cpp: a busy Blupi (taking a pickup, teleporting, waving
  goodbye, on the ground) survives a mine or small enemy (it explodes) and does not meet a lethal
  decoration; every patrol phase, lifts included, holds the original's extra tick. Four
  playthroughs re-timed (032, 057, 081, 091; the chains 030/050/080/090 with them): play pass 12
  70/78 then the 8 replayed 8/8. Golden frames and trace re-baselined (`cdface8`, then again) and
  passing: they had been stale since July. Live editor checks ED-0006 (BigDecor cycling) and
  ED-0007 (auto-tiling) with the saved world read back (`tools/qa_steps/editor_*.txt`). The .max
  sources NOT_APPLICABLE (MR-0001). `fe43883` BD-0025: the timed states share the original's one
  count (an invert under a power restarts both; both end together; a pop ends an invert): the 17
  worlds with invert signs or wasps 17/17. BD-0010's cosmetic list turned out done (BD-0054/0055,
  ByeBye). 375 has no source. GPU cost measured on the real GPU (SDL offscreen uses radeonsi:
  `LIBGL_DEBUG=verbose EGL_LOG_LEVEL=debug` shows it): with the reliefs 0.4-1.4 ms a frame (021,
  082, 093, 101, 125; Q-012 has the table). Perf runner: an autoplay script with
  `SDL_VIDEODRIVER=offscreen`, wall time / ticks.
- 2026-11-04 evening: play pass 14 (after BD-0025) 78/78. The original's screens and buttons
  (Game1.cpp/InputPad.cpp: `Def::Phase`, `Def::ButtonGlyph`) and every `Tables::table_*` checked against
  Galaxy (`plans/audit/ui_use_cases.md`). Found and fixed: no crouch on touch (`25a7a98`: the original's
  PlayDown, pad icon 23, beside Jump), the Setup's accelerometer sensitivity slider (`d42db03`), the
  powers' auras on Blupi (`c1c254a`: shield orb, power's stars, Cloud's sparks), the drink bottle's
  bubbles (`3f80314`), the yellow gauge for Invert/balloon/squash too (`6604d12`). `qa_live.sh` has
  `mousedown`/`mouseup`; typed cheats in it still need `keydown`/`keyup` and do not always register:
  probe worlds 985-989 (`build-probe/probe-worlds`) place the pickups in front of the spawn instead.
- 2026-11-04 night, the speeds and gates against Decor.cpp (`plans/audit/behavior_differences.md`
  BD-0058..0071, each with its verifier): BD-0058 the water's eased speed and stroke; BD-0059 the
  walk's ease-in (2, 4, 6, 8 px a tick, again after every landing; the Power 3/2); BD-0060 the
  conveyor's 2 px a tick, focus for the creature, spike trap and teleporter; BD-0061 the creature
  bursting a vehicle; BD-0062 no death cheering at the exit, shots pass him out of play; BD-0063 the
  spring's Power bounce; BD-0064 the training hints' inline pad pictograms and accelerometer texts;
  BD-0065 each animated tile cell on its own frame; BD-0066 focus for the drip, saw, crusher and the
  bridge; BD-0067 the traps snap while he is not focused; BD-0068 5 px a tick along a bar; BD-0069
  squashed: his own pace and the slow drift down; BD-0070 the helicopter's EasyMove figures (12 px,
  climb 7, dive 8) and its slow sink when let be -- `Def::EasyMove` is always true, Galaxy flew the
  unused branch; BD-0071 the jeep and tank's heavy fall, the jeep rolling and tipping off an edge.
  Owner reports: OR-0060 the teleporter's booth no longer hidden by the reliefs beside its niche;
  OR-0061 the Setup's "3D terrain" toggle (the reliefs cost 0.35-0.73 GB of RAM and 0.4-1.4 ms a
  frame; off by default everywhere, the owner's call; `GALAXY_TERRAIN_RELIEF=1` forces it on for a
  run). OR-0062: a relief no longer hangs into a corner whose cell is not solid (the hub doors' signs).
  `VerifyGESaveData` now runs in `<build>/verify-savedata/`: it deleted the game's own savedata.txt in
  the build directory on every ctest. Golden frames/trace re-baselined (`85fa28f`,
  `6f20045`, `949c3c7`); stale "not modeled" notes corrected (`e8887d4`).
- The playthroughs re-timed for them: an endpoint just past a ledge (055, 056, 058, 065: a body's
  half-width beyond), 053's alcove (its wall's corner), 057 (`wait 84`), 033 (a wasp cycle later),
  082 (`face 0` on the lifts), 064 (off shuttle B as it passes x 86; its bars at 5 px a tick), 102
  (`face -90` on the pillar), 114 (its ferry boarded as it arrives), 081's saucer chains by
  `tools/autoplay_chains.py 81` (the longer chains leave in the cycle before). The helicopter's sink
  needed its worlds' flights rewritten with the new autoplay `fly X Y Z` (holds a height; `6737e14`):
  013, 025, 041, 043, 044, 045, 046, 052, 058, 064, 105, 111, 112, 115 -- the heights from the
  hovering helicopter's runs before (a climb `until y >= H` used to level near H + 1.4), the water
  crossings held just above the water, landings `go X Z tol 0 with crouch until ground = 1`, and a
  landing before a dismount. Play pass 19 (every change above): 78/78. Web build rebuilt and loads
  in headless Chrome. After it: BD-0072 (the wind pushes him only focused) and the glue drop's kick
  back to the original 10 px (BD-0070 follow-up) -- neither met by a playthrough. Open: the owner's B or C (Q-012); the tail of square
  tiles whose match is unconfirmed (195 ledger rows, little used); the toys' floors are now fields of half balls (the scene's ball stands proud of its plate,
  the plate lies 0.37 under the face) -- authentic, but the owner may want the plate at the face.
- 2026-11-04 late night, owner reports: **OR-0063** the enemy Blupis (32 on his helicopter, 33 in
  his tank) were looked up on element.png (the bulldozer, mines, shield, angel, slime, dynamite);
  they are blupi1.png's (channels 11-13), his own frames with the hue turned 25 degrees, and are now
  drawn as his real model in the helicopter/tank clips the original's icons show, recoloured
  (`EnemyBlupi.hpp`, `DrawEnemyBlupis`, `BuildEnemyBlupiTextures`; billboards from blupi1.png
  without the model). **OR-0064** the steering (Left/Right) turns at the player's pace at F6-F8
  (`turnInput / GameSpeedFactor`); Down's turn-around stays the original's Turn in game ticks.
  **Q-013** the "lattice cube textured only outside": the girder 201 model checked from outside and
  inside in 054/056, not reproduced (its rivets are only in front; mirroring them was tried and
  dropped, see Q-013): needs the owner's screenshot or place.
- 2026-11-04 late night: **world 115's wind wing** (`8af1de8`, LV-0071): the original's sideways
  wind corridors -- a corridor one high with two fans back to back blowing onto a mine in each
  stream, blocks over it with one-cell gaps, treasure 3 on the block over the far mine; a second
  stream with treasure 4 a cell from its fan, reached only by dropping in from above. Treasures 3-4
  left the gallery's ledges (walk 34, 38). Playthrough 115 and the 110 chain pass. 114's sideways
  wind is the original's sealed decor (LV-0070 closed as such).
- 2026-11-04 late night, performance and the owner's next reports: models out of the camera's
  frustum are not drawn (`objectModelRadii_`; `GALAXY_PERF=1` prints draw ms, models, triangles,
  culled; `GALAXY_MODEL_CULL=0` draws all): the hub 17 -> 8 ms, world 063 from under 3 fps to
  16 ms (llvmpipe). Spheres tessellated to their size (`tools/rd4_to_gltf/lower_spheres.py`, run by
  `export_objects.py`): 41 parts 246 k -> 107 k triangles, the chest 68 k -> 8 k. **OR-0065** the
  wind animated (a frame a tick) on crossed full-cell plates (`TerrainRenderer::BuildWindFrame`).
  **OR-0066** a fan goes off only at contact with its mouth (`WorldRuntime::TryConsumeFan`).
  Golden frames re-baselined twice (spheres, wind). After a model change in `avatars3d/`, sync
  the build's copy: `cmake -DGE_COPY_SOURCE=$PWD/avatars3d -DGE_COPY_DESTINATION=$PWD/build-cna-opengl3/avatars3d
  -P cmake/CopyDirectoryIfDifferent.cmake` (the POST_BUILD copy runs only when the game relinks).
- 2026-11-04 late night, more owner reports: **OR-0067** a switch beside Blupi at his height is
  pressed (not only the one under him); **OR-0068** landing on any fan sets it off; **OR-0069** the
  decor that stands in his way in 3D is solid (`kSolidIn3DDecor`: columns, girder, posts, trestle,
  balustrade, machine pieces 49-51, slime column; the original's secret passages 22/73/85/86/339
  stay passable; 056's route and 058's pillars adjusted); **OR-0070..0072** the editor: V cycles the
  aimed block's variant (its family or its re-tiling row), O turns front models a quarter
  (`QuarterTurns`, the plate-turn byte 0-3), a lift's or creature's end is the next click (the
  original's `m_liftEnd`), H shows the keys; O also turns still objects and BigDecor (metadata type
  5, `World Format.md` "Turns"). Play pass 20 (before OR-0069): 78/78; **play pass 22 (all of it):
  78/78**; web build rebuilt and loads. After it: world 115's stream east of the gallery (the
  original's row 17, `f1e0eb38`): a helicopter carried over two treasures into an updraught to a
  landing; LV-0071 complete; 115 and the 110 chain pass. The Android debug APK (arm64-v8a,
  `./gradlew assembleDebug -Pandroid.injected.build.abi=arm64-v8a`) builds with all of it.
  Scripted editor runs: hold keys (`keydown`/`keyup`), a tap faster than a frame is missed.
- 2026-11-05: the house's chimney mapping (`uvsp`, D11 above), autoplay `shots N`, OR-0003 and
  OR-0049 checked live. **Play pass 23 (all of it, `build-probe/play-pass-23/`): 78/78**, in 22
  min with 4 lanes (was ~75: the model culling); ctest 112/112; golden frames unchanged;
  campaigncheck complete; watercheck: every body of water has a way out.
  levelcheck against the 2026-11-03 tree (`2fe3aaeb`): the same 63 of 78 worlds flagged (it does not
  model vehicles, crates, springs, dynamite), identical except 115, whose stream treasures (74/77,
  30, 17) are now reached by helicopter: its playthrough takes all 11 (t 3236, 3300).
- 2026-11-05, later: **every campaign target has evidence it is reachable**
  (`tools/audit/target_reach.py <play pass>`, `plans/audit/target_reach.md`: 921 targets, 0 unknown;
  `levelcheck --rides` adds lift rides and the helicopter). Its probes found **BD-0073**: an egg is a
  life, taken only under ten lives (Galaxy capped the eggs taken: 13 lives were possible). Fixed;
  26 playthroughs re-synced (an egg wait became `until mtick >=` the tick of play pass 23, so a
  mission keeps its timing alone or in a chain with a full lives row). **Play pass 25: 78/78.**
  053's cage corners: all four reached; from the far two Blupi died on the way back past the woken
  stars (three tries): a question for the owner's play (owner_questions.md).
  A hub chain's lives now reach 10 early: a script that waits on an egg must use `until mtick`.
- 2026-11-05, the owner's answers (asked when the owner asked to be asked, `manual_review.md`
  MR-0007): Q-012 B (as built); the toys' ball cubes 215-218 now wear the ball only on their sides,
  top and bottom the plate's plain colour (`e90be6f1`); Q-013 closed; Q-014 (b) built (M3D-0006,
  `609f28f0`: palms, trees, houses, machines, columns, fountain solid, a column of two cells; 025
  and 053 re-routed); 053's back cage eggs moved to the front wall (`e087b011`); OR-0028/0029/0033/
  0034/0048/0053 confirmed; the automated playthroughs suffice for the playability audit.
  **Play pass 27: 78/78.** Web build rebuilt with all of it and loaded in headless Chrome (DevTools:
  121 object models, 93/93 sounds, the title scene and menu). Still the owner's: D11's list,
  OR-0035/0038/0040/0041/0042/0044.
- 2026-11-05, MR-0008: OR-0035/0038/0040/0044 confirmed; OR-0042 analysed (the original's lick:
  three drawings, 0.1 s each, a lick every 0.4 s; Galaxy the same, blended); D11's item fixed
  (`83d7e4bc`: decals at the front behind their own region). Open: OR-0041 (unanswered), any
  further D11 model.
- 2026-11-05/06, more owner answers (MR-0008): hub 070's signs faced sideways (OR-0071's turns on
  the plate bit; front models' turns now metadata type 5: `c297af46`); the ball cubes' tops wear
  their picture again (`80afbc28`); the BigDecor solid as large as their models, plants and toys too
  (`34ddadb3`, M3D-0006); a step stands the crouched Blupi up as in the original, the mines at his
  height over a way raised over his head in 044/045/104/107 (`bebe73bd`, BD-0074, OR-0041); every
  Blupi animation of Mobile Eggbert has its 3D clip (`plans/audit/blupi_animations.md`). **Play pass
  30: 78/78.** Autoplay `with crouch` now matters only standing still (a step stands him up).
  The web build, once CNA configured again (its audit had failed on the cna session's work in
  progress), rebuilt with all of it and loads in headless Chrome (121 object models, 93/93 sounds).
- 2026-11-06, the editor (the owner chose two of the proposed improvements: copy and paste, and
  backups with export): **ED-0010 copy and paste** -- C on two corners copies a box (blocks, turns,
  objects, BigDecor), P pastes on and off with a yellow box where a click stamps it, Q turns the copy
  a quarter, X mirrors it, CTRL+click keeps what is under its empty cells, one U takes a stamp back
  (`Editor/Clipboard.*`, `EditCommand::Kind::Group`; `VerifyGEWorldEditor`; live
  `tools/qa_steps/editor_copy_paste.txt`, `plans/evidence/editor-copy-paste.jpg`). The palette now
  says when an item to place was picked (`UpdateResult::picked`; the pasting stops).
  **ED-0011 backups**: a world's first save after it was opened from the slots keeps the file as it
  was (three generations, `customworlds/gamerN/backups/`), a deleted slot too; the slots' Backups
  button restores (again: an older one, round to the start) (`VerifyGEEditorStorage`; live
  `tools/qa_steps/editor_backups.txt`, `plans/evidence/editor-backups.jpg`).
  **ED-0012 export and import**: Export copies a slot's world into `customworlds/shared/`
  (`A-world-3.vwr`; on the web it is downloaded too), Import opens the browser's file picker on the
  web, the shared worlds are listed first in "Other worlds" and a click copies one into a slot
  (`Editor/PlatformFiles.cpp` EM_JS; `VerifyGEEditorStorage`; checked in the web build in headless
  Chrome: `--ozone-platform=x11` is needed now, and the canvas there is 1000x553, so a game point
  (x, y) is page (1.25 x, 1.152 y); `Browser.setDownloadBehavior` and
  `Page.setInterceptFileChooserDialog` + `DOM.setFileInputFiles` drive the download and the picker).
  **ED-0013 slot thumbnails**: every save writes the editor's view (64 x 64, the middle of the
  frame) beside the world as `custom_00K.png`; the slots show it on the button; the dice's play-test
  waits the one frame it takes (`GalaxyEggbertGame` `SaveEditorThumbnail`; `VerifyGEEditorStorage`;
  live `tools/qa_steps/editor_thumbnails.txt`, `plans/evidence/editor-thumbnails.jpg`).
- 2026-11-06, the owner: D11 closed, no AI policy for now (MR-0009); copy and paste, backups,
  export and thumbnails built in the editor (above).
- 2026-11-06, **Wave F refreshed** (`plans/audit/release_audit.md`): at today's pins 112/112 CTest,
  golden frames, **play pass 31: 78/78** (`build-probe/play-pass-31`), target reach 921 / 0 unknown,
  campaigncheck, watercheck, patrol and lift envelopes 0; the stale ledgers regenerated (references
  and counts only, every status as before); the audit plan's wave table brought up to date.
  **BD-0031 measured**: a playthrough recorded with SDL's disk audio driver in real time (nothing
  aloud) and searched for every sound at every pitch (`tools/audit/sound_pitch.py`, its docstring has
  the commands): 14 channels in 012 and 045, each at its table pitch (0.0, 0.1, 0.2, 1.0), none
  elsewhere; BD-0031 FIXED_AND_VERIFIED.
  Old play passes 1-29 deleted (their results are in this file); 25 (target_reach.md) and 30 kept.
- 2026-11-06: UB-007 (Android without 3D models) is fixed in CNA (`22739f89b`, 2026-11-03 night) and
  seen working: the x86_64 APK in the API 35 emulator draws Blupi and the objects in 3D
  (`plans/evidence/android-emulator-models-ub007.jpg`; the emulator needs `-accel off` now, ~35 min
  to the title). UB-006 (each missed content name walks the APK through JNI) reported to the cna
  Claude session at the owner's request (`upstream_bugs.md`).
- 2026-11-06, the original's functions outside Decor.cpp that Galaxy never named (Game1, GameData,
  Worlds, Jauge, InputPad, Sound, Text, ...; and the tables) read against their callers: **BD-0075**
  Blupi's own 42 sound channels are not played while he is hidden (`4bc7af92`); **BD-0076** the
  world's sounds are heard only within 15 cells of him across and 11.25 up or down, the original's
  three screens by three (`Sound::PlayAt`; the lightning zapped through every world with a column:
  025 recorded, 58 zaps before, 20 only within reach after; `3f72267f`); **BD-0077** (open) Resume
  restarts the mission where the original continues the moment it was left -- asked as Q-015.
  The rest of that list is drawing, I/O and getters, or present under other names.
- 2026-11-06, Q-015 built (the owner: "udelej q 015", and from now on use the Codex CLI where
  possible): **BD-0077 Resume continues the mission in progress** -- the mission's start and every
  tick's inputs recorded (`ResumeLog`), written to `resume.txt` when the window loses focus in play,
  deleted on a clean exit or the mission's end, replayed fast on Continue (implemented by Codex to a
  design brief, reviewed here). Play pass 33 with `GALAXY_RESUME_SELFTEST=1500`: 78/78, 77 replays
  equal. Golden frames and trace re-baselined (fixed 1/60 s from the first Update; see BD-0077).
  The follow-up (Codex, reviewed here): resume v2 stores the loaded world path/byte hash, rejects v1 and changed worlds before replay,
  and caches the executable hash in the build id. Native build and 112/112 CTest pass; 012/045
  self-tests report equal and both playthroughs pass. Scratch-world fallback verified without
  editing shipped worlds; a changed executable with the same compile-time timestamp also refuses
  the log and loads the checkpoint. Evidence is in `build-probe/bd0077-*.log`.
- 2026-11-06, CNA's UB-006 fix confirmed on the APK (the cna session asked): with the relative
  `avatars3d` root the x86_64 APK's LoadContent shows no APK walk in `simpleperf` and loads the
  121 models in 17 min 21 s under software emulation (20 min 35 s with the workaround); the
  workaround (an absolute root on Android) is removed, `AndroidAssets.cpp` stays (std::ifstream
  reads). `plans/evidence/android-emulator-ub006-relative-root.jpg`.
- 2026-11-06, **BD-0078** Blupi starts a mission standing (`BlupiController::SettleOnGround` via
  `GalaxyEggbertGame::PlaceBlupi` at every start, hub arrival, respawn and Resume; the original's
  `m_blupiAir = false`): his first walking tick no longer moves at the airborne 8 px (Codex,
  reviewed here; the golden trace's ticks 0-9 X follow).
- 2026-11-06, the owner (MR-0010): develop pushed (352 commits, `e0e8eda7..164e4c29`); the sounds
  "je to ok" (no listening samples needed); MR-0001 closed, the shields and the charge stay the
  original's sprites; Android stays at "builds and starts" (Q-009).
- 2026-11-06, the CI fixed (the owner: "oprav padajici github ci joby"; MR-0011): its sibling pins
  were weeks old (cna-engine before its UB-005 fix: `VerifyTileFaces` failed). They now pin the
  pushed local HEADs the game is verified against; `python3 tools/ci_pin_siblings.py` bumps them
  after a verified local build (`--check` reports drift). The owner: keep `SkinnedModelEXT`, no
  release yet.
- 2026-11-06, dynamite fuse (uncommitted at the owner's request): the original 100-entry
  `table_dynamitef` selects 252-255 at 20 Hz; the existing model handles all four icons, and
  `tools/rd4_to_gltf/dynamite_sparks.py` extracts the three painted stars from element.png.
  Camera-facing decals attach to the shipped model's terminal fuse ring, including object turns;
  252 has no spark. No Ray Dream source was read. Build in `build-cna-opengl3`, 112/112 CTest,
  `VerifyGetObjIcon` (phases 0/1/2/5/50/99/100), mission 014 and golden frames/trace pass.
  Live showcase evidence: `build-probe/dynamite-spark/fuse-{253,254,255}.png` (an isolated QA
  world, before the blast); the off frame was also inspected. A new owner report starts an OR row.
- Owner's decisions 2026-11-02 (asked interactively, `plans/audit/manual_review.md` MR-0005/0006):
  the editor's auto-tiling (ED-0007) **fully**, right/left read from both X and Z; the world signs
  174-181 as the `world1.rd4` model **with the digit** as a decal from the sign sprite; OR-0006 not
  tried yet by the owner; next priority **Wave E** (per-world reviews).
- Done 2026-11-02 (Wave C): BD-0026 (an exit checked every 51 ticks while he stands on it, the
  original's `m_goalPhase`: the last treasure landing while he is on it wins), BD-0027 (no
  dynamite or decoy put down on a lift, `m_blupiTransport`); matching after review: the
  lollipop, the drink, taking dynamite, the shooters 32/33, the overcraft (46: boarding ends
  Cloud and Hide, as every vehicle); BD-0028 (a decoy meets the bulldozer and the shooters by
  the original's boxes); the fish and the bird's contact. All 29 types the original's worlds use
  are reviewed; left: the secret exit's `m_bFoundCle` (no consumer) and a close look at the
  shooters' and the big creature's height (the original draws them 6 px lower).
- **The original can be watched running**: `tools/qa_original.sh` drives the Linux build of the
  mobile-eggbert port (a copy in `build-probe/orig-port/` of the owner's
  `~/Downloads/SpeedyBlupi2013Linux`; never touch that one) like `qa_live.sh`; its Play always
  starts mission 1, so copy the map to watch over the copy's `worlds/world001.txt`. Use it
  whenever the source leaves a behaviour open.
- Open owner reports: OR-0004 (anything else about teleporters: the owner's word), OR-0011
  (Android: SDK, NDK 29 and emulator images (android-35 x86_64) are in ~/Android/Sdk; gradle
  wants NDK 30.0.14904198, CMake 4.1.2 is there; an APK build compiles everything for two ABIs.
  `android/app/src/main/assets/` links only `Content` and the old 2D `worlds` (May): the 3D game
  also needs `worlds3d`, `avatars3d`, `textures3d`, and CNA's Android file access for them is
  untested), OR-0017 (his
  collision is a point: a body
  half-width of 18 px, the original's BlupiRect, for horizontal moves only, then all
  playthroughs).
- Script lessons from the vehicle rules: wait ~40 frames for a skateboard to slow before Action
  (and never let it glide onto a lift); settle the overcraft (`hold 40 crouch`, `until ground`)
  before Action; fly the helicopter well clear of ceilings (descend before entering a lower
  passage; it drifts a cell after a stop).
- Done: baseline (70d2b99, 110/110); ledgers (`f4d67c8`); silent runs (`b33fac3`); **D60 crate
  push/pull** (`1c59eae`; BD-0001/0002, M3D-0001):
  `CratePush.hpp`, `InteractionSystem::UpdateCratePush()`, crates as boxes in Blupi's collision,
  `BlupiController::FollowCrate()/SetCrateDriven()/CanStandAt()/IsBlockSolidAt()`; tests and the 18
  crate playthroughs pass; evidence `plans/evidence/crate-push-012.png`.
- Game runs are silent now (owner's request): `tools/autoplay.sh`/`qa_live.sh` default to
  `SDL_AUDIODRIVER=dummy`; `QA_SOUND=1` for an explicit sound check.
- Done: **BD-0006** — every shipped lift within the original's speeds (`7e0d1cf` world061: the
  shuttle ladder's jump window ~5 frames -> ~0.9 s with the original's speed and a 30-tick dwell;
  `46a663b` 113, 024, 062, 073). `python3 tools/audit/lift_envelope.py` must report 0. Method for
  a lift problem: the original counterpart's speed in `../mobile-eggbert/worlds/worldNNN.txt`;
  probe with a copy of the builder (`--stage -1 out.vwr` reproduces 061 byte for byte) in a
  worlds dir of symlinks (`AUTOPLAY_WORLDS=<dir>`); measure windows with
  `tools/autoplay_search.py`. Never `pkill -f` a pattern that also matches a parent shell.
- Done: **BD-0003** (a crate could be pushed out over water: support needs a solid mask; the 18
  crate playthroughs unchanged).
- Done: ED-0001 editor variant cycling (`e8a86ae`) and ED-0006 for the BigDecor families; BD-0004
  follow stars and shot ranges vs decor (`92a1a6f`). After all of today's gameplay fixes: 110/110,
  all 78 playthroughs, the whole campaign 761140 ticks (2026-11-01).
- `.a3d`: decoded and read (`tools/a3d/a3d.py`, all 105, `plans/audit/a3d_format.md`). The lava
  frames are a height-field slab, the columns profiled shafts: the next step is a 3D lava surface
  and pillar models from them or their `.rds` twins.
- `.max` (MR-0001): the owner offered a 3D Studio MAX 3.1 ISO; setup was stopped unaccepted (the
  key file holds the generic key-generator serial, the ISO a `CRACKS` folder); still BLOCKED until
  a genuine licence is confirmed. The XP VM was shut down, its ISO detached, its network still
  `null`. Do not change the VM's security settings (a registry change for blank-password guest
  logons was refused by the permission classifier).
- Fan/lava/pillar live views: first captures in `build-probe/fa-live/vis*/` are too imprecise;
  take them in world999 from the exhibition rows' documented positions, first person (F3), close.
- Done (visual, Wave B): lava as a pool with the original wave from `Decor/lave/lave-1..5.a3d`
  (`90acade`, `LavaSurface.hpp`, data `tools/a3d/lava_surface.py`); fans as the original model
  `Decor/ventillo-h.rd4` turned per direction (`6daa06b`); pillars 76/77 as `support1/2.rd4`
  (`801c232`). Mechanism for more: `BlockModels.hpp` — animated families by base
  (`SetModelDrawnBases`) and still icons (`kStillModelIcons`, the terrain constructor's list), drawn
  by the host's model pass. A new block model: build it with `build_object_library.py` (probe in
  `build-probe/`), export with `export_cna_avatar.py`, copy `icons.txt`, add to
  `avatars3d/objects/index.txt` and `tools/rd4_to_gltf/objects.txt`, list its icon in
  `BlockModels.hpp`, regenerate `model_parity.md`/`assets.md`, look live (before/after).
- Done (Wave E): 066's blupit walks across room B1 so its line of fire must be crossed (`63e37fe`,
  LV-0040; crossing window measured with `autoplay_search.py`); 094's mine rows checked.
- Next: pillars 375/376 (no source found: `Decor/colonne1.rds` is a plain textured cube, not the
  turned column); the wind tiles
  (`Decor/vent-norm/` holds only rendered frames: a 2D effect candidate); a low-angle owner look at
  lava/fan/pillars; Wave E for the other mechanic-heavy worlds (199 next); Wave C type by type (an
  enemy review of types 4/16/17/20/44/54 was started 2026-11-01).

## 1. The project and the goal in one paragraph

Galaxy Eggbert is a faithful, true-3D tile/voxel remake of Mobile Eggbert / Speedy Blupi, written
in C++ on CNA (an XNA 4.0 reimplementation) and cna-engine. The owner's standing goal: finish it as
a faithful, fully playable, polished remake, keeping the existing architecture (voxel worlds,
`.vwr` format, the 3D editor, controls, camera, registries, saves), with gameplay taken from
`../mobile-eggbert` (C++, the authoritative reference) and models/animations converted from the
original Ray Dream scenes in `../speedyblupi-data`. Read, in this order:
`CLAUDE.md` (repository rules), `../CLAUDE.md` (build rules for every repository),
`plans/GALAXY_EGGBERT_COMPLETION.md` (direction, defects D1-D59, milestones in §13), then this file.
`plans/SESSION_SUMMARY_2026-10.md` summarises the whole session that brought the project here
(what was done, the evidence, the honest list of weaknesses).

## 2. State (verified)

| Milestone | State |
|---|---|
| A, B, D, E | done (plan §13) |
| F, stabilisation and completion audit | **done**: suite 110/110; `tools/qa_sweep.sh` loads all 79 worlds live, 79/79; `campaigncheck.py`, `watercheck.py`, `VerifyShippedWorlds` pass; every mission has a playthrough, every world a chain from its hub, and the whole campaign plays in one autoplay run (755171 ticks, ends on the win screen) |
| C, model/animation parity | **open only for D11 and BB3D-032** (below); everything else done: parity matrix `plans/inventory/model_parity.md`, every turning object matched every 5 degrees, objects and vehicles toned to their sprites, Blupi's 45 clips at 0.94-0.99 silhouette IoU |

Engine defects found by the playthroughs and fixed with tests: D55-D59 (penetration recovery, a
lift's reach through ledges, a lift taking him sideways off a ledge, a key opening one door cell,
dynamite taken two at a time / without Action). World defects fixed: 013, 025, 062, 066, 082, 094,
103, 104, 105, 113, 114, 121-124, 199 (details in the plan's §13 row F and the briefs in
`plans/briefs/`).

The Web/Emscripten Release build was freshly rebuilt and linked on 2026-10-31 against the current
CNA and cna-engine checkouts. Headless Chrome created a real WebGL 2 context, loaded `world001`,
82 object models and 93/93 sound channels, uploaded the terrain, and visibly rendered the
gamer-select menu over its 3D scene. The build exposed and fixed one portability defect:
`ChaseCameraYaw.hpp` now includes the standard header required by its initializer list.

## 3. What is open, and exactly how to continue

### D11 — the owner's list of models that look wrong — CLOSED 2026-11-06

The owner closed D11 on 2026-11-06 ("Ne, D11 uzavřít", `plans/audit/manual_review.md` MR-0009)
with the items named (the sign plates and digits, hub 070's signs, the ball cubes' tops, the house's
footprint), all fixed. Kept for a model someone names later: `plans/inventory/d11_candidates.md` lists the models that differ most from their sprites
(worst silhouette IoU, largest remaining colour error) with the numbers; the contact sheet of
sprite vs model for those (sent to the owner) is regenerated by
`tools/rd4_to_gltf/d11_candidates.py` (its docstring has the commands; the full library it reads
is `build-probe/rd4-objects-current/`). Seen on that sheet, worth looking at first even before
the owner's list arrives:

- `bd_maison` (the house): the front (`Facade`) matches. The east wall (`Mur porte`) was checked
  2026-11-05: rendered with the scene's own light (`scene_render`) its surface matches the sprite
  (dark pink, small foreshortened flowers; the sprite's door jamb is a bright streaky strip too).
  What differs in the game is light, not mapping: the original's light stands in front of the house
  (`Lumière1`, grazing the east wall at 85 degrees: diffuse 0.09, so the sprite's wall is 0.56
  of the front's brightness), while the game lights every model from one world direction, which
  reaches that wall fully. Faithful as a 3D object; not a model defect. The `uvsp` lead was real for
  the chimney instead: a domain with `uvsp` other than 0 has a real `bbox`, the map repeats every
  bbox width round the section, and the chimney now shows round flowers at the walls' size as the
  sprite does (`65a532cc`, README rule; only the house carries a `uvsp` among converted models;
  sprite / before / after: `plans/evidence/house-chimney-before-after.jpg`).
  Left: the window recess is dark in the sprite (the scene's cast shadow inside the house), lit in
  the game (no cast shadows on models).
- `bd_big02` (fountain bowl) and `bd_big0108` (column): grey in the sprites, near white in the
  model. Checked 2026-11-04: every shader of both scenes is white (0.996) with no highlight,
  shininess, reflection or glow (`Decor/palais/big02.rds`, `big0108.rds`; the red 0.5/0.5 ones are
  masters' defaults the instances override), one light, shadows on: the sprites' grey bowl and dark
  column side are that render's lighting and cast shadow, not the material. Faithful; closed unless
  the owner names them.
- `bd_cabane` (cabin): its logs are a procedural wood grain, flat here (decided, see below).
- `dynamite`: the sprite's lit fuse spark is an effect, not part of the model (expected).
- `bird`, plants 1 and 3: thin wings, legs and leaves a pixel off; faithful (recorded).

When the list arrives, for each named model:

1. Find its entry in `tools/rd4_to_gltf/objects.txt` (name, sprite sheet, icons, Ray Dream
   scenes, options) and its directory in `avatars3d/objects/<name>_<scene>/`.
2. Rebuild it alone: `python3 tools/rd4_to_gltf/build_object_library.py --originals
   ../speedyblupi-data/originals --sheets ../mobile-eggbert/Content/icons --objects
   tools/rd4_to_gltf/objects.txt --out build-probe/rd4-objects-probe --only <name>`; look at
   `build-probe/rd4-objects-probe/<dir>/compare.png` (rows: sprite, model, overlap) and the IoU in
   its `icons.txt`.
3. Find the cause in the converter (`tools/rd4_to_gltf/`, every verified Ray Dream convention is
   in its `README.md`), never by hand-editing the model. Probe hypotheses with temporary
   environment switches, measure IoU, revert what does not help, record the experiment in
   `plan.md` (see BB3D-032 for the style).
4. Export only that object: `export_cna_avatar.main(["--glb", <probe>/model.glb, "--cna-home",
   "../../libcna/cna", "--out", "avatars3d/objects/<dir>"])` and copy its `icons.txt` beside it.
   Do NOT run the full `export_objects.py` against a partial library: it deletes every directory
   the library lacks.
5. Check live (`tools/qa_live.sh`, below; world999's exhibition rows z 27-54, x 78-96 hold every
   object), regenerate `plans/inventory/model_parity.md` with `python3 tools/model_parity.py >
   plans/inventory/model_parity.md`, commit.

### BB3D-032 — the fish's head (~15 % large), cosmetic

Every lead has been tested and recorded in `plan.md` under BB3D-032 (jaws' deformer about origin
vs hinge, envelopes, interpolation, jaw scale, the hidden free-form, the body cylinders). No
remaining hypothesis. Leave it unless the owner's D11 list names the fish.

### Left as decided (do not reopen without the owner)

- Procedural textures (palm bark, wood grain, metal noise) stay flat colours: an approximate bake
  was measured worse against the sprite (plan §7.3).
- Objects and vehicles are toned to their sprites (owner's choice): `tone=A,B` per object in
  `objects.txt`, fitted by `tools/rd4_to_gltf/calibrate_tone.py <library> --objects
  tools/rd4_to_gltf/objects.txt`; vehicles by `calibrate_vehicle_tone.py`, applied with
  `export_cna_avatar.py --tone` (commands in `avatars3d/blupi/README.md`). A new object: build
  without a tone, calibrate, add the suggested `tone=`, rebuild, compare. The owner asked for an
  opinion on reverting; the recommendation given was to keep them and soften only an object that
  looks washed out in play (before/after sheets were sent). Revert = `git revert ad46640 a25e8cb`.
- levelcheck reports traps in 052 and 054 that the wasp's balloon actually leaves (not modelled).
- 066's blupit can be walked round (room B1 wider than its line of fire): design weakness, left.
- The charge (31) and the shield stay the original's sprites (no 3D source outside the `.max`;
  the shield/power scenes in the archive are other things, checked 2026-11-06); BigDecor are as
  large as their models (M3D-0006). MR-0002 settled.

## 4. Workflows

### Build and test (rules: `../CLAUDE.md`, `CLAUDE.md`)

```bash
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv      # the one shared ccache; never another
cmake --build build-cna-opengl3 --parallel 4
ctest --test-dir build-cna-opengl3 -j4                  # 112 tests, all pass
```

### Web build

Use the existing build tree and the workstation's actual CNA/cna-engine locations:

```bash
source /home/robertvokac/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-cna-webgl2 \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_HOME=/rv/data/development/github.com/libcna/cna \
  -DCNA_ENGINE_HOME=/rv/data/development/github.com/libcna/cna-engine \
  -DCNA_GRAPHICS_RENDERER=WEBGL2 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cmake --build build-cna-webgl2 --target GalaxyEggbertCNA -j2
```

The deployable files are `build-cna-webgl2/GalaxyEggbert.{html,js,wasm,data}` and the folder
`build-cna-webgl2/Content/music/` beside them (the music is fetched a track at a time, not packed;
OR-0014). Serve them over HTTP; opening the HTML directly as a `file:` URL is not a valid runtime
check.

Only existing build directories (`build-cna-opengl3/`, `build-cna-webgl2/`; `build-probe/` for
probes and converter output). Never build in `/tmp` or the scratchpad. Reconfigure only when broken:
`cmake -S . -B build-cna-opengl3 -DGALAXY_EGGBERT_BUILD_CNA=ON -DCNA_GRAPHICS_RENDERER=OPENGL33
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache`.

### Playthroughs (QA autoplay) — the regression net for any gameplay or world change

- `GalaxyEggbert --mission N --autoplay <script>` plays a script deterministically (fixed 1/60
  ticks, draws one frame in 30) and writes `autoplay_trace.txt`; exit 0 = passed. Commands and
  fields are documented in `src/GalaxyEggbert/Game/Autoplay.hpp` (go/jump/tol/until/with, lift,
  crate, clear/near with lane, hold, face, expect, trace, include; fields incl. blitz, temp, crush,
  mtick, keys, eggs, dynamite, phase).
- `tools/autoplay.sh build-cna-opengl3 <script> <mission> [--spawn x,y,z,yaw]` runs one under its
  own Xvfb with a fresh save in `$AUTOPLAY_RUN` (default `build-probe/autoplay-run/`; give each
  parallel run its own directory).
- Scripts: `tools/autoplay/worldNNN.txt` (a mission; a hub's `worldN0.txt` is its world's chain:
  each sign, the mission's script included, back past the bar it opened, the golden key to the main
  hub); `tools/autoplay/campaign.txt` (a new game through all twelve worlds, the gold bars and
  199; regenerate with `python3 tools/autoplay_campaign.py`). `tools/autoplay_all.sh
  build-cna-opengl3 [worldNNN ...]` replays mission scripts; the campaign run is the full replay
  (about two hours of wall time: run it in the background).
- After any change that can touch gameplay or a world: replay the affected missions, and before
  calling a batch done, the campaign. Also `tools/verify_demos.sh build-cna-opengl3` (the three
  main-menu demos, seconds; CTest `VerifyDemoReplays`): "different" means the simulation changed —
  bump `kDemoSimulationRevision` and re-record them (commands under "Main-menu demos" above).
- Writing or fixing a script (hard-won rules):
  - Write a section, run, read the trace, repeat; `trace N` logs every tick; `shots N` saves the frames of the next N ticks into the run directory's `autoplay_shots/` (a moment caught without `AUTOPLAY_SHOTS`).
  - Lifts: wait on the leading cube's unique end cell (other cubes pass through intermediate
    cells); board at the start of a dwell (wait for the far end, then the near end).
  - Jumps need a run-up and take off at an edge; jumps two up take off close to the wall.
  - Pickups need Blupi within ~0.6. Sticks of dynamite need `hold 2 action` (D59). Keys are
    spent by a door (the whole count clears); eggs stop at ten per mission (MAX_EGG_COUNT).
  - Patrols: `near X Y Z R lane x|z` for the enemy at its far end, then passing heading away.
  - Cycles: sync on the fields (`until temp = 19` then `until temp = 0` for vanishing saucers,
    `blitz` for lightning, `crush` for crushers); never on `tick` (it breaks in chains; use
    `mtick`, ticks since the mission loaded).
  - To find a delay, template it as `wait {D}` and run `python3 tools/autoplay_search.py
    <template> "<mission> [--spawn ...]" <from> <to> <step>`; pick the middle of the passing run.
  - Water: reach the edge, `until y >= surface`, face the bank, `hold 8 up jump`, `hold 30 up`.
- A script that fails after an engine change: check whether the mechanic changed against
  `../mobile-eggbert` before "fixing" the script; a real engine defect gets a test first.

### World changes

Worlds are built by `tools/worlds/worldNNN.py` (writes `worlds3d/worldNNN.vwr`; staged variants
`--stage K out.vwr` for checks). After a change: `python3 tools/levelcheck.py` on the stages the
world's brief (`plans/briefs/worldNNN.md`) names, `ctest -R VerifyShippedWorlds`, `python3
tools/campaigncheck.py`, `python3 tools/watercheck.py worlds3d/world*.vwr`, the mission's
playthrough and a live look; update the brief. Never convert or extrude the 2D maps. A changed
world011/074/105 makes its main-menu demo refuse to play (CTest `VerifyDemoFiles` fails): record
that demo again with `tools/record_demo.sh`.

### Live checks

`tools/qa_live.sh build-cna-opengl3 <out> <steps> [--mission N --spawn x,y,z,yaw]` runs under
Xvfb from `build-probe/qa-run/` (never touches the owner's `build-cna-opengl3/savedata.txt`);
steps: `sleep`, `shot NAME`, `key`, `keydown/keyup`, `focus` (Space is Action). `QA_LIVE_AVATARS=`
and `QA_LIVE_WORLDS=` point at alternative asset directories (A/B comparisons with
`git archive <rev> avatars3d`). `tools/qa_sweep.sh build-cna-opengl3` loads every world (79/79).

### Models

Converter: `tools/rd4_to_gltf/` (`README.md` holds every verified Ray Dream convention).
Objects: `build_object_library.py` + `objects.txt` → `export_objects.py` / `export_cna_avatar.py`
→ `avatars3d/objects/` (`avatars3d/objects/README.md`). Blupi and the vehicles:
`build_clip_library.py` → `avatars3d/blupi/` (`avatars3d/blupi/README.md`; rebuilding Blupi is
expensive and risky: verify byte-identical output before replacing anything). Parity:
`tools/model_parity.py`. Converter outputs and probes go to `build-probe/` (`rd4-objects-current/`
is a full library built 2026-10-27 with the current settings).

## 5. Rules that bind every change

- CNA (`../../libcna/cna`) is absolutely read-only: no edits, temporary changes, commits or
  issues. A CNA defect goes to `upstream_bugs.md` (English, UB-NNN). It is a live working tree
  another session edits: rebuild after its header changes before suspecting this repository.
- cna-engine: only justified, policy-compliant fixes, upstream with a test and CHANGELOG, no
  workarounds in Galaxy (owner's rule).
- Never modify `../mobile-eggbert`, `../speedyblupi-data`, `blupi_recon`, `free-eggbert`.
- Every gameplay feature traces to confirmed mobile-eggbert behaviour; fixes only for a reproduced
  bug, focused, with a regression test.
- Commit each discrete task; no AI attribution of any kind in commits or PRs; push only when the
  owner asks. Never `git reset --hard`, `git clean -fd`, forced checkout, destructive rebase.
- At most 2 concurrent subagents; give each its own autoplay run directory and file prefixes.
- The VirtualBox VM `xp` (Ray Dream Studio) must never get network access.

## 6. Sibling repositories (last verified together)

| Repo | Verified commit | Date |
|---|---|---|
| `../../libcna/cna` (live tree; what `build-cna-opengl3` compiles) | `5a754159d` | 2026-11-03 (112/112, wave F's playthroughs, Android APK) |
| `../../libcna/cna` | `090c998a1` | 2026-11-03 (112/112, 78/78 playthroughs, web build) |
| `../../libcna/cna-engine` | `2a93b7e` | 2026-11-03 (same) |
| `../../libcna/easy-gl` (via `CNA_EASYGL_ROOT`) | `b582bd8` (v0.1.1+2) | 2026-11-03 (same) |
| `../../libcna/meta-gl` | `795bcdc` (v0.4.1+2) | 2026-11-03 |
| `../../libcna/cna` | `dda46ec4d` | 2026-11-01 (110/110) |
| `../../libcna/cna-engine` | `d86c62da` | 2026-10-20 |
| `~/deps/cna-baseline` | `900d0db3b` | 2026-10-20 |
| `../../libcna/easy-gl` | `f4d6afaf` (v0.1.1) | 2026-10-20 |
| `~/deps/sharp-runtime-baseline` | `f2aecfef` | 2026-10-20 |
| `../../libcna/meta-gl` | `dd3b3005` (v0.4.1) | 2026-10-20 |

## 7. Gotchas

- `tests/golden/` is current as of 2026-11-08 (desktop controls hidden, compass hidden by default,
  water exterior faces; the movement trace is unchanged).
- The chase camera orbits to Blupi's side after 3 s idle; `--spawn` yaw 0 faces -Z, 90 +X.
- Types 32 and 33: the code's names ("blupih"/"blupit") do not match their icons; check against
  mobile-eggbert before converting.
- Effects (explosions, splashes, sparkles, goo) stay billboards (decision 2026-10-24).
- Part files of exported models are named after their colours (`<name>_rd_rrggbb`); toning
  renames them, and no code refers to those names.
