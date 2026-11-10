# Galaxy Eggbert multiplayer: analysis, options and implementation plan

Written 2026-11-08 as a hand-off for the AI agent that will implement multiplayer in
`galaxy-eggbert`. Every fact of §1-§14 comes from reading source and documentation in the
repositories named below (before the implementation).

**Status 2026-11-10: implemented on branch `multiplayer`** (worktree `../galaxy-eggbert-multiplayer`,
not merged into `develop`: the owner accepts it first, Q-019). The owner's answers (Q-019): SB2's
rules, Linux and Android, LAN with a documented ZeroTier setup, one world then 3-5 from SB2's
201-212, the host leaving ends the game, team colours by recolouring, speed and cheats as the
original, automatic sign-in as Player A/B/C, the original's defects not reproduced, chat in the
first version. What is done, as recommended in §7 (option A over CNA SystemLink):
- M1-M2: the protocol and its checks, the lobby (`src/GalaxyEggbert/Net/`, `VerifyNetProtocol`).
- The CNA adapter `src/GalaxyEggbertCNA/NetSession.*` (a stand-in without CNA networking: no
  Multiplayer on the web); the screens and the match in `GalaxyEggbertGameNet.cpp`.
- The rules (§3.3, all read in SB2's source, the [NV] ones settled: PlayPrepare removes types 7,
  12, 21, 32, 33; another team's bomb kills, one's own is taken back with the action key); the
  replication (§7.5) in `NetReplication`; poses with the model's transform; Android's permissions
  and multicast lock.
- Worlds 201-204 (`tools/worlds/world201-204.py`, briefs).
- Tests: `VerifyNetProtocol` in CTest; `tools/net_two_players.sh` with four scenarios
  (`tools/autoplay/net/`), all passing; the differences in BD-0120; the player's guide with ZeroTier
  `docs/handbook/09-multiplayer.md`.
Open: the Android build and a game between a phone and a PC (not run yet); worlds 205-212 (the
owner asked for 3-5); host migration (not wanted: the host leaving ends the game); a pause test
in the scenarios (autoplay has no Escape).

**Updated 2026-11-09** against `develop` at `c2a243b6`:

- The Galaxy code anchors in §5 were re-located. The host class has since been split into topic
  files, so the old `GalaxyEggbertGame.cpp:NNNN` numbers were wrong.
- New facts were added: the frame-rate setting (60–240), game speeds down to 0.125×, remote
  Blupis as `SceneModel`s, and the editor's existing team-colour recolouring.
- The AI policy and handover question was added (§2.5, §14), including the case where the owner
  implements multiplayer by hand with AI only as an advisor.
- The file was moved from `openeggbert/plan_gemultiplayer.md` (outside any git repository) to
  `galaxy-eggbert/plans/plan_gemultiplayer.md`. Paths in §0 are absolute and still valid.

Tags used throughout:

- **[V]**: verified by reading the cited file/line in this session.
- **[NV]**: not verified; check it before relying on it.
- **[REC]**: recommendation.
- **[OWNER]**: a decision that belongs to the owner (Robert Vokac), not to the agent.

Read §1–§3 before touching any code, §2.5 and §14 before deciding who writes it. §13 lists what
must be checked first.

---

## 0. Paths

| Name | Path | Notes |
|---|---|---|
| Galaxy Eggbert | `/rv/data/development/github.com/openeggbert/galaxy-eggbert` | the target repository |
| blupi_recon | `/rv/data/development/github.com/openeggbert/blupi_recon` | reference multiplayer (SB1/SB2 port); sibling, do not modify |
| CNA | `/rv/data/development/github.com/libcna/cna` | XNA 4.0 reimplementation; **read-only** for Galaxy work |
| cna-engine | `/rv/data/development/github.com/libcna/cna-engine` | Galaxy's helper modules (Core, Graphics) |
| CNA gamer services server | `/rv/data/development/github.com/libcna/cna-gamer-services-server` | online backend (accounts, relay) |
| mobile-eggbert | `/rv/data/development/github.com/openeggbert/mobile-eggbert` | Galaxy's primary reference; **read-only, never modify** |
| ENet | `~/deps/enet` (shared checkout), also vendored at `cna/third_party/enet` | do not clone again |

[V] Galaxy's `CMakeLists.txt` defaults `CNA_HOME` to `../cna`, but **`openeggbert/cna` does not
exist**. The existing build tree's cache points at the real checkouts
(`build-cna-opengl3/CMakeCache.txt`: `CNA_HOME=/rv/data/development/github.com/libcna/cna`,
`CNA_ENGINE_HOME=/rv/data/development/github.com/libcna/cna-engine`). Reuse that tree. Do not
configure a new one.

---

## 1. TL;DR

1. **Feasible.** The transport is solved by CNA. The real work is in Galaxy: remote players,
   world-event replication, a lobby, and multiplayer 3D maps.
2. **Sync model [REC]: the original SB1/SB2 model.** Each machine simulates only its own Blupi.
   It broadcasts its pose (unreliable, about 20 Hz) and the world events its Blupi caused
   (reliable). The host coordinates the lobby, the start, and the moving-platform/lift dwell
   boundaries. There is **no lockstep, no authoritative server and no rollback.** Reasons are in
   §6.1.
3. **Transport [REC]:** CNA's `Microsoft::Xna::Framework::Net::NetworkSession` with
   `NetworkSessionType::SystemLink`. That is ENet UDP with LAN discovery, already compiled into
   Galaxy's CNA build. **Not** CNA online GamerServices (`PlayerMatch`) for now. **Not** raw ENet
   unless CNA turns out unusable.
4. **Two findings that affect the effort:**
   - CNA networking **requires a signed-in gamer** (GamerServices/Guide). Galaxy uses no
     GamerServices today.
   - A native SystemLink host binds an **ephemeral port**, and there is no public join-by-address
     API. Internet play therefore needs a CNA change, a layer-2 VPN, or the online relay (§6.3).
5. **Estimate (Opus-class agent time, builds included, owner waiting excluded):**
   - MVP: **~19–28 h**.
   - Full: **~37–57 h**.
   - Breakdown in §9.
6. **Prerequisite:** owner decisions in §10. Q1 (scope/source of truth) blocks everything else.
7. **Who implements it, and when, matters as much as how (§2.5, §14).** An AI policy for the
   project (FNA-style, `/rv/data/development/github.com/ai-policy/AGENTS.md`) is planned but not
   introduced. Its final form (v2 or v3) is undecided.
   - Under v2, no AI may change the repository at all; multiplayer would be written by the owner,
     with AI as an advisor only.
   - Under v3, AI may edit within a scope the owner authorizes, but never commit, and the owner
     must understand every change.

   Multiplayer is the hardest part of the game to hand over: network code, plus a permanent
   replication cost on every future gameplay change. **Check the policy's status before writing
   any code.**

---

## 2. Hard constraints for this work

These come from `openeggbert/CLAUDE.md`, `galaxy-eggbert/CLAUDE.md` and the owner's recorded
preferences. Breaking them is worse than not finishing.

### 2.1 Faithful-remake scope [OWNER]

- Galaxy rule: *"Every gameplay feature must trace to confirmed `mobile-eggbert` behavior. Do not
  invent mechanics."* [V] `mobile-eggbert` has **no multiplayer**. A grep of its `src/` for
  `multiplayer|multi-player|network` found only a LICENSE file.
- Multiplayer therefore needs an explicit owner decision that the **Windows originals SB1/SB2**
  are its source of truth, as reconstructed in blupi_recon. Record it in `galaxy-eggbert/CLAUDE.md`
  and `plans/GALAXY_EGGBERT_COMPLETION.md`.
- Every multiplayer rule must then trace to SB1/SB2 code (§3.3), not to plausibility.
- Worlds are **hand-authored tile-based 3D**. There is no 2D→3D conversion or extrusion and no
  2.5D (rejected twice by the owner). SB1 multiplayer maps (`world101..110`) and SB2 maps
  (`world201..212`) give theme and mechanics only.
- Copying code or data from other repositories needs approval. The standing approval (2026-10-25)
  covers *small data tables needed for fidelity, citing the source*, from mobile-eggbert. Do not
  copy blupi_recon code [REC]; re-implement the design and cite file:line.

### 2.2 Repository boundaries

- **CNA is read-only.** Record every CNA bug or limitation in `galaxy-eggbert/upstream_bugs.md`
  (next `UB-NNN`), with a reproduction and the Galaxy workaround.
- cna-engine bugs are fixed in cna-engine (with a test and a CHANGELOG entry), never worked
  around in Galaxy. This applies only when the owner scopes cna-engine in.
- Do not modify blupi_recon, mobile-eggbert or other siblings.
- `InteractionSystem` must stay free of controller, camera and graphics dependencies.
- Engine-independent code lives in `include/GalaxyEggbert/` and `src/GalaxyEggbert/...`. The CNA
  host is `src/GalaxyEggbertCNA/`.
- Game code calls CNA directly. A thin Galaxy adapter around `NetworkSession` is fine. Hiding CNA
  behind a fake engine layer is not.

### 2.3 Build rules (SSD wear is shared by ~10 agents)

```bash
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
cmake --build build-cna-opengl3 --target GalaxyEggbertCNA --parallel 6   # 6 is the max; 4 is the habit
ctest --test-dir build-cna-opengl3 -j4 --output-on-failure
cd build-cna-opengl3 && ./GalaxyEggbert                                     # run from the build dir
```

- Do not create a new build directory (closed list) or a git worktree with its own build tree.
- Do not build in `/tmp` or the scratchpad.
- Do not set `CCACHE_MAXSIZE`, create a ccache, or export `CCACHE_HASHDIR`.
- `build-cna-webgl2/` exists for the web build. Android lives under `android/` (`ANDROID.md`).
  Windows is a MinGW cross build (`WINDOWS.md`).
- Test and autoplay runs must be silent: `SDL_AUDIODRIVER=dummy` (default in `tools/autoplay.sh`
  and `tools/qa_live.sh`).

### 2.4 Working practices

- Commit each discrete task separately. Never add AI attribution (no `Co-Authored-By`, no
  "Generated with"). Push only when asked.
- Update `handoff.md` after every committed task. Update `CURRENT.md` and `plan.md` only where
  their ownership requires it. `NEXT.md` is frozen; do not add to it.
- Do not block on questions. Write them to `plans/audit/owner_questions.md` as
  `### Q-NNN — <question> (<date>)` with options, recommendation and what was done meanwhile.
  Ask interactively only when the owner asks.
- **Other sessions work in the same worktree.**
  - On 2026-11-08 it held another session's uncommitted demo/resume/editor work. That work has
    since been committed.
  - On 2026-11-09 `git status` was clean, but `develop` moves fast (62 commits that day:
    rendering, frame rate, game speed).
  - Re-check `git status` and `git log` before starting. Preserve anything that is not yours.
    Never stage files you did not change.
- Owner-reported fixes are verified in the real campaign worlds from the player's approach, not
  only in a gallery or test world.
- The practices above (commit per task and so on) hold **only until the AI policy is
  introduced** (§2.5). After that, the policy overrides them.

### 2.5 AI policy and handover [OWNER] [V]

The owner plans to introduce an AI contribution policy for Galaxy Eggbert in the style of FNA.

**Status on 2026-11-09:**

- It is **not introduced**. `docs/handbook/08-backlog.md` §1 lists it as open work.
  `plans/audit/manual_review.md` MR-0009 (2026-11-06) records "Zatím ne" ("not now").
- The owner's recorded answers so far (backlog §1):
  - The policy applies to changes made **from its introduction date**. The existing history cannot
    be made to comply.
  - It is to be introduced after the backlog's §3 work is committed.
  - Whether `CLAUDE.md`'s "never add AI attribution" rule stays is open.
- The owner has **not decided** whether the final policy is v2 or v3 (owner, 2026-11-09). The
  backlog quotes v2. **Do not edit the backlog's wording.**

**The two candidate versions** (`/rv/data/development/github.com/ai-policy/`, git history):

| | v2 (`019b172`, 2026-10-31) | v3 (`149d063`, 2026-11-08) |
|---|---|---|
| AI changes the repository | **Never** ("AI must not be used to modify anything in this project"). AI may identify issues, explain code and research. "The solutions … must be authored by humans." | Only within a task or scope a human **explicitly authorizes**. A request for advice does not authorize edits. |
| Commits | (no AI changes at all) | **AI must not create, amend, rewrite or sign commits.** The human commits. |
| Human understanding | Implied: the human is the author | Explicit: the human must understand, explain, verify and validate every change, or remove it |
| Disclosure | The submitter confirms they are the author | Pull requests state whether AI was used, for what, and how it was validated |

**What this means for multiplayer:**

- **Before introduction:** an agent may implement it under today's practices (§2.4). The result
  joins the AI-written history that the policy will not cover, but the successor maintainer
  inherits it (§14).
- **After introduction, under v2:** an agent may **not** implement it. This document becomes an
  advisor's brief and a design document for a human author. It was moved into the repository
  (`plans/plan_gemultiplayer.md`) before the policy, on the owner's instruction of 2026-11-09.
  After introduction under v2, no AI may edit it. Updates are the owner's.
- **After introduction, under v3:** an agent may implement only what the owner authorizes, one
  scoped work package at a time (§9). It hands each package over **uncommitted** with:
  - a review guide;
  - the validation it ran;
  - a proposed commit message.

  The owner commits. Budget the owner's review time (§14).
- In every case, before writing code, read `ai-policy/AGENTS.md` and the backlog to see whether
  the policy has been introduced since this document was written. If it is unclear, ask the owner.

---

## 3. Reference implementation: blupi_recon multiplayer

### 3.1 What it is

[V] blupi_recon reconstructs Speedy Blupi 1/2 and Speedy Eggbert 1/2 from the original binaries.
Its `port/` is a portable build of those games.

Port 1.2.0 shipped native multiplayer for SB1 and SB2:

- 2–4 players.
- Windows, Linux and Android interoperate for the same edition.
- LAN discovery.
- Direct join by `--net-join HOST[:PORT]`.

**It is not a new multiplayer design.** The original games already had DirectPlay multiplayer.
The port kept all historical game logic unchanged (`CEvent` lobby, `CDecor::TreatNetData`,
`NetMessage` producers) and replaced only DirectPlay underneath the historical `CNetwork`
interface. That is why it was cheap. **Galaxy has none of that logic.**

### 3.2 The original synchronisation model [V]

Source: `blupi_recon/port/docs/MULTIPLAYER.md` and `MULTIPLAYER_ASSESSMENT.md` §1–§5.

- **Topology:** DirectPlay 5 peer session, at most 4 players, KEEPALIVE | MIGRATEHOST requested.
  The game never handled migration: `IsHost` is cached at create/join.
- **Authority:**
  - Each peer simulates locally and broadcasts **its own Blupi's pose and the events it
    produced**.
  - The host runs the lobby table, the start, and the moving-object (lift) dwell boundaries.
  - There is neither input lockstep nor a host-authoritative world.
- **Rates:**
  - Tick 50 ms (20 Hz).
  - One MOVE bundle per tick, or every other tick in "packed" mode.
  - Nothing is sent when the pose is unchanged and the queue is empty.
  - At most 5 embedded 8-byte events per bundle; event queue of 20. A full queue silently drops
    events (a historical defect).
- **Reliability:**
  - Any nonzero send flag means guaranteed.
  - Lobby traffic, ABANDON/LOST/DIE/PAUSE and any MOVE bundle carrying a decor change, a cloud
    stop or a non-transient object start/delete are reliable.
  - Plain pose bundles, sounds, lift synchro and transient effects are unreliable.
- **Ordering:**
  - An old MOVE pose (timestamp not newer) is ignored.
  - **Events embedded in an old packet are still applied.**
  - There is no general event sequencing, no de-duplication, no snapshot and no repair.
- **Prediction:** `DoNetSmooth` extrapolates a missing remote pose for at most 5 calls, then stops.
- **Interaction between players [V]:**
  - Remote Blupis are **only drawn** (`src/decblock.cpp:1252-1282`, `m_netBlupiPos/Icon/Sec`).
  - I found no physical collision between the local Blupi and remote Blupis.
  - Players interact only through shared world objects and decor.

### 3.3 Original multiplayer game rules [V unless marked]

All references are under `blupi_recon/src/` (SB1). SB2 equivalents are under `games/sb2/src/`.

| Rule | Evidence |
|---|---|
| Multiplayer lives = **10** (single player 3) | `decblock.cpp:972-985` (`PlayPrepare`) |
| Start position and direction by **team**: `m_blupiStartPos[m_team]` | `decblock.cpp:976-981`, `decor.cpp:203-240` |
| Teams 0..3. Team 0 uses the standard Blupi channel 2; teams 1..3 use channels `CHBLUPI1 + team - 1` (coloured Blupi sheets) | `decor.cpp:211-224`, `decblock.cpp:1155-1168` |
| Personal-bomb ("perso") icon per team: `0x6c + team` | `decblock.cpp:1171` |
| Personal bombs are per team: `ObjectStart(TYPE_BOMBEPERSO1 + m_team)` | `decblupi.cpp:1860-1897` |
| A player can pick their **own** team's bomb back (if `m_blupiPerso < 5` and `ACTION_DOWN`) | `decblupi.cpp:3183-3192`. Effect of other teams' bombs **[NV]** |
| Bomb types 1..4 have their own movement and icon handling | `decmove.cpp:637-655`, `:1603` |
| Some object types are handled specially in multiplayer: `TYPE_CAISSE` (crate), `TYPE_GOAL`, `TYPE_CLE` (key), `TYPE_BLUPIHELICO`, `TYPE_BLUPITANK` | `decblock.cpp:1047-1052`. The body was **not read [NV]**; it probably removes them. Read it. |
| Death in multiplayer always restarts (no game over by restart flag) | `decblupi.cpp:3503` |
| Lives exhausted: `PK_LOST` sent, player removed | `decblupi.cpp:3510-3519`, `decmove.cpp:196-203` |
| **Win:** when fewer than 2 players remain present, the remaining one wins (`ACTION_WIN`, sound 14, string 287) | `decblupi.cpp:3521-3550` |
| Lobby: 4 slots. Teams cycle 0..3 and need **not** be unique. Host START is enabled only when all present players are ready and more than one is ready. Chat. Host chooses the map: SB1 1..10 (+100), SB2 1..12 (+200). No map transfer (both must have it) | assessment §3, `event.cpp:915-974, 955-967, 4108-4135` |
| Remote Blupi drawn unless icon is -1, icon is in 276..284, or `SEC_HIDE` | `decblock.cpp:1256` |
| Host-only lift synchro: ASCENSEURSTART/END at dwell boundaries | `decmove.cpp:520-528, 569-577` |
| Clock wraps at 32768 ticks; pose acceptance uses plain `time > previous` (historical defect) | `decblock.cpp:1545-1546`, `decmove.cpp:130` |
| Cloud-stop producer passes the player **slot**; the receiver compares it with its **team** (historical defect when team ≠ slot) | `decblupi.cpp:3302-3307`, `decmove.cpp:181` |
| Multiplayer save/load is disabled | assessment §3 |

**[NV] The match goal.** The code read shows "last player standing wins". `TYPE_GOAL` being
special-cased suggests there may be more (for example a goal or treasure race). Before designing
Galaxy's rules, read:

- `decblock.cpp:1040-1060`;
- the `TYPE_GOAL` handling in `decblupi.cpp`;
- `blupi_recon/docs/SB2.md` lines 250 onward;
- the multiplayer map data.

### 3.4 Original wire messages [V]

Full tables with every check: `MULTIPLAYER_ASSESSMENT.md` §2 and §7.

**Lobby (all reliable):**

| Message | Size | Content and direction |
|---|---|---|
| JOIN | 28 B | DPID, name[20]; client → all |
| LEAVE | 12 B | DPID, host bool; host = true closes everyone |
| LOBBY | 132 B | map, 4 × NetPlayer{present, ready, DPID, team, name[22]}; host → all |
| TEAM | 12 B | client team change |
| READY | 12 B | client ready toggle |
| START | 2 B | host start |
| CHAT | 108 B | text[100] |
| CLOSE | ? | receive-only |

**Gameplay:**

- MOVE: 20 + 8n bytes, n ≤ 5. Header: size, type, keyPress, time, x, y, icon, sec, channel,
  transport (lift index, signed char), flag, nbMessages. Then the embedded events.
- ABANDON 4, LOST 4, DIE 4 (lives), PAUSE 3. All reliable.

**Embedded events (8 B each):**

| Code | Event | Content | Reliability |
|---|---|---|---|
| 20 | OBJECTSTART | speed, x, y, type | reliable except a transient-effect list |
| 21 | OBJECTDELETE | x, y, type | same as 20 |
| 30 | MODIFDECOR | x, y, tile icon | always reliable |
| 40 | PLAYSOUND | x, y, sound | — |
| 41 | STOPSOUND | sound | — |
| 50 / 51 | ASCENSEURSTART / ASCENSEUREND | object index; **host only** | — |
| 60 | STOPCLOUD | rank | always reliable |

Note: ObjectStart/ObjectDelete identify objects by **position + type**, not by an ID.

### 3.5 Port design choices worth copying (as design, not code)

[V] `port/docs/MULTIPLAYER.md` "Port design":

- **ENet** (1.3.18, vendored unmodified). Star topology: clients connect to the host, and the host
  relays each client's payload to the others with the original origin id and **never echoes it
  back**.
  - Channel 0: reliable, ordered per origin.
  - Channel 1: unreliable **unsequenced**, so late bundles still deliver their events.
- **Envelope:** game protocol id, protocol version, session epoch, origin member id (taken from the
  connection, **never from a client-written field**), payload length.
- **One atomic validator per game.** Exact lengths, sender owns the ids it names, range checks,
  NUL-terminated strings, event counts. Rejected packets are counted and logged, never passed on.
- **Content fingerprint:** a 64-bit hash of game, edition and multiplayer maps, checked at join.
  Mismatches are refused.
- **Host migration unsupported.** Host loss generates trusted LEAVE/ABANDON locally.
- **Discovery:** LAN broadcast on port + 1. Default game port 28640, discovery 28641.
- **Android:** INTERNET permission. A Wi-Fi multicast lock is held while in the foreground.
- **Deterministic tests:** a test-only ingress gate releases messages at tick boundaries in a fixed
  order. Two headless instances driven by scripts then repeat byte-for-byte per peer. Normal play
  never waits.

### 3.6 Cost of the blupi_recon work [V] (git log)

All on 2026-10-25, after the static assessment was written:

| Time | Step |
|---|---|
| 10:42 | ENet vendored; design note and assessment committed |
| 11:46 | SB1 two-instance pilot working |
| 13:46 | Cross-platform wire guards, content handshake, LAN discovery |
| 13:50 | Android permissions and multicast lock |
| 13:55 | Four players (host relay) |
| 16:43 | SB2 multiplayer |

- Port 1.2.0 release notes followed on 2026-10-26.
- Code: `port/common/net/` is 1,044 lines (`cnetwork.cpp` 275, `game_protocol.*` 270,
  `session.*` 499).
- Tests: `port/tests/net_session.py`, `test_net_protocol.cpp`, `test_net_transport.cpp`,
  `tests/net/*.input`.
- Owner tests: SB1 Android ↔ Windows worked (2026-10-25); SB2 local builds worked (2026-10-26).

So about **6 hours** covered transport, protocol, validator, tests and platforms **for a game whose
multiplayer logic already existed**. In Galaxy this corresponds roughly to work packages WP1 and
WP8 only (§9).

### 3.7 Lessons and hazards to carry over

- **Validate every packet atomically before it touches game state.** The original's parsing is
  unsafe: actual length ignored, unchecked indices, `strcpy` (assessment §7, §9 risk 1).
- **Identify the sender from the connection,** never from payload fields.
- **Do not echo** a relayed packet back to its origin, or effects and chat duplicate.
- **Remote-applied events must not be re-broadcast.** The original's `ObjectStart(..., FALSE)` is
  the "do not send" flag.
- **Use one reliable ordered stream per origin.** Create-before-delete ordering matters.
- **Use a content fingerprint and a protocol version;** refuse mismatches before START.
- **Buffer gameplay packets arriving before the receiver enters play.** Do not misparse them as
  lobby messages.
- **Do not reproduce historical defects:** 32768-tick wrap, slot/team cloud ambiguity, silent
  queue overflow. They were port-fidelity questions in blupi_recon. In Galaxy they are simply bugs
  to avoid; note that choice in `plans/audit/behavior_differences.md`.
- **Cross-peer full-state hashes are not equal by design.** Only per-peer repeatability is
  testable.

### 3.8 Files to read in blupi_recon

- `port/docs/MULTIPLAYER.md` (129 lines: status, design, milestones).
- `port/docs/MULTIPLAYER_ASSESSMENT.md` (384 lines: every call site, message tables, validation
  rules, risks).
- `port/common/net/{session,cnetwork,game_protocol}.*`.
- `port/tests/net_session.py`, `port/tests/test_net_protocol.cpp`.
- Original logic:
  - `src/decmove.cpp:20-370` (send/receive, smoothing, lift sync);
  - `src/decnet.cpp` (event producers);
  - `src/decor.cpp:195-280` (`SetMulti`);
  - `src/decblock.cpp:970-990, 1040-1060, 1150-1175, 1250-1285`;
  - `src/decblupi.cpp:1860-1900, 3180-3200, 3490-3555`;
  - `src/event.cpp` (lobby phases; line table in assessment §3).

---

## 4. CNA networking stack (what Galaxy would use)

### 4.1 Build state [V]

- `build-cna-opengl3/CMakeCache.txt` contains `CNA_ENABLE_NET=ON`,
  `CNA_ENABLE_GAMER_SERVICES=ON`, `CNA_ENABLE_VOICE=AUTO` and `CMAKE_BUILD_TYPE=Release`.
  `CNA_ENABLE_NET` defaults ON (`cna/CMakeLists.txt:180`) and requires GamerServices
  (`:181-190`).
- Galaxy links the `CNA` aggregate target (`galaxy-eggbert/CMakeLists.txt:221`).
- The current `build-cna-opengl3/GalaxyEggbert` (11.5 MB) contains **no**
  `NetworkSession` symbols (`nm -C` count 0). Static linking dropped the unused objects, so **the
  first use pulls in** `CNA_Net`, `CNA_GamerServices`, `enet` and their dependencies.
- **[NV] check:**
  - the link (libcurl, OpenSSL, nlohmann headers);
  - binary size;
  - the MinGW cross build;
  - the Android build. `modules/net/CMakeLists.txt`: libcurl is required on non-Emscripten except
    Android, where the service/relay transport is then unavailable.
- **[NV]** Voice is `AUTO`. With libopus found, Opus voice is compiled in. Galaxy probably wants
  `CNA_ENABLE_VOICE=OFF`, which is a cache change in the existing tree, not a new tree.

### 4.2 API surface [V]

Headers are in `cna/modules/net/include/Microsoft/Xna/Framework/Net/`.

- `NetworkSession::Create(NetworkSessionType, int maxLocalGamers, int maxGamers)`.
  - An overload adds `privateGamerSlots` and `NetworkSessionProperties`.
  - Another takes an explicit `std::vector<SignedInGamer*>`.
  - Async: `BeginCreate` / `EndCreate` (`NetworkSession.hpp:574-684`).
- `NetworkSession::Find(type, maxLocalGamers, NetworkSessionProperties)` returns
  `AvailableNetworkSessionCollection` (async: `BeginFind` / `EndFind`).
- `NetworkSession::Join(const AvailableNetworkSession*)` (async: `BeginJoin` / `EndJoin`).
- `session->Update()` must be called every frame. Events are raised from it: `GamerJoined`,
  `GamerLeft`, `GameStarted`, `GameEnded`, `HostChanged`, `SessionEnded`.
- `LocalNetworkGamer::SendData(PacketWriter&, SendDataOptions [, NetworkGamer* recipient])` and
  byte-vector overloads. Receive with `ReceiveData(PacketReader&, NetworkGamer*& sender)` and
  `getIsDataAvailableProperty()` (`LocalNetworkGamer.hpp:34-192`).
- `SendDataOptions`: `None`, `Reliable`, `InOrder`, `ReliableInOrder`, `Chat`; flags.
  `SendDataOptions.hpp`.
- `MaxSupportedGamers = 31` (`NetworkSession.hpp:77`). Galaxy needs 4.
- **Ownership:** a returned `NetworkSession*` is caller-owned. Call `Dispose()`, then `delete` it
  (`NetworkSession.hpp:60-75`).
- **[NV]** Maximum payload size per `SendData`. Galaxy packets are tiny (< 100 B), so this is
  probably irrelevant, but check `NetPacketCodec.hpp`.
- **[NV]** How many integer slots `NetworkSessionProperties` has (XNA: 8). Use them to advertise
  protocol version, simulation revision and a world/content hash, so incompatible sessions are not
  listed.

### 4.3 Sign-in requirement [V]

This was **not** in the first chat estimate.

- Net sessions are made of signed-in gamers: `cna/CMakeLists.txt:187-190`, and
  `cna/modules/graphics/examples/common/SignInEXT.hpp`.
- Without a configured CNA service, the Guide signs in **local offline profiles**.
- The demos follow this pattern:
  1. add `GamerServicesComponent` to `Components` in the game constructor;
  2. each `Update`, if `Gamer::getSignedInGamersProperty()` is empty, call
     `Guide::ShowSignIn(1, false)` once;
  3. after sign-in, create or join the session.
- For unattended runs and tests, set `CNA_GAMER_SERVICES_AUTO_SIGN_IN=<profile name>`.
- [V] Galaxy currently uses **no** GamerServices (grep of `src/GalaxyEggbertCNA/` is empty).
- [OWNER] The Guide is drawn by CNA over the game in **CNA's own Xbox-360-like look**. This is UX
  that the original never had. Option: show the Guide only when the player picks "Multiplayer",
  or auto-create a local profile named after the Galaxy player name, if CNA allows creating a
  profile programmatically **[NV]**.
- Risk: adding `GamerServicesComponent` must not change single-player behaviour. For example, it
  must not show the Guide at startup or consume input. Add it lazily, or verify with
  `verify_demos.sh` and golden frames.

### 4.4 SystemLink specifics [V]

- Transport: ENet UDP (`src/Internal/ENetBackend.cpp`, 1,925 lines).
- **LAN discovery** by UDP broadcast on `kDiscoveryPort = 61190`
  (`src/Internal/ENetDiscoveryService.cpp:40`), with QoS measurement. Several processes on one
  machine share the discovery port via `SO_REUSEADDR` / `SO_REUSEPORT` (`:237-270`), so two local
  instances work.
- **The native host binds an ephemeral port** (`ENetHostHandle::CreateHost(0, …)`,
  `ENetBackend.cpp:1609-1617`). Emscripten uses a fixed 61191.
- Join connects to the address and port from the discovery reply. A manually constructed
  SystemLink `AvailableNetworkSession` without discovery data stays **disconnected**
  (`NetworkSession.cpp:1898-1905`).
- `AvailableNetworkSession::CreateInternal(..., hostAddress, hostPort, type)` exists and is marked
  `CNAEXT`, but is documented "for CNA internal use" (`AvailableNetworkSession.hpp:122-132`).
  Using it for direct-IP join would depend on an internal API **and** on knowing the ephemeral host
  port. **Do not rely on it without the owner/CNA agreeing.**
- **Host migration** is supported in SystemLink. The CNA policy for same-machine host replacement
  is in `docs/gamer-services-known-limitations.md:285`.
- The comment at `NetworkSession.hpp:183` states a **star topology** ("a star topology gives
  surviving clients no direct channel"). Verify whether the host relays client-to-client packets
  transparently **[NV]**. The XNA API suggests yes; data sent to all gamers reaches all.
- Voice is routed automatically when libopus is compiled in.
- Examples to imitate, under `cna/modules/net/examples/`:
  - `demo_net_avatar_sync/src/SyncGame.cpp` (391 lines): SystemLink host/join from `--host` /
    `--join`, sign-in, reliable description every 120 frames, unreliable pose every frame.
  - `demo_net_client_server_arena`, `demo_session_browser`, `demo_session_lifecycle_events`,
    `demo_simulated_network_conditions` (CNA has a `NetworkSimulator` for latency and loss).
- Platforms per CNA `README.md` §7:
  - Linux: native, including a two-process loopback test.
  - Windows: WinSock2, MinGW, verified under Wine.
  - Android: NDK, verified on an x86_64 emulator.
  - **Web: no multiplayer.** No discovery, no service transport, no relay. This is the owner's
    decision of 2026-09-28 (`docs/browser-network-readiness.md`).

### 4.5 Online: GamerServices and `cna-gamer-services-server` [V]

- `NetworkSessionType::PlayerMatch` / `Ranked` go through the CNA service's session directory.
  **Every datagram is carried by the service's authenticated TLS/WSS relay.** There are no direct
  peer-to-peer connections.
- Server version 0.1.0. Dependencies: OpenSSL ≥ 3, Boost (Beast), SQLite, nlohmann/json. Needs a
  certificate. Linux is the only tested host.
- **Accounts are provisioned by the administrator.** There is no self-registration or password
  change over the wire.
- Public-internet deployment is "not independently validated"
  (`cna/docs/gamer-services-known-limitations.md` §31).
- Benefits if ever wanted: friends, invitations, parties, leaderboards, achievements, online host
  migration.
- Because the XNA API is the same, moving from SystemLink to PlayerMatch later should mostly be a
  session-type and sign-in change **[NV]**.

---

## 5. Galaxy codebase facts relevant to multiplayer

### 5.1 Size and single-player assumptions [V]

- **The host class is split by topic** (as of 2026-11-09; on 2026-11-08 it was one 9,415-line
  `GalaxyEggbertGame.cpp`). `src/GalaxyEggbertCNA/` holds 12,801 lines in all.

  | File | Lines | What |
  |---|---|---|
  | `GalaxyEggbertGame.hpp` | 1,431 | the class `GalaxyEggbertGame final : public Microsoft::Xna::Framework::Game` (`:78`) |
  | `GalaxyEggbertGame.cpp` | 1,437 | constructor, input, vehicles, sounds, cheats, `ApplyFrameRate` |
  | `GalaxyEggbertGameUpdate.cpp` | 3,388 | `Update` and `UpdateTick` with its steps |
  | `GalaxyEggbertGameScene.cpp` | 1,479 | `BuildSceneFrame`: what the world draws |
  | `GalaxyEggbertGameDraw.cpp` | 922 | `Draw`: the frame |
  | `GalaxyEggbertGameLoad.cpp` | 1,143 | content loading |
  | `GalaxyEggbertGameDemo.cpp` | 1,145 | demos, demo menu |
  | `GalaxyEggbertGameMissions.cpp` | 708 | missions, title scene, custom worlds |
  | `GameHelpers.hpp` | 789 | shared file-local helpers (with `GameInternal.hpp`) |

  - One `GalaxyEggbert::Game::BlupiController blupi_` (`.hpp:280`). The pattern `blupi_\b` occurs
    **647 times** across the host `.cpp` files.
  - One `GalaxyEggbert::Game::InteractionSystem interaction_` (`.hpp:355`), 150 occurrences.
  - One `WorldRuntime worldRuntime_` (`.hpp:200`), 236 occurrences.
  - `galaxy-eggbert/CLAUDE.md` "Source layout" describes the split and is authoritative if this
    table drifts again.
- `src/GalaxyEggbert/Game/BlupiController.{hpp,cpp}`: 1,801 + 3,376 lines. Holds the player's
  physics and animation state: position `m_x/m_y/m_z`, `m_yaw`, `m_velocityY`, `m_onGround`,
  `m_animPhase`, `AnimState`, `VehicleMode {None, Helicopter, Jeep, Tank, Skateboard, Overcraft}`
  (`.hpp:703`), shield, balloon, ecrase, secret-power timers, and more.
- `src/GalaxyEggbert/Game/InteractionSystem.{hpp,cpp}`: 1,188 + 4,121 lines.
  - `Update(float dt, WorldRuntime&, float blupiX, float blupiY, float blupiZ, float blupiMoveDX, Sound&, …about 20 flags…)`
    (`.hpp:227`) takes **exactly one Blupi**.
  - It owns lives, keys, eggs, treasures, bullets, dynamite, "perso" counts, voyage (life-loss and
    reward animations), events (`EventKind`, `.hpp:340`), and `std::mt19937 rng_{0}` (`.hpp:1117`)
    with `SeedRandom` (`.hpp:145`).
- `src/GalaxyEggbert/Game/WorldRuntime.hpp` (385 lines) holds `MobileObjSpec` (`:24`): type,
  start/end path, `currentX/Y/Z`, `active` flag, phase, patrol timing. These are the world's
  moving and interactive objects, loaded from `.vwr`. Their vector order is deterministic from the
  file, which makes it a candidate for stable network object ids **[NV]**.
- `src/GalaxyEggbert/Game/EnemyBlupi.hpp`: enemy Blupis (ObjectType 32 helicopter, 33 tank).
  - Not remote players; do not confuse them.
  - **Useful precedent:** `RecolorEnemyBlupi` recolours Blupi's textures with a 25° HSV hue
    shift to match the original's `blupi1.png`. Team colours for remote players could be made the
    same way, if the original's team sheets differ only in hue **[NV]**.
- "Perso" already exists in Galaxy: `InteractionSystem::PersoCount()`, `TryPerso(...)`
  (`.hpp:789-790`), `CheatSetPerso`. Multiplayer needs **per-team** perso types (SB1
  `TYPE_BOMBEPERSO1..4`). Check whether Galaxy's ObjectType enum already has them **[NV]**
  (`include/GalaxyEggbert/Def/ObjectType.hpp`, `ObjectDefinitionRegistry`).

### 5.2 Main loop and entry points [V]

Line numbers are as of `c2a243b6` (2026-11-09). Re-locate them by function name.

| What | Where |
|---|---|
| `Update` (per loop pass: frame-rate interpolation gate, demos, capture modes, autoplay, input capture, then `UpdateTick`, then resume/demo logging) | `GalaxyEggbertGameUpdate.cpp:83` |
| `UpdateTick` (one 1/60 s simulation step) | `GalaxyEggbertGameUpdate.cpp:287`; `simDt = dt * GameSpeedFactor(gameSpeed_)` at `:394`, then `worldRuntime_.Update(simDt)` |
| Tick steps | `UpdateTickScreens` `:456`, `UpdateTickWorld` `:1089`, `UpdateTickActions` `:1757`, `UpdateTickCamera` `:2569`, `UpdateTickEvents` `:2942`, `UpdateScreenPlay` `:3186` (all `GalaxyEggbertGameUpdate.cpp`). **Network receive belongs before the world step and send after the events step** (§7.2 step 6). |
| `Draw` | `GalaxyEggbertGameDraw.cpp:76` |
| `BuildSceneFrame` | `GalaxyEggbertGameScene.cpp:92`. `AppendSceneBlupi` is at `:463` (called at `:123` opaque and `:136` translucent); `AppendSceneEnemyBlupis` at `:612` |
| `LoadMission(int, const std::string* validatedBytes)` | `GalaxyEggbertGameMissions.cpp:226`. Resets `interaction_` and `blupi_`; seeds the mission from the resume/demo seed or `std::random_device` (re-locate the seed lines inside) |
| Title scene and menus | `LoadTitleScene()` / `UpdateTitleScene(float)` (`GalaxyEggbertGameMissions.cpp:547, 574`; `.hpp:616-617`); demo menu `UpdateDemoMenu()` (`GalaxyEggbertGameDemo.cpp:725`); idle demo after `kDemoIdleSeconds = 40` (`DemoReplay.hpp`) |
| Custom worlds | `LoadCustomWorldForEditing`, `LoadCustomWorldForPlayTest` (`GalaxyEggbertGameMissions.cpp:345, 382`) |
| Blupi model transform | `BlupiModelWorld(const CharredShape&)` (`.hpp:796`) |
| Engine-independent input | `InputPad` |
| Command line | `src/GalaxyEggbertCNA/main.cpp` (139 lines). Existing flags: `--golden-capture`, `--golden-capture-trace`, `--mission`, `--autoplay`, `--record-demo`, `--demo-ticks`, `--demo-title`, `--verify-demo`, `--galaxy`, `--spawn`. Add `--net-host` / `--net-join` here. |

- [V] There is **no multiplayer menu, phase or state** anywhere.
- [NV] How menu screens are structured, as functions or as a state enum. Read around
  `UpdateTitleScene` and the pause/setup screens before designing the lobby.
- **Rendering a second Blupi.**
  - [V] The world is drawn by `src/GalaxyEggbert/Game/WorldRenderer` from a `SceneFrame`
    (`src/GalaxyEggbert/Game/SceneFrame.hpp:190`). Skinned models are `SceneModel`s (`:104`): a
    model, a clip and time, a world matrix, alpha and translucency, a tint, and per-part texture
    overrides.
  - Blupi, vehicles and enemy Blupis go into `opaqueModels` (`:207`); a translucent Blupi goes into
    `translucentModels`.
  - **A remote Blupi is therefore "one more `SceneModel`"**, as the enemy Blupis already are
    (`AppendSceneEnemyBlupis`, `GalaxyEggbertGameScene.cpp:612`). The renderer does not need to
    change.
  - [NV] `AppendSceneBlupi` (`GalaxyEggbertGameScene.cpp:463`) builds the local Blupi's model from
    game-class members: face texture, lean, charred shape, mockery variant, vehicle, ghost/hide.
    To draw N Blupis, factor that into a function taking a plain pose/visual struct.
  - **This refactor touches single-player rendering.** Guard it with
    `tools/verify_golden_frames.sh` and a screenshot check.
- **[V] Team colours already exist in the editor.**
  - `Editor/PlayerStartColor.hpp:12` `RecolorPlayerStartPixel(color, r, g, b)` recolours only
    Blupi's yellow body material to orange, blue or green. Eyes, eyelashes and shoes keep their
    colours.
  - The colours are measured from the established `Content/icons/button.png` icons 96–98.
  - The editor uses it to draw a coloured Blupi model at each player start. It caches the recoloured
    textures in `editorStartTextures_` (`GalaxyEggbertGameScene.cpp` around `:1135-1175`;
    `.hpp:1247`).
  - Remote players can reuse this: no new asset set, only derived textures.
  - [NV] Whether those button icons are the original's multiplayer team colours (compare with
    `CHBLUPI1..3` in blupi_recon). This narrows Q9.

### 5.3 Simulation and determinism [V]

- **One simulation step is always 1/60 s.** `UpdateTick` uses `constexpr float dt = 1.0f / 60.0f`
  (`GalaxyEggbertGameUpdate.cpp`, top of `UpdateTick`). Reference logic ticks at the original's
  20 Hz (`MobileObjSpec::phase`, patrol timing).
- **How often a step runs depends on mode** (new since 2026-11-09):
  - Normal play: CNA fixed timestep at 60 Hz (`GalaxyEggbertGame.hpp:101` comment).
  - **Setup frame rate 61–240** (`a75c642a`, `ccd7906c`): the loop runs at that rate. The game still
    steps once per 1/60 s of it, and the frames between draw the camera, Blupi and every moving
    object interpolated (`Update`, `GalaxyEggbertGameUpdate.cpp:107-130`; `ApplyFrameRate`,
    `GalaxyEggbertGame.cpp:407`). It is "one step per pass, no catching up", so a machine that
    cannot keep pace runs the simulation slower than real time.
  - Autoplay, captures, demo verification and resume replay step once per loop pass, unthrottled
    (`FrameRateInterpolates`, `GalaxyEggbertGame.cpp:400`; `EnableAutoplay` `:266-271`).
  - **Consequence for multiplayer:** tick counters on two machines are not a shared clock. Use them
    only to order one sender's packets (as the original did), never to align machines. Lift sync
    must come from host events, not from equal tick numbers. The frame-rate setting itself is local
    and harmless: interpolation only draws, and remote poses should be interpolated the same way.
- **Game speed** (`include/GalaxyEggbert/Def/GameSpeed.hpp`):
  - Values: `Slow` 0.5 (the original's Tab), `SuperSlow` 0.25 and `UltraSlow` 0.125 (Galaxy's,
    owner 2026-11-09, `fcc1a627`), plus `Fast` / `Faster` / `Fastest` (×2/×4/×8, F7/F8 and the
    "quick" cheat).
  - They scale `simDt` for the core simulation only.
  - **In a session, one player at 0.125× and another at ×8 would see the world drift apart.**
    [REC] Lock the game speed to Normal in multiplayer and ignore Tab and the speed keys there.
    [NV] What the original did with speed and cheats in multiplayer (Q11).
- Floats are used everywhere in movement.
- `ResumeLog` records the start state, seed and raw input of each 1/60 s tick. `DemoReplay` ships
  demos (`Content/demos/demo001..003.gdemo`) with `kDemoSimulationRevision = 1` and an end-state
  signature.
- Per `DemoReplay.hpp`: *"Resume files are refused by any other build, since replaying them needs
  the identical binary."* **Bit-exact determinism holds only within one binary.** Cross-platform
  (Linux, Windows, Android, different compilers) lockstep would desync. This is the main argument
  against lockstep (§6.1).
- `ctest` runs `VerifyDemoReplays` via `tools/verify_demos.sh`. This is **the regression guard
  for single-player simulation.** Any change that alters the RNG draw order, update order or
  physics in single player shows up there. Never bump `kDemoSimulationRevision` or re-record demos
  to make multiplayer changes pass.

### 5.4 Player starts [V]

- `include/GalaxyEggbert/PlayerStart.hpp`:
  `enum class PlayerStartColor : std::uint8_t { Yellow, Orange, Blue, Green }` (Eggbert 2
  `PlaceDecor10` / decblock BUILD: four separately coloured starts). Single player uses yellow.
  - Yellow keeps the world header spawn fields.
  - Orange, Blue and Green are sparse block metadata **type 7**, one byte with bits 1/2/3.
  - Per-colour 2-bit headings are metadata **type 8**.
  - `CollectPlayerStarts(world)`, `PlayerStartYaw(start)`.
- The editor already places, selects and turns all four. Recent commits: `b70f05c0`,
  `716723e4`, `f8ac5ce9`.
- **This is the multiplayer spawn model.** Mapping team 0..3 to Yellow/Orange/Blue/Green matches
  the original's `m_blupiStartPos[team]`, if the original's team order is the same colour order
  **[NV]**. Check `blupi_recon` channel/team colours.

### 5.5 Tests and tools [V]

- `CMakeLists.txt` has 24 `add_test` calls. `ctest -N` in `build-cna-opengl3` lists **120
  tests** (2026-11-09; the backlog's 112 is from 2026-11-07). The tests include:
  `VerifyInteractionSystem`, `VerifyGEInputPad`, `VerifyBlupiMovement`, `VerifyGESaveData`,
  `VerifyShippedWorlds`, `VerifyAutoplay`, `VerifyDemoReplay`, `VerifyDemoFiles`,
  `VerifyDemoReplays`, editor verifiers, and render-mapping verifiers.
- Pattern: pure-logic verifier executables in `tools/Verify*.cpp`. Add `tools/VerifyNetProtocol.cpp`
  in the same style.
- Tools: `tools/autoplay*.sh|py`, `qa_live.sh`, `qa_sweep.sh`, `play_pass.sh`,
  `verify_golden_frames.sh`, `verify_golden_trace.sh`, `campaigncheck.py`, `levelcheck.py`,
  `worldkit.py`, `vwr_inspect.py`, `record_demo.sh`.
- **[NV]** Whether Galaxy can run without a window (headless) for a two-process test. CNA has a
  headless build variant, but Galaxy's tree is OpenGL ES 3. Autoplay presumably runs windowed.
- **[NV]** Where Galaxy stores saves and settings. Two instances on one machine may overwrite each
  other's files. blupi_recon used separate user roots per test instance. Use separate
  `HOME` / `XDG_*` dirs (or the Galaxy equivalent) in tests.

### 5.6 Platforms [V]

- Desktop Linux, Windows (MinGW cross), Android (`android/`), Web (`build-cna-webgl2/`, `web/`).
  The web build is planned for release (`plans/audit/owner_questions.md` Q-015).
- `android/app/src/main/AndroidManifest.xml` declares only `android.permission.VIBRATE`.
  Multiplayer needs `INTERNET` and, for LAN discovery, `CHANGE_WIFI_MULTICAST_STATE` plus a
  multicast lock around discovery.
  - blupi_recon's Android notes apply: target 37 needs `ACCESS_LOCAL_NETWORK`; app backgrounding
    drops the session after the ENet timeout. See `MULTIPLAYER_ASSESSMENT.md` §6 "Android".
  - Which side of JNI acquires the lock in a CNA/SDL app is **[NV]**.
- **Web: multiplayer is unavailable** (CNA refuses). The menu entry must be hidden or disabled
  under Emscripten, and the web build must still compile.

---

## 6. Options

### 6.1 Synchronisation model

| Option | What it means here | Verdict |
|---|---|---|
| **A. Original peer model:** own Blupi local; broadcast pose + events; host coordinates lobby, start and lifts | Matches SB1/SB2 exactly (faithful); no single-player refactor of the simulation; tolerant of float differences across platforms | **[REC]** |
| B. Deterministic input lockstep | All peers run the full simulation from all inputs. ResumeLog/DemoReplay infrastructure exists, **but determinism is per-binary only** (§5.3), so Linux ↔ Windows ↔ Android would desync. The simulation would need N Blupis inside `InteractionSystem`/`GalaxyEggbertGame`, i.e. a deep refactor. Input latency on the internet. Not what the original did. | Reject |
| C. Host-authoritative server simulation | Host simulates everyone; clients send input and render snapshots. Needs N-Blupi simulation (same refactor as B) and snapshot/interpolation of the whole world. Not faithful. | Reject |
| D. Rollback (GGPO-style) | Needs B's determinism plus state save/restore every frame. | Reject |

Consequence of A: two players can pick up the same egg in the same instant on different machines,
and both get it. The original has the same race. Document it rather than "fixing" it with
authority.

### 6.2 Transport

| Option | Pros | Cons | Verdict |
|---|---|---|---|
| **CNA `NetworkSession` SystemLink** | Already compiled into CNA; XNA API (Galaxy's direction); ENet (same as blupi_recon); LAN discovery, QoS, host migration, reliable/unreliable; Linux/Windows/Android verified by CNA; CNA owns transport bugs | Requires GamerServices sign-in (§4.3); no public direct-IP join and an ephemeral host port (§4.4); Galaxy cannot fix CNA bugs itself | **[REC]** |
| CNA `PlayerMatch` (online via cna-gamer-services-server) | Internet play through relay with NAT traversal; invitations, friends, leaderboards | Needs a hosted TLS server, admin-provisioned accounts, all traffic through the relay; server 0.1.0, public deployment unvalidated; ops burden | Later, optional |
| Raw ENet in Galaxy (like blupi_recon) | Full control; fixed port and `--net-join` trivial; no sign-in | Bypasses CNA (against Galaxy's direction); duplicates CNA's transport; Galaxy then owns reliability, discovery and host loss | Fallback only, if CNA SystemLink proves unusable; ask the owner |
| SDL_net / own UDP | — | UDP alone cannot provide reliable ordered delivery; reinvents ENet | Reject |

### 6.3 Internet play (beyond LAN)

| Option | Notes |
|---|---|
| a. LAN only for v1 | Simplest. Matches milestone 2 of blupi_recon. **[REC] for the MVP.** |
| b. Layer-2 VPN (ZeroTier and similar) | Broadcast discovery works over an L2 virtual network. Tailscale does **not** carry broadcast, so discovery fails there. Document it; no code needed. |
| c. CNA upstream feature: fixed/configurable SystemLink host port + public "find/join at address" | Clean. **Requires the owner to scope the CNA repository.** Record the request as a `UB-NNN` limitation in `upstream_bugs.md`. |
| d. `AvailableNetworkSession::CreateInternal(..., address, port)` | Internal API, and the host port is ephemeral. Fragile; not recommended without CNA's agreement. |
| e. PlayerMatch through the CNA relay | See §6.2. |

### 6.4 Lobby and UX variants [OWNER]

- **Faithful lobby:** like SB1/SB2. Session list, create (name), join, a lobby with 4 slots
  showing name/colour/ready, host map choice, chat, START when everyone is ready.
- Galaxy currently has no text entry for session names **[NV]**. The player/gamer name could come
  from the signed-in profile's gamertag.
- Whether chat is needed in v1 (the original had it).

---

## 7. Recommended design (option A over CNA SystemLink)

### 7.1 Module layout [REC]

| Path | Contents |
|---|---|
| `src/GalaxyEggbert/Net/` (engine-independent, no CNA) | `NetProtocol.hpp/.cpp`: message structs, a little-endian codec with explicit sizes, atomic validator, protocol id/version constants. `NetLobby.hpp/.cpp`: lobby state machine (slots, colours, ready, map, start gate), testable without sockets. `RemotePlayer.hpp/.cpp`: last received pose, interpolation/extrapolation (original: at most 5 extrapolations), visibility rules. `NetEvents.hpp/.cpp`: outgoing event queue with bounded capacity (overflow is an error, not silent), plus the remote-event application interface. |
| `src/GalaxyEggbertCNA/NetSession.hpp/.cpp` (thin CNA adapter) | Owns `NetworkSession*`; does sign-in, create/find/join, `Update()`, send/receive and mapping of `NetworkGamer*` to slot/team; converts CNA events into lobby/game events. |
| `GalaxyEggbertGame` | A nullable `std::unique_ptr<NetSession>`. **Every multiplayer branch is guarded by "session exists".** |
| `tools/VerifyNetProtocol.cpp` | Codec round-trip, malformed-packet corpus, lobby state machine, event application. Registered in `CMakeLists.txt` next to the other verifiers. |

### 7.2 Session lifecycle

Mirrors the SB1/SB2 phases (assessment §3):

1. Main menu → **Multiplayer**. Hidden or disabled on Emscripten.
2. Sign in if nobody is signed in (Guide, or the auto-sign-in environment variable in tests).
3. **Session list:** `BeginFind(SystemLink, 1, props)`, refreshed. Create: `Create(SystemLink, 1, 4, 0, props)`.
4. **Lobby:**
   - Slots from `AllGamers`.
   - Colours (teams 0..3 = Yellow/Orange/Blue/Green; not required unique, as in the original).
   - Ready toggles. The host picks the multiplayer world. Optional chat.
   - Host START is enabled when ≥ 2 players are present and all are ready.
   - Use XNA's `IsReady` / `StartGame` / `GameStarted` where they fit **[NV]**; CNA implements
     the StartGame/EndGame state broadcast.
5. **START:**
   - Everyone loads the same world, checked by content hash.
   - Each player spawns at its colour's `PlayerStart`, with lives = 10.
   - The single-player mission and campaign state is untouched: no `SaveData` writes and no
     resume log.
   - Game speed is locked to Normal for the whole session. Tab and the speed keys are ignored
     (§5.3). Cheats follow the owner's answer to Q11.
6. **Play:** per tick, drain received packets **before** the local simulation step, as the
   original did (`ReadInput` then `MoveStep`), and send the pose bundle and this tick's events
   after the events step.
   - In Galaxy terms: receive before `worldRuntime_.Update(simDt)` in `UpdateTick`; send after
     `UpdateTickEvents`.
   - Pump `session->Update()` once per loop pass, also on interpolated frames (§5.3), so the
     connection stays serviced.
7. **End:**
   - LOST when lives are exhausted.
   - WIN when fewer than 2 players remain.
   - ABANDON on quit.
   - Session ended or host lost → back to the menu with a message.

### 7.3 Messages: Galaxy protocol v1 [REC]

Use one envelope per packet: `u16 protocolId` ('GE' multiplayer), `u8 version`,
`u8 kind`, `u32 sessionEpoch`, then the payload. The sender is the CNA `NetworkGamer*`, never a
payload field.

| Kind | Reliability | Payload (sketch) | From |
|---|---|---|---|
| HELLO / content | ReliableInOrder | protocol version, `kDemoSimulationRevision`-like simulation revision, build id, world hash | joiner → host |
| LOBBY | ReliableInOrder | map/world id + hash, 4 × {present, ready, team, gamer id} | host |
| TEAM / READY / CHAT | ReliableInOrder | team 0..3 / bool / text ≤ 99 bytes | client |
| START | ReliableInOrder | world id, hash, start tick | host |
| POSE | None (unreliable, unordered) | `u32 tick` (no 15-bit wrap), position xyz (float32, or int32 fixed-point in 1/256 cell), yaw (u16), `AnimState` (u8), anim phase (u8), `VehicleMode` (u8), flags (shield, hide, cloud, balloon, ecrase, power, invert, crouch, …), riding-lift object id (i16, -1 = none), sound-relevant state | each player, about 20 Hz and on change |
| EVENTS | ReliableInOrder (bundle) | ≤ N events, each `{kind, id, args}` | each player |
| TRANSIENT | None | effect spawns, local sounds | each player |
| LIFT | None (+ periodic reliable resync) | object id, dwell boundary start/end, phase | **host only** |
| DIE / LOST / ABANDON / PAUSE | ReliableInOrder | lives / — / — / bool | each player |

Notes on this design:

- **Prefer stable object ids to the original's position + type matching.** The index in the
  world's `MobileObjSpec` vector serves world objects. Spawned objects (bombs, dynamite, effects)
  get `{owner slot, local sequence}`. This avoids the original's ambiguity; record it in
  `behavior_differences.md` as an implementation detail, not a mechanic change.
- Event kinds to map from the original:
  - OBJECTSTART / OBJECTDELETE → Galaxy spawn, retire or pickup (`active = false`);
  - MODIFDECOR → voxel/tile change (doors, switches, destructible blocks) **[NV: Galaxy's tile-edit
    API]**;
  - PLAYSOUND / STOPSOUND;
  - lift synchro;
  - STOPCLOUD.
- Enumerate which Galaxy `InteractionSystem::EventKind`s and world mutations are visible to other
  players. **This inventory is the biggest unknown (§9 WP5).** Start from the original's producer
  list (`src/decnet.cpp`), not from Galaxy's event list.
- Pose rate: Galaxy simulates at 60 Hz and the original sends at 20 Hz. 20 Hz plus interpolation
  is enough for ≤ 4 players on LAN. At about 40 B × 20 Hz × 3 peers, bandwidth is trivial.

### 7.4 Remote players

- **Rendering.** Draw the real Blupi 3D model with the remote pose, animation state and vehicle,
  in team colours.
  - Each remote Blupi is one more `SceneModel` in the `SceneFrame` (§5.2). Factor
    `AppendSceneBlupi` into a function taking a plain visual struct.
  - Team colours: reuse the editor's `RecolorPlayerStartPixel` (§5.2). It produces derived textures
    from colours measured on established icons, not a new copied asset set. [NV] Confirm against
    the original's `CHBLUPI1..3` sheets; if they differ, ask the owner (Q9).
  - Interpolate remote poses between received samples, as the local Blupi is interpolated at frame
    rates above 60.
- **No physics or interaction** for remote Blupis on the local machine, as in the original. Their
  machine resolves their contacts and sends the resulting events.
  - [NV] Whether remote Blupis should at least be visible to local hazards or enemies. In the
    original they were not.
- **Hidden** when the remote player uses Hide (`SEC_HIDE`), and for the icon ranges the original
  excluded (276..284 = which animation?) **[NV]**.
- **Camera:** stays on the local player. A spectate mode after LOST is optional and an owner
  decision.
- **HUD:** show other players (name, colour, lives). The original had a lobby list and
  notifications such as "X has left" **[NV]**: check the original's in-game multiplayer HUD and
  strings.

### 7.5 World replication rules

- The **local machine is authoritative for everything its own Blupi does.** Pickups it collects,
  objects it spawns, decor it changes, and its own deaths.
- When an event is applied from the network, run the *effect* without re-emitting it. The
  application path must not push to the outgoing queue.
- The **host is authoritative** for moving platforms and lifts.
  - All machines simulate patrols locally (`AdvancePatrolStep`, deterministic timing).
  - The host sends dwell-boundary synchro; clients snap to it.
  - [NV] Whether Galaxy's patrol timing drifts across machines at all, given the 60 Hz fixed step
    and the same world. Probably minimal drift, but a riding player would desync otherwise.
- Late or old events still apply (as in the original). Old poses are discarded by tick.
- Objects removed or special in multiplayer (crates, goal, keys, helicopter/tank pickups):
  implement exactly what `decblock.cpp:1047+` does after reading it.

### 7.6 Content compatibility

- Refuse a join if protocol version, simulation revision or world hash differ. Advertise them in
  `NetworkSessionProperties` so incompatible sessions are not listed.
- `ResumeStart` / `DemoReplay` already compute a `worldHash` of the world file
  (`ResumeLog.hpp: ResumeStart::worldHash`). Reuse that hashing.
- There is no world transfer, as in the original. Both players must have the same multiplayer
  worlds; they ship with the game.

### 7.7 Validation and security

- Decode into bounded structs and validate everything atomically before applying anything.
  Count and log rejections. The rule table in `MULTIPLAYER_ASSESSMENT.md` §7 is a good checklist:
  exact lengths, ranges, owned ids, NUL-terminated text, event counts, object id in range and of
  a compatible type, coordinates inside the world plus margins.
- Rate-limit per peer and cap queues.
- LAN-only v1 lowers the stakes, but validate as if exposed. Internet comes later.

### 7.8 Disconnects and host loss

- Client leaves: CNA raises `GamerLeft`. Remove the player as if ABANDON was received; if fewer
  than 2 remain, the last one wins (original rule).
- Host leaves:
  - CNA SystemLink supports **host migration**; the original game did not.
  - [OWNER] Either (a) end the session for everyone ("host left", like blupi_recon), or (b)
    allow migration. Then lift synchro and lobby authority must move to the new host
    (`HostChanged`).
  - **[REC] (a) for v1.**
- Android backgrounding: the session drops after the ENet timeout. Show a message; do not crash.

### 7.9 Single-player safety contract (must hold after every commit)

1. Without a session, **no** new code runs in `Update`/`Draw`, apart from a null check.
2. The single-player RNG stream (`interaction_.rng_`) is never consumed by multiplayer code
   during single player.
3. The update order is unchanged.
4. The `SaveData` format, resume log format and demo format are unchanged.
5. `ctest` (including `VerifyDemoReplays`) is green, and `tools/verify_golden_frames.sh` is
   unchanged where it applies **[NV: coverage]**.
6. The web build compiles and shows no multiplayer entry.
7. `GamerServicesComponent` does not change single-player startup or input (§4.3).

---

## 8. Testing and verification plan

- **Pure logic** (`tools/VerifyNetProtocol.cpp`, registered as a test): codec round-trips,
  malformed corpus (length, count, index, text, unknown kind, wrong version), lobby state
  machine, event application without re-emission, interpolation.
- **Two-process loopback on one machine:**
  - Two Galaxy instances with `CNA_GAMER_SERVICES_AUTO_SIGN_IN` (different profile names),
    `SDL_AUDIODRIVER=dummy`, and separate user-data dirs.
  - Test flags: `--net-host` / `--net-join`, scripted input via autoplay where possible.
  - Assert lobby membership, START, that each side sees the other's pose and that a world
    mutation propagates (pickup removed on both), then a graceful leave and the win.
  - For repeatability, copy the blupi_recon idea: a test-only ingress gate that releases messages
    at tick boundaries.
  - Bound every wait and kill children on timeout.
  - [NV] Whether CNA's discovery works between two processes in the sandbox and network
    namespace used by agents. CNA claims a genuine two-process loopback test on Linux.
- **Network conditions:** CNA's `NetworkSimulator` (`demo_simulated_network_conditions`) for
  latency, loss and reordering.
- **Live check** (CLAUDE.md requires a proportional visual check for rendering changes): two
  windows, screenshots, remote Blupi visible, animated, coloured, riding a lift.
- **Platforms:** the Windows MinGW build running against Linux (blupi_recon tested Android ↔
  Windows); the Android build with permissions; the web build compiles with multiplayer hidden.
- **Single-player regression** after every commit: `ctest --test-dir build-cna-opengl3 -j4`, plus
  golden frames where relevant.

---

## 9. Work breakdown, milestones and estimates

Hours are agent working time for an Opus-class model at high effort. They include builds with
`-j6` and verification. They exclude waiting for the owner and the iteration that follows owner
bug reports, which on this project is substantial.

The basis is blupi_recon's measured ~6 h for transport, protocol, tests and platforms on a game
whose multiplayer logic already existed, scaled up for Galaxy's missing game logic and its
large host class (12.8k lines, split by topic since 2026-11-09).

These are **agent hours**. They do not convert into hours for a human author, whose dominant cost
is understanding the existing AI-written code they hook into (§14). They also exclude the owner's
review time under AI policy v3.

| WP | Contents | MVP | Full |
|---|---|---|---|
| WP1 | Net layer: CNA adapter, codec, validator, `VerifyNetProtocol` | 3–4 h | 5–7 h |
| WP2 | GamerServices sign-in integration (component, offline profile, auto-sign-in for tests) | 1–2 h | 2–3 h |
| WP3 | Lobby UI (list, create, join, colour, ready, map choice, start, optional chat) | 3–4 h | 5–7 h |
| WP4 | Remote players (draw-path refactor, pose, animation, vehicles, colours, interpolation) and rules (lives 10, starts by colour, die/lost/abandon/win, pause) | 4–6 h | 7–10 h |
| WP5 | World replication (pickups, objects, decor, lifts host-sync, sounds, effects, per-team perso bombs, multiplayer object rules) with echo guards | 4–6 h | 9–14 h |
| WP6 | Multiplayer 3D worlds (hand-authored, with briefs, `levelcheck` / `VerifyShippedWorlds`) | 1–2 h (1 world) | 4–8 h (3–5 worlds) |
| WP7 | Tests (two-process), live checks, Windows/Android/web, docs (`handoff.md`, `CURRENT.md`, `plan.md`, `behavior_differences.md`, `upstream_bugs.md`) | 3–4 h | 5–8 h |
| | **Total** | **~19–28 h** | **~37–57 h** |

The chat answer of 2026-11-08 said 18–26 h / 35–54 h. These totals add WP2, because the sign-in
requirement was found later.

The **largest uncertainties** are WP5 (inventory of shared world mutations in a 4,121-line
interaction system) and WP6 (content). Internet play (§6.3 c/e) and host migration are outside
both columns.

### Milestones and acceptance criteria

| M | Contents | Acceptance |
|---|---|---|
| M0 | Owner decisions Q1–Q11 recorded (§10) | Q1 and Q10 answered; scope written into CLAUDE.md/plans; the AI policy status checked (§2.5) |
| M1 | **Spike** (2–3 h): `--net-host` / `--net-join`, SystemLink create/find/join, sign-in by environment variable, pose-only coloured "ghost" Blupi in an existing world with two starts | Two local windows see each other move; single-player `ctest` + `verify_demos.sh` green; link, size and Android impact noted. **Re-estimate after M1.** |
| M2 | Protocol v1 + validator + content handshake + `VerifyNetProtocol` | Malformed corpus rejected; mismatched world refused |
| M3 | Lobby UI + menu entry (hidden on web) | Create/join/ready/start through the UI on two instances |
| M4 | Remote players complete + rules | Animation, vehicles, hide; lives 10; leave and win work |
| M5 | World replication + ≥ 1 (MVP) / 3–5 (full) multiplayer worlds | A pickup or object change on one side appears on the other; lifts in sync; perso bombs per team |
| M6 | Platforms + two-process automated test + docs | Windows ↔ Linux on LAN; Android builds with permissions; web compiles; `handoff.md` updated |
| M7 (optional) | Internet (§6.3), host migration, PlayerMatch | Owner-scoped |

---

## 10. Open owner decisions [OWNER]

**Answered on 2026-11-10** (`plans/audit/owner_questions.md` Q-019): Q1 yes; Q2 SB2/SE2; Q3 one world,
then 3-5 from SB2's world201-212; Q4 Linux + Android; Q5 LAN + documented ZeroTier; Q6 automatic as
player A/B/C; Q7 the session ends; Q8 defects not reproduced; Q9 recolouring; Q10 an agent before
the AI policy, on branch `multiplayer`, merged on acceptance; Q11 as the original; chat in v1.

Write these into `plans/audit/owner_questions.md` as Q-NNN entries. Do not block on them; do the
spike meanwhile if Q1 is at least provisionally yes.

1. **Q1 – Scope and source of truth.** Is multiplayer approved for Galaxy, with SB1/SB2 (via
   blupi_recon) as its source of truth, given that mobile-eggbert has none?
   - Recommendation: yes. Record it in `galaxy-eggbert/CLAUDE.md` "Faithful-remake rules".
2. **Q2 – Which original.** SB1 rules, SB2 rules, or Speedy Eggbert 2 rules? Galaxy's editor
   follows "Eggbert 2", which suggests SB2/SE2. Do the multiplayer rules differ between them
   **[NV]**?
3. **Q3 – Maps.** How many multiplayer worlds and which themes? Which SB1/SB2 multiplayer maps give
   identity (`world101..110` / `201..212`)?
4. **Q4 – Platforms.** Desktop LAN only for v1? Android in v1? Web excluded (CNA cannot do it)?
5. **Q5 – Internet.**
   - (a) LAN only;
   - (b) document a ZeroTier-style VPN;
   - (c) scope CNA for a fixed host port + join-by-address;
   - (d) host a CNA gamer services server for PlayerMatch.
   - Recommendation: (a) for v1, then (c).
6. **Q6 – Sign-in UX.** Is CNA's Guide (Xbox-like look) acceptable for multiplayer sign-in, or
   should Galaxy auto-create or sign in a local profile **[NV: possible?]**?
7. **Q7 – Host loss.** End the session (recommended, faithful) or use CNA host migration?
8. **Q8 – Historical defects.** Keep or fix the original's defects (team/slot cloud, wrap, silent
   event drop)? Recommendation: do not reproduce them.
9. **Q9 – Team colours.** May Galaxy derive team-coloured Blupi textures (recolour, as for the
   enemy Blupis and the editor's player starts, §5.2), or must they come from original sheets
   (asset approval)?
10. **Q10 – Who implements it, when, and on which branch (§2.5, §14).** Options:
    - (a) not now: keep this document as a proposal;
    - (b) an agent before the AI policy is introduced, on a separate branch behind a build option
      that is off by default;
    - (c) an agent under v3, one authorized work package at a time, with the owner reviewing and
      committing;
    - (d) the owner by hand, with AI as an advisor (the only route under v2).

    Recommendation: (a), or (d) whenever the owner wants to. If (b), never merge into `develop`
    without the owner's acceptance.
11. **Q11 – Game speed and cheats in a session.** Lock the speed to Normal and disable cheats
    (recommended), or follow whatever the original did **[NV]**?

---

## 11. Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Hooks in shared simulation code (`InteractionSystem`, `GalaxyEggbertGame`) change single-player behaviour | Owner's working game regresses | Session-null guards; no RNG draws; `verify_demos.sh` + golden frames each commit (§7.9) |
| Blupi draw-path refactor breaks single-player visuals | Visible regressions | Golden frames + screenshots before and after |
| `GamerServicesComponent` affects startup or input | Single-player regression | Add lazily on entering multiplayer if CNA allows **[NV]**; else verify |
| Unknown inventory of world mutations to replicate | WP5 overrun; desyncs | Start from the original producers (`decnet.cpp`), not Galaxy's events; ship a minimal set first |
| CNA bug in SystemLink | Blocked (CNA read-only) | `upstream_bugs.md` entry; ask the owner to scope CNA; raw ENet as a last-resort fallback (owner decision) |
| Link pulls libcurl/OpenSSL into all builds (desktop, MinGW, Android) | Build breaks or bigger binaries | Check in M1; set `CNA_ENABLE_VOICE=OFF`; report |
| Two local instances share save/settings files | Corrupted owner saves during tests | Separate user dirs in tests **[NV: mechanism]** |
| Lift/platform drift between machines | Riding players desync | Host dwell synchro + periodic resync |
| Faithful-remake rule conflict | Work rejected | Q1 before M2 |
| Dirty worktree from another session | Mixing changes into commits | Stage only own files; re-check `git status` |
| Internet play expectations | Owner expects online play that SystemLink cannot give | State LAN-only clearly in docs and UI |
| **Handover: nobody understands the network code** | Under AI policy v3, code the human cannot explain must be removed; under v2, a human must maintain it alone | §14: separate branch, build option off by default, MVP scope, a human-readable explainer, a small surface in shared code |
| **Permanent replication cost** | Every later gameplay change (new object, event, mechanic) must decide what other players see; forgetting it gives silent desyncs | One documented replication list; a `VerifyNetProtocol` check that every replicated `EventKind` has a codec entry; keep the hooks in one place |
| Policy introduction delayed by the work | The backlog wants the policy only after outstanding work is committed; multiplayer adds 37–57 agent hours plus real-network tests by the owner | Decide Q10 first |
| Different game speeds or cheats in a session | Worlds drift apart between machines | Lock speed to Normal in sessions (§5.3, Q11) |
| `develop` moves fast (62 commits on 2026-11-09) | A long-lived branch conflicts in the host files | Small hooks; rebase often; keep new code in new files |

---

## 12. First-steps checklist for the implementing agent

0. **Check the AI policy (§2.5).** Read `/rv/data/development/github.com/ai-policy/AGENTS.md`
   and `docs/handbook/08-backlog.md` §1, and look for an `AI.md` or a policy file in
   `galaxy-eggbert`.
   - If a policy is in force, it overrides everything below.
   - Under v2: advise only, change nothing.
   - Under v3: edit only within the scope the owner authorized, and never commit.
   - Check the owner's answer to Q10.
1. Read `galaxy-eggbert/CLAUDE.md`, `CURRENT.md`, `handoff.md` and this file. Check `git status`
   and `git log -30`.
2. Check `plans/audit/owner_questions.md` for answers to §10. If absent, add Q-entries for
   Q1–Q11 (only while no policy forbids editing).
3. Read the blupi_recon files in §3.8 (at least `MULTIPLAYER.md` and the assessment §1–§4, §7)
   and the open [NV] original-rule items in §3.3.
4. Read `cna/modules/net/examples/demo_net_avatar_sync/src/SyncGame.cpp` and
   `cna/modules/graphics/examples/common/SignInEXT.hpp`.
5. Do the M1 spike in the existing `build-cna-opengl3` tree, with the env and `-j6` rules (§2.3).
6. After M1, re-estimate and write the findings into `handoff.md`. Commit only if no policy is in
   force and Q10 allows it (no attribution, no push). Otherwise leave the change uncommitted with
   a review guide for the owner.

## 13. Unverified items to check first

- [ ] The `decblock.cpp:1047+` body: what multiplayer does with crates, goal, keys and
  helicopter/tank pickups. The full match goal (§3.3).
- [ ] The effect of other teams' perso bombs (`decblupi.cpp`, `decmove.cpp:637-655, 1603`).
- [ ] Whether Galaxy's ObjectType enum and registry have per-team perso types.
- [ ] The original team ↔ colour order vs Galaxy's Yellow/Orange/Blue/Green.
- [ ] Linking `CNA_Net` into Galaxy: dependencies, size, MinGW, Android, Emscripten compile.
- [ ] Whether `GamerServicesComponent` can be added lazily, and whether a local profile can be
  created programmatically.
- [ ] Whether SystemLink relays client↔client data through the host transparently (XNA semantics).
- [ ] The `NetworkSessionProperties` slot count; the maximum `SendData` payload.
- [ ] Galaxy's tile-edit API for replicated decor changes; stable ids for `MobileObjSpec`.
- [ ] The save/settings location, for multi-instance tests; headless capability.
- [ ] Coverage of `tools/verify_golden_frames.sh` and `verify_golden_trace.sh`.
- [ ] The Android multicast lock from a CNA/SDL app (JNI side).
- [ ] What the original did with game speed and cheats in multiplayer (Q11).
- [ ] Whether `Content/icons/button.png` icons 96–98 match the original's team colours
  (`CHBLUPI1..3`).
- [ ] Whether CNA's fixed-timestep catch-up (normal 60 Hz mode) has a limit that lets the
  simulation fall behind real time, as the >60 interpolation mode does by design.

---

## 14. Who implements it, and the handover

Added 2026-11-09, from the owner's questions: "Will multiplayer by an AI agent make the whole game
worse and harder to hand over after the AI policy?" and "What if I write it myself, even if it took
me twenty years?"

### 14.1 Why multiplayer is the hardest part to hand over

- **The codebase is already almost entirely AI-written.** `docs/provenance/ai-involvement.md` §4:
  only 19 files (3,412 lines) of the current code tree predate the first agent instruction file.
  Multiplayer does not change that character. It adds the part that is hardest to understand
  afterwards.
- **Network code is hard to review and explain.** It has:
  - untrusted input from other machines;
  - timing, loss and reordering;
  - host loss and disconnects;
  - bugs that appear only with 2–4 instances on real networks.

  Under v3 a human must be able to explain every change; under v2 a human must have written it.
- **Size.** A rough guess for the full scope is 3,000–6,000 lines including tests:
  - protocol and validator;
  - lobby;
  - remote players;
  - replication hooks;
  - CNA adapter;
  - sign-in.

  This is not measured. blupi_recon's transport alone is 1,044 lines, and there the game logic
  already existed.
- **A permanent cost on every later change.** Once multiplayer exists, every new object, event or
  mechanic has to decide what other players see. A maintainer working by hand pays that on every
  change, not once.
- **New moving parts to inherit:**
  - CNA Net and GamerServices sign-in (with CNA's own Guide UI);
  - ENet;
  - Android permissions;
  - LAN discovery;
  - a CNA tree that is itself still changing.
- **A rule exception to explain.** `mobile-eggbert` has no multiplayer, so the feature rests on an
  owner decision (Q1). The successor must find that decision recorded.

### 14.2 What keeps the risk small if an agent does it (Q10 b or c)

- Work on a **separate branch**. Do not merge into `develop` until the owner has played it on a
  real LAN and accepted it. The handover base stays as it is.
- Add a **CMake option, off by default** (for example `GALAXY_EGGBERT_MULTIPLAYER`; no new `GE`
  prefix, per `docs/handbook/07-contributing.md`). With it off, the single-player binary contains
  none of the new code.
- **MVP first** (LAN, 2 players, one world; §9). Stop there unless the owner asks for more.
- **New code in new files** (`src/GalaxyEggbert/Net/`, `src/GalaxyEggbertCNA/NetSession.*`).
  Keep the hooks in the host and `InteractionSystem` few, and each one a single guarded call.
- **A human-readable explainer**, for example `docs/handbook/09-multiplayer.md`: architecture, the
  message table, who is authoritative for what, the replication list, how to test with two
  instances, and the known limits. Written so the owner can review and explain the code from it.
- Under v3: one work package per authorization, each handed over uncommitted with a review guide.

### 14.3 If the owner writes it by hand (Q10 d)

This is the route that fits both policy versions, and the only one under v2.

- **What it removes.**
  - The comprehension risk disappears: the author understands the code.
  - There is no deadline; the backlog has nothing that waits for multiplayer.
- **What it does not remove.**
  - The regression risk to single player (§11) and the permanent replication cost (§14.1) stay.
  - The existing test suite still guards single player, whoever writes the code: `ctest` (120
    tests), golden frames, demo verification and the 78-world play pass.
- **The real first cost is reading.** The author must first learn the code the hooks go into:
  - `GalaxyEggbertGameUpdate.cpp` (3,388 lines);
  - `InteractionSystem` (about 5,300 lines);
  - `BlupiController` (about 5,200 lines);
  - `SceneFrame` / `AppendSceneBlupi`.

  That knowledge is needed for any maintenance after the handover anyway, so it is not wasted.
- **How to use this document then.**
  - Read it as a design document and a reading list, not as instructions. §3 (the original's
    model), §5 (where things are), §6–§7 (design) and §13 (what to check) apply unchanged.
  - The hour estimates in §9 do not apply.
  - Under v2, AI may still be used to explain code, research CNA's API and review the owner's
    changes. It must not write the code or the repository's documentation.
- **A sensible order for a human:**
  1. The M1 spike (§9): two windows seeing each other move as coloured Blupis, pose only.
  2. The protocol with its verifier.
  3. The lobby.
  4. Replication, one event kind at a time, each with a two-instance check.

  Each step is playable and testable on its own, so it can stop and resume over any length of
  time.
