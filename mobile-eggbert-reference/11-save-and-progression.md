# Save System & Level Progression

**Status:** `DOC-301`. Scope: prose description of save/load *behavior* and world/mission
progression, with key numeric constants — not pseudocode, not verbatim/line-by-line code
transcription. The exact save-file byte layout (`GameData`'s field offsets) is explicitly **out of
scope** here; that remains a separate open question requiring its own approval (`easy3d.md` §12
Q7). Only what gets saved/loaded conceptually and when is covered. Door *mechanics* (icon ranges,
key mapping, opening animation) are documented in `06-doors.md` and only cross-referenced here.

## Two distinct persistence systems

mobile-eggbert has two unrelated save mechanisms that are easy to conflate:

1. **`GameData`** (`GameData.cpp`/`.hpp`) — the long-term player-progress profile: lives, last
   world reached, and the 200-entry door-unlock array, times up to 3 gamer slots, plus a handful
   of global control/audio settings. Backed by its own file (`Worlds::ReadGameData()` /
   `WriteGameData()`, filename `"SpeedyBlupi"`, `Worlds.cpp:130-134`).
2. **`Decor::CurrentWrite()`/`CurrentRead()`** (`Decor.cpp:10986`, `Decor.cpp:11143`) — a
   quick-resume snapshot of the entire *live simulation* (Blupi's exact in-level state, the full
   tile grid, active moving objects, HUD gauges), written to a separate `"CurrentGame"` slot
   (`Worlds.cpp:136-140`). This is not "the save game" in the progression sense — it exists purely
   so the app can resume mid-level after being backgrounded.

## Quick-resume snapshot (`CurrentWrite`/`CurrentRead`)

`Decor::CurrentWrite()` (`Decor.cpp:10986`) serialises the live game state field-by-field into a
`Worlds` key/value writer and flushes it to the current-game slot. What it captures, conceptually:

- Blupi's full runtime state: position, valid/restart position, action, direction, animation
  phase, velocity, every status flag (shield, power, cloud, hide, invert, helico, jeep, tank,
  skate, swim/`nage`, surf, wind/`vent`, suspend, balloon, crushed/`ecrase`, motor-high, focus,
  front-facing, air), held key flags, dynamite count, bullet count, timers (shield, fire, no-ascent,
  "ouf" hurt-timer), start position/direction, and level index.
- World/session-level state: scroll position, current music/region/term codes, elapsed time,
  mission index, lives (`m_nbVies`), treasure count and target, goal phase, and the persistent
  200-entry door array (`_Doors_`).
- The entire 100x100 foreground (`Decor`) and background (`BigDecor`) tile grids, written as one
  row-of-100-ints record per row (`Decor.cpp:11079-11098`).
- Every active `MoveObject` (moving/animated object), written sparsely — only slots whose `type`
  is non-zero get a record, each carrying its own pool index so it can be restored to the same
  slot (`Decor.cpp:11099-11120`).
- The two HUD gauges (`m_jauges[0]`/`[1]` — hide flag, mode, level), `Decor.cpp:11121-11128`.

`Decor::CurrentRead()` (`Decor.cpp:11143`) is the exact inverse: it starts from a freshly
`InitDecor()`-ed state and overwrites every field above, so any field not present in the saved
text falls back to the engine's normal init default. Time-based `MoveObject` fields are re-scaled
through `Config::ScaleTime()` on load, so a snapshot made at one frame rate replays correctly at
another (`Decor.cpp:11134-11142`). The moving-object read loop stops at the first record whose
`type` is 0, matching the sparse-write convention.

**When this happens:** `Game1::OnDeactivated()` (`Game1.cpp:186-198`) writes the snapshot via
`CurrentWrite()` only if the app is backgrounded while in the `Play` phase; if backgrounded in any
other phase (menu, pause, etc.) it instead calls `Decor::CurrentDelete()` (`Decor.cpp:10974`,
thin wrapper over `Worlds::DeleteCurrentGame()`) to discard any stale snapshot. `OnExiting()`
(`Game1.cpp:206-209`) always deletes the snapshot on a clean quit. On the next launch, the `Wait`
boot phase (`Game1.cpp:261-271`) attempts `decor.CurrentRead()`; if it succeeds the app jumps
straight to the `Resume` phase instead of the normal main-menu flow (`Game1.cpp:19-20` phase-graph
comment). There is exactly one snapshot slot — it is not a per-level list of resume points, just
"was the app killed mid-play."

## Save slots ("gamers") and the profile data

`GameData` supports up to 3 independent player profiles ("gamer" slots A/B/C,
`GameData.hpp:96`, `GameData.cpp:152` `MaxGamer`). A global header byte selects which slot is
active (`getSelectedGamerProperty()`/`setSelectedGamerProperty()`, switched via
`Game1::SetGamer()`, `Game1.cpp:973-977`, bound to the `InitGamerA/B/C` buttons,
`Game1.cpp:282-285`), plus a few global control/audio settings that are *not* per-gamer: sound
on/off, jump-button side, auto-zoom, accelerometer-tilt on/off and its sensitivity
(`GameData.hpp:129-211`).

Each of the 3 gamer slots independently stores: a lives count (`nbVies`, default 3), the index of
the last world reached (`lastWorld`, default 1), and its own 200-entry door-unlock array (180
secondary/sub-level door flags + 20 main/world door flags — see `06-doors.md` for what a "door" or
"gold" flag actually gates). Resetting a slot (`GameData::Reset()` / `Initialize(gamer)`,
`GameData.cpp:99-101,158-166`) puts lives back to 3, last-world back to 1, and clears every door
flag for that slot only — the other two slots and the global settings are untouched. This reset is
reachable both from the in-game Setup screen's "Reset" button and from cheat code 5
(`Game1.cpp:311-313`, `507-508`).

`GameData::GetGamerInfo()` (`GameData.cpp:119-138`) computes, per slot, the lives count plus a
count of opened main doors (out of 20) and opened secondary doors (out of 180) by scanning the
door array; this is used purely to print a progress summary on each gamer-select button
(`Game1::DrawButtonGamerText`, `Game1.cpp:861-867`), not for gameplay logic.

`GameData::Read()`/`Write()` (`GameData.cpp` ~84-97) load/flush the entire byte blob (all 3
slots + global settings) as one unit — there is no partial/per-slot file I/O. `Read()` is called
once at boot (`Game1.cpp:256`, `First` phase). `Write()` is called after every settings toggle,
after `Reset()`, after `SetGamer()`, and — most importantly for progression — at the end of
`Game1::MemorizeGamerProgress()` (`Game1.cpp:1060-1068`, see below).

## Level/world files vs. save data

`Decor::Read(int gamer, int rank, bool bUser)` (`Decor.cpp:11304`) is a *third*, unrelated kind of
read: it loads a designed level's static data (tile grids, start position/direction, initial
moving-object records) from a world file, not from a save. In the current port, `gamer` is unused
(`Worlds::ReadWorld`/`GetWorldFilename`, `Worlds.cpp:142-196` explicitly discard it) and `rank`
directly selects the file `worlds/world{rank:3digits}.txt`; the sole call site always passes
`gamer=0, bUser=false` with `rank` set to the mission number (`Game1::StartMission`,
`Game1.cpp:466`). `bUser` is intended to distinguish user-authored/private levels from the
built-in campaign, but is not exercised by any current call site. Tile icon `0` from the file is
normalised to `-1` ("empty"), since the level editor uses `0` to mean "no tile"
(`Decor.cpp:11295-11302`). Only Blupi's *start* position/direction are read here — his live
position is placed later by `PlayPrepare()` (`Decor.cpp:11295-11303`).

`Decor::Delete()` (`Decor.cpp:11379`) and `Decor::FileExist()` (`Decor.cpp:11387`) are both stubs
in this port — `Delete()` unconditionally returns success without touching storage, and
`FileExist()` unconditionally reports "no file"; their parameters (`gamer`, `rank`, `bUser`) are
unused. Both are `private` methods with **zero call sites anywhere in the codebase** (not even
from within `Decor.cpp` itself) — there is no other level-deletion/existence-check mechanism
elsewhere in this port to point to; they are simply dead code, likely vestigial from a
platform-level (IsolatedStorageFile) file-management path in the original C#/XNA game that this
port never reimplemented.

## Door-unlock persistence across saves

The 200-entry door array (`m_doors` in `Decor`) is transient live state inside `Decor` during
play; it is synced to/from the persistent `GameData` copy at level-transition boundaries via two
thin wrappers:

- `Decor::InitializeDoors(GameData& gameData)` (`Decor.cpp:1701`) copies `gameData`'s stored door
  array into `m_doors` when a level starts (`Game1::StartMission`, `Game1.cpp:470`, right after
  `Decor::Read()` loads the level and before `AdaptDoors()` interprets those flags).
- `Decor::MemorizeDoors(GameData& gameData)` (`Decor.cpp:1719`) copies `m_doors` back into
  `gameData` when a level ends, from inside `Game1::MemorizeGamerProgress()`
  (`Game1.cpp:1060-1068`), which also copies the current lives count into `gameData` and then
  calls `gameData.Write()` to flush everything to disk in one step.

Both wrappers log the flag at `m_doors[m_mission + 1]` specifically (the "did the player just
unlock the next sub-level" slot) for diagnostics, but otherwise their behavior is a straight
array copy in each direction — no filtering or transformation happens in `Decor` itself.

The array is actually mutated by two "win" hooks, both reachable only from the same win-animation
branch point (`Decor.cpp:6411-6435`, inside the main per-frame update, when Blupi's win animation
(`BlupiAction::Win`) finishes) but **mutually exclusive** — exactly one of them runs per win
(chosen by an `if`/`else if` chain keyed on `m_bPrivate` / `m_mission` / `m_bFoundCle`; the other
two branches in that chain call neither hook):
`Decor::OpenDoorsWin()` (`Decor.cpp:11695`) sets `m_doors[m_mission + 1] = 1` — unlocking the very
next sub-level — and `Decor::OpenGoldsWin()` (`Decor.cpp:11707`) sets
`m_doors[180 + m_mission / 10] = 1` — marking an entire world as cleared, which is what lets the
hub screen later reveal that world's collected gold pickup (see `06-doors.md` for the door/gold
tile mechanics `AdaptDoors()` drives from these same flags; not re-explained here). A third
variant, `Decor::OpenDoorsTresor()` (`Decor.cpp:11642`), opens every treasure-gated door (icons
421..421+`m_nbTresor`-1) whenever a treasure pickup is collected — this is immediate in-level
door-opening, not a persisted unlock flag.

`MemorizeGamerProgress()` (and therefore the `GameData` flush) runs at exactly three points in the
top-level phase machine (`Game1.cpp:417-437`, phase-graph comment `Game1.cpp:32-34`): losing a
level (`Decor::IsTerminated() == -1`), winning the final level (`== -2`), and completing a level
that unlocks a specific next mission (`>= 1`, immediately followed by `StartMission()` for that
next mission). So progress is durably saved at every level boundary, not just on quit.

## Spawn-point resolution at level-load boundaries

- `Decor::MainSwitchInitialize(int lastWorld)` (`Decor.cpp:11508`) only acts when the just-loaded
  level is the hub (`m_mission == 1`): it calls `SearchWorld(lastWorld, ...)` and, if a match is
  found, overwrites Blupi's start position/direction so re-entering the hub places him standing
  next to the world he most recently completed, rather than at the hub's authored default spawn.
  It has no effect on ordinary gameplay levels. Called from `Game1::StartMission`
  (`Game1.cpp:472`) with `gameData.getLastWorldProperty()` — itself updated whenever a non-hub
  mission starts (`Game1.cpp:462-465`).
- `Decor::SearchWorld(int world, TinyPoint& blupi, Direction& dir)` (`Decor.cpp:11398`) scans the
  100x100 grid for the given world's terminal marker icon pair (looked up in
  `Tables::world_terminal`, a 30-entry table = 2 icons x up to 15 worlds — valid `world` range is
  0..12, `Decor.cpp:11400`), then places Blupi on whichever adjacent cell is passable, facing
  toward the marker.
- `Decor::SearchDoor(int n, TinyPoint& cel, TinyPoint& blupi)` (`Decor.cpp:11439`) and
  `Decor::SearchGold(int n, TinyPoint& cel)` (`Decor.cpp:11486`) are grid scans used by
  `AdaptDoors()` (not by `MainSwitchInitialize`) to locate, respectively, a specific numbered
  world-door sign plus its actual door tile and stand-point, and the gold-pickup marker tile for a
  given world index — see `06-doors.md` for what happens once these cells are found (door-open
  animation, gold-collected animation). They are progression *lookups*, not persistence, and are
  re-run from the static level grid every time rather than cached.

## Lives (`m_nbVies`)

Lives are per-gamer-slot persistent state (stored via `GameData::getNbViesProperty()`/
`setNbViesProperty()`) but live as `Decor::GetNbVies()`/`SetNbVies()` (`Decor.cpp:1691,1696`)
during play. Flow, gathered from all `m_nbVies` use sites in `Decor.cpp`:

- Default is 3 (`PlayPrepare(bTest=true)` for test levels, `Decor.cpp:385`; also the factory
  default in `GameData::Initialize(gamer)`).
- Loaded into `Decor` at the start of every mission from the persisted profile value
  (`Game1::StartMission`, `Game1.cpp:469`: `decor.SetNbVies(gameData.getNbViesProperty())`).
- Gained by collecting an egg pickup (`ObjectType6`), up to a cap of `MAX_EGG_COUNT = 10`
  (`Decor.cpp:96`) — the pickup animates into the HUD life counter and increments `m_nbVies` only
  once that fly-in animation completes (`Decor.cpp:6007-6012` triggers it, `Decor.cpp:10262-10268`
  applies the actual `m_nbVies++` when the animation finishes).
- Lost when Blupi dies (one of the `Clear1..8`/`Drown`/`Glu`/`Electro` hurt animations completing,
  `Decor.cpp:6374-6398`): if `m_nbVies > 0` a life is spent (decremented as part of the
  "Blupi flies off to the HUD life counter" animation start, `Decor.cpp:10176`) and the level
  restarts Blupi at his last valid position; if `m_nbVies` was already 0, the level itself is
  failed (`m_nbVies = -1`, `m_term = -1`) and `Decor::DoorsLost()` (`Decor.cpp:11716`) runs, which
  — despite the name — does not touch the door array at all: it simply resets `m_nbVies` back to
  the default 3, matching the original game's behavior of always giving the player a fresh set of
  lives after a game-over rather than a permanent depletion.
- Also reset to 3 by falling an extreme distance off the level (`Decor.cpp:6406-6410`, a fallback
  "you fell out of the world" case that also calls `DoorsLost()`), and set to 9 by cheat code 4
  ("LayEgg", `Decor.cpp:1789-1791`).
- Persisted back to the profile at every level-end boundary via `Game1::MemorizeGamerProgress()`
  (`Game1.cpp:1064`: `gameData.setNbViesProperty(decor.GetNbVies())`), alongside the door-array
  sync described above.

The HUD gauges (`m_jauges[0]`/`[1]`) that also round-trip through the quick-resume snapshot are a
separate display mechanism (mode/level/hide flags for on-screen meters) and are not otherwise
tied to progression logic examined here.
