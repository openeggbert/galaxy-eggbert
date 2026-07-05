# Doors — Detailed Behavior (2D)

**Status:** believed complete for the 2D behavior itself (icon range, key mapping, opening
animation, collision) — the open item is entirely on the 3D-mapping side, not further 2D research.

**`DOC-303` (2026-07-05):** extended with the full door/key gameplay logic beyond the original
icon/animation scope — door detection probe, key-pickup-to-flag flow, treasure-gated doors,
win/lose door effects, door-state persistence/adaptation, and world-transition door search. All
under explicit user approval recorded in `CLAUDE.md` (2026-07-05).

Confirmed directly from `Decor.cpp` (not just inferred from `BlockTypes.hpp`):

- Door tiles are icons 334, 335, 336 in the main `Decor` grid (`Decor::IsDoor`, `Decor.cpp` ~7360:
  tests `icon >= 334 && icon <= 336`). `IsDoor` probes two cells, not one: Blupi's own cell first,
  then one tile further in his facing direction (+60 world-units if not facing left, -60 if facing
  left), so he can trigger a door he is walking into a step before actually reaching it, not only
  while standing on it.
- The required key is derived as a bitmask: `doorKeyMask = 1 << (icon - 334)` — icon 334 needs bit
  0, 335 needs bit 1, 336 needs bit 2, matching `DoorKeyFlags::Key1/Key2/Key3` and
  `ObjectType49/50/51` (the three key pickups) exactly.
- Opening (`Decor::OpenDoor`, `Decor.cpp` ~11667): the tile's `icon` is set to `-1` (removed from
  the static grid, becomes passable) and a **temporary `MoveObject` of type 22** ("door opening
  animation") is spawned at that cell, using the door's own icon, sliding from the door's position
  upward by one tile (`posEnd.Y = posStart.Y - 1`) over `stepAdvance = Config::ScaleTime(50)` ticks,
  playing `SoundChannel33`. So visually a door **slides up and out of the way**, it does not fade,
  shatter, or swing.
- Doors are solid/blocking while present (implied by being a normal `Decor` grid icon, subject to
  the same `IsBlocIcon()` tile collision as walls) — i.e. in 2D a door is simply an opaque, solid
  textured tile like any other wall tile until removed.

## Key pickup mechanic

The three key pickups are transient `MoveObject`s of type `ObjectType49` (key 1), `ObjectType50`
(key 2), `ObjectType51` (key 3), each animated by cycling through a 12-frame icon table
(`Tables::table_cle1/2/3`, `Decor.cpp` ~8324-8338) — a shimmering/spinning key sprite, not a static
tile.

- Pickup is detected in the same per-object collision pass as other world items (`Decor.cpp`
  ~5962-6006): when Blupi touches a key object of a given type **and he does not already hold that
  key's flag**, the world object is deleted (`ObjectDelete`) and four small sparkle particles
  (`ObjectType39`, offset ±10/±60 units) are spawned at its position.
- The pickup does **not** set the `DoorKeyFlags` bit immediately. Instead it starts a "voyage"
  (HUD fly-to-icon) animation via `VoyageInit`, flying the key icon (215 for key 1, 222 for key 2,
  229 for key 3) from the key's world position to its fixed HUD slot — `(520,418)`, `(530,418)`,
  `(540,418)` respectively — playing `SoundChannel11` at the start of the flight (`Decor.cpp`
  ~10194-10205, inside `VoyageInit`). `SoundChannel3` is a separate confirmation sound that only
  plays later, when the voyage completes and the key flag is actually set (see next bullet) — not
  during the flight itself.
- The `DoorKeyFlags` bit is only OR'd into `m_blupiCle` when that voyage animation **completes**
  (`Decor.cpp` ~10276-10289, matched by `m_voyageIcon`/`m_voyageChannel`). Until the flight finishes,
  the key is not yet "held" for door-opening purposes, though the pickup has already been consumed
  from the world.
- Keys are **not consumed on pickup** — holding a key is a persistent boolean flag, not a count —
  but they **are consumed on use**: opening a matching door clears that key's bit from `m_blupiCle`
  (`Decor.cpp` ~5619, `ToRaw(m_blupiCle) & ~ToRaw(doorKeyMask)`), so each key opens exactly one
  door of its color before it must be picked up again elsewhere in the level.
- When a door is opened by key (as opposed to by `AdaptDoors`/`OpenDoorsTresor`), a second cosmetic
  voyage animation plays in the opposite direction — from the HUD key slot back out to the door's
  screen position — using icon `214 + (icon - 334) * 7` (i.e. 214/221/228, the "spent key" variant
  one below each held-key icon). This is purely visual feedback; the door has already been opened
  and the key flag already cleared by the time this animation is queued.
- The `WeelKeys` cheat code grants all three keys at once (`m_blupiCle |= DoorKeyFlags::All`,
  `Decor.cpp` ~2041), bypassing pickup entirely.
- The held-key HUD indicators are drawn in `Decor::DrawInfo` (`Decor.cpp` ~1218-1235): icon 215 at
  `(520,418)` while `Key1` is set, 222 at `(530,418)` for `Key2`, 229 at `(540,418)` for `Key3` — same
  positions/icons the pickup voyage animations fly toward.

## Treasure-gated doors — `Decor::OpenDoorsTresor` (`Decor.cpp` ~11642)

A second, separate door family uses consecutive icons starting at 421: a door requiring N
treasures uses icon `420 + N` (421 = needs 1 treasure, 422 = needs 2, etc.), distinct from the
key-gated 334-336 doors. `OpenDoorsTresor` scans the whole 100x100 `m_decor` grid and calls
`OpenDoor` on every tile whose icon falls in `421 .. 421 + m_nbTresor - 1`, i.e. every treasure door
whose requirement is now met by the player's current treasure count. It is invoked once whenever a
treasure item's pickup voyage animation completes (icon 6, `Decor.cpp` ~10270-10274, which also
increments `m_nbTresor`), so newly-qualifying treasure doors across the whole level open together
at that moment, not just the nearest one.

## Win/lose door effects

- **`Decor::OpenDoorsWin`** (`Decor.cpp` ~11695): called on a normal sublevel win when the player
  has not found the level's hidden key (`!m_bFoundCle`, see the win-dispatch at `Decor.cpp`
  ~6411-6433). It only sets the persistent flag `m_doors[m_mission + 1] = 1`
  unlocking the next sublevel; it does not animate or touch any door tile in the current level.
- **`Decor::OpenGoldsWin`** (`Decor.cpp` ~11707): called instead when the player *did* find the
  hidden key/gold (`m_bFoundCle` true) on win. It sets `m_doors[180 + m_mission / 10] = 1`, the
  world's gold-collected flag at the hub's conventional offset (180+worldIndex) — this is what
  later makes `AdaptDoors` animate that world's gold icon rising on the hub screen.
- **`Decor::DoorsLost`** (`Decor.cpp` ~11716): despite the name, it does not touch `m_doors` or any
  door tile at all — losing a life simply resets `m_nbVies` (life count) back to the default 3.
  Door/key/treasure progress earned so far in the attempt is unaffected by this call; it only
  resets lives. Called both on Blupi's death animation completing and on falling out of the level
  bounds (`Decor.cpp` ~6397, ~6409).

## `Decor::AdaptDoors(bool bPrivate)` (`Decor.cpp` ~11533)

Applies the persisted `m_doors[]` flags (and the `m_bCheatDoors` open-doors cheat) to the current
level's actual door/gate tiles. Behavior forks on `bPrivate` and on `m_mission`:

- If `bPrivate` is true (a user/private level, not part of the campaign), the function returns
  immediately and does nothing — private levels have no progression gating, so every gate is
  treated as already open by whatever authored the level itself.
- Otherwise, on the hub (`m_mission == 1`): for each of 20 worlds, if that world's gold flag
  (`m_doors[180 + i] == 1`) is set (or the cheat is active), the hub's gold-pickup icon (183) for
  that world is removed and replaced with a rising `ObjectType22` animation (identical slide-style
  motion to a door opening, one tile up, `SoundChannel33`) to visually reveal it as collected. It
  then scans the whole grid and swaps icons 158-165 to +8 and 410-415 to +5 (world-entry/bonus
  tile "unlocked" variants) wherever the matching `m_doors[180 + offset]` flag or the cheat is set,
  and swaps icon 309 to 310 when `m_doors[189]` is set — these are the hub's visual
  locked/unlocked tile-icon pairs.
- Otherwise, on a world's entry screen (`m_mission % 10 == 0`; any other mission number returns
  immediately with no effect): for each of 10 possible sublevel doors, `SearchDoor` locates the
  door-sign/door-tile pair for that sublevel number; if the sublevel's flag
  (`m_doors[m_mission + i] == 1`) is set (or the cheat is active), `OpenDoor` is called on it and
  Blupi's start position/facing is snapped to stand beside that now-open door **facing towards
  it** (`Decor.cpp` ~11610-11617: `Direction::Right` when Blupi's placement cell is to the door's
  left, `Direction::Left` when it's to the door's right — he always ends up looking at the door,
  not away from it).

## `Decor::SearchDoor(int n, TinyPoint& cel, TinyPoint& blupi)` (`Decor.cpp` ~11439)

Used by `AdaptDoors` during world-entry-screen transitions to locate a specific numbered sublevel
door. World-door **sign** icons 174-181 encode door numbers 1-8 (`icon - 174 + 1 == n`); the
function scans the grid for a sign matching `n`, then looks up to two cells to either side
(nearer cell preferred) for the actual door tile (icon 182) adjacent to that sign. It returns both
the door cell (`cel`) and the cell just beyond the door where Blupi should be placed
(`blupi`, offset one tile further out plus a small `BLUPIOFFY` vertical offset) — this is purely a
lookup, it does not itself open anything or move Blupi; `AdaptDoors` uses the result to call
`OpenDoor` and set `m_blupiStartPos`/`m_blupiStartDir`.

## Persistence — `InitializeDoors`/`MemorizeDoors` (`Decor.cpp` ~1701/~1719)

`InitializeDoors` loads the 200-entry `m_doors[]` array from `GameData::GetDoors`, and
`MemorizeDoors` writes it back via `GameData::SetDoors`; both are thin wrappers with only a debug
log line beyond the copy. The byte-level save-file mechanics for how `GameData` itself persists
this array to disk are documented in `11-save-and-progression.md` — not duplicated here.

Door icon crops (see `02-tiles.md`'s image-generation note):

![Door1](images/tile-334-Door1.png) ![Door2](images/tile-335-Door2.png) ![Door3](images/tile-336-Door3.png)

**`DOC-234` (2026-07-04):** re-verified these 3 crops against the corrected `object-m.png` grid
formula. All 3 had the `DOC-230`/`S3D-2` leading-margin bug (confirmed pixel-exact against the old
`x=col*65, y=row*65` formula) and were regenerated with the corrected `x=1+col*65, y=1+row*65`.

## Open question for the 3D mapping (not decided here)

The user wants doors in the 3D target rendered as **billboards with transparency** rather than
opaque textured cubes — presumably so a door can visually read as a thin barrier/frame rather than
a solid block, and/or so the slide-up-and-vanish animation translates naturally to a billboard
sliding along Y. This document does not resolve how (a new `BlockMetadata`-tagged block? a distinct
object type layered over an `Air` cell? something else) — that is exactly the kind of decision this
catalog exists to inform, not make. Tracked in `09-open-questions.md`.
